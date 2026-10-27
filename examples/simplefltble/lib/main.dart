import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:simplefltble/simplefltble.dart';

void main() => runApp(const ExplorerApp());

String hex(List<int> data) => data.map((byte) => byte.toRadixString(16).padLeft(2, '0')).join(' ');

String describe(Peripheral peripheral) => peripheral.identifier.isEmpty ? '(no name)' : peripheral.identifier;

void showError(BuildContext context, Object error) {
  ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('$error')));
}

class ExplorerApp extends StatelessWidget {
  const ExplorerApp({super.key});

  @override
  Widget build(BuildContext context) => MaterialApp(
    title: 'SimpleBLE Explorer',
    theme: ThemeData(colorSchemeSeed: Colors.indigo),
    home: const ScanPage(),
  );
}

/// Scans with the first adapter and lists what it finds.
class ScanPage extends StatefulWidget {
  const ScanPage({super.key});

  @override
  State<ScanPage> createState() => _ScanPageState();
}

class _ScanPageState extends State<ScanPage> {
  Adapter? _adapter;
  final _peripherals = <Peripheral>[];
  final _subscriptions = <StreamSubscription<Object?>>[];
  bool _scanning = false;
  String _status = 'Starting...';

  @override
  void initState() {
    super.initState();
    _start();
  }

  Future<void> _start() async {
    try {
      // Android asks for the Bluetooth permissions at run time. iOS and macOS
      // ask the first time the app uses Bluetooth.
      if (Platform.isAndroid) await [Permission.bluetoothScan, Permission.bluetoothConnect].request();

      final adapters = await Adapter.getAdapters();
      if (adapters.isEmpty) {
        setState(() => _status = 'No adapters found');
        return;
      }
      final adapter = adapters.first;
      _subscriptions.addAll([
        adapter.onScanStart.listen((_) => setState(() => _scanning = true)),
        adapter.onScanStop.listen((_) => setState(() => _scanning = false)),
        adapter.onScanFound.listen((peripheral) => setState(() => _peripherals.add(peripheral))),
        adapter.onScanUpdated.listen((_) => setState(() {})),
      ]);
      setState(() {
        _adapter = adapter;
        _status = '${adapter.identifier} [${adapter.address}]';
      });
    } catch (error) {
      setState(() => _status = '$error');
    }
  }

  Future<void> _scan() async {
    setState(_peripherals.clear);
    try {
      await _adapter!.scanFor(const Duration(seconds: 5));
    } catch (error) {
      if (mounted) showError(context, error);
    }
  }

  @override
  void dispose() {
    for (final subscription in _subscriptions) {
      subscription.cancel();
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => Scaffold(
    appBar: AppBar(title: const Text('SimpleBLE Explorer'), bottom: _scanning ? const _Progress() : null),
    body: Column(
      children: [
        ListTile(leading: const Icon(Icons.bluetooth), title: Text(_status)),
        const Divider(height: 1),
        Expanded(
          child: ListView(
            children: [
              for (final peripheral in _peripherals)
                ListTile(
                  title: Text(describe(peripheral)),
                  subtitle: Text(peripheral.address),
                  trailing: Text('${peripheral.rssi ?? '-'} dBm'),
                  enabled: peripheral.isConnectable,
                  onTap: () => Navigator.of(
                    context,
                  ).push(MaterialPageRoute<void>(builder: (_) => DevicePage(peripheral: peripheral))),
                ),
            ],
          ),
        ),
      ],
    ),
    floatingActionButton: FloatingActionButton.extended(
      onPressed: _adapter == null || _scanning ? null : _scan,
      icon: const Icon(Icons.search),
      label: const Text('Scan'),
    ),
  );
}

class _Progress extends StatelessWidget implements PreferredSizeWidget {
  const _Progress();

  @override
  Size get preferredSize => const Size.fromHeight(4);

  @override
  Widget build(BuildContext context) => const LinearProgressIndicator();
}

/// Connects to a peripheral and lists its services.
class DevicePage extends StatefulWidget {
  const DevicePage({super.key, required this.peripheral});

  final Peripheral peripheral;

  @override
  State<DevicePage> createState() => _DevicePageState();
}

class _DevicePageState extends State<DevicePage> {
  List<Service>? _services;
  String _status = 'Connecting...';

  Peripheral get _peripheral => widget.peripheral;

  @override
  void initState() {
    super.initState();
    _connect();
  }

  Future<void> _connect() async {
    try {
      await _peripheral.connect();
      setState(() {
        _services = _peripheral.services();
        _status = 'Connected, MTU ${_peripheral.mtu}';
      });
    } catch (error) {
      setState(() => _status = '$error');
    }
  }

  @override
  void dispose() {
    if (_peripheral.isConnected) _peripheral.disconnect();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => Scaffold(
    appBar: AppBar(title: Text(describe(_peripheral))),
    body: ListView(
      children: [
        ListTile(leading: const Icon(Icons.link), title: Text(_status), subtitle: Text(_peripheral.address)),
        for (final service in _services ?? <Service>[]) ...[
          ListTile(title: const Text('Service'), subtitle: Text(service.uuid), tileColor: Colors.black12),
          for (final characteristic in service.characteristics)
            CharacteristicTile(peripheral: _peripheral, service: service, characteristic: characteristic),
        ],
      ],
    ),
  );
}

/// Reads, writes and subscribes to one characteristic.
class CharacteristicTile extends StatefulWidget {
  const CharacteristicTile({super.key, required this.peripheral, required this.service, required this.characteristic});

  final Peripheral peripheral;
  final Service service;
  final Characteristic characteristic;

  @override
  State<CharacteristicTile> createState() => _CharacteristicTileState();
}

class _CharacteristicTileState extends State<CharacteristicTile> {
  String _value = '';
  StreamSubscription<Object?>? _subscription;

  Set<CharacteristicCapability> get _capabilities => widget.characteristic.capabilities;

  Future<void> _run(Future<void> Function() action) async {
    try {
      await action();
    } catch (error) {
      if (mounted) showError(context, error);
    }
  }

  Future<void> _read() => _run(() async {
    final value = await widget.peripheral.read(widget.service.uuid, widget.characteristic.uuid);
    setState(() => _value = hex(value));
  });

  Future<void> _write() async {
    final controller = TextEditingController();
    final text = await showDialog<String>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Write text'),
        content: TextField(controller: controller, autofocus: true),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
          TextButton(onPressed: () => Navigator.pop(context, controller.text), child: const Text('Write')),
        ],
      ),
    );
    if (text == null) return;
    await _run(() async {
      final data = utf8.encode(text);
      if (_capabilities.contains(CharacteristicCapability.writeRequest)) {
        await widget.peripheral.writeRequest(widget.service.uuid, widget.characteristic.uuid, data);
      } else {
        await widget.peripheral.writeCommand(widget.service.uuid, widget.characteristic.uuid, data);
      }
    });
  }

  Future<void> _toggleSubscription() => _run(() async {
    final subscription = _subscription;
    if (subscription != null) {
      setState(() => _subscription = null);
      await subscription.cancel();
      return;
    }
    final values = _capabilities.contains(CharacteristicCapability.notify)
        ? await widget.peripheral.notify(widget.service.uuid, widget.characteristic.uuid)
        : await widget.peripheral.indicate(widget.service.uuid, widget.characteristic.uuid);
    setState(() {
      _subscription = values.listen(
        (value) => setState(() => _value = hex(value)),
        onDone: () => setState(() => _subscription = null),
      );
    });
  });

  @override
  void dispose() {
    _subscription?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => ListTile(
    title: Text(widget.characteristic.uuid),
    subtitle: Text(
      _value.isEmpty ? _capabilities.map((capability) => capability.name).join(', ') : _value,
      style: const TextStyle(fontFamily: 'monospace'),
    ),
    trailing: Wrap(
      children: [
        if (_capabilities.contains(CharacteristicCapability.read))
          IconButton(tooltip: 'Read', icon: const Icon(Icons.download), onPressed: _read),
        if (_capabilities.contains(CharacteristicCapability.writeRequest) ||
            _capabilities.contains(CharacteristicCapability.writeCommand))
          IconButton(tooltip: 'Write', icon: const Icon(Icons.upload), onPressed: _write),
        if (_capabilities.contains(CharacteristicCapability.notify) ||
            _capabilities.contains(CharacteristicCapability.indicate))
          IconButton(
            tooltip: _subscription == null ? 'Subscribe' : 'Unsubscribe',
            icon: Icon(_subscription == null ? Icons.notifications_none : Icons.notifications_active),
            onPressed: _toggleSubscription,
          ),
      ],
    ),
  );
}
