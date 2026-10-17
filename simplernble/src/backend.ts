import { Adapter } from './adapter';
import { async, sync } from './errors';
import { native, type BackendHandle } from './native';
import { intern } from './registry';

const construct = Symbol('construct');

/** A source of adapters, such as the operating system's Bluetooth stack. */
export class Backend {
  readonly #handle: BackendHandle;

  private constructor(token: typeof construct, handle: BackendHandle) {
    if (token !== construct) throw new TypeError('Use Backend.getBackends()');
    this.#handle = handle;
  }

  /** @internal */
  static wrap(handle: BackendHandle): Backend {
    return intern(handle, h => new Backend(construct, h));
  }

  /** Every available backend. */
  static async getBackends(): Promise<Backend[]> {
    return (await async(() => native.getBackends())).map(Backend.wrap);
  }

  /** The operating system's Bluetooth backend. */
  static async getNative(): Promise<Backend> {
    return Backend.wrap(await async(() => native.getNativeBackend()));
  }

  identifier(): string {
    return sync(() => this.#handle.identifier());
  }

  bluetoothEnabled(): Promise<boolean> {
    return async(() => this.#handle.bluetoothEnabled());
  }

  async adapters(): Promise<Adapter[]> {
    return (await async(() => this.#handle.adapters())).map(Adapter.wrap);
  }
}
