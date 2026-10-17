package org.simplektble

import android.Manifest
import android.app.Activity
import android.content.Context
import android.content.pm.PackageManager
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

private const val DEFAULT_PERMISSION_REQUEST_CODE = 7101

private val centralPermissions = arrayOf(
    Manifest.permission.BLUETOOTH_SCAN,
    Manifest.permission.BLUETOOTH_CONNECT
)

/** The runtime permissions Android requires before scanning and connecting. */
public val SimpleKtBle.requiredPermissions: Array<String>
    get() = centralPermissions.copyOf()

public fun SimpleKtBle.hasPermissions(context: Context): Boolean {
    return centralPermissions.all { permission ->
        ContextCompat.checkSelfPermission(context, permission) == PackageManager.PERMISSION_GRANTED
    }
}

/** Asks for the missing [requiredPermissions]. The result arrives in the activity's onRequestPermissionsResult. */
public fun SimpleKtBle.requestPermissions(
    activity: Activity,
    requestCode: Int = DEFAULT_PERMISSION_REQUEST_CODE
) {
    val missingPermissions = centralPermissions.filterNot { permission ->
        ContextCompat.checkSelfPermission(activity, permission) == PackageManager.PERMISSION_GRANTED
    }

    if (missingPermissions.isNotEmpty()) {
        ActivityCompat.requestPermissions(activity, missingPermissions.toTypedArray(), requestCode)
    }
}
