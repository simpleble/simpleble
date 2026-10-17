import { NitroModules } from 'react-native-nitro-modules';

import type { SimpleRnBLE } from './specs/SimpleRnBLE.nitro';

export type {
  AdapterEvent,
  AdapterHandle,
  BackendHandle,
  LocalCharacteristicEvent,
  LocalCharacteristicHandle,
  LocalPeripheralEvent,
  LocalPeripheralHandle,
  LocalServiceHandle,
  ManufacturerDataEntry,
  NativeService,
  PeripheralEvent,
  PeripheralHandle,
} from './specs/SimpleRnBLE.nitro';

/** @internal */
export const native = NitroModules.createHybridObject<SimpleRnBLE>('SimpleRnBLE');
