group = "org.simplefltble"
version = file("../pubspec.yaml").readLines().first { it.startsWith("version:") }.substringAfter(":").trim()

buildscript {
    repositories {
        google()
        mavenCentral()
    }
    dependencies {
        classpath("com.android.tools.build:gradle:9.1.0")
    }
}

plugins {
    id("com.android.library")
}

android {
    namespace = "org.simpledartble"
    compileSdk = 36

    defaultConfig {
        minSdk = 31
        consumerProguardFiles("consumer-rules.pro")
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    // NOTE: SimpleBLE's Android backend calls into the simpledroidbridge classes, so the app needs them. The published
    // package carries a copy in bridge/, which simpledartble/tool/prepublish.dart makes.
    val repository = file("../../simpledroidbridge/src/main/java")
    sourceSets {
        getByName("main") {
            java.srcDir(if (repository.exists()) repository else file("bridge"))
        }
    }
}
