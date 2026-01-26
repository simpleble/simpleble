===
FAQ
===

Right now, the best way to have your questions answered is on our `Discord`_ server.
More information will be made available here soon.

**What platforms are supported by SimpleBLE?**
SimpleBLE supports Windows 10+, Linux (Ubuntu 20.04+ and other distros using Bluez),
MacOS 13.0+ (Ventura and newer), iOS 15.0+, and Android API 31+.

Please check the overview page on more information about platform-specific limitations.

**When should I set configuration values?**

All configuration values must be set prior to any other interaction with a SimpleBLE component.
This includes calling ``SimpleBLE::Adapter::get_adapters()`` or any other function that
interacts with the underlying Bluetooth stack. Changes made after initialization may not
take effect or could lead to undefined behavior.

**Why do I get UUIDs for the peripheral address on MacOS?**

MacOS has a policy of not allowing applications to access the Bluetooth address of
peripherals. Instead, it provides a UUID that is unique to the peripheral. This
UUID is not persistent across reboots, so you should not use it to identify a
peripheral. Instead, you should use the name of the peripheral, which is
persistent across reboots.

**I get a "Bluetooth not enabled" warning on Windows, despite Bluetooth being enabled.**

This is a known issue when running a version of SimpleBLE built for a 32-bit architecture
on a 64-bit Windows machine. The issue is that the underlying Windows API will not allow
us to query the state of the Bluetooth adapter when running in 32-bit mode. The solution
is to consume a 64-bit version of SimpleBLE instead.

**What is the purpose behind the plain interface?**

Building SimpleBLE with the plain-flavored interface allows you to use a version of the
library that is independent of the operating system, which is achieved by hardcoding
all behaviors and responses. This is useful for testing and debugging, especially when
you don't need to deal with operating-system shenanigans and just want to test the API.

**What is the correct approach to memory management of SimpleBLE objects in the C-API?**

**How are vectors supposed to be handled in the C-API?**

**What is the default behavior of functions that are not supported by a specific backend?**

Successfully calling a function that is not supported by a specific backend will result in
a no-op.

.. _Discord: https://discord.gg/N9HqNEcvP3
