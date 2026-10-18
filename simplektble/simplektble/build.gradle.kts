import com.vanniktech.maven.publish.JavadocJar
import com.vanniktech.maven.publish.KotlinMultiplatform
import com.vanniktech.maven.publish.SourcesJar
import org.gradle.internal.os.OperatingSystem
import org.jetbrains.kotlin.gradle.ExperimentalKotlinGradlePluginApi
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.jetbrains.kotlin.konan.target.KonanTarget

plugins {
    alias(libs.plugins.kotlinMultiplatform)
    alias(libs.plugins.androidKotlinMultiplatformLibrary)
    alias(libs.plugins.mavenPublish)
}

group = "org.simpleble"
version = rootProject.version

// Builds the native libraries against the PLAIN backend, for the tests.
val plain = providers.gradleProperty("simplektble.plain").map(String::toBoolean).getOrElse(false)
val flavor = if (plain) "plain" else "native"
val repositoryRoot: File = rootDir.parentFile

// Registers the CMake configure and build steps for one native build, and returns the build step.
fun registerCMakeBuild(
    name: String,
    sourceDir: File,
    buildDir: Provider<Directory>,
    arguments: List<String>
): TaskProvider<Exec> {
    val configure = tasks.register<Exec>("configure$name") {
        group = "native"
        // NOTE: Later builds rerun CMake on their own when its inputs change.
        onlyIf { !buildDir.get().file("CMakeCache.txt").asFile.exists() }
        environment("JAVA_HOME", System.getProperty("java.home"))
        commandLine(
            listOf("cmake", "-S", sourceDir.absolutePath, "-B", buildDir.get().asFile.absolutePath) +
                arguments + if (plain) listOf("-DSIMPLEBLE_PLAIN=ON") else emptyList()
        )
    }
    return tasks.register<Exec>("build$name") {
        group = "native"
        dependsOn(configure)
        environment("JAVA_HOME", System.getProperty("java.home"))
        commandLine("cmake", "--build", buildDir.get().asFile.absolutePath, "--config", "Release", "--parallel")
    }
}

kotlin {
    explicitApi()

    compilerOptions {
        freeCompilerArgs.add("-Xexpect-actual-classes")
    }

    @OptIn(ExperimentalKotlinGradlePluginApi::class)
    applyDefaultHierarchyTemplate {
        common {
            group("jni") {
                withJvm()
                withCompilations { it.target.platformType.name == "androidJvm" }
            }
        }
    }

    android {
        namespace = "org.simplektble"
        compileSdk = 35
        minSdk = 31

        withDeviceTestBuilder {
            sourceSetTreeName = "test"
        }.configure {
            instrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        }

        compilerOptions {
            jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_17)
        }
    }

    jvm {
        compilerOptions {
            jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_17)
        }
    }

    listOf(iosArm64(), iosSimulatorArm64(), macosArm64()).forEach(::configureSimpleCBle)

    sourceSets {
        commonMain.dependencies {
            api(libs.kotlinx.coroutines.core)
            implementation(libs.kotlinx.atomicfu)
        }
        commonTest.dependencies {
            implementation(kotlin("test"))
        }
        androidMain.dependencies {
            implementation(libs.androidx.core.ktx)
            implementation(libs.androidx.startup.runtime)
            implementation(project(":simplektble-android-native"))
        }
        getByName("androidDeviceTest").dependencies {
            implementation(libs.androidx.test.runner)
        }
    }
}

// JVM: the JNI library is built from ../cpp for the host, or taken from a directory holding the libraries of every
// platform as <arch>/<library>, and packaged under native/<arch>/ in the jar.
val jvmNativeLibs = providers.gradleProperty("simplektble.jvmNativeLibs")
val jvmNativeBuildDir = layout.buildDirectory.dir("native/jvm-$flavor")
val hostArch = System.getProperty("os.arch").let { arch ->
    when {
        arch.contains("amd64") || arch.contains("x86_64") -> "x64"
        arch.contains("aarch64") -> "aarch64"
        arch.contains("x86") || arch.contains("i386") || arch.contains("i686") -> "x86"
        else -> error("Unsupported architecture: $arch")
    }
}

val buildJvmNative = registerCMakeBuild(
    "JvmNative",
    file("../cpp"),
    jvmNativeBuildDir,
    buildList {
        add("-DCMAKE_BUILD_TYPE=Release")
        if (OperatingSystem.current().isMacOsX) add("-DCMAKE_OSX_DEPLOYMENT_TARGET=13")
    }
)

tasks.named<ProcessResources>("jvmProcessResources") {
    if (jvmNativeLibs.isPresent) {
        from(jvmNativeLibs) {
            include("*/*simplektble-jni.*")
            into("native")
        }
    } else {
        dependsOn(buildJvmNative)
        from(jvmNativeBuildDir) {
            include("lib/libsimplektble-jni.so", "lib/libsimplektble-jni.dylib", "bin/**/simplektble-jni.dll")
            eachFile { path = "native/$hostArch/$name" }
            includeEmptyDirs = false
        }
    }
}

// Apple: SimpleCBLE and the identity functions in ../cpp/apple are built as static libraries for each target, which
// the cinterop klib embeds.
fun configureSimpleCBle(target: KotlinNativeTarget) {
    val platformArguments = when (target.konanTarget) {
        KonanTarget.IOS_ARM64 -> listOf(
            "-DCMAKE_SYSTEM_NAME=iOS", "-DCMAKE_OSX_SYSROOT=iphoneos",
            "-DCMAKE_OSX_ARCHITECTURES=arm64", "-DCMAKE_OSX_DEPLOYMENT_TARGET=15.8"
        )
        KonanTarget.IOS_SIMULATOR_ARM64 -> listOf(
            "-DCMAKE_SYSTEM_NAME=iOS", "-DCMAKE_OSX_SYSROOT=iphonesimulator",
            "-DCMAKE_OSX_ARCHITECTURES=arm64", "-DCMAKE_OSX_DEPLOYMENT_TARGET=15.8"
        )
        KonanTarget.MACOS_ARM64 -> listOf("-DCMAKE_OSX_ARCHITECTURES=arm64", "-DCMAKE_OSX_DEPLOYMENT_TARGET=13")
        else -> error("Unsupported Apple target ${target.name}")
    }

    val buildDir = layout.buildDirectory.dir("native/${target.name}-$flavor")
    val build = registerCMakeBuild(
        "SimpleCBle${target.name.replaceFirstChar(Char::uppercase)}",
        file("../cpp/apple"),
        buildDir,
        listOf("-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=OFF") + platformArguments
    )

    val cinterop = target.compilations.getByName("main").cinterops.create("simplecble") {
        definitionFile.set(file("src/nativeInterop/cinterop/simplecble.def"))
        packageName("org.simplektble.cinterop")
        includeDirs(
            repositoryRoot.resolve("simplecble/include"),
            buildDir.get().dir("simplecble/export").asFile,
            file("../cpp/apple/include")
        )
        extraOpts("-libraryPath", buildDir.get().dir("lib").asFile.absolutePath)
    }
    tasks.named(cinterop.interopProcessingTaskName) { dependsOn(build) }
}

mavenPublishing {
    configure(KotlinMultiplatform(javadocJar = JavadocJar.Empty(), sourcesJar = SourcesJar.Sources()))
    coordinates(group.toString(), "simplektble", version.toString())

    pom {
        name.set("SimpleKtBLE")
        description.set("Kotlin Multiplatform bindings for SimpleBLE on Android, iOS, macOS, Windows and Linux.")
    }
}

// Every artifact carries the license, as SimpleJavaBLE's jar does.
val license = repositoryRoot.resolve("LICENSE.md")

listOf("jvmJar", "allMetadataJar").forEach { name ->
    tasks.named<Jar>(name) { from(license) { into("META-INF") } }
}

// NOTE: Android apps fail to build when two dependencies ship META-INF/LICENSE.md, so it goes in its own directory.
val androidLicense = tasks.register<Copy>("androidLicense") {
    from(license) { into("META-INF/simplektble") }
    into(layout.buildDirectory.dir("generated/license/android"))
}
kotlin.sourceSets.getByName("androidMain").resources.srcDir(androidLicense)

// The published jar must carry the JNI library of every desktop platform.
val expectedJvmNativeLibraries = listOf(
    "native/x64/libsimplektble-jni.so",
    "native/aarch64/libsimplektble-jni.so",
    "native/x64/libsimplektble-jni.dylib",
    "native/aarch64/libsimplektble-jni.dylib",
    "native/x64/simplektble-jni.dll",
)

val verifyJvmNativeLibraries = tasks.register("verifyJvmNativeLibraries") {
    group = "verification"
    description = "Checks that the jvm jar contains the JNI library of every desktop platform."
    val jar = tasks.named<Jar>("jvmJar").flatMap { it.archiveFile }
    inputs.file(jar)
    doLast {
        val entries = zipTree(jar).matching { include("native/**") }.files.map { file ->
            "native/" + file.parentFile.name + "/" + file.name
        }.toSet()
        val missing = expectedJvmNativeLibraries.filterNot(entries::contains)
        check(missing.isEmpty()) { "The jvm jar is missing $missing. Pass -Psimplektble.jvmNativeLibs=<dir>." }
    }
}

tasks.matching { it.name == "publishJvmPublicationToMavenCentralRepository" }.configureEach {
    dependsOn(verifyJvmNativeLibraries)
}
