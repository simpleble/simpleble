import { native } from './native';

/** SimpleBLE's log levels, from 'none' (off) to the most detailed, 'verbose'. */
export type LogLevel = 'none' | 'fatal' | 'error' | 'warn' | 'info' | 'debug' | 'verbose';

/** One entry of SimpleBLE's log. */
export interface LogEntry {
  readonly level: Exclude<LogLevel, 'none'>;
  /** The component that logged it, such as `SimpleBLE` or `SimpleBLE->SimpleBluez`. */
  readonly module: string;
  readonly file: string;
  readonly line: number;
  readonly function: string;
  readonly message: string;
}

/**
 * SimpleBLE's log, process-wide. By default, entries up to 'info' go to
 * standard output.
 */
export const Logging: {
  level: LogLevel;
  setCallback(callback: ((entry: LogEntry) => void) | null): void;
  logToStdout(): void;
  logToFile(path?: string): void;
} = Object.freeze({
  /** The most detailed level that is logged; 'none' turns logging off. */
  get level(): LogLevel {
    return native.loggingGetLevel();
  },
  set level(level: LogLevel) {
    native.loggingSetLevel(level);
  },

  /**
   * Delivers each entry to `callback` on the JavaScript thread, in the order
   * they were logged, instead of writing it out. null discards entries.
   */
  setCallback(callback: ((entry: LogEntry) => void) | null): void {
    if (callback === null) {
      native.loggingSetCallback(null);
      return;
    }
    if (typeof callback !== 'function') throw new TypeError('callback must be a function or null');
    native.loggingSetCallback(entry => callback(Object.freeze(entry)));
  },

  /** Writes entries to standard output, the default. */
  logToStdout(): void {
    native.loggingToStdout();
  },

  /** Appends entries to `path`, or to a new timestamped file in the working directory. */
  logToFile(path?: string): void {
    native.loggingToFile(path);
  },
});
