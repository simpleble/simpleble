import asyncio
from typing import List, Callable, Optional, Union, Any
import simplepyble

class Adapter:
    def __init__(self, internal_adapter: simplepyble.Adapter):
        self._internal = internal_adapter

    @classmethod
    def get_adapters(cls) -> List['Adapter']:
        return [cls(adapter) for adapter in simplepyble.Adapter.get_adapters()]

    def identifier(self) -> str:
        return self._internal.identifier()

    def address(self) -> str:
        return self._internal.address()

    def is_powered(self) -> bool:
        return self._internal.is_powered()
        
    def initialized(self) -> bool:
        return self._internal.initialized()
        
    def bluetooth_enabled(self) -> bool:
        return self._internal.bluetooth_enabled()

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

    def scan_get_results(self):
        # Return raw peripherals for now, as requested to focus on Adapter first.
        return self._internal.scan_get_results()
        
    def get_paired_peripherals(self):
        return self._internal.get_paired_peripherals()

    def _wrap_callback(self, callback: Callable) -> Callable:
        loop = asyncio.get_running_loop()
        
        def wrapper(payload=None):
            # Inspect callback to see if it takes arguments
            # Note: simplepyble callbacks might differ in arguments.
            # set_callback_on_scan_found passes a peripheral.
            # set_callback_on_scan_start passes nothing.
            
            # We need to handle arguments correctly.
            # But the wrapper signature here must match what C++ expects?
            # No, C++ calls this wrapper. The wrapper is defined inside _wrap_callback.
            # Wait, `set_callback_on_scan_found` expects a callback that takes 1 arg.
            # `set_callback_on_scan_start` expects 0 args.
            # So I cannot use a generic wrapper with optional payload for all.
            # I need specific wrappers.
            pass
            
        return wrapper

    def set_callback_on_scan_start(self, callback: Callable[[], None]):
        loop = asyncio.get_running_loop()
        def wrapper():
            if asyncio.iscoroutinefunction(callback):
                asyncio.run_coroutine_threadsafe(callback(), loop)
            else:
                loop.call_soon_threadsafe(callback)
        self._internal.set_callback_on_scan_start(wrapper)

    def set_callback_on_scan_stop(self, callback: Callable[[], None]):
        loop = asyncio.get_running_loop()
        def wrapper():
            if asyncio.iscoroutinefunction(callback):
                asyncio.run_coroutine_threadsafe(callback(), loop)
            else:
                loop.call_soon_threadsafe(callback)
        self._internal.set_callback_on_scan_stop(wrapper)

    def set_callback_on_scan_found(self, callback: Callable[[Any], None]):
        loop = asyncio.get_running_loop()
        def wrapper(peripheral):
            if asyncio.iscoroutinefunction(callback):
                asyncio.run_coroutine_threadsafe(callback(peripheral), loop)
            else:
                loop.call_soon_threadsafe(callback, peripheral)
        self._internal.set_callback_on_scan_found(wrapper)

    def set_callback_on_scan_updated(self, callback: Callable[[Any], None]):
        loop = asyncio.get_running_loop()
        def wrapper(peripheral):
            if asyncio.iscoroutinefunction(callback):
                asyncio.run_coroutine_threadsafe(callback(peripheral), loop)
            else:
                loop.call_soon_threadsafe(callback, peripheral)
        self._internal.set_callback_on_scan_updated(wrapper)
