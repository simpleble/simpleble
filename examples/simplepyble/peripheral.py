import time

import simplepyble

SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0"
CHARACTERISTIC_UUID = "12345678-1234-5678-1234-56789abcdef1"

if __name__ == "__main__":
    adapters = simplepyble.Adapter.get_adapters()

    if len(adapters) == 0:
        raise SystemExit("No Bluetooth adapters found.")

    peripheral = adapters[0].create_local_peripheral()

    # Add a service UUID to the advertisement.
    peripheral.add_advertised_service(SERVICE_UUID)

    Capability = simplepyble.LocalCharacteristicCapability
    service = peripheral.add_service(SERVICE_UUID)
    characteristic = service.add_characteristic(
        CHARACTERISTIC_UUID,
        [
            Capability.READ,
            Capability.WRITE_REQUEST,
            Capability.WRITE_COMMAND,
            Capability.NOTIFY,
            Capability.INDICATE,
        ],
    )
    characteristic.set_value(b"ready")

    def on_write(value):
        print(f"Write: {value.hex()}")
        # Echo the written value to subscribed clients.
        characteristic.set_value(value)

    characteristic.set_callback_on_write(on_write)
    characteristic.set_callback_on_subscribed(lambda: print("Client subscribed."))
    characteristic.set_callback_on_unsubscribed(lambda: print("Client unsubscribed."))

    peripheral.set_callback_on_client_connected(lambda address: print(f"Client connected: {address}"))
    peripheral.set_callback_on_client_disconnected(lambda address: print(f"Client disconnected: {address}"))

    peripheral.start()
    print("Advertising. Press Ctrl+C to stop.")

    try:
        while True:
            time.sleep(0.1)
    except KeyboardInterrupt:
        pass

    # The write callback references the characteristic; unregister it to release both.
    characteristic.set_callback_on_write(None)
    peripheral.stop()
