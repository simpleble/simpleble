import { type HybridObject } from 'react-native-nitro-modules'
import { type Descriptor } from './Descriptor.nitro'

export interface Characteristic
  extends HybridObject<{
    ios: 'c++'
    android: 'c++'
  }> {
  /**
   * Check if the characteristic is initialized (has a valid internal handle).
   */
  initialized(): boolean

  /**
   * Get the characteristic UUID.
   */
  uuid(): string

  /**
   * Get all descriptors belonging to this characteristic.
   */
  descriptors(): Descriptor[]

  /**
   * Get a list of capability strings for this characteristic.
   * Example: ["read", "write", "notify"]
   */
  capabilities(): string[]

  /**
   * Check if the characteristic supports read operations.
   */
  can_read(): boolean

  /**
   * Check if the characteristic supports write-request operations.
   */
  can_write_request(): boolean

  /**
   * Check if the characteristic supports write-command operations.
   */
  can_write_command(): boolean

  /**
   * Check if the characteristic supports notify operations.
   */
  can_notify(): boolean

  /**
   * Check if the characteristic supports indicate operations.
   */
  can_indicate(): boolean
}
