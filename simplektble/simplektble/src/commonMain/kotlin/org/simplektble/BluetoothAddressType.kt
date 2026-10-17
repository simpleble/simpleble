package org.simplektble

public enum class BluetoothAddressType(public val value: Int) {
    PUBLIC(0),
    RANDOM(1),
    UNSPECIFIED(2);

    override fun toString(): String {
        return when (this) {
            PUBLIC -> "Public"
            RANDOM -> "Random"
            UNSPECIFIED -> "Unspecified"
        }
    }

    public companion object {
        public fun fromInt(value: Int): BluetoothAddressType {
            return entries.find { it.value == value } ?: UNSPECIFIED
        }
    }
}
