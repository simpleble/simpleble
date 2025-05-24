========
Tutorial
========

In this page you'll find all the information you need to get up and running with
SimpleDroidBLE, the Kotlin Android frontend for SimpleBLE.


Getting started
===============

First, follow the `usage <usage.html>`_ page. Then, this page will explain a few
basic concepts about how SimpleDroidBLE is organized.

SimpleDroidBLE has two main classes: the `Adapter` class, which you can think of
as a physical Bluetooth adapter, and a `Peripheral`, representing a concrete 
Bluetooth device.

To initialize SimpleDroidBLE, you'll first need to provide ``SimpleDroidBle.contextReference``
with a WeakReference to the app's context. In this step, the native code (``simpleble-jni.so``)
will be loaded. This provides a native interface to ``simpleble`` as a whole,
which is then used by the Kotlin frontend to perform lower-level tasks.

After this, you will be able to call ``SimpleDroidBle#requestPermissions`` successfully. 
This requests the required Android app permissions to the user; namely Bluetooth 
access, administration, scanning, and connection.

.. code-block:: kotlin

    import android.app.Activity
    import android.os.Bundle
    import org.simpleble.android.SimpleDroidBle
    import java.lang.ref.WeakReference

    class MainActivity : Activity() {
        override fun onCreate(savedInstanceState: Bundle?) {
            /* ... */
            SimpleDroidBle.contextReference = WeakReference(this)    
            /* ... */
            SimpleDroidBle.requestPermissions()
        }
    }

This is all you need to set up SimpleDroidBLE on app start.

The Adapter class
-----------------

After setup, the main entry point to SimpleDroidBLE is the Adapter object (or 
static class in Java), which has two main methods.
   
    - The first one is ``Adapter#isBluetoothEnabled()``, which will let you know if 
      Bluetooth is enabled.
    - The second one is the ``Adapter#getAdapters()`` method, which will return the 
      device's bluetooth adapter(s).

.. warning::
   When using SimpleDroidBLE make sure to only reference a single instance of the Adapter object, as every instance creates a new internal referece in the native code.
   Having multiple instances (by unbounded calls to ``Adapter#getAdapters``, for example) may cause a memory leak.
   This can be accomplished by acquiring said instance either on your app's Application class, or, in a single activity app, on your Activity's viewmodel; and treating the Adapter as a global, singleton object.

The following code snippet shows how to use these functions

.. code-block:: kotlin
    
    import android.util.Log
    import org.simpleble.android.Adapter

    // We assume simpleble is already set-up
    fun listBluetoothDevices() {
        if (!Adapter.isBluetoothEnabled()) {
            Log.e("Example", "Bluetooth is not enabled")
            return
        }

        val adapters = Adapter.getAdapters()
        if (adapters.isEmpty()) {
            Log.e("Example", "No Bluetooth adapters found")
            return
        }

        val adapter = adapters.first()
        Log.i("Example", "Adapter identifier: ${adapter.identifier}")
        Log.i("Example", "Adapter address: ${adapter.address}")
    }

The above code will print the identifier and address of the first adapter found
using ``Adapter#identifier`` and ``Adapter#address``, respectively.
If you have more than one adapter, you can use the identifier to select the
one you want to use. The identifier is a string that uniquely identifies the
adapter within the operating system. The address is a string that represents
the MAC address of the adapter.

Scanning for peripherals
========================

SimpleDroidBLE is a `flow <https://kotlinlang.org/api/kotlinx.coroutines/kotlinx-coroutines-core/kotlinx.coroutines.flow/-flow/>`_-driven Kotlin interface.
This means that events are emitted via ``Flow``s, and consumed mainly in `Kotlin coroutines <https://kotlinlang.org/docs/coroutines-overview.html>`_.

Once you have a list of available adapters, you can start scanning for
peripherals. To do so, you need to call the ``Adapter#scanFor(Int)`` method.
This will start emitting ``Peripheral`` objects that are in range of the adapter 
on the ``Adapter#onScanFound`` flow.

.. code-block:: kotlin

    import android.util.Log
    import kotlinx.coroutines.CoroutineScope
    import kotlinx.coroutines.launch
    import org.simpleble.android.Adapter

    suspend fun listNearbyDevices() {
        // Get an adapter
        val adapter = Adapter.getAdapters().first()

        CoroutineScope(Dispatchers.Main).launch {
            adapter.onScanFound.collect {
                Log.d("Example", "Found device: ${it.identifier} [${it.address}] ${it.rssi} dBm")
            }
        }

        // Scan for peripherals for 5000 milliseconds
        adapter.scanFor(5000)
    }

The above code will print the identifier, address, and signal strength of each 
peripheral found using ``Peripheral#identifier``, ``Peripheral#address``, and
``Peripheral#rssi``, respectively.

Alternatively, you can use ``Adapter#scanStart()`` and ``Adapter#scanStop()`` to
start and stop scanning asynchronously. This is useful if you want to scan for 
peripherals in the background while performing other tasks instead of ``suspend``ing
with ``Adapter#scanFor(Int)``.

The Adapter class also has other flows which correspond to different events on
the scan process. This includes ``Adapter#onScanStart`` and ``Adapter#onScanStop``,
which emit a value when scanning either starts or stops, respectively; ``Adapter#onScanActive``,
which emits whether the ``Adapter`` is currently scanning for devices; and
``Adapter#onScanUpdated``, which emits when a peripheral property has changed.

The following code snippet shows how to use these functions

.. code-block:: kotlin

    import android.util.Log
    import kotlinx.coroutines.CoroutineScope
    import kotlinx.coroutines.launch
    import org.simpleble.android.Adapter

    suspend fun listNearbyDevices() {
        // Get an adapter
        val adapter = Adapter.getAdapters().first()

        CoroutineScope(Dispatchers.Main).run {
            launch {
                adapter.onScanFound.collect {
                    Log.d("Example", "Found device: ${it.identifier} [${it.address}] ${it.rssi} dBm")
                }
            }

            // We need multiple `launch` calls because `collect` blocks until the flow terminates
            launch {
                adapter.onScanUpdated.collect {
                    Log.d("Example", "Device updated: ${it.identifier} [${it.address}] ${it.rssi} dBm")
                }
            }
            
            launch {
                adapter.onScanStart.collect {
                    Log.d("Example", "Scan started")
                }
            }
            
            launch {
                adapter.onScanStop.collect {
                    Log.d("Example", "Scan stopped")
                }
            }

            launch {
                adapter.onScanActive.collect {
                    Log.d("Example", "Scan state changed to $it")
                }
            }
        }

        adapter.scanStart()

        // Wait for 5 seconds
        delay(5000)

        // Stop scanning for peripherals
        adapter.scanStop()
    }

Connecting to a peripheral
==========================

Once you have a list of peripherals, you can connect to one of them. To do so,
you can call ``Peripheral#connect()`` on one of the ``Peripheral`` objects 
obtained via your ``Adapter``.

.. code-block:: kotlin

    import android.util.Log
    import kotlinx.coroutines.CoroutineScope
    import kotlinx.coroutines.launch
    import org.simpleble.android.Adapter

    suspend fun connectToPeripheral() {
        val adapter = Adapters.getAdapters().first()

        CoroutineScope(Dispatchers.Main).launch {
            adapter.onScanFound.collect { peripheral ->
                adapter.scanStop()
                peripheral.connect()
            }
        }

        // Scan for peripherals for 5000 milliseconds
        adapter.scanStart()
    }

Similarly to the ``Adapter`` class, the ``Peripheral`` class has several flows.
``Peripheral#onConnected`` will emit a value when a connection is established,
``Peripheral#onConnectionActive`` will emit whether the connection is established
(analogously to ``Adapter#onScanActive``), and ``Peripheral#onDisconnected`` will
emit when the peripheral is finally disconnected from the adapter.

Here is a code sample of the aforementioned flows 

.. code-block:: kotlin

    import android.util.Log
    import kotlinx.coroutines.CoroutineScope
    import kotlinx.coroutines.launch
    import org.simpleble.android.Adapter

    suspend fun peripheralState() {
        // Get an adapter
        val adapter = Adapter.getAdapters().first()

        CoroutineScope(Dispatchers.Main).run {
            launch {
                adapter.onScanUpdated.collect { peripheral ->
                    adapter.scanStop()

                    launch {
                        peripheral.onConnected.collect {
                            Log.i("Example", "Successfully connected to ${peripheral.identifier}")
                        }
                    }

                    launch {
                        peripheral.onDisconnected.collect {
                            Log.i("Example", "Successfully disonnected from ${peripheral.identifier}")
                        }
                    }

                    launch {
                        peripheral.onConnectionActive.collect {
                            Log.i("Example", "Connection changed to $it for ${peripheral.identifier}")
                        }
                    }

                    peripheral.connect()
                    delay(500)
                    peripheral.disconnect()
                }
            }
        }

        adapter.scanStart()
    }

Learn by example
================

To learn how to use SimpleDroidBLE, please refer to the `examples`_ provided
in the repository. 

.. _examples: https://github.com/OpenBluetoothToolbox/SimpleBLE/tree/main/examples/simpleble-android