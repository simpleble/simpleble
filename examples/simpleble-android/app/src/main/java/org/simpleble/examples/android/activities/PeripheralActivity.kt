package org.simpleble.examples.android.activities

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.simpleble.android.Adapter
import org.simpleble.android.BluetoothUUID
import org.simpleble.android.LocalAdvertisement
import org.simpleble.android.LocalCharacteristic
import org.simpleble.android.LocalCharacteristicCapability
import org.simpleble.android.LocalPeripheral
import org.simpleble.android.SimpleDroidBle
import org.simpleble.examples.android.ui.SimpleBleExplorerTheme

@OptIn(androidx.compose.material3.ExperimentalMaterial3Api::class)
class PeripheralActivity : ComponentActivity() {
    private var peripheral: LocalPeripheral? = null
    private var characteristic: LocalCharacteristic? = null
    private val collectors = mutableListOf<Job>()
    private var state by mutableStateOf(PeripheralDemoState())
    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) {
        if (SimpleDroidBle.hasPeripheralPermissions(this)) configurePeripheral()
        else state = state.copy(error = "Nearby devices permission is required for BLE advertising.")
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (SimpleDroidBle.hasPeripheralPermissions(this)) configurePeripheral()
        else permissionLauncher.launch(SimpleDroidBle.requiredPeripheralPermissions)

        setContent {
            SimpleBleExplorerTheme {
                PeripheralDemoScreen(
                    state = state,
                    onBack = ::finish,
                    onStart = ::startPeripheral,
                    onStop = ::stopPeripheral
                )
            }
        }
    }

    override fun onDestroy() {
        collectors.forEach(Job::cancel)
        peripheral?.takeIf { it.isStarted }?.let { active ->
            runBlocking(Dispatchers.IO) { runCatching { active.stop() } }
        }
        super.onDestroy()
    }

    private fun configurePeripheral() {
        runCatching {
            val adapter = Adapter.getAdapters().firstOrNull()
                ?: error("No Bluetooth adapter is available.")
            val localPeripheral = adapter.createLocalPeripheral(this)
            localPeripheral.advertisement = LocalAdvertisement(
                localName = adapter.identifier,
                serviceUuids = listOf(BluetoothUUID(SERVICE_UUID))
            )
            val localCharacteristic = localPeripheral
                .addService(BluetoothUUID(SERVICE_UUID))
                .addCharacteristic(
                    BluetoothUUID(CHARACTERISTIC_UUID),
                    LocalCharacteristicCapability.Read,
                    LocalCharacteristicCapability.WriteRequest,
                    LocalCharacteristicCapability.WriteCommand,
                    LocalCharacteristicCapability.Notify,
                    LocalCharacteristicCapability.Indicate
                )
            localCharacteristic.value = "ready".encodeToByteArray()
            localCharacteristic.setReadHandler { localCharacteristic.value }
            localCharacteristic.setWriteHandler { value -> localCharacteristic.value = value }

            peripheral = localPeripheral
            characteristic = localCharacteristic
            observe(localPeripheral, localCharacteristic)
            state = state.copy(adapterName = adapter.identifier, ready = true)
        }.onFailure { error -> state = state.copy(error = error.message ?: error.toString()) }
    }

    private fun observe(localPeripheral: LocalPeripheral, localCharacteristic: LocalCharacteristic) {
        collectors += lifecycleScope.launch {
            localPeripheral.onClientConnected.collect { address -> addEvent("Connected: $address") }
        }
        collectors += lifecycleScope.launch {
            localPeripheral.onClientDisconnected.collect { address -> addEvent("Disconnected: $address") }
        }
        collectors += lifecycleScope.launch {
            localCharacteristic.onSubscribed.collect { addEvent("Notifications enabled") }
        }
        collectors += lifecycleScope.launch {
            localCharacteristic.onUnsubscribed.collect { addEvent("Notifications disabled") }
        }
        collectors += lifecycleScope.launch {
            localCharacteristic.onWrite.collect { value ->
                addEvent("Write: ${value.toHex()}")
            }
        }
    }

    private fun startPeripheral() {
        val localPeripheral = peripheral ?: return
        lifecycleScope.launch {
            state = state.copy(busy = true, error = null)
            runCatching { localPeripheral.start() }
                .onSuccess {
                    state = state.copy(busy = false, started = true)
                    addEvent("Advertising started")
                }
                .onFailure { state = state.copy(busy = false, error = it.message ?: it.toString()) }
        }
    }

    private fun stopPeripheral() {
        val localPeripheral = peripheral ?: return
        lifecycleScope.launch {
            state = state.copy(busy = true, error = null)
            runCatching { localPeripheral.stop() }
                .onSuccess {
                    state = state.copy(busy = false, started = false)
                    addEvent("Advertising stopped")
                }
                .onFailure { state = state.copy(busy = false, error = it.message ?: it.toString()) }
        }
    }

    private fun addEvent(event: String) {
        state = state.copy(events = (listOf(event) + state.events).take(12))
    }

    private fun ByteArray.toHex(): String = joinToString(" ") { "%02x".format(it) }

    private companion object {
        const val SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0"
        const val CHARACTERISTIC_UUID = "12345678-1234-5678-1234-56789abcdef1"
    }
}

private data class PeripheralDemoState(
    val adapterName: String = "",
    val ready: Boolean = false,
    val started: Boolean = false,
    val busy: Boolean = false,
    val error: String? = null,
    val events: List<String> = emptyList()
)

@OptIn(androidx.compose.material3.ExperimentalMaterial3Api::class)
@Composable
private fun PeripheralDemoScreen(
    state: PeripheralDemoState,
    onBack: () -> Unit,
    onStart: () -> Unit,
    onStop: () -> Unit
) {
    Scaffold(topBar = { TopAppBar(title = { Text("Peripheral mode") }) }) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(20.dp)
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(14.dp)
        ) {
            Text(
                if (state.started) "Advertising" else "Stopped",
                style = MaterialTheme.typography.headlineSmall
            )
            if (state.adapterName.isNotEmpty()) Text("Device name: ${state.adapterName}")
            Text("Service", style = MaterialTheme.typography.labelLarge)
            Text(
                "12345678-1234-5678-1234-56789abcdef0",
                fontFamily = FontFamily.Monospace,
                style = MaterialTheme.typography.bodySmall
            )
            Text("The characteristic supports read, both write forms, notify, and indicate. Writes are echoed.")
            state.error?.let { Text(it, color = MaterialTheme.colorScheme.error) }
            Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                Button(
                    onClick = if (state.started) onStop else onStart,
                    enabled = state.ready && !state.busy
                ) {
                    Text(if (state.started) "Stop" else "Start")
                }
                OutlinedButton(onClick = onBack, enabled = !state.busy) { Text("Back") }
            }
            if (state.events.isNotEmpty()) {
                Text("Events", style = MaterialTheme.typography.titleMedium)
                Card(modifier = Modifier.fillMaxWidth()) {
                    Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
                        state.events.forEach { Text(it, fontFamily = FontFamily.Monospace) }
                    }
                }
            }
        }
    }
}
