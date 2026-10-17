// Runs before `npm pack` and `npm publish`. Android apps build SimpleBLE from
// source and iOS apps link a prebuilt framework, so the package carries both.
import { spawnSync } from 'node:child_process';
import { copyFileSync, cpSync, existsSync, rmSync } from 'node:fs';
import { dirname, join, sep } from 'node:path';
import { fileURLToPath } from 'node:url';

const PACKAGE_DIR = join(dirname(fileURLToPath(import.meta.url)), '..');
const REPOSITORY = join(PACKAGE_DIR, '..');
const NATIVE = join(PACKAGE_DIR, 'native');

function run(command, args) {
  const result = spawnSync(join(PACKAGE_DIR, 'node_modules', '.bin', command), args, { cwd: PACKAGE_DIR, stdio: 'inherit' });
  if (result.status !== 0) process.exit(result.status ?? 1);
}

if (!existsSync(join(PACKAGE_DIR, 'prebuilt', 'release', 'SimpleBLE.xcframework'))) {
  console.error('Missing prebuilt/release/SimpleBLE.xcframework: run scripts/build-ios.sh first');
  process.exit(1);
}

// What an Android build of SimpleBLE reads, without its tests.
const excluded = [join('simpleble', 'test'), join('dependencies', 'internal', 'test')].map(path => join(REPOSITORY, path));
const filter = source => !excluded.some(path => source === path || source.startsWith(path + sep));
rmSync(NATIVE, { recursive: true, force: true });
for (const path of ['VERSION', 'cmake', 'dependencies', 'simpleble', join('simpledroidbridge', 'src', 'main', 'java')]) {
  cpSync(join(REPOSITORY, path), join(NATIVE, path), { recursive: true, filter });
}
copyFileSync(join(REPOSITORY, 'LICENSE.md'), join(PACKAGE_DIR, 'LICENSE.md'));

run('nitrogen', ['--logLevel=warning']);
rmSync(join(PACKAGE_DIR, 'lib'), { recursive: true, force: true });
run('tsc', ['-p', 'tsconfig.json']);
