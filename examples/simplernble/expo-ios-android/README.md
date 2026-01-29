# SimpleRNBLE Example (Expo iOS/Android)

## Overview

This example is an **Expo** app that uses **SimpleRNBLE** (the `simplernble` package) to scan, connect, and read data from Bluetooth Low Energy (BLE) peripherals.

## Prerequisites

Make sure your machine is set up for React Native development. Follow the official guide: [Set Up Your Environment](https://reactnative.dev/docs/set-up-your-environment).

Quick checklist:

- Node.js
- Watchman (macOS)
- JDK (for Android builds)
- Android Studio + Android SDK (for Android)
- Xcode + CocoaPods (for iOS)

## Get started

1. Install dependencies

```bash
npm install
```

2. Start the app

Before starting, make sure you've configured **Versions** and **Bluetooth permissions** (see the sections below).

For Android:
```bash
npm run android
```

For iOS:
```bash
npm run ios
```

Start hacking by editing the files inside the `app/` directory.

## Platform requirements

### Versions

Native build versions are configured using the `expo-build-properties` config plugin (it runs during prebuild) in `app.json` under `expo.plugins`.

```bash
npx expo install expo-build-properties
```

This project configures Android + iOS versions like this:

```json
{
  "expo": {
    "plugins": [
      [
        "expo-build-properties",
        {
          "android": {
            "minSdkVersion": 31,
            "targetSdkVersion": 35,
            "compileSdkVersion": 35
          },
          "ios": {
            "deploymentTarget": "15.8"
          }
        }
      ]
    ]
  }
}
```

After changing versions, run `npx expo prebuild` to apply them.

## Bluetooth permissions

This app requires Bluetooth permissions to be configured at the native layer for BLE scanning/connecting to work.

### Configure for Android

Add the following permissions to `android/app/src/main/AndroidManifest.xml`:

```xml
<!-- Required permissions for BLE on Android 12+ (API 31+) -->
<uses-permission android:name="android.permission.BLUETOOTH_SCAN" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
```

### Configure for iOS

Add a Bluetooth usage description to `ios/<YourApp>/Info.plist`:

```xml
<key>NSBluetoothAlwaysUsageDescription</key>
<string>This app needs Bluetooth access to scan and connect to BLE devices.</string>
```

## Troubleshooting
- If you previously denied Bluetooth permissions, re-enable them in the OS settings and relaunch the app.

## Notes

This example uses native code and requires a [development build](https://docs.expo.dev/develop/development-builds/introduction/). It will **not work** with Expo Go.

## Learn more

- [SimpleBLE documentation](https://simpleble.readthedocs.io/en/latest/)
- [SimpleBLE tutorial](https://simpleble.readthedocs.io/en/latest/simpleble/tutorial.html)
- [Expo documentation](https://docs.expo.dev/)
