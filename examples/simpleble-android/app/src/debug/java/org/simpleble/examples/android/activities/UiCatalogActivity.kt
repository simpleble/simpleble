package org.simpleble.examples.android.activities

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import org.simpleble.examples.android.ui.SimpleBleExplorerTheme
import org.simpleble.examples.android.views.ExplorerPreviewFixtures
import org.simpleble.examples.android.views.ExplorerScreen
import org.simpleble.examples.android.views.PermissionScreen

/** Debug-only host for deterministic emulator screenshots of the Compose preview fixtures. */
class UiCatalogActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val scenario = intent.getStringExtra("scenario")

        setContent {
            SimpleBleExplorerTheme {
                when (scenario) {
                    "permission" -> PermissionScreen(false, {}, {})
                    "permission-denied" -> PermissionScreen(true, {}, {})
                    else -> ExplorerScreen(
                        state = when (scenario) {
                            "captured-scan" -> ExplorerPreviewFixtures.capturedScan
                            "bluetooth-off" -> ExplorerPreviewFixtures.bluetoothOff
                            "captured-connection" -> ExplorerPreviewFixtures.capturedConnection
                            "read-write" -> ExplorerPreviewFixtures.readWriteDetail
                            else -> ExplorerPreviewFixtures.edgeCaseScan
                        },
                        onAction = {},
                        onRequestBluetooth = {}
                    )
                }
            }
        }
    }
}
