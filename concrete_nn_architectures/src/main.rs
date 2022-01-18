use concrete::*;
use csv::*;
use std::time::Instant;
use std::io::{self, Write};

fn() load_model(path: &str) -> anyhow::Result<tch::CModule> {
    let model_file = std::env::current_dir()?
        .join(path);
    let model = tch::CModule::load(model_file)?;
    Ok(model)
}

fn main() {
    println!("Hello, world!");
}
