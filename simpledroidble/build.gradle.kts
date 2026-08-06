// Top-level build file where you can add configuration options common to all sub-projects/modules.
plugins {
    alias(libs.plugins.jetbrainsKotlinAndroid) apply false
    alias(libs.plugins.androidLibrary) apply false
    id("com.vanniktech.maven.publish") version "0.34.0" apply false
}

version = file("../VERSION").readText().trim()
