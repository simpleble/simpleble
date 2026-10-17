// Runs against an addon built with `npm run build:plain`. PLAIN's adapter can
// host local GATT services, and lists a started host in its own scans as
// `Plain Adapter Peripheral`, so one process exercises both sides.
import assert from 'node:assert/strict';
import { once } from 'node:events';
import { after, before, test } from 'node:test';

import { Adapter, LocalCharacteristic, LocalService } from '../dist/index.js';

const SERVICE = '00001234-0000-1000-8000-00805f9b34fb';
const VALUE = '00005678-0000-1000-8000-00805f9b34fb';

// Waits for an event without letting Node exit meanwhile.
function within(promise, ms = 5_000) {
  let timer;
  const timeout = new Promise((_, reject) => {
    timer = setTimeout(() => reject(new Error(`Nothing within ${ms} ms`)), ms);
  });
  return Promise.race([promise, timeout]).finally(() => clearTimeout(timer));
}

const [adapter] = await Adapter.getAdapters();
const host = adapter.createLocalPeripheral();
let service;
let value;
let peer;

before(async () => {
  service = host.addService(SERVICE);
  value = service.addCharacteristic(VALUE, ['read', 'writeRequest', 'notify']);
  await value.setValue(Buffer.from([0x00, 0xff]));
  await host.start();

  const found = new Promise(resolve => {
    adapter.on('scanFound', peripheral => peripheral.identifier() === 'Plain Adapter Peripheral' && resolve(peripheral));
  });
  await adapter.scanStart();
  peer = await within(found);
  await adapter.scanStop();
  adapter.removeAllListeners();
});

after(async () => {
  if (peer.isConnected()) await peer.disconnect();
  await host.stop();
});

test('a local peripheral keeps one object per native object and converts its characteristics', async () => {
  assert.ok(service instanceof LocalService && value instanceof LocalCharacteristic);
  assert.equal(host.services()[0], service);
  assert.equal(service.characteristics()[0], value);
  assert.equal(service.uuid(), SERVICE);
  assert.deepEqual(value.capabilities(), ['read', 'writeRequest', 'notify']);
  assert.deepEqual(value.value(), Buffer.from([0x00, 0xff]));
  assert.throws(() => service.addCharacteristic(VALUE, ['shout']), TypeError);

  assert.equal(host.isStarted(), true);
  assert.equal(await host.isAdvertising(), true);
});

test('central operations reach the local events', async () => {
  const connected = once(host, 'clientConnected');
  await peer.connect();
  const [address] = await within(connected);
  assert.equal(address, adapter.address());

  assert.deepEqual(await peer.read(SERVICE, VALUE), Buffer.from([0x00, 0xff]));

  const written = once(value, 'write');
  await peer.writeRequest(SERVICE, VALUE, Buffer.from([1, 2, 3]));
  assert.deepEqual((await within(written))[0], Buffer.from([1, 2, 3]));

  const subscribed = once(value, 'subscribed');
  let received;
  const notified = new Promise(resolve => (received = resolve));
  await peer.notify(SERVICE, VALUE, received);
  await within(subscribed);
  await value.setValue(Buffer.from([7]));
  assert.deepEqual(await within(notified), Buffer.from([7]));

  const unsubscribed = once(value, 'unsubscribed');
  await peer.unsubscribe(SERVICE, VALUE);
  await within(unsubscribed);

  const disconnected = once(host, 'clientDisconnected');
  await peer.disconnect();
  assert.deepEqual(await within(disconnected), [address]);
});

test('read handlers answer reads with a value or a promise until cleared', async () => {
  await peer.connect();

  await value.setReadHandler(() => Buffer.from('now'));
  assert.equal((await peer.read(SERVICE, VALUE)).toString(), 'now');

  await value.setReadHandler(async () => {
    await new Promise(resolve => setTimeout(resolve, 50));
    return new Uint8Array([9]);
  });
  assert.deepEqual(await peer.read(SERVICE, VALUE), Buffer.from([9]));

  await value.setReadHandler(null);
  assert.deepEqual(await peer.read(SERVICE, VALUE), value.value());

  await assert.rejects(value.setReadHandler('value'), TypeError);
  await peer.disconnect();
});

test('platform-specific calls check their arguments and report other platforms as unsupported', async () => {
  const unsupported = { name: 'SimpleBLEError', code: 'OPERATION_NOT_SUPPORTED' };
  assert.throws(() => host.setAdvertisementLocalName('Name'), unsupported);
  assert.throws(() => host.setAdvertisementLocalName(5), TypeError);
  await assert.rejects(adapter.retrieveCachedPeripherals([peer.address()]), unsupported);
  await assert.rejects(adapter.retrieveCachedPeripherals([5]), TypeError);
});
