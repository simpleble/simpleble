package org.simplejavable;

/**
 * A failure reported by SimpleBLE.
 * <p>
 * Each kind of failure has its own subclass, so it can be caught separately, and {@link #getCode()} identifies it.
 * Failures without a dedicated subclass are thrown as this class with {@link ErrorCode#UNCLASSIFIED_EXCEPTION}.
 */
public class SimpleJavaBleException extends RuntimeException {
    private final ErrorCode code;

    public SimpleJavaBleException(String message) {
        this(ErrorCode.UNCLASSIFIED_EXCEPTION, message);
    }

    protected SimpleJavaBleException(ErrorCode code, String message) {
        super(message);
        this.code = code;
    }

    /**
     * Gets SimpleBLE's classification of the failure.
     * @return The error code.
     */
    public ErrorCode getCode() {
        return code;
    }
}
