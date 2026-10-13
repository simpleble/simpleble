package org.simplejavable.examples.peripheral;

import org.simplejavable.Adapter;
import org.simplejavable.BluetoothAddress;
import org.simplejavable.BluetoothUUID;
import org.simplejavable.LocalCharacteristic;
import org.simplejavable.LocalCharacteristicCapability;
import org.simplejavable.LocalPeripheral;
import org.simplejavable.LocalService;

import java.nio.charset.StandardCharsets;
import java.util.HexFormat;
import java.util.List;
import java.util.concurrent.CountDownLatch;

public class PeripheralExample {
    private static final BluetoothUUID SERVICE_UUID = new BluetoothUUID("12345678-1234-5678-1234-56789abcdef0");
    private static final BluetoothUUID CHARACTERISTIC_UUID = new BluetoothUUID("12345678-1234-5678-1234-56789abcdef1");

    public static void main(String[] args) throws InterruptedException {
        List<Adapter> adapters = Adapter.getAdapters();
        if (adapters.isEmpty()) {
            System.err.println("No Bluetooth adapters found.");
            System.exit(1);
            return;
        }

        LocalPeripheral peripheral = adapters.get(0).createLocalPeripheral();

        // Add a service UUID to the advertisement.
        peripheral.addAdvertisedService(SERVICE_UUID);

        LocalService service = peripheral.addService(SERVICE_UUID);
        LocalCharacteristic characteristic = service.addCharacteristic(
            CHARACTERISTIC_UUID,
            LocalCharacteristicCapability.READ, LocalCharacteristicCapability.WRITE_REQUEST,
            LocalCharacteristicCapability.WRITE_COMMAND, LocalCharacteristicCapability.NOTIFY,
            LocalCharacteristicCapability.INDICATE);
        characteristic.setValue("ready".getBytes(StandardCharsets.UTF_8));

        characteristic.setEventListener(new LocalCharacteristic.EventListener() {
            @Override
            public void onWrite(byte[] value) {
                System.out.println("Write: " + HexFormat.of().formatHex(value));
                // Echo the written value to subscribed clients.
                characteristic.setValue(value);
            }

            @Override
            public void onSubscribed() {
                System.out.println("Client subscribed.");
            }

            @Override
            public void onUnsubscribed() {
                System.out.println("Client unsubscribed.");
            }
        });

        peripheral.setEventListener(new LocalPeripheral.EventListener() {
            @Override
            public void onClientConnected(BluetoothAddress address) {
                System.out.println("Client connected: " + address);
            }

            @Override
            public void onClientDisconnected(BluetoothAddress address) {
                System.out.println("Client disconnected: " + address);
            }
        });

        // Ctrl+C runs this hook, which stops advertising before the process exits.
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            characteristic.setEventListener(null);
            peripheral.stop();
        }));

        peripheral.start();
        System.out.println("Advertising. Press Ctrl+C to stop.");

        new CountDownLatch(1).await();
    }
}
