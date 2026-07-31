package org.simplejavable;

final class ThrowingCallback {
    void onDataReceived(byte[] data) {
        throw new IllegalStateException("callback failed");
    }
}
