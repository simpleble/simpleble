// PLAIN simulates one adapter and one peripheral that advertises every 100 ms.
// Its state lasts for the whole app, so tests run in order.
import { Adapter, Backend, Peripheral } from 'simplernble';

import { assert, once, test, within } from './harness';

const ascii = (text: string) => new Uint8Array([...text].map(char => char.charCodeAt(0)));

let adapter: Adapter;

test('backends and adapters keep one object per native object', async () => {
  [adapter] = await Adapter.getAdapters();
  const backends = await Backend.getBackends();
  const plain = backends.find(backend => backend.identifier() === 'Plain');
  assert.ok(plain, `No PLAIN backend in ${backends.map(backend => backend.identifier())}`);
  assert.equal((await Backend.getBackends()).find(backend => backend.identifier() === 'Plain'), plain);

  assert.equal(adapter.identifier(), 'Plain Adapter');
  assert.equal(adapter.address(), 'AA:BB:CC:DD:EE:FF');
  assert.equal((await Adapter.getAdapters())[0], adapter);
  assert.equal((await plain!.adapters())[0], adapter);
  assert.equal(await Adapter.bluetoothEnabled(), true);
  assert.equal(await plain!.bluetoothEnabled(), true);
  assert.equal(await Backend.getNative(), plain);
});

test('scan events deliver the same Peripheral object as the scan results', async () => {
  const events: string[] = [];
  adapter.on('scanStart', () => events.push('scanStart'));
  adapter.on('scanStop', () => events.push('scanStop'));
  const found = once(adapter, 'scanFound');

  await adapter.scanStart();
  const [peripheral] = (await within(found)) as [Peripheral];
  assert.equal(await adapter.scanIsActive(), true);
  const stopped = once(adapter, 'scanStop');
  await adapter.scanStop();
  await within(stopped);
  adapter.removeAllListeners();

  assert.deepEqual(events, ['scanStart', 'scanStop']);
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
  assert.deepEqual(peripheral.manufacturerData(), new Map([[0x004c, ascii('test')]]));
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
  const events: string[] = [];
  adapter.on('powerOff', () => events.push('powerOff'));
  adapter.on('powerOn', () => events.push('powerOn'));

  // Not awaited one by one: the order of the calls alone decides the outcome.
  const lastPowerOn = new Promise<void>(resolve => {
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

  await assert.rejects(adapter.scanFor(1.5), RangeError);
  await assert.rejects(adapter.scanFor(2 ** 31), RangeError);
  await assert.rejects(adapter.scanFor('100' as never), TypeError);
});
