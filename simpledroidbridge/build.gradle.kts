import com.vanniktech.maven.publish.AndroidSingleVariantLibrary

plugins {
    id("com.android.library") version "8.7.1"
    id("com.vanniktech.maven.publish") version "0.34.0"
}

group = "org.simpleble"
version = file("../VERSION").readText().trim()

android {
    namespace = "org.simpleble.android.bridge"
    compileSdk = 31

    defaultConfig {
        minSdk = 31
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_9
        targetCompatibility = JavaVersion.VERSION_1_9
    }
    buildTypes {
        getByName("debug") {
            isJniDebuggable = true
        }
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
    coordinates(group.toString(), "simpledroidbridge", version.toString())
    publishToMavenCentral()
    if (
        providers.gradleProperty("signingInMemoryKey").orNull?.isNotBlank() == true ||
        providers.gradleProperty("signing.secretKeyRingFile").orNull?.isNotBlank() == true
    ) {
        signAllPublications()
    }

    pom {
        name.set("SimpleDroidBridge")
        description.set("Android Java callback bridge used by SimpleBLE and SimpleDroidBLE.")
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
