#!/bin/bash
# Build a downloadable release zip (ad-hoc signed, no Homebrew needed at runtime).
# TC_ARCH=native|x86_64|arm64|universal selects the architecture (default: native).
# On an Intel Mac the default is an Intel build; TC_DEPLOYMENT_TARGET defaults to 13.0 (Ventura).
set -euo pipefail
cd "$(dirname "$0")/.."
version=$(python3 -c "import re;print(re.search(r\"'CFBundleShortVersionString':'([^']+)'\",open('scripts/build-gui.sh').read()).group(1))")
out="$PWD/build/release"; rm -rf "$out"; mkdir -p "$out"
TC_SIGN=adhoc TC_APP_OUTPUT="$out/Trackpad Anything.app" bash scripts/build-gui.sh
# Refuse to ship a helper that still needs a non-system library.
if otool -L "$out/Trackpad Anything.app/Contents/Resources/usb-helper" | tail -n +2 | grep -v -E '^\s+/(System|usr/lib)/'; then echo "usb-helper links a non-system library" >&2; exit 1; fi
arch="${TC_ARCH:-native}"
case "$arch" in
  universal) label="Universal" ;;
  arm64) label="AppleSilicon" ;;
  x86_64) label="Intel" ;;
  native)
    case "$(uname -m)" in
      arm64) label="AppleSilicon" ;;
      x86_64) label="Intel" ;;
      *) label="$(uname -m)" ;;
    esac ;;
esac
printf 'Built slices: %s\n' "$(lipo -archs "$out/Trackpad Anything.app/Contents/MacOS/TrackpadCompatGUI")"
printf 'Helper slices: %s\n' "$(lipo -archs "$out/Trackpad Anything.app/Contents/Resources/usb-helper")"
zip="$out/Trackpad-Anything-$version-macOS-$label.zip"
ditto -c -k --keepParent "$out/Trackpad Anything.app" "$zip"
shasum -a 256 "$zip" | tee "$zip.sha256"
