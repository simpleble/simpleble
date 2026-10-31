import { EventEmitter } from 'node:events';

import { HandlerSlot } from './handlers';
import { LocalPeripheral } from './local';
import { native, type AdapterHandle } from './native';
import { Peripheral } from './peripheral';
import { intern, registered, releaseFrom, retainWhileListening, unregister, type Releasable } from './registry';

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
  readonly #passkeyRequest = new HandlerSlot<(peer: string) => string | undefined | Promise<string | undefined>>();
  readonly #passkeyDisplay = new HandlerSlot<(peer: string, passkey: string) => void>();
  readonly #numericComparison = new HandlerSlot<(peer: string, passkey: string) => boolean | Promise<boolean>>();
  readonly #pairingAuthorization = new HandlerSlot<(peer: string) => boolean | Promise<boolean>>();
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
   * The peripherals CoreBluetooth already knows on macOS, by the identifiers
   * address() returned earlier, without scanning. Unknown identifiers are
   * skipped, and a returned peripheral might be out of range. Other platforms
   * reject with OPERATION_NOT_SUPPORTED.
   */
  async retrieveCachedPeripherals(identifiers: readonly string[]): Promise<Peripheral[]> {
    return (await this.#handle.retrieveCachedPeripherals(identifiers)).map(Peripheral.wrap);
  }

  /**
   * Answers a request for the passkey a peer shows while it pairs through this
   * adapter on Linux, with six digits, or undefined to reject pairing. Peers are
   * the peripherals this adapter connects to and the clients of its local
   * peripherals, and each handler receives the peer's address. A handler can
   * return a promise, e.g. of the user's input, and BlueZ waits for it.
   * Requests reach it once Config.simpleBluez.interactivePairing is set. Other
   * platforms and adapters reject with OPERATION_NOT_SUPPORTED.
   */
  setPasskeyRequestHandler(
    handler: ((peer: string) => string | undefined | Promise<string | undefined>) | null,
  ): Promise<void> {
    return this.#passkeyRequest.set(handler, id => this.#handle.setPasskeyRequestHandler(id));
  }

  /** Shows the six-digit passkey a peer pairing through this adapter on Linux asks the user to enter on it. */
  setPasskeyDisplayHandler(handler: ((peer: string, passkey: string) => void) | null): Promise<void> {
    return this.#passkeyDisplay.set(handler, id => this.#handle.setPasskeyDisplayHandler(id));
  }

  /**
   * Answers whether a peer pairing through this adapter on Linux shows the same
   * six-digit number, accepting or rejecting pairing.
   */
  setNumericComparisonHandler(
    handler: ((peer: string, passkey: string) => boolean | Promise<boolean>) | null,
  ): Promise<void> {
    return this.#numericComparison.set(handler, id => this.#handle.setNumericComparisonHandler(id));
  }

  /**
   * Accepts or rejects Just Works pairing with a peer through this adapter on
   * Linux. Without a handler, Just Works pairing is accepted.
   */
  setPairingAuthorizationHandler(handler: ((peer: string) => boolean | Promise<boolean>) | null): Promise<void> {
    return this.#pairingAuthorization.set(handler, id => this.#handle.setPairingAuthorizationHandler(id));
  }

  /** A host for GATT services of this process, advertised by this adapter. */
  createLocalPeripheral(): LocalPeripheral {
    return LocalPeripheral.wrap(this.#handle.createLocalPeripheral());
  }

  /**
   * Stops this adapter's scan and local peripherals, disconnects the
   * peripherals connected through it, removes its pairing handlers, and
   * releases the adapter and every object that came from it. Once the promise
   * resolves, a Dongl is free for other programs. Calls on the closed objects
   * then fail with OBJECT_NOT_INITIALIZED, they emit no more events, and
   * getAdapters() returns a new Adapter for the device.
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
    const hosts = this.#handle.hosts().map(LocalPeripheral.wrap);
    await Promise.allSettled([...connections.map(peripheral => peripheral.disconnect()), ...hosts.map(host => host.stop())]);

    // NOTE: BlueZ keeps its adapters after close(), so the pairing handlers are
    // removed from it rather than left rejecting its pairing requests.
    const pairing: Promise<void>[] = [];
    if (this.#passkeyRequest.isSet) pairing.push(this.setPasskeyRequestHandler(null));
    if (this.#passkeyDisplay.isSet) pairing.push(this.setPasskeyDisplayHandler(null));
    if (this.#numericComparison.isSet) pairing.push(this.setNumericComparisonHandler(null));
    if (this.#pairingAuthorization.isSet) pairing.push(this.setPairingAuthorizationHandler(null));
    await Promise.allSettled(pairing);

    for (const object of registered()) {
      (object as Partial<Releasable>)[releaseFrom]?.(key);
    }
    await this.#handle.close();
    this.removeAllListeners();
    unregister(key, this);
  }
}
