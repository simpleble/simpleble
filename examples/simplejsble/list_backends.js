import { Backend } from 'simplejsble';

for (const backend of await Backend.getBackends()) {
  console.log(`Backend: ${backend.identifier()}`);
  for (const adapter of await backend.adapters()) {
    console.log(`    Adapter: ${adapter.identifier()} [${adapter.address()}]`);
  }
}
