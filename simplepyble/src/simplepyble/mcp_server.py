from fastmcp import FastMCP
import argparse
import time
from typing import Dict, List, Optional

import simplepyble


class BleState:
    def __init__(self) -> None:
        self.adapters: List[simplepyble.Adapter] = simplepyble.Adapter.get_adapters()
        self.adapter: Optional[simplepyble.Adapter] = (
            self.adapters[0] if self.adapters else None
        )
        self.peripherals: Dict[str, simplepyble.Peripheral] = {}
        self.scan_results: List[simplepyble.Peripheral] = []
        self.notifications: Dict[str, List[Dict]] = {}

    def refresh_adapters(self) -> None:
        self.adapters = simplepyble.Adapter.get_adapters()
        if self.adapters and not self.adapter:
            self.adapter = self.adapters[0]

    def set_active_adapter(self, adapter_index: int) -> None:
        self.refresh_adapters()
        if not self.adapters:
            raise RuntimeError("No Bluetooth adapter available")
        if adapter_index < 0 or adapter_index >= len(self.adapters):
            raise RuntimeError("Adapter index out of range")
        self.adapter = self.adapters[adapter_index]


ble_state = BleState()
mcp = FastMCP(name="SimpleBLE MCP Server")


@mcp.tool(
    name="get_adapters",
    description="List available Bluetooth adapters on the host.",
    tags={"adapter", "ble", "read"},
    annotations={
        "title": "Get Adapters",
        "readOnlyHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "discovery"},
)
def get_adapters() -> List[Dict[str, str]]:
    """List all available Bluetooth adapters."""
    ble_state.refresh_adapters()
    return [
        {"identifier": adapter.identifier(), "address": adapter.address()}
        for adapter in ble_state.adapters
    ]


@mcp.tool(
    name="scan_for",
    description="Scan for nearby BLE peripherals using the selected adapter.",
    tags={"adapter", "scan", "ble", "read"},
    annotations={
        "title": "Scan For",
        "readOnlyHint": True,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "discovery"},
)
def scan_for(timeout_ms: int = 5000, adapter_index: int = 0) -> List[Dict]:
    """Scan for nearby BLE devices."""
    ble_state.set_active_adapter(adapter_index)
    if not ble_state.adapter:
        raise RuntimeError("No Bluetooth adapter available")

    ble_state.adapter.scan_for(timeout_ms)
    ble_state.scan_results = ble_state.adapter.scan_get_results()

    results: List[Dict] = []
    for peripheral in ble_state.scan_results:
        manufacturer_data: Dict[str, str] = {}
        for company_id, data in peripheral.manufacturer_data().items():
            manufacturer_data[str(company_id)] = data.hex()

        results.append(
            {
                "identifier": peripheral.identifier(),
                "address": peripheral.address(),
                "rssi": peripheral.rssi(),
                "connectable": peripheral.is_connectable(),
                "manufacturer_data": manufacturer_data,
            }
        )
    return results


@mcp.tool(
    name="connect",
    description="Connect to a peripheral previously discovered in the last scan.",
    tags={"peripheral", "connect", "ble"},
    annotations={
        "title": "Connect",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "connection"},
)
def connect(address: str) -> Dict[str, str]:
    """Connect to a peripheral from the last scan results."""
    if address in ble_state.peripherals:
        peripheral = ble_state.peripherals[address]
        if peripheral.is_connected():
            return {"message": f"Already connected to {peripheral.identifier()}", "address": address}

    target: Optional[simplepyble.Peripheral] = None
    for peripheral in ble_state.scan_results:
        if peripheral.address() == address:
            target = peripheral
            break

    if not target:
        if address in ble_state.peripherals:
            target = ble_state.peripherals[address]
        else:
            raise RuntimeError("Device not found in scan results. Please scan first.")

    try:
        target.connect()
    except Exception as exc:  # pragma: no cover - backend-specific errors
        raise RuntimeError(f"Failed to connect: {exc}") from exc

    ble_state.peripherals[address] = target
    ble_state.notifications.setdefault(address, [])
    return {"message": f"Connected to {target.identifier()}", "address": address}


@mcp.tool(
    name="disconnect",
    description="Disconnect from a connected peripheral.",
    tags={"peripheral", "disconnect", "ble"},
    annotations={
        "title": "Disconnect",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "connection"},
)
def disconnect(address: str) -> Dict[str, str]:
    """Disconnect from a connected peripheral."""
    if address not in ble_state.peripherals:
        raise RuntimeError("Device not found")

    peripheral = ble_state.peripherals[address]
    try:
        peripheral.disconnect()
    except Exception as exc:  # pragma: no cover - backend-specific errors
        raise RuntimeError(f"Failed to disconnect: {exc}") from exc

    ble_state.notifications.pop(address, None)
    return {"message": f"Disconnected from {peripheral.identifier()}", "address": address}


@mcp.tool(
    name="services",
    description="List services and characteristics on a connected peripheral.",
    tags={"peripheral", "gatt", "ble", "read"},
    annotations={
        "title": "Services",
        "readOnlyHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def services(address: str) -> Dict:
    """List services and characteristics for a connected device."""
    if address not in ble_state.peripherals:
        raise RuntimeError("Device not found or not connected")

    peripheral = ble_state.peripherals[address]
    if not peripheral.is_connected():
        return {"address": address, "connected": False}

    services = []
    for service in peripheral.services():
        characteristics = [char.uuid() for char in service.characteristics()]
        services.append({"uuid": service.uuid(), "characteristics": characteristics})

    return {
        "identifier": peripheral.identifier(),
        "address": address,
        "connected": peripheral.is_connected(),
        "mtu": peripheral.mtu(),
        "services": services,
    }


@mcp.tool(
    name="read",
    description="Read a characteristic value from a connected peripheral.",
    tags={"peripheral", "gatt", "ble", "read"},
    annotations={
        "title": "Read",
        "readOnlyHint": True,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def read(address: str, service_uuid: str, char_uuid: str) -> Dict[str, str]:
    """Read a characteristic value from a connected device."""
    if address not in ble_state.peripherals:
        raise RuntimeError("Device not found")

    peripheral = ble_state.peripherals[address]
    if not peripheral.is_connected():
        raise RuntimeError("Device not connected")

    try:
        data = peripheral.read(service_uuid, char_uuid)
    except Exception as exc:  # pragma: no cover - backend-specific errors
        raise RuntimeError(f"Read failed: {exc}") from exc

    return {
        "service_uuid": service_uuid,
        "char_uuid": char_uuid,
        "data_hex": data.hex(),
        "data_utf8": data.decode("utf-8", errors="ignore"),
    }


@mcp.tool(
    name="notify",
    description="Collect notifications from a characteristic for a fixed duration.",
    tags={"peripheral", "gatt", "ble", "notify"},
    annotations={
        "title": "Notify",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def notify(
    address: str,
    service_uuid: str,
    char_uuid: str,
    duration_ms: int = 5000,
) -> List[Dict[str, str]]:
    """Collect notifications for a duration, then unsubscribe."""
    if address not in ble_state.peripherals:
        raise RuntimeError("Device not found")

    peripheral = ble_state.peripherals[address]
    if not peripheral.is_connected():
        raise RuntimeError("Device not connected")

    samples: List[Dict[str, str]] = []

    def notification_callback(data: bytes) -> None:
        samples.append(
            {
                "service": service_uuid,
                "characteristic": char_uuid,
                "data_hex": data.hex(),
                "data_utf8": data.decode("utf-8", errors="ignore"),
                "type": "notification",
            }
        )

    try:
        peripheral.notify(service_uuid, char_uuid, notification_callback)
        time.sleep(max(duration_ms, 0) / 1000)
        peripheral.unsubscribe(service_uuid, char_uuid)
    except Exception as exc:  # pragma: no cover - backend-specific errors
        raise RuntimeError(f"Notify failed: {exc}") from exc

    return samples


def main() -> None:
    parser = argparse.ArgumentParser(description="SimplePyBLE MCP Server")
    parser.add_argument("--transport", default="stdio", choices=["stdio", "http"])
    parser.add_argument("--host", default="127.0.0.1", help="Host to bind to")
    parser.add_argument("--port", type=int, default=8000, help="Port to bind to")
    args = parser.parse_args()

    if args.transport == "http":
        mcp.run(transport="http", host=args.host, port=args.port)
    else:
        mcp.run()


if __name__ == "__main__":
    main()
