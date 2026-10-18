package org.simplektble

import kotlin.test.AfterTest
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlin.time.Duration.Companion.milliseconds

class ConfigTest {
    @AfterTest
    fun resetConfig() {
        Config.resetAll()
    }

    @Test
    fun valuesRoundTripAndReset() {
        val useSystemBus = Config.SimpleBluez.useSystemBus
        val connectionTimeout = Config.SimpleBluez.connectionTimeout
        val useDeferredDisconnect = Config.WinRT.useDeferredDisconnect
        val useDonglBackend = Config.Dongl.useDonglBackend

        Config.SimpleBluez.useSystemBus = !useSystemBus
        Config.SimpleBluez.connectionTimeout = 1500.milliseconds
        Config.SimpleBluez.disconnectionTimeout = 500.milliseconds
        Config.WinRT.experimentalReinitializeWinrtApartmentOnMainThread = true
        Config.WinRT.useDeferredDisconnect = !useDeferredDisconnect
        Config.Dongl.useDonglBackend = !useDonglBackend
        Config.Dongl.autoUpdate = true
        Config.Dongl.forceUpdate = true

        assertEquals(!useSystemBus, Config.SimpleBluez.useSystemBus)
        assertEquals(1500.milliseconds, Config.SimpleBluez.connectionTimeout)
        assertEquals(500.milliseconds, Config.SimpleBluez.disconnectionTimeout)
        assertTrue(Config.WinRT.experimentalReinitializeWinrtApartmentOnMainThread)
        assertEquals(!useDeferredDisconnect, Config.WinRT.useDeferredDisconnect)
        assertEquals(!useDonglBackend, Config.Dongl.useDonglBackend)
        assertTrue(Config.Dongl.autoUpdate)
        assertTrue(Config.Dongl.forceUpdate)

        Config.Dongl.reset()
        assertEquals(useDonglBackend, Config.Dongl.useDonglBackend)
        assertEquals(!useSystemBus, Config.SimpleBluez.useSystemBus)

        Config.resetAll()
        assertEquals(useSystemBus, Config.SimpleBluez.useSystemBus)
        assertEquals(connectionTimeout, Config.SimpleBluez.connectionTimeout)
        assertEquals(useDeferredDisconnect, Config.WinRT.useDeferredDisconnect)
        assertFalse(Config.WinRT.experimentalReinitializeWinrtApartmentOnMainThread)
    }
}
