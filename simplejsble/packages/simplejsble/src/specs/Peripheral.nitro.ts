import { type HybridObject } from 'react-native-nitro-modules'

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
}
