// PLAIN's adapter can host local GATT services, and lists a started host in its
// own scans as `Plain Adapter Peripheral`, so one app exercises both sides.
import { Adapter, LocalCharacteristic, LocalService, type LocalPeripheral, type Peripheral } from 'simplernble';

import { assert, once, test, within } from './harness';

const SERVICE = '00001234-0000-1000-8000-00805f9b34fb';
const VALUE = '00005678-0000-1000-8000-00805f9b34fb';

const bytes = (...values: number[]) => new Uint8Array(values);
const ascii = (text: string) => new Uint8Array([...text].map(char => char.charCodeAt(0)));

let adapter: Adapter;
let host: LocalPeripheral;
let service: LocalService;
let value: LocalCharacteristic;
let peer: Peripheral;

declare const ErrorUtils: {
  getGlobalHandler(): (error: unknown, isFatal?: boolean) => void;
  setGlobalHandler(handler: (error: unknown, isFatal?: boolean) => void): void;
};

test('a local peripheral keeps one object per native object and converts its characteristics', async () => {
  [adapter] = await Adapter.getAdapters();
  host = adapter.createLocalPeripheral();
  service = host.addService(SERVICE);
  value = service.addCharacteristic(VALUE, ['read', 'writeRequest', 'notify']);
  await value.setValue(bytes(0x00, 0xff));
  await host.start();

  assert.ok(service instanceof LocalService && value instanceof LocalCharacteristic);
  assert.equal(host.services()[0], service);
  assert.equal(service.characteristics()[0], value);
  assert.equal(service.uuid(), SERVICE);
  assert.deepEqual(value.capabilities(), ['read', 'writeRequest', 'notify']);
  assert.deepEqual(value.value(), bytes(0x00, 0xff));
  assert.throws(() => service.addCharacteristic(VALUE, ['shout' as never]));

  assert.equal(host.isStarted(), true);
  assert.equal(await host.isAdvertising(), true);
});

test('central operations reach the local events', async () => {
  const found = new Promise<Peripheral>(resolve => {
    adapter.on('scanFound', peripheral => peripheral.identifier() === 'Plain Adapter Peripheral' && resolve(peripheral));
  });
  await adapter.scanStart();
  peer = await within(found);
  await adapter.scanStop();
  adapter.removeAllListeners();

  const connected = once(host, 'clientConnected');
  await peer.connect();
  const [address] = await within(connected);
  assert.equal(address, adapter.address());

  assert.deepEqual(await peer.read(SERVICE, VALUE), bytes(0x00, 0xff));

  const written = once(value, 'write');
  await peer.writeRequest(SERVICE, VALUE, bytes(1, 2, 3));
  assert.deepEqual((await within(written))[0], bytes(1, 2, 3));

  const subscribed = once(value, 'subscribed');
  let received!: (value: Uint8Array) => void;
  const notified = new Promise<Uint8Array>(resolve => (received = resolve));
  await peer.notify(SERVICE, VALUE, received);
  await within(subscribed);
  await value.setValue(bytes(7));
  assert.deepEqual(await within(notified), bytes(7));

  const unsubscribed = once(value, 'unsubscribed');
  await peer.unsubscribe(SERVICE, VALUE);
  await within(unsubscribed);

  const disconnected = once(host, 'clientDisconnected');
  await peer.disconnect();
  assert.deepEqual(await within(disconnected), [address]);
});

test('read handlers answer reads with a value or a promise until cleared', async () => {
  await peer.connect();

  await value.setReadHandler(() => ascii('now'));
  assert.deepEqual(await peer.read(SERVICE, VALUE), ascii('now'));

  await value.setReadHandler(async () => {
    await new Promise(resolve => setTimeout(resolve, 50));
    return bytes(9);
  });
  assert.deepEqual(await peer.read(SERVICE, VALUE), bytes(9));

  await value.setReadHandler(null);
  assert.deepEqual(await peer.read(SERVICE, VALUE), value.value());

  await assert.rejects(value.setReadHandler('value' as never), TypeError);
});

test('a failing read handler answers an empty value and its error is uncaught', async () => {
  const previous = ErrorUtils.getGlobalHandler();
  let caught!: (error: unknown) => void;
  const uncaught = new Promise<unknown>(resolve => (caught = resolve));
  ErrorUtils.setGlobalHandler(error => caught(error));
  try {
    await value.setReadHandler(() => {
      throw new Error('from handler');
    });
    assert.deepEqual(await peer.read(SERVICE, VALUE), bytes());
    assert.equal(((await within(uncaught)) as Error).message, 'from handler');
  } finally {
    ErrorUtils.setGlobalHandler(previous);
    await value.setReadHandler(null);
  }
  await peer.disconnect();
  await host.stop();
  assert.equal(host.isStarted(), false);
});
