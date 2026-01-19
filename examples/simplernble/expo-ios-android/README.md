# SimpleRNBLE Example (Expo iOS/Android)

## Overview

This example is an **Expo** app that uses **SimpleRNBLE** (the `simplernble` package) to scan, connect, and read data from Bluetooth Low Energy (BLE) peripherals.

## Get started

1. Install dependencies

```bash
npm install
```

2. Start the app

For Android:
```bash
npm run android
```

For iOS:
```bash
npm run ios
```

**Note:** This example uses native code and requires a [development build](https://docs.expo.dev/develop/development-builds/introduction/). It will **not work** with Expo Go.

Start hacking by editing the files inside the `app/` directory.

## Platform requirements

### Android (SDK levels)

Android SDK versions are configured in `app.json` under `expo.android`:

- `minSdkVersion`: minimum Android SDK version supported
- `targetSdkVersion`: target Android SDK version
- `compileSdkVersion`: Android SDK version used for compilation

These are required and must be configured in `app.json`:

```json
{
  "expo": {
    "android": {
      "minSdkVersion": 31,
      "targetSdkVersion": 35,
      "compileSdkVersion": 35
    }
  }
}
```

### iOS (deployment target)

The minimum iOS deployment target is required and must be configured in `app.json` under `expo.ios.deploymentTarget`:

```json
{
  "expo": {
    "ios": {
      "deploymentTarget": "15.8"
    }
  }
}
```

## Bluetooth permissions

This app requires Bluetooth permissions to be configured in `app.json` for BLE scanning/connecting to work.

### Android (Android 12+ / API 31+)

Add the following permissions under `expo.android.permissions` in `app.json`:

```json
{
  "expo": {
    "android": {
      "permissions": [
        "BLUETOOTH_SCAN",
        "BLUETOOTH_CONNECT"
      ]
    }
  }
}
```

### iOS

Add a Bluetooth usage description under `expo.ios.infoPlist` in `app.json`:

```json
{
  "expo": {
    "ios": {
      "infoPlist": {
        "NSBluetoothAlwaysUsageDescription": "This app needs Bluetooth access to scan and connect to BLE devices."
      }
    }
  }
}
```

## Troubleshooting

- If scanning/connecting fails on Android, confirm **Nearby devices** permissions are granted in system settings (Android can deny/block Bluetooth runtime permissions even if the manifest entries exist).
- If you previously denied Bluetooth permissions, re-enable them in the OS settings and relaunch the app.

## Learn more

- [SimpleBLE documentation](https://simpleble.readthedocs.io/en/latest/)
- [SimpleBLE tutorial](https://simpleble.readthedocs.io/en/latest/simpleble/tutorial.html)
- [Expo documentation](https://docs.expo.dev/)
