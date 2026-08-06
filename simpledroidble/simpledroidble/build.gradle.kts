import com.vanniktech.maven.publish.AndroidSingleVariantLibrary

plugins {
    alias(libs.plugins.androidLibrary)
    alias(libs.plugins.jetbrainsKotlinAndroid)
    id("com.vanniktech.maven.publish")
}

group = "org.simpleble"
version = rootProject.version

android {
    namespace = "org.simpleble.android"
    compileSdk = 35
    ndkVersion = "29.0.14206865"

    defaultConfig {
        minSdk = 31
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        ndk {
            abiFilters += setOf("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
        }

        buildConfigField(
            "String",
            "VERSION_NAME",
            "\"${rootProject.version}\""
        )

        consumerProguardFiles("consumer-rules.pro")
        externalNativeBuild {
            cmake {
                cppFlags("")
            }
        }
    }

    buildTypes {
        create("plain") {
            initWith(getByName("debug"))
            matchingFallbacks += "debug"
            externalNativeBuild.cmake.arguments += "-DSIMPLEBLE_PLAIN=ON"
        }
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    testBuildType = "plain"
    externalNativeBuild {
        cmake {
            path("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
    buildFeatures {
        buildConfig = true
    }
}

mavenPublishing {
    configure(
        AndroidSingleVariantLibrary(
            variant = "release",
            sourcesJar = true,
            publishJavadocJar = true,
        )
    )
    coordinates(group.toString(), "simpledroidble", version.toString())
    publishToMavenCentral()
    if (
        providers.gradleProperty("signingInMemoryKey").orNull?.isNotBlank() == true ||
        providers.gradleProperty("signing.secretKeyRingFile").orNull?.isNotBlank() == true
    ) {
        signAllPublications()
    }

    pom {
        name.set("SimpleDroidBLE")
        description.set("Kotlin bindings for the SimpleBLE Android Bluetooth Low Energy backend.")
        inceptionYear.set("2021")
        url.set("https://github.com/simpleble/simpleble")

        licenses {
            license {
                name.set("Business Source License 1.1")
                url.set("https://github.com/simpleble/simpleble/blob/main/LICENSE.md")
                distribution.set("repo")
            }
        }

        developers {
            developer {
                id.set("kdewald")
                name.set("Kevin Dewald")
                email.set("kevin@simpleble.org")
                organization.set("The California Open Source Company")
                organizationUrl.set("https://californiaopensource.com")
            }
        }

        scm {
            url.set("https://github.com/simpleble/simpleble")
            connection.set("scm:git:https://github.com/simpleble/simpleble.git")
            developerConnection.set("scm:git:ssh://git@github.com/simpleble/simpleble.git")
        }
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    api(libs.kotlinx.coroutines.core)
    implementation("org.simpleble:simpledroidbridge:${project.version}")
    androidTestImplementation(libs.androidx.test.ext.junit)
    androidTestImplementation(libs.androidx.test.runner)
}
