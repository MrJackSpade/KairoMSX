"""KairoMSX's third-party notices: the sources shared/tools/notices.py writes kairomsx/src/main/assets/THIRD_PARTY_NOTICES.txt from.
Run it again after an import."""
from notices import excerpt, text

OUT = 'kairomsx/src/main/assets/THIRD_PARTY_NOTICES.txt'

HEADER = '''KairoMSX third-party notices
These notices apply to the emulator code and libraries in this package. KairoMSX emulates the MSX
with openMSX 21.0 and starts games with C-BIOS 0.29; KairoMSX is not affiliated with the openMSX
team or the C-BIOS authors.'''

OPENMSX_TERMS = '''openMSX is distributed under the GNU General Public License, version 2; its README names no
later version. Its contributors are listed in doc/authors.txt of its source. Some of its source
files carry other terms, given below them. The GPL version 2 text follows.'''

# (title, source, [text]) in the order shown.
SECTIONS = [
    ('openMSX 21.0', 'https://openmsx.org/, third_party/openmsx',
     [OPENMSX_TERMS, text('openmsx/doc/GPL.txt')]),
    ('openMSX: YM2413 emulation by Nuke.YKT (GPL-2.0-or-later)', 'third_party/openmsx/src/sound/opll.hh',
     [excerpt('openmsx/src/sound/opll.hh', 'Copyright (C) 2019 Nuke.YKT', 'GNU General Public License for more details.')]),
    ('openMSX: PNG code from SDL_image (LGPL-2.0-or-later)', 'third_party/openmsx/src/video/PNG.cc',
     [excerpt('openmsx/src/video/PNG.cc', 'The copyright notice below applies', 'Software Foundation, Inc., 675 Mass Ave')]),
    ('openMSX: LZ4 (BSD-2-Clause)', 'third_party/openmsx/src/utils/lz4.hh',
     [excerpt('openmsx/src/utils/lz4.hh', 'LZ4 - Fast LZ compression algorithm', 'OF THIS SOFTWARE, EVEN IF ADVISED')]),
    ('openMSX: xxHash (BSD-2-Clause)', 'third_party/openmsx/src/utils/xxhash.hh',
     [excerpt('openmsx/src/utils/xxhash.hh', 'xxHash - Fast Hash algorithm', 'OF THIS SOFTWARE, EVEN IF ADVISED')]),
    ('openMSX: UTF8-CPP (Boost Software License 1.0)', 'third_party/openmsx/src/utils/utf8_core.hh',
     [excerpt('openmsx/src/utils/utf8_core.hh', 'Copyright 2006 Nemanja Trifunovic', 'DEALINGS IN THE SOFTWARE.')]),
    ('openMSX: fast_log (MIT)', 'third_party/openmsx/src/utils/fast_log2.hh',
     [excerpt('openmsx/src/utils/fast_log2.hh', 'MIT License', 'Copyright (c) 2022 Nadav Rotem'),
      'The MIT license text is included with Dear ImGui below.']),
    ('openMSX: Tiger tree hash (public domain)', 'third_party/openmsx/src/utils/TigerTree.hh',
     [excerpt('openmsx/src/utils/TigerTree.hh', 'Copyright (C) 2001 Bitzi', 'for more info.')]),
    ('openMSX: function_ref (CC0)', 'third_party/openmsx/src/utils/function_ref.hh',
     [excerpt('openmsx/src/utils/function_ref.hh', 'function_ref - A low-overhead', 'You should have received a copy of the CC0')]),
    ('C-BIOS 0.29', 'https://cbios.sourceforge.net/, third_party/openmsx/Contrib/cbios',
     [excerpt('openmsx/Contrib/README.cbios', 'Copyright (c) 2002-2005 BouKiCHi', 'SUCH DAMAGE.')]),
    ('Dear ImGui', 'https://github.com/ocornut/imgui, third_party/openmsx/src/3rdparty/imgui',
     [text('openmsx/src/3rdparty/imgui/LICENSE.txt')]),
    ('zlib 1.3.2', 'https://zlib.net/, third_party/zlib', [text('zlib/LICENSE')]),
    ('libpng 1.6.59', 'http://www.libpng.org/pub/png/libpng.html, third_party/libpng', [text('libpng/LICENSE')]),
    ('Tcl 8.6.18', 'https://www.tcl-lang.org/, third_party/tcl', [text('tcl/license.terms')]),
    ('SDL2 2.32.10', 'https://www.libsdl.org/, third_party/sdl2', [text('sdl2/LICENSE.txt')]),
]
