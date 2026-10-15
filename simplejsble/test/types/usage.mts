// Compiled by `npm run test:types`, never run. Checks the published types.
import { Adapter, Backend, Config, Peripheral, SimpleBLEError, type ErrorCode } from 'simplejsble';

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
const paired: Peripheral[] = await adapter.getPairedPeripherals();

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
