import { async, sync } from './errors';
import type { NativeService, PeripheralEvent, PeripheralHandle } from './native';
import { intern, NativeEmitter, release, retain } from './registry';

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
  readonly data: Uint8Array;
  readonly characteristics: readonly Characteristic[];
}

export interface PeripheralEvents {
  connected: [];
  disconnected: [];
}

const construct = Symbol('construct');

interface Subscription {
  readonly id: number;
  readonly callback: (value: Uint8Array) => void;
  // Whether the call that made it has succeeded.
  settled: boolean;
}

let nextId = 1;

// NOTE: Some backends, like CoreBluetooth, match UUIDs regardless of case, and
// the others match only lowercase ones. Lowercase keys name the characteristic
// the backend uses either way.
function subscriptionKey(service: string, characteristic: string): string {
  return `${service}\n${characteristic}`.toLowerCase();
}

function uuids(...values: unknown[]): void {
  if (values.some(value => typeof value !== 'string')) throw new TypeError('UUIDs must be strings');
}

/** The bytes of `data`, in an ArrayBuffer of their own. @internal */
export function arrayBuffer(data: unknown): ArrayBuffer {
  if (!(data instanceof Uint8Array)) throw new TypeError('data must be a Uint8Array');
  return data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength) as ArrayBuffer;
}

function convert(services: NativeService[]): Service[] {
  return services.map(service =>
    Object.freeze({
      uuid: service.uuid,
      data: new Uint8Array(service.data),
      characteristics: Object.freeze(
        service.characteristics.map(characteristic =>
          Object.freeze({
            uuid: characteristic.uuid,
            capabilities: Object.freeze(characteristic.capabilities as CharacteristicCapability[]),
            descriptors: Object.freeze(characteristic.descriptors.map(descriptor => Object.freeze({ uuid: descriptor.uuid }))),
          }),
        ),
      ),
    }),
  );
}

/**
 * A remote device. An adapter gives you one Peripheral object per device, so
 * its events, scan results and peripheral lists return the same object.
 *
 * A peripheral with listeners or subscriptions stays alive, even if nothing
 * else references it.
 */
export class Peripheral extends NativeEmitter<PeripheralEvents> {
  readonly #handle: PeripheralHandle;
  // Subscriptions by subscriptionKey().
  readonly #subscriptions = new Map<string, Subscription>();

  private constructor(token: typeof construct, handle: PeripheralHandle) {
    if (token !== construct) throw new TypeError('Peripherals come from an Adapter');
    super();
    this.#handle = handle;
    handle.subscribe(
      this.weakly((self, event: PeripheralEvent) => self.#onEvent(event)),
      this.weakly((self, id: number, value: ArrayBuffer) => self.#deliver(id, value)),
    );
  }

  /** @internal */
  static wrap(handle: PeripheralHandle): Peripheral {
    return intern(handle, h => new Peripheral(construct, h));
  }

  identifier(): string {
    return sync(() => this.#handle.identifier());
  }

  address(): string {
    return sync(() => this.#handle.address());
  }

  addressType(): BluetoothAddressType {
    return sync(() => this.#handle.addressType());
  }

  /** The last received signal strength in dBm, if any. */
  rssi(): number | undefined {
    return sync(() => this.#handle.rssi());
  }

  /** The advertised transmit power in dBm, if any. */
  txPower(): number | undefined {
    return sync(() => this.#handle.txPower());
  }

  mtu(): number {
    return sync(() => this.#handle.mtu());
  }

  isConnected(): boolean {
    return sync(() => this.#handle.isConnected());
  }

  isConnectable(): boolean {
    return sync(() => this.#handle.isConnectable());
  }

  isPaired(): Promise<boolean> {
    return async(() => this.#handle.isPaired());
  }

  /** Advertised manufacturer data by company identifier. The arrays are copies. */
  manufacturerData(): Map<number, Uint8Array> {
    return sync(() => new Map(this.#handle.manufacturerData().map(entry => [entry.companyId, new Uint8Array(entry.data)])));
  }

  /** The services of a connected peripheral, or the advertised ones before connecting. */
  services(): Service[] {
    return sync(() => convert(this.#handle.services()));
  }

  connect(): Promise<void> {
    return async(() => this.#handle.connect());
  }

  async disconnect(): Promise<void> {
    // Calls run in call order, so this ends the subscriptions made before it,
    // and not those of calls made while it runs.
    const ended = [...this.#subscriptions];
    await async(() => this.#handle.disconnect());
    for (const [key, subscription] of ended) this.#forget(key, subscription);
  }

  unpair(): Promise<void> {
    return async(() => this.#handle.unpair());
  }

  async read(service: string, characteristic: string): Promise<Uint8Array> {
    uuids(service, characteristic);
    return new Uint8Array(await async(() => this.#handle.read(service, characteristic)));
  }

  async writeRequest(service: string, characteristic: string, data: Uint8Array): Promise<void> {
    uuids(service, characteristic);
    const buffer = arrayBuffer(data);
    return async(() => this.#handle.writeRequest(service, characteristic, buffer));
  }

  async writeCommand(service: string, characteristic: string, data: Uint8Array): Promise<void> {
    uuids(service, characteristic);
    const buffer = arrayBuffer(data);
    return async(() => this.#handle.writeCommand(service, characteristic, buffer));
  }

  async readDescriptor(service: string, characteristic: string, descriptor: string): Promise<Uint8Array> {
    uuids(service, characteristic, descriptor);
    return new Uint8Array(await async(() => this.#handle.readDescriptor(service, characteristic, descriptor)));
  }

  async writeDescriptor(service: string, characteristic: string, descriptor: string, data: Uint8Array): Promise<void> {
    uuids(service, characteristic, descriptor);
    const buffer = arrayBuffer(data);
    return async(() => this.#handle.writeDescriptor(service, characteristic, descriptor, buffer));
  }

  /** Calls `callback` with each notified value until unsubscribe() or a disconnection. */
  notify(service: string, characteristic: string, callback: (value: Uint8Array) => void): Promise<void> {
    return this.#subscribe(id => this.#handle.notify(service, characteristic, id), service, characteristic, callback);
  }

  /** Calls `callback` with each indicated value until unsubscribe() or a disconnection. */
  indicate(service: string, characteristic: string, callback: (value: Uint8Array) => void): Promise<void> {
    return this.#subscribe(id => this.#handle.indicate(service, characteristic, id), service, characteristic, callback);
  }

  async unsubscribe(service: string, characteristic: string): Promise<void> {
    uuids(service, characteristic);
    // Like disconnect(), this ends only the subscription made before it.
    const key = subscriptionKey(service, characteristic);
    const ended = this.#subscriptions.get(key);
    await async(() => this.#handle.unsubscribe(service, characteristic));
    if (ended) this.#forget(key, ended);
  }

  #onEvent(event: PeripheralEvent): void {
    // A disconnection ends the subscriptions made before it. One still being
    // made either started after it or fails, which forgets it.
    if (event === 'disconnected') {
      for (const [key, subscription] of [...this.#subscriptions]) {
        if (subscription.settled) this.#forget(key, subscription);
      }
    }
    this.emit(event);
  }

  #deliver(id: number, value: ArrayBuffer): void {
    // NOTE: Values of a subscription that has ended are dropped.
    for (const subscription of this.#subscriptions.values()) {
      if (subscription.id === id) return subscription.callback(new Uint8Array(value));
    }
  }

  async #subscribe(
    start: (id: number) => Promise<void>,
    service: string,
    characteristic: string,
    callback: (value: Uint8Array) => void,
  ): Promise<void> {
    uuids(service, characteristic);
    if (typeof callback !== 'function') throw new TypeError('callback must be a function');

    // The callback is in place before the first value can arrive. A new
    // subscription to the same characteristic replaces the previous one.
    const key = subscriptionKey(service, characteristic);
    const subscription: Subscription = { id: nextId++, callback, settled: false };
    if (!this.#subscriptions.has(key)) retain(this);
    this.#subscriptions.set(key, subscription);
    try {
      await async(() => start(subscription.id));
    } catch (error) {
      this.#forget(key, subscription);
      throw error;
    }
    subscription.settled = true;
  }

  // Forgets `subscription` unless a later one has replaced it.
  #forget(key: string, subscription: Subscription): void {
    if (this.#subscriptions.get(key) !== subscription) return;
    this.#subscriptions.delete(key);
    release(this);
  }
}
