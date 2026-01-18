import { StyleSheet, Text, View, TouchableOpacity, ScrollView, ActivityIndicator } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { useState, useEffect, useRef } from 'react';
import { HybridAdapter, type Adapter, type Peripheral } from 'simplejsble';

export default function HomeScreen() {
  const [adapter, setAdapter] = useState<Adapter | null>(null);
  const [isBluetoothEnabled, setIsBluetoothEnabled] = useState<boolean>(false);
  const [scanning, setScanning] = useState<boolean>(false);
  const [peripherals, setPeripherals] = useState<Peripheral[]>([]);
  const [connectedPeripheral, setConnectedPeripheral] = useState<Peripheral | null>(null);
  const [mtu, setMtu] = useState<number>(0);
  const [statusMessage, setStatusMessage] = useState<string>('Initializing...');
  const [connectionVerified, setConnectionVerified] = useState<boolean>(false);
  const [connectionDetails, setConnectionDetails] = useState<{
    isConnected: boolean;
    rssi: number;
    addressType: string;
    isConnectable: boolean;
  } | null>(null);
  
  const adapterRef = useRef<Adapter | null>(null);

  useEffect(() => {
    const initAdapter = () => {
      try {
        const isEnabled = HybridAdapter.bluetooth_enabled();
        setIsBluetoothEnabled(isEnabled);
        
        if (!isEnabled) {
          setStatusMessage('Bluetooth is disabled. Please enable Bluetooth.');
          return;
        }

        const adapters = HybridAdapter.get_adapters();
        if (adapters.length === 0) {
          setStatusMessage('No Bluetooth adapters found.');
          return;
        }

        const firstAdapter = adapters[0] as Adapter;
        adapterRef.current = firstAdapter;
        setAdapter(firstAdapter);
        setStatusMessage('Ready to scan. Press "Scan for Devices" to start.');
      } catch (error) {
        console.error('Error initializing adapter:', error);
        setStatusMessage('Error initializing Bluetooth adapter.');
      }
    };

    initAdapter();
  }, []);

  const startScan = () => {
    if (!adapter || scanning) return;

    setPeripherals([]);
    setStatusMessage('Starting scan...');

    try {
      adapter.set_callback_on_scan_start(() => {
        console.log('Scan started.');
        setStatusMessage('Scanning...');
      });

      adapter.set_callback_on_scan_stop(() => {
        console.log('Scan stopped.');
        setScanning(false);
        const results = adapter.scan_get_results();
        const connectableCount = results.filter((p: Peripheral) => p.is_connectable()).length;
        setStatusMessage(`Scan complete. Found ${connectableCount} connectable device${connectableCount !== 1 ? 's' : ''}.`);
      });

      adapter.set_callback_on_scan_found((peripheral: Peripheral) => {
        console.log(`Found device: ${peripheral.identifier()} [${peripheral.address()}]`);
        if (peripheral.is_connectable()) {
          setPeripherals(prev => {
            const exists = prev.some(p => p.address() === peripheral.address());
            if (!exists) {
              return [...prev, peripheral];
            }
            return prev;
          });
        }
      });

      adapter.scan_start();
      
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

  const connectToDevice = (peripheral: Peripheral) => {
    if (connectedPeripheral) {
      setStatusMessage('Please disconnect from current device first.');
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

      peripheral.connect();
    } catch (error) {
      console.error('Error connecting to device:', error);
      setStatusMessage('Error connecting to device.');
    }
  };

  const verifyConnection = (peripheral: Peripheral) => {
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

  const disconnect = () => {
    if (!connectedPeripheral) return;

    try {
      connectedPeripheral.disconnect();
    } catch (error) {
      console.error('Error disconnecting:', error);
      setStatusMessage('Error disconnecting.');
    }
  };

  return (
    <SafeAreaView style={styles.safeArea}>
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

        {!isBluetoothEnabled && (
          <View style={styles.warningContainer}>
            <Text style={styles.warningText}>⚠️ Bluetooth is disabled</Text>
          </View>
        )}

        {scanning && (
          <View style={styles.loadingContainer}>
            <ActivityIndicator size="large" color="#007AFF" />
            <Text style={styles.loadingText}>Scanning for devices...</Text>
            <Text style={styles.loadingSubtext}>Please wait, this may take a few seconds</Text>
          </View>
        )}

        {adapter && (
          <>

            {!connectedPeripheral && (
              <TouchableOpacity
                style={[styles.button, scanning && styles.buttonDisabled]}
                onPress={startScan}
                disabled={scanning}
              >
                {scanning ? (
                  <View style={styles.buttonContent}>
                    <ActivityIndicator size="small" color="#fff" style={styles.spinner} />
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
                      Your iPhone is connected to {connectedPeripheral.identifier()}.
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
                  Connectable Devices ({peripherals.length}):
                </Text>
                <ScrollView 
                  style={styles.devicesList}
                  nestedScrollEnabled={true}
                  showsVerticalScrollIndicator={true}
                >
                  {peripherals.map((peripheral, index) => (
                    <View key={`${peripheral.address()}-${index}`} style={styles.deviceItem}>
                      <View style={styles.deviceInfoContainer}>
                        <Text style={styles.deviceName}>
                          [{index}] {peripheral.identifier()}
                        </Text>
                        <Text style={styles.deviceAddress}>
                          {peripheral.address()}
                        </Text>
                      </View>
                      <TouchableOpacity
                        style={styles.connectButton}
                        onPress={() => connectToDevice(peripheral)}
                      >
                        <Text style={styles.connectButtonText}>Connect</Text>
                      </TouchableOpacity>
                    </View>
                  ))}
                </ScrollView>
              </View>
            )}
          </>
        )}
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safeArea: {
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
    backgroundColor: '#fff',
    padding: 15,
    borderRadius: 8,
    marginBottom: 20,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
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
    backgroundColor: '#ccc',
  },
  buttonContent: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  spinner: {
    marginRight: 10,
  },
  buttonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  connectedContainer: {
    backgroundColor: '#fff',
    padding: 20,
    borderRadius: 8,
    marginBottom: 15,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
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
    backgroundColor: '#fff',
    borderRadius: 8,
    padding: 15,
    marginBottom: 15,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
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
  deviceInfoContainer: {
    flex: 1,
    marginRight: 10,
  },
  deviceName: {
    fontSize: 16,
    fontWeight: '600',
    color: '#333',
    marginBottom: 5,
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
  connectButtonText: {
    color: '#fff',
    fontSize: 14,
    fontWeight: '600',
  },
  loadingContainer: {
    backgroundColor: '#fff',
    padding: 30,
    borderRadius: 12,
    marginBottom: 20,
    alignItems: 'center',
    justifyContent: 'center',
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
    borderWidth: 2,
    borderColor: '#007AFF',
    borderStyle: 'dashed',
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
