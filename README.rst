.. image:: docs/public/simpleBLE-logo-dark.png#gh-light-mode-only
   :alt: SimpleBLE
   :align: center
   :width: 360px

.. image:: docs/public/simpleBLE-logo.png#gh-dark-mode-only
   :alt: SimpleBLE
   :align: center
   :width: 360px

SimpleBLE
=========

**Build Bluetooth once. Ship it everywhere.**

|CI Main| |Latest Release| |License|

SimpleBLE is the cross-platform Bluetooth Low Energy stack for Windows, macOS, Linux, iOS, and Android. Use it from
C++, C, Python, Java, or Rust, with dedicated APIs for Kotlin on Android and Unreal Engine.

Why SimpleBLE
-------------

* **One cross-platform API:** Keep application-facing BLE logic consistent across desktop and mobile.
* **Native underneath:** Use WinRT, CoreBluetooth, BlueZ, and Android Bluetooth through one maintained stack.
* **Built for real products:** Central-role support across all five operating systems, with Linux/C++ peripheral support.
* **Commercially supported:** Ship proprietary applications with licensing and direct technical support available.

Get Started
-----------

Choose the API that fits your project in the `Installation`_ guide, then run the `Quickstart`_. For tutorials, recipes,
platform notes, and API references, visit the `Documentation`_.

Dongl: Official SimpleBLE Hardware
----------------------------------

`Dongl`_ is the official SimpleBLE hardware companion. Its N-Series firmware is available for nRF52840-based boards,
with pre-loaded production hardware coming soon.

Use Dongl when you want firmware visibility, customizability, and direct support from the team building SimpleBLE.

* Flash the Dongl N-Series firmware onto existing nRF52840 hardware for prototyping.
* Join the waitlist for official pre-loaded Dongl hardware.

Documentation and Support
-------------------------

* Read the `Documentation`_ and `Announcements`_.
* Join the `Discord`_ community.
* Review the `security policy`_ or learn how to `contribute`_.
* Explore the lower-level `SimpleBluez`_ and `SimpleDBus`_ libraries.
* Visit |website|_ or |email|_ about commercial licensing and professional services.
* Browse the `SimpleBLE Store`_.

License
-------

SimpleBLE is available under the Business Source License 1.1 (BUSL-1.1). It is free to use for non-commercial purposes
and requires a commercial license for commercial use. Each version converts to the GNU General Public License version 3
four years after its initial release. Qualifying non-commercial users may instead continue using and distributing that
version under the original BUSL-1.1 terms through the Non-Commercial Perpetual Use Grant in ``LICENSE.md``.

Free commercial licenses are available for qualifying small projects and early-stage companies. See |website-url|_ for
pricing and commercial terms, or |leavemessage|_ to discuss your use case.

----

**SimpleBLE** is a project powered by |caos|_.

.. Badges

.. |CI Main| image:: https://github.com/simpleble/simpleble/actions/workflows/ci_main.yml/badge.svg?branch=main
   :alt: CI Main
   :target: https://github.com/simpleble/simpleble/actions/workflows/ci_main.yml?query=branch%3Amain

.. |Latest Release| image:: https://img.shields.io/github/v/release/simpleble/simpleble?sort=semver&display_name=tag
   :alt: Latest release
   :target: https://github.com/simpleble/simpleble/releases/latest

.. |License| image:: https://img.shields.io/badge/license-BUSL--1.1-blue
   :alt: BUSL 1.1 license
   :target: https://github.com/simpleble/simpleble/blob/main/LICENSE.md

.. Links

.. |email| replace:: email us
.. _email: mailto:contact@simpleble.org

.. |leavemessage| replace:: leave us a message
.. _leavemessage: https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simpleble_readme

.. |website| replace:: our website
.. _website: https://simpleble.org?utm_source=github&utm_medium=referral&utm_campaign=simpleble_readme

.. |website-url| replace:: www.simpleble.org
.. _website-url: https://simpleble.org?utm_source=github&utm_medium=referral&utm_campaign=simpleble_readme

.. |caos| replace:: **The California Open Source Company**
.. _caos: https://californiaopensource.com?utm_source=github&utm_medium=referral&utm_campaign=simpleble_readme

.. _Announcements: https://simpleble.org/blog/news?utm_source=github&utm_medium=referral&utm_campaign=simpleble_readme
.. _Documentation: https://docs.simpleble.org/
.. _Discord: https://discord.gg/N9HqNEcvP3
.. _Dongl: https://www.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=dongl_launch
.. _SimpleBLE Store: https://simpleble.store?utm_source=github&utm_medium=referral&utm_campaign=simpleble_readme
.. _SimpleBluez: https://github.com/simpleble/simpleble/tree/main/simplebluez
.. _SimpleDBus: https://github.com/simpleble/simpleble/tree/main/simpledbus
.. _security policy: https://github.com/simpleble/simpleble/blob/main/SECURITY.md
.. _contribute: https://github.com/simpleble/simpleble/blob/main/CONTRIBUTING.md
.. _Installation: https://docs.simpleble.org/installation
.. _Quickstart: https://docs.simpleble.org/quickstart
