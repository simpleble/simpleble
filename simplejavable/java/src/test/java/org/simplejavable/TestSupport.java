package org.simplejavable;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

/**
 * Helpers for tests that run against a native library built with -DSIMPLEBLE_PLAIN=ON, whose single backend
 * simulates "Plain Adapter" and the remote "Plain Peripheral". The simulated state lasts for the whole test JVM,
 * so tests clear their listeners, disconnect, and stop local hosts before returning.
 */
final class TestSupport {
    static final long TIMEOUT_SECONDS = 10;

    private TestSupport() {}

    static Adapter plainAdapter() {
        List<Adapter> adapters = Adapter.getAdapters();
        assertEquals(1, adapters.size());
        return adapters.get(0);
    }

    static Peripheral scanFor(Adapter adapter, String identifier) throws Exception {
        CompletableFuture<Peripheral> found = new CompletableFuture<>();
        adapter.setEventListener(new Adapter.EventListener() {
            @Override
            public void onScanFound(Peripheral peripheral) {
                if (identifier.equals(peripheral.getIdentifier())) {
                    found.complete(peripheral);
                }
            }
        });

        adapter.scanStart();
        try {
            return found.get(TIMEOUT_SECONDS, TimeUnit.SECONDS);
        } finally {
            adapter.scanStop();
            adapter.setEventListener(null);
        }
    }
}
