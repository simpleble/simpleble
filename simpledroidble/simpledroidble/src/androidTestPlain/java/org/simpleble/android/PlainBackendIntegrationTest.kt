package org.simpleble.android

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import kotlinx.coroutines.async
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.buffer
import kotlinx.coroutines.flow.collect
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
        val service = services.first { it.uuid == BluetoothUUID("0000180f-0000-1000-8000-00805f9b34fb") }
        val characteristic = service.characteristics.single()
        assertTrue(characteristic.canRead)
        assertTrue(characteristic.canNotify)
        assertArrayEquals(
            byteArrayOf(100),
            activePeripheral.read(service.uuid, characteristic.uuid)
        )
        val descriptor = characteristic.descriptors.single()
        assertArrayEquals(
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
        assertArrayEquals(
            byteArrayOf(0x00, 0x7f, 0x80.toByte(), 0xff.toByte()),
            activePeripheral.read(writeService.uuid, writeCharacteristic.uuid)
        )
        assertArrayEquals(
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
        assertArrayEquals(byteArrayOf(100), firstPayload)

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
        assertTrue(overflow is SimpleDroidBleException)
        assertEquals(
            "Notification buffer overflow for ${service.uuid}/${characteristic.uuid}",
            overflow?.message
        )

        val secondPayload = withTimeout(3_000) {
            activePeripheral.notify(
                service.uuid,
                characteristic.uuid
            ).first()
        }
        assertArrayEquals(byteArrayOf(100), secondPayload)

        activePeripheral.disconnect()
        assertTrue(!activePeripheral.isConnected)
    }

    @Test
    fun localGattLoopback() = runBlocking {
        val activeAdapter = Adapter.getAdapters().single()
        adapter = activeAdapter
        val local = activeAdapter.createLocalPeripheral(InstrumentationRegistry.getInstrumentation().targetContext)
        val serviceUuid = BluetoothUUID("0000fff0-0000-1000-8000-00805f9b34fb")
        val valueUuid = BluetoothUUID("0000fff1-0000-1000-8000-00805f9b34fb")
        val value = local.addService(serviceUuid).addCharacteristic(
            valueUuid,
            LocalCharacteristicCapability.Read,
            LocalCharacteristicCapability.WriteRequest,
            LocalCharacteristicCapability.Notify
        )
        val payload = byteArrayOf(0x00, 0xff.toByte(), 0x80.toByte())
        value.value = payload
        local.addAdvertisedService(serviceUuid)
        try {
            local.start()
            assertTrue(local.isStarted)
            assertTrue(local.isAdvertising)
            activeAdapter.scanFor(250)
            val peer = activeAdapter.scanGetResults().single { it.identifier == "Plain Adapter Peripheral" }
            peripheral = peer
            peer.connect()
            assertArrayEquals(payload, peer.read(serviceUuid, valueUuid))
            value.setReadHandler { byteArrayOf(0x42) }
            assertArrayEquals(byteArrayOf(0x42), peer.read(serviceUuid, valueUuid))
            value.setReadHandler(null)
            peer.writeRequest(serviceUuid, valueUuid, byteArrayOf(0x12))
            assertArrayEquals(byteArrayOf(0x12), value.value)

            val subscribed = async { withTimeout(2_000) { value.onSubscribed.first() } }
            val notification = async { withTimeout(3_000) { peer.notify(serviceUuid, valueUuid).first() } }
            subscribed.await()
            value.value = payload
            assertArrayEquals(payload, notification.await())
            local.stop()
            assertTrue(!peer.isConnected)
            assertTrue(!local.isStarted)
        } finally {
            local.stop()
        }
    }
}
