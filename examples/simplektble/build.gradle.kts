plugins {
    kotlin("jvm") version "2.3.21"
}

dependencies {
    implementation("org.simpleble:simplektble")
}

kotlin {
    jvmToolchain(17)
}

// One task per example, for example `runScan`. The examples ask for their choices on the console.
listOf("ListAdapters", "ListBackends", "Scan", "Connect", "Read", "Write", "Notify", "Peripheral", "Dongl").forEach { name ->
    tasks.register<JavaExec>("run$name") {
        group = "examples"
        description = "Runs the $name example."
        classpath = sourceSets.main.get().runtimeClasspath
        mainClass.set("org.simplektble.examples.${name.lowercase()}.${name}Kt")
        standardInput = System.`in`
    }
}
