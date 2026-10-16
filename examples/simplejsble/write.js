import { createInterface } from 'node:readline/promises';

import { Adapter } from 'simplejsble';

const input = createInterface({ input: process.stdin, output: process.stdout });

// Lists `items` and returns the one the user picks.
async function choose(title, items, describe) {
  console.log(`Please select ${title}:`);
  items.forEach((item, i) => console.log(`${i}: ${describe(item)}`));
  const choice = Number(await input.question('Enter choice: '));
  if (!Number.isInteger(choice) || !items[choice]) throw new Error('Invalid choice');
  return items[choice];
}

const describe = device => `${device.identifier()} [${device.address()}]`;

const adapters = await Adapter.getAdapters();
if (adapters.length === 0) throw new Error('No adapters found');

const adapter = await choose('an adapter', adapters, describe);
console.log(`Selected adapter: ${describe(adapter)}`);

adapter.on('scanStart', () => console.log('Scan started.'));
adapter.on('scanStop', () => console.log('Scan complete.'));
adapter.on('scanFound', peripheral => console.log(`Found ${describe(peripheral)}`));

// Scan for 5 seconds
await adapter.scanFor(5000);
adapter.removeAllListeners();

const peripherals = adapter.scanGetResults();
if (peripherals.length === 0) throw new Error('No peripherals found');
const peripheral = await choose('a peripheral', peripherals, describe);

console.log(`Connecting to: ${describe(peripheral)}`);
await peripheral.connect();

console.log('Successfully connected, listing services...');
const pairs = peripheral.services().flatMap(service =>
  service.characteristics.map(characteristic => ({ service: service.uuid, characteristic: characteristic.uuid })),
);
if (pairs.length === 0) throw new Error('No characteristics found');

const pair = await choose('a service/characteristic pair', pairs, p => `${p.service} ${p.characteristic}`);

const content = await input.question('Enter content to write: ');
input.close();

await peripheral.writeRequest(pair.service, pair.characteristic, Buffer.from(content));

await peripheral.disconnect();
