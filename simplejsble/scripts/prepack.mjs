// Runs before `npm pack` and `npm publish`: the package carries the addon for
// every supported platform, so it refuses to pack without all of them.
import { spawnSync } from 'node:child_process';
import { copyFileSync, existsSync, rmSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const require = createRequire(import.meta.url);

const TARGETS = ['darwin-arm64', 'darwin-x64', 'linux-arm64', 'linux-x64', 'win32-arm64', 'win32-x64'];
const missing = TARGETS.filter(target => !existsSync(join(root, 'prebuilds', target, 'simplejsble.node')));
if (missing.length > 0) {
  console.error(`Missing prebuilt addons: ${missing.join(', ')}`);
  process.exit(1);
}

copyFileSync(join(root, '..', 'LICENSE.md'), join(root, 'LICENSE.md'));

rmSync(join(root, 'dist'), { recursive: true, force: true });
const tsc = spawnSync(process.execPath, [require.resolve('typescript/bin/tsc'), '-p', 'tsconfig.json'], {
  cwd: root,
  stdio: 'inherit',
});
process.exit(tsc.status ?? 1);
