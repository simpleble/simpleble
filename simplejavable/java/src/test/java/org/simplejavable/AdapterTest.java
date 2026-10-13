package org.simplejavable;

import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.concurrent.CountDownLatch;
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
}
