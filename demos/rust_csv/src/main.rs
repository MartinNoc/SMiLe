extern crate csv;
extern crate serde;
// This let's us write: #[derive(Deserialize)]
#[macro_use]
extern crate serde_derive;

use std::env;
use std::error::Error;
use std::ffi::OsString;
use std::process;
use std::collections::HashMap;

#[derive(Debug,Deserialize,Serialize)]
#[serde(rename_all = "PascalCase")]
struct Record {
    mx : f64,
    time : u64,
    my : f64,
}


fn run() -> Result<(), Box<Error>> {
    let file_path = get_first_arg()?;
    let mut rdr = csv::ReaderBuilder::new().from_path(file_path)?;
    let mut wtr = csv::WriterBuilder::new().from_path("./csv_output")?;

    // wtr.write_record(rdr.headers()?)?;
    
    for result in rdr.deserialize() {
        let record: Record  = result?;
        // println!("{:?}", record);
        wtr.serialize(record)?;
    }

    wtr.flush()?;

    Ok(())
}

/// Returns the first positional argument sent to this process. If there are no
/// positional arguments, then this returns an error.
fn get_first_arg() -> Result<OsString, Box<Error>> {
    match env::args_os().nth(1) {
        None => Err(From::from("expected 1 argument, but got none")),
        Some(file_path) => Ok(file_path),
    }
}

fn main() {
    if let Err(err) = run() {
        println!("{}", err);
        process::exit(1);
    }
}
