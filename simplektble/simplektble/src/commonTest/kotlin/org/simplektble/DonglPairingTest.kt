package org.simplektble

import kotlin.test.AfterTest
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.async
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeout

class DonglPairingTest {
    private var adapter: Adapter? = null

    @AfterTest
    fun cleanUp() {
        runBlocking { runCatching { adapter?.let { if (it.scanIsActive()) it.scanStop() } } }
    }

    @Test
    fun pairingHandlersReachTheBackend(): Unit = runBlocking {
        val activeAdapter = Adapter.getAdapters().single()
        adapter = activeAdapter
        val found = async(start = CoroutineStart.UNDISPATCHED) {
            withTimeout(2_000) { activeAdapter.onScanFound.first() }
        }
        activeAdapter.scanStart()
        val peripheral = found.await()
        activeAdapter.scanStop()

        // Only Dongl peripherals pair through these handlers, so PLAIN's peripheral rejects each of them.
        val rejections = listOf(
            assertFailsWith<SimpleKtBleException> { peripheral.setPasskeyRequestHandler { "123456" } },
            assertFailsWith<SimpleKtBleException> { peripheral.setPasskeyDisplayHandler { } },
            assertFailsWith<SimpleKtBleException> { peripheral.setNumericComparisonHandler { true } }
        )
        rejections.forEach { assertEquals(ErrorCode.OPERATION_NOT_SUPPORTED, it.code) }
    }
}
