package org.simpleble.examples.android.activities

import android.bluetooth.BluetoothAdapter
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.Uri
import android.os.Bundle
import android.provider.Settings
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.core.content.ContextCompat
import org.simpleble.android.SimpleDroidBle
import org.simpleble.examples.android.BuildConfig
import org.simpleble.examples.android.ui.SimpleBleExplorerTheme
import org.simpleble.examples.android.viewmodels.BluetoothViewModel
import org.simpleble.examples.android.views.ExplorerScreen
import org.simpleble.examples.android.views.PermissionScreen

class MainActivity : ComponentActivity() {
    private val bluetoothViewModel: BluetoothViewModel by viewModels()
    private var hasPermissions by mutableStateOf(BuildConfig.PLAIN_BACKEND)
    private var permissionRequested by mutableStateOf(false)
    private val bluetoothStateReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent?.action == BluetoothAdapter.ACTION_STATE_CHANGED && hasPermissions) {
                bluetoothViewModel.loadAdapters()
            }
        }
    }

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) {
        permissionRequested = true
        refreshPermissionState()
        if (hasPermissions) bluetoothViewModel.loadAdapters()
    }

    private val bluetoothEnableLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) {
        bluetoothViewModel.loadAdapters()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        refreshPermissionState()

        setContent {
            SimpleBleExplorerTheme {
                if (hasPermissions) {
                    ExplorerScreen(
                        state = bluetoothViewModel.state,
                        onAction = bluetoothViewModel::onAction,
                        onOpenPeripheralMode = {
                            startActivity(Intent(this, PeripheralActivity::class.java))
                        },
                        onRequestBluetooth = {
                            bluetoothEnableLauncher.launch(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE))
                        }
                    )
                } else {
                    PermissionScreen(
                        permissionRequested = permissionRequested,
                        onRequestPermissions = {
                            permissionLauncher.launch(SimpleDroidBle.requiredPermissions)
                        },
                        onOpenSettings = ::openAppSettings
                    )
                }
            }
        }
    }

    override fun onResume() {
        super.onResume()
        refreshPermissionState()
        if (hasPermissions) bluetoothViewModel.loadAdapters()
    }

    override fun onStart() {
        super.onStart()
        ContextCompat.registerReceiver(
            this,
            bluetoothStateReceiver,
            IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED),
            ContextCompat.RECEIVER_NOT_EXPORTED
        )
    }

    override fun onStop() {
        unregisterReceiver(bluetoothStateReceiver)
        super.onStop()
    }

    override fun onPause() {
        bluetoothViewModel.onActivityPaused()
        super.onPause()
    }

    private fun refreshPermissionState() {
        hasPermissions = BuildConfig.PLAIN_BACKEND || SimpleDroidBle.hasPermissions(this)
    }

    private fun openAppSettings() {
        startActivity(
            Intent(
                Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                Uri.fromParts("package", packageName, null)
            )
        )
    }
}
