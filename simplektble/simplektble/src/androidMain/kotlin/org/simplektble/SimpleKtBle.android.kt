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

private val peripheralPermissions = arrayOf(
    Manifest.permission.BLUETOOTH_CONNECT,
    Manifest.permission.BLUETOOTH_ADVERTISE
)

/** The runtime permissions Android requires before scanning and connecting. */
public val SimpleKtBle.requiredPermissions: Array<String>
    get() = centralPermissions.copyOf()

/** The runtime permissions Android requires before hosting a GATT server and advertising. */
public val SimpleKtBle.requiredPeripheralPermissions: Array<String>
    get() = peripheralPermissions.copyOf()

public fun SimpleKtBle.hasPermissions(context: Context): Boolean {
    return hasPermissions(context, centralPermissions)
}

public fun SimpleKtBle.hasPeripheralPermissions(context: Context): Boolean {
    return hasPermissions(context, peripheralPermissions)
}

/** Asks for the missing [requiredPermissions]. The result arrives in the activity's onRequestPermissionsResult. */
public fun SimpleKtBle.requestPermissions(
    activity: Activity,
    requestCode: Int = DEFAULT_PERMISSION_REQUEST_CODE
) {
    requestPermissions(activity, centralPermissions, requestCode)
}

/** Asks for the missing [requiredPeripheralPermissions]. The result arrives in the activity's onRequestPermissionsResult. */
public fun SimpleKtBle.requestPeripheralPermissions(
    activity: Activity,
    requestCode: Int = DEFAULT_PERMISSION_REQUEST_CODE
) {
    requestPermissions(activity, peripheralPermissions, requestCode)
}

private fun hasPermissions(context: Context, permissions: Array<String>): Boolean {
    return permissions.all { permission ->
        ContextCompat.checkSelfPermission(context, permission) == PackageManager.PERMISSION_GRANTED
    }
}

private fun requestPermissions(activity: Activity, permissions: Array<String>, requestCode: Int) {
    val missingPermissions = permissions.filterNot { permission ->
        ContextCompat.checkSelfPermission(activity, permission) == PackageManager.PERMISSION_GRANTED
    }

    if (missingPermissions.isNotEmpty()) {
        ActivityCompat.requestPermissions(activity, missingPermissions.toTypedArray(), requestCode)
    }
}
