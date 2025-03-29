=====
Usage
=====

SimpleDroidBLE is an Android-specific package that provides an API similar to
SimpleBLE, using modern Kotlin idiomatic code.

This code is currently under active development and some features are not yet
implemented or their API might change, but should be enough to help you get
started.

Obtain AAR dependencies
=======================

Using prebuilt AAR
------------------

You can download the latest SimpleDroidBLE packages from the `releases page <https://github.com/OpenBluetoothToolbox/SimpleBLE/releases/>`_.

Building from source
--------------------

Build ``simpledroidble`` and ``simpleble-bridge``, either using gradle (``./simpledroidble/gradlew assembleRelease``), or Android Studio. The following libraries will be built:
    - ``simpledroidble/simpledroidble/build/outputs/aar/simpledroidble-release.aar`` 
    - ``simpleble/src/backends/android/simpleble-bridge/build/outputs/aar/simpleble-bridge-release.aar``

Add SimpleDroidBLE to your project
==================================

From AAR file
-------------

Place the package on your app's directory, and take note of its path. We'll use ``example-app/app/libs/{simpledroidble,simpleble-bridge}.aar``.
Then, declare the dependency on your app's ``build.gradle{.kts}`` file (note: relative paths begin from your app's module folder)

.. code-block:: kotlin

    dependencies {
      /* ... */
      implementation(files("libs/simpledroidble.aar", "libs/simpleble-bridge.aar"))
    }

Consuming Locally
-----------------

If you want to use SimpleDroidBLE as part of your project from a local copy,
you can do so by adding the following to your ``settings.gradle`` or ``settings.gradle.kts`` file.
Make sure this include is before your ``include(":app")`` statement

.. code-block:: groovy

    includeBuild("path/to/simpledroidble") {
        dependencySubstitution {
            substitute module("org.simpleble.android:simpledroidble") with project(":simpledroidble")
        }
    }

Or, in Kotlin

.. code-block:: kotlin

    includeBuild("path/to/simpledroidble") {
        dependencySubstitution {
            substitute(module("org.simpleble.android:simpledroidble")).using(project(":simpledroidble"))
        }
    }

Then, inside your ``build.gradle`` file, you can add the following dependency

.. code-block:: groovy

    dependencies {
        implementation "org.simpleble.android:simpledroidble"
    }

If you are using Kotlin (``build.gradle.kts``)

.. code-block:: kotlin

    dependencies {
        implementation("org.simpleble.android:simpledroidble")
    }