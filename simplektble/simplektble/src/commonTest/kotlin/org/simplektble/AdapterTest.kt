package org.simplektble

import kotlin.test.AfterTest
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.async
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeout

class AdapterTest {
    @AfterTest
    fun powerOn() {
        runBlocking { Adapter.getAdapters().single().powerOn() }
    }

    @Test
    fun powerChangesReachTheFlows() = runBlocking {
        val adapter = Adapter.getAdapters().single()
        assertTrue(adapter.isPowered())

        val poweredOff = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(2_000) { adapter.onPowerOff.first() }
        }
        adapter.powerOff()
        poweredOff.await()
        assertFalse(adapter.isPowered())

        val poweredOn = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(2_000) { adapter.onPowerOn.first() }
        }
        adapter.powerOn()
        poweredOn.await()
        assertTrue(adapter.isPowered())
    }

    @Test
    fun platformFunctionsReachTheBackend() {
        val adapter = Adapter.getAdapters().single()

        // Only the platforms' own backends have these, so PLAIN rejects each of them.
        val retrieve = assertFailsWith<SimpleKtBleException> {
            adapter.retrieveCachedPeripherals(listOf(BluetoothAddress("11:22:33:44:55:66")))
        }
        assertEquals(ErrorCode.OPERATION_NOT_SUPPORTED, retrieve.code)

        val local = adapter.createLocalPeripheral()
        val localName = assertFailsWith<SimpleKtBleException> { local.setAdvertisementLocalName("SimpleKtBLE") }
        assertEquals(ErrorCode.OPERATION_NOT_SUPPORTED, localName.code)
        val noLocalName = assertFailsWith<SimpleKtBleException> { local.setAdvertisementLocalName(null) }
        assertEquals(ErrorCode.OPERATION_NOT_SUPPORTED, noLocalName.code)
    }
}
