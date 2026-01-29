import { useEffect, useState } from 'react';
import { ActivityIndicator, ScrollView, StyleSheet, Text, TouchableOpacity, View } from 'react-native';
import { Adapter, Peripheral } from 'simplernble';

export default function ConnectExample() {
  const [adapter, setAdapter] = useState<typeof Adapter | null>(null);
  const [isBluetoothEnabled, setIsBluetoothEnabled] = useState<boolean>(false);
  const [isInitializing, setIsInitializing] = useState<boolean>(true);
  const [scanning, setScanning] = useState<boolean>(false);
  const [peripherals, setPeripherals] = useState<typeof Peripheral[]>([]);
  const [connectedPeripheral, setConnectedPeripheral] = useState<typeof Peripheral | null>(null);
  const [mtu, setMtu] = useState<number>(0);
  const [statusMessage, setStatusMessage] = useState<string>('Initializing Bluetooth...');
  const [connectionVerified, setConnectionVerified] = useState<boolean>(false);
  const [connectionDetails, setConnectionDetails] = useState<{
    isConnected: boolean;
    rssi: number;
    addressType: string;
    isConnectable: boolean;
  } | null>(null);

  useEffect(() => {
    const initAdapter = async () => {
      try {
        setIsInitializing(true);
        
        const isEnabled = await Adapter.bluetooth_enabled();
        setIsBluetoothEnabled(isEnabled);
        
        if (!isEnabled) {
          setStatusMessage('Bluetooth is disabled. Please enable Bluetooth.');
          setIsInitializing(false);
          return;
        }

        const adapters = await Adapter.get_adapters();
        if (adapters.length === 0) {
          setStatusMessage('No Bluetooth adapters found.');
          setIsInitializing(false);
          return;
        }

        const firstAdapter = adapters[0] as typeof Adapter;
        setAdapter(firstAdapter);
        setStatusMessage('Ready to scan. Press "Scan for Devices" to start.');
      } catch (error) {
        console.error('Error initializing adapter:', error);
        setStatusMessage('Error initializing Bluetooth adapter.');
      } finally {
        setIsInitializing(false);
      }
    };

    initAdapter();
  }, []);

  const startScan = () => {
    if (!adapter || scanning) return;

    setPeripherals([]);
    setStatusMessage('Starting scan...');
    setScanning(true);

    try {
      adapter.set_callback_on_scan_start(() => {
        console.log('Scan started.');
        setStatusMessage('Scanning...');
      });

      adapter.set_callback_on_scan_stop(() => {
        console.log('Scan stopped.');
        setScanning(false);
        const results = adapter.scan_get_results();
        const totalCount = results.length;
        const connectableCount = results.filter((p: typeof Peripheral) => p.is_connectable()).length;
        console.log(`Scan results: ${totalCount} total devices, ${connectableCount} connectable`);
        setStatusMessage(`Scan complete. Found ${totalCount} device${totalCount !== 1 ? 's' : ''} (${connectableCount} connectable).`);
      });

      adapter.set_callback_on_scan_found((peripheral: typeof Peripheral) => {
        // Log all discovered devices for debugging
        const isConnectable = peripheral.is_connectable();
        const identifier = peripheral.identifier() || 'Unknown';
        const address = peripheral.address();
        console.log(`Found device: ${identifier} [${address}] - Connectable: ${isConnectable}`);
        
        // Add all devices to the list, not just connectable ones
        setPeripherals(prev => {
          const exists = prev.some(p => p.address() === peripheral.address());
          if (!exists) {
            return [...prev, peripheral];
          }
          return prev;
        });
      });

      setTimeout(() => {
        adapter.scan_start();
      }, 0);
      
      setTimeout(() => {
        if (adapter.scan_is_active()) {
          adapter.scan_stop();
        }
      }, 5000);
    } catch (error) {
      console.error('Error during scan:', error);
      setScanning(false);
      setStatusMessage('Error during scan.');
    }
  };

  const connectToDevice = async (peripheral: typeof Peripheral) => {
    if (connectedPeripheral) {
      setStatusMessage('Please disconnect from current device first.');
      return;
    }

    if (!peripheral.is_connectable()) {
      setStatusMessage('This device is not connectable.');
      return;
    }

    setStatusMessage(`Connecting to ${peripheral.identifier()}...`);

    try {
      peripheral.set_callback_on_connected(() => {
        console.log('Successfully connected.');
        setConnectedPeripheral(peripheral);
        
        setTimeout(() => {
          verifyConnection(peripheral);
        }, 500);
      });

      peripheral.set_callback_on_disconnected(() => {
        console.log('Disconnected.');
        setConnectedPeripheral(null);
        setMtu(0);
        setConnectionVerified(false);
        setConnectionDetails(null);
        setStatusMessage('Disconnected.');
      });

      await peripheral.connect();
    } catch (error) {
      console.error('Error connecting to device:', error);
      setStatusMessage('Error connecting to device.');
    }
  };

  const verifyConnection = (peripheral: typeof Peripheral) => {
    try {
      const isConnected = peripheral.is_connected();
      const currentMtu = peripheral.mtu();
      const rssi = peripheral.rssi();
      const addressType = peripheral.address_type();
      const isConnectable = peripheral.is_connectable();

      setMtu(currentMtu);
      setConnectionVerified(isConnected);
      setConnectionDetails({
        isConnected,
        rssi,
        addressType,
        isConnectable,
      });

      if (isConnected) {
        setStatusMessage(`✅ Connected to ${peripheral.identifier()}. MTU: ${currentMtu}`);
      } else {
        setStatusMessage(`❌ Connection verification failed`);
      }
    } catch (error) {
      console.error('Error verifying connection:', error);
      setConnectionVerified(false);
      setStatusMessage('Error verifying connection.');
    }
  };

  const disconnect = async () => {
    if (!connectedPeripheral) return;

    try {
      await connectedPeripheral.disconnect();
    } catch (error) {
      console.error('Error disconnecting:', error);
      setStatusMessage('Error disconnecting.');
    }
  };

  return (
    <View style={styles.root}>
      <ScrollView 
        style={styles.scrollView}
        contentContainerStyle={styles.scrollContent}
        showsVerticalScrollIndicator={true}
      >
        <Text style={styles.title}>BLE Connect Example</Text>
        
        <View style={styles.statusContainer}>
          <Text style={styles.statusLabel}>Status:</Text>
          <Text style={styles.statusText}>{statusMessage}</Text>
        </View>

        {isInitializing && (
          <View style={styles.loadingContainer}>
            <ActivityIndicator size="large" color="#007AFF" />
            <Text style={styles.loadingText}>Initializing Bluetooth...</Text>
            <Text style={styles.loadingSubtext}>Please wait while we set up the Bluetooth adapter</Text>
          </View>
        )}

        {!isInitializing && !isBluetoothEnabled && (
          <View style={styles.warningContainer}>
            <Text style={styles.warningText}>⚠️ Bluetooth is disabled</Text>
          </View>
        )}

        {!isInitializing && scanning && (
          <View style={styles.loadingContainer}>
            <ActivityIndicator size="large" color="#007AFF" />
            <Text style={styles.loadingText}>Scanning for devices...</Text>
            <Text style={styles.loadingSubtext}>Please wait, this may take a few seconds</Text>
          </View>
        )}

        {!isInitializing && adapter && (
          <>
            {!connectedPeripheral && (
              <TouchableOpacity
                style={[styles.button, scanning && styles.buttonDisabled].filter(Boolean)}
                onPress={startScan}
                disabled={scanning}
              >
                {scanning ? (
                  <View style={styles.buttonContent}>
                    <ActivityIndicator size="small" color="#ffffff" style={{ marginRight: 10 }} />
                    <Text style={styles.buttonText}>Scanning...</Text>
                  </View>
                ) : (
                  <Text style={styles.buttonText}>Scan for Devices</Text>
                )}
              </TouchableOpacity>
            )}

            {connectedPeripheral && (
              <View style={styles.connectedContainer}>
                <View style={styles.connectionHeader}>
                  <View style={[
                    styles.connectionIndicator,
                    connectionVerified ? styles.connectionIndicatorActive : styles.connectionIndicatorInactive
                  ]}>
                    <Text style={styles.connectionIndicatorText}>
                      {connectionVerified ? '🟢' : '🟡'}
                    </Text>
                  </View>
                  <Text style={styles.connectedTitle}>
                    {connectionVerified ? 'Connected & Verified' : 'Connecting...'}
                  </Text>
                </View>

                <View style={styles.deviceDetailsSection}>
                  <Text style={styles.sectionTitle}>Device Information</Text>
                  <Text style={styles.deviceInfo}>
                    <Text style={styles.deviceInfoLabel}>Name:</Text> {connectedPeripheral.identifier()}
                  </Text>
                  <Text style={styles.deviceInfo}>
                    <Text style={styles.deviceInfoLabel}>Address:</Text> {connectedPeripheral.address()}
                  </Text>
                  {connectionDetails && (
                    <>
                      <Text style={styles.deviceInfo}>
                        <Text style={styles.deviceInfoLabel}>Address Type:</Text> {connectionDetails.addressType}
                      </Text>
                      <Text style={styles.deviceInfo}>
                        <Text style={styles.deviceInfoLabel}>RSSI:</Text> {connectionDetails.rssi} dBm
                      </Text>
                      <Text style={styles.deviceInfo}>
                        <Text style={styles.deviceInfoLabel}>Connectable:</Text> {connectionDetails.isConnectable ? 'Yes' : 'No'}
                      </Text>
                    </>
                  )}
                  <Text style={styles.deviceInfo}>
                    <Text style={styles.deviceInfoLabel}>MTU:</Text> {mtu} bytes
                  </Text>
                  <Text style={styles.deviceInfo}>
                    <Text style={styles.deviceInfoLabel}>Connection Status:</Text>{' '}
                    <Text style={[
                      styles.statusBadge,
                      connectionVerified ? styles.statusBadgeSuccess : styles.statusBadgeWarning
                    ]}>
                      {connectionVerified ? 'VERIFIED ✓' : 'PENDING...'}
                    </Text>
                  </Text>
                </View>

                {connectionVerified && (
                  <View style={styles.verificationSection}>
                    <Text style={styles.verificationTitle}>✅ Connection Verified</Text>
                    <Text style={styles.verificationText}>
                      Your device is connected to {connectedPeripheral.identifier()}.
                    </Text>
                  </View>
                )}

                <View style={styles.buttonRow}>
                  <TouchableOpacity
                    style={[styles.button, styles.verifyButton, { marginRight: 5, marginBottom: 0 }]}
                    onPress={() => verifyConnection(connectedPeripheral)}
                  >
                    <Text style={styles.buttonText}>🔄 Verify</Text>
                  </TouchableOpacity>
                  <TouchableOpacity
                    style={[styles.button, styles.disconnectButton, { marginLeft: 5, marginBottom: 0 }]}
                    onPress={disconnect}
                  >
                    <Text style={styles.buttonText}>Disconnect</Text>
                  </TouchableOpacity>
                </View>
              </View>
            )}

            {!connectedPeripheral && peripherals.length > 0 && (
              <View style={styles.devicesContainer}>
                <Text style={styles.devicesTitle}>
                  Discovered Devices ({peripherals.length}):
                </Text>
                <ScrollView 
                  style={styles.devicesList}
                  nestedScrollEnabled={true}
                  showsVerticalScrollIndicator={true}
                >
                  {peripherals.map((peripheral, index) => {
                    const isConnectable = peripheral.is_connectable();
                    const identifier = peripheral.identifier() || 'Unknown';
                    return (
                      <View 
                        key={`${peripheral.address()}-${index}`} 
                        style={[
                          styles.deviceItem,
                          !isConnectable && styles.deviceItemNonConnectable
                        ]}
                      >
                        <View style={styles.deviceInfoContainer}>
                          <View style={styles.deviceNameRow}>
                            <Text style={styles.deviceName}>
                              [{index}] {identifier}
                            </Text>
                            {!isConnectable && (
                              <Text style={styles.nonConnectableBadge}>Not Connectable</Text>
                            )}
                          </View>
                          <Text style={styles.deviceAddress}>
                            {peripheral.address()}
                          </Text>
                        </View>
                        <TouchableOpacity
                          style={[
                            styles.connectButton,
                            !isConnectable && styles.connectButtonDisabled
                          ].filter(Boolean)}
                          onPress={() => connectToDevice(peripheral)}
                          disabled={!isConnectable}
                        >
                          <Text style={styles.connectButtonText}>
                            {isConnectable ? 'Connect' : 'N/A'}
                          </Text>
                        </TouchableOpacity>
                      </View>
                    );
                  })}
                </ScrollView>
              </View>
            )}
          </>
        )}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  root: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  scrollView: {
    flex: 1,
  },
  scrollContent: {
    padding: 20,
    paddingBottom: 40,
  },
  container: {
    flex: 1,
    padding: 20,
    backgroundColor: '#f5f5f5',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 20,
    textAlign: 'center',
    color: '#333',
  },
  statusContainer: {
    backgroundColor: '#ffffff',
    padding: 15,
    borderRadius: 8,
    marginBottom: 20,
    borderWidth: 1,
    borderColor: '#e0e0e0',
  },
  statusLabel: {
    fontSize: 14,
    fontWeight: '600',
    color: '#666',
    marginBottom: 5,
  },
  statusText: {
    fontSize: 16,
    color: '#333',
  },
  warningContainer: {
    backgroundColor: '#fff3cd',
    padding: 15,
    borderRadius: 8,
    marginBottom: 20,
    borderLeftWidth: 4,
    borderLeftColor: '#ffc107',
  },
  warningText: {
    fontSize: 16,
    color: '#856404',
  },
  button: {
    backgroundColor: '#007AFF',
    padding: 15,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 15,
    minHeight: 50,
    justifyContent: 'center',
  },
  buttonDisabled: {
    backgroundColor: '#cccccc',
  },
  buttonContent: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  spinner: {
    marginRight: 10,
  },
  buttonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '600',
  },
  connectedContainer: {
    backgroundColor: '#ffffff',
    padding: 20,
    borderRadius: 8,
    marginBottom: 15,
    borderWidth: 1,
    borderColor: '#e0e0e0',
  },
  connectionHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 20,
    paddingBottom: 15,
    borderBottomWidth: 1,
    borderBottomColor: '#e0e0e0',
  },
  connectionIndicator: {
    width: 40,
    height: 40,
    borderRadius: 20,
    justifyContent: 'center',
    alignItems: 'center',
    marginRight: 12,
  },
  connectionIndicatorActive: {
    backgroundColor: '#d4edda',
  },
  connectionIndicatorInactive: {
    backgroundColor: '#fff3cd',
  },
  connectionIndicatorText: {
    fontSize: 20,
  },
  connectedTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#333',
    flex: 1,
  },
  deviceDetailsSection: {
    marginBottom: 20,
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: '#333',
    marginBottom: 12,
  },
  deviceInfo: {
    fontSize: 15,
    marginBottom: 8,
    color: '#666',
    lineHeight: 22,
  },
  deviceInfoLabel: {
    fontWeight: '600',
    color: '#333',
  },
  statusBadge: {
    paddingHorizontal: 8,
    paddingVertical: 4,
    borderRadius: 4,
    fontSize: 12,
    fontWeight: '600',
    overflow: 'hidden',
  },
  statusBadgeSuccess: {
    backgroundColor: '#d4edda',
    color: '#155724',
  },
  statusBadgeWarning: {
    backgroundColor: '#fff3cd',
    color: '#856404',
  },
  verificationSection: {
    backgroundColor: '#e8f5e9',
    padding: 15,
    borderRadius: 8,
    marginTop: 15,
    borderLeftWidth: 4,
    borderLeftColor: '#4caf50',
  },
  verificationTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#2e7d32',
    marginBottom: 8,
  },
  verificationText: {
    fontSize: 14,
    color: '#333',
    marginBottom: 12,
  },
  verificationSubtext: {
    fontSize: 12,
    color: '#666',
    marginTop: 8,
    marginBottom: 4,
  },
  codeBlock: {
    backgroundColor: '#263238',
    padding: 12,
    borderRadius: 6,
    marginVertical: 8,
  },
  codeText: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#aed581',
  },
  buttonRow: {
    flexDirection: 'row',
    marginTop: 15,
    justifyContent: 'space-between',
    marginBottom: 0,
  },
  verifyButton: {
    backgroundColor: '#007AFF',
    flex: 1,
  },
  disconnectButton: {
    backgroundColor: '#FF3B30',
    flex: 1,
  },
  devicesContainer: {
    backgroundColor: '#ffffff',
    borderRadius: 8,
    padding: 15,
    marginBottom: 15,
    borderWidth: 1,
    borderColor: '#e0e0e0',
    maxHeight: 400,
  },
  devicesTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 15,
    color: '#333',
  },
  devicesList: {
    maxHeight: 350,
  },
  deviceItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 15,
    backgroundColor: '#f9f9f9',
    borderRadius: 8,
    marginBottom: 10,
    borderLeftWidth: 4,
    borderLeftColor: '#007AFF',
  },
  deviceItemNonConnectable: {
    borderLeftColor: '#999',
    opacity: 0.7,
  },
  deviceInfoContainer: {
    flex: 1,
    marginRight: 10,
  },
  deviceNameRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 5,
    flexWrap: 'wrap',
  },
  deviceName: {
    fontSize: 16,
    fontWeight: '600',
    color: '#333',
    marginRight: 8,
  },
  nonConnectableBadge: {
    fontSize: 11,
    fontWeight: '500',
    color: '#999',
    backgroundColor: '#e0e0e0',
    paddingHorizontal: 6,
    paddingVertical: 2,
    borderRadius: 4,
  },
  deviceAddress: {
    fontSize: 14,
    color: '#666',
  },
  connectButton: {
    backgroundColor: '#34C759',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 6,
  },
  connectButtonDisabled: {
    backgroundColor: '#cccccc',
  },
  connectButtonText: {
    color: '#ffffff',
    fontSize: 14,
    fontWeight: '600',
  },
  loadingContainer: {
    backgroundColor: '#ffffff',
    padding: 30,
    borderRadius: 12,
    marginBottom: 20,
    alignItems: 'center',
    justifyContent: 'center',
    borderWidth: 1,
    borderColor: '#e0e0e0',
  },
  loadingText: {
    fontSize: 18,
    fontWeight: '600',
    color: '#333',
    marginTop: 15,
    marginBottom: 5,
  },
  loadingSubtext: {
    fontSize: 14,
    color: '#666',
    textAlign: 'center',
  },
});
