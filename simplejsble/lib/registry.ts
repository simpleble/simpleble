import type { EventEmitter } from 'node:events';

// One JavaScript object per native object, keyed by the native object's address.

const objects = new Map<string, WeakRef<object>>();
const finalizer = new FinalizationRegistry<{ key: string; ref: WeakRef<object> }>(({ key, ref }) => {
  if (objects.get(key) === ref) objects.delete(key);
});

// Objects with listeners for native events or with subscriptions are kept
// alive, so they keep working after the caller drops its own reference.
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
 * Lets an object see a native event before its listeners do, even without
 * listeners, e.g. to forget subscriptions on a disconnection.
 * @internal
 */
export const onNativeEvent = Symbol('onNativeEvent');

/** @internal */
export function lookup(key: string): object | undefined {
  return objects.get(key)?.deref();
}

/** Every object that still stands for a native object. @internal */
export function registered(): object[] {
  return [...objects.values()].map(ref => ref.deref()).filter(object => object !== undefined);
}

/**
 * Forgets `object`, which no longer stands for a native object, so the next
 * object for `key` is a new one.
 * @internal
 */
export function unregister(key: string, object: object): void {
  if (lookup(key) === object) objects.delete(key);
  retained.delete(object);
}

/**
 * Returns the object for `key`, created if there is none. `discard` releases
 * the new handle when an object exists, as an unused handle would otherwise
 * keep the native object until garbage collection.
 * @internal
 */
export function intern<T extends object>(key: string, create: () => T, discard?: () => void): T {
  const existing = lookup(key);
  if (existing) {
    discard?.();
    return existing as T;
  }

  const object = create();
  const ref = new WeakRef(object);
  objects.set(key, ref);
  finalizer.register(object, { key, ref });
  return object;
}

/**
 * Keeps `emitter` alive while it has listeners for any of `events`. Returns a
 * function that reinstalls the hooks, which removeAllListeners() also removes.
 * @internal
 */
export function retainWhileListening(emitter: EventEmitter, events: readonly string[]): () => void {
  const listening = () => events.some(event => emitter.listenerCount(event) > 0);
  let held = false;

  // NOTE: 'newListener' fires before the listener is added, 'removeListener'
  // after it is removed.
  const onNewListener = (event: string | symbol) => {
    if (held || typeof event !== 'string' || !events.includes(event)) return;
    held = true;
    retain(emitter);
  };
  const onRemoveListener = (event: string | symbol) => {
    if (!held || typeof event !== 'string' || !events.includes(event) || listening()) return;
    held = false;
    release(emitter);
  };

  const hook = () => {
    emitter.off('newListener', onNewListener).off('removeListener', onRemoveListener);
    emitter.on('newListener', onNewListener).on('removeListener', onRemoveListener);
  };
  hook();
  return hook;
}
