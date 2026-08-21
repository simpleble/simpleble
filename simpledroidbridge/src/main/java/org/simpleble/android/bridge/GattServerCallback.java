package org.simpleble.android.bridge;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattServerCallback;
import android.bluetooth.BluetoothGattService;

public final class GattServerCallback extends BluetoothGattServerCallback {
    public GattServerCallback() {}

    @Override
    public void onConnectionStateChange(BluetoothDevice device, int status, int newState) {
        onConnectionStateChangeCallback(device, status, newState);
    }

    @Override
    public void onServiceAdded(int status, BluetoothGattService service) {
        onServiceAddedCallback(status, service);
    }

    @Override
    public void onCharacteristicReadRequest(
            BluetoothDevice device,
            int requestId,
            int offset,
            BluetoothGattCharacteristic characteristic) {
        onCharacteristicReadRequestCallback(device, requestId, offset, characteristic);
    }

    @Override
    public void onCharacteristicWriteRequest(
            BluetoothDevice device,
            int requestId,
            BluetoothGattCharacteristic characteristic,
            boolean preparedWrite,
            boolean responseNeeded,
            int offset,
            byte[] value) {
        onCharacteristicWriteRequestCallback(
                device, requestId, characteristic, preparedWrite, responseNeeded, offset, value);
    }

    @Override
    public void onDescriptorReadRequest(
            BluetoothDevice device,
            int requestId,
            int offset,
            BluetoothGattDescriptor descriptor) {
        onDescriptorReadRequestCallback(device, requestId, offset, descriptor);
    }

    @Override
    public void onDescriptorWriteRequest(
            BluetoothDevice device,
            int requestId,
            BluetoothGattDescriptor descriptor,
            boolean preparedWrite,
            boolean responseNeeded,
            int offset,
            byte[] value) {
        onDescriptorWriteRequestCallback(
                device, requestId, descriptor, preparedWrite, responseNeeded, offset, value);
    }

    @Override
    public void onExecuteWrite(BluetoothDevice device, int requestId, boolean execute) {
        onExecuteWriteCallback(device, requestId, execute);
    }

    @Override
    public void onNotificationSent(BluetoothDevice device, int status) {
        onNotificationSentCallback(device, status);
    }

    private native void onConnectionStateChangeCallback(BluetoothDevice device, int status, int newState);
    private native void onServiceAddedCallback(int status, BluetoothGattService service);
    private native void onCharacteristicReadRequestCallback(
            BluetoothDevice device, int requestId, int offset, BluetoothGattCharacteristic characteristic);
    private native void onCharacteristicWriteRequestCallback(
            BluetoothDevice device,
            int requestId,
            BluetoothGattCharacteristic characteristic,
            boolean preparedWrite,
            boolean responseNeeded,
            int offset,
            byte[] value);
    private native void onDescriptorReadRequestCallback(
            BluetoothDevice device, int requestId, int offset, BluetoothGattDescriptor descriptor);
    private native void onDescriptorWriteRequestCallback(
            BluetoothDevice device,
            int requestId,
            BluetoothGattDescriptor descriptor,
            boolean preparedWrite,
            boolean responseNeeded,
            int offset,
            byte[] value);
    private native void onExecuteWriteCallback(BluetoothDevice device, int requestId, boolean execute);
    private native void onNotificationSentCallback(BluetoothDevice device, int status);
}
