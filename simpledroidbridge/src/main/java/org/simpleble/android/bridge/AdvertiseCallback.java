package org.simpleble.android.bridge;

import android.bluetooth.le.AdvertiseSettings;

public final class AdvertiseCallback extends android.bluetooth.le.AdvertiseCallback {
    public AdvertiseCallback() {}

    @Override
    public void onStartSuccess(AdvertiseSettings settingsInEffect) {
        onStartSuccessCallback();
    }

    @Override
    public void onStartFailure(int errorCode) {
        onStartFailureCallback(errorCode);
    }

    private native void onStartSuccessCallback();
    private native void onStartFailureCallback(int errorCode);
}
