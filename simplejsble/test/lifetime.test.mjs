// Process-level behavior, each case in its own Node process. Needs an addon
// built with `npm run build:plain`.
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { test } from 'node:test';

// NOTE: A URL, because import() doesn't take Windows paths.
const entry = new URL('../dist/index.js', import.meta.url).href;

function node(source, args = []) {
  return spawnSync(process.execPath, [...args, '--input-type=module', '-e', source], {
    encoding: 'utf8',
    timeout: 20_000,
    env: { ...process.env, SIMPLEJSBLE_ENTRY: entry },
  });
}

test('an adapter with listeners keeps receiving events after its last reference is dropped', () => {
  const child = node(
    `
    const { Adapter } = await import(process.env.SIMPLEJSBLE_ENTRY);
    let found = false;
    await (async () => {
      const [adapter] = await Adapter.getAdapters();
      adapter.on('scanFound', () => (found = true));
      await adapter.scanStart();
    })();
    for (let i = 0; i < 20 && !found; i++) {
      globalThis.gc();
      await new Promise(resolve => setTimeout(resolve, 50));
    }
    if (!found) throw new Error('no scanFound after garbage collection');
    const [adapter] = await Adapter.getAdapters();
    await adapter.scanStop();
    `,
    ['--expose-gc'],
  );
  assert.equal(child.status, 0, child.stderr);
});

test('listeners alone do not keep Node running', () => {
  const child = node(`
    const { Adapter } = await import(process.env.SIMPLEJSBLE_ENTRY);
    const [adapter] = await Adapter.getAdapters();
    adapter.on('scanFound', () => {});
  `);
  assert.equal(child.error, undefined);
  assert.equal(child.status, 0, child.stderr);
});

test('an active scan keeps Node running until it stops', () => {
  const child = node(`
    import { once } from 'node:events';
    const { Adapter } = await import(process.env.SIMPLEJSBLE_ENTRY);
    const [adapter] = await Adapter.getAdapters();
    await adapter.scanStart();
    const [peripheral] = await once(adapter, 'scanFound');
    console.log(peripheral.identifier());
    await adapter.scanStop();
  `);
  assert.equal(child.error, undefined);
  assert.equal(child.status, 0, child.stderr);
  assert.equal(child.stdout.trim(), 'Plain Peripheral');
});

test('a scan ended by powering off no longer keeps Node running', () => {
  // PLAIN, like several backends, stops scanning on power-off without a scan-stop callback.
  const child = node(`
    const { Adapter } = await import(process.env.SIMPLEJSBLE_ENTRY);
    const [adapter] = await Adapter.getAdapters();
    await adapter.scanStart();
    await adapter.powerOff();
    await adapter.powerOn();
  `);
  assert.equal(child.error, undefined);
  assert.equal(child.status, 0, child.stderr);
});

test('a pending operation keeps Node running until it settles', () => {
  const child = node(`
    const { Adapter } = await import(process.env.SIMPLEJSBLE_ENTRY);
    const [adapter] = await Adapter.getAdapters();
    adapter.scanFor(300).then(() => console.log('settled'));
  `);
  assert.equal(child.status, 0, child.stderr);
  assert.equal(child.stdout.trim(), 'settled');
});

test('an exception in a listener is uncaught and later events still arrive', () => {
  const child = node(`
    const { Adapter } = await import(process.env.SIMPLEJSBLE_ENTRY);
    process.on('uncaughtException', error => console.log('uncaught', error.message));
    const [adapter] = await Adapter.getAdapters();
    adapter.on('scanStart', () => { throw new Error('from listener'); });
    adapter.on('scanStop', () => console.log('scanStop'));
    await adapter.scanStart();
    await adapter.scanStop();
  `);
  assert.equal(child.status, 0, child.stderr);
  assert.deepEqual(child.stdout.trim().split('\n'), ['uncaught from listener', 'scanStop']);
});

test('a second Node environment cannot load the addon while another owns it', () => {
  const child = node(`
    import { Worker } from 'node:worker_threads';
    import { once } from 'node:events';
    await import(process.env.SIMPLEJSBLE_ENTRY);
    const worker = new Worker(
      'import(process.env.SIMPLEJSBLE_ENTRY).then(() => process.exit(1), error => { console.log(error.code); });',
      { eval: true },
    );
    await once(worker, 'exit');
  `);
  assert.equal(child.status, 0, child.stderr);
  assert.equal(child.stdout.trim(), 'ERR_SIMPLEJSBLE_ENVIRONMENT');
});

test('a terminated worker releases the addon for the next environment', () => {
  const child = node(`
    import { Worker } from 'node:worker_threads';
    import { once } from 'node:events';
    // NOTE: Workers inherit --input-type=module, so this source is a module too.
    const source = \`
      import { parentPort } from 'node:worker_threads';
      const { Adapter } = await import(process.env.SIMPLEJSBLE_ENTRY);
      const [adapter] = await Adapter.getAdapters();
      adapter.on('scanUpdated', () => {});
      await adapter.scanStart();
      parentPort.postMessage('scanning');
    \`;
    for (let i = 0; i < 10; i++) {
      const worker = new Worker(source, { eval: true });
      await once(worker, 'message');
      await worker.terminate();
    }
    const { Adapter } = await import(process.env.SIMPLEJSBLE_ENTRY);
    const [adapter] = await Adapter.getAdapters();
    await adapter.scanStop();
    console.log('owned');
  `);
  assert.equal(child.status, 0, child.stderr);
  assert.equal(child.stdout.trim(), 'owned');
});
