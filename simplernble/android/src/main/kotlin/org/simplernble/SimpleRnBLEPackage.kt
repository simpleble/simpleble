package org.simplernble

import com.facebook.react.BaseReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.model.ReactModuleInfoProvider
import com.margelo.nitro.simplernble.SimpleRnBLEOnLoad

// Loads the native library when React Native autolinks the package. The
// module itself is a Nitro Hybrid Object, so the package has no native modules.
class SimpleRnBLEPackage : BaseReactPackage() {
  override fun getModule(name: String, reactContext: ReactApplicationContext): NativeModule? = null

  override fun getReactModuleInfoProvider(): ReactModuleInfoProvider = ReactModuleInfoProvider { emptyMap() }

  companion object {
    init {
      SimpleRnBLEOnLoad.initializeNative()
    }
  }
}
