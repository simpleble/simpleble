// Builds the addon for this machine and copies it to prebuilds/<platform>-<arch>.
// `--plain` builds against SimpleBLE's PLAIN backend, for tests only.
import { spawnSync } from 'node:child_process';
import { copyFileSync, mkdirSync } from 'node:fs';
import { createRequire } from 'node:module';
import { availableParallelism } from 'node:os';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const require = createRequire(import.meta.url);

const plain = process.argv.includes('--plain');
const extra = process.argv.slice(2).filter(arg => arg !== '--plain');

function run(command, args) {
  const result = spawnSync(command, args, { cwd: root, stdio: 'inherit' });
  if (result.status !== 0) process.exit(result.status ?? 1);
}

const cmakeJs = require.resolve('cmake-js/bin/cmake-js');
const options = ['--arch', process.arch, `--CDSIMPLEBLE_PLAIN=${plain ? 'ON' : 'OFF'}`, ...extra];
run(process.execPath, [cmakeJs, 'configure', ...options]);
run(process.execPath, [cmakeJs, 'build', ...options, '--parallel', String(availableParallelism())]);

const addon = join(root, 'build', 'Release', 'simplejsble.node');

// NOTE: The addon links with Node's symbols left undefined, so a missing
// definition of our own only shows up at runtime. Catch it here instead.
if (process.platform !== 'win32') {
  const symbols = spawnSync('nm', ['-u', addon], { encoding: 'utf8' }).stdout ?? '';
  const missing = symbols.split('\n').filter(symbol => symbol.includes('SimpleJSBLE'));
  if (missing.length > 0) {
    console.error(`Undefined addon symbols:\n${missing.join('\n')}`);
    process.exit(1);
  }
}

const destination = join(root, 'prebuilds', `${process.platform}-${process.arch}`);
mkdirSync(destination, { recursive: true });
copyFileSync(addon, join(destination, 'simplejsble.node'));

run(process.execPath, [require.resolve('typescript/bin/tsc'), '-p', 'tsconfig.json']);
