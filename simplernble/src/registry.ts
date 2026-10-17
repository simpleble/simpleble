import { EventEmitter } from './events';

// One public object per native handle. Nitro returns the same handle object
// while it is alive, so a WeakMap keyed by it finds the public object.
const objects = new WeakMap<object, object>();

/** The public object of `handle`, created if there is none. @internal */
export function intern<H extends object, T extends object>(handle: H, create: (handle: H) => T): T {
  const existing = objects.get(handle);
  if (existing) return existing as T;
  const object = create(handle);
  objects.set(handle, object);
  return object;
}

// Objects with listeners or subscriptions are kept alive, so they keep working
// after the caller drops its own reference.
const retained = new Map<object, number>();

/** @internal */
export function retain(object: object): void {
  retained.set(object, (retained.get(object) ?? 0) + 1);
}

/** @internal */
export function release(object: object): void {
  const count = (retained.get(object) ?? 0) - 1;
  if (count > 0) {
    retained.set(object, count);
  } else {
    retained.delete(object);
  }
}

/**
 * An emitter that stays alive while it has listeners, and whose native events
 * reach it without native code keeping it alive.
 */
export class NativeEmitter<Events extends { [K in keyof Events]: unknown[] }> extends EventEmitter<Events> {
  #held = false;

  protected override listenersChanged(): void {
    const listening = this.hasListeners();
    if (listening === this.#held) return;
    this.#held = listening;
    if (listening) retain(this);
    else release(this);
  }

  /** A callback for native code that reaches this object while it is alive. */
  protected weakly<A extends unknown[]>(fn: (self: this, ...args: A) => void): (...args: A) => void {
    const ref = new WeakRef(this);
    return (...args: A) => {
      const self = ref.deref();
      if (self) fn(self, ...args);
    };
  }
}
