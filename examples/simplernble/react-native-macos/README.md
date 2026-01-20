# SimpleRNBLE Example (React Native macOS)

## Overview

This example is a **React Native macOS** app that uses **SimpleRNBLE** (the `simplernble` package) to scan, connect, and read data from Bluetooth Low Energy (BLE) peripherals.

## Get started

1. Install dependencies

```bash
npm install
```

2. Install CocoaPods dependencies (first clone, or after native dependency changes)

Before starting, make sure you've configured **Versions** (see the section below).

```bash
cd macos
pod install
cd ..
```

```bash
cd ios
pod install
cd ..
```

3. Start Metro

```bash
npm start
```

4. Build and run

Before starting, make sure you've configured **Bluetooth permissions** (see the section below).

```bash
npm run macos
```

```bash
npm run ios
```

```bash
npm run android
```

Start hacking by editing `src/app.tsx`.

## Platform requirements

### macOS

This example requires **macOS 13.0+** (see `MACOSX_DEPLOYMENT_TARGET` in the Xcode project build settings).

## Bluetooth permissions

This app requires Bluetooth permissions to be configured for BLE scanning/connecting to work.

### macOS usage description (required)

Add `NSBluetoothAlwaysUsageDescription` to `macos/reactNativeMacos-macOS/Info.plist`:

Open `macos/reactNativeMacos-macOS/Info.plist` in a text editor and add the following entry:

```xml
<key>NSBluetoothAlwaysUsageDescription</key>
<string>This app needs Bluetooth access to scan and connect to BLE devices.</string>
```

### macOS sandbox entitlement (only if App Sandbox is enabled)

If you enable **App Sandbox** in Xcode, also enable **Bluetooth** hardware access (Signing & Capabilities → App Sandbox → Bluetooth).

Concretely, this corresponds to the entitlement key:

- `com.apple.security.device.bluetooth` = `true`

In this repo, entitlements live at:

- `macos/reactNativeMacos-macOS/reactNativeMacos.entitlements`

Example:

```xml
<key>com.apple.security.device.bluetooth</key>
<true/>
```

Without this entitlement, Bluetooth access may fail even if the usage description is present.

## Troubleshooting

- If you previously denied Bluetooth permissions, re-enable them in the OS settings and relaunch the app.
- If App Sandbox is enabled and Bluetooth doesn’t work, confirm the Bluetooth entitlement (`com.apple.security.device.bluetooth`) is enabled for the app target.

## Learn more

- [SimpleBLE documentation](https://simpleble.readthedocs.io/en/latest/)
- [SimpleBLE tutorial](https://simpleble.readthedocs.io/en/latest/simpleble/tutorial.html)
- [React Native macOS: Get Started](https://microsoft.github.io/react-native-macos/docs/getting-started)
- [React Native macOS docs](https://microsoft.github.io/react-native-macos/)
