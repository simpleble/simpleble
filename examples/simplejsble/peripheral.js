import { once } from 'node:events';
import { createInterface } from 'node:readline/promises';

import { Adapter } from 'simplejsble';

const SERVICE_UUID = '12345678-1234-5678-1234-56789abcdef0';
const CHARACTERISTIC_UUID = '12345678-1234-5678-1234-56789abcdef1';

const input = createInterface({ input: process.stdin, output: process.stdout });

// Lists `items` and returns the one the user picks.
async function choose(title, items, describe) {
  console.log(`Please select ${title}:`);
  items.forEach((item, i) => console.log(`${i}: ${describe(item)}`));
  const choice = Number(await input.question('Enter choice: '));
  if (!Number.isInteger(choice) || !items[choice]) throw new Error('Invalid choice');
  return items[choice];
}

const adapters = await Adapter.getAdapters();
if (adapters.length === 0) throw new Error('No adapters found');

const adapter = await choose('an adapter', adapters, adapter => `${adapter.identifier()} [${adapter.address()}]`);
input.close();

const host = adapter.createLocalPeripheral();

// Add a service UUID to the advertisement.
host.addAdvertisedService(SERVICE_UUID);

const service = host.addService(SERVICE_UUID);
const characteristic = service.addCharacteristic(CHARACTERISTIC_UUID, [
  'read',
  'writeRequest',
  'writeCommand',
  'notify',
  'indicate',
]);
await characteristic.setValue(Buffer.from('ready'));

characteristic.on('write', async value => {
  console.log(`Write: ${value.toString('hex')}`);
  // Echo the written value to subscribed clients.
  await characteristic.setValue(value);
});
characteristic.on('subscribed', () => console.log('Client subscribed.'));
characteristic.on('unsubscribed', () => console.log('Client unsubscribed.'));

host.on('clientConnected', address => console.log(`Client connected: ${address}`));
host.on('clientDisconnected', address => console.log(`Client disconnected: ${address}`));

await host.start();
console.log('Advertising. Press Ctrl+C to stop.');

await once(process, 'SIGINT');
await host.stop();
