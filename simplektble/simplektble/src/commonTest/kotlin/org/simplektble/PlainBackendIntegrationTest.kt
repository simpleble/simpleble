package org.simplektble

import kotlin.test.AfterTest
import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.async
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.take
import kotlinx.coroutines.flow.toList
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

        // A collector that falls behind keeps receiving values instead of failing.
        var payloadCount = 0
        withTimeout(5_000) {
            activePeripheral.notify(service.uuid, characteristic.uuid).take(3).collect {
                payloadCount++
                delay(500)
            }
        }
        assertEquals(3, payloadCount)

        val secondPayload = withTimeout(3_000) {
            activePeripheral.notify(
                service.uuid,
                characteristic.uuid
            ).first()
        }
        assertContentEquals(byteArrayOf(100), secondPayload)

        // A disconnection completes the flow, after the values received before it.
        val beforeDisconnection = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(5_000) { activePeripheral.notify(service.uuid, characteristic.uuid).toList() }
        }
        delay(1_000)
        activePeripheral.disconnect()
        assertTrue(beforeDisconnection.await().isNotEmpty())
        assertTrue(!activePeripheral.isConnected)

        // After reconnecting, collecting again subscribes again.
        activePeripheral.connect()
        val afterReconnection = withTimeout(3_000) {
            activePeripheral.notify(service.uuid, characteristic.uuid).first()
        }
        assertContentEquals(byteArrayOf(100), afterReconnection)
        activePeripheral.disconnect()
    }

    @Test
    fun localManufacturerDataWithoutConnections() = runBlocking {
        val activeAdapter = Adapter.getAdapters().single()
        adapter = activeAdapter
        val local = activeAdapter.createLocalPeripheral()
        local.setAdvertisedManufacturerData(0x004c, byteArrayOf(0x02))
        local.setAdvertisedManufacturerData(0x004c, byteArrayOf(0x02, 0x15))
        local.setConnectable(false)
        try {
            local.start()
            activeAdapter.scanFor(250)
            val beacon = activeAdapter.scanGetResults().single { it.identifier == "Plain Adapter Peripheral" }
            assertContentEquals(byteArrayOf(0x02, 0x15), beacon.manufacturerData()[0x004c])
            assertFalse(beacon.isConnectable)
        } finally {
            local.stop()
        }
    }

    @Test
    fun localGattLoopback() = runBlocking {
        val activeAdapter = Adapter.getAdapters().single()
        adapter = activeAdapter
        val local = activeAdapter.createLocalPeripheral()
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
            assertContentEquals(payload, peer.read(serviceUuid, valueUuid))
            value.setReadHandler { byteArrayOf(0x42) }
            assertContentEquals(byteArrayOf(0x42), peer.read(serviceUuid, valueUuid))
            value.setReadHandler(null)
            peer.writeRequest(serviceUuid, valueUuid, byteArrayOf(0x12))
            assertContentEquals(byteArrayOf(0x12), value.value)

            val subscribed = async { withTimeout(2_000) { value.onSubscribed.first() } }
            val notification = async { withTimeout(3_000) { peer.notify(serviceUuid, valueUuid).first() } }
            subscribed.await()
            value.value = payload
            assertContentEquals(payload, notification.await())
            local.stop()
            assertTrue(!peer.isConnected)
            assertTrue(!local.isStarted)
        } finally {
            local.stop()
        }
    }
}
