import { EventEmitter } from 'node:events';

import { HandlerSlot } from './handlers';
import type { NativeService, PeripheralHandle } from './native';
import {
  intern,
  onNativeEvent,
  release,
  releaseFrom,
  retain,
  retainWhileListening,
  unregister,
} from './registry';

export type BluetoothAddressType = 'public' | 'random' | 'unspecified';

export type CharacteristicCapability = 'read' | 'writeRequest' | 'writeCommand' | 'notify' | 'indicate';

export interface Descriptor {
  readonly uuid: string;
}

export interface Characteristic {
  readonly uuid: string;
  readonly capabilities: readonly CharacteristicCapability[];
  readonly descriptors: readonly Descriptor[];
}

export interface Service {
  readonly uuid: string;
  /** Advertised service data. */
  readonly data: Buffer;
  readonly characteristics: readonly Characteristic[];
}

export interface PeripheralEvents {
  connected: [];
  disconnected: [];
}

const EVENTS = ['connected', 'disconnected'] as const;

const construct = Symbol('construct');

// Notification and indication callbacks by subscription id. The addon passes
// only the id with each value.
const callbacks = new Map<number, (value: Buffer) => void>();
let nextId = 1;

interface Subscription {
  readonly id: number;
  // Whether the call that made it has succeeded.
  settled: boolean;
}

// NOTE: Some backends, like CoreBluetooth, match UUIDs regardless of case, and
// the others match only lowercase ones. Lowercase keys name the characteristic
// the backend uses either way.
function subscriptionKey(service: string, characteristic: string): string {
  return `${service}\n${characteristic}`.toLowerCase();
}

/** @internal */
export function deliver(id: number, value: Buffer): void {
  callbacks.get(id)?.(value);
}

function freeze(services: NativeService[]): Service[] {
  for (const service of services) {
    for (const characteristic of service.characteristics) {
      for (const descriptor of characteristic.descriptors) Object.freeze(descriptor);
      Object.freeze(characteristic.descriptors);
      Object.freeze(characteristic.capabilities);
      Object.freeze(characteristic);
    }
    Object.freeze(service.characteristics);
    Object.freeze(service);
  }
  return services as Service[];
}

/**
 * A remote device. There is one Peripheral object per native peripheral object,
 * so an Adapter's events, scan results and peripheral lists return the same
 * object for the same device.
 *
 * A peripheral with listeners or subscriptions stays alive, even if nothing
 * else references it. An open connection keeps Node.js running until it ends.
 */
export class Peripheral extends EventEmitter<PeripheralEvents> {
  readonly #handle: PeripheralHandle;
  readonly #rehook: () => void;
  // Subscriptions by subscriptionKey().
  readonly #subscriptions = new Map<string, Subscription>();
  readonly #passkeyRequest = new HandlerSlot<() => string | undefined | Promise<string | undefined>>();
  readonly #passkeyDisplay = new HandlerSlot<(passkey: string) => void>();
  readonly #numericComparison = new HandlerSlot<(passkey: string) => boolean | Promise<boolean>>();

  private constructor(token: typeof construct, handle: PeripheralHandle) {
    if (token !== construct) throw new TypeError('Peripherals come from an Adapter');
    super();
    this.#handle = handle;
    this.#rehook = retainWhileListening(this as EventEmitter, EVENTS);
    handle.subscribe();
  }

  /** @internal */
  static wrap(handle: PeripheralHandle): Peripheral {
    return intern(handle.key(), () => new Peripheral(construct, handle), () => handle.release());
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

  connect(): Promise<void> {
    return this.#handle.connect();
  }

  async disconnect(): Promise<void> {
    // Calls run in call order, so this ends the subscriptions made before it,
    // and not those of calls made while it runs.
    const ended = [...this.#subscriptions];
    await this.#handle.disconnect();
    for (const [key, subscription] of ended) this.#forget(key, subscription);
  }

  unpair(): Promise<void> {
    return this.#handle.unpair();
  }

  /** The services of a connected peripheral, or the advertised ones before connecting. */
  services(): Service[] {
    return freeze(this.#handle.services());
  }

  async read(service: string, characteristic: string): Promise<Buffer> {
    return this.#handle.read(service, characteristic);
  }

  async writeRequest(service: string, characteristic: string, data: Uint8Array): Promise<void> {
    return this.#handle.writeRequest(service, characteristic, data);
  }

  async writeCommand(service: string, characteristic: string, data: Uint8Array): Promise<void> {
    return this.#handle.writeCommand(service, characteristic, data);
  }

  async readDescriptor(service: string, characteristic: string, descriptor: string): Promise<Buffer> {
    return this.#handle.readDescriptor(service, characteristic, descriptor);
  }

  async writeDescriptor(service: string, characteristic: string, descriptor: string, data: Uint8Array): Promise<void> {
    return this.#handle.writeDescriptor(service, characteristic, descriptor, data);
  }

  /** Calls `callback` with each notified value until unsubscribe() or a disconnection. */
  notify(service: string, characteristic: string, callback: (value: Buffer) => void): Promise<void> {
    return this.#subscribe(id => this.#handle.notify(service, characteristic, id), service, characteristic, callback);
  }

  /** Calls `callback` with each indicated value until unsubscribe() or a disconnection. */
  indicate(service: string, characteristic: string, callback: (value: Buffer) => void): Promise<void> {
    return this.#subscribe(id => this.#handle.indicate(service, characteristic, id), service, characteristic, callback);
  }

  async unsubscribe(service: string, characteristic: string): Promise<void> {
    // Like disconnect(), this ends only the subscription made before it.
    const key = subscriptionKey(service, characteristic);
    const ended = this.#subscriptions.get(key);
    await this.#handle.unsubscribe(service, characteristic);
    if (ended) this.#forget(key, ended);
  }

  /**
   * Answers a Dongl's request for the passkey the peer shows, with six digits,
   * or undefined to reject pairing. Set it before connect(), as pairing can
   * start as soon as the link is up. The handler can return a promise, e.g. of
   * the user's input, and the Dongl waits for it.
   */
  setPasskeyRequestHandler(handler: (() => string | undefined | Promise<string | undefined>) | null): Promise<void> {
    return this.#passkeyRequest.set(handler, id => this.#handle.setPasskeyRequestHandler(id));
  }

  /** Shows a Dongl's six-digit passkey, which the user enters on the peer. Set it before connect(). */
  setPasskeyDisplayHandler(handler: ((passkey: string) => void) | null): Promise<void> {
    return this.#passkeyDisplay.set(handler, id => this.#handle.setPasskeyDisplayHandler(id));
  }

  /**
   * Answers whether the peer shows the same six-digit number as a Dongl,
   * accepting or rejecting pairing. Set it before connect(). The handler can
   * return a promise.
   */
  setNumericComparisonHandler(handler: ((passkey: string) => boolean | Promise<boolean>) | null): Promise<void> {
    return this.#numericComparison.set(handler, id => this.#handle.setNumericComparisonHandler(id));
  }

  /**
   * Secures a Dongl connection with LE Secure Connections. With `mitm`, the
   * peer must also be authenticated through the passkey or numeric comparison
   * handlers. Pairing started by this call creates no bond, and the Dongl
   * disconnects if it fails.
   */
  async secureConnection(mitm = false): Promise<void> {
    return this.#handle.secureConnection(mitm);
  }

  override removeAllListeners(...args: Parameters<EventEmitter<PeripheralEvents>['removeAllListeners']>): this {
    super.removeAllListeners(...args);
    this.#rehook();
    return this;
  }

  /** @internal */
  [releaseFrom](adapterKey: string): void {
    if (this.#handle.owner() !== adapterKey) return;

    const key = this.#handle.key();
    for (const [subscriptionKey, subscription] of [...this.#subscriptions]) this.#forget(subscriptionKey, subscription);
    for (const slot of [this.#passkeyRequest, this.#passkeyDisplay, this.#numericComparison]) slot.drop();
    this.removeAllListeners();
    this.#handle.release();
    unregister(key, this);
  }

  /** @internal */
  [onNativeEvent](event: string): void {
    if (event !== 'disconnected') return;

    // A disconnection ends the subscriptions made before it. One still being
    // made either started after it or fails, which forgets it.
    for (const [key, subscription] of [...this.#subscriptions]) {
      if (subscription.settled) this.#forget(key, subscription);
    }
  }

  async #subscribe(
    start: (id: number) => Promise<void>,
    service: string,
    characteristic: string,
    callback: (value: Buffer) => void,
  ): Promise<void> {
    if (typeof callback !== 'function') throw new TypeError('callback must be a function');

    // The callback is in place before the first value can arrive. A new
    // subscription to the same characteristic replaces the previous one.
    const key = subscriptionKey(service, characteristic);
    const subscription: Subscription = { id: nextId++, settled: false };
    this.#remember(key, subscription, callback);
    try {
      await start(subscription.id);
    } catch (error) {
      this.#forget(key, subscription);
      throw error;
    }
    subscription.settled = true;
  }

  #remember(key: string, subscription: Subscription, callback: (value: Buffer) => void): void {
    const previous = this.#subscriptions.get(key);
    if (previous) {
      callbacks.delete(previous.id);
    } else {
      retain(this);
    }
    this.#subscriptions.set(key, subscription);
    callbacks.set(subscription.id, callback);
  }

  // Forgets `subscription` unless a later one has replaced it.
  #forget(key: string, subscription: Subscription): void {
    if (this.#subscriptions.get(key) !== subscription) return;
    this.#subscriptions.delete(key);
    callbacks.delete(subscription.id);
    release(this);
  }
}
