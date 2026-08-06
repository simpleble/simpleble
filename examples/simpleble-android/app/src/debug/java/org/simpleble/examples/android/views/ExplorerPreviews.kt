package org.simpleble.examples.android.views

import android.content.res.Configuration
import androidx.compose.runtime.Composable
import androidx.compose.ui.tooling.preview.Preview
import org.simpleble.android.BluetoothUUID
import org.simpleble.android.Characteristic
import org.simpleble.android.Descriptor
import org.simpleble.android.Service
import org.simpleble.examples.android.ui.SimpleBleExplorerTheme
import org.simpleble.examples.android.viewmodels.BleUiState
import org.simpleble.examples.android.viewmodels.CharacteristicTarget
import org.simpleble.examples.android.viewmodels.PeripheralSummary

internal object ExplorerPreviewFixtures {
    private const val HEART_RATE_SERVICE = "0000180d-0000-1000-8000-00805f9b34fb"
    private const val HEART_RATE_MEASUREMENT = "00002a37-0000-1000-8000-00805f9b34fb"

    private val nordicHrm = PeripheralSummary(
        name = "Nordic_HRM",
        address = "EE:66:9A:62:11:C6",
        rssi = -42,
        connectable = true,
        paired = true
    )

    val capturedScan = BleUiState(
        plainBackend = false,
        bluetoothEnabled = true,
        hasAdapter = true,
        adapterName = "OnePlus 7T",
        adapterAddress = "02:00:00:00:00:00",
        peripherals = listOf(nordicHrm),
        status = "Adapter status refreshed"
    )

    val edgeCaseScan = capturedScan.copy(
        plainBackend = true,
        adapterName = "An unusually long Bluetooth adapter name",
        isScanning = true,
        peripherals = listOf(
            nordicHrm.copy(paired = false),
            PeripheralSummary("", "11:22:33:44:55:66", -67, true, false),
            PeripheralSummary(
                "A peripheral with a name long enough to test wrapping",
                "22:33:44:55:66:77",
                -81,
                true,
                false
            ),
            PeripheralSummary("Beacon", "33:44:55:66:77:88", -101, false, false)
        ),
        status = "Scanning for nearby Bluetooth Low Energy advertisements"
    )

    val bluetoothOff = capturedScan.copy(
        bluetoothEnabled = false,
        peripherals = emptyList(),
        status = "Bluetooth is off"
    )

    val capturedConnection = capturedScan.copy(
        selectedPeripheral = nordicHrm,
        connected = true,
        mtu = 247,
        services = nordicServices(),
        selectedCharacteristic = CharacteristicTarget(
            service = BluetoothUUID(HEART_RATE_SERVICE),
            characteristic = BluetoothUUID(HEART_RATE_MEASUREMENT),
            canRead = false,
            canWriteRequest = false,
            canWriteCommand = false,
            canNotify = true,
            canIndicate = false
        ),
        notifying = true,
        notifications = listOf(
            "16 c8 8f 00 90 00 91 00",
            "16 c9 8f 00 90 00 92 00",
            "16 ca 90 00 91 00 93 00"
        ),
        status = "Listening for notifications"
    )

    val readWrite = capturedConnection.copy(
        selectedCharacteristic = CharacteristicTarget(
            service = BluetoothUUID("00001800-0000-1000-8000-00805f9b34fb"),
            characteristic = BluetoothUUID("00002a00-0000-1000-8000-00805f9b34fb"),
            canRead = true,
            canWriteRequest = true,
            canWriteCommand = false,
            canNotify = false,
            canIndicate = false
        ),
        writeHex = "4e 6f 72 64 69 63 5f 48 52 4d",
        readValue = "4e 6f 72 64 69 63 5f 48 52 4d",
        notifying = false,
        notifications = emptyList(),
        error = "Example operation error with enough detail to exercise the dismiss layout."
    )

    val layoutDetail = capturedConnection.copy(
        services = listOf(capturedConnection.services[2])
    )

    val readWriteDetail = readWrite.copy(
        services = listOf(readWrite.services[0])
    )

    val renderStates = listOf(
        capturedScan,
        edgeCaseScan,
        bluetoothOff,
        capturedConnection,
        layoutDetail,
        readWriteDetail
    )

    private fun nordicServices() = listOf(
        Service(
            "00001800-0000-1000-8000-00805f9b34fb",
            listOf(
                characteristic("00002a00-0000-1000-8000-00805f9b34fb", read = true, write = true),
                characteristic("00002a01-0000-1000-8000-00805f9b34fb", read = true)
            )
        ),
        Service(
            "00001801-0000-1000-8000-00805f9b34fb",
            listOf(characteristic("00002a05-0000-1000-8000-00805f9b34fb", indicate = true))
        ),
        Service(
            HEART_RATE_SERVICE,
            listOf(
                characteristic(HEART_RATE_MEASUREMENT, notify = true),
                characteristic("00002a38-0000-1000-8000-00805f9b34fb", read = true)
            )
        ),
        Service(
            "0000180f-0000-1000-8000-00805f9b34fb",
            listOf(characteristic("00002a19-0000-1000-8000-00805f9b34fb", read = true, notify = true))
        ),
        Service(
            "0000180a-0000-1000-8000-00805f9b34fb",
            listOf(characteristic("00002a29-0000-1000-8000-00805f9b34fb", read = true))
        )
    )

    private fun characteristic(
        uuid: String,
        read: Boolean = false,
        write: Boolean = false,
        writeCommand: Boolean = false,
        notify: Boolean = false,
        indicate: Boolean = false
    ) = Characteristic(
        uuid = uuid,
        descriptors = if (notify || indicate) {
            listOf(Descriptor("00002902-0000-1000-8000-00805f9b34fb"))
        } else {
            emptyList()
        },
        canRead = read,
        canWriteRequest = write,
        canWriteCommand = writeCommand,
        canNotify = notify,
        canIndicate = indicate
    )
}

@Preview(name = "Nordic HRM scan", group = "Captured hardware", showBackground = true, widthDp = 390, heightDp = 844)
@Composable
private fun CapturedScanPreview() = ExplorerPreview(ExplorerPreviewFixtures.capturedScan)

@Preview(
    name = "Narrow screen and large text",
    group = "Edge cases",
    showBackground = true,
    widthDp = 320,
    heightDp = 1000,
    fontScale = 1.3f
)
@Composable
private fun EdgeCaseScanPreview() = ExplorerPreview(ExplorerPreviewFixtures.edgeCaseScan)

@Preview(name = "Bluetooth off", group = "States", showBackground = true, widthDp = 390, heightDp = 844)
@Composable
private fun BluetoothOffPreview() = ExplorerPreview(ExplorerPreviewFixtures.bluetoothOff)

@Preview(name = "Heart rate notifications", group = "Captured hardware", showBackground = true, widthDp = 390, heightDp = 1200)
@Composable
private fun CapturedConnectionPreview() = ExplorerPreview(ExplorerPreviewFixtures.capturedConnection)

@Preview(name = "Notification tools", group = "States", showBackground = true, widthDp = 390, heightDp = 1000)
@Composable
private fun NotificationToolsPreview() = ExplorerPreview(ExplorerPreviewFixtures.layoutDetail)

@Preview(
    name = "Read, write and error",
    group = "States",
    showBackground = true,
    widthDp = 390,
    heightDp = 1200,
    uiMode = Configuration.UI_MODE_NIGHT_YES
)
@Composable
private fun ReadWriteDarkPreview() = ExplorerPreview(ExplorerPreviewFixtures.readWriteDetail)

@Preview(name = "Permission request", group = "Permissions", showBackground = true, widthDp = 390, heightDp = 844)
@Composable
private fun PermissionRequestPreview() {
    SimpleBleExplorerTheme {
        PermissionScreen(permissionRequested = false, onRequestPermissions = {}, onOpenSettings = {})
    }
}

@Preview(name = "Permission denied", group = "Permissions", showBackground = true, widthDp = 320, heightDp = 700, fontScale = 1.3f)
@Composable
private fun PermissionDeniedPreview() {
    SimpleBleExplorerTheme {
        PermissionScreen(permissionRequested = true, onRequestPermissions = {}, onOpenSettings = {})
    }
}

@Composable
private fun ExplorerPreview(state: BleUiState) {
    SimpleBleExplorerTheme {
        ExplorerScreen(state = state, onAction = {}, onRequestBluetooth = {})
    }
}
