// Handlers answer questions a native thread asks and waits for, such as a local
// characteristic read or a pairing request. The addon passes only the handler's
// id with each question, and the question's arguments.

type Handler = (...args: string[]) => unknown;

const handlers = new Map<number, Handler>();
let nextId = 1;

/** @internal */
export function answer(id: number, args: string[], respond?: (value: unknown) => void): void {
  const handler = handlers.get(id);
  // NOTE: An error in a handler is unhandled, as one in a listener is uncaught,
  // and the native side then gets no answer.
  Promise.resolve()
    .then(() => handler?.(...args))
    .then(respond, error => {
      respond?.(undefined);
      throw error;
    });
}

/**
 * The handler of one native callback. A new handler reaches the native object
 * through `install`, in call order with the object's other calls; the previous
 * one answers until then.
 * @internal
 */
export class HandlerSlot<H extends (...args: never[]) => unknown> {
  #current?: number;

  async set(handler: H | null, install: (id: number | undefined) => Promise<void>): Promise<void> {
    if (handler !== null && typeof handler !== 'function') throw new TypeError('handler must be a function or null');

    const id = handler === null ? undefined : nextId++;
    if (id !== undefined) handlers.set(id, handler as unknown as Handler);
    const previous = this.#current;
    this.#current = id;
    try {
      await install(id);
    } catch (error) {
      if (id !== undefined) handlers.delete(id);
      if (this.#current === id) this.#current = previous;
      throw error;
    }
    if (previous !== undefined) handlers.delete(previous);
  }

  /** Whether a handler is installed or being installed. */
  get isSet(): boolean {
    return this.#current !== undefined;
  }

  /** Forgets the handler of a native object that is gone. */
  drop(): void {
    if (this.#current !== undefined) handlers.delete(this.#current);
    this.#current = undefined;
  }
}
