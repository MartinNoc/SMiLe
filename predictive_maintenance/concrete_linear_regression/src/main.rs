use concrete::*;
use csv::*;
use std::time::Instant;
use std::io::{self, Write};

const HISTORY_LENGTH: usize = 32; // past features used for prediction

const PRECISION: usize = 6; // only ~7 bits remain uncontaminated after each bootstrap
const PADDING: usize = 11; // 6 used for constant multiplication, 5 for cascaded addition

const LOWER: f64 = 17.; // limits of the input data
const UPPER: f64 = 35.;

// 128 bit security parameters from zama whitepaper
// ~700 seconds for key generation, ~36 seconds for loading pre-generated keys
// ~1.1GB combined key size
// ~7.9 seconds per prediction
const LWE_DIMENSION: usize = 938;
const LWE_NOISE: i32 = -23;
const RLWE_SIZE: usize = 4096;
const RLWE_NOISE: i32 = -62;
const BASE_LOG: usize = 6;
const LEVEL: usize = 4;

/*
// 128 bit security parameters from concrete presets
// precision loss during bootstrap! leading to unstable result
// ~170 seconds for generating keys
// ~0.5GB combined key size
// ~3.9 seconds per prediction
const LWE_DIMENSION: usize = 830;
const LWE_NOISE: i32 = -20;
const RLWE_SIZE: usize = 2048;
const RLWE_NOISE: i32 = -52;
const BASE_LOG: usize = 6;
const LEVEL: usize = 4;
*/

const EMA_FACTOR: f64 = 0.7;
const TOOL_CHANGE_THRESHOLD: f64 = 0.2;

fn he_generate_keys() -> anyhow::Result<(LWESecretKey, LWESecretKey, LWEBSK, LWEKSK)>
{
    /* 
        generate HE keys and save to files    
    */
    
    let now = Instant::now();
    print!("Generating keys...");
    io::stdout().flush().unwrap();
    
    let rlwe_params = RLWEParams {dimension: 1, polynomial_size: RLWE_SIZE, log2_std_dev: RLWE_NOISE};
    let lwe_params = LWEParams {dimension: LWE_DIMENSION, log2_std_dev: LWE_NOISE};
    
    let sk_rlwe = RLWESecretKey::new(&rlwe_params);
    let sk_in = LWESecretKey::new(&lwe_params);
    let sk_out = sk_rlwe.to_lwe_secret_key();
    
    // create bootstrapping key
    let bsk = LWEBSK::new(&sk_in, &sk_rlwe, BASE_LOG, LEVEL);
    
    // create key switching key
    let ksk = LWEKSK::new(&sk_out, &sk_in, BASE_LOG, LEVEL);
    
    // save generated keys
    sk_in.save("sk_in.json").unwrap();
    sk_out.save("sk_out.json").unwrap();
    bsk.save("bsk.json");
    ksk.save("ksk.json");
    
    println!(" Took {} seconds", now.elapsed().as_secs_f64());
    
    Ok((sk_in, sk_out, bsk, ksk))
}

fn he_load_keys() -> anyhow::Result<(LWESecretKey, LWESecretKey, LWEBSK, LWEKSK)>
{
    /*
        load HE keys from files
    */
    
    let now = Instant::now();
    print!("Loading keys...");
    io::stdout().flush().unwrap();
    
    let sk_in = LWESecretKey::load("sk_in.json").unwrap();
    let sk_out = LWESecretKey::load("sk_out.json").unwrap();
    let bsk = LWEBSK::load("bsk.json");
    let ksk = LWEKSK::load("ksk.json");
    
    println!(" Took {} seconds", now.elapsed().as_secs_f64());
    
    Ok((sk_in, sk_out, bsk, ksk))
}

fn cascade_additions(buf: &mut Vec<LWE>, rounds: u32)
{
    /*
        perform a cascading addition scheme on a buffer of ciphertexts
    */
    
	let mut ops = HISTORY_LENGTH >> 1;
	let mut step = 2;
	let mut off = 1;
    for _j in 0..rounds {
        for k in 0..ops {
            let idx = k * step;
            buf[idx] = buf[idx].add_with_padding(&buf[idx + off]).unwrap();
        }
	    ops = ops >> 1;
	    step = step << 1;
	    off = off << 1;
	}
}

fn test_cipher_threshold(c: &LWE, threshold: f64, padding: usize, bsk: &LWEBSK, ksk: &LWEKSK)
    -> anyhow::Result<(LWE, LWE)>
{
    /* 
        test if ciphertext c is greater or equal than the threshold
        return encrypted control one-hot encoded as (true, false)
    */
    
    let control_encoder = Encoder::new(0., UPPER - LOWER, PRECISION, padding)?;
    let mut t = c.bootstrap_with_function(&bsk, |x| if x - threshold >= 0. {UPPER - LOWER} else {0.}, &control_encoder)?;
    let mut f = c.bootstrap_with_function(&bsk, |x| if x - threshold < 0. {UPPER - LOWER} else {0.}, &control_encoder)?;
    t = t.keyswitch(&ksk)?;
    f = f.keyswitch(&ksk)?;
    
    Ok((t, f))
}

fn apply_control(a: &LWE, c: &LWE, padding: usize, bsk: &LWEBSK, ksk: &LWEKSK)
    -> anyhow::Result<LWE>
{
    /*
        apply encrypted control c to ciphertext a
        return encryption of 0 or a, depending on the state of the control
    */
    
    let mut r = a.add_with_padding_exact(&c)?;

    let result_encoder = Encoder::new(0., UPPER - LOWER, PRECISION, padding)?;
    r = r.bootstrap_with_function(&bsk, |x| f64::max(0., x - (UPPER - LOWER)), &result_encoder)?;
    r = r.keyswitch(&ksk)?;

    Ok(r)
}

fn predict_target(c: &LWE, c_buf: &Vec<LWE>, ema: &mut LWE, previous_max: &mut LWE, bsk: &LWEBSK, ksk: &LWEKSK)
    -> anyhow::Result<LWE>
{
    /*
        homomorphically predict height of next target, depending on previous features
        return target
    */
    
    let bootstrap_encoder = Encoder::new(0., UPPER - LOWER, PRECISION, 2)?;
    
    if c_buf.len() == 0 {
        // initialise ema and previous max to 0 via bootstrap
        *ema = c.bootstrap_with_function(&bsk, |_x| 0., &bootstrap_encoder)?;
        *ema = ema.keyswitch(&ksk)?;
        *previous_max = ema.clone();
    }
    else {
        // get copy of c with less padding
        let mut c_stripped = c.clone();
        c_stripped.remove_padding_inplace(PADDING - 2).unwrap();
        
        // determine if tool change took place
        let change_to_last = c_buf.last().unwrap().sub_with_padding(&c)?;                
        let (yes_tool_change, no_tool_change) =
            test_cipher_threshold(&change_to_last, TOOL_CHANGE_THRESHOLD * (UPPER - LOWER), 2, &bsk, &ksk)?;
        
        // determine if c is the new maximum
        let diff_from_max = c_stripped.sub_with_padding_exact(&previous_max)?;
        let (yes_new_max, no_new_max) = 
            test_cipher_threshold(&diff_from_max, 0., 2, &bsk, &ksk)?;
        
        // multiply EMA factors via bootstrap to get contributions to new EMA
        let mut old_contribution = ema.bootstrap_with_function(&bsk, |x| x * (1. - EMA_FACTOR), &bootstrap_encoder)?;
        let mut new_contribution = previous_max.bootstrap_with_function(&bsk, |x| x * EMA_FACTOR, &bootstrap_encoder)?;
        old_contribution = old_contribution.keyswitch(&ksk)?;
        new_contribution = new_contribution.keyswitch(&ksk)?;
        
        // update exponential moving average
        let mut ema_update = old_contribution.add_with_new_min(&new_contribution, 0.)?;
        ema_update = apply_control(&ema_update, &yes_tool_change, 2, &bsk, &ksk)?;
        let ema_residue = apply_control(&ema, &no_tool_change, 2, &bsk, &ksk)?;
        *ema = ema_residue.add_with_new_min(&ema_update, 0.)?;
        
        // update previous max
        let max_update = apply_control(&c_stripped, &yes_new_max, 2, &bsk, &ksk)?;
        let max_residue = apply_control(&previous_max, &no_new_max, 2, &bsk, &ksk)?;
        let mut new_max_candidate = max_residue.add_with_new_min(&max_update, 0.)?;
        new_max_candidate = apply_control(&new_max_candidate, &no_tool_change, 2, &bsk, &ksk)?;
        let toolchange_override = apply_control(&c_stripped, &yes_tool_change, 2, &bsk, &ksk)?;
        *previous_max = toolchange_override.add_with_new_min(&new_max_candidate, 0.)?;
    }
    
    Ok(ema.clone())
}

fn perform_analytic_lin_regression(c_buf: &Vec<LWE>) 
    -> anyhow::Result<(LWE, LWE)>
{
    /*
        homomorphic analytic linear regression for y=a*x+b on buffer of ciphertexts
        calculates  a = (E[x²]E[y] - E[x]E[xy]) / Var[x]  and  b = Cov[x,y] / Var[x]
        this can be done as two sums over the ciphertexts, weighted by some plaintext factors
        x is taken to be equidistant in interval [0, 1]
        return a and b
    */
    
    if c_buf.len() < HISTORY_LENGTH {
        return Ok((LWE::zero(LWE_DIMENSION).unwrap(),
            LWE::zero(LWE_DIMENSION).unwrap()));
    }

    // create sum buffers
    let mut a_buf: Vec<LWE> = Vec::with_capacity(HISTORY_LENGTH);
    let mut b_buf: Vec<LWE> = Vec::with_capacity(HISTORY_LENGTH);
    
    // calculate plaintext factors
    let mut a_fac: Vec<f64> = Vec::with_capacity(c_buf.len());
    let mut b_fac: Vec<f64> = Vec::with_capacity(c_buf.len());
    let n = HISTORY_LENGTH as f64;
    
    let mut xsum = 0.0;
    let mut xsqsum = 0.0;
    
    for i in 0..HISTORY_LENGTH {
        let x = i as f64 / n;
        
        xsum += x;
        xsqsum += x * x;
    }
    
    let denom = xsqsum * n - xsum * xsum;
    let mut a_fac_max: f64 = 0.;
    let mut b_fac_max: f64 = 0.;
    
    for i in 0..HISTORY_LENGTH {
        let x = i as f64 / n;
        
        a_fac.push((n * x - xsum) / denom);
        b_fac.push((xsqsum - xsum * x) / denom);
        
        a_fac_max = a_fac_max.max(a_fac[i].abs());
        b_fac_max = b_fac_max.max(b_fac[i].abs());
    }
    
    // multiply factors to ciphertexts
    for i in 0..HISTORY_LENGTH {
        let a_term = c_buf[i].mul_constant_with_padding(a_fac[i], a_fac_max, 6)?;
        let b_term = c_buf[i].mul_constant_with_padding(b_fac[i], b_fac_max, 6)?;

        a_buf.push(a_term);
        b_buf.push(b_term);
    }
    
    // sum up buffers
    cascade_additions(&mut a_buf, 5);
    cascade_additions(&mut b_buf, 5);
    
    Ok((a_buf[0].clone(), b_buf[0].clone()))
}

fn main() {
    // setup buffers, files, variables, encoders
    let mut c_buf: Vec<LWE> = Vec::with_capacity(HISTORY_LENGTH);
    let mut result_buf: Vec<(f64, f64, f64)> = Vec::with_capacity(700);
    
    let mut rdr = ReaderBuilder::new()
        .has_headers(false)
        .from_path("../sensitive_data/10_features.csv").unwrap();
    
    let mut ema: LWE = LWE::zero(LWE_DIMENSION).unwrap();
    let mut previous_max: LWE = LWE::zero(LWE_DIMENSION).unwrap();
    
    let feature_encoder = Encoder::new(0., UPPER - LOWER, PRECISION, PADDING).unwrap();
    
    // generate keys (load instead in subsequent runs to save time)
    let (sk_in, _sk_out, bsk, ksk) = he_generate_keys().unwrap();
    //let (sk_in, _sk_out, bsk, ksk) = he_load_keys().unwrap();
    
    // loop over features
    let mut counter = 0;
    for result in rdr.records() {
        let now = Instant::now();
        print!("Working on datapoint {} of {}...", counter + 1, 700);
        io::stdout().flush().unwrap();
        counter += 1;
    
        // pre-process
        let record = result.unwrap();
        let feature = record.get(0).unwrap().parse::<f64>().unwrap();
        let c = LWE::encode_encrypt(&sk_in, feature - LOWER, &feature_encoder).unwrap();
        
        // homomorphic action
        let t_enc = predict_target(&c, &c_buf, &mut ema, &mut previous_max, &bsk, &ksk).unwrap();
        c_buf.push(c);
        let (a_enc, b_enc) = perform_analytic_lin_regression(&c_buf).unwrap();
        
        // post-process
        if c_buf.len() == HISTORY_LENGTH {
            let a = a_enc.decrypt_decode(&sk_in).unwrap();
            let b = b_enc.decrypt_decode(&sk_in).unwrap();
            let t = t_enc.decrypt_decode(&sk_in).unwrap();
       
            result_buf.push((a, b, t));
            c_buf.remove(0);
        }
        
        println!(" Took {} seconds", now.elapsed().as_secs_f64());
    }
    
    // write predictions into .csv
    let mut wtr = csv::Writer::from_path("../sensitive_data/10_predictions.csv").unwrap();
    
    for i in result_buf {
        wtr.write_record(&[i.0.to_string(), i.1.to_string(), i.2.to_string()]).unwrap();
    }
    
    wtr.flush().unwrap();
}
