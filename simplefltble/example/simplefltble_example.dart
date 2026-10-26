// A Flutter app that scans for five seconds and lists the peripherals it finds.
// The app needs the setup from SimpleFltBLE's usage guide: the Bluetooth
// permissions on Android, and NSBluetoothAlwaysUsageDescription on iOS and
// macOS. SimpleBLE Explorer also connects, reads, writes and subscribes:
// https://github.com/simpleble/simpleble/tree/main/examples/simplefltble
import 'package:flutter/material.dart';
import 'package:simplefltble/simplefltble.dart';

void main() => runApp(const MaterialApp(home: ScanPage()));

class ScanPage extends StatefulWidget {
  const ScanPage({super.key});

  @override
  State<ScanPage> createState() => _ScanPageState();
}

class _ScanPageState extends State<ScanPage> {
  final _found = <Peripheral>[];
  var _scanning = false;

  Future<void> _scan() async {
    setState(() {
      _found.clear();
      _scanning = true;
    });
    final adapter = (await Adapter.getAdapters()).first;
    final found = adapter.onScanFound.listen((peripheral) {
      if (mounted) setState(() => _found.add(peripheral));
    });
    await adapter.scanFor(const Duration(seconds: 5));
    await found.cancel();
    if (mounted) setState(() => _scanning = false);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('SimpleFltBLE')),
      body: ListView(
        children: [
          for (final peripheral in _found)
            ListTile(
              title: Text(peripheral.identifier),
              subtitle: Text('${peripheral.address}  ${peripheral.rssi} dBm'),
            ),
        ],
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: _scanning ? null : _scan,
        child: const Icon(Icons.search),
      ),
    );
  }
}
