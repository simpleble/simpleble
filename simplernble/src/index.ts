import { NitroModules } from "react-native-nitro-modules";
import type { Adapter as AdapterInterface } from "./specs/Adapter.nitro";
import type { Peripheral as PeripheralInterface, BluetoothAddressType } from "./specs/Peripheral.nitro";
import type { Service as ServiceInterface } from "./specs/Service.nitro";
import type { Characteristic as CharacteristicInterface } from "./specs/Characteristic.nitro";
import type { Descriptor as DescriptorInterface } from "./specs/Descriptor.nitro";

export type { BluetoothAddressType };

export { fromHex, toHex, toString, fromString } from "./utils/bytearray";

export const Adapter = NitroModules.createHybridObject<AdapterInterface>("Adapter");
export const Peripheral = NitroModules.createHybridObject<PeripheralInterface>("Peripheral");
export const Service = NitroModules.createHybridObject<ServiceInterface>("Service");
export const Characteristic = NitroModules.createHybridObject<CharacteristicInterface>("Characteristic");
export const Descriptor = NitroModules.createHybridObject<DescriptorInterface>("Descriptor");