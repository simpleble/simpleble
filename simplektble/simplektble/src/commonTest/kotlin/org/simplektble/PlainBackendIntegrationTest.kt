package org.simplektble

import kotlin.test.AfterTest
import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertSame
import kotlin.test.assertTrue
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitCancellation
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.take
import kotlinx.coroutines.flow.toList
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeout
import kotlinx.coroutines.yield
import kotlin.time.Duration.Companion.milliseconds
import kotlin.time.TimeSource

class PlainBackendIntegrationTest {
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

        // A collector that falls behind gets every value sent meanwhile, at once, instead of losing them.
        val arrivals = mutableListOf<TimeSource.Monotonic.ValueTimeMark>()
        withTimeout(10_000) {
            activePeripheral.notify(service.uuid, characteristic.uuid).take(4).collect {
                arrivals += TimeSource.Monotonic.markNow()
                // PLAIN notifies once a second, so this falls about three values behind.
                if (arrivals.size == 1) delay(3_500)
            }
        }
        val catchUp = arrivals[3] - arrivals[1]
        assertTrue(catchUp < 500.milliseconds, "The values sent during the delay took $catchUp to arrive")

        val secondPayload = withTimeout(3_000) {
            activePeripheral.notify(
                service.uuid,
                characteristic.uuid
            ).first()
        }
        assertContentEquals(byteArrayOf(100), secondPayload)

        // unsubscribe() ends the subscription from outside its collector, and completes the flow.
        val beforeUnsubscribe = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(5_000) { activePeripheral.notify(service.uuid, characteristic.uuid).toList() }
        }
        delay(1_500)
        activePeripheral.unsubscribe(service.uuid, characteristic.uuid)
        assertTrue(beforeUnsubscribe.await().isNotEmpty())

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
    fun teardownFinishesInACancelledCoroutine() = runBlocking {
        val activeAdapter = Adapter.getAdapters().single()
        adapter = activeAdapter
        activeAdapter.scanFor(250)
        val target = activeAdapter.scanGetResults().single { it.identifier == "Plain Peripheral" }
        peripheral = target
        target.connect()
        activeAdapter.scanStart()

        val owner = launch(start = CoroutineStart.UNDISPATCHED) {
            try {
                awaitCancellation()
            } finally {
                activeAdapter.scanStop()
                target.disconnect()
            }
        }
        owner.cancelAndJoin()

        assertFalse(activeAdapter.scanIsActive())
        assertFalse(target.isConnected)
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
    fun localServicesComeFromSimpleBLE() {
        val local = Adapter.getAdapters().single().createLocalPeripheral()
        val second = local.addService(BluetoothUUID("0000fff2-0000-1000-8000-00805f9b34fb"))
        val first = local.addService(BluetoothUUID("0000fff1-0000-1000-8000-00805f9b34fb"))
        val characteristic = first.addCharacteristic(
            BluetoothUUID("0000fff3-0000-1000-8000-00805f9b34fb"),
            CharacteristicCapability.Read
        )

        // PLAIN lists services by UUID, and each listed object is the one that was added.
        assertEquals(listOf(first, second), local.services())
        assertSame(characteristic, local.services().first().characteristics().single())
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
            CharacteristicCapability.Read,
            CharacteristicCapability.WriteRequest,
            CharacteristicCapability.Notify
        )
        val payload = byteArrayOf(0x00, 0xff.toByte(), 0x80.toByte())
        value.setValue(payload)
        local.addAdvertisedService(serviceUuid)
        try {
            local.start()
            assertTrue(local.isStarted)
            assertTrue(local.isAdvertising())
            activeAdapter.scanFor(250)
            val peer = activeAdapter.scanGetResults().single { it.identifier == "Plain Adapter Peripheral" }
            peripheral = peer
            peer.connect()
            assertContentEquals(payload, peer.read(serviceUuid, valueUuid))
            value.setReadHandler { byteArrayOf(0x42) }
            assertContentEquals(byteArrayOf(0x42), peer.read(serviceUuid, valueUuid))
            // A read handler that throws serves the stored value.
            value.setReadHandler { error("The read handler failed") }
            assertContentEquals(payload, peer.read(serviceUuid, valueUuid))
            value.setReadHandler(null)
            peer.writeRequest(serviceUuid, valueUuid, byteArrayOf())
            assertContentEquals(byteArrayOf(), value.value)
            peer.writeRequest(serviceUuid, valueUuid, byteArrayOf(0x12))
            assertContentEquals(byteArrayOf(0x12), value.value)

            val subscribed = async { withTimeout(2_000) { value.onSubscribed.first() } }
            val notification = async { withTimeout(3_000) { peer.notify(serviceUuid, valueUuid).first() } }
            subscribed.await()
            value.setValue(payload)
            assertContentEquals(payload, notification.await())
            local.stop()
            assertTrue(!peer.isConnected)
            assertTrue(!local.isStarted)
        } finally {
            local.stop()
        }
    }
}
