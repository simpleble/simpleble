package org.simplektble.examples.listbackends

import org.simplektble.Backend
import org.simplektble.examples.describe

fun main() {
    for (backend in Backend.getBackends()) {
        println("Backend: ${backend.identifier}")
        for (adapter in backend.getAdapters()) {
            println("    Adapter: ${adapter.describe()}")
        }
    }
}
