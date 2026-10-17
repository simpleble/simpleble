const path = require('path');
const { getDefaultConfig, mergeConfig } = require('@react-native/metro-config');

// NOTE: simplernble is a link to the package's sources in this repository. Metro
// watches them, and resolves their imports from this app, so the app and the
// package share one copy of React Native.
const library = path.resolve(__dirname, '../../simplernble');

// With SIMPLERNBLE_TESTS=1 (see simplernble/scripts/test.mjs), the app runs the
// binding tests instead of the example.
const tests = process.env.SIMPLERNBLE_TESTS === '1';

module.exports = mergeConfig(getDefaultConfig(__dirname), {
  watchFolders: [library],
  resolver: {
    nodeModulesPaths: [path.resolve(__dirname, 'node_modules')],
    blockList: [new RegExp(`${library.replace(/[/\\]/g, '[/\\\\]')}[/\\\\]node_modules[/\\\\].*`)],
    resolveRequest: (context, moduleName, platform) => {
      if (tests && moduleName === './App' && context.originModulePath === path.join(__dirname, 'index.js')) {
        return { type: 'sourceFile', filePath: path.join(library, 'test', 'TestApp.tsx') };
      }
      return context.resolveRequest(context, moduleName, platform);
    },
  },
});
