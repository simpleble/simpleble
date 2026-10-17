import { native } from './native';

/** SimpleBLE's log levels, from 'none' (off) to the most detailed, 'verbose'. */
export type LogLevel = 'none' | 'fatal' | 'error' | 'warn' | 'info' | 'debug' | 'verbose';

/** One entry of SimpleBLE's log. */
export interface LogEntry {
  readonly level: Exclude<LogLevel, 'none'>;
  /** The component that logged it, such as `SimpleBLE`. */
  readonly module: string;
  readonly file: string;
  readonly line: number;
  readonly function: string;
  readonly message: string;
}

const LEVELS: readonly LogLevel[] = ['none', 'fatal', 'error', 'warn', 'info', 'debug', 'verbose'];

/**
 * SimpleBLE's log, app-wide. By default, entries up to 'info' go to the
 * system's log: logcat on Android, the console on iOS.
 */
export const Logging: {
  level: LogLevel;
  setCallback(callback: ((entry: LogEntry) => void) | null): void;
  logToSystem(): void;
  logToFile(path?: string): void;
} = Object.freeze({
  /** The most detailed level that is logged; 'none' turns logging off. */
  get level(): LogLevel {
    return LEVELS[native.logLevel] ?? 'none';
  },
  set level(level: LogLevel) {
    if (!LEVELS.includes(level)) throw new TypeError(`level must be one of ${LEVELS.join(', ')}`);
    native.logLevel = LEVELS.indexOf(level);
  },

  /**
   * Delivers each entry to `callback` on the JavaScript thread, in the order
   * they were logged, instead of writing it out. null discards entries.
   */
  setCallback(callback: ((entry: LogEntry) => void) | null): void {
    if (callback !== null && typeof callback !== 'function') throw new TypeError('callback must be a function or null');
    native.setLogCallback(callback ? entry => callback(Object.freeze({ ...entry, level: LEVELS[entry.level] }) as LogEntry) : undefined);
  },

  /** Writes entries to the system's log, the default. */
  logToSystem(): void {
    native.logToSystem();
  },

  /** Appends entries to `path`, or to a new timestamped file in the working directory. */
  logToFile(path?: string): void {
    if (path !== undefined && typeof path !== 'string') throw new TypeError('path must be a string');
    native.logToFile(path);
  },
});
