package org.simpleble.examples.android.viewmodels

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import org.simpleble.android.Adapter
import org.simpleble.android.BluetoothUUID
import org.simpleble.android.Characteristic
import org.simpleble.android.Peripheral
import org.simpleble.android.Service
import org.simpleble.examples.android.BuildConfig

class BluetoothViewModel : ViewModel() {
    var state by mutableStateOf(
        BleUiState(
            plainBackend = BuildConfig.PLAIN_BACKEND
        )
    )
        private set

    private var adapter: Adapter? = null
    private var selectedPeripheral: Peripheral? = null
    private val peripheralsByAddress = linkedMapOf<String, Peripheral>()
    private val operationMutex = Mutex()
    private var adapterEventsJob: Job? = null
    private var connectionEventsJob: Job? = null
    private var notificationJob: Job? = null

    fun onAction(action: ExplorerAction) {
        when (action) {
            ExplorerAction.RefreshAdapter -> loadAdapters()
            ExplorerAction.ToggleScan -> if (state.isScanning) stopScan() else startScan()
            ExplorerAction.Connect -> connectSelected()
            ExplorerAction.Disconnect -> disconnect()
            ExplorerAction.RefreshServices -> loadServices()
            ExplorerAction.Read -> readSelected()
            ExplorerAction.WriteRequest -> writeSelected(useRequest = true)
            ExplorerAction.WriteCommand -> writeSelected(useRequest = false)
            ExplorerAction.ToggleNotifications -> {
                if (state.notifying) stopNotifications() else startNotifications()
            }
            ExplorerAction.ClosePeripheral -> closePeripheral()
            ExplorerAction.ClearError -> state = state.copy(error = null)
            is ExplorerAction.SelectPeripheral -> selectPeripheral(action.address)
            is ExplorerAction.SelectCharacteristic -> {
                selectCharacteristic(action.service, action.characteristic)
            }
            is ExplorerAction.SetWriteHex -> state = state.copy(writeHex = action.value)
            is ExplorerAction.SetSearchQuery -> state = state.copy(searchQuery = action.value)
        }
    }

    fun loadAdapters() {
        launchOperation("Refreshing adapter", "Adapter status refreshed") {
            val bluetoothEnabled = Adapter.isBluetoothEnabled()
            val activeAdapter = Adapter.getAdapters().firstOrNull()
            val adapterInfo = AdapterInfo(
                adapter = activeAdapter,
                bluetoothEnabled = bluetoothEnabled,
                name = activeAdapter?.identifier.orEmpty(),
                address = activeAdapter?.address?.toString().orEmpty()
            )

            adapter = adapterInfo.adapter
            observeAdapter(adapterInfo.adapter)
            state = state.copy(
                bluetoothEnabled = adapterInfo.bluetoothEnabled,
                hasAdapter = adapterInfo.adapter != null,
                adapterName = adapterInfo.name,
                adapterAddress = adapterInfo.address
            )
        }
    }

    fun onActivityPaused() {
        stopScan()
    }

    override fun onCleared() {
        adapterEventsJob?.cancel()
        connectionEventsJob?.cancel()
        val subscription = notificationJob
        subscription?.cancel()

        val activeAdapter = adapter
        val activePeripheral = selectedPeripheral
        val cleanupScope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
        cleanupScope.launch {
            subscription?.join()
            runCatching { activeAdapter?.takeIf { it.scanIsActive }?.scanStop() }
            runCatching { activePeripheral?.takeIf { it.isConnected }?.disconnect() }
            cleanupScope.cancel()
        }

        super.onCleared()
    }

    private fun startScan() {
        val activeAdapter = adapter ?: return loadAdapters()
        if (state.connected) {
            state = state.copy(error = "Disconnect before starting a new scan.")
            return
        }

        launchOperation("Starting scan", "Scanning") {
            stopNotificationsNow()
            peripheralsByAddress.clear()
            selectedPeripheral = null
            state = state.copy(
                peripherals = emptyList(),
                selectedPeripheral = null,
                services = emptyList(),
                selectedCharacteristic = null,
                readValue = null,
                notifications = emptyList()
            )
            activeAdapter.scanStart()
        }
    }

    private fun stopScan() {
        val activeAdapter = adapter ?: return
        launchOperation("Stopping scan", "Scan stopped") {
            stopScanNow(activeAdapter)
        }
    }

    private suspend fun stopScanNow(activeAdapter: Adapter? = adapter) {
        val currentAdapter = activeAdapter ?: return
        if (currentAdapter.scanIsActive) currentAdapter.scanStop()
        state = state.copy(isScanning = false)
    }

    private fun selectPeripheral(address: String) {
        if (state.connected) {
            state = state.copy(error = "Disconnect before selecting another peripheral.")
            return
        }

        selectedPeripheral = peripheralsByAddress[address]
        state = state.copy(
            selectedPeripheral = state.peripherals.firstOrNull { it.address == address },
            services = emptyList(),
            selectedCharacteristic = null,
            readValue = null,
            notifications = emptyList(),
            error = null
        )
    }

    private fun closePeripheral() {
        launchOperation("Closing peripheral", "Ready") {
            disconnectNow()
            selectedPeripheral = null
            state = state.copy(
                selectedPeripheral = null,
                services = emptyList(),
                selectedCharacteristic = null,
                readValue = null,
                notifications = emptyList()
            )
        }
    }

    private fun connectSelected() {
        val peripheral = selectedPeripheral ?: return
        launchOperation("Connecting", "Connected") {
            stopNotificationsNow()
            stopScanNow()
            observeConnection(peripheral)
            peripheral.connect()

            val connected = peripheral.isConnected
            val connection = ConnectionInfo(
                connected = connected,
                services = if (connected) peripheral.services() else emptyList(),
                mtu = if (connected) peripheral.mtu else null
            )
            state = state.copy(
                connected = connection.connected,
                services = connection.services,
                mtu = connection.mtu,
                selectedCharacteristic = null
            )
        }
    }

    private fun disconnect() {
        launchOperation("Disconnecting", "Disconnected") {
            disconnectNow()
        }
    }

    private suspend fun disconnectNow() {
        stopNotificationsNow()
        val peripheral = selectedPeripheral
        if (peripheral != null) {
            if (peripheral.isConnected) peripheral.disconnect()
        }
        connectionEventsJob?.cancel()
        connectionEventsJob = null
        state = state.copy(
            connected = false,
            mtu = null,
            services = emptyList(),
            selectedCharacteristic = null,
            readValue = null,
            notifications = emptyList(),
            notifying = false
        )
    }

    private fun loadServices() {
        val peripheral = selectedPeripheral ?: return
        launchOperation("Refreshing services", "Services refreshed") {
            val connected = peripheral.isConnected
            val connection = ConnectionInfo(
                connected = connected,
                services = if (connected) peripheral.services() else emptyList(),
                mtu = if (connected) peripheral.mtu else null
            )
            state = state.copy(
                connected = connection.connected,
                services = connection.services,
                mtu = connection.mtu
            )
        }
    }

    private fun selectCharacteristic(service: String, characteristic: String) {
        val characteristicInfo = state.services
            .firstOrNull { it.uuid == service }
            ?.characteristics
            ?.firstOrNull { it.uuid == characteristic }
            ?: return

        launchOperation("Selecting characteristic", "Characteristic selected") {
            stopNotificationsNow()
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
                readValue = null,
                notifications = emptyList()
            )
        }
    }

    private fun readSelected() {
        val peripheral = selectedPeripheral ?: return
        val target = state.selectedCharacteristic ?: return
        launchOperation("Reading characteristic", "Read complete") {
            val payload = peripheral.read(BluetoothUUID(target.service), BluetoothUUID(target.characteristic))
            state = state.copy(readValue = payload.toHexString())
        }
    }

    private fun writeSelected(useRequest: Boolean) {
        val peripheral = selectedPeripheral ?: return
        val target = state.selectedCharacteristic ?: return
        launchOperation(
            if (useRequest) "Writing with response" else "Writing without response",
            if (useRequest) "Write request complete" else "Write command complete"
        ) {
            val payload = parseHex(state.writeHex)
            if (useRequest) {
                peripheral.writeRequest(
                    BluetoothUUID(target.service),
                    BluetoothUUID(target.characteristic),
                    payload
                )
            } else {
                peripheral.writeCommand(
                    BluetoothUUID(target.service),
                    BluetoothUUID(target.characteristic),
                    payload
                )
            }
        }
    }

    private fun startNotifications() {
        val peripheral = selectedPeripheral ?: return
        val target = state.selectedCharacteristic ?: return

        viewModelScope.launch {
            operationMutex.withLock {
                stopNotificationsNow()
                state = state.copy(
                    notifying = true,
                    notifications = emptyList(),
                    status = if (target.canNotify) "Subscribing to notifications" else "Subscribing to indications",
                    error = null
                )

                notificationJob = viewModelScope.launch {
                    try {
                        val flow = if (target.canNotify) {
                            peripheral.notify(BluetoothUUID(target.service), BluetoothUUID(target.characteristic))
                        } else {
                            peripheral.indicate(BluetoothUUID(target.service), BluetoothUUID(target.characteristic))
                        }
                        state = state.copy(status = "Listening")
                        flow.collect { payload ->
                            state = state.copy(
                                notifications = (state.notifications + payload.toHexString()).takeLast(50)
                            )
                        }
                    } catch (error: CancellationException) {
                        throw error
                    } catch (error: Exception) {
                        state = state.copy(
                            error = error.message ?: error::class.java.simpleName,
                            status = "Subscription failed"
                        )
                    } finally {
                        notificationJob = null
                        state = state.copy(notifying = false)
                    }
                }
            }
        }
    }

    private fun stopNotifications() {
        viewModelScope.launch {
            operationMutex.withLock {
                stopNotificationsNow()
                state = state.copy(status = "Subscription stopped", error = null)
            }
        }
    }

    private suspend fun stopNotificationsNow() {
        val activeJob = notificationJob
        notificationJob = null
        activeJob?.cancelAndJoin()
        state = state.copy(notifying = false)
    }

    private fun observeAdapter(activeAdapter: Adapter?) {
        adapterEventsJob?.cancel()
        if (activeAdapter == null) return

        adapterEventsJob = viewModelScope.launch {
            launch {
                activeAdapter.onScanActive.collect { active ->
                    state = state.copy(isScanning = active)
                }
            }
            launch { activeAdapter.onScanFound.collect(::upsertPeripheral) }
            launch { activeAdapter.onScanUpdated.collect(::upsertPeripheral) }
        }
    }

    private fun observeConnection(peripheral: Peripheral) {
        connectionEventsJob?.cancel()
        connectionEventsJob = viewModelScope.launch {
            peripheral.onConnectionActive.collect { connected ->
                state = state.copy(
                    connected = connected,
                    status = if (connected) state.status else "Peripheral disconnected",
                    services = if (connected) state.services else emptyList(),
                    selectedCharacteristic = if (connected) state.selectedCharacteristic else null,
                    mtu = if (connected) state.mtu else null
                )
                if (!connected) notificationJob?.cancel()
            }
        }
    }

    private fun upsertPeripheral(peripheral: Peripheral) {
        val summary = PeripheralSummary(
            name = peripheral.identifier,
            address = peripheral.address.toString(),
            rssi = peripheral.rssi,
            connectable = peripheral.isConnectable,
            paired = peripheral.isPaired
        )
        peripheralsByAddress[summary.address] = peripheral
        val summaries = (state.peripherals.filterNot { it.address == summary.address } + summary)
            .sortedByDescending { it.rssi }
        state = state.copy(
            peripherals = summaries,
            selectedPeripheral = state.selectedPeripheral?.let { selected ->
                summaries.firstOrNull { it.address == selected.address } ?: selected
            }
        )
    }

    private fun launchOperation(
        progressStatus: String,
        successStatus: String,
        block: suspend () -> Unit
    ) {
        viewModelScope.launch {
            operationMutex.withLock {
                state = state.copy(busy = true, status = progressStatus, error = null)
                try {
                    block()
                    state = state.copy(status = successStatus)
                } catch (error: CancellationException) {
                    throw error
                } catch (error: Exception) {
                    state = state.copy(
                        error = error.message ?: error::class.java.simpleName,
                        status = "Operation failed"
                    )
                } finally {
                    state = state.copy(busy = false)
                }
            }
        }
    }
}

sealed interface ExplorerAction {
    data object RefreshAdapter : ExplorerAction
    data object ToggleScan : ExplorerAction
    data object Connect : ExplorerAction
    data object Disconnect : ExplorerAction
    data object RefreshServices : ExplorerAction
    data object Read : ExplorerAction
    data object WriteRequest : ExplorerAction
    data object WriteCommand : ExplorerAction
    data object ToggleNotifications : ExplorerAction
    data object ClosePeripheral : ExplorerAction
    data object ClearError : ExplorerAction
    data class SelectPeripheral(val address: String) : ExplorerAction
    data class SelectCharacteristic(val service: String, val characteristic: String) : ExplorerAction
    data class SetWriteHex(val value: String) : ExplorerAction
    data class SetSearchQuery(val value: String) : ExplorerAction
}

data class BleUiState(
    val plainBackend: Boolean,
    val bluetoothEnabled: Boolean = false,
    val hasAdapter: Boolean = false,
    val adapterName: String = "",
    val adapterAddress: String = "",
    val isScanning: Boolean = false,
    val busy: Boolean = false,
    val searchQuery: String = "",
    val peripherals: List<PeripheralSummary> = emptyList(),
    val selectedPeripheral: PeripheralSummary? = null,
    val connected: Boolean = false,
    val mtu: Int? = null,
    val services: List<Service> = emptyList(),
    val selectedCharacteristic: CharacteristicTarget? = null,
    val writeHex: String = "",
    val readValue: String? = null,
    val notifying: Boolean = false,
    val notifications: List<String> = emptyList(),
    val status: String = "Ready",
    val error: String? = null
)

data class PeripheralSummary(
    val name: String,
    val address: String,
    val rssi: Int,
    val connectable: Boolean,
    val paired: Boolean
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
        if (canRead) add("Read")
        if (canWriteRequest) add("Write")
        if (canWriteCommand) add("Write without response")
        if (canNotify) add("Notify")
        if (canIndicate) add("Indicate")
    }
    return if (capabilities.isEmpty()) "No common GATT operations" else capabilities.joinToString(" · ")
}

private data class AdapterInfo(
    val adapter: Adapter?,
    val bluetoothEnabled: Boolean,
    val name: String,
    val address: String
)

private data class ConnectionInfo(
    val connected: Boolean,
    val services: List<Service>,
    val mtu: Int?
)
