use concrete::*;
//use csv::*;
//use std::fs::File;

const MIN_ELEMENTS: usize = 10;
const MAX_ELEMENTS: usize = 32; // power of two

const PRECISION: usize = 6; // precision after bootstrapping ~7 bits

const UPPER: f64 = -4.; // limits of the input data
const LOWER: f64 = -9.;

fn he_generate_keys() -> anyhow::Result<(LWESecretKey, LWEBSK, LWEKSK)>
{
    let rlwe_params = RLWEParams {dimension: 1, polynomial_size: 2048, log2_std_dev: -82};
    let lwe_params = LWEParams {dimension: 592, log2_std_dev: -23};
    
    let sk_rlwe = RLWESecretKey::new(&rlwe_params);
    let sk_in = LWESecretKey::new(&lwe_params);
    let sk_out = sk_rlwe.to_lwe_secret_key();
    
    // create bootstrapping key
    let base_log: usize = 6;
    let level: usize = 4;
    let bsk = LWEBSK::new(&sk_in, &sk_rlwe, base_log, level);
    
    // create key switching key
    let ksk = LWEKSK::new(&sk_out, &sk_in, base_log, level);
    
    // save generated keys
    sk_in.save("sk_in.json").unwrap();
    bsk.save("bsk.json");
    ksk.save("ksk.json");
    
    Ok((sk_in, bsk, ksk))
}

fn he_load_keys() -> anyhow::Result<(LWESecretKey, LWEBSK, LWEKSK)>
{
    let sk_in = LWESecretKey::load("sk_in.json").unwrap();
    let bsk = LWEBSK::load("bsk.json");
    let ksk = LWEKSK::load("ksk.json");
    
    Ok((sk_in, bsk, ksk))
}

fn cascade_additions(buf: &mut Vec<LWE>, level: u32, rounds: u32)
{
	let mut ops = (MAX_ELEMENTS >> 1) >> level;
	let mut step = 2 << level;
	let mut off = 1 << level;
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

/*fn refresh_padding(buf: &mut Vec<LWE>, level: u32, bsk: &LWEBSK, ksk: &LWEKSK)
{
    let rounds = MAX_ELEMENTS >> level;
    for i in 0..rounds {
        let idx = i * (1 << level);
        let encoder_in = &buf[idx].encoder;
        let encoder_out = Encoder::new(encoder_in.get_min(), encoder_in.get_max(), PRECISION, PADDING).unwrap();
        buf[idx] = buf[idx].bootstrap_with_function(&bsk, |x| x, &encoder_out).unwrap();
        buf[idx] = buf[idx].keyswitch(&ksk).unwrap();
    }
}*/

fn predict_analytic(c_buf: &Vec<LWE>, sk_in: &LWESecretKey) 
    -> anyhow::Result<(f64, f64)>
{
    if c_buf.len() < MIN_ELEMENTS {
        return Ok((0.0, 0.0));
    }

    // create sum buffers, fill with zeros
    let mut a_buf: Vec<LWE> = Vec::with_capacity(MAX_ELEMENTS);
    let mut b_buf: Vec<LWE> = Vec::with_capacity(MAX_ELEMENTS);
    
    // calculate factors
    let mut a_fac: Vec<f64> = Vec::with_capacity(c_buf.len());
    let mut b_fac: Vec<f64> = Vec::with_capacity(c_buf.len());
    let mut xsum = 0.0;
    let mut xsqsum = 0.0;
    let n = c_buf.len() as f64;
    for i in 0..c_buf.len() {
        let fac = i as f64 / n;
        xsum += fac;
        xsqsum += fac*fac;
    }
    let denom = xsqsum * n - xsum * xsum;
    let mut a_fac_max: f64 = 0.;
    let mut b_fac_max: f64 = 0.;
    for i in 0..c_buf.len() {
        let fac = i as f64 / n;
        a_fac.push((n * fac - xsum) / denom);
        b_fac.push((xsqsum - xsum * fac) / denom);
        
        a_fac_max = a_fac_max.max(a_fac[i].abs());
        b_fac_max = b_fac_max.max(b_fac[i].abs());
    }
    
    // calculate sum terms
    for i in 0..c_buf.len() {
        let a_term = c_buf[i].mul_constant_with_padding(a_fac[i], a_fac_max, 6)?;
        let b_term = c_buf[i].mul_constant_with_padding(b_fac[i], b_fac_max, 6)?;

        a_buf.push(a_term);
        b_buf.push(b_term);
    }
    
    // cascade sum for a
    cascade_additions(&mut a_buf, 0, 5);
    
    // cascade sum for b
    cascade_additions(&mut b_buf, 0, 5);
    
    // debug prints
    println!("X={}, X2={}", xsum, xsqsum);
    println!("Denom={}", denom);
    
    // decrypt to check result
    let a_clear = a_buf[0].decrypt_decode(&sk_in).unwrap();
    let b_clear = b_buf[0].decrypt_decode(&sk_in).unwrap();
    
    Ok((a_clear, b_clear))
}

/*fn predict_descent(c_buf: &Vec<LWE>, sk_in: &LWESecretKey, bsk: &LWEBSK, ksk: &LWEKSK)
{
    let alpha = 0.1;
    let iter_max = 5;
    
    let encoder = Encoder::new(0.0, UPPER-LOWER, PRECISION, 9).unwrap();
    
    let mut a = LWE::encode_encrypt(&sk_in, 0.0, encoder)?;
    let mut b = LWE::encode_encrypt(&sk_in, 0.0, encoder)?;
    
    let mut p_buf: Vec<LWE> = Vec::with_capacity(MAX_ELEMENTS);
    let mut a_buf: Vec<LWE> = Vec::with_capacity(MAX_ELEMENTS);
    let mut b_buf: Vec<LWE> = Vec::with_capacity(MAX_ELEMENTS);
    for i in 0..iter_max {
        // make predictions and calculate terms
        for j in 0..c_buf.len() {
            let x = i as f64 / c_buf.len();
            p_buf.push(a.mul_constant_with_padding(x, 1., 6));
            p_buf[i].
            a_buf.push(c_buf.mul_constant_with_padding(-2.*x/c_buf.len(), 2./c_buf.len(), 6));
            b_buf.push(c_buf.mul_constant_with_padding(-2.*x/c_buf.len(), 2./c_buf.len(), 6));
            
            
        }
    }
}*/

fn fill_with_example(c_buf: &mut Vec<LWE>, sk_in: &LWESecretKey)
{
    let encoder = Encoder::new(0.0, UPPER-LOWER, PRECISION, 11).unwrap();
    
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.0190652, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.129837, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.432281, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.377101, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.483336, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.637487, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.553456, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.609443, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.627642, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.728216, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.785635, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.818216, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.75084, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.94247, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.698027, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.731775, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.856312, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 1.01139, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.807444, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.791304, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.964357, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 1.08082, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 1.09392, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 1.05232, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 1.12343, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 1.22764, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.944031, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 0.985583, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 1.32213, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 1.30813, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 1.43636, &encoder).unwrap());
    c_buf.push(LWE::encode_encrypt(&sk_in, 1.37614, &encoder).unwrap());
}

fn main() {
    let mut c_buf: Vec<LWE> = Vec::with_capacity(MAX_ELEMENTS);
    
    // prefer to load keys once generated
    //let (sk_in, bsk, ksk) = he_generate_keys().unwrap();
    let (sk_in, _bsk, _ksk) = he_load_keys().unwrap();
    
    fill_with_example(&mut c_buf, &sk_in);
    let result = predict_analytic(&c_buf, &sk_in).unwrap();

    println!("Predicted line: {}x + {}", result.0, result.1);
}
