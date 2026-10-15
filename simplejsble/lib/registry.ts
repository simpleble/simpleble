import type { EventEmitter } from 'node:events';

// One JavaScript object per native object, keyed by the native object's address.

const objects = new Map<string, WeakRef<object>>();
const finalizer = new FinalizationRegistry<{ key: string; ref: WeakRef<object> }>(({ key, ref }) => {
  if (objects.get(key) === ref) objects.delete(key);
});

// Objects with listeners for native events are kept alive, so listeners keep
// working after the caller drops its own reference.
const retained = new Set<object>();

/** @internal */
export function lookup(key: string): object | undefined {
  return objects.get(key)?.deref();
}

/** @internal */
export function intern<T extends object>(key: string, create: () => T): T {
  const existing = lookup(key);
  if (existing) return existing as T;

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

  // NOTE: 'newListener' fires before the listener is added, 'removeListener'
  // after it is removed.
  const onNewListener = (event: string | symbol) => {
    if (typeof event === 'string' && events.includes(event)) retained.add(emitter);
  };
  const onRemoveListener = (event: string | symbol) => {
    if (typeof event === 'string' && events.includes(event) && !listening()) retained.delete(emitter);
  };

  const hook = () => {
    emitter.off('newListener', onNewListener).off('removeListener', onRemoveListener);
    emitter.on('newListener', onNewListener).on('removeListener', onRemoveListener);
  };
  hook();
  return hook;
}
