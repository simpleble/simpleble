import org.gradle.internal.os.OperatingSystem
import java.util.zip.ZipFile

plugins {
    id("java-library")
    id("com.vanniktech.maven.publish") version "0.34.0"
}

group = "org.simpleble"
version = file("../../VERSION").readText().trim()

repositories {
    mavenCentral()
}

java {
    toolchain {
        languageVersion.set(JavaLanguageVersion.of(17))
    }
}

mavenPublishing {
    coordinates(group.toString(), "simplejavable", version.toString())
    publishToMavenCentral()
    if (
        providers.gradleProperty("signingInMemoryKey").orNull?.isNotBlank() == true ||
        providers.gradleProperty("signing.secretKeyRingFile").orNull?.isNotBlank() == true
    ) {
        signAllPublications()
    }

    pom {
        name.set("SimpleJavaBLE")
        description.set("Java bindings for the cross-platform SimpleBLE Bluetooth Low Energy library.")
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

// Native library acquisition options
val nativeLibPath: String? by project // -PnativeLibPath=...
val buildFromCMake: String? by project // -PbuildFromCMake (presence is what matters)

// Build native libraries using CMake
tasks.register<Exec>("generateCMake") {
    val cmakePath = "../cpp" // Default CMake location
    val cmakeBuildPath = layout.buildDirectory.dir("build_cpp").get().asFile
    workingDir(cmakePath)
    commandLine(
        "cmake",
        "-B", cmakeBuildPath.absolutePath,
        "-DCMAKE_BUILD_TYPE=Release"
    )
}

tasks.register<Exec>("buildNativeCMake") {
    dependsOn("generateCMake")
    val cmakePath = "../cpp" // Default CMake location
    val cmakeBuildPath = layout.buildDirectory.dir("build_cpp").get().asFile
    workingDir(cmakePath)
    commandLine(
        "cmake",
        "--build", cmakeBuildPath.absolutePath,
        "--config", "Release"
    )
}

// Add native libraries to jar based on the selected mode
tasks.jar {
    // TODO: Remove this once main class is not needed.
    manifest {
        attributes["Main-Class"] = "org.simplejavable.Main"
    }

    from(file("../../LICENSE.md")) {
        into("META-INF")
    }

    buildFromCMake?.let {
        // Build from CMake when explicitly requested
        dependsOn("buildNativeCMake")
        val cmakeBuildOutputPath = layout.buildDirectory.dir("build_cpp/lib").get().asFile
        val currentArch = System.getProperty("os.arch").let { arch ->
            when {
                arch.contains("amd64") || arch.contains("x86_64") -> "x64"
                arch.contains("aarch64") -> "aarch64"
                arch.contains("x86") || arch.contains("i386") || arch.contains("i686") -> "x86"
                else -> error("Unsupported architecture: $arch")
            }
        }
        from(cmakeBuildOutputPath) {
            include("**/*.so", "**/*.dll", "**/*.dylib")
            into("native/$currentArch")
        }
    } ?: nativeLibPath?.let { path ->
        // Use local path approach when CMake build not requested
        // Assumes the directory structure within 'path' matches the desired 'native/<arch>' structure
        val nativeLibDir = file(path)
        if (nativeLibDir.isDirectory) {
            from(nativeLibDir) {
                include("**/*.so", "**/*.dll", "**/*.dylib")
                into("native")
            }
        } else {
             logger.warn("nativeLibPath '$path' provided is not a directory. Cannot include native libraries.")
        }
    } ?: logger.warn("Please provide -PnativeLibPath or use -PbuildFromCMake to build from CMake. The generated JAR will not contain native libraries.")
}

val verifyReleasePublication by tasks.registering {
    group = "publishing"
    description = "Verifies the Maven Central publication without uploading it."
    dependsOn("jar", "sourcesJar", "plainJavadocJar", "generatePomFileForMavenPublication")

    doLast {
        val publicationVersion = project.version.toString()
        check(publicationVersion.matches(Regex("""\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?"""))) {
            "Release version '$publicationVersion' is not a valid semantic version."
        }
        val publicationFiles = listOf(
            layout.buildDirectory.file("libs/simplejavable-$publicationVersion.jar").get().asFile,
            layout.buildDirectory.file("libs/simplejavable-$publicationVersion-sources.jar").get().asFile,
            layout.buildDirectory.file("libs/simplejavable-$publicationVersion-javadoc.jar").get().asFile,
            layout.buildDirectory.file("publications/maven/pom-default.xml").get().asFile,
        )
        publicationFiles.forEach { publicationFile ->
            check(publicationFile.isFile && publicationFile.length() > 0) {
                "Missing or empty publication file: ${publicationFile.absolutePath}"
            }
        }

        val expectedJarEntries = setOf(
            "META-INF/LICENSE.md",
            "native/x64/libsimplejavable.so",
            "native/aarch64/libsimplejavable.so",
            "native/x64/libsimplejavable.dylib",
            "native/aarch64/libsimplejavable.dylib",
            "native/x64/simplejavable.dll",
        )
        ZipFile(publicationFiles.first()).use { jar ->
            val missingEntries = expectedJarEntries.filter { entryName ->
                jar.getEntry(entryName)?.size?.let { it > 0 } != true
            }
            check(missingEntries.isEmpty()) {
                "Publication JAR has missing or empty native libraries or license: ${missingEntries.sorted()}"
            }
        }

        val pom = publicationFiles.last().readText()
        listOf(
            "<groupId>org.simpleble</groupId>",
            "<artifactId>simplejavable</artifactId>",
            "<version>$publicationVersion</version>",
            "<name>SimpleJavaBLE</name>",
            "<licenses>",
            "<developers>",
            "<scm>",
        ).forEach { requiredPomValue ->
            check(requiredPomValue in pom) {
                "Generated POM is missing: $requiredPomValue"
            }
        }

        logger.lifecycle("Verified org.simpleble:simplejavable:$publicationVersion")
    }
}

val verifyMavenCentralCredentials by tasks.registering {
    group = "publishing"
    description = "Checks Maven Central credentials and signing configuration before upload."
    dependsOn(verifyReleasePublication)

    doLast {
        listOf("mavenCentralUsername", "mavenCentralPassword").forEach { propertyName ->
            check(providers.gradleProperty(propertyName).orNull?.isNotBlank() == true) {
                "Missing Gradle property '$propertyName'."
            }
        }
        check(
            providers.gradleProperty("signingInMemoryKey").orNull?.isNotBlank() == true ||
            providers.gradleProperty("signing.secretKeyRingFile").orNull?.isNotBlank() == true
        ) {
            "Missing signingInMemoryKey or signing.secretKeyRingFile Gradle property."
        }
    }
}

val centralPublishingTasks = setOf(
    "prepareMavenCentralPublishing",
    "publishMavenPublicationToMavenCentralRepository",
    "publishAllPublicationsToMavenCentralRepository",
    "publishToMavenCentral",
    "publishAndReleaseToMavenCentral",
)
tasks.matching { it.name in centralPublishingTasks }.configureEach {
    dependsOn(verifyMavenCentralCredentials)
}
