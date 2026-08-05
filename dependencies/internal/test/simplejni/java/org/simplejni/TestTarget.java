package org.simplejni;

public final class TestTarget {
    private final int value;

    public TestTarget(int value) {
        this.value = value;
    }

    public int value() {
        return value;
    }

    public boolean truth() {
        return true;
    }

    public Object echoObject(Object object) {
        return object;
    }

    public String echoString(String string) {
        return string;
    }

    public byte[] echoBytes(byte[] bytes) {
        return bytes;
    }

    public void throwException() {
        throw new IllegalStateException("callback failed");
    }

    public void throwBrokenException() {
        throw new BrokenException();
    }

    private static final class BrokenException extends RuntimeException {
        @Override
        public String toString() {
            throw new IllegalStateException("toString failed");
        }
    }
}
