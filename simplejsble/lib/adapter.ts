import { EventEmitter } from 'node:events';

import { native, type AdapterHandle } from './native';
import { Peripheral, releaseFrom } from './peripheral';
import { intern, registered, retainWhileListening, unregister } from './registry';

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
 * references it. close() releases it and its peripherals right away.
 */
export class Adapter extends EventEmitter<AdapterEvents> {
  readonly #handle: AdapterHandle;
  readonly #rehook: () => void;
  #closing?: Promise<void>;

  private constructor(token: typeof construct, handle: AdapterHandle) {
    if (token !== construct) throw new TypeError('Use Adapter.getAdapters()');
    super();
    this.#handle = handle;
    this.#rehook = retainWhileListening(this as EventEmitter, EVENTS);
    handle.subscribe();
  }

  /** @internal */
  static wrap(handle: AdapterHandle): Adapter {
    return intern(handle.key(), () => new Adapter(construct, handle), () => handle.release());
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

  /**
   * Stops this adapter's scan, disconnects the peripherals connected through
   * it, and releases the adapter and its peripherals. Once the promise
   * resolves, a Dongl is free for other programs. Calls on the closed adapter
   * and its peripherals then fail with OBJECT_NOT_INITIALIZED, they emit no
   * more events, and getAdapters() returns a new Adapter for the device.
   */
  close(): Promise<void> {
    this.#closing ??= this.#close();
    return this.#closing;
  }

  async [Symbol.asyncDispose](): Promise<void> {
    await this.close();
  }

  override removeAllListeners(...args: Parameters<EventEmitter<AdapterEvents>['removeAllListeners']>): this {
    super.removeAllListeners(...args);
    this.#rehook();
    return this;
  }

  async #close(): Promise<void> {
    const key = this.#handle.key();

    // NOTE: A Dongl whose port closes during a scan can stop answering until
    // it's replugged, so the scan and the connections end first. Failures
    // don't stop the release.
    try {
      if (await this.#handle.scanIsActive()) await this.#handle.scanStop();
    } catch {}
    const connections = this.#handle.connections().map(Peripheral.wrap);
    await Promise.allSettled(connections.map(peripheral => peripheral.disconnect()));

    for (const object of registered()) {
      if (object instanceof Peripheral) object[releaseFrom](key);
    }
    await this.#handle.close();
    this.removeAllListeners();
    unregister(key, this);
  }
}
