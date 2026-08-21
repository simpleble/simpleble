package org.simpleble.android

enum class LocalCharacteristicCapability(internal val bit: Int) {
    Read(1),
    WriteRequest(2),
    WriteCommand(4),
    Notify(8),
    Indicate(16)
}
