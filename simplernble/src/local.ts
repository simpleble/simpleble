import { async, sync } from './errors';
import type { LocalCharacteristicEvent, LocalCharacteristicHandle, LocalPeripheralEvent, LocalPeripheralHandle, LocalServiceHandle } from './native';
import { arrayBuffer, type CharacteristicCapability } from './peripheral';
import { intern, NativeEmitter } from './registry';

const construct = Symbol('construct');

export interface LocalPeripheralEvents {
  clientConnected: [address: string];
  clientDisconnected: [address: string];
}

/**
 * GATT services this app hosts on an adapter, from
 * Adapter.createLocalPeripheral(). Configure the advertisement and services
 * before start(), and stop() before changing them.
 *
 * Client events carry the client's address, or a platform-specific identifier
 * where the platform hides it. While it has listeners it stays alive, even if
 * nothing else references it.
 */
export class LocalPeripheral extends NativeEmitter<LocalPeripheralEvents> {
  readonly #handle: LocalPeripheralHandle;

  private constructor(token: typeof construct, handle: LocalPeripheralHandle) {
    if (token !== construct) throw new TypeError('Use Adapter.createLocalPeripheral()');
    super();
    this.#handle = handle;
    handle.subscribe(this.weakly((self, event: LocalPeripheralEvent, address: string) => self.emit(event, address)));
  }

  /** @internal */
  static wrap(handle: LocalPeripheralHandle): LocalPeripheral {
    return intern(handle, h => new LocalPeripheral(construct, h));
  }

  /** Adds service UUIDs to the advertisement. */
  addAdvertisedService(uuid: string | readonly string[]): void {
    const uuids = typeof uuid === 'string' ? [uuid] : [...uuid];
    if (!uuids.every(item => typeof item === 'string')) throw new TypeError('UUIDs must be strings');
    sync(() => this.#handle.addAdvertisedService(uuids));
  }

  /** Sets the advertised manufacturer data of a company identifier. */
  setAdvertisedManufacturerData(companyId: number, data: Uint8Array): void {
    if (!Number.isInteger(companyId) || companyId < 0 || companyId > 0xffff) {
      throw new RangeError('companyId must be an integer from 0 to 65535');
    }
    const buffer = arrayBuffer(data);
    sync(() => this.#handle.setAdvertisedManufacturerData(companyId, buffer));
  }

  /** Whether clients can connect, true by default. */
  setConnectable(connectable: boolean): void {
    if (typeof connectable !== 'boolean') throw new TypeError('connectable must be a boolean');
    sync(() => this.#handle.setConnectable(connectable));
  }

  /**
   * Sets the local name in the advertisement on iOS, or removes it with null.
   * This changes only the advertisement, not the device's Bluetooth name.
   * Android throws OPERATION_NOT_SUPPORTED.
   */
  setAdvertisementLocalName(name: string | null): void {
    if (name !== null && typeof name !== 'string') throw new TypeError('name must be a string or null');
    sync(() => this.#handle.setAdvertisementLocalName(name ?? undefined));
  }

  addService(uuid: string): LocalService {
    if (typeof uuid !== 'string') throw new TypeError('UUIDs must be strings');
    return LocalService.wrap(sync(() => this.#handle.addService(uuid)));
  }

  services(): LocalService[] {
    return sync(() => this.#handle.services()).map(LocalService.wrap);
  }

  removeAllServices(): void {
    sync(() => this.#handle.removeAllServices());
  }

  /** Publishes the services and starts advertising. */
  start(): Promise<void> {
    return async(() => this.#handle.start());
  }

  stop(): Promise<void> {
    return async(() => this.#handle.stop());
  }

  isStarted(): boolean {
    return sync(() => this.#handle.isStarted());
  }

  isAdvertising(): Promise<boolean> {
    return async(() => this.#handle.isAdvertising());
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
    return intern(handle, h => new LocalService(construct, h));
  }

  uuid(): string {
    return sync(() => this.#handle.uuid());
  }

  addCharacteristic(uuid: string, capabilities: readonly CharacteristicCapability[]): LocalCharacteristic {
    if (typeof uuid !== 'string') throw new TypeError('UUIDs must be strings');
    if (!Array.isArray(capabilities) || !capabilities.every(item => typeof item === 'string')) {
      throw new TypeError('capabilities must be an array of strings');
    }
    return LocalCharacteristic.wrap(sync(() => this.#handle.addCharacteristic(uuid, [...capabilities])));
  }

  characteristics(): LocalCharacteristic[] {
    return sync(() => this.#handle.characteristics()).map(LocalCharacteristic.wrap);
  }
}

export interface LocalCharacteristicEvents {
  /** A client wrote `value`, which value() already returns. */
  write: [value: Uint8Array];
  /** The first client subscribed. */
  subscribed: [];
  /** The last client unsubscribed. */
  unsubscribed: [];
}

/**
 * A characteristic of a LocalService. Reads return value() unless a read
 * handler computes the value. While it has listeners it stays alive, even if
 * nothing else references it.
 */
export class LocalCharacteristic extends NativeEmitter<LocalCharacteristicEvents> {
  readonly #handle: LocalCharacteristicHandle;

  private constructor(token: typeof construct, handle: LocalCharacteristicHandle) {
    if (token !== construct) throw new TypeError('Use LocalService.addCharacteristic()');
    super();
    this.#handle = handle;
    handle.subscribe(
      this.weakly((self, event: LocalCharacteristicEvent) => self.emit(event)),
      this.weakly((self, value: ArrayBuffer) => self.emit('write', new Uint8Array(value))),
    );
  }

  /** @internal */
  static wrap(handle: LocalCharacteristicHandle): LocalCharacteristic {
    return intern(handle, h => new LocalCharacteristic(construct, h));
  }

  uuid(): string {
    return sync(() => this.#handle.uuid());
  }

  capabilities(): CharacteristicCapability[] {
    return sync(() => this.#handle.capabilities()) as CharacteristicCapability[];
  }

  /** The stored value, a copy. */
  value(): Uint8Array {
    return new Uint8Array(sync(() => this.#handle.value()));
  }

  /** Stores `value` and publishes it to subscribed clients. */
  async setValue(value: Uint8Array): Promise<void> {
    const buffer = arrayBuffer(value);
    return async(() => this.#handle.setValue(buffer));
  }

  /**
   * Computes the value of each read, or stops doing so with null. The handler
   * can return a promise, and the read waits for it. If it throws or rejects,
   * the read gets an empty value and the error is uncaught.
   */
  async setReadHandler(handler: (() => Uint8Array | Promise<Uint8Array>) | null): Promise<void> {
    if (handler !== null && typeof handler !== 'function') throw new TypeError('handler must be a function or null');
    const answer = handler
      ? async () => {
          try {
            return Array.from(new Uint8Array(arrayBuffer(await handler())));
          } catch (error) {
            // An error in a handler behaves like one thrown from a timer.
            setTimeout(() => {
              throw error;
            });
            return [];
          }
        }
      : undefined;
    return async(() => this.#handle.setReadHandler(answer));
  }
}
