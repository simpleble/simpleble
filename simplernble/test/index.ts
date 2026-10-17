// The binding tests, in the order they run.
import './adapter.test';
import './gatt.test';
import './local.test';
import './config.test';

export { run, type Result } from './harness';
