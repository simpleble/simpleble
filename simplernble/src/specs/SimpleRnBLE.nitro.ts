// The native objects. They are internal: the public classes in src/ own them,
// and the published declarations leave them out.
import type { HybridObject } from 'react-native-nitro-modules';

type Native = HybridObject<{ ios: 'c++'; android: 'c++' }>;

export type AddressType = 'public' | 'random' | 'unspecified';

export type AdapterEvent = 'scanStart' | 'scanStop' | 'scanFound' | 'scanUpdated' | 'powerOn' | 'powerOff';

export type PeripheralEvent = 'connected' | 'disconnected';

export type LocalPeripheralEvent = 'clientConnected' | 'clientDisconnected';

export type LocalCharacteristicEvent = 'subscribed' | 'unsubscribed';

// NOTE: Log levels cross as numbers, from 0 (none) to 6 (verbose): an enum's
// DEBUG would clash with the DEBUG macro of iOS debug builds.
export interface LogEntry {
  level: number;
  module: string;
  file: string;
  line: number;
  function: string;
  message: string;
}

export type ConnectionPriorityRequest = 'disabled' | 'balanced' | 'high' | 'lowPower' | 'dck';

export interface ManufacturerDataEntry {
  companyId: number;
  data: ArrayBuffer;
}

export interface NativeDescriptor {
  uuid: string;
}

export interface NativeCharacteristic {
  uuid: string;
  capabilities: string[];
  descriptors: NativeDescriptor[];
}

export interface NativeService {
  uuid: string;
  data: ArrayBuffer;
  characteristics: NativeCharacteristic[];
}

export interface PeripheralHandle extends Native {
  identifier(): string;
  address(): string;
  addressType(): AddressType;
  rssi(): number | undefined;
  txPower(): number | undefined;
  mtu(): number;
  isConnected(): boolean;
  isConnectable(): boolean;
  isPaired(): Promise<boolean>;
  manufacturerData(): ManufacturerDataEntry[];
  services(): NativeService[];
  connect(): Promise<void>;
  disconnect(): Promise<void>;
  unpair(): Promise<void>;
  read(service: string, characteristic: string): Promise<ArrayBuffer>;
  writeRequest(service: string, characteristic: string, data: ArrayBuffer): Promise<void>;
  writeCommand(service: string, characteristic: string, data: ArrayBuffer): Promise<void>;
  readDescriptor(service: string, characteristic: string, descriptor: string): Promise<ArrayBuffer>;
  writeDescriptor(service: string, characteristic: string, descriptor: string, data: ArrayBuffer): Promise<void>;
  // Values of the subscription with the id arrive through subscribe()'s onValue.
  notify(service: string, characteristic: string, id: number): Promise<void>;
  indicate(service: string, characteristic: string, id: number): Promise<void>;
  unsubscribe(service: string, characteristic: string): Promise<void>;
  // Called once, when the public Peripheral is created. Events and values arrive in order.
  subscribe(onEvent: (event: PeripheralEvent) => void, onValue: (id: number, value: ArrayBuffer) => void): void;
}

export interface LocalCharacteristicHandle extends Native {
  uuid(): string;
  capabilities(): string[];
  value(): ArrayBuffer;
  setValue(value: ArrayBuffer): Promise<void>;
  // NOTE: An ArrayBuffer from JavaScript is only readable on the JavaScript
  // thread, and a read waits on another, so the handler answers bytes as numbers.
  setReadHandler(handler: (() => Promise<number[]>) | undefined): Promise<void>;
  // Called once, when the public LocalCharacteristic is created. Events arrive in order.
  subscribe(onEvent: (event: LocalCharacteristicEvent) => void, onWrite: (value: ArrayBuffer) => void): void;
}

export interface LocalServiceHandle extends Native {
  uuid(): string;
  addCharacteristic(uuid: string, capabilities: string[]): LocalCharacteristicHandle;
  characteristics(): LocalCharacteristicHandle[];
}

export interface LocalPeripheralHandle extends Native {
  addAdvertisedService(uuids: string[]): void;
  setAdvertisedManufacturerData(companyId: number, data: ArrayBuffer): void;
  setConnectable(connectable: boolean): void;
  setAdvertisementLocalName(name: string | undefined): void;
  addService(uuid: string): LocalServiceHandle;
  services(): LocalServiceHandle[];
  removeAllServices(): void;
  start(): Promise<void>;
  stop(): Promise<void>;
  isStarted(): boolean;
  isAdvertising(): Promise<boolean>;
  // Called once, when the public LocalPeripheral is created. Events arrive in order.
  subscribe(listener: (event: LocalPeripheralEvent, address: string) => void): void;
}

export interface AdapterHandle extends Native {
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
  retrieveCachedPeripherals(identifiers: string[]): Promise<PeripheralHandle[]>;
  createLocalPeripheral(): LocalPeripheralHandle;
  // Called once, when the public Adapter is created. Events arrive in order.
  subscribe(listener: (event: AdapterEvent, peripheral: PeripheralHandle | undefined) => void): void;
}

export interface BackendHandle extends Native {
  identifier(): string;
  bluetoothEnabled(): Promise<boolean>;
  adapters(): Promise<AdapterHandle[]>;
}

export interface SimpleRnBLE extends Native {
  getBackends(): Promise<BackendHandle[]>;
  getNativeBackend(): Promise<BackendHandle>;
  getAdapters(): Promise<AdapterHandle[]>;
  bluetoothEnabled(): Promise<boolean>;

  connectionPriorityRequest: ConnectionPriorityRequest;
  resetConfig(): void;

  logLevel: number;
  setLogCallback(callback: ((entry: LogEntry) => void) | undefined): void;
  logToSystem(): void;
  logToFile(path: string | undefined): void;
}
