import type { EventEmitter } from 'node:events';

import { SimpleBLEError, type ErrorCode } from './errors';
import { native } from './native';
import { Peripheral, deliver } from './peripheral';
import { lookup, onNativeEvent } from './registry';

interface NativeTarget extends EventEmitter {
  [onNativeEvent]?(event: string): void;
}

native.setup({
  makeError: (code, message, nativeCode) => new SimpleBLEError(code as ErrorCode, message, nativeCode),
  dispatch: (key, event, peripheralKey, external) => {
    // Objects with listeners are kept alive, so a missing object had none.
    const target = lookup(key) as NativeTarget | undefined;
    if (!target) return;

    target[onNativeEvent]?.(event);
    if (target.listenerCount(event) === 0) return;

    if (peripheralKey === undefined) {
      target.emit(event);
      return;
    }
    const peripheral =
      (lookup(peripheralKey) as Peripheral | undefined) ?? Peripheral.wrap(native.peripheralHandle(external!));
    target.emit(event, peripheral);
  },
  deliver,
});

export { Adapter, type AdapterEvents } from './adapter';
export { Backend } from './backend';
export { Config, type DonglConfig, type SimpleBluezConfig, type WinRTConfig } from './config';
export { SimpleBLEError, type ErrorCode } from './errors';
export {
  Peripheral,
  type BluetoothAddressType,
  type Characteristic,
  type CharacteristicCapability,
  type Descriptor,
  type PeripheralEvents,
  type Service,
} from './peripheral';
