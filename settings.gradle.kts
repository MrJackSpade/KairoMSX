apply(from = "shared/gradle/android-settings.gradle")

rootProject.name = "KairoMSX"
include(":kairomsx", ":frontend")
project(":frontend").projectDir = file("shared/frontend")
