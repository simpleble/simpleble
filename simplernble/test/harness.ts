// A minimal test harness for the binding tests, which run inside a React
// Native app (see TestApp.tsx). Tests run one at a time, in the order the
// files register them.

type Body = () => unknown;

const tests: { name: string; body: Body }[] = [];

export function test(name: string, body: Body): void {
  tests.push({ name, body });
}

export interface Result {
  name: string;
  ok: boolean;
  error?: string;
  ms: number;
}

const TIMEOUT_MS = 30_000;

export async function run(report: (result: Result) => unknown): Promise<Result[]> {
  const results: Result[] = [];
  for (const { name, body } of tests) {
    const started = Date.now();
    let result: Result;
    try {
      await within(Promise.resolve().then(body), TIMEOUT_MS);
      result = { name, ok: true, ms: Date.now() - started };
    } catch (error) {
      const message = error instanceof Error ? `${error.name}: ${error.message}` : String(error);
      result = { name, ok: false, error: message, ms: Date.now() - started };
    }
    results.push(result);
    await report(result);
  }
  return results;
}

/** Waits for `promise`, failing after `ms`. */
export function within<T>(promise: Promise<T>, ms = 5_000): Promise<T> {
  let timer: ReturnType<typeof setTimeout> | undefined;
  const timeout = new Promise<never>((_, reject) => {
    timer = setTimeout(() => reject(new Error(`Nothing within ${ms} ms`)), ms);
  });
  return Promise.race([promise, timeout]).finally(() => clearTimeout(timer));
}

/** The arguments of the next `event` of `emitter`. */
// eslint-disable-next-line @typescript-eslint/no-explicit-any
export function once(emitter: { once(event: any, listener: (...args: any[]) => void): unknown }, event: string): Promise<unknown[]> {
  return new Promise(resolve => emitter.once(event, (...args: unknown[]) => resolve(args)));
}

function describe(value: unknown): string {
  if (value instanceof Uint8Array) return `Uint8Array [${Array.from(value).join(', ')}]`;
  if (value instanceof Map) return `Map ${JSON.stringify([...value].map(([k, v]) => [k, describe(v)]))}`;
  try {
    return JSON.stringify(value) ?? String(value);
  } catch {
    return String(value);
  }
}

function deepEqual(actual: unknown, expected: unknown): boolean {
  if (Object.is(actual, expected)) return true;
  if (actual instanceof Uint8Array && expected instanceof Uint8Array) {
    return actual.length === expected.length && actual.every((byte, i) => byte === expected[i]);
  }
  if (actual instanceof Map && expected instanceof Map) {
    return actual.size === expected.size && [...actual].every(([key, value]) => deepEqual(value, expected.get(key)));
  }
  if (Array.isArray(actual) && Array.isArray(expected)) {
    return actual.length === expected.length && actual.every((item, i) => deepEqual(item, expected[i]));
  }
  if (typeof actual === 'object' && typeof expected === 'object' && actual !== null && expected !== null) {
    const keys = Object.keys(expected);
    return keys.length === Object.keys(actual).length && keys.every(key => deepEqual((actual as never)[key], (expected as never)[key]));
  }
  return false;
}

// A class the error must be an instance of, properties it must have, or a
// predicate (an arrow function, which has no prototype).
type Expected = (new (...args: never[]) => unknown) | Record<string, unknown> | ((error: unknown) => boolean);

function matches(error: unknown, expected: Expected | undefined): boolean {
  if (expected === undefined) return true;
  if (typeof expected === 'function') {
    return expected.prototype !== undefined
      ? error instanceof (expected as new (...args: never[]) => unknown)
      : (expected as (error: unknown) => boolean)(error);
  }
  return typeof error === 'object' && error !== null && Object.entries(expected).every(([key, value]) => (error as never)[key] === value);
}

export const assert = {
  ok(value: unknown, message = 'Expected a truthy value'): void {
    if (!value) throw new Error(message);
  },

  equal(actual: unknown, expected: unknown, message?: string): void {
    if (!Object.is(actual, expected)) throw new Error(message ?? `Expected ${describe(expected)}, got ${describe(actual)}`);
  },

  deepEqual(actual: unknown, expected: unknown, message?: string): void {
    if (!deepEqual(actual, expected)) throw new Error(message ?? `Expected ${describe(expected)}, got ${describe(actual)}`);
  },

  throws(fn: () => unknown, expected?: Expected, message?: string): void {
    try {
      fn();
    } catch (error) {
      if (matches(error, expected)) return;
      throw new Error(message ?? `Unexpected error: ${describe(error instanceof Error ? `${error.name}: ${error.message}` : error)}`);
    }
    throw new Error(message ?? 'Expected an error');
  },

  async rejects(promise: Promise<unknown> | (() => Promise<unknown>), expected?: Expected, message?: string): Promise<void> {
    try {
      await (typeof promise === 'function' ? promise() : promise);
    } catch (error) {
      if (matches(error, expected)) return;
      throw new Error(message ?? `Unexpected rejection: ${error instanceof Error ? `${error.name}: ${error.message}` : describe(error)}`);
    }
    throw new Error(message ?? 'Expected a rejection');
  },
};
