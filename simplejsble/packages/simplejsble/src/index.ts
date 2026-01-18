import { NitroModules } from "react-native-nitro-modules";
import type { Adapter } from "./specs/Adapter.nitro";
import type { Peripheral, BluetoothAddressType } from "./specs/Peripheral.nitro";
import type { Service } from "./specs/Service.nitro";
import type { Characteristic } from "./specs/Characteristic.nitro";
import type { Descriptor } from "./specs/Descriptor.nitro";

export type { Adapter, Peripheral, BluetoothAddressType, Service, Characteristic, Descriptor };

export { fromHex, toHex, toString, fromString } from "./utils/bytearray";

export const HybridAdapter = NitroModules.createHybridObject<Adapter>("Adapter");