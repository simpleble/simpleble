const fs = require('fs');
const path = require('path');

const packageDir = path.resolve(__dirname, '..');
const repoRoot = path.resolve(packageDir, '../../..');

// Directories to clean before packing (native build artifacts only, not lib/)
const dirsToClean = [
  'android/build',
  'android/.cxx',
  'android/.gradle',
  'ios/build',
];

// Directories/files to copy from repo root
const itemsToCopy = [
  'simpleble',
  'cmake',
  'dependencies',
  'VERSION',
];

console.log('Preparing package for npm publish...');
console.log(`Package dir: ${packageDir}`);
console.log(`Repo root: ${repoRoot}`);

// Clean build artifacts first
console.log('\nCleaning build artifacts...');
dirsToClean.forEach((dir) => {
  const dirPath = path.join(packageDir, dir);
  if (fs.existsSync(dirPath)) {
    fs.rmSync(dirPath, { recursive: true, force: true });
    console.log(`✓ Cleaned: ${dir}`);
  }
});
console.log('');

itemsToCopy.forEach((item) => {
  const src = path.join(repoRoot, item);
  const dest = path.join(packageDir, item);

  if (!fs.existsSync(src)) {
    console.warn(`Warning: ${src} does not exist, skipping...`);
    return;
  }

  // Remove existing copy if it exists
  if (fs.existsSync(dest)) {
    fs.rmSync(dest, { recursive: true, force: true });
  }

  // Copy directory or file
  const stat = fs.statSync(src);
  if (stat.isDirectory()) {
    fs.cpSync(src, dest, { recursive: true });
    console.log(`✓ Copied directory: ${item}`);
  } else {
    fs.copyFileSync(src, dest);
    console.log(`✓ Copied file: ${item}`);
  }
});

console.log('Package preparation complete!');
