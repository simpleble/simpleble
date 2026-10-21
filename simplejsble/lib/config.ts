import { native } from './native';

function setting<T extends boolean | number>(name: string) {
  return {
    get: (): T => native.configGet(name) as T,
    set: (value: T): void => native.configSet(name, value),
    enumerable: true,
  };
}

function group<T extends object>(name: string, settings: Record<keyof T, PropertyDescriptor>): T & { reset(): void } {
  const object = { reset: () => native.configReset(name) };
  for (const [key, descriptor] of Object.entries(settings)) {
    Object.defineProperty(object, key, descriptor as PropertyDescriptor);
  }
  return Object.freeze(object) as T & { reset(): void };
}

export interface SimpleBluezConfig {
  /** Reach BlueZ over the D-Bus system bus (true) or the session bus. */
  useSystemBus: boolean;
  /** How long connect() waits for BlueZ, in milliseconds. */
  connectionTimeout: number;
  /** How long disconnect() waits for BlueZ, in milliseconds. */
  disconnectionTimeout: number;
}

export interface WinRTConfig {
  experimentalUseOwnMtaApartment: boolean;
  experimentalReinitializeWinrtApartmentOnMainThread: boolean;
  useDeferredDisconnect: boolean;
}

export interface AndroidConfig {
  /** Negotiate MTU on connect. Defaults to true. */
  negotiateMtu: boolean;
}

export interface DonglConfig {
  /** Include Dongl adapters in Backend.getBackends() and Adapter.getAdapters(). */
  useDonglBackend: boolean;
  autoUpdate: boolean;
  forceUpdate: boolean;
}

/**
 * Process-wide settings of each backend. Set them before the first call that
 * reaches a backend; later changes might not take effect.
 */
export const Config = Object.freeze({
  simpleBluez: group<SimpleBluezConfig>('simpleBluez', {
    useSystemBus: setting<boolean>('simpleBluez.useSystemBus'),
    connectionTimeout: setting<number>('simpleBluez.connectionTimeout'),
    disconnectionTimeout: setting<number>('simpleBluez.disconnectionTimeout'),
  }),
  winRT: group<WinRTConfig>('winRT', {
    experimentalUseOwnMtaApartment: setting<boolean>('winRT.experimentalUseOwnMtaApartment'),
    experimentalReinitializeWinrtApartmentOnMainThread: setting<boolean>(
      'winRT.experimentalReinitializeWinrtApartmentOnMainThread',
    ),
    useDeferredDisconnect: setting<boolean>('winRT.useDeferredDisconnect'),
  }),
  android: group<AndroidConfig>('android', {
    negotiateMtu: setting<boolean>('android.negotiateMtu'),
  }),
  dongl: group<DonglConfig>('dongl', {
    useDonglBackend: setting<boolean>('dongl.useDonglBackend'),
    autoUpdate: setting<boolean>('dongl.autoUpdate'),
    forceUpdate: setting<boolean>('dongl.forceUpdate'),
  }),
  /** Restores every backend's defaults. */
  resetAll(): void {
    native.configReset('all');
  },
});
