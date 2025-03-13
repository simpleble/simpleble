import org.gradle.internal.os.OperatingSystem

plugins {
    kotlin("jvm") version "2.0.20"
}

group = "org.simplejavable"
version = "v${file("../../VERSION").readText().trim()}"

repositories {
    mavenCentral()
}

kotlin {
    jvmToolchain(17)
}

// Native library acquisition options
val nativeLibUrl: String? by project // -PnativeLibUrl=...
val nativeLibPath: String? by project // -PnativeLibPath=...
val cmakePath = "../cpp" // Default CMake location
val cmakeBuildPath = layout.buildDirectory.dir("build_cpp").get().asFile
val cmakeBuildOutputPath = layout.buildDirectory.dir("build_cpp/lib").get().asFile

// OS and architecture detection for CMake builds
val currentOs = when {
    OperatingSystem.current().isWindows -> "windows"
    OperatingSystem.current().isMacOsX -> "macos"
    OperatingSystem.current().isLinux -> "linux"
    else -> error("Unsupported OS")
}

val currentArch = System.getProperty("os.arch").let { arch ->
    when {
        arch.contains("amd64") || arch.contains("x86_64") -> "x86_64"
        arch.contains("aarch64") -> "aarch64"
        else -> error("Unsupported architecture: $arch")
    }
}

// Build native libraries using CMake
tasks.register<Exec>("buildNativeCMake") {
    workingDir(cmakePath)
    commandLine("cmake",
        "-B", cmakeBuildPath.absolutePath,
        "-DCMAKE_BUILD_TYPE=Release")

    doLast {
        exec {
            workingDir(cmakePath)
            commandLine("cmake", "--build", cmakeBuildPath.absolutePath, "--config", "Release")
        }
    }
}

// Add native libraries to jar based on the selected mode
tasks.jar {
    // TODO: Remove this once main class is not needed.
    manifest {
        attributes["Main-Class"] = "org.simplejavable.Main"
    }

    nativeLibUrl?.let { url ->
        from(uri(url)) {
            into("native")
        }
    } ?: nativeLibPath?.let { path ->
        from(path) {
            include("**/*.so", "**/*.dll", "**/*.dylib")
            into("native")
        }
    } ?: run {
        dependsOn("buildNativeCMake")
        from(cmakeBuildOutputPath) {
            include("**/*.so", "**/*.dll", "**/*.dylib")
            into("native/$currentOs/$currentArch")
            // eachFile {
            //     // Only organize files for CMake build output
            //     path = "native/$currentOs/$currentArch/$name"
            // }
        }
    }
}