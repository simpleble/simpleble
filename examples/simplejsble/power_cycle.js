import { setTimeout as sleep } from 'node:timers/promises';

import { Adapter } from 'simplejsble';

const adapters = await Adapter.getAdapters();
if (adapters.length === 0) console.log('No adapters found');

for (const adapter of adapters) {
  console.log(`Adapter: ${adapter.identifier()} [${adapter.address()}]`);

  console.log('Powering off adapter');
  await adapter.powerOff();
  console.log(`Adapter powered: ${await adapter.isPowered()}`);
  await sleep(1000);

  console.log('Powering on adapter');
  await adapter.powerOn();
  console.log(`Adapter powered: ${await adapter.isPowered()}`);
}
