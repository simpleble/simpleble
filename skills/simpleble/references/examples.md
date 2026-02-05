# SimpleBLE Examples

## Example 1: Basic Device Discovery

**User**: "Find any BLE devices nearby."

**Agent Workflow**:
1. Call `get_adapters` to ensure Bluetooth is available.
2. Call `scan_for(timeout_ms=5000)` to search for devices.
3. Display the list of found devices (Identifier, Address, RSSI).

## Example 2: Reading Device Information

**User**: "Read the firmware version from the device at address AA:BB:CC:DD:EE:FF."

**Agent Workflow**:
1. Call `scan_for` (briefly) to ensure the device is in cache.
2. Call `connect("AA:BB:CC:DD:EE:FF")`.
3. Call `services("AA:BB:CC:DD:EE:FF")` to find the Device Information Service (usually `0000180a-...`).
4. Identify the Firmware Revision String characteristic (usually `00002a26-...`).
5. Call `read("AA:BB:CC:DD:EE:FF", "180a", "2a26")`.
6. Display the `data_utf8` result.
7. Call `disconnect("AA:BB:CC:DD:EE:FF")`.

## Example 3: Debugging a Connection Issue

**User**: "I'm debugging my peripheral, please ensure it is connectable and I can read some data."

**Agent Workflow**:
1. Call `scan_for` to see if the device appears in the scan results.
2. Check the `connectable` field in the scan results.
3. If connectable, call `connect(address)`.
4. Call `services(address)` to explore available GATT characteristics.
5. Attempt a `read` from a known characteristic (e.g., Device Name or Battery Level) to verify data access.
6. Report the status of each step to the user to help pinpoint where it fails.

## Example 4: Writing to a Characteristic

**User**: "Send the command 0x01 to the control characteristic on my device at AA:BB:CC:DD:EE:FF."

**Agent Workflow**:
1. Call `scan_for` to ensure the device is in cache.
2. Call `connect("AA:BB:CC:DD:EE:FF")`.
3. Call `services("AA:BB:CC:DD:EE:FF")` to find the relevant service and characteristic.
4. Call `write_request("AA:BB:CC:DD:EE:FF", "service-uuid", "char-uuid", "01")` for a write with response, or `write_command(...)` for a write without response.
5. Call `disconnect("AA:BB:CC:DD:EE:FF")`.

## Example 5: Streaming Notification Data

**User**: "Subscribe to heart rate notifications from my device at AA:BB:CC:DD:EE:FF."

**Agent Workflow**:
1. Call `scan_for` to ensure the device is in cache.
2. Call `connect("AA:BB:CC:DD:EE:FF")`.
3. Call `services("AA:BB:CC:DD:EE:FF")` to find the Heart Rate Service (usually `0000180d-...`).
4. Identify the Heart Rate Measurement characteristic (usually `00002a37-...`).
5. Call `notify("AA:BB:CC:DD:EE:FF", "180d", "2a37")` to subscribe.
6. Wait, then call `get_notifications("AA:BB:CC:DD:EE:FF")` to retrieve buffered samples.
7. Repeat `get_notifications` as needed to collect more data.
8. Call `unsubscribe("AA:BB:CC:DD:EE:FF", "180d", "2a37")` when done.
9. Call `disconnect("AA:BB:CC:DD:EE:FF")`.

## Example 6: Troubleshooting Discovery

**User**: "I've made this Bluetooth integration but I'm not finding any device, please see if you can find any BLE device."

**Agent Workflow**:
1. Call `get_adapters` to verify the host's Bluetooth hardware is active.
2. Call `scan_for(timeout_ms=10000)` with a longer timeout to give devices more time to advertise.
3. Analyze the results: if no devices are found, suggest checking physical proximity or if the device is already paired/connected to another host.
4. If devices are found but not the specific one, list the found identifiers to see if it's appearing with a different name.
