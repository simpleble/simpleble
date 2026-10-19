# SimpleSharpBLE

The ultimate cross-platform library and bindings for Bluetooth Low Energy (BLE), designed for simplicity and ease of use.

SimpleSharpBLE provides the C# and .NET bindings for SimpleBLE.

## Key Features

* **Cross-Platform**: Enterprise-grade support for Windows, macOS, Linux, Android, iOS/iPadOS, and Mac Catalyst
* **Easy Integration**: Clean, consistent API across all platforms
* **Multiple Language Bindings**: Production-ready bindings for C, C++, C#, Python, Java and Rust, with more coming soon
* **Commercial Ready**: Source-available commercial license for proprietary applications

## Support & Resources

We're here to help you succeed with SimpleBLE:

* **Documentation**: Visit our [Documentation](https://docs.simpleble.org) page for comprehensive guides
* **Commercial Support**: Check [our website](https://simpleble.org?utm_source=nuget&utm_medium=referral&utm_campaign=simplesharpble_readme) or [email us](mailto:contact@simpleble.org) about licensing and professional services
* **Community**: Join our [Discord](https://discord.gg/N9HqNEcvP3) server for discussions and help
* **Dongl**: Try the [official SimpleBLE Bluetooth dongle](https://www.simpleble.org/dongl?utm_source=nuget&utm_medium=referral&utm_campaign=dongl_launch) for reliable prototyping and production

**Don't hesitate to reach out if you need assistance - we're happy to help!**

## Installation

Requirements:

- .NET 10
- Windows or glibc-based Linux (x64/ARM64), or macOS 14 or newer on Apple Silicon
- Android 12 or newer (ARM64/x64)
- iOS/iPadOS 15.8 or newer, the iOS simulator, and Mac Catalyst 16.0 or newer (ARM64)
- On Linux, BlueZ and D-Bus; on Ubuntu, install them with `sudo apt-get install bluez libdbus-1-3`

Add SimpleSharpBLE to your project from [NuGet](https://www.nuget.org/packages/SimpleSharpBLE/):

```sh
dotnet add package SimpleSharpBLE
```

## Usage

Scan for nearby devices:

```csharp
using SimpleSharpBLE;

var adapters = Adapter.GetAdapters();
try
{
    if (adapters.Count == 0)
    {
        Console.WriteLine("No Bluetooth adapters found.");
        return;
    }

    var adapter = adapters[0];
    Console.WriteLine($"Using adapter: {adapter.Identifier} [{adapter.Address}]");
    adapter.ScanFor(TimeSpan.FromSeconds(5));

    var peripherals = adapter.ScanGetResults();
    try
    {
        Console.WriteLine("Scan results:");
        foreach (var peripheral in peripherals)
        {
            Console.WriteLine($"- {peripheral.Identifier} [{peripheral.Address}] {peripheral.Rssi} dBm");
        }
    }
    finally
    {
        foreach (var peripheral in peripherals) peripheral.Dispose();
    }
}
finally
{
    foreach (var adapter in adapters) adapter.Dispose();
}
```

See the [SimpleSharpBLE documentation](https://docs.simpleble.org/simplesharpble/usage) for the
full API guide and platform setup, and the [code examples](https://github.com/simpleble/simpleble/tree/main/examples/simplesharpble)
on GitHub for connect, read/write, notify, and peripheral hosting flows.

Blocking GATT calls also have `Async` counterparts. Cancellation prevents queued work
from starting; operations already in native code complete normally. Keep write buffers
unchanged until the returned task completes.

The bindings support trimming and NativeAOT. In MAUI, use the same API and platform
setup below; dispatch UI updates from callbacks with `MainThread.BeginInvokeOnMainThread`.

On Android, enable Bluetooth and grant Nearby devices permissions, then initialize on the application thread before using the API:

```csharp
Java.Lang.JavaSystem.LoadLibrary("simpleble");
Advanced.Android.Initialize(Java.Interop.JniEnvironment.Runtime.InvocationPointer,
    Android.App.Application.Context.Handle);
```

See the [Android scan example](https://github.com/simpleble/simpleble/tree/main/examples/simplesharpble-android)
for the manifest, runtime permissions, and scanning off the UI thread.

On iOS/iPadOS and Mac Catalyst, set `SupportedOSPlatformVersion` to at least 15.8 (iOS) or 16.0
(Mac Catalyst), add `NSBluetoothAlwaysUsageDescription` to your app's `Info.plist`, and run blocking
BLE operations off the UI thread. Sandboxed Catalyst apps also need the
`com.apple.security.device.bluetooth` entitlement. Native libraries link automatically;
`Advanced.IOS` applies to both iOS and Catalyst.

## License

SimpleSharpBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md), the Business
Source License 1.1 (BUSL-1.1). It is free for non-commercial use, and commercial use needs a commercial license. We offer
free commercial licenses for small projects and early-stage companies: [email us](mailto:contact@simpleble.org) or
[leave us a message](https://www.simpleble.org/contact?utm_source=nuget&utm_medium=referral&utm_campaign=simplesharpble_readme)
to discuss your use case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing) for terms.

---

**SimpleBLE** is a project powered by [**The California Open Source Company**](https://californiaopensource.com?utm_source=nuget&utm_medium=referral&utm_campaign=simplesharpble_readme).
