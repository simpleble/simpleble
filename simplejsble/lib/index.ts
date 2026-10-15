import type { EventEmitter } from 'node:events';

import { SimpleBLEError, type ErrorCode } from './errors';
import { native } from './native';
import { Peripheral } from './peripheral';
import { lookup } from './registry';

native.setup({
  makeError: (code, message, nativeCode) => new SimpleBLEError(code as ErrorCode, message, nativeCode),
  dispatch: (key, event, peripheralKey) => {
    // Objects with listeners are kept alive, so a missing object had none.
    const target = lookup(key) as EventEmitter | undefined;
    if (!target || target.listenerCount(event) === 0) return;

    if (peripheralKey === undefined) {
      target.emit(event);
      return;
    }
    const peripheral = (lookup(peripheralKey) as Peripheral | undefined) ?? Peripheral.wrap(native.eventPeripheral());
    target.emit(event, peripheral);
  },
});

export { Adapter, type AdapterEvents } from './adapter';
export { Backend } from './backend';
export { Config, type DonglConfig, type SimpleBluezConfig, type WinRTConfig } from './config';
export { SimpleBLEError, type ErrorCode } from './errors';
export { Peripheral, type BluetoothAddressType } from './peripheral';
