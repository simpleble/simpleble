# SimpleRsBLE

The ultimate cross-platform library and bindings for Bluetooth Low Energy (BLE), designed for simplicity and ease of use.

## Key Features

* **Cross-Platform**: Enterprise-grade support for Windows, macOS, and Linux
* **Easy Integration**: Clean, consistent API across all platforms
* **Multiple Language Bindings**: Production-ready bindings for C, C++, C#, Python, Java and Rust, with more coming soon
* **Commercial Ready**: Source-available commercial license for proprietary applications

## Support & Resources

We're here to help you succeed with SimpleBLE:

* **Documentation**: Visit our [Documentation](https://docs.simpleble.org) page for comprehensive guides
* **Commercial Support**: Check [our website](https://simpleble.org?utm_source=crates_io&utm_medium=referral&utm_campaign=simplersble_readme) or [email us](mailto:contact@simpleble.org) about licensing and professional services
* **Community**: Join our [Discord](https://discord.gg/N9HqNEcvP3) server for discussions and help
* **Dongl**: Try the [official SimpleBLE Bluetooth dongle](https://www.simpleble.org/dongl?utm_source=crates_io&utm_medium=referral&utm_campaign=dongl_launch) for reliable prototyping and production

**Don't hesitate to reach out if you need assistance - we're happy to help!**

## Installation

Requirements:

- Rust 1.82 or newer
- CMake 3.21 or newer
- On Debian or Ubuntu, DBus development headers: `sudo apt-get install libdbus-1-dev`

Add SimpleRsBLE to your project from crates.io:

```bash
cargo add simplersble
```

Please review our [code examples](https://github.com/simpleble/simpleble/tree/main/examples/simplersble/src/bin) on GitHub for more information on how to use SimpleRsBLE.

## Scan for nearby devices

Add the dependencies:

```toml
[dependencies]
simplersble = "1"
tokio = { version = "1", features = ["full"] }
futures = "0.3"
```

Then scan:

```rust
use futures::stream::StreamExt;

#[tokio::main]
async fn main() {
    let mut adapters = simplersble::Adapter::get_adapters().unwrap();
    if adapters.is_empty() {
        println!("No Bluetooth adapters found.");
        return;
    }

    let adapter = adapters.remove(0);

    let mut events = adapter.on_scan_event();
    tokio::spawn(async move {
        while let Some(Ok(event)) = events.next().await {
            if let simplersble::ScanEvent::Found(peripheral) = event {
                println!(
                    "Found: {} [{}] {} dBm",
                    peripheral.identifier().unwrap(),
                    peripheral.address().unwrap(),
                    peripheral.rssi().unwrap()
                );
            }
        }
    });

    adapter.scan_for(5000).unwrap();

    println!("Scan results:");
    for peripheral in adapter.scan_get_results().unwrap() {
        println!(
            "- {} [{}]",
            peripheral.identifier().unwrap(),
            peripheral.address().unwrap()
        );
    }
}
```

# License

SimpleRsBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md), the Business
Source License 1.1 (BUSL-1.1). It is free for non-commercial use, and commercial use needs a commercial license. We offer
free commercial licenses for small projects and early-stage companies: [email us](mailto:contact@simpleble.org) or
[leave us a message](https://www.simpleble.org/contact?utm_source=crates_io&utm_medium=referral&utm_campaign=simplersble_readme)
to discuss your use case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing) for terms.

---

**SimpleBLE** is a project powered by [**The California Open Source Company**](https://californiaopensource.com?utm_source=crates_io&utm_medium=referral&utm_campaign=simplersble_readme).
