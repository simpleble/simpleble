// Runs against an addon built with `npm run build:plain`, using the GATT
// layout of PLAIN's default device (docs/extras/plain-backend.md).
import assert from 'node:assert/strict';
import { once } from 'node:events';
import { after, before, test } from 'node:test';

import { Adapter, SimpleBLEError } from '../dist/index.js';

const uuid = short => `0000${short}-0000-1000-8000-00805f9b34fb`;
const BATTERY = uuid('180f');
const BATTERY_LEVEL = uuid('2a19');
const TEST = uuid('fff0');
const VALUE = uuid('fff1');
const ERROR = uuid('fff2');
const DESCRIPTION = uuid('2901');
const CCCD = uuid('2902');

// Waits for an event without letting Node exit meanwhile.
function within(promise, ms = 5_000) {
  let timer;
  const timeout = new Promise((_, reject) => {
    timer = setTimeout(() => reject(new Error(`Nothing within ${ms} ms`)), ms);
  });
  return Promise.race([promise, timeout]).finally(() => clearTimeout(timer));
}

const [adapter] = await Adapter.getAdapters();
let peripheral;

before(async () => {
  const found = once(adapter, 'scanFound');
  await adapter.scanStart();
  [peripheral] = await within(found);
  await adapter.scanStop();
});

after(async () => {
  if (peripheral.isConnected()) await peripheral.disconnect();
});

test('services() before connecting converts the advertised services and their data', () => {
  const services = peripheral.services();
  const test = services.find(service => service.uuid === TEST);
  assert.deepEqual(test.data, Buffer.from([0x00, 0x7f, 0x80, 0xff]));
  assert.ok(services.some(service => service.uuid === BATTERY));
});

test('connect() and disconnect() reach the device and emit events', async () => {
  const events = [];
  peripheral.on('connected', () => events.push('connected'));
  peripheral.on('disconnected', () => events.push('disconnected'));

  const connected = once(peripheral, 'connected');
  await peripheral.connect();
  await within(connected);
  assert.equal(peripheral.isConnected(), true);
  assert.deepEqual(await adapter.getConnectedPeripherals(), [peripheral]);

  const disconnected = once(peripheral, 'disconnected');
  await peripheral.disconnect();
  await within(disconnected);
  assert.equal(peripheral.isConnected(), false);
  peripheral.removeAllListeners();

  assert.deepEqual(events, ['connected', 'disconnected']);
});

test('services() converts the discovered GATT tree', async () => {
  await peripheral.connect();
  const services = peripheral.services();

  const value = services.find(service => service.uuid === TEST).characteristics.find(c => c.uuid === VALUE);
  assert.deepEqual(value.capabilities, ['read', 'writeRequest', 'writeCommand', 'notify', 'indicate']);
  assert.deepEqual(value.descriptors.map(descriptor => descriptor.uuid).sort(), [DESCRIPTION, CCCD]);

  const level = services.find(service => service.uuid === BATTERY).characteristics.find(c => c.uuid === BATTERY_LEVEL);
  assert.deepEqual(level.capabilities, ['read', 'notify']);
  assert.ok(Object.isFrozen(services[0]) && Object.isFrozen(value.capabilities));
});

test('values and descriptors round-trip, including empty ones', async () => {
  const original = await peripheral.read(TEST, VALUE);
  const description = await peripheral.readDescriptor(TEST, VALUE, DESCRIPTION);
  assert.deepEqual(original, Buffer.from([0x00, 0x7f, 0x80, 0xff]));
  assert.equal(description.toString(), 'Plain value');

  await peripheral.writeRequest(TEST, VALUE, Buffer.from([1, 2, 3]));
  assert.deepEqual(await peripheral.read(TEST, VALUE), Buffer.from([1, 2, 3]));
  await peripheral.writeCommand(TEST, VALUE, new Uint8Array([]));
  assert.deepEqual(await peripheral.read(TEST, VALUE), Buffer.alloc(0));
  await peripheral.writeDescriptor(TEST, VALUE, DESCRIPTION, Buffer.from('changed'));
  assert.equal((await peripheral.readDescriptor(TEST, VALUE, DESCRIPTION)).toString(), 'changed');

  await peripheral.writeRequest(TEST, VALUE, original);
  await peripheral.writeDescriptor(TEST, VALUE, DESCRIPTION, description);
});

test('GATT failures become SimpleBLEError with the SimpleBLE error code', async () => {
  await assert.rejects(
    peripheral.read(TEST, ERROR),
    error => error instanceof SimpleBLEError && error.code === 'OPERATION_FAILED',
  );

  await assert.rejects(peripheral.read(42, VALUE), TypeError);
  await assert.rejects(peripheral.writeRequest(TEST, VALUE, 'text'), TypeError);
  await assert.rejects(peripheral.notify(TEST, VALUE, 'not a function'), TypeError);
});

test('notify() and indicate() subscribe until unsubscribe()', async () => {
  // The CCCD tells notify (1) and indicate (2) apart.
  for (const [subscribe, flag] of [
    [peripheral.notify.bind(peripheral), 1],
    [peripheral.indicate.bind(peripheral), 2],
  ]) {
    const values = [];
    let received;
    const first = new Promise(resolve => (received = resolve));
    await subscribe(TEST, VALUE, value => {
      values.push(value);
      received();
    });
    assert.deepEqual(await peripheral.readDescriptor(TEST, VALUE, CCCD), Buffer.from([flag, 0]));

    await within(first);
    assert.ok(Buffer.isBuffer(values[0]));
    assert.deepEqual(values[0], Buffer.from([0x00, 0x7f, 0x80, 0xff]));

    await peripheral.unsubscribe(TEST, VALUE);
    assert.deepEqual(await peripheral.readDescriptor(TEST, VALUE, CCCD), Buffer.from([0, 0]));
  }
});

test('subscribing again to a characteristic replaces its callback', async () => {
  const replaced = [];
  let received;
  const first = new Promise(resolve => (received = resolve));
  await peripheral.notify(TEST, VALUE, value => replaced.push(value));
  await peripheral.notify(TEST, VALUE, received);

  await within(first);
  await peripheral.unsubscribe(TEST, VALUE);
  assert.deepEqual(replaced, []);
});

test('subscriptions follow call order when calls are not awaited one by one', async () => {
  // Each sequence ends with a subscription, which must keep receiving values.
  const sequences = [
    () => peripheral.unsubscribe(TEST, VALUE),
    () => {
      peripheral.disconnect();
      peripheral.connect();
    },
  ];
  for (const sequence of sequences) {
    await peripheral.notify(TEST, VALUE, () => {});
    sequence();

    let received;
    const value = new Promise(resolve => (received = resolve));
    await peripheral.notify(TEST, VALUE, received);
    await within(value);
    await peripheral.unsubscribe(TEST, VALUE);
  }
});

test('pairing handlers reach the core', async () => {
  // Only Dongl peripherals take them.
  for (const set of ['setPasskeyRequestHandler', 'setPasskeyDisplayHandler', 'setNumericComparisonHandler']) {
    await assert.rejects(peripheral[set](() => {}), { code: 'OPERATION_NOT_SUPPORTED' }, set);
  }
  await assert.rejects(peripheral.setPasskeyRequestHandler('123456'), TypeError);
});

test('unpair() reaches the device', async () => {
  assert.equal(await peripheral.isPaired(), true);
  await peripheral.unpair();
  assert.equal(await peripheral.isPaired(), false);
});
