// The Android plugin for Kotlin Multiplatform cannot build native code, so the JNI library is built here and
// consumed by the androidMain source set of :simplektble.
//
// NOTE: The SimpleBLE Android backend calls into the simpledroidbridge classes. They are compiled in here because
// the simpledroidbridge build uses an older Android Gradle plugin, and one build cannot mix plugin versions.

plugins {
    alias(libs.plugins.androidLibrary)
}

group = "org.simpleble"
version = rootProject.version

val plain = providers.gradleProperty("simplektble.plain").map(String::toBoolean).getOrElse(false)

android {
    namespace = "org.simplektble.androidnative"
    compileSdk = 35
    ndkVersion = "29.0.14206865"

    defaultConfig {
        minSdk = 31

        ndk {
            abiFilters += setOf("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
        }

        externalNativeBuild {
            cmake {
                if (plain) arguments += "-DSIMPLEBLE_PLAIN=ON"
            }
        }
    }

    externalNativeBuild {
        cmake {
            path("../cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    sourceSets {
        getByName("main") {
            java.directories += "../../simpledroidbridge/src/main/java"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}
