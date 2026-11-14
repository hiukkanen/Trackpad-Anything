#!/bin/bash
# Build a downloadable release zip (ad-hoc signed, Apple Silicon, no Homebrew needed at runtime).
set -euo pipefail
cd "$(dirname "$0")/.."
version=$(python3 -c "import re;print(re.search(r\"'CFBundleShortVersionString':'([^']+)'\",open('scripts/build-gui.sh').read()).group(1))")
out="$PWD/build/release"; rm -rf "$out"; mkdir -p "$out"
TC_SIGN=adhoc TC_APP_OUTPUT="$out/Trackpad Anything.app" bash scripts/build-gui.sh
# Refuse to ship a helper that still needs a non-system library.
if otool -L "$out/Trackpad Anything.app/Contents/Resources/usb-helper" | tail -n +2 | grep -v -E '^\s+/(System|usr/lib)/'; then echo "usb-helper links a non-system library" >&2; exit 1; fi
zip="$out/Trackpad-Anything-$version-macOS-AppleSilicon.zip"
ditto -c -k --keepParent "$out/Trackpad Anything.app" "$zip"
shasum -a 256 "$zip" | tee "$zip.sha256"
