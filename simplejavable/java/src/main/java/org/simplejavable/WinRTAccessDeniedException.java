package org.simplejavable;

/**
 * Windows denied access to the operation.
 */
public class WinRTAccessDeniedException extends WinRTException {
    public WinRTAccessDeniedException(String message) {
        super(ErrorCode.WINRT_ACCESS_DENIED, message);
    }
}
