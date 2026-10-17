// The Android plugin for Kotlin Multiplatform cannot build native code, so the JNI library is built here and
// consumed by the androidMain source set of :simplektble.
//
// NOTE: The SimpleBLE Android backend calls into the simpledroidbridge classes. They are compiled in here because
// the simpledroidbridge build uses an older Android Gradle plugin, and one build cannot mix plugin versions.

import com.vanniktech.maven.publish.AndroidSingleVariantLibrary
import com.vanniktech.maven.publish.JavadocJar
import com.vanniktech.maven.publish.SourcesJar

plugins {
    alias(libs.plugins.androidLibrary)
    alias(libs.plugins.mavenPublish)
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

mavenPublishing {
    configure(
        AndroidSingleVariantLibrary(
            javadocJar = JavadocJar.Empty(),
            sourcesJar = SourcesJar.Sources(),
            variant = "release",
        )
    )
    coordinates(group.toString(), "simplektble-android-native", version.toString())

    pom {
        name.set("SimpleKtBLE Android native library")
        description.set("The SimpleBLE native library for SimpleKtBLE on Android.")
    }
}

// NOTE: Android apps fail to build when two dependencies ship META-INF/LICENSE.md, so it goes in its own directory.
abstract class LicenseResources : DefaultTask() {
    @get:InputFile
    abstract val license: RegularFileProperty

    @get:OutputDirectory
    abstract val outputDirectory: DirectoryProperty

    @TaskAction
    fun copy() {
        val target = outputDirectory.file("META-INF/simplektble-android-native/LICENSE.md").get().asFile
        target.parentFile.mkdirs()
        license.get().asFile.copyTo(target, overwrite = true)
    }
}

val licenseResources = tasks.register<LicenseResources>("licenseResources") {
    license.set(rootDir.parentFile.resolve("LICENSE.md"))
    outputDirectory.set(layout.buildDirectory.dir("generated/license"))
}

androidComponents {
    onVariants { variant ->
        variant.sources.resources?.addGeneratedSourceDirectory(licenseResources, LicenseResources::outputDirectory)
    }
}
