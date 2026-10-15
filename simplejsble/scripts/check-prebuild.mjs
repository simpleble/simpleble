// Checks properties of prebuilds/<platform>-<arch>/simplejsble.node that a
// build can lose without failing: the glibc baseline on Linux, the deployment
// target on macOS, and on Windows the static C++ runtime and a delay-loaded
// node.exe, without which the addon can't load in Electron.
import { execFileSync } from 'node:child_process';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const addon = join(root, 'prebuilds', `${process.platform}-${process.arch}`, 'simplejsble.node');

const MAX_GLIBC = '2.28';
const MAX_GLIBCXX = '3.4.25';
const MAX_MACOS = '13.0';

function compare(a, b) {
  const x = a.split('.').map(Number);
  const y = b.split('.').map(Number);
  for (let i = 0; i < Math.max(x.length, y.length); i++) {
    if ((x[i] ?? 0) !== (y[i] ?? 0)) return (x[i] ?? 0) - (y[i] ?? 0);
  }
  return 0;
}

function highest(text, prefix) {
  const versions = [...text.matchAll(new RegExp(`${prefix}_([0-9.]+)`, 'g'))].map(match => match[1]);
  return versions.sort(compare).at(-1);
}

function fail(message) {
  console.error(`${addon}: ${message}`);
  process.exit(1);
}

if (process.platform === 'linux') {
  const symbols = execFileSync('objdump', ['-T', addon], { encoding: 'utf8' });
  const glibc = highest(symbols, 'GLIBC');
  const glibcxx = highest(symbols, 'GLIBCXX');
  if (glibc && compare(glibc, MAX_GLIBC) > 0) fail(`needs glibc ${glibc}, more than ${MAX_GLIBC}`);
  if (glibcxx && compare(glibcxx, MAX_GLIBCXX) > 0) fail(`needs libstdc++ ${glibcxx}, more than ${MAX_GLIBCXX}`);
  console.log(`glibc ${glibc ?? 'none'}, libstdc++ ${glibcxx ?? 'none'}`);
} else if (process.platform === 'darwin') {
  const commands = execFileSync('otool', ['-l', addon], { encoding: 'utf8' });
  const minimum = commands.match(/minos ([0-9.]+)/)?.[1] ?? commands.match(/LC_VERSION_MIN_MACOSX[\s\S]*?version ([0-9.]+)/)?.[1];
  if (!minimum) fail('has no minimum macOS version');
  if (compare(minimum, MAX_MACOS) > 0) fail(`needs macOS ${minimum}, more than ${MAX_MACOS}`);
  console.log(`macOS ${minimum}`);
} else if (process.platform === 'win32') {
  const vswhere = join(process.env['ProgramFiles(x86)'], 'Microsoft Visual Studio', 'Installer', 'vswhere.exe');
  const dumpbin = execFileSync(vswhere, ['-latest', '-products', '*', '-find', '**/dumpbin.exe'], { encoding: 'utf8' })
    .split(/\r?\n/)
    .filter(Boolean)[0];
  if (!dumpbin) fail('dumpbin.exe not found');

  const output = execFileSync(dumpbin, ['/nologo', '/dependents', addon], { encoding: 'utf8' });
  const [direct, delayed = ''] = output.split(/Image has the following delay load dependencies:/i);
  if (/vcruntime|msvcp|ucrtbase|api-ms-win-crt/i.test(output)) fail('depends on the Visual C++ runtime');
  if (/node\.exe/i.test(direct)) fail('imports node.exe directly instead of delay-loading it');
  if (!/node\.exe/i.test(delayed)) fail('does not delay-load node.exe');
  console.log('static C++ runtime, node.exe delay-loaded');
}
