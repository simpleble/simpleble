pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "simplektble-examples"

// The examples build SimpleKtBLE from this checkout.
includeBuild("../../simplektble") {
    dependencySubstitution {
        substitute(module("org.simpleble:simplektble")).using(project(":simplektble"))
    }
}
