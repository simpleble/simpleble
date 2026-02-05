# SimpleBLE Reference

## Tool Catalog

### Adapter Management
- `get_adapters`: Lists all Bluetooth adapters. Returns `identifier` and `address`.
- `scan_for(timeout_ms, adapter_index)`: Scans for devices. Returns `identifier`, `address`, `rssi`, `connectable`, and `manufacturer_data`.

### Connection Management
- `connect(address)`: Establishes a GATT connection.
- `disconnect(address)`: Terminates the connection.

### GATT Operations
- `services(address)`: Discovers services and characteristics. Returns a list of services with their UUIDs and associated characteristic UUIDs.
- `read(address, service_uuid, char_uuid)`: Reads a single value. Returns `data_hex` and `data_utf8`.
- `notify(address, service_uuid, char_uuid, duration_ms)`: Subscribes to notifications for `duration_ms`, collects samples, and then unsubscribes.

## Platform-Specific Behavior

### macOS / iOS
- **Addresses**: Uses randomized UUIDs (e.g., `5E2A...`) instead of hardware MAC addresses. These UUIDs are persistent for the local machine but different across different hosts.
- **Permissions**: Requires Bluetooth permissions. If the MCP server fails to start or scan, check System Settings.

### Linux (BlueZ)
- **Addresses**: Uses standard MAC addresses (e.g., `AA:BB:CC:DD:EE:FF`).
- **Dependencies**: Requires `dbus` and `libdbus-1-dev` (or equivalent) to be installed.

## Data Encoding
- **Hex**: Always provided as a lowercase string without `0x` prefix.
- **UTF-8**: Decoded using `errors="ignore"`. If the data is purely binary, this field may contain garbled text or be empty.

## Error Handling
- **"Device not found in scan results"**: You must call `scan_for` before `connect` so the server knows which peripheral to use.
- **"Device not connected"**: Call `connect` before attempting `services`, `read`, or `notify`.
