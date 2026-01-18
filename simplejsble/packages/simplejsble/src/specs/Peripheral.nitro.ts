import { type HybridObject } from 'react-native-nitro-modules'
import { type Service } from './Service.nitro'

/**
 * Bluetooth address type enumeration.
 * Maps to SimpleBLE::BluetoothAddressType
 */
export type BluetoothAddressType = 'public' | 'random' | 'unspecified'

/**
 * Minimal wrapper for SimpleBLE::Peripheral.
 * Exposes core properties and connection methods without
 * introducing Service/ByteArray types.
 */
export interface Peripheral
  extends HybridObject<{
    ios: 'c++'
    android: 'c++'
  }> {
  /**
   * Check if the peripheral is initialized (has a valid internal handle).
   */
  initialized(): boolean

  /**
   * Human- able identifier (device name or fallback).
   */
  identifier(): string

  /**
   * Bluetooth address (MAC on Android, UUID on IOS/Macos).
   */
  address(): string

  /**
   * Address type (public, random, or unspecified).
   */
  address_type(): BluetoothAddressType

  /**
   * Received Signal Strength Indicator in dBm.
   */
  rssi(): number

  /**
   * Advertised transmit power in dBm.
   * Returns -32768 if not advertised.
   */
  tx_power(): number

  /**
   * Maximum Transmission Unit size.
   */
  mtu(): number

  /**
   * Initiate a connection to the peripheral.
   */
  connect(): void

  /**
   * Disconnect from the peripheral.
   */
  disconnect(): void

  /**
   * Check if the peripheral is currently connected.
   */
  is_connected(): boolean

  /**
   * Check if the peripheral is connectable.
   */
  is_connectable(): boolean

  /**
   * Check if the peripheral is paired with the system.
   */
  is_paired(): boolean

  /**
   * Remove the pairing/bond with the peripheral.
   */
  unpair(): void

  /**
   * Set callback to be invoked when a connection is established.
   */
  set_callback_on_connected(callback: () => void): void

  /**
   * Set callback to be invoked when the peripheral disconnects.
   */
  set_callback_on_disconnected(callback: () => void): void

  /**
   * Get all services available on the peripheral.
   * Requires an active connection.
   */
  services(): Service[]

  /**
   * Get manufacturer data from the peripheral's advertisement.
   * Returns a map of manufacturer ID (number) to data (ArrayBuffer).
   */
  manufacturer_data(): Record<number, ArrayBuffer>

  /**
   * Read a characteristic value.
   * Requires an active connection.
   * @param service Service UUID
   * @param characteristic Characteristic UUID
   * @returns ArrayBuffer containing the characteristic data
   */
  read(service: string, characteristic: string): ArrayBuffer

  /**
   * Write to a characteristic using write-request (with response).
   * Requires an active connection.
   * @param service Service UUID
   * @param characteristic Characteristic UUID
   * @param data Data to write as ArrayBuffer
   */
  write_request(service: string, characteristic: string, data: ArrayBuffer): void

  /**
   * Write to a characteristic using write-command (without response).
   * Requires an active connection.
   * @param service Service UUID
   * @param characteristic Characteristic UUID
   * @param data Data to write as ArrayBuffer
   */
  write_command(service: string, characteristic: string, data: ArrayBuffer): void

  /**
   * Subscribe to notifications from a characteristic.
   * Requires an active connection.
   * @param service Service UUID
   * @param characteristic Characteristic UUID
   * @param callback Callback to receive notification data as ArrayBuffer
   */
  notify(service: string, characteristic: string, callback: (data: ArrayBuffer) => void): void

  /**
   * Subscribe to indications from a characteristic.
   * Requires an active connection.
   * @param service Service UUID
   * @param characteristic Characteristic UUID
   * @param callback Callback to receive indication data as ArrayBuffer
   */
  indicate(service: string, characteristic: string, callback: (data: ArrayBuffer) => void): void

  /**
   * Unsubscribe from notifications or indications on a characteristic.
   * Requires an active connection.
   * @param service Service UUID
   * @param characteristic Characteristic UUID
   */
  unsubscribe(service: string, characteristic: string): void

  /**
   * Read a descriptor value.
   * Requires an active connection.
   * @param service Service UUID
   * @param characteristic Characteristic UUID
   * @param descriptor Descriptor UUID
   * @returns ArrayBuffer containing the descriptor data
   */
  read_descriptor(service: string, characteristic: string, descriptor: string): ArrayBuffer

  /**
   * Write to a descriptor.
   * Requires an active connection.
   * @param service Service UUID
   * @param characteristic Characteristic UUID
   * @param descriptor Descriptor UUID
   * @param data Data to write as ArrayBuffer
   */
  write_descriptor(service: string, characteristic: string, descriptor: string, data: ArrayBuffer): void
}
