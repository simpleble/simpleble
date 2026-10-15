import { Adapter } from './adapter';
import { native, type BackendHandle } from './native';
import { intern } from './registry';

const construct = Symbol('construct');

/** A source of adapters: the operating system's Bluetooth stack, Dongl or the simulator. */
export class Backend {
  readonly #handle: BackendHandle;

  private constructor(token: typeof construct, handle: BackendHandle) {
    if (token !== construct) throw new TypeError('Use Backend.getBackends()');
    this.#handle = handle;
  }

  /** @internal */
  static wrap(handle: BackendHandle): Backend {
    return intern(handle.key(), () => new Backend(construct, handle));
  }

  /** Every available backend. */
  static async getBackends(): Promise<Backend[]> {
    return (await native.getBackends()).map(Backend.wrap);
  }

  /** The operating system's Bluetooth backend. */
  static async getNative(): Promise<Backend> {
    return Backend.wrap(await native.getNativeBackend());
  }

  /** The Dongl backend, regardless of `Config.dongl.useDonglBackend`. */
  static async getDongl(): Promise<Backend> {
    return Backend.wrap(await native.getDonglBackend());
  }

  static async getSimulator(): Promise<Backend> {
    return Backend.wrap(await native.getSimulatorBackend());
  }

  identifier(): string {
    return this.#handle.identifier();
  }

  bluetoothEnabled(): Promise<boolean> {
    return this.#handle.bluetoothEnabled();
  }

  async adapters(): Promise<Adapter[]> {
    return (await this.#handle.adapters()).map(Adapter.wrap);
  }
}
