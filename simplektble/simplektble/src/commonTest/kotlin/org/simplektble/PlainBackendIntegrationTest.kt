package org.simplektble

import kotlin.test.AfterTest
import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertTrue
import kotlinx.coroutines.async
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.buffer
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeout
import kotlinx.coroutines.yield

class PlainBackendIntegrationTest {
    private var adapter: Adapter? = null
    private var peripheral: Peripheral? = null

    @AfterTest
    fun cleanUp() {
        runBlocking {
            runCatching { adapter?.takeIf { it.scanIsActive }?.scanStop() }
            runCatching { peripheral?.takeIf { it.isConnected }?.disconnect() }
        }
    }

    @Test
    fun scanConnectReadNotifyAndDisconnect() = runBlocking {
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
        val service = services.first { it.uuid == BluetoothUUID("0000180f-0000-1000-8000-00805f9b34fb") }
        val characteristic = service.characteristics.single()
        assertTrue(characteristic.canRead)
        assertTrue(characteristic.canNotify)
        assertContentEquals(
            byteArrayOf(100),
            activePeripheral.read(service.uuid, characteristic.uuid)
        )
        val descriptor = characteristic.descriptors.single()
        assertContentEquals(
            byteArrayOf(0x00, 0x00),
            activePeripheral.read(
                service.uuid,
                characteristic.uuid,
                descriptor.uuid
            )
        )
        activePeripheral.write(
            service.uuid,
            characteristic.uuid,
            descriptor.uuid,
            byteArrayOf(0x01, 0x00)
        )

        val writeService = services.first { it.uuid == BluetoothUUID("0000fff0-0000-1000-8000-00805f9b34fb") }
        val writeCharacteristic = writeService.characteristics.single {
            it.uuid == BluetoothUUID("0000fff1-0000-1000-8000-00805f9b34fb")
        }
        assertTrue(writeCharacteristic.canWriteRequest)
        assertTrue(writeCharacteristic.canWriteCommand)
        activePeripheral.writeRequest(
            writeService.uuid,
            writeCharacteristic.uuid,
            byteArrayOf(0x01, 0x02)
        )
        activePeripheral.writeCommand(
            writeService.uuid,
            writeCharacteristic.uuid,
            byteArrayOf(0x00, 0x7f, 0x80.toByte(), 0xff.toByte())
        )
        assertContentEquals(
            byteArrayOf(0x00, 0x7f, 0x80.toByte(), 0xff.toByte()),
            activePeripheral.read(writeService.uuid, writeCharacteristic.uuid)
        )
        assertContentEquals(
            byteArrayOf(0x00, 0x7f, 0x80.toByte(), 0xff.toByte()),
            withTimeout(3_000) {
                activePeripheral.indicate(writeService.uuid, writeCharacteristic.uuid).first()
            }
        )

        val firstPayload = withTimeout(3_000) {
            activePeripheral.notify(
                service.uuid,
                characteristic.uuid
            ).first()
        }
        assertContentEquals(byteArrayOf(100), firstPayload)

        var payloadCount = 0
        val overflow = runCatching {
            withTimeout(5_000) {
                activePeripheral.notify(
                    service.uuid,
                    characteristic.uuid
                ).buffer(0).collect {
                    payloadCount++
                    delay(2_000)
                }
            }
        }.exceptionOrNull()
        assertEquals(1, payloadCount)
        assertIs<SimpleKtBleException>(overflow)
        assertEquals(
            "Notification buffer overflow for ${service.uuid}/${characteristic.uuid}",
            overflow.message
        )

        val secondPayload = withTimeout(3_000) {
            activePeripheral.notify(
                service.uuid,
                characteristic.uuid
            ).first()
        }
        assertContentEquals(byteArrayOf(100), secondPayload)

        activePeripheral.disconnect()
        assertTrue(!activePeripheral.isConnected)
    }
}
