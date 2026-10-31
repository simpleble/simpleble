// Compiled by `npm run test:types`, never run. Checks the published types.
import {
  Adapter,
  Backend,
  Config,
  LocalCharacteristic,
  LocalPeripheral,
  LocalService,
  Logging,
  Peripheral,
  SimpleBLEError,
  type CharacteristicCapability,
  type ErrorCode,
  type LogEntry,
} from 'simplejsble';

const [backend] = await Backend.getBackends();
const name: string = backend.identifier();
const [adapter] = await backend.adapters();

adapter.on('scanFound', (peripheral: Peripheral) => {
  const rssi: number | undefined = peripheral.rssi();
  const data: Map<number, Buffer> = peripheral.manufacturerData();
});
adapter.on('scanStop', () => {});

// @ts-expect-error Unknown events are rejected.
adapter.on('scanFond', () => {});
// @ts-expect-error Event arguments are checked.
adapter.on('scanFound', (peripheral: string) => {});
// @ts-expect-error Objects come from the library, not from constructors.
new Adapter();

// @ts-expect-error Getters are synchronous.
const address: Promise<string> = adapter.address();
// @ts-expect-error I/O returns a promise.
adapter.scanStart() satisfies void;
// @ts-expect-error A Dongl answers over its serial port, so this is I/O too.
const powered: boolean = adapter.isPowered();

const results: Peripheral[] = adapter.scanGetResults();

const [peripheral] = results;
peripheral.on('disconnected', () => {});
// @ts-expect-error Peripheral events are checked too.
peripheral.on('disconected', () => {});
await peripheral.connect();
for (const service of peripheral.services()) {
  for (const characteristic of service.characteristics) {
    const capability: CharacteristicCapability | undefined = characteristic.capabilities[0];
    const descriptor: string | undefined = characteristic.descriptors[0]?.uuid;
  }
}
const value: Buffer = await peripheral.read('service', 'characteristic');
await peripheral.writeRequest('service', 'characteristic', new Uint8Array([1]));
// @ts-expect-error Values are bytes.
await peripheral.writeCommand('service', 'characteristic', 'text');
await peripheral.notify('service', 'characteristic', (data: Buffer) => {});
// @ts-expect-error The callback receives a Buffer.
await peripheral.indicate('service', 'characteristic', (data: string) => {});
// @ts-expect-error Service trees are read-only.
peripheral.services()[0].uuid = 'changed';
const paired: Peripheral[] = await adapter.getPairedPeripherals();
const cached: Peripheral[] = await adapter.retrieveCachedPeripherals([peripheral.address()]);

await peripheral.setPasskeyRequestHandler(async () => '123456');
await peripheral.setNumericComparisonHandler((passkey: string) => passkey === '123456');
await peripheral.secureConnection(true);
await peripheral.setPasskeyDisplayHandler(null);
// @ts-expect-error A passkey is a string.
await peripheral.setPasskeyRequestHandler(() => 123456);

Config.simpleBluez.interactivePairing = true;
await adapter.setPasskeyRequestHandler(async (peer: string) => '123456');
await adapter.setPasskeyDisplayHandler((peer: string, passkey: string) => {});
await adapter.setNumericComparisonHandler(async (peer, passkey) => passkey === '123456');
await adapter.setPairingAuthorizationHandler(null);
// @ts-expect-error Authorization answers with a boolean.
await adapter.setPairingAuthorizationHandler((peer: string) => 'yes');

const host: LocalPeripheral = adapter.createLocalPeripheral();
const localService: LocalService = host.addService('service');
const localValue: LocalCharacteristic = localService.addCharacteristic('characteristic', ['read', 'notify']);
// @ts-expect-error Capabilities are checked.
localService.addCharacteristic('characteristic', ['shout']);
localValue.on('write', (data: Buffer) => {});
host.on('clientConnected', (address: string) => {});
// @ts-expect-error Local events are checked too.
host.on('clientConected', () => {});
await localValue.setReadHandler(async () => new Uint8Array([1]));
// @ts-expect-error Read handlers return bytes.
await localValue.setReadHandler(() => 'text');
host.setAdvertisementLocalName('Name');
host.setAdvertisementLocalName(null);
await host.start();
const advertising: boolean = await host.isAdvertising();

{
  await using closing = adapter;
}
await adapter.close();

Config.dongl.useDonglBackend = false;
// @ts-expect-error Settings are typed.
Config.simpleBluez.connectionTimeout = '2s';

try {
  await adapter.powerOn();
} catch (error) {
  if (error instanceof SimpleBLEError) {
    const code: ErrorCode = error.code;
    const hresult: number | undefined = error.nativeCode;
  }
}

export { name, results, paired };

Logging.level = 'warn';
// @ts-expect-error Levels are checked.
Logging.level = 'loud';
Logging.setCallback((entry: LogEntry) => console.log(entry.level, entry.message));
Logging.setCallback(null);
Logging.logToFile('ble.log');
