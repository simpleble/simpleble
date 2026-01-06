const path = require('path');
const { getDefaultConfig } = require('expo/metro-config');

const root = path.resolve(__dirname, '..');
const pkg = path.join(root, 'packages/simplejsble');

const config = getDefaultConfig(__dirname);

config.watchFolders = [root];

config.resolver = {
    ...config.resolver,
    nodeModulesPaths: [
        path.resolve(__dirname, 'node_modules'),
        path.resolve(root, 'node_modules')
    ]
};

module.exports = config;