import assert from 'node:assert/strict';
import { test } from 'node:test';

import { Config } from '../dist/index.js';

const groups = {
  simpleBluez: ['useSystemBus', 'connectionTimeout', 'disconnectionTimeout', 'interactivePairing'],
  winRT: ['experimentalUseOwnMtaApartment', 'experimentalReinitializeWinrtApartmentOnMainThread', 'useDeferredDisconnect'],
  dongl: ['useDonglBackend', 'autoUpdate', 'forceUpdate'],
};

function snapshot() {
  return Object.fromEntries(
    Object.entries(groups).map(([group, names]) => [group, Object.fromEntries(names.map(name => [name, Config[group][name]]))]),
  );
}

// A value different from `current`, and different for every duration setting.
function changed(current, index) {
  return typeof current === 'boolean' ? !current : current + 1000 * (index + 1);
}

function changeAll() {
  let index = 0;
  for (const [group, names] of Object.entries(groups)) {
    for (const name of names) Config[group][name] = changed(Config[group][name], index++);
  }
}

test('every setting reaches its own native setting', t => {
  t.after(() => Config.resetAll());
  const before = snapshot();

  // Changing all of them at once shows a setting wired to the wrong native one.
  changeAll();
  let index = 0;
  for (const [group, names] of Object.entries(groups)) {
    for (const name of names) assert.equal(Config[group][name], changed(before[group][name], index++), `${group}.${name}`);
  }
});

test('each reset() restores only its own group, and resetAll() restores every group', t => {
  t.after(() => Config.resetAll());
  const defaults = snapshot();

  for (const group of Object.keys(groups)) {
    Config.resetAll();
    changeAll();
    const changedValues = snapshot();
    for (const other of Object.keys(groups)) assert.notDeepEqual(changedValues[other], defaults[other]);
    Config[group].reset();
    const after = snapshot();
    for (const other of Object.keys(groups)) {
      assert.deepEqual(after[other], other === group ? defaults[other] : changedValues[other], `${group}.reset() and ${other}`);
    }
  }

  changeAll();
  Config.resetAll();
  assert.deepEqual(snapshot(), defaults);
});

test('settings reject values of the wrong type', t => {
  t.after(() => Config.resetAll());
  assert.throws(() => (Config.dongl.useDonglBackend = 1), TypeError);
  assert.throws(() => (Config.simpleBluez.connectionTimeout = '5'), TypeError);
  assert.throws(() => (Config.simpleBluez.connectionTimeout = -1), RangeError);
});
