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

# SDL's Android build adds the NDK's cpufeatures, which is Apache-2.0 and cannot be combined with
# GPL-2.0-only openMSX. SDL only calls it for 32-bit ARM's NEON check (src/cpuinfo/SDL_cpuinfo.c),
# so add it only there.
SDL_CPUFEATURES = [
    ('sdl2/CMakeLists.txt',
     '''  list(APPEND SOURCE_FILES ${ANDROID_CORE_SOURCES} ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
  set_property(SOURCE "${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c" APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-declaration-after-statement")
''',
     '''  list(APPEND SOURCE_FILES ${ANDROID_CORE_SOURCES})
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

PATCHES = {
    'sdl-cpufeatures': SDL_CPUFEATURES,
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
