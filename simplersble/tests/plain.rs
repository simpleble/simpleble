// Integration tests against SimpleBLE's simulated backend. Build them with SIMPLERSBLE_PLAIN=1;
// without it this file is empty, so that a regular `cargo test` never touches Bluetooth hardware.
#![cfg(simplersble_plain)]

use std::collections::HashMap;
use std::sync::mpsc;
use std::time::Duration;

use futures::{Stream, StreamExt};
use simplersble::{
    advanced, config, Adapter, Backend, CharacteristicCapability, ClientEvent, ConnectionEvent, Error,
    LocalCharacteristic, LocalCharacteristicEvent, LocalPeripheral, Peripheral, ScanEvent, ValueChangedEvent,
};

// The simulated backend keeps its state for the lifetime of the process, so tests run one at a time.
static SERIAL: tokio::sync::Mutex<()> = tokio::sync::Mutex::const_new(());

const TIMEOUT: Duration = Duration::from_secs(10);

const ALL_CAPABILITIES: [CharacteristicCapability; 5] = [
    CharacteristicCapability::Read,
    CharacteristicCapability::WriteRequest,
    CharacteristicCapability::WriteCommand,
    CharacteristicCapability::Notify,
    CharacteristicCapability::Indicate,
];

async fn next_event<T, S>(stream: &mut S) -> T
where
    S: Stream<Item = Result<T, Error>> + Unpin,
{
    tokio::time::timeout(TIMEOUT, stream.next())
        .await
        .expect("timed out waiting for an event")
        .expect("event stream ended")
        .expect("event stream failed")
}

fn plain_adapter() -> Adapter {
    let mut adapters = Adapter::get_adapters().unwrap();
    assert_eq!(adapters.len(), 1);
    adapters.remove(0)
}

fn advertises(peripheral: &Peripheral, service_uuid: &str) -> bool {
    match peripheral.services() {
        Ok(services) => services.iter().any(|service| service.uuid() == service_uuid),
        Err(_) => false,
    }
}

// Scans until a peripheral that advertises `service_uuid` is reported.
async fn find_peripheral(adapter: &Adapter, service_uuid: &str) -> Peripheral {
    let mut events = adapter.on_scan_event();
    adapter.scan_start().unwrap();

    let found = tokio::time::timeout(TIMEOUT, async {
        while let Some(event) = events.next().await {
            if let Ok(ScanEvent::Found(peripheral) | ScanEvent::Updated(peripheral)) = event {
                if advertises(&peripheral, service_uuid) {
                    return peripheral;
                }
            }
        }
        panic!("scan event stream ended");
    })
    .await
    .expect("timed out scanning");

    adapter.scan_stop().unwrap();
    found
}

fn host(adapter: &Adapter, service_uuid: &str, characteristic_uuid: &str) -> (LocalPeripheral, LocalCharacteristic) {
    let peripheral = adapter.create_local_peripheral().unwrap();
    peripheral.add_advertised_service(service_uuid).unwrap();
    let service = peripheral.add_service(service_uuid).unwrap();
    let characteristic = service.add_characteristic(characteristic_uuid, &ALL_CAPABILITIES).unwrap();
    (peripheral, characteristic)
}

// Connects to `host` from the same adapter; the simulated backend pairs on connection.
async fn connect(adapter: &Adapter, service_uuid: &str) -> Peripheral {
    let peer = find_peripheral(adapter, service_uuid).await;
    assert_eq!(peer.identifier().unwrap(), "Plain Adapter Peripheral");
    peer.connect().unwrap();
    peer
}

fn release(peer: &Peripheral) {
    peer.disconnect().unwrap();
    peer.unpair().unwrap();
}

#[tokio::test]
async fn config_round_trip() {
    let _serial = SERIAL.lock().await;
    config::reset_all();

    config::simplebluez::set_use_system_bus(false);
    config::simplebluez::set_connection_timeout(Duration::from_millis(1500));
    config::simplebluez::set_disconnection_timeout(Duration::from_millis(500));
    #[allow(deprecated)]
    config::winrt::set_experimental_use_own_mta_apartment(false);
    config::winrt::set_experimental_reinitialize_winrt_apartment_on_main_thread(true);
    config::winrt::set_use_deferred_disconnect(false);
    config::dongl::set_use_dongl_backend(false);
    config::dongl::set_auto_update(true);
    config::dongl::set_force_update(true);

    assert!(!config::simplebluez::use_system_bus());
    assert_eq!(config::simplebluez::connection_timeout(), Duration::from_millis(1500));
    assert_eq!(config::simplebluez::disconnection_timeout(), Duration::from_millis(500));
    #[allow(deprecated)]
    let own_mta_apartment = config::winrt::experimental_use_own_mta_apartment();
    assert!(!own_mta_apartment);
    assert!(config::winrt::experimental_reinitialize_winrt_apartment_on_main_thread());
    assert!(!config::winrt::use_deferred_disconnect());
    assert!(!config::dongl::use_dongl_backend());
    assert!(config::dongl::auto_update());
    assert!(config::dongl::force_update());

    config::reset_all();
    assert!(config::simplebluez::use_system_bus());
    assert_eq!(config::simplebluez::connection_timeout(), Duration::from_secs(2));
    assert!(config::winrt::use_deferred_disconnect());
    assert!(config::dongl::use_dongl_backend());
}

#[tokio::test]
async fn backend_enumeration() {
    let _serial = SERIAL.lock().await;

    let backends = Backend::get_backends().unwrap();
    assert_eq!(backends.len(), 1);
    assert_eq!(backends[0].identifier(), "Plain");
    assert!(backends[0].bluetooth_enabled().unwrap());

    let adapters = backends[0].adapters().unwrap();
    assert_eq!(adapters.len(), 1);
    assert_eq!(adapters[0].identifier().unwrap(), "Plain Adapter");

    assert_eq!(Backend::get_native().unwrap().identifier(), "Plain");
    for result in [Backend::get_dongl(), Backend::get_simulator()] {
        assert!(result.err().unwrap().to_string().contains("not supported"));
    }
}

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn local_peripheral_loopback() {
    let _serial = SERIAL.lock().await;
    const SERVICE: &str = "0000a100-0000-1000-8000-00805f9b34fb";
    const VALUE: &str = "0000a101-0000-1000-8000-00805f9b34fb";
    let (service, value) = (SERVICE.to_string(), VALUE.to_string());

    let adapter = plain_adapter();
    let (local, characteristic) = host(&adapter, SERVICE, VALUE);
    characteristic.set_value(b"ready").unwrap();
    assert_eq!(characteristic.capabilities().unwrap(), ALL_CAPABILITIES);

    let mut client_events = local.on_client_event();
    let mut characteristic_events = characteristic.on_event();
    local.start().unwrap();
    assert!(local.is_advertising().unwrap());

    let peer = connect(&adapter, SERVICE).await;
    let address = adapter.address().unwrap();
    match next_event(&mut client_events).await {
        ClientEvent::Connected(client) => assert_eq!(client, address),
        event => panic!("unexpected client event {event:?}"),
    }

    // Reads return the stored value until a read callback supplies one.
    assert_eq!(peer.read(&service, &value).unwrap(), b"ready");
    characteristic.set_callback_on_read(|| b"dynamic".to_vec()).unwrap();
    assert_eq!(peer.read(&service, &value).unwrap(), b"dynamic");
    characteristic.clear_callback_on_read().unwrap();
    assert_eq!(peer.read(&service, &value).unwrap(), b"ready");

    // Writes update the stored value before the event is delivered.
    peer.write_request(&service, &value, &b"hello".to_vec()).unwrap();
    match next_event(&mut characteristic_events).await {
        LocalCharacteristicEvent::Written(written) => assert_eq!(written, b"hello"),
        event => panic!("unexpected characteristic event {event:?}"),
    }
    assert_eq!(characteristic.value().unwrap(), b"hello");

    // Subscribing reports the first subscriber, and new values reach it.
    let mut notifications = Box::pin(peer.notify(&service, &value).unwrap());
    assert!(matches!(next_event(&mut characteristic_events).await, LocalCharacteristicEvent::Subscribed));
    characteristic.set_value(&[1, 2, 3]).unwrap();
    tokio::time::timeout(TIMEOUT, async {
        loop {
            let ValueChangedEvent::ValueUpdated(update) = next_event(&mut notifications).await;
            if update == [1, 2, 3] {
                break;
            }
        }
    })
    .await
    .expect("timed out waiting for the notification");

    peer.unsubscribe(&service, &value).unwrap();
    assert!(matches!(next_event(&mut characteristic_events).await, LocalCharacteristicEvent::Unsubscribed));

    release(&peer);
    match next_event(&mut client_events).await {
        ClientEvent::Disconnected(client) => assert_eq!(client, address),
        event => panic!("unexpected client event {event:?}"),
    }
    local.stop().unwrap();
}

// Dropping another handle for the same adapter or peripheral used to clear the callbacks of this one.
#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn handles_keep_their_events() {
    let _serial = SERIAL.lock().await;
    const SERVICE: &str = "0000a300-0000-1000-8000-00805f9b34fb";
    const VALUE: &str = "0000a301-0000-1000-8000-00805f9b34fb";

    let adapter = plain_adapter();
    let (local, _characteristic) = host(&adapter, SERVICE, VALUE);
    local.start().unwrap();

    drop(plain_adapter());
    let peer = find_peripheral(&adapter, SERVICE).await;

    let mut events = peer.on_connection_event();
    drop(adapter.scan_get_results().unwrap());
    peer.connect().unwrap();
    assert!(matches!(next_event(&mut events).await, ConnectionEvent::Connected));

    release(&peer);
    local.stop().unwrap();
}

// A blocking call on one clone used to make every other clone wait until it returned.
#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn clones_do_not_wait_for_each_other() {
    let _serial = SERIAL.lock().await;

    let adapter = plain_adapter();
    let scanner = adapter.clone();
    let scan = std::thread::spawn(move || scanner.scan_for(1000));

    tokio::time::sleep(Duration::from_millis(200)).await;
    assert!(adapter.scan_is_active().unwrap());
    scan.join().unwrap().unwrap();
}

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn read_callback_panics_and_reentrancy() {
    let _serial = SERIAL.lock().await;
    const SERVICE: &str = "0000a200-0000-1000-8000-00805f9b34fb";
    const VALUE: &str = "0000a201-0000-1000-8000-00805f9b34fb";
    let (service, value) = (SERVICE.to_string(), VALUE.to_string());

    let adapter = plain_adapter();
    let (local, characteristic) = host(&adapter, SERVICE, VALUE);
    local.start().unwrap();
    let peer = connect(&adapter, SERVICE).await;

    // A panicking read callback answers with an empty value instead of unwinding into C++.
    characteristic.set_callback_on_read(|| panic!("read callback panic (expected by the test)")).unwrap();
    assert_eq!(peer.read(&service, &value).unwrap(), b"");

    // A read callback can't replace itself, as the native side is still running it.
    let (result_sender, result) = mpsc::channel();
    let target = characteristic.clone();
    characteristic
        .set_callback_on_read(move || {
            let _ = result_sender.send(target.set_callback_on_read(Vec::new).is_err());
            Vec::new()
        })
        .unwrap();
    peer.read(&service, &value).unwrap();
    assert!(result.recv_timeout(TIMEOUT).unwrap());

    characteristic.clear_callback_on_read().unwrap();
    release(&peer);
    local.stop().unwrap();
}

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn local_manufacturer_data_and_connectability() {
    let _serial = SERIAL.lock().await;
    const SERVICE: &str = "0000a400-0000-1000-8000-00805f9b34fb";

    let adapter = plain_adapter();
    let local = adapter.create_local_peripheral().unwrap();
    local.add_service(SERVICE).unwrap();
    local.set_advertised_manufacturer_data(0x004c, &[0x02]).unwrap();
    local.set_advertised_manufacturer_data(0x004c, &[0x02, 0x15]).unwrap();
    local.set_connectable(false).unwrap();
    local.start().unwrap();

    let peer = find_peripheral(&adapter, SERVICE).await;
    assert_eq!(peer.manufacturer_data().unwrap(), HashMap::from([(0x004c, vec![0x02, 0x15])]));
    assert!(!peer.is_connectable().unwrap());
    local.stop().unwrap();
}

#[tokio::test]
async fn local_configuration_is_frozen_while_started() {
    let _serial = SERIAL.lock().await;
    const SERVICE: &str = "0000a300-0000-1000-8000-00805f9b34fb";
    const OTHER_SERVICE: &str = "0000a310-0000-1000-8000-00805f9b34fb";
    const VALUE: &str = "0000a301-0000-1000-8000-00805f9b34fb";

    let local = plain_adapter().create_local_peripheral().unwrap();
    let service = local.add_service(SERVICE).unwrap();
    let characteristic = service.add_characteristic(VALUE, &[CharacteristicCapability::Read]).unwrap();
    assert_eq!(local.services().len(), 1);

    local.start().unwrap();
    assert!(local.add_service(OTHER_SERVICE).is_err());

    // Values can still change while started.
    characteristic.set_value(b"live").unwrap();
    assert_eq!(characteristic.value().unwrap(), b"live");
    local.stop().unwrap();
}

#[tokio::test]
async fn dongl_pairing_callbacks_require_a_dongl() {
    let _serial = SERIAL.lock().await;
    const BATTERY_SERVICE: &str = "0000180f-0000-1000-8000-00805f9b34fb";

    let peer = find_peripheral(&plain_adapter(), BATTERY_SERVICE).await;
    assert!(advanced::dongl::set_passkey_request_callback(&peer, || Some("123456".to_string())).is_err());
    assert!(advanced::dongl::set_passkey_display_callback(&peer, |_| {}).is_err());
    assert!(advanced::dongl::set_numeric_comparison_callback(&peer, |_| true).is_err());
}
