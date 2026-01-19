const { withAndroidManifest } = require('@expo/config-plugins');

// This plugin ensures Expo autolinking detects the package
// The actual native module registration is handled by react-native.config.js
module.exports = function withSimplernble(config) {
  return config;
};
