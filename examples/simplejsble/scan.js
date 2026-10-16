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
input.close();
console.log(`Selected adapter: ${describe(adapter)}`);

adapter.on('scanStart', () => console.log('Scan started.'));
adapter.on('scanStop', () => console.log('Scan complete.'));
adapter.on('scanFound', peripheral => console.log(`Found ${describe(peripheral)}`));

// Scan for 5 seconds
await adapter.scanFor(5000);

console.log('The following peripherals were found:');
for (const peripheral of adapter.scanGetResults()) {
  const connectable = peripheral.isConnectable() ? 'Connectable' : 'Non-Connectable';
  console.log(`${describe(peripheral)} - ${connectable}`);
  console.log(`    Address Type: ${peripheral.addressType()}`);
  console.log(`    Tx Power: ${peripheral.txPower() ?? 'unknown'} dBm`);

  for (const [companyId, data] of peripheral.manufacturerData()) {
    console.log(`    Manufacturer ID: ${companyId}`);
    console.log(`    Manufacturer data: ${data.toString('hex')}`);
  }

  for (const service of peripheral.services()) {
    console.log(`    Service UUID: ${service.uuid}`);
    console.log(`    Service data: ${service.data.toString('hex')}`);
  }
}
