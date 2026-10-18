package org.simplektble

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.IO
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.withContext

/**
 * Runs a native call on [Dispatchers.IO] to the end, even in a cancelled coroutine, and returns normally there. Calls
 * that end something, such as a scan or a connection, use it so that several can follow each other in a finally block.
 */
internal suspend fun <T> runToEnd(block: () -> T): T {
    // NOTE: withContext(NonCancellable + Dispatchers.IO) throws on return in a cancelled coroutine, because it changes
    // the dispatcher. The outer withContext keeps the dispatcher, so it returns normally.
    return withContext(NonCancellable) { withContext(Dispatchers.IO) { block() } }
}
