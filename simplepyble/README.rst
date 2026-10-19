|PyPI Licence|

SimplePyBLE
===========

The ultimate cross-platform library and bindings for Bluetooth Low Energy (BLE), designed for simplicity and ease of use.

Key Features
------------

* **Cross-Platform**: Enterprise-grade support for Windows, macOS, Linux
* **Easy Integration**: Clean, consistent API across all platforms
* **Multiple Language Bindings**: Production-ready bindings for C, C++, C#, Python, Java and Rust, with more coming soon
* **Commercial Ready**: Source-available commercial license for proprietary applications

Support & Resources
--------------------

We're here to help you succeed with SimpleBLE:

* **Documentation**: Visit our `Documentation`_ page for comprehensive guides
* **Commercial Support**: Check out |website|_ or |email|_ about licensing and professional services.
* **Community**: Join our `Discord`_ server for discussions and help
* **Dongl**: Try the `official SimpleBLE Bluetooth dongle`_ for reliable prototyping and production.

**Don't hesitate to reach out if you need assistance - we're happy to help!**

Installation
------------

You can install SimplePyBLE from PyPI using pip: ::

   pip install simplepyble

When building from source on Debian or Ubuntu, install the DBus development headers first: ::

   sudo apt-get install libdbus-1-dev

Usage
-----

Scan for nearby devices: ::

    import simplepyble

    adapters = simplepyble.Adapter.get_adapters()
    if not adapters:
        raise SystemExit("No Bluetooth adapters found.")

    adapter = adapters[0]
    print(f"Using adapter: {adapter.identifier()} [{adapter.address()}]")

    adapter.set_callback_on_scan_found(
        lambda peripheral: print(
            f"Found: {peripheral.identifier()} [{peripheral.address()}] {peripheral.rssi()} dBm"
        )
    )

    adapter.scan_for(5000)

    print("Scan results:")
    for peripheral in adapter.scan_get_results():
        state = "connectable" if peripheral.is_connectable() else "not connectable"
        print(f"- {peripheral.identifier()} [{peripheral.address()}] {state}")

See the `code examples`_ on GitHub for connect, read/write, notify, local peripheral hosting, and Dongl pairing flows.

Asynchronous Support
--------------------

SimplePyBLE provides an asynchronous API via the ``simplepyble.aio`` module. This module
is designed to work with ``asyncio`` and provides a more idiomatic way to handle
asynchronous operations in Python.

Example: ::

   import asyncio
   from simplepyble.aio import Adapter

   async def main():
       adapters = Adapter.get_adapters()
       adapter = adapters[0]

       async with adapter:
           await adapter.scan_for(5000)
           peripherals = adapter.scan_get_results()
           for peripheral in peripherals:
               print(f"Found: {peripheral.identifier()} [{peripheral.address()}]")

   if __name__ == "__main__":
       asyncio.run(main())

Check out the `async examples`_ for more details.

License
=======

SimplePyBLE follows the `SimpleBLE license`_, the Business Source License 1.1 (BUSL-1.1). It is free for non-commercial
use, and commercial use needs a commercial license. We offer free commercial licenses for small projects and early-stage
companies: |email|_ or |leavemessage|_ to discuss your use case. Commercial licenses carry no royalties or revenue
sharing; see the `licensing page`_ for terms.

----

**SimpleBLE** is a project powered by |caos|_.

.. Links

.. |email| replace:: email us
.. _email: mailto:contact@simpleble.org

.. |leavemessage| replace:: leave us a message
.. _leavemessage: https://www.simpleble.org/contact?utm_source=pypi_simplepyble&utm_medium=referral&utm_campaign=simplepyble_readme

.. |website| replace:: our website
.. _website: https://simpleble.org?utm_source=pypi_simplepyble&utm_medium=referral&utm_campaign=simplepyble_readme

.. |caos| replace:: **The California Open Source Company**
.. _caos: https://californiaopensource.com?utm_source=pypi_simplepyble&utm_medium=referral&utm_campaign=simplepyble_readme

.. _SimplePyBLE: https://pypi.org/project/simplepyble/

.. _SimpleBLE: https://github.com/simpleble/simpleble/

.. _SimpleBLE license: https://github.com/simpleble/simpleble/blob/main/LICENSE.md

.. _licensing page: https://docs.simpleble.org/licensing

.. _official SimpleBLE Bluetooth dongle: https://www.simpleble.org/dongl?utm_source=pypi_simplepyble&utm_medium=referral&utm_campaign=dongl_launch

.. _code examples: https://github.com/simpleble/simpleble/tree/main/examples/simplepyble

.. _async examples: https://github.com/simpleble/simpleble/tree/main/examples/simplepyble

.. _Discord: https://discord.gg/N9HqNEcvP3

.. _Documentation: https://docs.simpleble.org

.. |PyPI Licence| image:: https://img.shields.io/pypi/l/simplepyble
