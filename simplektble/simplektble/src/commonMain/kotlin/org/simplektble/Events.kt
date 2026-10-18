package org.simplektble

import kotlinx.coroutines.flow.MutableSharedFlow

/**
 * A flow for native events. Every event reaches each active collector, in the order it happened: when a collector
 * falls behind, its buffer grows instead of dropping events.
 */
internal fun <T> eventFlow(replay: Int = 0): MutableSharedFlow<T> {
    return MutableSharedFlow(replay = replay, extraBufferCapacity = Int.MAX_VALUE)
}
