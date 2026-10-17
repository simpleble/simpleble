import { native } from './native';

/** How an Android connection asks for its connection interval. */
export type ConnectionPriorityRequest = 'disabled' | 'balanced' | 'high' | 'lowPower' | 'dck';

const PRIORITIES: readonly ConnectionPriorityRequest[] = ['disabled', 'balanced', 'high', 'lowPower', 'dck'];

export interface AndroidConfig {
  /** Requested after each connection; 'disabled' leaves Android's default. */
  connectionPriorityRequest: ConnectionPriorityRequest;
}

/**
 * App-wide settings of each backend. Set them before the first call that
 * reaches a backend.
 */
export const Config: {
  android: AndroidConfig & { reset(): void };
  resetAll(): void;
} = Object.freeze({
  android: Object.freeze({
    get connectionPriorityRequest(): ConnectionPriorityRequest {
      return native.connectionPriorityRequest;
    },
    set connectionPriorityRequest(value: ConnectionPriorityRequest) {
      if (!PRIORITIES.includes(value)) throw new TypeError(`connectionPriorityRequest must be one of ${PRIORITIES.join(', ')}`);
      native.connectionPriorityRequest = value;
    },
    reset(): void {
      native.connectionPriorityRequest = 'disabled';
    },
  }),
  resetAll(): void {
    native.resetConfig();
  },
});
