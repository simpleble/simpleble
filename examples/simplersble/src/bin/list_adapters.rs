use simplersble;

fn main() {

    println!(
        "Bluetooth enabled: {}",
        simplersble::Adapter::bluetooth_enabled().unwrap()
    );

    let adapters = simplersble::Adapter::get_adapters().unwrap();

    // If the adapter list is empty, print a message and exit
    if adapters.is_empty() {
        println!("No adapters found.");
        return;
    }

    for adapter in adapters.iter() {
        println!("Adapter: {} [{}]", adapter.identifier().unwrap(), adapter.address().unwrap());
    }
}
