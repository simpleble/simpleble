package org.simpleble.android

import androidx.test.ext.junit.runners.AndroidJUnit4
import kotlinx.coroutines.async
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeout
import kotlinx.coroutines.yield
import org.junit.After
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class PlainBackendIntegrationTest {
    private var adapter: Adapter? = null
    private var peripheral: Peripheral? = null

    @After
    fun cleanUp() {
        runBlocking {
            runCatching { adapter?.takeIf { it.scanIsActive }?.scanStop() }
            runCatching { peripheral?.takeIf { it.isConnected }?.disconnect() }
        }
    }

    @Test
    fun scanConnectReadNotifyAndDisconnect() = runBlocking {
        assertEquals(BuildConfig.VERSION_NAME, SimpleDroidBle.getVersion())
        val activeAdapter = Adapter.getAdapters().single()
        adapter = activeAdapter
        assertEquals("Plain Adapter", activeAdapter.identifier)

        val found = async { withTimeout(2_000) { activeAdapter.onScanFound.first() } }
        yield()
        activeAdapter.scanStart()
        val activePeripheral = found.await()
        peripheral = activePeripheral
        activeAdapter.scanStop()

        assertEquals("Plain Peripheral", activePeripheral.identifier)
        activePeripheral.connect()
        assertTrue(activePeripheral.isConnected)

        val services = activePeripheral.services()
        val service = services.first { it.uuid == "0000180f-0000-1000-8000-00805f9b34fb" }
        val characteristic = service.characteristics.single()
        assertTrue(characteristic.canRead)
        assertTrue(characteristic.canNotify)
        assertArrayEquals(
            byteArrayOf(),
            activePeripheral.read(BluetoothUUID(service.uuid), BluetoothUUID(characteristic.uuid))
        )
        val descriptor = characteristic.descriptors.single()
        assertArrayEquals(
            byteArrayOf(),
            activePeripheral.read(
                BluetoothUUID(service.uuid),
                BluetoothUUID(characteristic.uuid),
                BluetoothUUID(descriptor.uuid)
            )
        )
        activePeripheral.write(
            BluetoothUUID(service.uuid),
            BluetoothUUID(characteristic.uuid),
            BluetoothUUID(descriptor.uuid),
            byteArrayOf(0x01, 0x00)
        )

        val writeService = services.first { it.uuid == "0000fff0-0000-1000-8000-00805f9b34fb" }
        val writeCharacteristic = writeService.characteristics.single()
        assertTrue(writeCharacteristic.canWriteRequest)
        assertTrue(writeCharacteristic.canWriteCommand)
        activePeripheral.writeRequest(
            BluetoothUUID(writeService.uuid),
            BluetoothUUID(writeCharacteristic.uuid),
            byteArrayOf(0x01, 0x02)
        )
        activePeripheral.writeCommand(
            BluetoothUUID(writeService.uuid),
            BluetoothUUID(writeCharacteristic.uuid),
            byteArrayOf(0x03, 0x04)
        )

        val firstPayload = withTimeout(3_000) {
            activePeripheral.notify(
                BluetoothUUID(service.uuid),
                BluetoothUUID(characteristic.uuid)
            ).first()
        }
        assertEquals("Hello from notify", firstPayload.decodeToString())

        val secondPayload = withTimeout(3_000) {
            activePeripheral.notify(
                BluetoothUUID(service.uuid),
                BluetoothUUID(characteristic.uuid)
            ).first()
        }
        assertEquals("Hello from notify", secondPayload.decodeToString())

        activePeripheral.disconnect()
        assertTrue(!activePeripheral.isConnected)
    }
}
