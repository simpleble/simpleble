package org.simpleble.examples.android.views

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.test.assertWidthIsEqualTo
import androidx.compose.ui.test.getUnclippedBoundsInRoot
import androidx.compose.ui.test.junit4.createComposeRule
import androidx.compose.ui.test.onAllNodesWithTag
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.onRoot
import androidx.compose.ui.test.performScrollToIndex
import androidx.compose.ui.unit.dp
import org.junit.Rule
import org.junit.Test
import org.simpleble.examples.android.ui.SimpleBleExplorerTheme

class ExplorerLayoutTest {
    @get:Rule
    val compose = createComposeRule()

    @Test
    fun scanCardsFillTheContentWidth() {
        compose.setContent {
            SimpleBleExplorerTheme {
                ExplorerScreen(ExplorerPreviewFixtures.edgeCaseScan, onAction = {}, onRequestBluetooth = {})
            }
        }

        val rootBounds = compose.onRoot().getUnclippedBoundsInRoot()
        val contentWidth = rootBounds.right - rootBounds.left - 40.dp
        compose.onNodeWithTag("adapter-card").assertWidthIsEqualTo(contentWidth)
        compose.onAllNodesWithTag("peripheral-card")[0].assertWidthIsEqualTo(contentWidth)
    }

    @Test
    fun detailCardsFillTheContentWidth() {
        compose.setContent {
            SimpleBleExplorerTheme {
                ExplorerScreen(ExplorerPreviewFixtures.layoutDetail, onAction = {}, onRequestBluetooth = {})
            }
        }

        val rootBounds = compose.onRoot().getUnclippedBoundsInRoot()
        val contentWidth = rootBounds.right - rootBounds.left - 40.dp
        compose.onNodeWithTag("connection-card").assertWidthIsEqualTo(contentWidth)
        compose.onNodeWithTag("service-card").assertWidthIsEqualTo(contentWidth)
        compose.onNodeWithTag("peripheral-content").performScrollToIndex(3)
        compose.onNodeWithTag("characteristic-tools").assertWidthIsEqualTo(contentWidth)
    }

    @Test
    fun previewCatalogStatesRender() {
        var state by mutableStateOf(ExplorerPreviewFixtures.capturedScan)
        compose.setContent {
            SimpleBleExplorerTheme {
                ExplorerScreen(state, onAction = {}, onRequestBluetooth = {})
            }
        }

        ExplorerPreviewFixtures.renderStates.forEach { nextState ->
            compose.runOnIdle { state = nextState }
            compose.waitForIdle()
            compose.onRoot().assertExists()
        }
    }
}
