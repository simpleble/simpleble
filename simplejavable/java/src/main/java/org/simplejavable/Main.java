package org.simplejavable;

public class Main {
    public static void main(String[] args) {
        // Check if Bluetooth is enabled
        // System.out.println("Bluetooth enabled: " + Adapter.isBluetoothEnabled());

        // Get list of adapters
        var adapterList = Adapter.getAdapters();

        if (adapterList.isEmpty()) {
            System.out.println("No adapter found");
            System.exit(1);
        }

        // Print information for each adapter
        for (var adapter : adapterList) {
            System.out.println("Adapter: " + adapter.getIdentifier() + " [" + adapter.getAddress() + "]");
        }
    }
}