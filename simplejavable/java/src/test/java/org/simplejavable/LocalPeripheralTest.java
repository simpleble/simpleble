package org.simplejavable;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.simplejavable.LocalCharacteristicCapability.NOTIFY;
import static org.simplejavable.LocalCharacteristicCapability.READ;
import static org.simplejavable.LocalCharacteristicCapability.WRITE_REQUEST;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import org.junit.jupiter.api.Test;

class LocalPeripheralTest {
    private static final BluetoothUUID SERVICE = uuid("aa00");
    private static final BluetoothUUID READABLE = uuid("aa01");
    private static final BluetoothUUID ECHO = uuid("aa02");

    @Test
    void servesAClientInTheSameProcess() throws Exception {
        Adapter adapter = TestSupport.plainAdapter();
        LocalPeripheral host = adapter.createLocalPeripheral();
        Peripheral client = null;

        try {
            host.addAdvertisedService(SERVICE);
            LocalService service = host.addService(SERVICE);

            LocalCharacteristic readable = service.addCharacteristic(READABLE, READ);
            readable.setValue(new byte[] {0x00, (byte) 0xff});

            LocalCharacteristic echo = service.addCharacteristic(ECHO, WRITE_REQUEST, NOTIFY);
            BlockingQueue<byte[]> writes = new LinkedBlockingQueue<>();
            CountDownLatch subscribed = new CountDownLatch(1);
            echo.setEventListener(new LocalCharacteristic.EventListener() {
                @Override
                public void onWrite(byte[] value) {
                    writes.add(value);
                }

                @Override
                public void onSubscribed() {
                    subscribed.countDown();
                }
            });

            BlockingQueue<String> clients = new LinkedBlockingQueue<>();
            host.setEventListener(new LocalPeripheral.EventListener() {
                @Override
                public void onClientConnected(BluetoothAddress address) {
                    clients.add("connected");
                }

                @Override
                public void onClientDisconnected(BluetoothAddress address) {
                    clients.add("disconnected");
                }
            });

            host.start();
            client = TestSupport.scanFor(adapter, "Plain Adapter Peripheral");
            client.connect();
            assertEquals("connected", clients.poll(TestSupport.TIMEOUT_SECONDS, TimeUnit.SECONDS));

            assertArrayEquals(new byte[] {0x00, (byte) 0xff}, client.read(SERVICE, READABLE));
            readable.setReadHandler(() -> new byte[] {0x01});
            assertArrayEquals(new byte[] {0x01}, client.read(SERVICE, READABLE));

            // A handler that throws serves the stored value instead.
            readable.setReadHandler(() -> {
                throw new IllegalStateException("Expected read handler failure");
            });
            assertArrayEquals(new byte[] {0x00, (byte) 0xff}, client.read(SERVICE, READABLE));

            client.writeRequest(SERVICE, ECHO, new byte[] {1, 2, 3});
            assertArrayEquals(new byte[] {1, 2, 3}, writes.poll(TestSupport.TIMEOUT_SECONDS, TimeUnit.SECONDS));

            BlockingQueue<byte[]> notifications = new LinkedBlockingQueue<>();
            client.notify(SERVICE, ECHO, notifications::add);
            assertTrue(subscribed.await(TestSupport.TIMEOUT_SECONDS, TimeUnit.SECONDS));
            echo.setValue(new byte[] {4, 5});
            assertArrayEquals(new byte[] {4, 5}, notifications.poll(TestSupport.TIMEOUT_SECONDS, TimeUnit.SECONDS));

            client.disconnect();
            assertEquals("disconnected", clients.poll(TestSupport.TIMEOUT_SECONDS, TimeUnit.SECONDS));
        } finally {
            if (client != null && client.isConnected()) {
                client.disconnect();
            }
            host.stop();
            host.setEventListener(null);
        }
    }

    @Test
    void advertisesManufacturerDataWithoutConnections() throws Exception {
        Adapter adapter = TestSupport.plainAdapter();
        LocalPeripheral host = adapter.createLocalPeripheral();

        try {
            host.setAdvertisedManufacturerData(0x004c, new byte[] {0x02});
            host.setAdvertisedManufacturerData(0x004c, new byte[] {0x02, 0x15});
            host.setConnectable(false);
            host.start();

            Peripheral beacon = TestSupport.scanFor(adapter, "Plain Adapter Peripheral");
            assertArrayEquals(new byte[] {0x02, 0x15}, beacon.manufacturerData().get(0x004c));
            assertFalse(beacon.isConnectable());
        } finally {
            host.stop();
        }
    }

    private static BluetoothUUID uuid(String shortUuid) {
        return new BluetoothUUID("0000" + shortUuid + "-0000-1000-8000-00805f9b34fb");
    }
}
