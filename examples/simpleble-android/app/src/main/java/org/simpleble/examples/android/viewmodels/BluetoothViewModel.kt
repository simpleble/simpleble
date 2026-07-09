package org.simpleble.examples.android.viewmodels

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.simpleble.android.Adapter
import org.simpleble.android.BluetoothUUID
import org.simpleble.android.Characteristic
import org.simpleble.android.Peripheral
import org.simpleble.android.Service
import org.simpleble.android.SimpleDroidBle
import java.util.Locale

class BluetoothViewModel : ViewModel() {
    var state by mutableStateOf(BleUiState(version = SimpleDroidBle.getVersion()))
        private set

    private var adapter: Adapter? = null
    private var selectedPeripheral: Peripheral? = null
    private val peripheralsByAddress = linkedMapOf<String, Peripheral>()
    private var adapterEventsJob: Job? = null
    private var connectionEventsJob: Job? = null
    private var notificationJob: Job? = null

    fun loadAdapters() {
        viewModelScope.launch {
            runOperation("Adapter refreshed") {
                val bluetoothEnabled = Adapter.isBluetoothEnabled()
                val adapters = Adapter.getAdapters()
                adapter = adapters.firstOrNull()
                observeAdapter(adapter)
                state = state.copy(
                    bluetoothEnabled = bluetoothEnabled,
                    hasAdapter = adapter != null,
                    adapterSummary = adapter?.let {
                        "Android exposes one active adapter: ${it.identifier} [${it.address}]"
                    } ?: "No Android Bluetooth adapter available",
                    error = null
                )
            }
        }
    }

    fun startScan() {
        val activeAdapter = adapter ?: return loadAdapters()
        viewModelScope.launch {
            runOperation("Scanning") {
                peripheralsByAddress.clear()
                selectedPeripheral = null
                state = state.copy(
                    peripherals = emptyList(),
                    selectedPeripheral = null,
                    services = emptyList(),
                    selectedCharacteristic = null,
                    readValue = "",
                    notifications = emptyList()
                )
                activeAdapter.scanStart()
            }
        }
    }

    fun stopScan() {
        val activeAdapter = adapter ?: return
        viewModelScope.launch {
            runOperation("Scan stopped") {
                if (activeAdapter.scanIsActive) {
                    activeAdapter.scanStop()
                }
                state = state.copy(isScanning = false)
            }
        }
    }

    fun selectPeripheral(address: String) {
        selectedPeripheral = peripheralsByAddress[address]
        state = state.copy(
            selectedPeripheral = state.peripherals.firstOrNull { it.address == address },
            services = emptyList(),
            selectedCharacteristic = null,
            readValue = "",
            notifications = emptyList(),
            error = null
        )
    }

    fun connectSelected() {
        val peripheral = selectedPeripheral ?: return
        viewModelScope.launch {
            stopNotifications()
            stopScan()
            runOperation("Connected") {
                observeConnection(peripheral)
                peripheral.connect()
                state = state.copy(connected = peripheral.isConnected)
                loadServices()
            }
        }
    }

    fun disconnect() {
        val peripheral = selectedPeripheral ?: return
        viewModelScope.launch {
            stopNotifications()
            runOperation("Disconnected") {
                if (peripheral.isConnected) {
                    peripheral.disconnect()
                }
                state = state.copy(
                    connected = false,
                    services = emptyList(),
                    selectedCharacteristic = null,
                    readValue = "",
                    notifications = emptyList()
                )
            }
        }
    }

    fun loadServices() {
        val peripheral = selectedPeripheral ?: return
        viewModelScope.launch {
            runOperation("Services refreshed") {
                state = state.copy(
                    connected = peripheral.isConnected,
                    services = if (peripheral.isConnected) peripheral.services() else emptyList()
                )
            }
        }
    }

    fun selectCharacteristic(service: String, characteristic: String) {
        val characteristicInfo = state.services
            .firstOrNull { it.uuid == service }
            ?.characteristics
            ?.firstOrNull { it.uuid == characteristic }
            ?: return

        state = state.copy(
            selectedCharacteristic = CharacteristicTarget(
                service = service,
                characteristic = characteristic,
                canRead = characteristicInfo.canRead,
                canWriteRequest = characteristicInfo.canWriteRequest,
                canWriteCommand = characteristicInfo.canWriteCommand,
                canNotify = characteristicInfo.canNotify,
                canIndicate = characteristicInfo.canIndicate
            ),
            readValue = "",
            notifications = emptyList(),
            error = null
        )
    }

    fun readSelected() {
        val peripheral = selectedPeripheral ?: return
        val target = state.selectedCharacteristic ?: return
        viewModelScope.launch {
            runOperation("Read complete") {
                val payload = peripheral.read(BluetoothUUID(target.service), BluetoothUUID(target.characteristic))
                state = state.copy(readValue = payload.toHexString())
            }
        }
    }

    fun setWriteHex(value: String) {
        state = state.copy(writeHex = value)
    }

    fun writeRequest() {
        writeSelected(useRequest = true)
    }

    fun writeCommand() {
        writeSelected(useRequest = false)
    }

    fun startNotifications() {
        val peripheral = selectedPeripheral ?: return
        val target = state.selectedCharacteristic ?: return

        notificationJob?.cancel()
        notificationJob = viewModelScope.launch {
            try {
                state = state.copy(notifying = true, notifications = emptyList())
                val flow = if (target.canNotify) {
                    peripheral.notify(BluetoothUUID(target.service), BluetoothUUID(target.characteristic))
                } else {
                    peripheral.indicate(BluetoothUUID(target.service), BluetoothUUID(target.characteristic))
                }
                flow.collect { payload ->
                    state = state.copy(notifications = state.notifications + payload.toHexString())
                }
            } catch (error: CancellationException) {
                throw error
            } catch (error: Exception) {
                state = state.copy(error = error.message ?: error::class.java.simpleName, status = "Notification failed")
            } finally {
                state = state.copy(notifying = false)
            }
        }
        state = state.copy(status = "Subscribed", error = null)
    }

    fun stopNotifications() {
        val target = state.selectedCharacteristic
        notificationJob?.cancel()
        notificationJob = null
        if (target != null) {
            state = state.copy(notifying = false)
        }
    }

    fun onActivityPaused() {
        stopScan()
    }

    override fun onCleared() {
        runBlocking {
            notificationJob?.cancelAndJoin()
        }
        runCatching {
            adapter?.takeIf { it.scanIsActive }?.scanStop()
        }
        runCatching {
            selectedPeripheral?.takeIf { it.isConnected }?.let { peripheral ->
                runBlocking(Dispatchers.IO) { peripheral.disconnect() }
            }
        }
        super.onCleared()
    }

    private fun observeAdapter(adapter: Adapter?) {
        adapterEventsJob?.cancel()
        if (adapter == null) return

        adapterEventsJob = viewModelScope.launch {
            launch {
                adapter.onScanActive.collect {
                    state = state.copy(isScanning = it)
                }
            }
            launch {
                adapter.onScanFound.collect(::upsertPeripheral)
            }
            launch {
                adapter.onScanUpdated.collect(::upsertPeripheral)
            }
        }
    }

    private fun observeConnection(peripheral: Peripheral) {
        connectionEventsJob?.cancel()
        connectionEventsJob = viewModelScope.launch {
            peripheral.onConnectionActive.collect { connected ->
                state = state.copy(connected = connected)
            }
        }
    }

    private fun upsertPeripheral(peripheral: Peripheral) {
        val address = peripheral.address.toString()
        peripheralsByAddress[address] = peripheral
        state = state.copy(
            peripherals = peripheralsByAddress.values.map {
                PeripheralSummary(
                    name = it.identifier,
                    address = it.address.toString(),
                    rssi = it.rssi,
                    connectable = it.isConnectable
                )
            }
        )
    }

    private fun writeSelected(useRequest: Boolean) {
        val peripheral = selectedPeripheral ?: return
        val target = state.selectedCharacteristic ?: return
        viewModelScope.launch {
            runOperation(if (useRequest) "Write request complete" else "Write command complete") {
                val payload = parseHex(state.writeHex)
                if (useRequest) {
                    peripheral.writeRequest(BluetoothUUID(target.service), BluetoothUUID(target.characteristic), payload)
                } else {
                    peripheral.writeCommand(BluetoothUUID(target.service), BluetoothUUID(target.characteristic), payload)
                }
            }
        }
    }

    private suspend fun runOperation(successStatus: String, block: suspend () -> Unit) {
        try {
            block()
            state = state.copy(status = successStatus, error = null)
        } catch (error: CancellationException) {
            throw error
        } catch (error: Exception) {
            state = state.copy(error = error.message ?: error::class.java.simpleName, status = "Operation failed")
        }
    }
}

data class BleUiState(
    val version: String = "",
    val bluetoothEnabled: Boolean = false,
    val hasAdapter: Boolean = false,
    val adapterSummary: String = "Adapter not loaded",
    val isScanning: Boolean = false,
    val peripherals: List<PeripheralSummary> = emptyList(),
    val selectedPeripheral: PeripheralSummary? = null,
    val connected: Boolean = false,
    val services: List<Service> = emptyList(),
    val selectedCharacteristic: CharacteristicTarget? = null,
    val writeHex: String = "",
    val readValue: String = "",
    val notifying: Boolean = false,
    val notifications: List<String> = emptyList(),
    val status: String = "Idle",
    val error: String? = null
)

data class PeripheralSummary(
    val name: String,
    val address: String,
    val rssi: Int,
    val connectable: Boolean
)

data class CharacteristicTarget(
    val service: String,
    val characteristic: String,
    val canRead: Boolean,
    val canWriteRequest: Boolean,
    val canWriteCommand: Boolean,
    val canNotify: Boolean,
    val canIndicate: Boolean
)

fun Characteristic.capabilitySummary(): String {
    val capabilities = buildList {
        if (canRead) add("read")
        if (canWriteRequest) add("write request")
        if (canWriteCommand) add("write command")
        if (canNotify) add("notify")
        if (canIndicate) add("indicate")
    }
    return if (capabilities.isEmpty()) "no common GATT operations" else capabilities.joinToString()
}

private fun ByteArray.toHexString(): String {
    return joinToString(" ") { byte ->
        "%02x".format(Locale.US, byte.toInt() and 0xff)
    }
}

private fun parseHex(value: String): ByteArray {
    val normalized = value
        .replace("0x", "", ignoreCase = true)
        .replace(Regex("[^0-9a-fA-F]"), "")

    require(normalized.length % 2 == 0) { "Hex input must contain complete bytes." }

    return normalized.chunked(2)
        .map { it.toInt(16).toByte() }
        .toByteArray()
}
