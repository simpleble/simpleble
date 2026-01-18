import { NitroModules } from "react-native-nitro-modules";
import type { Adapter } from "./specs/Adapter.nitro";
import type { Peripheral, BluetoothAddressType } from "./specs/Peripheral.nitro";

export type { Adapter, Peripheral, BluetoothAddressType };

export const HybridAdapter = NitroModules.createHybridObject<Adapter>("Adapter");