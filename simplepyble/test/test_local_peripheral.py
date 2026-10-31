# Note: This test suite is only evaluating the Python bindings, not the C++ library.
#       It relies on the PLAIN build's local GATT loopback: a started local peripheral
#       is discovered and connected to by the same process.
import asyncio
import subprocess
import sys
import textwrap
import threading

import pytest

import simplepyble
from simplepyble import aio

HOST_NAME = "Plain Adapter Peripheral"
ADAPTER_ADDRESS = "AA:BB:CC:DD:EE:FF"
SERVICE = "00001234-0000-1000-8000-00805f9b34fb"
VALUE = "00005678-0000-1000-8000-00805f9b34fb"
DYNAMIC = "00005679-0000-1000-8000-00805f9b34fb"
TIMEOUT = 5

Capability = simplepyble.LocalCharacteristicCapability


@pytest.fixture(autouse=True)
def restore_plain_state():
    adapter = simplepyble.Adapter.get_adapters()[0]
    adapter.power_on()

    yield

    adapter.scan_stop()
    adapter.set_callback_on_scan_found(None)
    for peripheral in adapter.scan_get_results():
        peripheral.set_callback_on_connected(None)
        peripheral.set_callback_on_disconnected(None)
        if peripheral.is_connected():
            peripheral.disconnect()
        peripheral.unpair()


@pytest.fixture
def host():
    local = simplepyble.Adapter.get_adapters()[0].create_local_peripheral()

    yield local

    clear_local_callbacks(local)
    local.stop()


def clear_local_callbacks(local):
    local.set_callback_on_client_connected(None)
    local.set_callback_on_client_disconnected(None)
    for service in local.services():
        for characteristic in service.characteristics():
            characteristic.set_callback_on_read(None)
            characteristic.set_callback_on_write(None)
            characteristic.set_callback_on_subscribed(None)
            characteristic.set_callback_on_unsubscribed(None)


def scan_for(adapter, identifier):
    found = threading.Event()

    def on_found(peripheral):
        if peripheral.identifier() == identifier:
            found.set()

    adapter.set_callback_on_scan_found(on_found)
    adapter.scan_start()
    try:
        assert found.wait(TIMEOUT)
    finally:
        adapter.scan_stop()
        adapter.set_callback_on_scan_found(None)
    return next(p for p in adapter.scan_get_results() if p.identifier() == identifier)


def configure(local):
    service = local.add_service(SERVICE)
    value = service.add_characteristic(VALUE, [Capability.READ, Capability.WRITE_REQUEST, Capability.NOTIFY])
    dynamic = service.add_characteristic(DYNAMIC, [Capability.READ])
    value.set_value(b"\x00\xff")
    return value, dynamic


def test_local_peripheral_configuration(host):
    assert host.initialized() is True
    assert host.is_started() is False
    host.add_advertised_service(SERVICE)
    host.add_advertised_service([SERVICE, DYNAMIC])

    service = host.add_service(SERVICE)
    assert service.initialized() is True
    assert service.uuid() == SERVICE
    characteristic = service.add_characteristic(VALUE, [Capability.READ, Capability.INDICATE])
    assert characteristic.uuid() == VALUE
    assert characteristic.capabilities() == {Capability.READ, Capability.INDICATE}
    assert [s.uuid() for s in host.services()] == [SERVICE]
    assert [c.uuid() for c in service.characteristics()] == [VALUE]

    characteristic.set_value(bytearray(b"\x00\xff"))
    assert characteristic.value() == b"\x00\xff"

    host.remove_all_services()
    assert host.services() == []


def test_local_peripheral_loopback(host):
    adapter = simplepyble.Adapter.get_adapters()[0]
    value, dynamic = configure(host)

    connected = threading.Event()
    disconnected = threading.Event()
    clients = []
    host.set_callback_on_client_connected(lambda address: (clients.append(address), connected.set()))
    host.set_callback_on_client_disconnected(lambda address: (clients.append(address), disconnected.set()))

    writes = []

    def on_write(payload):
        writes.append(payload)
        # Echo the written value to subscribed clients.
        value.set_value(payload)

    value.set_callback_on_write(on_write)
    dynamic.set_callback_on_read(lambda: b"dynamic")
    subscribed = threading.Event()
    unsubscribed = threading.Event()
    value.set_callback_on_subscribed(subscribed.set)
    value.set_callback_on_unsubscribed(unsubscribed.set)

    host.start()
    assert host.is_started() is True
    assert host.is_advertising() is True

    with pytest.raises(RuntimeError, match="cannot change while started"):
        host.add_service(DYNAMIC)

    peer = scan_for(adapter, HOST_NAME)
    peer.connect()
    assert connected.wait(TIMEOUT)
    assert clients == [ADAPTER_ADDRESS]

    assert peer.read(SERVICE, VALUE) == b"\x00\xff"
    assert peer.read(SERVICE, DYNAMIC) == b"dynamic"

    notifications = []
    notified = threading.Event()

    def on_notify(payload):
        notifications.append(payload)
        notified.set()

    peer.notify(SERVICE, VALUE, on_notify)
    assert subscribed.wait(TIMEOUT)

    # The write callback runs before the write response, and its echo is published.
    peer.write_request(SERVICE, VALUE, b"\x01\x02")
    assert writes == [b"\x01\x02"]
    assert value.value() == b"\x01\x02"
    assert notified.wait(TIMEOUT)
    assert notifications[-1] == b"\x01\x02"

    notified.clear()
    value.set_value(b"\x03")
    assert notified.wait(TIMEOUT)
    assert notifications[-1] == b"\x03"

    peer.unsubscribe(SERVICE, VALUE)
    assert unsubscribed.wait(TIMEOUT)

    host.stop()
    assert host.is_started() is False
    assert disconnected.wait(TIMEOUT)
    assert clients == [ADAPTER_ADDRESS, ADAPTER_ADDRESS]


def test_local_manufacturer_data_and_connectability(host):
    adapter = simplepyble.Adapter.get_adapters()[0]
    host.set_advertised_manufacturer_data(0x004C, b"\x02")
    host.set_advertised_manufacturer_data(0x004C, bytearray(b"\x02\x15"))
    host.set_connectable(False)
    host.start()

    peer = scan_for(adapter, HOST_NAME)
    assert peer.manufacturer_data() == {0x004C: b"\x02\x15"}
    assert peer.is_connectable() is False


def test_local_read_callback_errors_fail_the_read(host):
    adapter = simplepyble.Adapter.get_adapters()[0]
    _, dynamic = configure(host)
    dynamic.set_value(b"stored")
    host.start()
    peer = scan_for(adapter, HOST_NAME)
    peer.connect()

    def raise_from_callback():
        raise ValueError("simplepyble read callback failure")

    dynamic.set_callback_on_read(raise_from_callback)
    with pytest.raises(RuntimeError):
        peer.read(SERVICE, DYNAMIC)

    dynamic.set_callback_on_read(None)
    assert peer.read(SERVICE, DYNAMIC) == b"stored"
    peer.disconnect()


def test_local_peripheral_release_releases_the_gil():
    # A subprocess bounds the failure if releasing the host deadlocks with the GIL.
    code = textwrap.dedent("""
        import gc
        import threading

        import simplepyble

        adapter = simplepyble.Adapter.get_adapters()[0]
        local = adapter.create_local_peripheral()
        local.add_service("00001234-0000-1000-8000-00805f9b34fb").add_characteristic(
            "00005678-0000-1000-8000-00805f9b34fb", [simplepyble.LocalCharacteristicCapability.READ])
        entered = threading.Event()
        release = threading.Event()

        def on_disconnected(address):
            entered.set()
            release.wait(2)

        local.set_callback_on_client_disconnected(on_disconnected)
        local.start()

        found = threading.Event()
        adapter.set_callback_on_scan_found(
            lambda p: found.set() if p.identifier() == "Plain Adapter Peripheral" else None)
        adapter.scan_start()
        assert found.wait(5)
        adapter.scan_stop()
        adapter.set_callback_on_scan_found(None)
        peer = next(p for p in adapter.scan_get_results() if p.identifier() == "Plain Adapter Peripheral")
        peer.connect()
        peer.disconnect()
        assert entered.wait(5)

        # Releasing the host unregisters the callback that is still running.
        threading.Timer(0.05, release.set).start()
        del local
        gc.collect()
    """)
    subprocess.run([sys.executable, "-c", code], check=True, timeout=20)


def test_pairing_requires_a_supporting_backend():
    adapter = simplepyble.Adapter.get_adapters()[0]
    peripheral = scan_for(adapter, "Plain Peripheral")
    dongl = simplepyble.advanced.dongl

    with pytest.raises(RuntimeError, match="not supported"):
        dongl.set_passkey_request_callback(peripheral, lambda: "123456")
    with pytest.raises(RuntimeError, match="not supported"):
        dongl.set_passkey_display_callback(peripheral, print)
    with pytest.raises(RuntimeError, match="not supported"):
        dongl.set_numeric_comparison_callback(peripheral, lambda passkey: True)
    with pytest.raises(RuntimeError, match="not supported"):
        dongl.secure_connection(peripheral, mitm=True)

    if sys.platform.startswith("linux"):
        linux = simplepyble.advanced.linux
        with pytest.raises(RuntimeError, match="not supported"):
            linux.set_passkey_request_callback(adapter, lambda peer: "123456")
        with pytest.raises(RuntimeError, match="not supported"):
            linux.set_passkey_display_callback(adapter, print)
        with pytest.raises(RuntimeError, match="not supported"):
            linux.set_numeric_comparison_callback(adapter, lambda peer, passkey: True)
        with pytest.raises(RuntimeError, match="not supported"):
            linux.set_pairing_authorization_callback(adapter, lambda peer: True)


def test_async_local_peripheral_loopback():
    async def run():
        adapter = aio.Adapter.get_adapters()[0]
        local = adapter.create_local_peripheral()
        service = local.add_service(SERVICE)
        value = service.add_characteristic(VALUE, [Capability.READ, Capability.WRITE_REQUEST, Capability.NOTIFY])
        dynamic = service.add_characteristic(DYNAMIC, [Capability.READ])
        await value.set_value(b"\x00\xff")
        loop_thread = threading.get_ident()

        async def on_read():
            assert threading.get_ident() == loop_thread
            await asyncio.sleep(0)
            return b"async dynamic"

        dynamic.set_callback_on_read(on_read)
        writes = asyncio.Queue()
        value.set_callback_on_write(writes.put_nowait)
        subscribed = asyncio.Event()

        async def on_subscribed():
            subscribed.set()

        value.set_callback_on_subscribed(on_subscribed)
        connected = asyncio.Event()
        local.set_callback_on_client_connected(lambda address: connected.set())

        async with local:
            await local.start()
            assert local.is_started() is True

            found = asyncio.Event()
            adapter.set_callback_on_scan_found(lambda p: found.set() if p.identifier() == HOST_NAME else None)
            await adapter.scan_start()
            await asyncio.wait_for(found.wait(), TIMEOUT)
            await adapter.scan_stop()
            adapter.set_callback_on_scan_found(None)
            peer = next(p for p in adapter.scan_get_results() if p.identifier() == HOST_NAME)

            async with peer:
                await peer.connect()
                await asyncio.wait_for(connected.wait(), TIMEOUT)
                assert await peer.read(SERVICE, VALUE) == b"\x00\xff"
                assert await peer.read(SERVICE, DYNAMIC) == b"async dynamic"

                await peer.write_request(SERVICE, VALUE, b"\x01")
                assert await asyncio.wait_for(writes.get(), TIMEOUT) == b"\x01"

                notifications = asyncio.Queue()
                await peer.notify(SERVICE, VALUE, notifications.put_nowait)
                await asyncio.wait_for(subscribed.wait(), TIMEOUT)
                await value.set_value(b"\x02")
                while await asyncio.wait_for(notifications.get(), TIMEOUT) != b"\x02":
                    pass
                await peer.disconnect()

        # Leaving the context clears callbacks and stops the host.
        assert local.is_started() is False
        assert value.value() == b"\x02"

    asyncio.run(run())



