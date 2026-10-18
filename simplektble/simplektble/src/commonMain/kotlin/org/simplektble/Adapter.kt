package org.simplektble

import kotlinx.atomicfu.locks.SynchronizedObject
import kotlinx.atomicfu.locks.synchronized
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.IO
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.withContext
import kotlin.jvm.JvmStatic

public class Adapter private constructor(private val instanceId: Long) {
    private val _onPowerOn = eventFlow<Unit>()
    private val _onPowerOff = eventFlow<Unit>()
    private val _onScanStart = eventFlow<Unit>()
    private val _onScanStop = eventFlow<Unit>()
    private val _onScanActive = eventFlow<Boolean>(replay = 1)
    private val _onScanUpdated = eventFlow<Peripheral>()
    private val _onScanFound = eventFlow<Peripheral>()

    private val peripheralsLock = SynchronizedObject()
    private val peripherals = mutableMapOf<Long, Peripheral>()

    private fun peripheral(peripheralId: Long): Peripheral {
        return synchronized(peripheralsLock) {
            peripherals.getOrPut(peripheralId) { Peripheral(instanceId, peripheralId) }
        }
    }

    private val callbacks = object : AdapterCallback {
        override fun onPowerOn() {
            _onPowerOn.tryEmit(Unit)
        }

        override fun onPowerOff() {
            _onPowerOff.tryEmit(Unit)
        }

        override fun onScanStart() {
            _onScanStart.tryEmit(Unit)
            _onScanActive.tryEmit(true)
        }

        override fun onScanStop() {
            _onScanStop.tryEmit(Unit)
            _onScanActive.tryEmit(false)
        }

        override fun onScanUpdated(peripheralId: Long) {
            _onScanUpdated.tryEmit(peripheral(peripheralId))
        }

        override fun onScanFound(peripheralId: Long) {
            _onScanFound.tryEmit(peripheral(peripheralId))
        }
    }

    init { Native.adapterRegister(instanceId, callbacks) }

    public val identifier: String get() = Native.adapterIdentifier(instanceId)

    public val address: BluetoothAddress get() = BluetoothAddress(Native.adapterAddress(instanceId))

    /** Asks the Bluetooth stack whether this adapter is powered on. */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun isPowered(): Boolean {
        return withContext(Dispatchers.IO) {
            Native.adapterIsPowered(instanceId)
        }
    }

    /** Asks the Bluetooth stack to power this adapter on, where the platform lets applications do it. */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun powerOn() {
        withContext(Dispatchers.IO) {
            Native.adapterPowerOn(instanceId)
        }
    }

    /** Asks the Bluetooth stack to power this adapter off, where the platform lets applications do it. */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun powerOff() {
        withContext(Dispatchers.IO) {
            Native.adapterPowerOff(instanceId)
        }
    }

    public val onPowerOn: SharedFlow<Unit> get() = _onPowerOn

    public val onPowerOff: SharedFlow<Unit> get() = _onPowerOff

    @Throws(SimpleKtBleException::class)
    public fun scanStart() {
        Native.adapterScanStart(instanceId)
    }

    @Throws(SimpleKtBleException::class)
    public fun scanStop() {
        Native.adapterScanStop(instanceId)
    }

    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun scanFor(timeoutMs: Int) {
        withContext(Dispatchers.IO) {
            Native.adapterScanFor(instanceId, timeoutMs)
        }
    }

    /** Asks the Bluetooth stack whether this adapter is scanning. */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun scanIsActive(): Boolean {
        return withContext(Dispatchers.IO) {
            Native.adapterScanIsActive(instanceId)
        }
    }

    @Throws(SimpleKtBleException::class)
    public fun scanGetResults(): List<Peripheral> {
        return Native.adapterScanGetResults(instanceId).map(::peripheral)
    }

    public val onScanStart: SharedFlow<Unit> get() = _onScanStart

    public val onScanStop: SharedFlow<Unit> get() = _onScanStop

    public val onScanActive: SharedFlow<Boolean> get() = _onScanActive

    public val onScanUpdated: SharedFlow<Peripheral> get() = _onScanUpdated

    public val onScanFound: SharedFlow<Peripheral> get() = _onScanFound

    /** The peripherals paired with this adapter. Windows looks them up among the system's devices. */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun getPairedPeripherals(): List<Peripheral> {
        val peripheralIds = withContext(Dispatchers.IO) { Native.adapterGetPairedPeripherals(instanceId) }
        return peripheralIds.map(::peripheral)
    }

    /** The peripherals connected to this adapter. Windows looks them up among the system's devices. */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun getConnectedPeripherals(): List<Peripheral> {
        val peripheralIds = withContext(Dispatchers.IO) { Native.adapterGetConnectedPeripherals(instanceId) }
        return peripheralIds.map(::peripheral)
    }

    /**
     * The peripherals CoreBluetooth already knows on iOS and macOS, by the identifiers [Peripheral.address] returned
     * earlier, without scanning. Unknown identifiers are skipped, and a returned peripheral might be out of range. The
     * adapter must be powered on. Other platforms throw [ErrorCode.OPERATION_NOT_SUPPORTED].
     */
    @Throws(SimpleKtBleException::class)
    public fun retrieveCachedPeripherals(identifiers: List<BluetoothAddress>): List<Peripheral> {
        val values = identifiers.map(BluetoothAddress::value).toTypedArray()
        return Native.adapterRetrieveCachedPeripherals(instanceId, values).map(::peripheral)
    }

    /**
     * Creates a GATT server hosted by this application. Configure its services and advertising data before starting
     * it.
     */
    @Throws(SimpleKtBleException::class)
    public fun createLocalPeripheral(): LocalPeripheral {
        return LocalPeripheral(Native.adapterCreateLocalPeripheral(instanceId))
    }

    public companion object {
        private val adapterCacheLock = SynchronizedObject()
        private val adapterCache = mutableMapOf<Long, Adapter>()

        @JvmStatic
        @Throws(SimpleKtBleException::class)
        public fun isBluetoothEnabled(): Boolean {
            return Native.isBluetoothEnabled()
        }

        @JvmStatic
        @Throws(SimpleKtBleException::class)
        public fun getAdapters(): List<Adapter> {
            return Native.getAdapters().map(::forId)
        }

        /** The one Adapter object for a native adapter, wherever its ID came from. */
        internal fun forId(adapterId: Long): Adapter {
            return synchronized(adapterCacheLock) { adapterCache.getOrPut(adapterId) { Adapter(adapterId) } }
        }
    }
}
