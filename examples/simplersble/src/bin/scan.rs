use std::time::Duration;

use simplersble;
use futures::stream::StreamExt;

#[tokio::main]
async fn main() {
    let mut adapters = simplersble::Adapter::get_adapters().unwrap();

    // If the adapter list is empty, print a message and exit
    if adapters.is_empty() {
        println!("No adapters found.");
        return;
    }

    // Pick the first adapter
    let adapter = adapters.pop().unwrap();

    let mut scan_event = adapter.on_scan_event();
    tokio::spawn(async move {
        while let Some(Ok(event)) = scan_event.next().await {
            match event {
                simplersble::ScanEvent::Start => println!("Scan started."),
                simplersble::ScanEvent::Stop => println!("Scan stopped."),
                simplersble::ScanEvent::Found(peripheral) => println!("Found device: {} [{}] {} dBm", peripheral.identifier().unwrap(), peripheral.address().unwrap(), peripheral.rssi().unwrap()),
                simplersble::ScanEvent::Updated(peripheral) => println!("Updated device: {} [{}] {} dBm", peripheral.identifier().unwrap(), peripheral.address().unwrap(), peripheral.rssi().unwrap()),
            }
        }
    });

    adapter.scan_for(Duration::from_secs(5)).unwrap();
    println!("Scan complete.");

    println!("The following devices were found:");

    for (i, peripheral) in adapter.scan_get_results().unwrap().iter().enumerate() {
        let connectable_str = "unknown";
        let peripheral_str = format!(
            "{} [{}] {} dBm",
            peripheral.identifier().unwrap(),
            peripheral.address().unwrap(),
            peripheral.rssi().unwrap()
        );

        println!("{}: {} {}", i, peripheral_str, connectable_str);

        println!("    Tx Power: {}", peripheral.tx_power().unwrap());
        println!("    Address Type: {}", peripheral.address_type().unwrap());

        for service in peripheral.services().unwrap().iter() {
            println!("    Service UUID: {}", service.uuid());
            println!("    Service Data: {:?}", service.data());
        }

        for (manufacturer_id, manufacturer_data) in peripheral.manufacturer_data().unwrap().iter() {
            println!("    Manufacturer ID: {}", manufacturer_id);
            println!("    Manufacturer Data: {:?}", manufacturer_data);
        }
    }
}
