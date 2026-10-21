use futures::stream::StreamExt;
use simplersble::{CharacteristicCapability, ClientEvent, LocalCharacteristicEvent};

const SERVICE_UUID: &str = "12345678-1234-5678-1234-56789abcdef0";
const CHARACTERISTIC_UUID: &str = "12345678-1234-5678-1234-56789abcdef1";

fn to_hex(value: &[u8]) -> String {
    value.iter().map(|byte| format!("{:02x}", byte)).collect()
}

#[tokio::main]
async fn main() {
    let mut adapters = simplersble::Adapter::get_adapters().unwrap();
    if adapters.is_empty() {
        eprintln!("No Bluetooth adapters found.");
        std::process::exit(1);
    }

    let adapter = adapters.remove(0);
    let peripheral = adapter.create_local_peripheral().unwrap();

    // Add a service UUID to the advertisement.
    peripheral.add_advertised_service(SERVICE_UUID).unwrap();

    let service = peripheral.add_service(SERVICE_UUID).unwrap();
    let characteristic = service
        .add_characteristic(
            CHARACTERISTIC_UUID,
            &[
                CharacteristicCapability::Read,
                CharacteristicCapability::WriteRequest,
                CharacteristicCapability::WriteCommand,
                CharacteristicCapability::Notify,
                CharacteristicCapability::Indicate,
            ],
        )
        .unwrap();
    characteristic.set_value(b"ready").unwrap();

    let mut characteristic_events = characteristic.on_event();
    let echo = characteristic.clone();
    tokio::spawn(async move {
        while let Some(event) = characteristic_events.next().await {
            match event {
                LocalCharacteristicEvent::Written(value) => {
                    println!("Write: {}", to_hex(&value));
                    // Echo the written value to subscribed clients.
                    if let Err(e) = echo.set_value(&value) {
                        eprintln!("Failed to update the value: {}", e);
                    }
                }
                LocalCharacteristicEvent::Subscribed => println!("Client subscribed."),
                LocalCharacteristicEvent::Unsubscribed => println!("Client unsubscribed."),
            }
        }
    });

    let mut client_events = peripheral.on_client_event();
    tokio::spawn(async move {
        while let Some(event) = client_events.next().await {
            match event {
                ClientEvent::Connected(address) => println!("Client connected: {}", address),
                ClientEvent::Disconnected(address) => println!("Client disconnected: {}", address),
            }
        }
    });

    peripheral.start().unwrap();
    println!("Advertising. Press Ctrl+C to stop.");

    tokio::signal::ctrl_c().await.unwrap();

    peripheral.stop().unwrap();
}
