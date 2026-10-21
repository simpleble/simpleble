import type { EventEmitter } from 'node:events';

import { SimpleBLEError, type ErrorCode } from './errors';
import { answer } from './handlers';
import { native } from './native';
import { Peripheral, deliver } from './peripheral';
import { lookup, onNativeEvent } from './registry';

interface NativeTarget extends EventEmitter {
  [onNativeEvent]?(event: string): void;
}

native.setup({
  makeError: (code, message, nativeCode) => new SimpleBLEError(code as ErrorCode, message, nativeCode),
  // `argument` is the event's value, or a peripheral's key when `external` holds the peripheral.
  dispatch: (key, event, argument, external) => {
    // Objects with listeners are kept alive, so a missing object had none.
    const target = lookup(key) as NativeTarget | undefined;
    if (!target) return;

    target[onNativeEvent]?.(event);
    if (target.listenerCount(event) === 0) return;

    if (external !== undefined) {
      const peripheral =
        (lookup(argument as string) as Peripheral | undefined) ?? Peripheral.wrap(native.peripheralHandle(external));
      target.emit(event, peripheral);
    } else if (argument === undefined) {
      target.emit(event);
    } else {
      target.emit(event, argument);
    }
  },
  deliver,
  answer,
});

export { Adapter, type AdapterEvents } from './adapter';
export { Backend } from './backend';
export { Config, type AndroidConfig, type DonglConfig, type SimpleBluezConfig, type WinRTConfig } from './config';
export { SimpleBLEError, type ErrorCode } from './errors';
export { Logging, type LogEntry, type LogLevel } from './logging';
export {
  LocalCharacteristic,
  LocalPeripheral,
  LocalService,
  type LocalCharacteristicEvents,
  type LocalPeripheralEvents,
} from './local';
export {
  Peripheral,
  type BluetoothAddressType,
  type Characteristic,
  type CharacteristicCapability,
  type Descriptor,
  type PeripheralEvents,
  type Service,
} from './peripheral';
