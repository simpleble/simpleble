// build.gradle.kts
plugins {
    kotlin("jvm") version "2.0.20"
}

group = "org.simplejavable"
version = "1.0-SNAPSHOT"

repositories {
    mavenCentral()
}

kotlin {
    jvmToolchain(17)
}

// Native library acquisition options
val nativeLibUrl: String? by project      // -PnativeLibUrl=...
val nativeLibPath: String? by project     // -PnativeLibPath=...
val cmakePath = "../cpp"                  // Default CMake location
val cmakeBuildPath = layout.buildDirectory.dir("build_simplejavable").get().asFile

// Build native libraries using CMake
tasks.register<Exec>("buildNativeCMake") {
    workingDir(cmakePath)
    commandLine("cmake", "-B", cmakeBuildPath.absolutePath, "-DCMAKE_BUILD_TYPE=Release")
    doLast {
        exec {
            workingDir(cmakePath)
            commandLine("cmake", "--build", cmakeBuildPath.absolutePath, "--config", "Release")
        }
    }
}

// Add native libraries to jar based on the selected mode
tasks.jar {
    nativeLibUrl?.let { url ->
        from(uri(url)) { into("native") }
    } ?: nativeLibPath?.let { path ->
        from(path) {
            include("**/*.so", "**/*.dll", "**/*.dylib")
            into("native")
        }
    } ?: run {
        dependsOn("buildNativeCMake")
        from(cmakeBuildPath) {
            include("**/*.so", "**/*.dll", "**/*.dylib")
            into("native")
        }
    }
}