import com.vanniktech.maven.publish.MavenPublishBaseExtension

plugins {
    alias(libs.plugins.kotlinMultiplatform) apply false
    alias(libs.plugins.androidKotlinMultiplatformLibrary) apply false
    alias(libs.plugins.androidLibrary) apply false
    alias(libs.plugins.mavenPublish) apply false
}

version = file("../VERSION").readText().trim()

// Settings shared by every published module. Each module sets its own coordinates, name and description.
subprojects {
    plugins.withId("com.vanniktech.maven.publish") {
        extensions.configure<MavenPublishBaseExtension> {
            publishToMavenCentral()
            if (
                providers.gradleProperty("signingInMemoryKey").orNull?.isNotBlank() == true ||
                providers.gradleProperty("signing.secretKeyRingFile").orNull?.isNotBlank() == true
            ) {
                signAllPublications()
            }

            pom {
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

        // A PLAIN build only simulates Bluetooth, so it must never be published.
        val plain = providers.gradleProperty("simplektble.plain").map(String::toBoolean).getOrElse(false)
        tasks.withType<AbstractPublishToMaven>().configureEach {
            doFirst { check(!plain) { "Refusing to publish a build made with -Psimplektble.plain=true." } }
        }
    }
}
