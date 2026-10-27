package org.simplektble

public enum class CharacteristicCapability(internal val bit: Int) {
    Read(1),
    WriteRequest(2),
    WriteCommand(4),
    Notify(8),
    Indicate(16)
}
