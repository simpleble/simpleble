package org.simplektble

import kotlin.test.AfterTest
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.async
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeout

class ErrorCodeTest {
    private var adapter: Adapter? = null
    private var peripheral: Peripheral? = null

    @AfterTest
    fun cleanUp() {
        runBlocking {
            runCatching { adapter?.let { if (it.scanIsActive()) it.scanStop() } }
            runCatching { peripheral?.takeIf { it.isConnected }?.disconnect() }
        }
    }

    @Test
    fun failuresCarrySimpleBleCodes() = runBlocking {
        val activeAdapter = Adapter.getAdapters().single()
        adapter = activeAdapter
        val found = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(2_000) { activeAdapter.onScanFound.first() }
        }
        activeAdapter.scanStart()
        val activePeripheral = found.await()
        peripheral = activePeripheral
        activeAdapter.scanStop()

        assertCode(ErrorCode.PERIPHERAL_NOT_CONNECTED) { activePeripheral.read(TEST_SERVICE, BINARY_VALUE) }

        activePeripheral.connect()
        assertCode(ErrorCode.GATT_SERVICE_NOT_FOUND) { activePeripheral.read(UNKNOWN, BINARY_VALUE) }
        assertCode(ErrorCode.GATT_CHARACTERISTIC_NOT_FOUND) { activePeripheral.read(TEST_SERVICE, UNKNOWN) }
        assertCode(ErrorCode.OPERATION_NOT_SUPPORTED) {
            activePeripheral.indicate(BATTERY_SERVICE, BATTERY_LEVEL).first()
        }
        assertCode(ErrorCode.OPERATION_FAILED) { activePeripheral.read(TEST_SERVICE, ERROR_VALUE) }

        // The binding's own failures carry a code too.
        val collecting = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(5_000) { activePeripheral.notify(BATTERY_SERVICE, BATTERY_LEVEL).first() }
        }
        delay(100)
        assertCode(ErrorCode.OPERATION_FAILED) { activePeripheral.notify(BATTERY_SERVICE, BATTERY_LEVEL).first() }
        collecting.await()
        activePeripheral.disconnect()
    }

    private suspend fun assertCode(expected: ErrorCode, block: suspend () -> Unit) {
        val exception = assertFailsWith<SimpleKtBleException> { block() }
        assertEquals(expected, exception.code, exception.message)
    }

    private companion object {
        val BATTERY_SERVICE = BluetoothUUID("0000180f-0000-1000-8000-00805f9b34fb")
        val BATTERY_LEVEL = BluetoothUUID("00002a19-0000-1000-8000-00805f9b34fb")
        val TEST_SERVICE = BluetoothUUID("0000fff0-0000-1000-8000-00805f9b34fb")
        val BINARY_VALUE = BluetoothUUID("0000fff1-0000-1000-8000-00805f9b34fb")
        val ERROR_VALUE = BluetoothUUID("0000fff2-0000-1000-8000-00805f9b34fb")
        val UNKNOWN = BluetoothUUID("0000abcd-0000-1000-8000-00805f9b34fb")
    }
}
