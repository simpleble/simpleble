package org.simpleble.android

import android.Manifest
import android.app.Activity
import android.content.Context
import android.content.pm.PackageManager
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

class SimpleDroidBle private constructor() {
    companion object {
        const val DEFAULT_PERMISSION_REQUEST_CODE = 7101

        private val centralPermissions = arrayOf(
            Manifest.permission.BLUETOOTH_SCAN,
            Manifest.permission.BLUETOOTH_CONNECT
        )
        private val peripheralPermissions = arrayOf(
            Manifest.permission.BLUETOOTH_CONNECT,
            Manifest.permission.BLUETOOTH_ADVERTISE
        )

        @JvmStatic
        val requiredPermissions: Array<String>
            get() = centralPermissions.copyOf()

        @JvmStatic
        val requiredPeripheralPermissions: Array<String>
            get() = peripheralPermissions.copyOf()

        init {
            System.loadLibrary("simpleble-jni")
        }

        internal fun ensureLoaded() = Unit

        @JvmStatic
        fun hasPermissions(context: Context): Boolean {
            return hasPermissions(context, centralPermissions)
        }

        @JvmStatic
        fun hasPeripheralPermissions(context: Context): Boolean {
            return hasPermissions(context, peripheralPermissions)
        }

        private fun hasPermissions(context: Context, permissions: Array<String>): Boolean {
            return permissions.all { permission ->
                ContextCompat.checkSelfPermission(
                    context,
                    permission
                ) == PackageManager.PERMISSION_GRANTED
            }
        }

        @JvmStatic
        fun requestPermissions(
            activity: Activity,
            requestCode: Int = DEFAULT_PERMISSION_REQUEST_CODE
        ) {
            requestPermissions(activity, centralPermissions, requestCode)
        }

        @JvmStatic
        fun requestPeripheralPermissions(
            activity: Activity,
            requestCode: Int = DEFAULT_PERMISSION_REQUEST_CODE
        ) {
            requestPermissions(activity, peripheralPermissions, requestCode)
        }

        private fun requestPermissions(activity: Activity, permissions: Array<String>, requestCode: Int) {
            val missingPermissions = permissions.filterNot { permission ->
                ContextCompat.checkSelfPermission(activity, permission) == PackageManager.PERMISSION_GRANTED
            }

            if (missingPermissions.isNotEmpty()) {
                ActivityCompat.requestPermissions(
                    activity,
                    missingPermissions.toTypedArray(),
                    requestCode
                )
            }
        }

        @JvmStatic
        fun getVersion(): String {
            return BuildConfig.VERSION_NAME
        }
    }
}
