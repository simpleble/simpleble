package org.simplejavable;

/**
 * Windows reported a failure.
 */
public class WinRTException extends SimpleJavaBleException {
    public WinRTException(String message) {
        super(ErrorCode.WINRT_EXCEPTION, message);
    }

    protected WinRTException(ErrorCode code, String message) {
        super(code, message);
    }
}
