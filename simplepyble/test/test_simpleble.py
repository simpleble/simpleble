# Note: This test suite is only evaluating the Python bindings, not the C++ library.
#       The SimpleBLE implementation to test this on is the PLAIN version.
import asyncio
import logging

import simplepyble
from simplepyble import aio


def test_configuration_parity():
    simplepyble.config.base.reset_all()

    try:
        assert simplepyble.config.simplebluez.use_system_bus is True
        assert simplepyble.config.simplebluez.connection_timeout_ms == 2000
        assert simplepyble.config.simplebluez.disconnection_timeout_ms == 1000
        assert (
            simplepyble.config.android.connection_priority_request
            == simplepyble.AndroidConnectionPriority.DISABLED
        )
        assert simplepyble.config.dongl.use_dongl_backend is False
        assert simplepyble.config.dongl.auto_update is False
        assert simplepyble.config.dongl.force_update is False

        simplepyble.config.simplebluez.use_system_bus = False
        simplepyble.config.simplebluez.connection_timeout_ms = 1234
        simplepyble.config.simplebluez.disconnection_timeout_ms = 5678
        simplepyble.config.android.connection_priority_request = simplepyble.AndroidConnectionPriority.HIGH
        simplepyble.config.dongl.use_dongl_backend = True
        simplepyble.config.dongl.auto_update = True
        simplepyble.config.dongl.force_update = True

        assert simplepyble.config.simplebluez.use_system_bus is False
        assert simplepyble.config.simplebluez.connection_timeout_ms == 1234
        assert simplepyble.config.simplebluez.disconnection_timeout_ms == 5678
        assert simplepyble.config.android.connection_priority_request == simplepyble.AndroidConnectionPriority.HIGH
        assert simplepyble.config.dongl.use_dongl_backend is True
        assert simplepyble.config.dongl.auto_update is True
        assert simplepyble.config.dongl.force_update is True
    finally:
        simplepyble.config.base.reset_all()


def test_backend_parity():
    backends = simplepyble.Backend.get_backends()

    assert len(backends) == 1
    assert backends[0].initialized() is True
    assert backends[0].identifier() == "Plain"
    assert backends[0].bluetooth_enabled() is True
    assert len(backends[0].adapters()) == 1

    async_backends = aio.Backend.get_backends()
    assert len(async_backends) == 1
    assert async_backends[0].identifier() == "Plain"
    assert len(async_backends[0].adapters()) == 1


def test_platform_enums():
    assert simplepyble.OperatingSystem.IOS.name == "IOS"
    assert simplepyble.OperatingSystem.ANDROID.name == "ANDROID"


def test_async_adapter_parity():
    async def run():
        adapter = aio.Adapter.get_adapters()[0]
        assert adapter.initialized() is True
        assert adapter.bluetooth_enabled() is True
        assert adapter.is_powered() is True
        await adapter.power_on()
        await adapter.power_off()

    asyncio.run(run())


def test_get_adapters():
    assert simplepyble.Adapter.bluetooth_enabled() == True

    adapters = simplepyble.Adapter.get_adapters()
    assert len(adapters) == 1

    adapter = adapters[0]
    assert adapter.identifier() == "Plain Adapter"
    assert adapter.address() == "AA:BB:CC:DD:EE:FF"


def test_scan_blocking():
    adapter = simplepyble.Adapter.get_adapters()[0]

    adapter.scan_for(1)
    peripherals = adapter.scan_get_results()
    assert len(peripherals) == 1

    peripheral = peripherals[0]
    assert peripheral.identifier() == "Plain Peripheral"
    assert peripheral.address() == "11:22:33:44:55:66"
    assert peripheral.rssi() == -60
    assert peripheral.is_connected() == False
    assert peripheral.is_paired() == False


def test_scan_async():
    # TODO: Implement once we have proper callback and advertising emulation.
    pass


def test_logging_forwards_to_python_logging(caplog):
    adapter = simplepyble.Adapter.get_adapters()[0]

    def raise_from_callback():
        raise RuntimeError("simplepyble logging smoke test")

    adapter.set_callback_on_scan_start(raise_from_callback)

    with caplog.at_level(logging.ERROR, logger="simplepyble"):
        adapter.scan_start()
        adapter.scan_stop()

    adapter.set_callback_on_scan_start(None)

    records = [
        record
        for record in caplog.records
        if record.name == "simplepyble" and "simplepyble logging smoke test" in record.getMessage()
    ]
    assert len(records) == 1
    assert records[0].levelno == logging.ERROR
    assert records[0].simpleble_module == "SimpleBLE"
    assert records[0].simpleble_function == "scan_start"


def test_connect():
    adapter = simplepyble.Adapter.get_adapters()[0]

    adapter.scan_for(1)
    peripherals = adapter.scan_get_results()
    peripheral = peripherals[0]

    peripheral.connect()
    assert peripheral.is_connected() == True
    assert peripheral.is_paired() == True

    services = peripheral.services()
    assert len(services) == 1

    service = services[0]
    assert service.uuid() == "0000180f-0000-1000-8000-00805f9b34fb"

    characteristics = service.characteristics()
    assert len(characteristics) == 1
    
    characteristic = characteristics[0]
    assert characteristic.uuid() == "00002a19-0000-1000-8000-00805f9b34fb"

    peripheral.disconnect()
    assert peripheral.is_connected() == False
    assert peripheral.is_paired() == True
