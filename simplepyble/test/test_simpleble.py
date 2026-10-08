# Note: This test suite is only evaluating the Python bindings, not the C++ library.
#       The SimpleBLE implementation to test this on is the PLAIN version.
import asyncio
import logging
import subprocess
import sys
import textwrap
import threading

import pytest

import simplepyble
from simplepyble import aio

SERVICE = "0000fff0-0000-1000-8000-00805f9b34fb"
VALUE = "0000fff1-0000-1000-8000-00805f9b34fb"
ERROR_VALUE = "0000fff2-0000-1000-8000-00805f9b34fb"
DESCRIPTION = "00002901-0000-1000-8000-00805f9b34fb"


@pytest.fixture(autouse=True)
def restore_plain_state():
    adapter = simplepyble.Adapter.get_adapters()[0]
    adapter.power_on()

    yield

    adapter.scan_stop()
    for peripheral in adapter.scan_get_results():
        peripheral.set_callback_on_connected(None)
        peripheral.set_callback_on_disconnected(None)
        if peripheral.is_connected():
            peripheral.disconnect()
        peripheral.unpair()

    adapter.set_callback_on_scan_start(None)
    adapter.set_callback_on_scan_stop(None)
    adapter.set_callback_on_scan_found(None)
    adapter.set_callback_on_scan_updated(None)
    adapter.set_callback_on_power_on(None)
    adapter.set_callback_on_power_off(None)
    adapter.power_on()


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
        assert adapter.is_powered() is False
        await adapter.power_on()

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

    adapter.scan_for(250)
    peripherals = adapter.scan_get_results()
    assert len(peripherals) == 1

    peripheral = peripherals[0]
    assert peripheral.identifier() == "Plain Peripheral"
    assert peripheral.address() == "11:22:33:44:55:66"
    assert peripheral.rssi() == -60
    assert peripheral.is_connected() == False
    assert peripheral.is_paired() == False


def test_scan_async():
    adapter = simplepyble.Adapter.get_adapters()[0]
    found = threading.Event()
    updated = threading.Event()

    adapter.set_callback_on_scan_found(lambda peripheral: found.set())
    adapter.set_callback_on_scan_updated(lambda peripheral: updated.set())

    adapter.scan_start()
    assert found.wait(2)
    assert updated.wait(2)

    adapter.scan_stop()
    assert len(adapter.scan_get_results()) == 1


def test_logging_forwards_to_python_logging(caplog, monkeypatch):
    adapter = simplepyble.Adapter.get_adapters()[0]
    logged = threading.Event()
    emit = caplog.handler.emit

    def capture(record):
        emit(record)
        if "simplepyble logging smoke test" in record.getMessage():
            logged.set()

    monkeypatch.setattr(caplog.handler, "emit", capture)

    def raise_from_callback():
        raise RuntimeError("simplepyble logging smoke test")

    adapter.set_callback_on_scan_start(raise_from_callback)

    with caplog.at_level(logging.ERROR, logger="simplepyble"):
        adapter.scan_start()
        assert logged.wait(2)
        adapter.set_callback_on_scan_start(None)
        adapter.scan_stop()

    records = [
        record
        for record in caplog.records
        if record.name == "simplepyble" and "simplepyble logging smoke test" in record.getMessage()
    ]
    assert len(records) == 1
    assert records[0].levelno == logging.ERROR
    assert records[0].simpleble_module == "SimpleBLE"
    assert records[0].simpleble_function


def test_connect():
    adapter = simplepyble.Adapter.get_adapters()[0]

    adapter.scan_for(250)
    peripherals = adapter.scan_get_results()
    peripheral = peripherals[0]

    peripheral.connect()
    assert peripheral.is_connected() == True
    assert peripheral.is_paired() == True

    services = peripheral.services()
    assert len(services) == 2

    service = services[0]
    assert service.uuid() == "0000180f-0000-1000-8000-00805f9b34fb"

    characteristics = service.characteristics()
    assert len(characteristics) == 1
    
    characteristic = characteristics[0]
    assert characteristic.uuid() == "00002a19-0000-1000-8000-00805f9b34fb"

    peripheral.disconnect()
    assert peripheral.is_connected() == False
    assert peripheral.is_paired() == True


def test_gatt_binary_values_and_errors():
    adapter = simplepyble.Adapter.get_adapters()[0]
    adapter.scan_for(250)
    peripheral = adapter.scan_get_results()[0]
    peripheral.connect()
    assert peripheral.mtu() == 244

    payload = b"\x00\xff\x80\x01"
    peripheral.write_request(SERVICE, VALUE, payload)
    assert peripheral.read(SERVICE, VALUE) == payload

    peripheral.write_command(SERVICE, VALUE, b"")
    assert peripheral.read(SERVICE, VALUE) == b""

    peripheral.descriptor_write(SERVICE, VALUE, DESCRIPTION, payload)
    assert peripheral.descriptor_read(SERVICE, VALUE, DESCRIPTION) == payload

    with pytest.raises(RuntimeError, match="ATT error 0x08"):
        peripheral.read(SERVICE, ERROR_VALUE)
    with pytest.raises(RuntimeError, match="ATT error 0x13"):
        peripheral.write_request(SERVICE, ERROR_VALUE, payload)

    for subscribe in (peripheral.notify, peripheral.indicate):
        peripheral.write_request(SERVICE, VALUE, payload)
        received = []
        ready = threading.Event()

        def callback(value):
            received.append(value)
            ready.set()

        subscribe(SERVICE, VALUE, callback)
        assert ready.wait(2)
        peripheral.unsubscribe(SERVICE, VALUE)
        assert received[0] == payload

    peripheral.disconnect()


def test_callback_replacement_releases_the_gil():
    # A subprocess bounds the failure if callback replacement deadlocks with the GIL.
    code = textwrap.dedent("""
        import threading
        import time

        import simplepyble

        adapter = simplepyble.Adapter.get_adapters()[0]
        entered = threading.Event()
        release = threading.Event()

        def callback():
            entered.set()
            release.wait(2)

        adapter.set_callback_on_scan_start(callback)
        adapter.scan_start()
        assert entered.wait(2)

        worker = threading.Thread(target=lambda: adapter.set_callback_on_scan_start(None), daemon=True)
        worker.start()
        time.sleep(0.05)
        release.set()
        worker.join(2)
        assert not worker.is_alive()
        adapter.scan_stop()
    """)
    subprocess.run([sys.executable, "-c", code], check=True, timeout=8)
