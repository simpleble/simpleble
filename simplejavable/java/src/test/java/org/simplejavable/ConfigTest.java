package org.simplejavable;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.time.Duration;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;

@SuppressWarnings("deprecation")
class ConfigTest {
    @AfterEach
    void resetConfig() {
        Config.resetAll();
    }

    @Test
    void valuesRoundTripAndReset() {
        Config.SimpleBluez.setUseSystemBus(false);
        Config.SimpleBluez.setConnectionTimeout(Duration.ofMillis(1500));
        Config.SimpleBluez.setDisconnectionTimeout(Duration.ofMillis(500));
        Config.WinRT.setExperimentalUseOwnMtaApartment(false);
        Config.WinRT.setExperimentalReinitializeWinrtApartmentOnMainThread(true);
        Config.WinRT.setUseDeferredDisconnect(false);
        Config.Dongl.setUseDonglBackend(true);
        Config.Dongl.setAutoUpdate(true);
        Config.Dongl.setForceUpdate(true);

        assertFalse(Config.SimpleBluez.getUseSystemBus());
        assertEquals(Duration.ofMillis(1500), Config.SimpleBluez.getConnectionTimeout());
        assertEquals(Duration.ofMillis(500), Config.SimpleBluez.getDisconnectionTimeout());
        assertFalse(Config.WinRT.getExperimentalUseOwnMtaApartment());
        assertTrue(Config.WinRT.getExperimentalReinitializeWinrtApartmentOnMainThread());
        assertFalse(Config.WinRT.getUseDeferredDisconnect());
        assertTrue(Config.Dongl.getUseDonglBackend());
        assertTrue(Config.Dongl.getAutoUpdate());
        assertTrue(Config.Dongl.getForceUpdate());

        Config.resetAll();
        assertTrue(Config.SimpleBluez.getUseSystemBus());
        assertEquals(Duration.ofSeconds(2), Config.SimpleBluez.getConnectionTimeout());
        assertTrue(Config.WinRT.getUseDeferredDisconnect());
        assertFalse(Config.Dongl.getUseDonglBackend());
    }
}
