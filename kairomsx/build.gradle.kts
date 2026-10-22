plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

extra["kairoProduct"] = "kairomsx"
extra["kairoLabel"] = "KairoMSX"
apply(from = rootProject.file("shared/gradle/kairo-machine-app.gradle"))

// openMSX's data folder with the C-BIOS machines, as tools/stage_openmsx_data.py assembles it, in
// assets/openmsx; MainActivity copies it out before the first game.
val openmsxAssets = layout.buildDirectory.dir("generated/openmsx-assets")
val stageOpenmsxData = tasks.register<Exec>("stageOpenmsxData") {
    val script = rootProject.file("tools/stage_openmsx_data.py")
    inputs.file(script)
    inputs.dir(rootProject.file("third_party/openmsx/share"))
    inputs.dir(rootProject.file("third_party/openmsx/Contrib/cbios"))
    outputs.dir(openmsxAssets)
    val python = if (System.getProperty("os.name").startsWith("Windows")) "python" else "python3"
    commandLine(python, "-I", script.absolutePath, openmsxAssets.get().dir("openmsx").asFile.absolutePath)
}
android.sourceSets.getByName("main").assets.srcDir(openmsxAssets)
tasks.named("preBuild") { dependsOn(stageOpenmsxData) }
