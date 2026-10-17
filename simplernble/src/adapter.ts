import { async, sync } from './errors';
import { native, type AdapterEvent, type AdapterHandle, type PeripheralHandle } from './native';
import { LocalPeripheral } from './local';
import { Peripheral } from './peripheral';
import { intern, NativeEmitter } from './registry';

export interface AdapterEvents {
  scanStart: [];
  scanStop: [];
  scanFound: [peripheral: Peripheral];
  scanUpdated: [peripheral: Peripheral];
  powerOn: [];
  powerOff: [];
}

const construct = Symbol('construct');

/**
 * A Bluetooth adapter. Every enumeration returns the same Adapter object for an
 * adapter, and events arrive in the order they happened.
 */
export class Adapter extends NativeEmitter<AdapterEvents> {
  readonly #handle: AdapterHandle;

  private constructor(token: typeof construct, handle: AdapterHandle) {
    if (token !== construct) throw new TypeError('Use Adapter.getAdapters()');
    super();
    this.#handle = handle;
    handle.subscribe(
      this.weakly((self, event: AdapterEvent, peripheral: PeripheralHandle | undefined) => {
        if (peripheral) {
          self.emit(event as 'scanFound' | 'scanUpdated', Peripheral.wrap(peripheral));
        } else {
          self.emit(event as 'scanStart' | 'scanStop' | 'powerOn' | 'powerOff');
        }
      }),
    );
  }

  /** @internal */
  static wrap(handle: AdapterHandle): Adapter {
    return intern(handle, h => new Adapter(construct, h));
  }

  /** Every adapter of every available backend. */
  static async getAdapters(): Promise<Adapter[]> {
    return (await async(() => native.getAdapters())).map(Adapter.wrap);
  }

  /** Whether Bluetooth is enabled on any adapter. */
  static bluetoothEnabled(): Promise<boolean> {
    return async(() => native.bluetoothEnabled());
  }

  identifier(): string {
    return sync(() => this.#handle.identifier());
  }

  address(): string {
    return sync(() => this.#handle.address());
  }

  isPowered(): Promise<boolean> {
    return async(() => this.#handle.isPowered());
  }

  powerOn(): Promise<void> {
    return async(() => this.#handle.powerOn());
  }

  powerOff(): Promise<void> {
    return async(() => this.#handle.powerOff());
  }

  scanStart(): Promise<void> {
    return async(() => this.#handle.scanStart());
  }

  scanStop(): Promise<void> {
    return async(() => this.#handle.scanStop());
  }

  /** Scans for `timeoutMs` milliseconds, then stops. Other calls on this adapter wait for it. */
  async scanFor(timeoutMs: number): Promise<void> {
    if (typeof timeoutMs !== 'number') throw new TypeError('timeoutMs must be a number');
    if (!Number.isInteger(timeoutMs) || timeoutMs < 0 || timeoutMs > 2 ** 31 - 1) {
      throw new RangeError('timeoutMs must be an integer from 0 to 2147483647');
    }
    return async(() => this.#handle.scanFor(timeoutMs));
  }

  scanIsActive(): Promise<boolean> {
    return async(() => this.#handle.scanIsActive());
  }

  /** The peripherals seen by the current or last scan. */
  scanGetResults(): Peripheral[] {
    return sync(() => this.#handle.scanGetResults()).map(Peripheral.wrap);
  }

  async getPairedPeripherals(): Promise<Peripheral[]> {
    return (await async(() => this.#handle.getPairedPeripherals())).map(Peripheral.wrap);
  }

  async getConnectedPeripherals(): Promise<Peripheral[]> {
    return (await async(() => this.#handle.getConnectedPeripherals())).map(Peripheral.wrap);
  }

  /**
   * The peripherals CoreBluetooth already knows on iOS, by the identifiers
   * address() returned earlier, without scanning. Unknown identifiers are
   * skipped, and a returned peripheral might be out of range. Android rejects
   * with OPERATION_NOT_SUPPORTED.
   */
  async retrieveCachedPeripherals(identifiers: readonly string[]): Promise<Peripheral[]> {
    if (!Array.isArray(identifiers) || !identifiers.every(item => typeof item === 'string')) {
      throw new TypeError('identifiers must be an array of strings');
    }
    return (await async(() => this.#handle.retrieveCachedPeripherals([...identifiers]))).map(Peripheral.wrap);
  }

  /** A host for GATT services of this app, advertised by this adapter. */
  createLocalPeripheral(): LocalPeripheral {
    return LocalPeripheral.wrap(sync(() => this.#handle.createLocalPeripheral()));
  }
}
