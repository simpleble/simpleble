import { type HybridObject } from 'react-native-nitro-modules'

export type BluetoothAddress = string

export type BluetoothUUID = string

export interface Adapter extends HybridObject<{ ios: 'c++'; android: 'c++' }> {
  getAdapters(): Adapter[]
}
