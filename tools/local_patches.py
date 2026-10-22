#!/usr/bin/env python3
"""Applies KairoMSX's local changes to the third-party snapshots in third_party/.

Usage: local_patches.py [--check]

Each change replaces text that must occur exactly once in its file. A change already applied is
skipped, so the script can run on a fresh import or on the committed tree; --check fails unless
every change is applied. docs/source-import.md describes each change.
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / 'third_party'

# SDL's Android sources: its bridge to its own Java classes, left out with SDL_KAIRO_NO_JAVA (see
# SDL_NO_JAVA), and the NDK's cpufeatures, which SDL only calls for 32-bit ARM's NEON check
# (src/cpuinfo/SDL_cpuinfo.c), so arm64 builds leave it out.
SDL_ANDROID_SOURCES = [
    ('sdl2/CMakeLists.txt',
     '''  list(APPEND SOURCE_FILES ${ANDROID_CORE_SOURCES} ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
  set_property(SOURCE "${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c" APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-declaration-after-statement")
''',
     '''  if(NOT SDL_KAIRO_NO_JAVA) # KairoMSX: no bridge to SDL's Java classes
    list(APPEND SOURCE_FILES ${ANDROID_CORE_SOURCES})
  endif()
  # KairoMSX: only 32-bit ARM's NEON check uses cpufeatures (SDL_cpuinfo.c).
  if(SDL_CPU_ARM32)
    list(APPEND SOURCE_FILES ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
    set_property(SOURCE "${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c" APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-declaration-after-statement")
  endif()
'''),
    ('sdl2/CMakeLists.txt',
     '''if(ANDROID)
  target_include_directories(sdl-build-options INTERFACE "${ANDROID_NDK}/sources/android/cpufeatures")
endif()''',
     '''if(ANDROID AND SDL_CPU_ARM32) # KairoMSX: see the cpufeatures sources above
  target_include_directories(sdl-build-options INTERFACE "${ANDROID_NDK}/sources/android/cpufeatures")
endif()'''),
]

# openMSX draws through OpenGL and plays through SDL. KairoMSX's build replaces those with the
# host's (core/kairo/): the rasterizers include PostProcessor.hh from their own folder, so the
# header itself switches to KairoMSX's PostProcessor, and the mixer creates KairoMSX's sound
# driver where it would create SDL's.
OPENMSX_POSTPROCESSOR = [
    ('openmsx/src/video/PostProcessor.hh',
     '''#ifndef GLPOSTPROCESSOR_HH
#define GLPOSTPROCESSOR_HH
''',
     '''#ifndef GLPOSTPROCESSOR_HH
#define GLPOSTPROCESSOR_HH

#ifdef KAIRO_OPENMSX
#include "KairoPostProcessor.hh" // KairoMSX: the PostProcessor without OpenGL
#else
'''),
    ('openmsx/src/video/PostProcessor.hh',
     '''} // namespace openmsx

#endif // GLPOSTPROCESSOR_HH
''',
     '''} // namespace openmsx

#endif // KAIRO_OPENMSX
#endif // GLPOSTPROCESSOR_HH
'''),
]

OPENMSX_SOUND_DRIVER = [
    ('openmsx/src/sound/Mixer.cc',
     '#include "SDLSoundDriver.hh"\n',
     '''#ifdef KAIRO_OPENMSX
#include "KairoSoundDriver.hh" // KairoMSX: sound goes to the host
#else
#include "SDLSoundDriver.hh"
#endif
'''),
    ('openmsx/src/sound/Mixer.cc',
     '''			driver = std::make_unique<SDLSoundDriver>(''',
     '''#ifdef KAIRO_OPENMSX
			driver = std::make_unique<KairoSoundDriver>(
#else
			driver = std::make_unique<SDLSoundDriver>(
#endif'''),
]

# openMSX builds Dear ImGui with FreeType fonts; KairoMSX builds ImGui's core only for the key
# names of openMSX's settings and draws no ImGui text, so it builds no font loader.
OPENMSX_IMGUI_FREETYPE = [
    ('openmsx/src/3rdparty/imgui/imconfig.h',
     '#define IMGUI_ENABLE_FREETYPE\n',
     '#ifndef KAIRO_OPENMSX // KairoMSX: no ImGui fonts\n#define IMGUI_ENABLE_FREETYPE\n#endif\n'),
]

# ESE_SCC holds its SCSI controller in a unique_ptr to a type its header only declares, and has no
# declared destructor, so every file that destroys an ESE_SCC instantiates the controller's
# deletion where the controller is incomplete. The NDK's libc++ rejects that (as the standard
# requires); the destructor is defined where the controller is complete instead.
OPENMSX_ESE_SCC_DESTRUCTOR = [
    ('openmsx/src/memory/ESE_SCC.hh',
     '\tESE_SCC(DeviceConfig& config, bool withSCSI);\n',
     '\tESE_SCC(DeviceConfig& config, bool withSCSI);\n\t~ESE_SCC() override; // KairoMSX: where MB89352 is complete\n'),
    ('openmsx/src/memory/ESE_SCC.cc',
     '\t// initialized mapper\n\tranges::iota(mapper, byte(0));\n}\n',
     '\t// initialized mapper\n\tranges::iota(mapper, byte(0));\n}\n\nESE_SCC::~ESE_SCC() = default;\n'),
]

# Android's C library declares aligned_alloc only from API level 28; the apps support 26.
# posix_memalign returns the same aligned memory, freed with free() as aligned_alloc's is.
OPENMSX_ANDROID_ALIGNED_ALLOC = [
    ('openmsx/src/utils/MemoryOps.cc',
     '\tvoid* result = std::aligned_alloc(alignment, size);\n#endif\n',
     '#if defined(__ANDROID__) && __ANDROID_API__ < 28 // KairoMSX: no aligned_alloc before API 28\n'
     '\tvoid* result = nullptr;\n'
     '\tif (posix_memalign(&result, alignment, size) != 0) result = nullptr;\n'
     '#else\n'
     '\tvoid* result = std::aligned_alloc(alignment, size);\n'
     '#endif\n'
     '#endif\n'),
]

# SDL's Android port is a bridge to its own Java classes (org.libsdl.app.SDLActivity), with its own
# JNI_OnLoad, and needs SDL's Android video, audio and joystick drivers. In KairoMSX the Kairo
# frontend owns the Java side and the library's JNI_OnLoad, and openMSX starts no SDL subsystem.
# With SDL_KAIRO_NO_JAVA (core/deps.cmake) SDL is built without that bridge (src/core/android),
# and the few calls into it from the rest of SDL take their non-Android paths: files open with
# stdio, threads and getenv do not attach to Java, and there is no game controller mapping file in
# app storage or minimize-on-focus-loss setting.
SDL_NO_JAVA = [
    ('sdl2/src/stdlib/SDL_getenv.c',
     '#if defined(__ANDROID__)\n    /* Make sure variables from the application manifest are available */\n',
     '#if defined(__ANDROID__) && !defined(SDL_KAIRO_NO_JAVA) /* KairoMSX: no SDL Java side */\n'
     '    /* Make sure variables from the application manifest are available */\n'),
    ('sdl2/src/file/SDL_rwops.c',
     '    }\n#if defined(__ANDROID__)\n#ifdef HAVE_STDIO_H\n    /* Try to open the file on the filesystem first */\n',
     '    }\n#if defined(__ANDROID__) && !defined(SDL_KAIRO_NO_JAVA) /* KairoMSX: stdio only */\n'
     '#ifdef HAVE_STDIO_H\n    /* Try to open the file on the filesystem first */\n'),
    ('sdl2/src/thread/pthread/SDL_systhread.c',
     'static void *RunThread(void *data)\n{\n#ifdef __ANDROID__\n',
     'static void *RunThread(void *data)\n{\n#if defined(__ANDROID__) && !defined(SDL_KAIRO_NO_JAVA) /* KairoMSX */\n'),
    ('sdl2/src/video/SDL_video.c',
     '#ifdef __ANDROID__\n    {\n        extern SDL_bool Android_JNI_ShouldMinimizeOnFocusLoss(void);\n',
     '#if defined(__ANDROID__) && !defined(SDL_KAIRO_NO_JAVA) /* KairoMSX */\n'
     '    {\n        extern SDL_bool Android_JNI_ShouldMinimizeOnFocusLoss(void);\n'),
    ('sdl2/src/SDL.c',
     '#if defined(__ANDROID__)\n    extern SDL_bool SDL_IsAndroidTablet(void);\n',
     '#if defined(__ANDROID__) && !defined(SDL_KAIRO_NO_JAVA) /* KairoMSX */\n    extern SDL_bool SDL_IsAndroidTablet(void);\n'),
    ('sdl2/src/joystick/SDL_gamecontroller.c',
     '#if defined(__ANDROID__)\n#include "SDL_system.h"\n#endif\n',
     '#if defined(__ANDROID__)\n#include "SDL_system.h"\n#endif\n'
     '#ifdef SDL_KAIRO_NO_JAVA /* KairoMSX: the API level from the NDK instead of SDL\'s Java side */\n'
     '#include <android/api-level.h>\n'
     '#define SDL_GetAndroidSDKVersion() android_get_device_api_level()\n'
     '#endif\n'),
    ('sdl2/src/joystick/SDL_gamecontroller.c',
     '#if defined(__ANDROID__)\n    return SDL_snprintf(path, size, "%s/controller_map.txt", SDL_AndroidGetInternalStoragePath()) < size;\n',
     '#if defined(__ANDROID__) && !defined(SDL_KAIRO_NO_JAVA) /* KairoMSX */\n'
     '    return SDL_snprintf(path, size, "%s/controller_map.txt", SDL_AndroidGetInternalStoragePath()) < size;\n'),
]

# openMSX's debug builds check that an object saved by ID is not on the stack, assuming the heap
# lies below the stack. On Android arm64 heap pointers carry a tag in their top byte and thread
# stacks are mapped low, so every heap object fails the check and saving a state aborts. Android
# builds leave the check out.
OPENMSX_ANDROID_STACK_CHECK = [
    ('openmsx/src/serialize.cc',
     '\t#ifdef linux\n\tassert("Can\'t serialize ID of object located on the stack" &&\n\t       !addressOnStack(p));\n\t#endif\n\t++lastId;\n\tassert(!polyIdMap.contains(p));\n',
     '\t#if defined(linux) && !defined(__ANDROID__) // KairoMSX: tagged heap pointers on Android\n\tassert("Can\'t serialize ID of object located on the stack" &&\n\t       !addressOnStack(p));\n\t#endif\n\t++lastId;\n\tassert(!polyIdMap.contains(p));\n'),
    ('openmsx/src/serialize.cc',
     '\t#ifdef linux\n\tassert("Can\'t serialize ID of object located on the stack" &&\n\t       !addressOnStack(p));\n\t#endif\n\t++lastId;\n\tauto key',
     '\t#if defined(linux) && !defined(__ANDROID__) // KairoMSX: tagged heap pointers on Android\n\tassert("Can\'t serialize ID of object located on the stack" &&\n\t       !addressOnStack(p));\n\t#endif\n\t++lastId;\n\tauto key'),
]

# openMSX takes the thread that starts it as its main thread for the life of the process. KairoMSX
# runs one openMSX at a time, each on its own thread, so a machine hands the role back when it
# ends (MsxMachine's destructor), and the next one takes it.
OPENMSX_MAIN_THREAD = [
    ('openmsx/src/thread/Thread.hh',
     '\tvoid setMainThread();\n',
     '\tvoid setMainThread();\n\n'
     '\t/** KairoMSX: forgets the main thread, from that thread, when openMSX has shut down, so a\n'
     '\t  * later openMSX can run on another thread.\n'
     '\t  */\n'
     '\tvoid clearMainThread();\n'),
    ('openmsx/src/thread/Thread.cc',
     'bool isMainThread()\n',
     'void clearMainThread()\n{\n\tassert(mainThreadId == std::this_thread::get_id());\n'
     '\tmainThreadId = std::thread::id();\n}\n\nbool isMainThread()\n'),
]

PATCHES = {
    'openmsx-main-thread': OPENMSX_MAIN_THREAD,
    'openmsx-android-stack-check': OPENMSX_ANDROID_STACK_CHECK,
    'sdl-android-sources': SDL_ANDROID_SOURCES,
    'sdl-no-java': SDL_NO_JAVA,
    'openmsx-android-aligned-alloc': OPENMSX_ANDROID_ALIGNED_ALLOC,
    'openmsx-ese-scc-destructor': OPENMSX_ESE_SCC_DESTRUCTOR,
    'openmsx-imgui-freetype': OPENMSX_IMGUI_FREETYPE,
    'openmsx-postprocessor': OPENMSX_POSTPROCESSOR,
    'openmsx-sound-driver': OPENMSX_SOUND_DRIVER,
}


def main():
    check = sys.argv[1:] == ['--check']
    missing = []
    for name, changes in PATCHES.items():
        for rel, old, new in changes:
            path = ROOT / rel
            with open(path, encoding='utf-8', newline='') as f:
                text = f.read()
            if text.count(new) == 1:
                continue
            if check:
                missing.append(f'{name}: {rel}')
                continue
            if text.count(old) != 1:
                raise SystemExit(f'patch {name}: {rel} does not contain the text to replace exactly once')
            with open(path, 'w', encoding='utf-8', newline='') as f:
                f.write(text.replace(old, new))
            print(f'applied {name} to {rel}')
    if missing:
        raise SystemExit('not applied: ' + ', '.join(missing))


if __name__ == '__main__':
    main()
