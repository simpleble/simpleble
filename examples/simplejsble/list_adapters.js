import { Adapter } from 'simplejsble';

console.log(`Bluetooth enabled: ${await Adapter.bluetoothEnabled()}`);

const adapters = await Adapter.getAdapters();
if (adapters.length === 0) console.log('No adapters found');

for (const adapter of adapters) {
  console.log(`Adapter: ${adapter.identifier()} [${adapter.address()}]`);
}
