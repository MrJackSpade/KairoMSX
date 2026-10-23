"""KairoMSX's catalog settings: MSX cartridge, disk and tape media, artwork references and the
platform's slugs (LaunchBox splits MSX, MSX2 and MSX2+; MobyGames lists them as one)."""
import json
import re
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


# The machines a game can ask for, as MsxCatalog.machine names them.
MACHINE_OPTIONS = {"machine": {
    "auto", "Panasonic_FS-A1WSX", "Panasonic_FS-A1GT", "Philips_NMS_8250",
    *(f"C-BIOS_{generation}{region}" for generation in ("MSX1", "MSX2", "MSX2+") for region in ("", "_JP", "_EU"))},
    "memory": {"64", "128", "256", "512", "1024", "2048", "4096"}}
_BASIC = set(json.loads((Path(__file__).resolve().parent.parent / "catalog/research/basic-cartridges-v1.json")
                        .read_text(encoding="utf-8"))["cartridges"])
_JAPANESE = re.compile(r"\((?:J|Jp|JP|Japan)\)")
_EUROPEAN = re.compile(r"\((?:E|Eu|EU|Europe|UK|DE|De|ES|Es|FR|Fr|NL|Nl|IT|It)\)")


def MACHINE(name, set_name, content_id):
    """The machine a game needs, which Automatic cannot tell from the media alone: turbo R games
    the FS-A1GT; BASIC cartridges (tools/basic_cartridges.py) a machine with MSX-BASIC, which C-BIOS
    lacks; other cartridges the C-BIOS machine of their generation (an MSX1 game can fail on an
    MSX2+, as Champion Boulder Dash does), in their region for its character set and refresh rate."""
    if set_name == "turbor":
        return {"machine": "Panasonic_FS-A1GT"}
    if content_id in _BASIC:
        return {"machine": "Panasonic_FS-A1WSX"}
    if not content_id.startswith("sha256-rom-v1:"):
        return {}
    region = "_JP" if _JAPANESE.search(name) else "_EU" if _EUROPEAN.search(name) else ""
    return {"machine": {"msx": "C-BIOS_MSX1", "msx2": "C-BIOS_MSX2"}[set_name] + region}


def NOT_GAME(path):
    """The TOSEC turbo R set's folders besides Games: applications, demos, firmware and so on."""
    return path.startswith("MSX TurboR [TOSEC]/") and "MSX TurboR - Games (" not in path

# Disk sets (shared/tools/disk_sets.py): a game starts from its cartridge, first disk or tape; other disks are swapped in the game.
BOOT_KINDS = ["rom", "fd", "tape"]
COMPANION = {}
