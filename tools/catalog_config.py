"""KairoMSX's catalog settings: MSX cartridge, disk and tape media, artwork references and the
platform's slugs (LaunchBox splits MSX, MSX2 and MSX2+; MobyGames lists them as one)."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "shared/tools"))
import content_ids
from launchbox_artwork import References

APP = "kairomsx"
PLATFORMS = ("msx", "msx2", "msx2plus")
ARTWORK_CODE = 80
ARTWORK = References("KairoMSX", (ARTWORK_CODE,))

CARTRIDGE = {".rom", ".mx1", ".mx2", ".ri"}
DISK = {".dsk", ".di1", ".di2", ".dmk"}
TAPE = {".cas", ".tsx"}
LIMITS = {"rom": 16 * 1024**2, "fd": 16 * 1024**2, "tape": 16 * 1024**2}

KINDS = set(LIMITS)


def kind(name):
    suffix = Path(name).suffix.lower()
    return ("rom" if suffix in CARTRIDGE else "fd" if suffix in DISK else
            "tape" if suffix in TAPE else None)


MEDIA = content_ids.Media(kind, content_ids.same_file, LIMITS.__getitem__, 1)

MOBYGAMES = "msx"
SCANS = {
    "msx": (".downloads/scan/msx.json", ("msx", "msx2", "msx2plus")),
    "msx2": (".downloads/scan/msx2.json", ("msx2", "msx2plus", "msx")),
    "turbor": (".downloads/scan/turbor.json", ("msx2plus", "msx2", "msx")),
}


def IGNORE(path):
    """The TOSEC turbo R set includes system firmware."""
    return "- Firmware (" in path
