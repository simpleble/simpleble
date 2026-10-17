import { existsSync } from 'node:fs';
import { join } from 'node:path';

// Shapes of the objects the addon returns. They are internal: the public
// classes own them, and the published declarations leave them out.

/** @internal */
export interface BackendHandle {
  key(): string;
  identifier(): string;
  bluetoothEnabled(): Promise<boolean>;
  adapters(): Promise<AdapterHandle[]>;
}

/** @internal */
export interface AdapterHandle {
  key(): string;
  identifier(): string;
  address(): string;
  isPowered(): Promise<boolean>;
  powerOn(): Promise<void>;
  powerOff(): Promise<void>;
  scanStart(): Promise<void>;
  scanStop(): Promise<void>;
  scanFor(timeoutMs: number): Promise<void>;
  scanIsActive(): Promise<boolean>;
  scanGetResults(): PeripheralHandle[];
  getPairedPeripherals(): Promise<PeripheralHandle[]>;
  getConnectedPeripherals(): Promise<PeripheralHandle[]>;
  retrieveCachedPeripherals(identifiers: readonly string[]): Promise<PeripheralHandle[]>;
  subscribe(): void;
  createLocalPeripheral(): LocalPeripheralHandle;
  connections(): PeripheralHandle[];
  hosts(): LocalPeripheralHandle[];
  close(): Promise<void>;
  release(): void;
}

/** @internal */
export interface PeripheralHandle {
  key(): string;
  identifier(): string;
  address(): string;
  addressType(): 'public' | 'random' | 'unspecified';
  rssi(): number | undefined;
  txPower(): number | undefined;
  mtu(): number;
  isConnected(): boolean;
  isConnectable(): boolean;
  isPaired(): Promise<boolean>;
  manufacturerData(): [number, Buffer][];
  connect(): Promise<void>;
  disconnect(): Promise<void>;
  unpair(): Promise<void>;
  services(): NativeService[];
  read(service: string, characteristic: string): Promise<Buffer>;
  writeRequest(service: string, characteristic: string, data: Uint8Array): Promise<void>;
  writeCommand(service: string, characteristic: string, data: Uint8Array): Promise<void>;
  readDescriptor(service: string, characteristic: string, descriptor: string): Promise<Buffer>;
  writeDescriptor(service: string, characteristic: string, descriptor: string, data: Uint8Array): Promise<void>;
  notify(service: string, characteristic: string, id: number): Promise<void>;
  indicate(service: string, characteristic: string, id: number): Promise<void>;
  unsubscribe(service: string, characteristic: string): Promise<void>;
  subscribe(): void;
  setPasskeyRequestHandler(id: number | undefined): Promise<void>;
  setPasskeyDisplayHandler(id: number | undefined): Promise<void>;
  setNumericComparisonHandler(id: number | undefined): Promise<void>;
  owner(): string;
  release(): void;
}

/** @internal */
export interface LocalPeripheralHandle {
  key(): string;
  addAdvertisedService(uuid: string | readonly string[]): void;
  setAdvertisedManufacturerData(companyId: number, data: Uint8Array): void;
  setConnectable(connectable: boolean): void;
  setAdvertisementLocalName(name: string | null): void;
  addService(uuid: string): LocalServiceHandle;
  services(): LocalServiceHandle[];
  removeAllServices(): void;
  start(): Promise<void>;
  stop(): Promise<void>;
  isStarted(): boolean;
  isAdvertising(): Promise<boolean>;
  subscribe(): void;
  owner(): string;
  release(): void;
}

/** @internal */
export interface LocalServiceHandle {
  key(): string;
  uuid(): string;
  addCharacteristic(uuid: string, capabilities: readonly string[]): LocalCharacteristicHandle;
  characteristics(): LocalCharacteristicHandle[];
  owner(): string;
  release(): void;
}

/** @internal */
export interface LocalCharacteristicHandle {
  key(): string;
  uuid(): string;
  capabilities(): string[];
  value(): Buffer;
  setValue(value: Uint8Array): Promise<void>;
  setReadHandler(id: number | undefined): Promise<void>;
  subscribe(): void;
  owner(): string;
  release(): void;
}

/** @internal */
export interface NativeService {
  uuid: string;
  data: Buffer;
  characteristics: { uuid: string; capabilities: string[]; descriptors: { uuid: string }[] }[];
}

/** @internal */
export interface NativeModule {
  setup(options: {
    makeError(code: string, message: string, nativeCode?: number): Error;
    dispatch(key: string, event: string, argument?: unknown, peripheral?: object): void;
    deliver(id: number, value: Buffer): void;
    answer(id: number, argument: string | undefined, respond?: (value: unknown) => void): void;
  }): void;
  getBackends(): Promise<BackendHandle[]>;
  getNativeBackend(): Promise<BackendHandle>;
  getDonglBackend(): Promise<BackendHandle>;
  getSimulatorBackend(): Promise<BackendHandle>;
  getAdapters(): Promise<AdapterHandle[]>;
  peripheralHandle(peripheral: object): PeripheralHandle;
  bluetoothEnabled(): Promise<boolean>;
  configGet(name: string): boolean | number;
  configSet(name: string, value: boolean | number): void;
  configReset(group: string): void;
}

function load(): NativeModule {
  const target = `${process.platform}-${process.arch}`;
  const path = join(__dirname, '..', 'prebuilds', target, 'simplejsble.node');
  if (!existsSync(path)) {
    throw new Error(`simplejsble has no native addon for ${target}`);
  }
  return require(path) as NativeModule;
}

/** @internal */
export const native = load();
