const path = require('path');
const { getDefaultConfig } = require('expo/metro-config');


const projectRoot = __dirname;
const workspaceRoot = path.resolve(projectRoot, '../..');
const config = getDefaultConfig(projectRoot);


config.watchFolders = [workspaceRoot];


config.resolver = {
    ...config.resolver,
    nodeModulesPaths: [
        path.resolve(workspaceRoot, 'node_modules'),
        path.resolve(projectRoot, 'node_modules'),
    ],
    extraNodeModules: {
        ...config.resolver.extraNodeModules,
        'simplejsble': path.resolve(workspaceRoot),
    }
}

module.exports = config;    