package org.simpleble.android

import androidx.test.ext.junit.runners.AndroidJUnit4
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.async
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeout
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class SimpleDroidBlePlainTest {
    @Test
    fun exercisesNativeBoundary() = runBlocking {
        assertTrue(Adapter.isBluetoothEnabled())

        val adapter = Adapter.getAdapters().single()
        assertEquals("Plain Adapter", adapter.identifier)
        assertEquals("AA:BB:CC:DD:EE:FF", adapter.address.toString())

        val scanStarted = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(2_000) { adapter.onScanStart.first() }
        }
        val scanFound = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(2_000) { adapter.onScanFound.first() }
        }
        val scanUpdated = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(2_000) { adapter.onScanUpdated.first() }
        }

        adapter.scanStart()
        scanStarted.await()
        val peripheral = scanFound.await()
        assertSame(peripheral, scanUpdated.await())
        assertTrue(adapter.scanIsActive)
        assertSame(peripheral, adapter.scanGetResults().single())
        assertSame(peripheral, adapter.getPairedPeripherals().single())

        val scanStopped = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(2_000) { adapter.onScanStop.first() }
        }
        adapter.scanStop()
        scanStopped.await()
        assertFalse(adapter.scanIsActive)

        assertEquals("Plain Peripheral", peripheral.identifier)
        assertEquals("11:22:33:44:55:66", peripheral.address.toString())
        assertEquals(-60, peripheral.rssi)
        assertEquals(5, peripheral.txPower)
        assertTrue(peripheral.isConnectable)

        val connected = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(2_000) { peripheral.onConnected.first() }
        }
        peripheral.connect()
        connected.await()
        assertTrue(peripheral.isConnected)
        assertTrue(peripheral.isPaired)
        assertEquals(247, peripheral.mtu)

        val services = peripheral.services()
        assertEquals(2, services.size)
        val battery = services.first()
        assertEquals(BATTERY_SERVICE, battery.uuid)
        assertTrue(battery.characteristics.single().canNotify)
        assertEquals(CLIENT_CONFIGURATION_DESCRIPTOR, battery.characteristics.single().descriptors.single().uuid)
        assertArrayEquals("test".encodeToByteArray(), peripheral.manufacturerData()[0x004C])

        val payload = withTimeout(3_000) {
            peripheral.notify(BluetoothUUID(BATTERY_SERVICE), BluetoothUUID(BATTERY_CHARACTERISTIC)).first()
        }
        assertArrayEquals("Hello from notify".encodeToByteArray(), payload)

        repeat(250) {
            assertEquals(2, peripheral.services().size)
            assertArrayEquals("test".encodeToByteArray(), peripheral.manufacturerData()[0x004C])
        }

        val disconnected = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(2_000) { peripheral.onDisconnected.first() }
        }
        peripheral.disconnect()
        disconnected.await()
        assertFalse(peripheral.isConnected)
    }

    private companion object {
        const val BATTERY_SERVICE = "0000180f-0000-1000-8000-00805f9b34fb"
        const val BATTERY_CHARACTERISTIC = "00002a19-0000-1000-8000-00805f9b34fb"
        const val CLIENT_CONFIGURATION_DESCRIPTOR = "00002902-0000-1000-8000-00805f9b34fb"
    }
}
