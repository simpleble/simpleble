package org.simpleble.android

import android.Manifest
import android.app.Activity
import android.content.Context
import android.content.pm.PackageManager
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.lang.ref.WeakReference

class SimpleDroidBle {
    companion object {
        const val DEFAULT_PERMISSION_REQUEST_CODE = 7101

        var contextReference: WeakReference<Context>? = null
        private val context: Context
            get() {
                return contextReference?.get()
                    ?: throw IllegalStateException("SimpleDroidBle.initialize(context) must be called before using permission helpers.")
            }

        @JvmStatic
        val requiredPermissions: Array<String> = arrayOf(
            Manifest.permission.BLUETOOTH_SCAN,
            Manifest.permission.BLUETOOTH_CONNECT
        )

        @JvmStatic
        fun initialize(context: Context) {
            contextReference = WeakReference(context.applicationContext)
        }

        @JvmStatic
        val hasPermissions: Boolean
            get() = hasPermissions(context)

        init {
            System.loadLibrary("simpleble-jni")
        }

        @JvmStatic
        fun hasPermissions(context: Context): Boolean {
            return requiredPermissions.all { permission ->
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
            initialize(activity)
            val missingPermissions = requiredPermissions.filterNot { permission ->
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
        fun requestPermissions() {
            requestPermissions(context as? Activity
                ?: throw IllegalStateException("Use requestPermissions(activity) when SimpleDroidBle was initialized with an application context."))
        }

        @JvmStatic
        fun handleOnRequestPermissionsResult(
            requestCode: Int,
            permissions: Array<out String>,
            grantResults: IntArray
        ): Boolean {
            if (requestCode != DEFAULT_PERMISSION_REQUEST_CODE || permissions.size != grantResults.size) return false

            return requiredPermissions.all { permission ->
                val permissionIndex = permissions.indexOf(permission)
                permissionIndex >= 0 && grantResults[permissionIndex] == PackageManager.PERMISSION_GRANTED
            }
        }

        @JvmStatic
        fun getVersion(): String {
            return "1.0.0"
        }
    }
}
