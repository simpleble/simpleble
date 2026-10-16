import { EventEmitter } from 'node:events';

import { HandlerSlot } from './handlers';
import type { LocalCharacteristicHandle, LocalPeripheralHandle, LocalServiceHandle } from './native';
import type { CharacteristicCapability } from './peripheral';
import { intern, releaseFrom, retainWhileListening, unregister } from './registry';

const construct = Symbol('construct');

export interface LocalPeripheralEvents {
  clientConnected: [address: string];
  clientDisconnected: [address: string];
}

const PERIPHERAL_EVENTS = ['clientConnected', 'clientDisconnected'] as const;

/**
 * GATT services this process hosts on an adapter, from
 * Adapter.createLocalPeripheral(). Configure the advertisement and services
 * before start(), and stop() before changing them.
 *
 * Client events carry the client's address, or a platform-specific identifier
 * where the platform hides it. A started local peripheral keeps Node.js
 * running until it stops; while it has listeners it stays alive, even if
 * nothing else references it.
 */
export class LocalPeripheral extends EventEmitter<LocalPeripheralEvents> {
  readonly #handle: LocalPeripheralHandle;
  readonly #rehook: () => void;

  private constructor(token: typeof construct, handle: LocalPeripheralHandle) {
    if (token !== construct) throw new TypeError('Use Adapter.createLocalPeripheral()');
    super();
    this.#handle = handle;
    this.#rehook = retainWhileListening(this as EventEmitter, PERIPHERAL_EVENTS);
    handle.subscribe();
  }

  /** @internal */
  static wrap(handle: LocalPeripheralHandle): LocalPeripheral {
    return intern(handle.key(), () => new LocalPeripheral(construct, handle), () => handle.release());
  }

  /** Adds service UUIDs to the advertisement. */
  addAdvertisedService(uuid: string | readonly string[]): void {
    this.#handle.addAdvertisedService(uuid);
  }

  /** Sets the advertised manufacturer data of a company identifier. */
  setAdvertisedManufacturerData(companyId: number, data: Uint8Array): void {
    this.#handle.setAdvertisedManufacturerData(companyId, data);
  }

  /** Whether clients can connect, true by default. */
  setConnectable(connectable: boolean): void {
    this.#handle.setConnectable(connectable);
  }

  addService(uuid: string): LocalService {
    return LocalService.wrap(this.#handle.addService(uuid));
  }

  services(): LocalService[] {
    return this.#handle.services().map(LocalService.wrap);
  }

  removeAllServices(): void {
    this.#handle.removeAllServices();
  }

  /** Publishes the services and starts advertising. */
  start(): Promise<void> {
    return this.#handle.start();
  }

  stop(): Promise<void> {
    return this.#handle.stop();
  }

  isStarted(): boolean {
    return this.#handle.isStarted();
  }

  isAdvertising(): Promise<boolean> {
    return this.#handle.isAdvertising();
  }

  override removeAllListeners(...args: Parameters<EventEmitter<LocalPeripheralEvents>['removeAllListeners']>): this {
    super.removeAllListeners(...args);
    this.#rehook();
    return this;
  }

  /** @internal */
  [releaseFrom](adapterKey: string): void {
    if (this.#handle.owner() !== adapterKey) return;

    const key = this.#handle.key();
    this.removeAllListeners();
    this.#handle.release();
    unregister(key, this);
  }
}

/** A service of a LocalPeripheral. */
export class LocalService {
  readonly #handle: LocalServiceHandle;

  private constructor(token: typeof construct, handle: LocalServiceHandle) {
    if (token !== construct) throw new TypeError('Use LocalPeripheral.addService()');
    this.#handle = handle;
  }

  /** @internal */
  static wrap(handle: LocalServiceHandle): LocalService {
    return intern(handle.key(), () => new LocalService(construct, handle), () => handle.release());
  }

  uuid(): string {
    return this.#handle.uuid();
  }

  addCharacteristic(uuid: string, capabilities: readonly CharacteristicCapability[]): LocalCharacteristic {
    return LocalCharacteristic.wrap(this.#handle.addCharacteristic(uuid, capabilities));
  }

  characteristics(): LocalCharacteristic[] {
    return this.#handle.characteristics().map(LocalCharacteristic.wrap);
  }

  /** @internal */
  [releaseFrom](adapterKey: string): void {
    if (this.#handle.owner() !== adapterKey) return;

    const key = this.#handle.key();
    this.#handle.release();
    unregister(key, this);
  }
}

export interface LocalCharacteristicEvents {
  /** A client wrote `value`, which value() already returns. */
  write: [value: Buffer];
  /** The first client subscribed. */
  subscribed: [];
  /** The last client unsubscribed. */
  unsubscribed: [];
}

const CHARACTERISTIC_EVENTS = ['write', 'subscribed', 'unsubscribed'] as const;

/**
 * A characteristic of a LocalService. Reads return value() unless a read
 * handler computes the value. While it has listeners it stays alive, even if
 * nothing else references it.
 */
export class LocalCharacteristic extends EventEmitter<LocalCharacteristicEvents> {
  readonly #handle: LocalCharacteristicHandle;
  readonly #rehook: () => void;
  readonly #readHandler = new HandlerSlot<() => Uint8Array | Promise<Uint8Array>>();

  private constructor(token: typeof construct, handle: LocalCharacteristicHandle) {
    if (token !== construct) throw new TypeError('Use LocalService.addCharacteristic()');
    super();
    this.#handle = handle;
    this.#rehook = retainWhileListening(this as EventEmitter, CHARACTERISTIC_EVENTS);
    handle.subscribe();
  }

  /** @internal */
  static wrap(handle: LocalCharacteristicHandle): LocalCharacteristic {
    return intern(handle.key(), () => new LocalCharacteristic(construct, handle), () => handle.release());
  }

  uuid(): string {
    return this.#handle.uuid();
  }

  capabilities(): CharacteristicCapability[] {
    return this.#handle.capabilities() as CharacteristicCapability[];
  }

  /** The stored value, a copy. */
  value(): Buffer {
    return this.#handle.value();
  }

  /** Stores `value` and publishes it to subscribed clients. */
  async setValue(value: Uint8Array): Promise<void> {
    return this.#handle.setValue(value);
  }

  /**
   * Computes the value of each read, or stops doing so with null. The handler
   * can return a promise, and the read waits for it.
   */
  setReadHandler(handler: (() => Uint8Array | Promise<Uint8Array>) | null): Promise<void> {
    return this.#readHandler.set(handler, id => this.#handle.setReadHandler(id));
  }

  override removeAllListeners(
    ...args: Parameters<EventEmitter<LocalCharacteristicEvents>['removeAllListeners']>
  ): this {
    super.removeAllListeners(...args);
    this.#rehook();
    return this;
  }

  /** @internal */
  [releaseFrom](adapterKey: string): void {
    if (this.#handle.owner() !== adapterKey) return;

    const key = this.#handle.key();
    this.#readHandler.drop();
    this.removeAllListeners();
    this.#handle.release();
    unregister(key, this);
  }
}
