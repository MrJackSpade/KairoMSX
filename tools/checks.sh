#!/bin/sh
# KairoMSX's own checks, after the shared ones (shared/tools/check-app.sh).
set -e
python3 tools/system_roms.py --check
