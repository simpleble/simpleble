package org.simplektble

import android.content.Context
import androidx.startup.Initializer
import kotlin.concurrent.Volatile

internal actual fun loadNativeLibrary() {
    System.loadLibrary("simplektble-jni")
}

internal actual fun platformContext(): Any? {
    return checkNotNull(SimpleKtBleInitializer.applicationContext) {
        "SimpleKtBLE has no application context. Keep its androidx.startup initializer enabled."
    }
}

/** Keeps the application context, which hosting a GATT server needs. androidx.startup runs it when the app starts. */
internal class SimpleKtBleInitializer : Initializer<Unit> {
    override fun create(context: Context) {
        applicationContext = context.applicationContext
    }

    override fun dependencies(): List<Class<out Initializer<*>>> = emptyList()

    companion object {
        @Volatile
        var applicationContext: Context? = null
    }
}
