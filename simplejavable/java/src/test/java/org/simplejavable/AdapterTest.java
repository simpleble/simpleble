package org.simplejavable;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ForkJoinPool;
import java.util.concurrent.TimeUnit;
import org.junit.jupiter.api.Test;

class AdapterTest {
    @Test
    void listenerSurvivesAnotherEnumeration() throws Exception {
        Adapter adapter = TestSupport.plainAdapter();
        CountDownLatch found = new CountDownLatch(1);
        adapter.setEventListener(new Adapter.EventListener() {
            @Override
            public void onScanFound(Peripheral peripheral) {
                found.countDown();
            }
        });

        // Enumerating again used to replace, and then clear, the native scan callbacks.
        Adapter.getAdapters();
        Backend.getBackends().get(0).getAdapters();

        adapter.scanStart();
        try {
            assertTrue(found.await(TestSupport.TIMEOUT_SECONDS, TimeUnit.SECONDS));
        } finally {
            adapter.scanStop();
            adapter.setEventListener(null);
        }
    }

    @Test
    void asyncCallsLeaveTheCommonPoolFree() throws Exception {
        Adapter adapter = TestSupport.plainAdapter();
        List<CompletableFuture<?>> scans = new ArrayList<>();
        for (int i = 0; i < ForkJoinPool.getCommonPoolParallelism(); i++) {
            scans.add(adapter.scanForAsync(2000));
        }

        try {
            // Async calls used to block the common pool, so other work there waited for the scans.
            assertEquals(1, CompletableFuture.supplyAsync(() -> 1).get(1, TimeUnit.SECONDS));
        } finally {
            for (CompletableFuture<?> scan : scans) {
                scan.get(TestSupport.TIMEOUT_SECONDS, TimeUnit.SECONDS);
            }
        }
    }
}
