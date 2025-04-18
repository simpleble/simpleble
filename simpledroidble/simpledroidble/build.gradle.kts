plugins {
    alias(libs.plugins.androidLibrary)
    alias(libs.plugins.jetbrainsKotlinAndroid)
}

val androidExtension = project.extensions.getByType<com.android.build.gradle.LibraryExtension>()
val androidSdkDir = androidExtension.sdkDirectory.absolutePath
val androidNdkDir = androidExtension.ndkDirectory.absolutePath
val cmakePath = androidExtension.cmake.get().path
val cmakeVersion = androidExtension.cmake.get().version

val simpleDroidBleCompileSdkVersion = 35
val simpleDroidBleMinSdkVersion = 31


// Define the path to the root simpleble CMake project
val simplebleRootPath = rootProject.projectDir.resolve("../simpleble").canonicalPath
// Define where the custom CMake task will build the AAR
val simplebleBuildPath = layout.buildDirectory.dir("build_simpleble_root").get().asFile
// Define the expected output path of the AAR from the custom build
val simplebleAarPath = layout.buildDirectory.dir("build_simpleble_root/aar_output/simpleble-bridge.aar").get().asFile.absolutePath


// Task to build the simpleble-bridge.aar using the root CMakeLists.txt
tasks.register<Exec>("buildSimpleBleAar") {
    description = "Builds simpleble-bridge.aar from the root simpleble project"
    group = "build"

    // // Define inputs and outputs for Gradle's up-to-date checking
    // inputs.dir(simplebleRootPath)
    // // Declare dependency on NDK and SDK directories for up-to-date checks
    // inputs.dir(sdkDir)
    // inputs.dir(ndkDir)
    outputs.file(simplebleAarPath)

     // CMake Configuration Step
     commandLine(
         "cmake",
         "-S", simplebleRootPath, // Source directory
         "-B", simplebleBuildPath,
         "-DCMAKE_SYSTEM_NAME=Android",
         "-DCMAKE_ANDROID_NDK=$androidNdkDir",
         "-DCMAKE_SYSTEM_VERSION=$simpleDroidBleMinSdkVersion",
     )

     // CMake Build Step (runs after configuration)
     doLast {
         exec {
             workingDir(simplebleRootPath)
             commandLine(
                 "cmake",
                 "--build", simplebleBuildPath.absolutePath,
                 "--target", "build_aar", // Build the specific AAR target
                 "--config", "Release" // Or Debug
             )
         }
     }
}

// Make the standard build process depend on our custom AAR build task
tasks.named("preBuild") {
    dependsOn("buildSimpleBleAar")
}

android {
    namespace = "org.simpleble.android"
    compileSdk = 35

    defaultConfig {
        minSdk = 31

        consumerProguardFiles("consumer-rules.pro")
        externalNativeBuild {
            cmake {
                cppFlags("")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    externalNativeBuild {
        cmake {
            path("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
    kotlinOptions {
        jvmTarget = "1.8"
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)

    //noinspection UseTomlInstead
    implementation("org.simpleble.android.bridge:simpleble-bridge")

    // // Depend on the AAR file produced by the custom buildSimpleBleAar task
    // implementation(files(simplebleAarPath)) {
    //     // Ensure the dependency resolution waits for the task to complete
    //     builtBy("buildSimpleBleAar")
    // }
}