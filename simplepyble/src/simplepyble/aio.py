import asyncio
import atexit
import concurrent.futures
import inspect
import weakref
from collections.abc import Awaitable, Callable, Iterable
from typing import Any
import simplepyble

_active_adapters: weakref.WeakSet[Any] = weakref.WeakSet()
_active_peripherals: weakref.WeakSet[Any] = weakref.WeakSet()
_active_local_peripherals: weakref.WeakSet[Any] = weakref.WeakSet()

# Read and pairing callbacks block a native thread until they return. There's no point
# in waiting longer than 30 seconds, as that's when BLE gives up on the request.
_RESULT_TIMEOUT_S = 30.0

# Tasks running coroutine callbacks, as the loop itself only keeps weak references to them.
_callback_tasks: set[asyncio.Task[Any]] = set()

def _post(loop: asyncio.AbstractEventLoop, callback: Callable[..., Any], *args: Any) -> None:
    """Hand an event from a native thread to the loop the callback was registered on."""
    try:
        loop.call_soon_threadsafe(_run_callback, loop, callback, args)
    except RuntimeError:
        # The loop was closed after the callback was registered.
        pass

def _run_callback(loop: asyncio.AbstractEventLoop, callback: Callable[..., Any], args: tuple[Any, ...]) -> None:
    result = callback(*args)
    if inspect.isawaitable(result):
        task = asyncio.ensure_future(result)
        _callback_tasks.add(task)
        task.add_done_callback(lambda task: _report_callback_task(loop, task))

def _report_callback_task(loop: asyncio.AbstractEventLoop, task: asyncio.Task[Any]) -> None:
    # Nobody awaits these tasks, so their exceptions go to the loop's exception handler, like those of plain callbacks.
    _callback_tasks.discard(task)
    if task.cancelled() or task.exception() is None:
        return
    loop.call_exception_handler({
        "message": "Exception in SimplePyBLE callback",
        "exception": task.exception(),
        "task": task,
    })

def _wait_on_loop(loop: asyncio.AbstractEventLoop, callback: Callable[..., Any], *args: Any) -> Any:
    """Run a callback on its loop from a native thread and return its result.

    Raises RuntimeError when the loop cannot answer, which native code treats like
    a callback that raised: the read fails or the pairing request is rejected.
    """
    try:
        running = asyncio.get_running_loop()
    except RuntimeError:
        running = None
    if running is loop:
        raise RuntimeError("Callback was invoked on its event loop thread; waiting for it would deadlock")

    async def invoke():
        result = callback(*args)
        if inspect.isawaitable(result):
            result = await result
        return result

    coroutine = invoke()
    try:
        future = asyncio.run_coroutine_threadsafe(coroutine, loop)
    except RuntimeError:
        coroutine.close()
        raise RuntimeError("Event loop is closed") from None

    try:
        return future.result(timeout=_RESULT_TIMEOUT_S)
    except concurrent.futures.TimeoutError:
        future.cancel()
        raise RuntimeError("Event loop did not run the callback in time") from None

def _global_cleanup():
    # Clear the adapters we handed out.
    # NOTE: We don't enumerate adapters here, as that would start every backend during exit,
    # and macOS kills processes without Bluetooth permission as soon as CoreBluetooth starts.
    for a in list(_active_adapters):
        a._clear_all_callbacks_sync()

    # Clear peripherals that we tracked via aio.Peripheral
    for p in list(_active_peripherals):
        try:
            p._clear_all_callbacks_sync()
        except:
            pass

    # Clear and stop local peripherals created via aio.Adapter
    for p in list(_active_local_peripherals):
        try:
            p._clear_all_callbacks_sync()
        except:
            pass

atexit.register(_global_cleanup)

class Descriptor:
    def __init__(self, internal_descriptor: simplepyble.Descriptor):
        self._internal = internal_descriptor

    def initialized(self) -> bool:
        return self._internal.initialized()

    def uuid(self) -> str:
        return self._internal.uuid()

class Characteristic:
    def __init__(self, internal_characteristic: simplepyble.Characteristic):
        self._internal = internal_characteristic

    def initialized(self) -> bool:
        return self._internal.initialized()

    def uuid(self) -> str:
        return self._internal.uuid()

    def descriptors(self) -> list[Descriptor]:
        return [Descriptor(d) for d in self._internal.descriptors()]

    def capabilities(self) -> list[str]:
        return self._internal.capabilities()

    def can_read(self) -> bool:
        return self._internal.can_read()

    def can_write_request(self) -> bool:
        return self._internal.can_write_request()

    def can_write_command(self) -> bool:
        return self._internal.can_write_command()

    def can_notify(self) -> bool:
        return self._internal.can_notify()

    def can_indicate(self) -> bool:
        return self._internal.can_indicate()

class Service:
    def __init__(self, internal_service: simplepyble.Service):
        self._internal = internal_service

    def initialized(self) -> bool:
        return self._internal.initialized()

    def uuid(self) -> str:
        return self._internal.uuid()

    def data(self) -> bytes:
        return self._internal.data()

    def characteristics(self) -> list[Characteristic]:
        return [Characteristic(c) for c in self._internal.characteristics()]

class Peripheral:
    def __init__(self, internal_peripheral: simplepyble.Peripheral):
        self._internal = internal_peripheral
        self._subscriptions: set[tuple] = set()
        self._pairing_setters: set[Callable[..., None]] = set()
        _active_peripherals.add(self)

    def initialized(self) -> bool:
        return self._internal.initialized()

    def identifier(self) -> str:
        return self._internal.identifier()

    def address(self) -> str:
        return self._internal.address()

    def address_type(self) -> simplepyble.BluetoothAddressType:
        return self._internal.address_type()

    def rssi(self) -> int:
        return self._internal.rssi()

    def tx_power(self) -> int:
        return self._internal.tx_power()

    def mtu(self) -> int:
        return self._internal.mtu()

    async def connect(self):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.connect)

    async def disconnect(self):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.disconnect)

    def is_connected(self) -> bool:
        return self._internal.is_connected()

    def is_connectable(self) -> bool:
        return self._internal.is_connectable()

    def is_paired(self) -> bool:
        return self._internal.is_paired()

    async def unpair(self):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.unpair)

    def services(self) -> list[Service]:
        return [Service(s) for s in self._internal.services()]

    def manufacturer_data(self) -> dict[int, bytes]:
        return self._internal.manufacturer_data()

    async def read(self, service_uuid: str, characteristic_uuid: str) -> bytes:
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.read, service_uuid, characteristic_uuid)

    async def write_request(self, service_uuid: str, characteristic_uuid: str, payload: bytes):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.write_request, service_uuid, characteristic_uuid, payload)

    async def write_command(self, service_uuid: str, characteristic_uuid: str, payload: bytes):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.write_command, service_uuid, characteristic_uuid, payload)

    async def notify(
        self,
        service_uuid: str,
        characteristic_uuid: str,
        callback: Callable[[bytes], Awaitable[None] | None],
    ):
        loop = asyncio.get_running_loop()
        self._subscriptions.add((service_uuid, characteristic_uuid))
        return await loop.run_in_executor(
            None, self._internal.notify, service_uuid, characteristic_uuid, lambda payload: _post(loop, callback, payload)
        )

    async def indicate(
        self,
        service_uuid: str,
        characteristic_uuid: str,
        callback: Callable[[bytes], Awaitable[None] | None],
    ):
        loop = asyncio.get_running_loop()
        self._subscriptions.add((service_uuid, characteristic_uuid))
        return await loop.run_in_executor(
            None, self._internal.indicate, service_uuid, characteristic_uuid, lambda payload: _post(loop, callback, payload)
        )

    async def unsubscribe(self, service_uuid: str, characteristic_uuid: str):
        loop = asyncio.get_running_loop()
        self._subscriptions.discard((service_uuid, characteristic_uuid))
        return await loop.run_in_executor(None, self._internal.unsubscribe, service_uuid, characteristic_uuid)

    async def descriptor_read(self, service_uuid: str, characteristic_uuid: str, descriptor_uuid: str) -> bytes:
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.descriptor_read, service_uuid, characteristic_uuid, descriptor_uuid)

    async def descriptor_write(self, service_uuid: str, characteristic_uuid: str, descriptor_uuid: str, payload: bytes):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.descriptor_write, service_uuid, characteristic_uuid, descriptor_uuid, payload)

    def set_callback_on_connected(self, callback: Callable[[], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_connected(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_connected(lambda: _post(loop, callback))

    def set_callback_on_disconnected(self, callback: Callable[[], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_disconnected(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_disconnected(lambda: _post(loop, callback))

    def _clear_all_callbacks_sync(self):
        """Internal sync cleanup for atexit or __aexit__"""
        try:
            self._internal.set_callback_on_connected(None)
            self._internal.set_callback_on_disconnected(None)
            for s, c in list(self._subscriptions):
                try:
                    self._internal.unsubscribe(s, c)
                except:
                    pass
            self._subscriptions.clear()
            for setter in list(self._pairing_setters):
                try:
                    setter(self._internal, None)
                except:
                    pass
            self._pairing_setters.clear()
        except:
            pass

    async def __aenter__(self):
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        # Pairing callbacks wait for this loop, so unregister them from another thread.
        loop = asyncio.get_running_loop()
        await loop.run_in_executor(None, self._clear_all_callbacks_sync)

class LocalCharacteristic:
    def __init__(self, internal_characteristic: simplepyble.LocalCharacteristic):
        self._internal = internal_characteristic

    def initialized(self) -> bool:
        return self._internal.initialized()

    def uuid(self) -> str:
        return self._internal.uuid()

    def capabilities(self) -> set[simplepyble.LocalCharacteristicCapability]:
        return self._internal.capabilities()

    def value(self) -> bytes:
        return self._internal.value()

    async def set_value(self, value: bytes | bytearray | memoryview):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.set_value, value)

    def set_callback_on_read(self, callback: Callable[[], Awaitable[bytes] | bytes] | None):
        if callback is None:
            self._internal.set_callback_on_read(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_read(lambda: _wait_on_loop(loop, callback))

    def set_callback_on_write(self, callback: Callable[[bytes], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_write(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_write(lambda value: _post(loop, callback, value))

    def set_callback_on_subscribed(self, callback: Callable[[], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_subscribed(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_subscribed(lambda: _post(loop, callback))

    def set_callback_on_unsubscribed(self, callback: Callable[[], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_unsubscribed(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_unsubscribed(lambda: _post(loop, callback))

    def _clear_all_callbacks_sync(self):
        """Internal sync cleanup for LocalPeripheral teardown"""
        self._internal.set_callback_on_read(None)
        self._internal.set_callback_on_write(None)
        self._internal.set_callback_on_subscribed(None)
        self._internal.set_callback_on_unsubscribed(None)

class LocalService:
    def __init__(self, internal_service: simplepyble.LocalService):
        self._internal = internal_service

    def initialized(self) -> bool:
        return self._internal.initialized()

    def uuid(self) -> str:
        return self._internal.uuid()

    def add_characteristic(
        self, uuid: str, capabilities: Iterable[simplepyble.LocalCharacteristicCapability]
    ) -> LocalCharacteristic:
        return LocalCharacteristic(self._internal.add_characteristic(uuid, capabilities))

    def characteristics(self) -> list[LocalCharacteristic]:
        return [LocalCharacteristic(c) for c in self._internal.characteristics()]

class LocalPeripheral:
    def __init__(self, internal_peripheral: simplepyble.LocalPeripheral):
        self._internal = internal_peripheral
        _active_local_peripherals.add(self)

    def initialized(self) -> bool:
        return self._internal.initialized()

    def add_advertised_service(self, service_uuids: str | list[str]):
        self._internal.add_advertised_service(service_uuids)

    def set_advertised_manufacturer_data(self, company_id: int, data: bytes | bytearray | memoryview):
        self._internal.set_advertised_manufacturer_data(company_id, data)

    def set_connectable(self, connectable: bool):
        self._internal.set_connectable(connectable)

    def add_service(self, uuid: str) -> LocalService:
        return LocalService(self._internal.add_service(uuid))

    def services(self) -> list[LocalService]:
        return [LocalService(s) for s in self._internal.services()]

    def remove_all_services(self):
        self._internal.remove_all_services()

    async def start(self):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.start)

    async def stop(self):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.stop)

    def is_started(self) -> bool:
        return self._internal.is_started()

    def is_advertising(self) -> bool:
        return self._internal.is_advertising()

    def set_callback_on_client_connected(self, callback: Callable[[str], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_client_connected(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_client_connected(lambda address: _post(loop, callback, address))

    def set_callback_on_client_disconnected(self, callback: Callable[[str], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_client_disconnected(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_client_disconnected(lambda address: _post(loop, callback, address))

    def _clear_all_callbacks_sync(self):
        """Internal sync cleanup for atexit or __aexit__"""
        try:
            self._internal.set_callback_on_client_connected(None)
            self._internal.set_callback_on_client_disconnected(None)
            for service in self._internal.services():
                for characteristic in service.characteristics():
                    LocalCharacteristic(characteristic)._clear_all_callbacks_sync()
            self._internal.stop()
        except:
            pass

    async def __aenter__(self):
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        # Read callbacks wait for this loop, so unregister them from another thread.
        loop = asyncio.get_running_loop()
        await loop.run_in_executor(None, self._clear_all_callbacks_sync)

class Adapter:
    def __init__(self, internal_adapter: simplepyble.Adapter):
        self._internal = internal_adapter
        _active_adapters.add(self)

    @classmethod
    def get_adapters(cls) -> list['Adapter']:
        return [cls(adapter) for adapter in simplepyble.Adapter.get_adapters()]

    def identifier(self) -> str:
        return self._internal.identifier()

    def address(self) -> str:
        return self._internal.address()

    def is_powered(self) -> bool:
        return self._internal.is_powered()
        
    def initialized(self) -> bool:
        return self._internal.initialized()
        
    @staticmethod
    def bluetooth_enabled() -> bool:
        return simplepyble.Adapter.bluetooth_enabled()

    async def power_on(self):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.power_on)

    async def power_off(self):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.power_off)

    async def scan_start(self):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.scan_start)

    async def scan_stop(self):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.scan_stop)
        
    async def scan_for(self, duration_ms: int):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, self._internal.scan_for, duration_ms)
        
    def scan_is_active(self) -> bool:
        return self._internal.scan_is_active()

    def scan_get_results(self) -> list[Peripheral]:
        return [Peripheral(p) for p in self._internal.scan_get_results()]
        
    def get_paired_peripherals(self) -> list[Peripheral]:
        return [Peripheral(p) for p in self._internal.get_paired_peripherals()]

    def get_connected_peripherals(self) -> list[Peripheral]:
        return [Peripheral(p) for p in self._internal.get_connected_peripherals()]

    def create_local_peripheral(self) -> LocalPeripheral:
        return LocalPeripheral(self._internal.create_local_peripheral())

    def set_callback_on_scan_start(self, callback: Callable[[], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_scan_start(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_scan_start(lambda: _post(loop, callback))

    def set_callback_on_scan_stop(self, callback: Callable[[], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_scan_stop(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_scan_stop(lambda: _post(loop, callback))

    def set_callback_on_scan_found(self, callback: Callable[[Peripheral], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_scan_found(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_scan_found(lambda peripheral: _post(loop, callback, Peripheral(peripheral)))

    def set_callback_on_scan_updated(self, callback: Callable[[Peripheral], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_scan_updated(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_scan_updated(lambda peripheral: _post(loop, callback, Peripheral(peripheral)))

    def set_callback_on_power_on(self, callback: Callable[[], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_power_on(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_power_on(lambda: _post(loop, callback))

    def set_callback_on_power_off(self, callback: Callable[[], Awaitable[None] | None] | None):
        if callback is None:
            self._internal.set_callback_on_power_off(None)
            return
        loop = asyncio.get_running_loop()
        self._internal.set_callback_on_power_off(lambda: _post(loop, callback))

    def _clear_all_callbacks_sync(self):
        """Internal sync cleanup for atexit or __aexit__"""
        try:
            self._internal.set_callback_on_scan_start(None)
            self._internal.set_callback_on_scan_stop(None)
            self._internal.set_callback_on_scan_found(None)
            self._internal.set_callback_on_scan_updated(None)
            self._internal.set_callback_on_power_on(None)
            self._internal.set_callback_on_power_off(None)
            if self._internal.scan_is_active():
                self._internal.scan_stop()
        except:
            pass

    async def __aenter__(self):
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        self._clear_all_callbacks_sync()

class Backend:
    def __init__(self, internal_backend: simplepyble.Backend):
        self._internal = internal_backend

    @classmethod
    def get_backends(cls) -> list['Backend']:
        return [cls(backend) for backend in simplepyble.Backend.get_backends()]

    @classmethod
    def get_native(cls) -> 'Backend':
        return cls(simplepyble.Backend.get_native())

    @classmethod
    def get_dongl(cls) -> 'Backend':
        return cls(simplepyble.Backend.get_dongl())

    @classmethod
    def get_simulator(cls) -> 'Backend':
        return cls(simplepyble.Backend.get_simulator())

    def initialized(self) -> bool:
        return self._internal.initialized()

    def adapters(self) -> list[Adapter]:
        return [Adapter(adapter) for adapter in self._internal.adapters()]

    def bluetooth_enabled(self) -> bool:
        return self._internal.bluetooth_enabled()

    def identifier(self) -> str:
        return self._internal.identifier()

class _MacOSAdvanced:
    def set_advertisement_local_name(self, peripheral: LocalPeripheral, local_name: str | None):
        simplepyble.advanced.macos.set_advertisement_local_name(peripheral._internal, local_name)

    async def retrieve_cached_peripherals(self, adapter: Adapter, identifiers: list[str]) -> list[Peripheral]:
        loop = asyncio.get_running_loop()
        peripherals = await loop.run_in_executor(
            None,
            simplepyble.advanced.macos.retrieve_cached_peripherals,
            adapter._internal,
            identifiers,
        )
        return [Peripheral(peripheral) for peripheral in peripherals]

class _LinuxAdvanced:
    def set_advertisement_local_name(self, peripheral: LocalPeripheral, local_name: str | None):
        simplepyble.advanced.linux.set_advertisement_local_name(peripheral._internal, local_name)

class _DonglAdvanced:
    def set_passkey_request_callback(
        self,
        peripheral: Peripheral,
        callback: Callable[[], Awaitable[str | None] | str | None] | None,
    ):
        setter = simplepyble.advanced.dongl.set_passkey_request_callback
        if callback is None:
            setter(peripheral._internal, None)
            peripheral._pairing_setters.discard(setter)
            return
        loop = asyncio.get_running_loop()
        setter(peripheral._internal, lambda: _wait_on_loop(loop, callback))
        peripheral._pairing_setters.add(setter)

    def set_passkey_display_callback(
        self,
        peripheral: Peripheral,
        callback: Callable[[str], Awaitable[None] | None] | None,
    ):
        setter = simplepyble.advanced.dongl.set_passkey_display_callback
        if callback is None:
            setter(peripheral._internal, None)
            peripheral._pairing_setters.discard(setter)
            return
        loop = asyncio.get_running_loop()
        setter(peripheral._internal, lambda passkey: _post(loop, callback, passkey))
        peripheral._pairing_setters.add(setter)

    def set_numeric_comparison_callback(
        self,
        peripheral: Peripheral,
        callback: Callable[[str], Awaitable[bool] | bool] | None,
    ):
        setter = simplepyble.advanced.dongl.set_numeric_comparison_callback
        if callback is None:
            setter(peripheral._internal, None)
            peripheral._pairing_setters.discard(setter)
            return
        loop = asyncio.get_running_loop()
        setter(peripheral._internal, lambda passkey: _wait_on_loop(loop, callback, passkey))
        peripheral._pairing_setters.add(setter)

    async def secure_connection(self, peripheral: Peripheral, mitm: bool = False):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, simplepyble.advanced.dongl.secure_connection, peripheral._internal, mitm)

class _Advanced:
    def __init__(self):
        self.dongl = _DonglAdvanced()
        if hasattr(simplepyble.advanced, "macos"):
            self.macos = _MacOSAdvanced()
        if hasattr(simplepyble.advanced, "linux"):
            self.linux = _LinuxAdvanced()

advanced = _Advanced()
