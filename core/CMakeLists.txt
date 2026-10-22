# The MSX machine on openMSX (third_party/openmsx), driven one frame at a time, for the Android
# app and for host tests.
cmake_minimum_required(VERSION 3.22.1)
project(kairomsx_core LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# The emulator runs in real time only optimized, so the core is optimized in debug builds too.
add_compile_options(-O3)

include("${CMAKE_CURRENT_LIST_DIR}/deps.cmake")

# openMSX's emulation code, as its makefiles select it (every source but tests, main.cc,
# laserdisc and ALSA MIDI), without what draws, plays or reads input through SDL or OpenGL: the GL
# renderer and scalers, the ImGui windows, the on-screen display and the SDL video and sound
# drivers, and the real-time pacing, as KairoMSX's host paces frames. KairoMSX's replacements are
# in kairo/: classes of the same names compiled instead of openMSX's, with stand-in headers for the
# ones that live outside the folder of the code that includes them, plus the two source patches
# in tools/local_patches.py for the ones that do not.
set(OPENMSX "${THIRD_PARTY}/openmsx/src")
file(GLOB_RECURSE OPENMSX_SOURCES CONFIGURE_DEPENDS "${OPENMSX}/*.cc")
list(FILTER OPENMSX_SOURCES EXCLUDE REGEX "Test\.cc$")
list(FILTER OPENMSX_SOURCES EXCLUDE REGEX "/src/(main\.cc|unittest/|laserdisc/|video/ld/|imgui/|3rdparty/)")
list(FILTER OPENMSX_SOURCES EXCLUDE REGEX "/serial/MidiSessionALSA\.cc$")
list(FILTER OPENMSX_SOURCES EXCLUDE REGEX "/video/(GL[A-Za-z]*|OffScreenSurface|PostProcessor|VisibleSurface|SDLVideoSystem|RendererFactory)\.cc$")
list(FILTER OPENMSX_SOURCES EXCLUDE REGEX "/video/scalers/GL[A-Za-z]*\.cc$")
list(FILTER OPENMSX_SOURCES EXCLUDE REGEX "/console/(OSD[A-Za-z]*|TTFFont)\.cc$")
list(FILTER OPENMSX_SOURCES EXCLUDE REGEX "/sound/SDLSoundDriver\.cc$")
list(FILTER OPENMSX_SOURCES EXCLUDE REGEX "/src/RealTime\.cc$")

# Dear ImGui's core, without a back end or fonts: the settings code keeps openMSX's ImGui
# shortcuts.
set(IMGUI "${THIRD_PARTY}/openmsx/src/3rdparty/imgui")
add_library(kairo_imgui STATIC
    "${IMGUI}/imgui.cc" "${IMGUI}/imgui_draw.cc" "${IMGUI}/imgui_tables.cc"
    "${IMGUI}/imgui_widgets.cc" "${IMGUI}/imgui_stdlib.cc")
# openMSX's imconfig.h converts between ImGui's vectors and its own.
target_include_directories(kairo_imgui PUBLIC "${IMGUI}" "${OPENMSX}/video" "${OPENMSX}/utils")
target_compile_definitions(kairo_imgui PUBLIC KAIRO_OPENMSX)

add_library(kairo_openmsx STATIC ${OPENMSX_SOURCES} "${OPENMSX}/imgui/Shortcuts.cc"
    kairo/ImGuiManager.cc kairo/KeyChords.cc kairo/KairoHost.cc kairo/KairoOutputSurface.cc kairo/KairoSoundDriver.cc
    kairo/KairoVideoSystem.cc kairo/PostProcessor.cc kairo/RealTime.cc kairo/RendererFactory.cc)
# openMSX's build flags for clang: its tables are computed at compile time.
target_compile_options(kairo_openmsx PRIVATE -fconstexpr-steps=2000000
    # dynarray.hh uses std::size without including <iterator>, which GCC's headers bring in.
    "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include iterator>")
# KairoMSX's replacements come first, so its headers stand in for the excluded ones.
target_include_directories(kairo_openmsx PUBLIC "${CMAKE_CURRENT_LIST_DIR}/kairo" "${CMAKE_CURRENT_LIST_DIR}/generated")
foreach(dir . cassette commands config console cpu debugger events fdc file ide input memory resource
            security serial settings sound thread utils video video/scalers video/v9990 video/ld imgui)
    target_include_directories(kairo_openmsx PUBLIC "${OPENMSX}/${dir}")
endforeach()
target_link_libraries(kairo_openmsx PUBLIC kairo_imgui kairo_tcl kairo_png kairo_zlib SDL2-static)
if(WIN32)
    # The Windows socket authentication of openMSX's control connection.
    target_link_libraries(kairo_openmsx PUBLIC secur32)
endif()

# The frame-driven machine (msx_machine.h).
add_library(kairomsx_core STATIC msx_machine.cpp)
target_include_directories(kairomsx_core PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(kairomsx_core PUBLIC kairo_openmsx)

if(NOT ANDROID)
    add_executable(boot_test test/boot_test.cpp)
    target_link_libraries(boot_test PRIVATE kairomsx_core)
    if(WIN32)
        target_link_options(boot_test PRIVATE -static)
    endif()
endif()
