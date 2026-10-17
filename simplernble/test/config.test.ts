import { Config, Logging, type LogEntry, Adapter } from 'simplernble';

import { assert, test } from './harness';

test('settings reach the native settings, reject unknown values, and reset', () => {
  assert.equal(Config.android.connectionPriorityRequest, 'disabled');
  Config.android.connectionPriorityRequest = 'high';
  assert.equal(Config.android.connectionPriorityRequest, 'high');
  Config.android.reset();
  assert.equal(Config.android.connectionPriorityRequest, 'disabled');

  Config.android.connectionPriorityRequest = 'lowPower';
  Config.resetAll();
  assert.equal(Config.android.connectionPriorityRequest, 'disabled');

  assert.throws(() => (Config.android.connectionPriorityRequest = 'fast' as never), TypeError);
});

// SimpleBLE logs a warning, and doesn't scan, when scanning with Bluetooth off.
test('log entries reach the callback, null discards them, and the level filters them', async () => {
  const [adapter] = await Adapter.getAdapters();
  const entries: LogEntry[] = [];
  const wait = () => new Promise(resolve => setTimeout(resolve, 200));
  await adapter.powerOff();
  try {
    Logging.setCallback(entry => entries.push(entry));
    await adapter.scanStart();
    await wait();
    assert.equal(entries.length, 1);
    assert.equal(entries[0].level, 'warn');
    assert.equal(entries[0].module, 'SimpleBLE');
    assert.equal(entries[0].message, 'Bluetooth is not enabled.');
    assert.equal(typeof entries[0].line, 'number');
    assert.ok(Object.isFrozen(entries[0]));

    Logging.level = 'error';
    await adapter.scanStart();
    await wait();
    assert.equal(entries.length, 1);

    Logging.level = 'info';
    Logging.setCallback(null);
    await adapter.scanStart();
    await wait();
    assert.equal(entries.length, 1);

    assert.equal(Logging.level, 'info');
    assert.throws(() => (Logging.level = 'loud' as never), TypeError);
  } finally {
    Logging.logToSystem();
    await adapter.powerOn();
  }
});
