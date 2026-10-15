import { EventEmitter } from 'node:events';

import { native, type AdapterHandle } from './native';
import { Peripheral } from './peripheral';
import { intern, retainWhileListening } from './registry';

export interface AdapterEvents {
  scanStart: [];
  scanStop: [];
  scanFound: [peripheral: Peripheral];
  scanUpdated: [peripheral: Peripheral];
  powerOn: [];
  powerOff: [];
}

const EVENTS = ['scanStart', 'scanStop', 'scanFound', 'scanUpdated', 'powerOn', 'powerOff'] as const;

const construct = Symbol('construct');

/**
 * A Bluetooth adapter.
 *
 * There is one Adapter object per native adapter object. Most backends keep a
 * single native object per adapter, so every enumeration returns the same
 * Adapter. On Windows each enumeration creates a new native object, and
 * listeners on one Adapter only hear about calls made through it.
 *
 * Events are delivered on the JavaScript thread in the order they happened.
 * While an adapter has listeners it stays alive, even if nothing else
 * references it.
 */
export class Adapter extends EventEmitter<AdapterEvents> {
  readonly #handle: AdapterHandle;
  readonly #rehook: () => void;

  private constructor(token: typeof construct, handle: AdapterHandle) {
    if (token !== construct) throw new TypeError('Use Adapter.getAdapters()');
    super();
    this.#handle = handle;
    this.#rehook = retainWhileListening(this as EventEmitter, EVENTS);
    handle.subscribe();
  }

  /** @internal */
  static wrap(handle: AdapterHandle): Adapter {
    return intern(handle.key(), () => new Adapter(construct, handle));
  }

  /** Every adapter of every available backend. */
  static async getAdapters(): Promise<Adapter[]> {
    return (await native.getAdapters()).map(Adapter.wrap);
  }

  /** Whether Bluetooth is enabled on any adapter. */
  static bluetoothEnabled(): Promise<boolean> {
    return native.bluetoothEnabled();
  }

  identifier(): string {
    return this.#handle.identifier();
  }

  address(): string {
    return this.#handle.address();
  }

  isPowered(): Promise<boolean> {
    return this.#handle.isPowered();
  }

  powerOn(): Promise<void> {
    return this.#handle.powerOn();
  }

  powerOff(): Promise<void> {
    return this.#handle.powerOff();
  }

  scanStart(): Promise<void> {
    return this.#handle.scanStart();
  }

  scanStop(): Promise<void> {
    return this.#handle.scanStop();
  }

  /** Scans for `timeoutMs` milliseconds, then stops. Other calls on this adapter wait for it. */
  async scanFor(timeoutMs: number): Promise<void> {
    return this.#handle.scanFor(timeoutMs);
  }

  scanIsActive(): Promise<boolean> {
    return this.#handle.scanIsActive();
  }

  /** The peripherals seen by the current or last scan. */
  scanGetResults(): Peripheral[] {
    return this.#handle.scanGetResults().map(Peripheral.wrap);
  }

  getPairedPeripherals(): Promise<Peripheral[]> {
    return this.#handle.getPairedPeripherals().then(handles => handles.map(Peripheral.wrap));
  }

  getConnectedPeripherals(): Promise<Peripheral[]> {
    return this.#handle.getConnectedPeripherals().then(handles => handles.map(Peripheral.wrap));
  }

  override removeAllListeners(...args: Parameters<EventEmitter<AdapterEvents>['removeAllListeners']>): this {
    super.removeAllListeners(...args);
    this.#rehook();
    return this;
  }
}
