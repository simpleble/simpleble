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
- iOS/iPadOS 15 or newer, the iOS simulator, and Mac Catalyst 15 or newer (ARM64)
- On Linux, BlueZ and D-Bus; on Ubuntu, install them with `sudo apt-get install bluez libdbus-1-3`

Add SimpleSharpBLE to your project from [NuGet](https://www.nuget.org/packages/SimpleSharpBLE/):

```sh
dotnet add package SimpleSharpBLE --prerelease
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

See the [code examples](https://github.com/simpleble/simpleble/tree/main/examples/simplesharpble)
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

On iOS/iPadOS and Mac Catalyst, add `NSBluetoothAlwaysUsageDescription` to your app's `Info.plist`
and run blocking BLE operations off the UI thread. Sandboxed Catalyst apps also need the
`com.apple.security.device.bluetooth` entitlement. Native libraries link automatically;
`Advanced.IOS` applies to both iOS and Catalyst.

## License

Since January 20th 2025, SimpleBLE is now available under the Business Source License 1.1 (BUSL-1.1). Each
version of SimpleBLE will convert to the GNU General Public License version 3 after four years of its initial release.
Qualifying non-commercial users may instead continue using and distributing that version under the original
BUSL-1.1 terms under the Non-Commercial Perpetual Use Grant in `LICENSE.md`.

The project is free to use for non-commercial purposes, but requires a commercial license for commercial use. We
also offer FREE commercial licenses for small projects and early-stage companies - reach out to discuss your use case!

**Why purchase a commercial license?**

- Build and deploy unlimited commercial applications
- Use across your entire development team
- Zero revenue sharing or royalty payments
- Choose features that match your needs and budget
- Priority technical support included
- Clear terms for integrating into MIT-licensed projects

**Looking for information on pricing and commercial terms of service?** Visit [our website](https://simpleble.org?utm_source=nuget&utm_medium=referral&utm_campaign=simplesharpble_readme) for more details.

For further enquiries, please [email us](mailto:contact@simpleble.org) or [leave us a message on our website](https://www.simpleble.org/contact?utm_source=nuget&utm_medium=referral&utm_campaign=simplesharpble_readme) and we can discuss the specifics of your situation.

---

**SimpleBLE** is a project powered by [**The California Open Source Company**](https://californiaopensource.com?utm_source=nuget&utm_medium=referral&utm_campaign=simplesharpble_readme).
