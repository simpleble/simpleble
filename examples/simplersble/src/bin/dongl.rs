use std::io::Write;

use simplersble::{advanced, config, Adapter, Backend, Peripheral};

// Asks a question on the terminal and returns the trimmed answer.
fn ask(question: &str) -> String {
    print!("{}", question);
    std::io::stdout().flush().unwrap();

    let mut answer = String::new();
    std::io::stdin().read_line(&mut answer).unwrap();
    answer.trim().to_string()
}

// Returns the adapters of the backend whose identifier is "Dongl".
fn dongl_adapters() -> Vec<Adapter> {
    for backend in Backend::get_backends().unwrap() {
        if backend.identifier() == "Dongl" {
            return backend.adapters().unwrap();
        }
    }
    Vec::new()
}

// Answers pairing requests from the terminal, since the Dongl pairs without the operating
// system. Register the callbacks before connecting.
fn register_pairing_callbacks(peripheral: &Peripheral) {
    advanced::dongl::set_numeric_comparison_callback(peripheral, |passkey| {
        ask(&format!("Does the device show {}? [y/N] ", passkey)) == "y"
    })
    .unwrap();

    advanced::dongl::set_passkey_request_callback(peripheral, || {
        Some(ask("Enter the passkey shown by the device: "))
    })
    .unwrap();

    advanced::dongl::set_passkey_display_callback(peripheral, |passkey| {
        println!("Enter {} on the device.", passkey);
    })
    .unwrap();
}

fn main() {
    // Enable the Dongl backend before any other SimpleRsBLE call.
    config::dongl::set_use_dongl_backend(true);

    let adapters = dongl_adapters();
    if adapters.is_empty() {
        eprintln!("No Dongl found.");
        std::process::exit(1);
    }

    for adapter in adapters.iter() {
        println!("Dongl: {} [{}]", adapter.identifier().unwrap(), adapter.address().unwrap());
    }
    let adapter = &adapters[0];

    println!("Scanning for 5 seconds...");
    adapter.scan_for(5000).unwrap();

    let mut peripherals = adapter.scan_get_results().unwrap();
    if peripherals.is_empty() {
        println!("No devices found.");
        return;
    }

    for (i, peripheral) in peripherals.iter().enumerate() {
        println!(
            "{}: {} [{}] {} dBm",
            i,
            peripheral.identifier().unwrap(),
            peripheral.address().unwrap(),
            peripheral.rssi().unwrap()
        );
    }

    let selection = ask("Select a device to connect to: ");
    let Some(index) = selection.parse::<usize>().ok().filter(|&index| index < peripherals.len()) else {
        eprintln!("Invalid selection.");
        std::process::exit(1);
    };
    let peripheral = peripherals.swap_remove(index);

    register_pairing_callbacks(&peripheral);

    println!("Connecting...");
    peripheral.connect().unwrap();
    println!("Connected, paired: {}", peripheral.is_paired().unwrap());

    for service in peripheral.services().unwrap().iter() {
        println!("Service: {}", service.uuid());
        for characteristic in service.characteristics().iter() {
            println!("    Characteristic: {}", characteristic.uuid());
        }
    }

    peripheral.disconnect().unwrap();
}
