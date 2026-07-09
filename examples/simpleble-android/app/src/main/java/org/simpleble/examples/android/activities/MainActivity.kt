package org.simpleble.examples.android.activities

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.Button
import androidx.compose.material.Divider
import androidx.compose.material.MaterialTheme
import androidx.compose.material.OutlinedTextField
import androidx.compose.material.Surface
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import org.simpleble.android.SimpleDroidBle
import org.simpleble.examples.android.viewmodels.BluetoothViewModel
import org.simpleble.examples.android.viewmodels.capabilitySummary

class MainActivity : ComponentActivity() {
    private val bluetoothViewModel: BluetoothViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        SimpleDroidBle.initialize(applicationContext)

        setContent {
            MaterialTheme {
                Surface(modifier = Modifier.fillMaxSize()) {
                    var hasPermissions by remember {
                        mutableStateOf(SimpleDroidBle.hasPermissions(this@MainActivity))
                    }
                    val permissionLauncher = rememberLauncherForActivityResult(
                        ActivityResultContracts.RequestMultiplePermissions()
                    ) {
                        hasPermissions = SimpleDroidBle.hasPermissions(this@MainActivity)
                        if (hasPermissions) {
                            bluetoothViewModel.loadAdapters()
                        }
                    }

                    if (hasPermissions) {
                        LaunchedEffect(Unit) {
                            bluetoothViewModel.loadAdapters()
                        }
                        BleExampleScreen(bluetoothViewModel)
                    } else {
                        PermissionScreen {
                            permissionLauncher.launch(SimpleDroidBle.requiredPermissions)
                        }
                    }
                }
            }
        }
    }

    override fun onPause() {
        bluetoothViewModel.onActivityPaused()
        super.onPause()
    }
}

@Composable
private fun PermissionScreen(onRequestPermissions: () -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        verticalArrangement = Arrangement.Center
    ) {
        Text("SimpleBLE Android Example", style = MaterialTheme.typography.h5)
        Spacer(Modifier.height(12.dp))
        Text("Bluetooth scan and connect permissions are required before the adapter can be used.")
        Spacer(Modifier.height(16.dp))
        Button(onClick = onRequestPermissions) {
            Text("Grant Bluetooth permissions")
        }
    }
}

@Composable
private fun BleExampleScreen(viewModel: BluetoothViewModel) {
    val state = viewModel.state

    LazyColumn(
        modifier = Modifier.fillMaxSize(),
        contentPadding = PaddingValues(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        item {
            Text("SimpleBLE Android Example", style = MaterialTheme.typography.h5)
            Text("Version ${state.version}")
            Text("Bluetooth enabled: ${state.bluetoothEnabled}")
            Text(state.adapterSummary)
            state.error?.let { Text(it, color = MaterialTheme.colors.error) }
            Text(state.status)
        }

        item {
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(onClick = viewModel::loadAdapters) {
                    Text("Refresh adapter")
                }
                Button(
                    onClick = {
                        if (state.isScanning) viewModel.stopScan() else viewModel.startScan()
                    },
                    enabled = state.bluetoothEnabled && state.hasAdapter
                ) {
                    Text(if (state.isScanning) "Stop scan" else "Start scan")
                }
            }
        }

        item {
            SectionTitle("Discovered peripherals")
        }

        if (state.peripherals.isEmpty()) {
            item {
                Text(if (state.isScanning) "Scanning..." else "No peripherals discovered yet.")
            }
        } else {
            items(state.peripherals, key = { it.address }) { peripheral ->
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .clickable { viewModel.selectPeripheral(peripheral.address) }
                        .padding(vertical = 8.dp)
                ) {
                    Text(peripheral.name.ifBlank { "(unnamed)" }, fontWeight = FontWeight.Bold)
                    Text("${peripheral.address}  RSSI ${peripheral.rssi} dBm  ${if (peripheral.connectable) "connectable" else "not connectable"}")
                }
                Divider()
            }
        }

        state.selectedPeripheral?.let { selected ->
            item {
                SectionTitle("Connection")
                Text("${selected.name.ifBlank { "(unnamed)" }} [${selected.address}]")
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    Button(
                        onClick = {
                            if (state.connected) viewModel.disconnect() else viewModel.connectSelected()
                        },
                        enabled = selected.connectable || state.connected
                    ) {
                        Text(if (state.connected) "Disconnect" else "Connect")
                    }
                    Button(onClick = viewModel::loadServices, enabled = state.connected) {
                        Text("Refresh services")
                    }
                }
            }
        }

        if (state.connected) {
            item {
                SectionTitle("Services")
            }

            if (state.services.isEmpty()) {
                item { Text("No services discovered.") }
            } else {
                state.services.forEach { service ->
                    item {
                        Text(service.uuid, fontWeight = FontWeight.Bold)
                    }
                    items(service.characteristics, key = { "${service.uuid}:${it.uuid}" }) { characteristic ->
                        val selected = state.selectedCharacteristic?.service == service.uuid &&
                            state.selectedCharacteristic.characteristic == characteristic.uuid
                        Column(
                            modifier = Modifier
                                .fillMaxWidth()
                                .clickable {
                                    viewModel.selectCharacteristic(service.uuid, characteristic.uuid)
                                }
                                .padding(start = 12.dp, top = 6.dp, bottom = 6.dp)
                        ) {
                            Text(if (selected) "> ${characteristic.uuid}" else characteristic.uuid)
                            Text(characteristic.capabilitySummary())
                        }
                    }
                }
            }

            item {
                CharacteristicActions(viewModel)
            }
        }
    }
}

@Composable
private fun CharacteristicActions(viewModel: BluetoothViewModel) {
    val state = viewModel.state
    val target = state.selectedCharacteristic ?: return

    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        SectionTitle("Characteristic")
        Text("${target.service}\n${target.characteristic}")

        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            Button(onClick = viewModel::readSelected, enabled = target.canRead) {
                Text("Read")
            }
            Button(
                onClick = {
                    if (state.notifying) viewModel.stopNotifications() else viewModel.startNotifications()
                },
                enabled = target.canNotify || target.canIndicate || state.notifying
            ) {
                Text(if (state.notifying) "Stop notify" else "Subscribe")
            }
        }

        OutlinedTextField(
            value = state.writeHex,
            onValueChange = viewModel::setWriteHex,
            label = { Text("Write bytes as hex, for example 01 02 ff") },
            modifier = Modifier.fillMaxWidth()
        )

        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            Button(onClick = viewModel::writeRequest, enabled = target.canWriteRequest) {
                Text("Write request")
            }
            Button(onClick = viewModel::writeCommand, enabled = target.canWriteCommand) {
                Text("Write command")
            }
        }

        if (state.readValue.isNotEmpty()) {
            Text("Last read: ${state.readValue}")
        }

        if (state.notifications.isNotEmpty()) {
            Text("Notifications", fontWeight = FontWeight.Bold)
            state.notifications.takeLast(8).forEach { payload ->
                Text(payload)
            }
        }
    }
}

@Composable
private fun SectionTitle(text: String) {
    Text(text, style = MaterialTheme.typography.h6)
}
