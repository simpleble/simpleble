package org.simplejavable.examples.dongl;

import org.simplejavable.Adapter;
import org.simplejavable.Advanced;
import org.simplejavable.Backend;
import org.simplejavable.Peripheral;
import org.simplejavable.Service;
import org.simplejavable.examples.common.Utils;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.Scanner;

public class DonglExample {
    public static void main(String[] args) {
        List<Adapter> dongls = Backend.getDongl().getAdapters();
        if (dongls.isEmpty()) {
            System.err.println("No Dongl found.");
            System.exit(1);
            return;
        }

        Adapter adapter = dongls.get(0);
        System.out.println("Using " + adapter.getIdentifier() + " [" + adapter.getAddress() + "]");

        List<Peripheral> peripherals = new ArrayList<>();
        adapter.setEventListener(new Adapter.EventListener() {
            @Override
            public void onScanFound(Peripheral peripheral) {
                System.out.println("Found device: " + peripheral.getIdentifier() + " [" + peripheral.getAddress() + "]");
                if (peripheral.isConnectable()) {
                    peripherals.add(peripheral);
                }
            }
        });

        try {
            adapter.scanFor(5000);
        } catch (Exception e) {
            System.err.println("Scan failed: " + e.getMessage());
            System.exit(1);
            return;
        }

        System.out.println("The following connectable devices were found:");
        for (int i = 0; i < peripherals.size(); i++) {
            Peripheral peripheral = peripherals.get(i);
            System.out.println("[" + i + "] " + peripheral.getIdentifier() + " [" + peripheral.getAddress() + "]");
        }

        Optional<Integer> selection = Utils.getUserInputInt("Please select a device to connect to", peripherals.size() - 1);
        if (selection.isEmpty()) {
            System.exit(1);
            return;
        }

        Peripheral peripheral = peripherals.get(selection.get());

        // Register the pairing callbacks before connecting, since pairing can start as soon as the link is up.
        // They run on a worker thread while connect() blocks, so they can wait for the user's answer.
        Scanner input = new Scanner(System.in);
        Advanced.Dongl.setNumericComparisonCallback(peripheral, passkey -> {
            System.out.print("Does the device show " + passkey + "? [y/N] ");
            return input.nextLine().trim().equals("y");
        });
        Advanced.Dongl.setPasskeyRequestCallback(peripheral, () -> {
            System.out.print("Enter the passkey shown by the device: ");
            return input.nextLine().trim();
        });
        Advanced.Dongl.setPasskeyDisplayCallback(peripheral,
            passkey -> System.out.println("Enter " + passkey + " on the device."));

        System.out.println("Connecting to " + peripheral.getIdentifier() + " [" + peripheral.getAddress() + "]");
        peripheral.connect();
        System.out.println("Connected. Paired: " + peripheral.isPaired());

        for (Service service : peripheral.services()) {
            System.out.println("Service: " + service.uuid());
        }

        peripheral.disconnect();
        System.exit(0);
    }
}
