package org.simpleble.examples.android.views

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.CenterAlignedTopAppBar
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilledTonalButton
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import org.simpleble.android.Characteristic
import org.simpleble.android.Service
import org.simpleble.examples.android.viewmodels.BleUiState
import org.simpleble.examples.android.viewmodels.ExplorerAction
import org.simpleble.examples.android.viewmodels.PeripheralSummary
import org.simpleble.examples.android.viewmodels.capabilitySummary

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ExplorerScreen(
    state: BleUiState,
    onAction: (ExplorerAction) -> Unit,
    onRequestBluetooth: () -> Unit
) {
    val selected = state.selectedPeripheral
    Scaffold(
        topBar = {
            CenterAlignedTopAppBar(
                title = {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Text(
                            text = if (selected == null) "SimpleBLE Explorer" else selected.displayName,
                            maxLines = 1,
                            overflow = TextOverflow.Ellipsis
                        )
                        if (state.plainBackend) {
                            Text(
                                "PLAIN simulation",
                                style = MaterialTheme.typography.labelSmall,
                                color = MaterialTheme.colorScheme.primary
                            )
                        }
                    }
                },
                navigationIcon = {
                    if (selected != null) {
                        TextButton(
                            onClick = { onAction(ExplorerAction.ClosePeripheral) },
                            enabled = !state.busy
                        ) {
                            Text("Back")
                        }
                    }
                },
                colors = TopAppBarDefaults.centerAlignedTopAppBarColors(
                    containerColor = MaterialTheme.colorScheme.surface
                )
            )
        }
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding)
        ) {
            if (state.busy) {
                androidx.compose.material3.LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
            }
            state.error?.let { message ->
                ErrorBanner(message = message, onDismiss = { onAction(ExplorerAction.ClearError) })
            }

            if (selected == null) {
                ScanContent(
                    state = state,
                    onAction = onAction,
                    onRequestBluetooth = onRequestBluetooth
                )
            } else {
                PeripheralContent(state = state, onAction = onAction)
            }
        }
    }
}

@Composable
fun PermissionScreen(
    permissionRequested: Boolean,
    onRequestPermissions: () -> Unit,
    onOpenSettings: () -> Unit
) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .padding(28.dp),
        contentAlignment = Alignment.Center
    ) {
        Column(
            modifier = Modifier.fillMaxWidth(),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Surface(
                shape = CircleShape,
                color = MaterialTheme.colorScheme.primaryContainer,
                modifier = Modifier.size(88.dp)
            ) {
                Box(contentAlignment = Alignment.Center) {
                    Text(
                        "BLE",
                        style = MaterialTheme.typography.headlineMedium,
                        fontWeight = FontWeight.Black,
                        color = MaterialTheme.colorScheme.onPrimaryContainer
                    )
                }
            }
            Text(
                "Find nearby Bluetooth devices",
                style = MaterialTheme.typography.headlineSmall,
                fontWeight = FontWeight.Bold
            )
            Text(
                "SimpleBLE Explorer needs Nearby devices permission to scan and connect. Device data stays on this phone.",
                style = MaterialTheme.typography.bodyLarge,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            Button(onClick = onRequestPermissions, modifier = Modifier.fillMaxWidth()) {
                Text(if (permissionRequested) "Try again" else "Continue")
            }
            if (permissionRequested) {
                TextButton(onClick = onOpenSettings) {
                    Text("Open app settings")
                }
            }
        }
    }
}

@Composable
private fun ScanContent(
    state: BleUiState,
    onAction: (ExplorerAction) -> Unit,
    onRequestBluetooth: () -> Unit
) {
    val query = state.searchQuery.trim()
    val peripherals = remember(state.peripherals, query) {
        if (query.isEmpty()) state.peripherals else state.peripherals.filter {
            it.name.contains(query, ignoreCase = true) ||
                it.address.contains(query, ignoreCase = true)
        }
    }

    LazyColumn(
        modifier = Modifier.fillMaxSize(),
        contentPadding = PaddingValues(start = 20.dp, end = 20.dp, top = 12.dp, bottom = 32.dp),
        verticalArrangement = Arrangement.spacedBy(14.dp)
    ) {
        item {
            AdapterCard(state = state, onAction = onAction, onRequestBluetooth = onRequestBluetooth)
        }
        item {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column(modifier = Modifier.weight(1f)) {
                    Text("Nearby devices", style = MaterialTheme.typography.titleLarge, fontWeight = FontWeight.Bold)
                    Text(
                        if (state.isScanning) "Listening for advertisements" else "${state.peripherals.size} discovered",
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
                Spacer(Modifier.width(12.dp))
                Button(
                    onClick = { onAction(ExplorerAction.ToggleScan) },
                    enabled = state.bluetoothEnabled && state.hasAdapter && !state.busy
                ) {
                    if (state.isScanning) {
                        CircularProgressIndicator(
                            modifier = Modifier.size(18.dp),
                            strokeWidth = 2.dp,
                            color = MaterialTheme.colorScheme.onPrimary
                        )
                        Spacer(Modifier.width(8.dp))
                    }
                    Text(if (state.isScanning) "Stop" else "Scan")
                }
            }
        }
        if (state.peripherals.isNotEmpty()) {
            item {
                OutlinedTextField(
                    value = state.searchQuery,
                    onValueChange = { onAction(ExplorerAction.SetSearchQuery(it)) },
                    label = { Text("Filter by name or address") },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth()
                )
            }
        }
        if (peripherals.isEmpty()) {
            item {
                EmptyDevicesCard(scanning = state.isScanning, filtered = query.isNotEmpty())
            }
        } else {
            items(peripherals, key = { it.address }) { peripheral ->
                PeripheralCard(
                    peripheral = peripheral,
                    onClick = { onAction(ExplorerAction.SelectPeripheral(peripheral.address)) }
                )
            }
        }
    }
}

@Composable
private fun AdapterCard(
    state: BleUiState,
    onAction: (ExplorerAction) -> Unit,
    onRequestBluetooth: () -> Unit
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("adapter-card"),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.primaryContainer),
        shape = RoundedCornerShape(24.dp)
    ) {
        Column(
            modifier = Modifier.padding(20.dp),
            verticalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column(modifier = Modifier.weight(1f)) {
                    Text("Bluetooth adapter", style = MaterialTheme.typography.labelLarge)
                    Text(
                        state.adapterName.ifBlank { "Not available" },
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold,
                        maxLines = 2,
                        overflow = TextOverflow.Ellipsis
                    )
                    if (state.adapterAddress.isNotBlank()) {
                        Text(
                            state.adapterAddress,
                            style = MaterialTheme.typography.bodySmall,
                            fontFamily = FontFamily.Monospace
                        )
                    }
                }
                StatusPill(
                    text = if (state.bluetoothEnabled) "Ready" else "Off",
                    positive = state.bluetoothEnabled
                )
            }
            Text(
                state.status,
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onPrimaryContainer
            )
            if (!state.bluetoothEnabled && !state.plainBackend) {
                Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    FilledTonalButton(
                        onClick = { onAction(ExplorerAction.RefreshAdapter) },
                        enabled = !state.busy,
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Text("Refresh")
                    }
                    Button(onClick = onRequestBluetooth, modifier = Modifier.fillMaxWidth()) {
                        Text("Turn on Bluetooth")
                    }
                }
            } else {
                FilledTonalButton(
                    onClick = { onAction(ExplorerAction.RefreshAdapter) },
                    enabled = !state.busy
                ) {
                    Text("Refresh")
                }
            }
        }
    }
}

@Composable
private fun EmptyDevicesCard(scanning: Boolean, filtered: Boolean) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
    ) {
        Column(
            modifier = Modifier.padding(24.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Text(
                when {
                    filtered -> "No matching devices"
                    scanning -> "Scanning nearby…"
                    else -> "No devices yet"
                },
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.Bold
            )
            Text(
                when {
                    filtered -> "Try a different name or Bluetooth address."
                    scanning -> "Keep the peripheral awake and nearby. Results appear as advertisements arrive."
                    else -> "Start a scan to discover connectable BLE peripherals."
                },
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
    }
}

@Composable
private fun PeripheralCard(peripheral: PeripheralSummary, onClick: () -> Unit) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("peripheral-card")
            .clickable(enabled = peripheral.connectable, role = Role.Button, onClick = onClick),
        border = BorderStroke(1.dp, MaterialTheme.colorScheme.outline.copy(alpha = 0.25f)),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)
    ) {
        Row(
            modifier = Modifier.padding(18.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(14.dp)
        ) {
            SignalBadge(rssi = peripheral.rssi)
            Column(modifier = Modifier.weight(1f)) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        peripheral.displayName,
                        modifier = Modifier.weight(1f),
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.SemiBold,
                        maxLines = 2,
                        overflow = TextOverflow.Ellipsis
                    )
                    if (peripheral.connectable) {
                        Text("Open", color = MaterialTheme.colorScheme.primary, maxLines = 1)
                    }
                }
                Text(
                    peripheral.address,
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis
                )
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    Text("${peripheral.rssi} dBm", style = MaterialTheme.typography.labelMedium)
                    if (peripheral.paired) Text("Paired", style = MaterialTheme.typography.labelMedium)
                    if (!peripheral.connectable) Text("Unavailable", style = MaterialTheme.typography.labelMedium)
                }
            }
        }
    }
}

@Composable
private fun PeripheralContent(state: BleUiState, onAction: (ExplorerAction) -> Unit) {
    val peripheral = state.selectedPeripheral ?: return
    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .testTag("peripheral-content"),
        contentPadding = PaddingValues(start = 20.dp, end = 20.dp, top = 12.dp, bottom = 36.dp),
        verticalArrangement = Arrangement.spacedBy(14.dp)
    ) {
        item {
            Card(
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("connection-card"),
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.primaryContainer),
                shape = RoundedCornerShape(24.dp)
            ) {
                Column(
                    modifier = Modifier.padding(20.dp),
                    verticalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text("Connection", style = MaterialTheme.typography.labelLarge)
                        StatusPill(if (state.connected) "Connected" else "Disconnected", state.connected)
                    }
                    Text(peripheral.address, fontFamily = FontFamily.Monospace)
                    Text("${peripheral.rssi} dBm${state.mtu?.let { " · MTU $it" }.orEmpty()}")
                    Button(
                        onClick = {
                            onAction(if (state.connected) ExplorerAction.Disconnect else ExplorerAction.Connect)
                        },
                        enabled = !state.busy && (peripheral.connectable || state.connected),
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Text(if (state.connected) "Disconnect" else "Connect")
                    }
                }
            }
        }

        if (state.connected) {
            item {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Column {
                        Text("GATT services", style = MaterialTheme.typography.titleLarge, fontWeight = FontWeight.Bold)
                        Text("${state.services.size} services", color = MaterialTheme.colorScheme.onSurfaceVariant)
                    }
                    TextButton(
                        onClick = { onAction(ExplorerAction.RefreshServices) },
                        enabled = !state.busy
                    ) {
                        Text("Refresh")
                    }
                }
            }
            if (state.services.isEmpty()) {
                item { EmptyServicesCard() }
            } else {
                items(state.services, key = { it.uuid }) { service ->
                    ServiceCard(service = service, state = state, onAction = onAction)
                }
            }
            state.selectedCharacteristic?.let {
                item { CharacteristicActions(state = state, onAction = onAction) }
            }
        } else {
            item {
                Text(
                    "Connect to discover services and inspect characteristic capabilities.",
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    style = MaterialTheme.typography.bodyLarge
                )
            }
        }
    }
}

@Composable
private fun ServiceCard(service: Service, state: BleUiState, onAction: (ExplorerAction) -> Unit) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("service-card"),
        border = BorderStroke(1.dp, MaterialTheme.colorScheme.outline.copy(alpha = 0.2f))
    ) {
        Column(modifier = Modifier.padding(vertical = 8.dp)) {
            Column(modifier = Modifier.padding(horizontal = 18.dp, vertical = 10.dp)) {
                Text("Service", style = MaterialTheme.typography.labelMedium, color = MaterialTheme.colorScheme.primary)
                Text(service.uuid, style = MaterialTheme.typography.bodyMedium, fontFamily = FontFamily.Monospace)
            }
            service.characteristics.forEachIndexed { index, characteristic ->
                if (index > 0) HorizontalDivider(modifier = Modifier.padding(horizontal = 18.dp))
                val selected = state.selectedCharacteristic?.let {
                    it.service == service.uuid && it.characteristic == characteristic.uuid
                } == true
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .clickable(role = Role.Button) {
                            onAction(ExplorerAction.SelectCharacteristic(service.uuid, characteristic.uuid))
                        }
                        .padding(horizontal = 18.dp, vertical = 14.dp)
                ) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text(
                                characteristic.uuid,
                                style = MaterialTheme.typography.bodySmall,
                                fontFamily = FontFamily.Monospace,
                                fontWeight = if (selected) FontWeight.Bold else FontWeight.Normal
                            )
                            Text(
                                characteristic.capabilitySummary(),
                                style = MaterialTheme.typography.labelMedium,
                                color = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                        }
                        if (selected) StatusPill("Selected", true)
                    }
                }
            }
        }
    }
}

@Composable
private fun CharacteristicActions(state: BleUiState, onAction: (ExplorerAction) -> Unit) {
    val target = state.selectedCharacteristic ?: return
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("characteristic-tools"),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.secondaryContainer),
        shape = RoundedCornerShape(24.dp)
    ) {
        Column(
            modifier = Modifier.padding(20.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            Text("Characteristic tools", style = MaterialTheme.typography.titleLarge, fontWeight = FontWeight.Bold)
            Text(target.characteristic, style = MaterialTheme.typography.bodySmall, fontFamily = FontFamily.Monospace)
            Row(
                modifier = Modifier.horizontalScroll(rememberScrollState()),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                if (target.canRead) CapabilityPill("Read")
                if (target.canWriteRequest) CapabilityPill("Write")
                if (target.canWriteCommand) CapabilityPill("Write command")
                if (target.canNotify) CapabilityPill("Notify")
                if (target.canIndicate) CapabilityPill("Indicate")
            }
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(
                    onClick = { onAction(ExplorerAction.Read) },
                    enabled = target.canRead && !state.busy
                ) {
                    Text("Read")
                }
                OutlinedButton(
                    onClick = { onAction(ExplorerAction.ToggleNotifications) },
                    enabled = (target.canNotify || target.canIndicate || state.notifying) && !state.busy
                ) {
                    Text(if (state.notifying) "Stop listening" else "Listen")
                }
            }
            state.readValue?.let { value ->
                DataPanel(title = "Last read", value = value.ifEmpty { "(empty)" })
            }
            if (target.canWriteRequest || target.canWriteCommand) {
                OutlinedTextField(
                    value = state.writeHex,
                    onValueChange = { onAction(ExplorerAction.SetWriteHex(it)) },
                    label = { Text("Hex bytes") },
                    supportingText = { Text("Example: 01 02 ff") },
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = true
                )
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    if (target.canWriteRequest) {
                        Button(
                            onClick = { onAction(ExplorerAction.WriteRequest) },
                            enabled = !state.busy && state.writeHex.isNotBlank()
                        ) { Text("Write") }
                    }
                    if (target.canWriteCommand) {
                        OutlinedButton(
                            onClick = { onAction(ExplorerAction.WriteCommand) },
                            enabled = !state.busy && state.writeHex.isNotBlank()
                        ) { Text("Write command") }
                    }
                }
            }
            if (state.notifications.isNotEmpty()) {
                DataPanel(
                    title = "Latest value · ${state.notifications.size}",
                    value = state.notifications.last()
                )
            }
        }
    }
}

@Composable
private fun DataPanel(title: String, value: String) {
    Surface(
        color = MaterialTheme.colorScheme.surface.copy(alpha = 0.72f),
        shape = RoundedCornerShape(14.dp),
        modifier = Modifier.fillMaxWidth()
    ) {
        Column(modifier = Modifier.padding(14.dp)) {
            Text(title, style = MaterialTheme.typography.labelMedium)
            Text(value, fontFamily = FontFamily.Monospace, style = MaterialTheme.typography.bodyMedium)
        }
    }
}

@Composable
private fun CapabilityPill(text: String) {
    Surface(
        shape = CircleShape,
        color = MaterialTheme.colorScheme.surface.copy(alpha = 0.7f)
    ) {
        Text(
            text,
            modifier = Modifier.padding(horizontal = 12.dp, vertical = 7.dp),
            style = MaterialTheme.typography.labelMedium
        )
    }
}

@Composable
private fun EmptyServicesCard() {
    Surface(
        color = MaterialTheme.colorScheme.surfaceVariant,
        shape = RoundedCornerShape(18.dp),
        modifier = Modifier.fillMaxWidth()
    ) {
        Text(
            "No services were reported. Refresh after the peripheral finishes discovery.",
            modifier = Modifier.padding(20.dp),
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
    }
}

@Composable
private fun ErrorBanner(message: String, onDismiss: () -> Unit) {
    Surface(color = MaterialTheme.colorScheme.errorContainer, modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier.padding(horizontal = 20.dp, vertical = 12.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                message,
                modifier = Modifier.weight(1f),
                color = MaterialTheme.colorScheme.onErrorContainer
            )
            TextButton(onClick = onDismiss) { Text("Dismiss") }
        }
    }
}

@Composable
private fun StatusPill(text: String, positive: Boolean) {
    Surface(
        shape = CircleShape,
        color = if (positive) {
            MaterialTheme.colorScheme.primary.copy(alpha = 0.14f)
        } else {
            MaterialTheme.colorScheme.error.copy(alpha = 0.14f)
        }
    ) {
        Text(
            text,
            modifier = Modifier.padding(horizontal = 10.dp, vertical = 5.dp),
            style = MaterialTheme.typography.labelMedium,
            color = if (positive) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.error
        )
    }
}

@Composable
private fun SignalBadge(rssi: Int) {
    Surface(shape = CircleShape, color = MaterialTheme.colorScheme.secondaryContainer, modifier = Modifier.size(50.dp)) {
        Box(contentAlignment = Alignment.Center) {
            Text(
                when {
                    rssi >= -55 -> "•••"
                    rssi >= -75 -> "••"
                    else -> "•"
                },
                fontWeight = FontWeight.Black,
                color = MaterialTheme.colorScheme.onSecondaryContainer
            )
        }
    }
}

private val PeripheralSummary.displayName: String
    get() = name.ifBlank { "Unnamed" }
