import simplepyble

if __name__ == "__main__":
    # Enable the Dongl backend before any other SimpleBLE call.
    simplepyble.config.dongl.use_dongl_backend = True

    adapters = []
    for backend in simplepyble.Backend.get_backends():
        if backend.identifier() == "Dongl":
            adapters = backend.adapters()

    if len(adapters) == 0:
        raise SystemExit("No Dongl found.")

    # Query the user to pick a Dongl
    print("Please select a Dongl:")
    for i, adapter in enumerate(adapters):
        print(f"{i}: {adapter.identifier()} [{adapter.address()}]")

    choice = int(input("Enter choice: "))
    adapter = adapters[choice]

    print(f"Selected Dongl: {adapter.identifier()} [{adapter.address()}]")

    adapter.set_callback_on_scan_found(lambda peripheral: print(f"Found {peripheral.identifier()} [{peripheral.address()}]"))

    # Scan for 5 seconds
    adapter.scan_for(5000)
    peripherals = adapter.scan_get_results()

    if len(peripherals) == 0:
        raise SystemExit("No peripherals found.")

    # Query the user to pick a peripheral
    print("Please select a peripheral:")
    for i, peripheral in enumerate(peripherals):
        print(f"{i}: {peripheral.identifier()} [{peripheral.address()}]")

    choice = int(input("Enter choice: "))
    peripheral = peripherals[choice]

    # Pairing can start as soon as the link is up, so register the callbacks before connecting.
    # They run on a SimpleBLE worker thread and may block while the user answers.
    def request_passkey():
        passkey = input("Enter the passkey shown on the device (empty to reject): ")
        return passkey or None

    def confirm_passkey(passkey):
        return input(f"Does the device show {passkey}? [y/N] ") == "y"

    dongl = simplepyble.advanced.dongl
    dongl.set_passkey_request_callback(peripheral, request_passkey)
    dongl.set_passkey_display_callback(peripheral, lambda passkey: print(f"Enter {passkey} on the device."))
    dongl.set_numeric_comparison_callback(peripheral, confirm_passkey)

    print(f"Connecting to: {peripheral.identifier()} [{peripheral.address()}]")
    peripheral.connect()

    print("Successfully connected, listing services...")
    for service in peripheral.services():
        print(f"Service: {service.uuid()}")
        for characteristic in service.characteristics():
            print(f"    Characteristic: {characteristic.uuid()}")

    print(f"Paired: {peripheral.is_paired()}")
    peripheral.disconnect()

    dongl.set_passkey_request_callback(peripheral, None)
    dongl.set_passkey_display_callback(peripheral, None)
    dongl.set_numeric_comparison_callback(peripheral, None)
