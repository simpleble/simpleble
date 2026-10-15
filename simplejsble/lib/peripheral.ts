import type { PeripheralHandle } from './native';
import { intern } from './registry';

export type BluetoothAddressType = 'public' | 'random' | 'unspecified';

const construct = Symbol('construct');

/**
 * A remote device. There is one Peripheral object per native peripheral object,
 * so an Adapter's events, scan results and peripheral lists return the same
 * object for the same device.
 */
export class Peripheral {
  readonly #handle: PeripheralHandle;

  private constructor(token: typeof construct, handle: PeripheralHandle) {
    if (token !== construct) throw new TypeError('Peripherals come from an Adapter');
    this.#handle = handle;
  }

  /** @internal */
  static wrap(handle: PeripheralHandle): Peripheral {
    return intern(handle.key(), () => new Peripheral(construct, handle));
  }

  identifier(): string {
    return this.#handle.identifier();
  }

  address(): string {
    return this.#handle.address();
  }

  addressType(): BluetoothAddressType {
    return this.#handle.addressType();
  }

  /** The last received signal strength in dBm, if any. */
  rssi(): number | undefined {
    return this.#handle.rssi();
  }

  /** The advertised transmit power in dBm, if any. */
  txPower(): number | undefined {
    return this.#handle.txPower();
  }

  mtu(): number {
    return this.#handle.mtu();
  }

  isConnected(): boolean {
    return this.#handle.isConnected();
  }

  isConnectable(): boolean {
    return this.#handle.isConnectable();
  }

  isPaired(): Promise<boolean> {
    return this.#handle.isPaired();
  }

  /** Advertised manufacturer data by company identifier. The buffers are copies. */
  manufacturerData(): Map<number, Buffer> {
    return new Map(this.#handle.manufacturerData());
  }
}
