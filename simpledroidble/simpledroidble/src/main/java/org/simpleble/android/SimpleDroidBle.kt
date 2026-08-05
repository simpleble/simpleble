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

        private val bluetoothPermissions = arrayOf(
            Manifest.permission.BLUETOOTH_SCAN,
            Manifest.permission.BLUETOOTH_CONNECT
        )

        @JvmStatic
        val requiredPermissions: Array<String>
            get() = bluetoothPermissions.copyOf()

        init {
            System.loadLibrary("simpleble-jni")
        }

        internal fun ensureLoaded() = Unit

        @JvmStatic
        fun hasPermissions(context: Context): Boolean {
            return bluetoothPermissions.all { permission ->
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
            val missingPermissions = bluetoothPermissions.filterNot { permission ->
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
