// NOTE: Listeners are stored untyped; the public methods check their types.
// eslint-disable-next-line @typescript-eslint/no-explicit-any
type Listener = (...args: any[]) => void;

/**
 * A typed event emitter with the methods of Node.js's EventEmitter that
 * applications use. Listeners run in the order they were added; one that
 * throws stops the event, as in Node.js.
 */
export class EventEmitter<Events extends { [K in keyof Events]: unknown[] }> {
  readonly #listeners = new Map<keyof Events, Listener[]>();

  on<K extends keyof Events>(event: K, listener: (...args: Events[K]) => void): this {
    const listeners = this.#listeners.get(event) ?? [];
    this.#listeners.set(event, [...listeners, listener as Listener]);
    if (listeners.length === 0) this.listenersChanged();
    return this;
  }

  addListener<K extends keyof Events>(event: K, listener: (...args: Events[K]) => void): this {
    return this.on(event, listener);
  }

  once<K extends keyof Events>(event: K, listener: (...args: Events[K]) => void): this {
    const wrapper = (...args: Events[K]) => {
      this.off(event, wrapper);
      listener(...args);
    };
    (wrapper as { listener?: unknown }).listener = listener;
    return this.on(event, wrapper);
  }

  off<K extends keyof Events>(event: K, listener: (...args: Events[K]) => void): this {
    const listeners = this.#listeners.get(event);
    if (!listeners) return this;
    let index = listeners.length - 1;
    while (index >= 0 && listeners[index] !== listener && (listeners[index] as { listener?: unknown }).listener !== listener) {
      index--;
    }
    if (index < 0) return this;
    const remaining = [...listeners.slice(0, index), ...listeners.slice(index + 1)];
    if (remaining.length > 0) {
      this.#listeners.set(event, remaining);
    } else {
      this.#listeners.delete(event);
      this.listenersChanged();
    }
    return this;
  }

  removeListener<K extends keyof Events>(event: K, listener: (...args: Events[K]) => void): this {
    return this.off(event, listener);
  }

  removeAllListeners(event?: keyof Events): this {
    const changed = event === undefined ? this.#listeners.size > 0 : this.#listeners.has(event);
    if (event === undefined) this.#listeners.clear();
    else this.#listeners.delete(event);
    if (changed) this.listenersChanged();
    return this;
  }

  listenerCount(event: keyof Events): number {
    return this.#listeners.get(event)?.length ?? 0;
  }

  emit<K extends keyof Events>(event: K, ...args: Events[K]): boolean {
    const listeners = this.#listeners.get(event);
    if (!listeners) return false;
    for (const listener of listeners) (listener as (...args: Events[K]) => void)(...args);
    return true;
  }

  /** Whether any event has listeners. */
  protected hasListeners(): boolean {
    return this.#listeners.size > 0;
  }

  /** Called when an event gains its first listener or loses its last one. */
  protected listenersChanged(): void {}
}
