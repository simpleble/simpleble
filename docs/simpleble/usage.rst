=====
Usage
=====

Please follow the instructions below to build and run SimpleBLE in your specific environment.

System Requirements
===================

To build SimpleBLE from source, ensure your system meets the following requirements, which vary
by operating system. These dependencies and version constraints ensure compatibility and optimal performance.

General Requirements
--------------------

To build SimpleBLE, you need:

- `CMake`_ (Version 3.21 or higher). Refer to our `CMake Primer`_ for setup guidance.

.. _CMake: https://cmake.org
.. _CMake Primer: ../cmake_primer.html

Linux
-----

SimpleBLE is designed to work on Linux distributions using BlueZ as the Bluetooth stack.

**Supported Distributions**

- Primary: Ubuntu 20.04 and newer
- Other major distributions using BlueZ may work but are not officially supported.

**Dependencies**

- **APT-based Distributions** (e.g., Ubuntu):

  Install `libdbus-1-dev` using:

  .. code-block:: bash

     sudo apt install libdbus-1-dev

- **RPM-based Distributions** (e.g., Fedora, CentOS):

  Install `dbus-devel` using:

  .. code-block:: bash

     # On Fedora
     sudo dnf install dbus-devel

     # On CentOS
     sudo yum install dbus-devel

**Notes**

- BlueZ compatibility should ensure broad support, but Ubuntu is the primary tested platform.

Windows
-------

**Supported Versions**

- Windows 10 and newer

**Dependencies**

- `Windows SDK`_ (Version 10.0.19041.0 or higher)

.. _Windows SDK: https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/

**Notes**

- Only a single Bluetooth adapter is supported by the OS backend.
- WSL does not support Bluetooth.

MacOS
-----

**Supported Versions**

- macOS 13.0 (Ventura) and newer

**Dependencies**

- `Xcode Command Line Tools`_ (install via ``xcode-select --install``)

.. _Xcode Command Line Tools: https://developer.apple.com/xcode/resources/

**Exceptions**

- macOS 12.0, 12.1, and 12.2 have a known bug where the adapter fails to return peripherals after scanning.

**Notes**

- Only a single Bluetooth adapter is supported by the OS backend.

Android
-------

**Supported Versions**

- API 31 and newer

**Dependencies**

- `Android Studio`_
- `Android NDK`_ (Version 25 or higher; older versions may work but are untested)

.. _Android Studio: https://developer.android.com/studio
.. _Android NDK: https://developer.android.com/ndk

**Notes**

- Older APIs lack certain JVM API features required by SimpleBLE.
- Removing bonds is not supported due to limitations in the public API; non-public API workarounds are needed.
- Address type is unavailable, as it requires API 35 or newer.

iOS
---

**Supported Versions**

- iOS 15.8 and newer

**Notes**

- Older iOS versions may work but lack formal testing.


Building and Installing SimpleBLE (Source)
============================================

Compiling the library is done using `CMake`_ and relies heavily on plenty of CMake
functionality. It is strongly suggested that you get familiarized with CMake before
blindly following the instructions below.


Building SimpleBLE
------------------

You can use the following commands to build SimpleBLE: ::

   cmake -S <path-to-simpleble> -B build_simpleble
   cmake --build build_simpleble -j7

Note that if you want to modify the build configuration, you can do so by passing
additional arguments to the ``cmake`` command. For example, to build a shared library
set the ``BUILD_SHARED_LIBS`` CMake variable to ``TRUE`` ::

   cmake -S <path-to-simpleble> -B build_simpleble -DBUILD_SHARED_LIBS=TRUE

To build a plain-flavored version of the library, set the ``SIMPLEBLE_PLAIN`` CMake
variable to ``TRUE`` ::

   cmake -S <path-to-simpleble> -B build_simpleble -DSIMPLEBLE_PLAIN=TRUE

To modify the log level, set the ``SIMPLEBLE_LOG_LEVEL`` CMake variable to one of the
following values: ``VERBOSE``, ``DEBUG``, ``INFO``, ``WARN``, ``ERROR``, ``FATAL`` ::

   cmake -S <path-to-simpleble> -B build_simpleble -DSIMPLEBLE_LOG_LEVEL=DEBUG

**(Linux only)** To force the usage of the DBus session bus, enable the ``SIMPLEBLE_USE_SESSION_DBUS`` flag ::

   cmake -S <path-to-simplebluez> -B build_simplebluez -DSIMPLEBLE_USE_SESSION_DBUS=TRUE

Installing SimpleBLE
--------------------

To install SimpleBLE, you can use the following commands: ::

   cmake --install build_simpleble

Note that if you want to modify the installation configuration, you can do so by passing
additional arguments to the ``cmake`` command. For example, to install the library to
a specific location, set the ``CMAKE_INSTALL_PREFIX`` CMake variable to the desired
location ::

   cmake --install build_simpleble --prefix /usr/local

Note that on Linux and MacOS, you will need to run the ``cmake --install`` command
with ``sudo`` privileges. ::

   sudo cmake --install build_simpleble


Usage with CMake (Installed)
============================

Once SimpleBLE has been installed, it can be consumed from within CMake::

   find_package(simpleble REQUIRED CONFIG)
   target_link_libraries(<your-target> simpleble::simpleble)

Note that this example assumes that SimpleBLE has been installed to a location
that is part of the default CMake module path.


Usage with CMake (Local)
=============================

You can add the ``simpleble`` library directory into your project and include it in
your ``CMakeLists.txt`` file ::

   add_subdirectory(<path-to-simpleble> ${CMAKE_BINARY_DIR}/simpleble)
   target_link_libraries(<your-target> simpleble::simpleble)


Usage with CMake (Vendorized)
=============================

If you want to use a vendorized copy of SimpleBLE, you can do so by using FetchContent
and specifying the location from where SimpleBLE should be consumed from. ::

   include(FetchContent)
   FetchContent_Declare(
       simpleble
       GIT_REPOSITORY <simpleble-git-repository>
       GIT_TAG <simpleble-git-tag>
       GIT_SHALLOW YES
   )

   # Note that here we manually do what FetchContent_MakeAvailable() would do,
   # except to ensure that the dependency can also get what it needs, we add
   # custom logic between the FetchContent_Populate() and add_subdirectory()
   # calls.
   FetchContent_GetProperties(simpleble)
   if(NOT simpleble_POPULATED)
       FetchContent_Populate(simpleble)
       list(APPEND CMAKE_MODULE_PATH "${simpleble_SOURCE_DIR}/cmake/find")
       add_subdirectory("${simpleble_SOURCE_DIR}/simpleble" "${simpleble_BINARY_DIR}")
   endif()

   set(simpleble_FOUND 1)

You can put this code inside ``Findsimpleble.cmake`` and add it to your CMake
module path, as depicted in `cmake-init-fetchcontent`_.

Once vendorized using the above approach, you can consume SimpleBLE from
within CMake as you'd normally do ::

   find_package(simpleble REQUIRED)
   target_link_libraries(<your-target> simpleble::simpleble)

One key security feature of SimpleBLE is that it allows the user to specify
the URLs and tags of all internal dependencies, thus allowing compilation
from internal or secure sources without the risk of those getting compromised.


Usage alongside native code in Android
======================================

When using SimpleBLE alongside native code in Android, you must include a small
Android dependency module that includes some necessary bridge classes used by SimpleBLE.
This is required because the Android JVM doesn't allow programatic definition of
derived classes, which forces us to bring these definitions in externally.

To include this dependency module, add the following to your `settings.gradle` file:

.. code-block:: groovy

   includeBuild("path/to/simpleble/src/backends/android/simpleble-bridge") {
       dependencySubstitution {
           substitute module("org.simpleble.android.bridge:simpleble-bridge") with project(":")
       }
   }

.. code-block:: kotlin

   includeBuild("path/to/simpleble/src/backends/android/simpleble-bridge") {
    dependencySubstitution {
        substitute(module("org.simpleble.android.bridge:simpleble-bridge")).using(project(":"))
    }
   }

**NOTE:** We will provide Maven packages in the future.


Build Examples
==============

Use the following instructions to build the provided SimpleBLE examples: ::

   cmake -S <path-to-simpleble>/examples/simpleble -B build_simpleble_examples -DSIMPLEBLE_LOCAL=ON
   cmake --build build_simpleble_examples -j7


Testing
=======

To build and run unit and integration tests, the following packages are
required: ::

   sudo apt install libgtest-dev libgmock-dev python3-dev
   pip3 install -r <path-to-simpleble>/test/requirements.txt


Unit Tests
----------

To run the unit tests, run the following command: ::

   cmake -S <path-to-simpleble> -B build_simpleble_test -DSIMPLEBLE_TEST=ON
   cmake --build build_simpleble_test -j7
   ./build_simpleble_test/bin/simpleble_test


Address Sanitizer Tests
-----------------------

To run the address sanitizer tests, run the following command: ::

   cmake -S <path-to-simpleble> -B build_simpleble_test -DSIMPLEBLE_SANITIZE=Address -DSIMPLEBLE_TEST=ON
   cmake --build build_simpleble_test -j7
   PYTHONMALLOC=malloc ./build_simpleble_test/bin/simpleble_test

It's important for ``PYTHONMALLOC`` to be set to ``malloc``, otherwise the tests will
fail due to Python's memory allocator from triggering false positives.


Thread Sanitizer Tests
----------------------

To run the thread sanitizer tests, run the following command: ::

   cmake -S <path-to-simpleble> -B build_simpleble_test -DSIMPLEBLE_SANITIZE=Thread -DSIMPLEBLE_TEST=ON
   cmake --build build_simpleble_test -j7
   ./build_simpleble_test/bin/simpleble_test


.. Links

.. _cmake-init-fetchcontent: https://github.com/friendlyanon/cmake-init-fetchcontent

