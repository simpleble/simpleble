plugins {
    alias(libs.plugins.androidApplication)
    alias(libs.plugins.jetbrainsKotlinAndroid)
}

android {
    namespace = "org.simpleble.examples.android"
    compileSdk = 35

    defaultConfig {
        applicationId = "org.simpleble.examples.android"
        minSdk = 31
        targetSdk = 35
        versionCode = 1
        versionName = "1.0.1"
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        buildConfigField("boolean", "PLAIN_BACKEND", "false")

    }

    buildTypes {
        create("plain") {
            initWith(getByName("debug"))
            matchingFallbacks += "debug"
            applicationIdSuffix = ".plain"
            versionNameSuffix = "-plain"
            buildConfigField("boolean", "PLAIN_BACKEND", "true")
        }
        release {
            isMinifyEnabled = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    testBuildType = "plain"
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
    buildFeatures {
        compose = true
        buildConfig = true
    }
    composeOptions {
        kotlinCompilerExtensionVersion = "1.5.11"
    }
    sourceSets.getByName("plain") {
        java.srcDir("src/debug/java")
        manifest.srcFile("src/debug/AndroidManifest.xml")
    }
}

dependencies {

    implementation(libs.androidx.core.ktx)
    implementation(libs.material.components)

    implementation(libs.ui)
    implementation(libs.ui.tooling.preview)
    implementation(libs.foundation)
    implementation(libs.material3)
    implementation(libs.activity.ktx)
    implementation(libs.activity.compose)
    implementation(libs.lifecycle.viewmodel.ktx)

    //noinspection UseTomlInstead
    implementation("org.simpleble:simpledroidble:${file("../../../VERSION").readText().trim()}")

    debugImplementation(libs.ui.tooling)
    debugImplementation(libs.ui.test.manifest)
    add("plainImplementation", libs.ui.tooling)
    add("plainImplementation", libs.ui.test.manifest)
    androidTestImplementation(libs.ui.test.junit4)
    testImplementation(libs.junit)
}
