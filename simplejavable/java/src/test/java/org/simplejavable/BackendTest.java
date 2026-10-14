package org.simplejavable;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.Test;

class BackendTest {
    @Test
    void namedBackends() {
        Backend backend = Backend.getNative();
        assertEquals("Plain", backend.getIdentifier());
        assertSame(Backend.getBackends().get(0), backend);

        assertThrows(UnsupportedOperationException.class, Backend::getDongl);
        assertThrows(UnsupportedOperationException.class, Backend::getSimulator);
    }
}
