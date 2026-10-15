// Runs against an addon built with `npm run build:plain`. PLAIN simulates one
// adapter and one peripheral that advertises every 100 ms. Its state lasts for
// the whole process, so tests in this file run in order.
import assert from 'node:assert/strict';
import { once } from 'node:events';
import { test } from 'node:test';

import { Adapter, Backend, Peripheral, SimpleBLEError } from '../dist/index.js';

const [adapter] = await Adapter.getAdapters();

// Waits for an event without letting Node exit meanwhile: only active scans and
// pending calls keep it running, and some backends report events after the
// call that caused them has settled.
function within(promise, ms = 5_000) {
  let timer;
  const timeout = new Promise((_, reject) => {
    timer = setTimeout(() => reject(new Error(`No event within ${ms} ms`)), ms);
  });
  return Promise.race([promise, timeout]).finally(() => clearTimeout(timer));
}

test('backends and adapters keep one object per native object', async () => {
  const backends = await Backend.getBackends();
  assert.deepEqual(backends.map(backend => backend.identifier()), ['Plain']);
  assert.equal((await Backend.getBackends())[0], backends[0]);

  assert.equal(adapter.identifier(), 'Plain Adapter');
  assert.equal(adapter.address(), 'AA:BB:CC:DD:EE:FF');
  assert.equal((await Adapter.getAdapters())[0], adapter);
  assert.equal((await backends[0].adapters())[0], adapter);
  assert.equal(await Adapter.bluetoothEnabled(), true);
  assert.equal(await backends[0].bluetoothEnabled(), true);
});

test('scan events deliver the same Peripheral object as the scan results', async () => {
  const events = [];
  const record = name => (...args) => events.push([name, ...args]);
  adapter.on('scanStart', record('scanStart'));
  adapter.on('scanStop', record('scanStop'));
  const found = once(adapter, 'scanFound');

  await adapter.scanStart();
  const [peripheral] = await within(found);
  assert.equal(await adapter.scanIsActive(), true);
  const stopped = once(adapter, 'scanStop');
  await adapter.scanStop();
  await within(stopped);
  adapter.removeAllListeners();

  assert.deepEqual(events.map(([name]) => name), ['scanStart', 'scanStop']);
  assert.ok(peripheral instanceof Peripheral);
  assert.equal(adapter.scanGetResults()[0], peripheral);
  assert.equal(await adapter.scanIsActive(), false);

  assert.equal(peripheral.identifier(), 'Plain Peripheral');
  assert.equal(peripheral.address(), '11:22:33:44:55:66');
  assert.equal(peripheral.addressType(), 'public');
  assert.equal(peripheral.rssi(), -60);
  assert.equal(peripheral.txPower(), 5);
  assert.equal(peripheral.isConnectable(), true);
  assert.equal(peripheral.isConnected(), false);
  assert.deepEqual(peripheral.manufacturerData(), new Map([[0x004c, Buffer.from('test')]]));
});

test('listeners added after removeAllListeners() still receive events', async () => {
  adapter.on('scanStart', () => {});
  adapter.removeAllListeners();

  const started = once(adapter, 'scanStart');
  await adapter.scanStart();
  await within(started);
  await adapter.scanStop();
});

test('operations on one adapter run in call order', async () => {
  const events = [];
  adapter.on('powerOff', () => events.push('powerOff'));
  adapter.on('powerOn', () => events.push('powerOn'));

  // Not awaited one by one: the order of the calls alone decides the outcome.
  const lastPowerOn = new Promise(resolve => {
    let count = 0;
    adapter.on('powerOn', () => ++count === 2 && resolve());
  });
  await Promise.all([adapter.powerOff(), adapter.powerOn(), adapter.powerOff(), adapter.powerOn()]);
  await within(lastPowerOn);
  adapter.removeAllListeners();

  assert.equal(await adapter.isPowered(), true);
  assert.deepEqual(events, ['powerOff', 'powerOn', 'powerOff', 'powerOn']);
});

test('scanFor() checks its argument before calling the core', async () => {
  await adapter.scanFor(0);

  await assert.rejects(adapter.scanFor(-1), RangeError);
  await assert.rejects(adapter.scanFor(1.5), RangeError);
  await assert.rejects(adapter.scanFor(2 ** 31), RangeError);
  await assert.rejects(adapter.scanFor('100'), TypeError);
});

test('native failures become SimpleBLEError with the SimpleBLE error code', async () => {
  assert.equal(await Backend.getNative(), (await Backend.getBackends())[0]);

  // A PLAIN build has no Dongl backend.
  await assert.rejects(Backend.getDongl(), error => {
    assert.ok(error instanceof SimpleBLEError);
    assert.equal(error.code, 'OPERATION_NOT_SUPPORTED');
    return true;
  });
});
