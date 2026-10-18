package org.simplektble

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertSame
import kotlin.test.assertTrue

class BackendTest {
    @Test
    fun namedBackends() {
        val backend = Backend.getNative()
        assertEquals("Plain", backend.identifier)
        assertSame(Backend.getBackends().single(), backend)
        assertTrue(backend.isBluetoothEnabled())
        assertSame(Adapter.getAdapters().single(), backend.getAdapters().single())

        assertFailsWith<SimpleKtBleException> { Backend.getDongl() }
        assertFailsWith<SimpleKtBleException> { Backend.getSimulator() }
    }
}
