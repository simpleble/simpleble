import { createInterface } from 'node:readline/promises';

import { Backend } from 'simplejsble';

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

const dongl = await Backend.getDongl();
const adapters = await dongl.adapters();
if (adapters.length === 0) throw new Error('No Dongl found');

const adapter = await choose('a Dongl', adapters, describe);
console.log(`Selected Dongl: ${describe(adapter)}`);

adapter.on('scanFound', peripheral => console.log(`Found ${describe(peripheral)}`));

// Scan for 5 seconds
await adapter.scanFor(5000);
adapter.removeAllListeners();

const peripherals = adapter.scanGetResults();
if (peripherals.length === 0) throw new Error('No peripherals found');
const peripheral = await choose('a peripheral', peripherals, describe);

// Pairing can start as soon as the link is up, so set the handlers before
// connecting. The Dongl pairs without the operating system.
await peripheral.setPasskeyRequestHandler(async () => {
  const passkey = await input.question('Enter the passkey shown on the device (empty to reject): ');
  return passkey || undefined;
});
await peripheral.setPasskeyDisplayHandler(passkey => console.log(`Enter ${passkey} on the device.`));
await peripheral.setNumericComparisonHandler(
  async passkey => (await input.question(`Does the device show ${passkey}? [y/N] `)) === 'y',
);

console.log(`Connecting to: ${describe(peripheral)}`);
await peripheral.connect();

console.log('Successfully connected, listing services...');
for (const service of peripheral.services()) {
  console.log(`Service: ${service.uuid}`);
  for (const characteristic of service.characteristics) {
    console.log(`    Characteristic: ${characteristic.uuid}`);
  }
}
console.log(`Paired: ${await peripheral.isPaired()}`);
input.close();

await peripheral.disconnect();
await adapter.close();
