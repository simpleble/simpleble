// Runs the binding tests (test/) in the example app against SimpleBLE's PLAIN
// backend, on an Android emulator or an iOS simulator:
//
//   node scripts/test.mjs android    # ANDROID_SERIAL picks the device
//   node scripts/test.mjs ios        # SIMULATOR picks the device, else the booted one
//
// The app reports each result to a server this script runs on port 8099.
import { spawn, spawnSync } from 'node:child_process';
import { once } from 'node:events';
import { createServer } from 'node:http';
import { homedir } from 'node:os';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const PACKAGE_DIR = join(dirname(fileURLToPath(import.meta.url)), '..');
const REPOSITORY = join(PACKAGE_DIR, '..');
const EXAMPLE = join(REPOSITORY, 'examples', 'simplernble');
const APP_ID = 'org.simplernble.example';
const REPORT_PORT = 8099;
const TIMEOUT_MS = 5 * 60_000;

const platform = process.argv[2];
if (platform !== 'android' && platform !== 'ios') {
  console.error('Usage: node scripts/test.mjs android|ios');
  process.exit(2);
}

function run(command, args, options = {}) {
  const result = spawnSync(command, args, { stdio: 'inherit', ...options });
  if (result.status !== 0) throw new Error(`${command} ${args.join(' ')} failed with ${result.status}`);
}

function output(command, args, options = {}) {
  return spawnSync(command, args, { encoding: 'utf8', ...options }).stdout.trim();
}

// Collects the results the app posts.
const results = [];
let finish;
const finished = new Promise(resolve => (finish = resolve));
const server = createServer((request, response) => {
  let body = '';
  request.on('data', chunk => (body += chunk));
  request.on('end', () => {
    response.end();
    const data = JSON.parse(body || '{}');
    if (request.url === '/result') {
      results.push(data);
      console.log(`${data.ok ? '✓' : '✗'} ${data.name} (${data.ms} ms)${data.error ? `\n    ${data.error}` : ''}`);
    } else if (request.url === '/done') {
      finish(data);
    }
  });
});
server.listen(REPORT_PORT);

// A Metro of its own, which serves the tests in place of the example.
const metro = spawn('npx', ['react-native', 'start', '--port', '8081', '--reset-cache'], {
  cwd: EXAMPLE,
  env: { ...process.env, SIMPLERNBLE_TESTS: '1' },
  stdio: ['ignore', 'pipe', 'inherit'],
});
let metroOutput = '';
metro.stdout.on('data', chunk => (metroOutput += chunk));
const metroReady = new Promise((resolve, reject) => {
  metro.stdout.on('data', () => metroOutput.includes('Dev server ready') && resolve());
  metro.on('exit', code => reject(new Error(`Metro exited with ${code}; is port 8081 in use?\n${metroOutput}`)));
});

function android() {
  const sdk = process.env.ANDROID_HOME ?? join(homedir(), 'Library', 'Android', 'sdk');
  const env = { ...process.env, ANDROID_HOME: sdk };
  const adb = (...args) => run(join(sdk, 'platform-tools', 'adb'), args, { env });

  run(join(REPOSITORY, 'utils', 'gradle-react-native', 'gradlew'),
    ['-p', join(EXAMPLE, 'android'), ':app:assembleDebug', '-Psimplernble.plain=true', '--console=plain', '-q'], { env });
  adb('install', '-r', join(EXAMPLE, 'android', 'app', 'build', 'outputs', 'apk', 'debug', 'app-debug.apk'));
  for (const permission of ['BLUETOOTH_SCAN', 'BLUETOOTH_CONNECT']) {
    adb('shell', 'pm', 'grant', APP_ID, `android.permission.${permission}`);
  }
  output(join(sdk, 'platform-tools', 'adb'), ['reverse', 'tcp:8081', 'tcp:8081'], { env });
  output(join(sdk, 'platform-tools', 'adb'), ['reverse', `tcp:${REPORT_PORT}`, `tcp:${REPORT_PORT}`], { env });
  adb('shell', 'am', 'force-stop', APP_ID);
  adb('shell', 'am', 'start', '-n', `${APP_ID}/.MainActivity`);
}

function ios() {
  const booted = Object.values(JSON.parse(output('xcrun', ['simctl', 'list', 'devices', 'booted', '-j'])).devices).flat();
  const device = process.env.SIMULATOR ?? booted[0]?.udid;
  if (!device) throw new Error('No booted simulator; boot one or set SIMULATOR');
  const env = { ...process.env, LANG: 'en_US.UTF-8', SIMPLERNBLE_PLAIN: '1' };
  const iosDir = join(EXAMPLE, 'ios');

  run(join(PACKAGE_DIR, 'scripts', 'build-ios.sh'), [], { env: { ...env, PLAIN: '1' } });
  run('pod', ['install', '--silent'], { cwd: iosDir, env });
  run('xcodebuild', ['-workspace', 'SimpleRnBLEExample.xcworkspace', '-scheme', 'SimpleRnBLEExample',
    '-configuration', 'Debug', '-sdk', 'iphonesimulator', '-destination', `id=${device}`,
    '-derivedDataPath', 'build/DerivedData', '-quiet', 'build'], { cwd: iosDir, env });
  const app = join(iosDir, 'build', 'DerivedData', 'Build', 'Products', 'Debug-iphonesimulator', 'SimpleRnBLEExample.app');
  run('xcrun', ['simctl', 'install', device, app]);
  spawnSync('xcrun', ['simctl', 'terminate', device, APP_ID]);
  run('xcrun', ['simctl', 'launch', device, APP_ID]);
}

let exitCode = 1;
try {
  await metroReady;
  if (platform === 'android') android();
  else ios();

  let timer;
  const timeout = new Promise((_, reject) => (timer = setTimeout(() => reject(new Error('The tests did not finish in time')), TIMEOUT_MS)));
  const summary = await Promise.race([finished, timeout]);
  clearTimeout(timer);
  console.log(`\n${summary.passed} passed, ${summary.failed} failed`);
  exitCode = summary.failed === 0 ? 0 : 1;
} catch (error) {
  console.error(error.message);
} finally {
  server.close();
  metro.kill();
  await once(metro, 'exit').catch(() => {});
}
process.exit(exitCode);
