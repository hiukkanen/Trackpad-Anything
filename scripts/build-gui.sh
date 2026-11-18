#!/bin/bash
set -euo pipefail
# Ventura (13.0) is the minimum: the native Magic Trackpad 2 driver exists there too.
export MACOSX_DEPLOYMENT_TARGET="${TC_DEPLOYMENT_TARGET:-13.0}"
cd "$(dirname "$0")/.."
(cd GUI && npm run build && npx astryx doctor --built dist)
app="${TC_APP_OUTPUT:-$PWD/build/Trackpad Anything.app}"
mkdir -p "$app/Contents/MacOS" "$app/Contents/Resources"

# TC_ARCH: native (default, build on the Mac you will run on), x86_64, arm64 or universal.
# A universal build needs a static libusb containing both slices (lipo -archs).
arch_flag=""
case "${TC_ARCH:-native}" in
  native) ;;
  x86_64) arch_flag="-arch x86_64" ;;
  arm64) arch_flag="-arch arm64" ;;
  universal) arch_flag="-arch x86_64 -arch arm64" ;;
  *) echo "TC_ARCH must be native, x86_64, arm64 or universal" >&2; exit 2 ;;
esac
libusb_a="$(pkg-config --variable=libdir libusb-1.0)/libusb-1.0.a"
if [ ! -f "$libusb_a" ]; then echo "static libusb not found at $libusb_a (brew install libusb pkg-config)" >&2; exit 1; fi
if [ -n "$arch_flag" ]; then
  libusb_archs="$(lipo -archs "$libusb_a")"
  case "${TC_ARCH:-native}" in
    universal)
      for a in x86_64 arm64; do
        case " $libusb_archs " in *" $a "*) ;; *) echo "libusb ($libusb_a) is missing the $a slice: $libusb_archs" >&2; exit 1 ;; esac
      done ;;
    *)
      case " $libusb_archs " in *" ${TC_ARCH} "*) ;; *) echo "libusb ($libusb_a) is $libusb_archs, not ${TC_ARCH}" >&2; exit 1 ;; esac ;;
  esac
fi
# shellcheck disable=SC2086 # word splitting of $arch_flag is intentional
clang $arch_flag -O2 -Wall -Wextra -c Sources/Core/GestureEngine.c -o build/engine.o
# shellcheck disable=SC2086
clang $arch_flag -O2 -Wall -Wextra -fobjc-arc Sources/App/GUIHost.m build/engine.o -framework Cocoa -framework WebKit -framework ApplicationServices -framework IOKit -framework ServiceManagement -o "$app/Contents/MacOS/TrackpadCompatGUI"
# space-command: CLI for Karabiner or scripts ("space-command left|right").
# shellcheck disable=SC2086
clang $arch_flag -O2 -Wall -Wextra Sources/Command/main.c -o "$app/Contents/Resources/space-command"
# shellcheck disable=SC2086
clang $arch_flag -O2 -Wall -Wextra -fobjc-arc Sources/BluetoothSetup/main.m -framework Foundation -o "$app/Contents/Resources/bt-spoof"
# libusb is linked statically so the app runs without Homebrew.
# shellcheck disable=SC2086
clang $arch_flag -O2 -Wall -Wextra Sources/USBHelper/main.c $(pkg-config --cflags libusb-1.0) "$libusb_a" -lobjc -framework IOKit -framework CoreFoundation -framework Security -o "$app/Contents/Resources/usb-helper"
rsync -a --delete GUI/dist/ "$app/Contents/Resources/GUI/"
python3 - "$app" <<'PY'
import os,sys,plistlib
from pathlib import Path
p=Path(sys.argv[1])/'Contents/Info.plist'
minver=os.environ.get('MACOSX_DEPLOYMENT_TARGET','13.0')
p.write_bytes(plistlib.dumps({'CFBundleIdentifier':'org.trackpadcompat.gui.development','CFBundleName':'Trackpad Anything','CFBundleDisplayName':'Trackpad Anything','CFBundleExecutable':'TrackpadCompatGUI','CFBundlePackageType':'APPL','CFBundleVersion':'1.1.0','CFBundleShortVersionString':'1.1.0','NSHumanReadableCopyright':'© 2026 KUGNUS LAB','LSMinimumSystemVersion':minver,'NSHighResolutionCapable':True,'CFBundleIconFile':'AppIcon','CFBundleDevelopmentRegion':'en','CFBundleLocalizations':['en','ko','ja','zh-Hans']}))
PY
if [ -f Resources/AppIcon.icns ]; then cp Resources/AppIcon.icns "$app/Contents/Resources/"; fi
cp Resources/MenuBarMascot.png Resources/MenuBarMascot@2x.png "$app/Contents/Resources/"
cp GUI/src/locales.json "$app/Contents/Resources/locales.json"
# libusb is LGPL-2.1 and linked statically: ship its license next to the binary (see docs/THIRD-PARTY.md).
cp "$(brew --prefix libusb)/COPYING" "$app/Contents/Resources/ThirdParty-libusb-COPYING.txt"
cp docs/THIRD-PARTY.md "$app/Contents/Resources/ThirdParty-NOTICES.md"
# TC_SIGN=adhoc: no personal certificate (release builds, or building from source without setup).
if [ "${TC_SIGN:-local}" = adhoc ]; then
  for target in "$app/Contents/Resources/usb-helper" "$app/Contents/Resources/bt-spoof" "$app/Contents/Resources/space-command" "$app"; do codesign --force --sign - --timestamp=none "$target"; done
else
  python3 scripts/sign-local.py "$app"
fi
codesign --verify --strict "$app"
printf '%s\n' "$app"
