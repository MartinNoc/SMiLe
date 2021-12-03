use concrete::*;
use tch::Tensor;

fn test_he() -> anyhow::Result<()> {
    // generate a secret key
    let secret_key = LWESecretKey::new(&LWE128_630);

    // the two values to add
    let m1 = 8.2;
    let m2 = 5.6;

    // encode in [0, 10[ with 8 bits of precision and 1 bit of padding
    let encoder = Encoder::new(0., 10., 8, 1)?;

    // encrypt plaintexts
    let mut c1 = LWE::encode_encrypt(&secret_key, m1, &encoder)?;
    let c2 = LWE::encode_encrypt(&secret_key, m2, &encoder)?;

    // add the two ciphertexts homomorphically, and store in c1
    c1.add_with_padding_inplace(&c2)?;

    // decrypt and decode the result
    let m3 = c1.decrypt_decode(&secret_key)?;

    // print the result and compare to non-FHE addition
    println!("Expected: {}, Result: {}", m1 + m2, m3);
    Ok(())
}

fn load_model(path: &str) -> anyhow::Result<tch::CModule> {
    // load TorchScript file from specified path and return as tch model

    let model_file = std::env::current_dir()?
        .join(path);
    let model = tch::CModule::load(model_file)?;
    Ok(model)
}

fn get_example_input() -> anyhow::Result<Tensor> {
   // return hardcoded example input, should lead to output 0.1934
   
   let example: [f32; 16] = [
        0.7216, 0.3540, 0.8186, 0.0568,
        0.2227, 0.4065, 0.1240, 0.8263,
        0.8697, 0.7265, 0.1013, 0.7650,
        0.2213, 0.1780, 0.2303, 0.0906];
    let mut example_tensor = Tensor::of_slice(&example);
    example_tensor = example_tensor.reshape(&[1, 16]);
    
    Ok(example_tensor)
}

fn test_nn() -> anyhow::Result<()> {
    // load model
    let model = load_model("data/traced_simple_nn.pt")?;
    
    // define example input and wrap into tensor
    let example_input = get_example_input()?;
    
    // perform inference on example
    let output = model.forward_ts(&[example_input])?;
    
    // print result compared to expected target
    println!("Expected: {}, Result: {}", 0.1934, output.double_value(&[0]));
    
    Ok(())
}

fn extract_model_parameters(model: &tch::CModule) -> anyhow::Result<([f32; 128], [f32; 8], [f32; 8], [f32;1])> {
    // copy parameters from given model into arrays
    
    // get model parameters
    let params = model.named_parameters().unwrap();
    
    // define arrays
    let mut w0: [f32; 128] = [0.0; 128];
    let mut b0: [f32; 8] = [0.0; 8];
    let mut w1: [f32; 8] = [0.0; 8];
    let mut b1: [f32; 1] = [0.0; 1];
    
    // copy
    params[0].1.copy_data(&mut w0, 128);
    params[1].1.copy_data(&mut b0, 8);
    params[2].1.copy_data(&mut w1, 8);
    params[3].1.copy_data(&mut b1, 1);
    
    Ok((w0, b0, w1, b1))
}

fn manual_forward_pass(model: &tch::CModule, input: &Tensor) -> anyhow::Result<f32> {
    // perform forward pass operations manually
    
    // get model parameters
    let (w0, b0, w1, b1) = extract_model_parameters(model).unwrap();
    
    // copy input into rust array
    let mut l0_input: [f32; 16] = [0.0; 16];
    input.copy_data(&mut l0_input, 16);
    
    // prepare result array
    let mut l1_input: [f32; 8] = [0.0; 8];
    let mut output: f32 = 0.0;
    
    // input -> hidden
    for i in 0..8 {
        for j in 0..16 {
            l1_input[i] += l0_input[j] * w0[i * 16 + j];
        }
        l1_input[i] = l1_input[i] + b0[i];
        l1_input[i] = l1_input[i].max(0.0);
    }

    // hidden -> output
    for i in 0..8 {
        output += l1_input[i] * w1[i];
    }
    output += b1[0];
    
    Ok(output)
}

fn inspect_encoder(c: &VectorLWE) {
    // print encoder properties for given vector of encrypted values
    for en in &c.encoders{
        println!("  Offset: {}, Delta: {}, Precision: {}, Padding: {}", en.o, en.delta, en.nb_bit_precision, en.nb_bit_padding);
        break; // actually, we probably only really need to see the first one :)
    }
}

fn he_forward_pass(model: &tch::CModule, input: &Tensor) -> anyhow::Result<f64> {
    // perform a homomorphically encrypted forward pass
    
    // get model parameters
    let (w0, b0, w1, b1) = extract_model_parameters(model).unwrap();
    
    // convert to f64 for concrete
    let w0: [f64; 128] = w0.map(|x| x as f64);
    let b0: [f64; 8] = b0.map(|x| x as f64);
    let w1: [f64; 8] = w1.map(|x| x as f64);
    let b1: [f64; 1] = b1.map(|x| x as f64);
    
    // copy input into rust array
    let mut l0_input: [f32; 16] = [0.0; 16];
    input.copy_data(&mut l0_input, 16);
    let l0_input: [f64; 16] = l0_input.map(|x| x as f64);
    
    /*
    // key generation takes a while! Load the saved keys instead of regenerating after running once
    // create secret keys
    let rlwe_params = RLWEParams {dimension: 1, polynomial_size: 2048, log2_std_dev: -82};
    let lwe_params = LWEParams {dimension: 592, log2_std_dev: -23};
    
    let sk_rlwe = RLWESecretKey::new(&rlwe_params);
    let sk_in = LWESecretKey::new(&lwe_params);
    let sk_out = sk_rlwe.to_lwe_secret_key();
    
    // create bootstrapping key
    let base_log: usize = 4;
    let level: usize = 6;
    let bsk = LWEBSK::new(&sk_in, &sk_rlwe, base_log, level);
    
    // create key switching key
    let ksk = LWEKSK::new(&sk_out, &sk_in, base_log, level);
    
    // save generated keys
    sk_in.save("sk_in.json")?;
    bsk.save("bsk.json");
    ksk.save("ksk.json");
    */
    
    // load keys that were previously generated
    let sk_in = LWESecretKey::load("sk_in.json").unwrap();
    let bsk = LWEBSK::load("bsk.json");
    let ksk = LWEKSK::load("ksk.json");
    
    
    // create input encoder
    let min = 0.0;
    let max = 1.0;
    let precision = 4;
    let padding = 3;
    let encoder = Encoder::new(min, max, precision, padding)?;
    
    // encrypt input
    let c = VectorLWE::encode_encrypt(&sk_in, &l0_input, &encoder)?;
    println!("Initial encoder properties:");
    inspect_encoder(&c);
    
    println!("Multiplying hidden layer weights by functional bootstrap");
    let mut c128: Vec<VectorLWE> = Vec::with_capacity(128);
    for i in 0..8 {
        for j in 0..16 {
            // choose encoder interval to be tight around the values
            let mut lower = 0.0;
            let mut upper = max * 0.5;
            // if we multiply with a negative weight the interval is flipped
            if w0[i*16 + j] < 0.0 {
               lower = -upper;
               upper = 0.0;
            }
            
            let encoder_mult = Encoder::new(lower, upper, precision, padding)?;
            c128.push(c.bootstrap_nth_with_function(&bsk, |x| x * w0[i*16 + j], &encoder_mult, j).unwrap());
            c128[i * 16 + j] = c128[i * 16 + j].keyswitch(&ksk).unwrap();
        }
    }
    /*
    // debug print to see cleartext values and encoder properties
    println!("Post-multiplication encoder properties:");
    inspect_encoder(&c128[0]);
    let mut control: [f64; 8] = [0.0; 8];
    for i in 0..8 {
        control[i] = (c128[i*16].decrypt_decode(&sk_in)?)[0];
    }
    println!("Multiplication output: {:?}", control);
    */
    
    
    println!("Adding up hidden layer, first round");
    for i in 0..8 {
        // Perform additions in a pyramid pattern to use padding optimally
    	let mut ops = 8;
    	let mut step = 2;
    	let mut off = 1;
        for _j in 0..2 {
            for k in 0..ops {
                let idx = i * 16 + k * step;
                c128[idx] = c128[idx].add_with_padding(&c128[idx + off])?;
            }
    	    ops /= 2;
    	    step *= 2;
    	    off *= 2;
    	}
    }
    /*
    // debug print to see cleartext values and encoder properties
    println!("Post-first-addition encoder properties:");
    inspect_encoder(&c128[0]);
    let mut control: [f64; 8] = [0.0; 8];
    for i in 0..8 {
        control[i] = (c128[i*16].decrypt_decode(&sk_in)?)[0];
    }
    println!("Addition output: {:?}", control);
    */
    
    
    println!("Performing recovery bootstrap to free padding bits");
    for i in 0..32 {
        let encoder_old = &c128[0].encoders[0];
        let encoder_out = Encoder::new(encoder_old.get_min(), encoder_old.get_max(), precision, padding)?;
        c128[i * 4] = c128[i * 4].bootstrap_nth_with_function(&bsk, |x| x, &encoder_out, 0).unwrap();
        c128[i * 4] = c128[i * 4].keyswitch(&ksk).unwrap();
    }
    /*
    // debug print to see cleartext values and encoder properties
    println!("Post-recovery encoder properties:");
    inspect_encoder(&c128[0]);
    let mut control: [f64; 8] = [0.0; 8];
    for i in 0..8 {
        control[i] = (c128[i*16].decrypt_decode(&sk_in)?)[0];
    }
    println!("Recovery output: {:?}", control);
    */
    
    
    // Second round of additions of input layer
    println!("Adding up hidden layer, second round");
    for i in 0..8 {
        // Continue pyramid where we left off
    	let mut ops = 2;
    	let mut step = 8;
    	let mut off = 4;
        for _j in 0..2 {
            for k in 0..ops {
                let idx = i * 16 + k * step;
                c128[idx] = c128[idx].add_with_padding(&c128[idx + off])?;
            }
    	    ops /= 2;
    	    step *= 2;
    	    off *= 2;
    	}
    }
    /*
    // debug print to see cleartext values and encoder properties
    println!("Post-second-addition encoder properties:");
    inspect_encoder(&c128[0]);
    let mut control: [f64; 8] = [0.0; 8];
    for i in 0..8 {
        control[i] = (c128[i*16].decrypt_decode(&sk_in)?)[0];
    }
    println!("Addition output: {:?}", control);
    */
    
    
    println!("Performing functional bootstrap to add bias, apply ReLU, and multiply output layer weights");
    let mut c8: Vec<VectorLWE> = Vec::with_capacity(8);
    for i in 0..8 {
        // Again we choose tight encoder interval and flip for negative weights
        let mut lower = 0.0;
        let mut upper = 0.15;
        if w1[i] < 0.0 {
            lower = -upper;
            upper = 0.0;
        }
        let encoder_out = Encoder::new(lower, upper, precision, padding)?;
        c8.push(c128[i * 16].bootstrap_nth_with_function(&bsk, |x| w1[i] * f64::max(0.0, x + b0[i]), &encoder_out, 0).unwrap());
        c8[i] = c8[i].keyswitch(&ksk).unwrap();
    }
    /*
    // debug print to see cleartext values and encoder properties
    println!("Post-ReLU encoder properties:");
    inspect_encoder(&c8[0]);
    let mut control: [f64; 8] = [0.0; 8];
    for i in 0..8 {
        control[i] = (c8[i].decrypt_decode(&sk_in)?)[0];
    }
    println!("Hidden layer output: {:?}", control);
    */
    
    
    println!("Adding up output layer");
    // Another pyramid pattern, this time we can do it in one go and use up all padding
    let mut ops = 4;
    let mut step = 2;
    let mut off = 1;
    for _j in 0..3 {
        for k in 0..ops {
            let idx = k * step;
            c8[idx] = c8[idx].add_with_padding(&c8[idx + off])?;
        }
        ops /= 2;
        step *= 2;
        off *= 2;
    }
    
    // Decode output and add bias
    let unbiased_output = (c8[0].decrypt_decode(&sk_in)?)[0];
    Ok(unbiased_output + b1[0])
}

fn main() -> anyhow::Result<()> {
    // run some simple tests
    println!("\nHomomorphic encryption test:");
    test_he().expect("Error in homomorphic encryption test!");
    println!("\nNeural network test:");
    test_nn().expect("Error in neural network test!");
    
    // perform a forward pass fully manually
    let model = load_model("data/traced_simple_nn.pt")?;
    let example_input = get_example_input()?;
    println!("\nManual forward pass:");
    let manual_output = manual_forward_pass(&model, &example_input).expect("Error in manual forward pass!");
    println!("Expected: {}, Result: {}", 0.1934, manual_output);
    
    // perform a forward pass homomorphically encrypted
    println!("\nHomomorphic forward pass:");
    let homomorphic_output = he_forward_pass(&model, &example_input).expect("Error in homomorphic forward pass!");
    println!("Expected: {}, Result: {}", manual_output, homomorphic_output);
    
    Ok(())
}
