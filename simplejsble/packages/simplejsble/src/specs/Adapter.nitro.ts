import { type HybridObject } from 'react-native-nitro-modules'
import { type Peripheral } from './Peripheral.nitro'

/**
 * Wraps SimpleBLE::Adapter
 */
export interface Adapter
  extends HybridObject<{
    ios: 'c++'
    android: 'c++'
  }> {
  /**
   * Check if Bluetooth is enabled on the system.
   */
  bluetooth_enabled(): boolean

  /**
   * Retrieve a list of all available Bluetooth adapters.
   */
  get_adapters(): Adapter[]

  /**
   * Check if the adapter is initialized (has a valid internal handle).
   */
  initialized(): boolean

  /**
   * Human-readable identifier for the adapter.
   */
  identifier(): string

  /**
   * Bluetooth address of the adapter (MAC on Android/Linux, identifier on IOS/Macos).
   */
  address(): string

  /**
   * Check if the adapter is currently powered on.
   */
  is_powered(): boolean

  /**
   * Start scanning for peripherals.
   */
  scan_start(): void

  /**
   * Stop scanning for peripherals.
   */
  scan_stop(): void

  /**
   * Scan for peripherals for a specified duration (blocking).
   * @param timeout_ms Duration in milliseconds.
   */
  scan_for(timeout_ms: number): void

  /**
   * Check if a scan is currently in progress.
   */
  scan_is_active(): boolean

  /**
   * Get the list of peripherals discovered during the last scan.
   */
  scan_get_results(): Peripheral[]

  /**
   * Set callback to be invoked when scanning starts.
   */
  set_callback_on_scan_start(callback: () => void): void

  /**
   * Set callback to be invoked when scanning stops.
   */
  set_callback_on_scan_stop(callback: () => void): void

  /**
   * Set callback to be invoked when a known peripheral's advertisement is updated.
   */
  set_callback_on_scan_updated(callback: (peripheral: Peripheral) => void): void

  /**
   * Set callback to be invoked when a new peripheral is discovered.
   */
  set_callback_on_scan_found(callback: (peripheral: Peripheral) => void): void

  /**
   * Retrieve a list of all paired peripherals.
   * - Android: Supported (uses BluetoothAdapter.getBondedDevices())
   * - iOS: Not supported - CoreBluetooth does not expose a direct API to list paired devices.
   *   Pairing is handled at the OS level, and CoreBluetooth only exposes devices discovered via scanning
   *   or those you've connected to.
   */
  get_paired_peripherals(): Peripheral[]

  /**
   * Retrieve a list of all connected peripherals.
   * Currently only implemented on Windows.
   * Note: This could potentially be implemented on Android/iOS but is not yet available.
   */
  get_connected_peripherals(): Peripheral[]
}
