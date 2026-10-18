package org.simplektble

import kotlin.reflect.KMutableProperty0
import kotlin.test.AfterTest
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotEquals
import kotlin.time.Duration
import kotlin.time.Duration.Companion.milliseconds

class ConfigTest {
    /** A setting, the group that resets it, and a change away from its default. */
    private class Setting(val name: String, val group: String, val read: () -> Any, val change: () -> Unit)

    private fun boolean(group: String, property: KMutableProperty0<Boolean>) =
        Setting("$group.${property.name}", group, { property.get() }) { property.set(!property.get()) }

    private fun duration(group: String, property: KMutableProperty0<Duration>, changed: Duration) =
        Setting("$group.${property.name}", group, { property.get() }) { property.set(changed) }

    // NOTE: Every setting is checked against all the others, so a key or group mixed up on the native side shows up.
    private val settings = listOf(
        boolean("SimpleBluez", Config.SimpleBluez::useSystemBus),
        duration("SimpleBluez", Config.SimpleBluez::connectionTimeout, 1234.milliseconds),
        duration("SimpleBluez", Config.SimpleBluez::disconnectionTimeout, 4321.milliseconds),
        boolean("WinRT", Config.WinRT::experimentalReinitializeWinrtApartmentOnMainThread),
        boolean("WinRT", Config.WinRT::useDeferredDisconnect),
        boolean("Dongl", Config.Dongl::useDonglBackend),
        boolean("Dongl", Config.Dongl::autoUpdate),
        boolean("Dongl", Config.Dongl::forceUpdate)
    )

    private val groups = mapOf<String, () -> Unit>(
        "SimpleBluez" to Config.SimpleBluez::reset,
        "WinRT" to Config.WinRT::reset,
        "CoreBluetooth" to Config.CoreBluetooth::reset,
        "Dongl" to Config.Dongl::reset
    )

    @AfterTest
    fun resetConfig() {
        Config.resetAll()
    }

    @Test
    fun eachSettingChangesOnlyItself() {
        Config.resetAll()
        val defaults = settings.associate { it.name to it.read() }

        for (setting in settings) {
            setting.change()
            for (other in settings) {
                if (other === setting) {
                    assertNotEquals(defaults[other.name], other.read(), "${setting.name} did not change")
                } else {
                    assertEquals(defaults[other.name], other.read(), "Changing ${setting.name} changed ${other.name}")
                }
            }
            Config.resetAll()
        }
    }

    @Test
    fun eachGroupResetsOnlyItsSettings() {
        Config.resetAll()
        val defaults = settings.associate { it.name to it.read() }

        for ((group, reset) in groups) {
            settings.forEach { it.change() }
            reset()
            for (setting in settings) {
                if (setting.group == group) {
                    assertEquals(defaults[setting.name], setting.read(), "$group.reset() kept ${setting.name}")
                } else {
                    assertNotEquals(defaults[setting.name], setting.read(), "$group.reset() reset ${setting.name}")
                }
            }
            Config.resetAll()
        }

        settings.forEach { it.change() }
        Config.resetAll()
        for (setting in settings) {
            assertEquals(defaults[setting.name], setting.read(), "resetAll() kept ${setting.name}")
        }
    }

    @Test
    fun timeoutsMustFitSimpleBle() {
        assertFailsWith<IllegalArgumentException> { Config.SimpleBluez.connectionTimeout = (-1).milliseconds }
        assertFailsWith<IllegalArgumentException> { Config.SimpleBluez.disconnectionTimeout = Duration.INFINITE }
        Config.SimpleBluez.connectionTimeout = Duration.ZERO
        assertEquals(Duration.ZERO, Config.SimpleBluez.connectionTimeout)
    }
}
