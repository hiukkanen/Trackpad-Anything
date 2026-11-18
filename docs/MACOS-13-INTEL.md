# macOS 13 Ventura on an Intel Mac

The app is not Sonoma-only in principle: the native Magic Trackpad 2 driver (`AppleHSBluetoothDriver`) ships with
Ventura too. Three things had to be changed to build and run there:

| Blocker | Fix in this repo |
|---|---|
| `MACOSX_DEPLOYMENT_TARGET=14.0` and `LSMinimumSystemVersion=14.0` — Ventura refuses to launch | Both default to **13.0** now (`TC_DEPLOYMENT_TARGET` overrides) |
| Release bundle linked an arm64 libusb | Build natively on Intel, or use `TC_ARCH=x86_64` / `universal` (`TC_ARCH`) |
| `bt-spoof` was written against Sonoma's cache layout | Same cache path on Ventura, but the layout is **not hardware-verified**; the tool fails closed if it differs |

## Build on the Intel Mac itself (simplest)

Use **Intel Homebrew** (`/usr/local`, the default on an Intel Mac). Its libusb is x86_64, so a native build just works:

```sh
xcode-select --install
brew install libusb pkg-config
node --version          # 22.13+ required for the GUI build
cd GUI && npm ci && cd ..

bash scripts/build-gui.sh          # -> build/Trackpad Anything.app (x86_64, min OS 13.0)
bash scripts/test.sh
bash scripts/package-release.sh    # -> build/release/Trackpad-Anything-*-macOS-Intel.zip
```

Open the app with right-click → **Open** the first time (it is ad-hoc signed, not notarized), then allow
Accessibility.

## Build on an Apple Silicon Mac for Intel

Two options:

1. **Rosetta Homebrew** — install an x86_64 libusb under `/usr/local` and build the Intel slice:
   ```sh
   arch -x86_64 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   arch -x86_64 /usr/local/bin/brew install libusb pkg-config
   TC_ARCH=x86_64 bash scripts/build-gui.sh
   ```
   `pkg-config` must resolve to the Intel one (`/usr/local/bin/pkg-config`), otherwise the architecture check in
   `build-gui.sh` stops the build with a clear message.

2. **Universal build** — needs a static libusb that contains both slices. If your libusb is a fat library
   (`lipo -archs` reports `x86_64 arm64`), run:
   ```sh
   TC_ARCH=universal bash scripts/package-release.sh
   ```
   This produces a `macOS-Universal.zip`. Do not mix a thin libusb into a universal link — the build checks for it.

## Bluetooth on Ventura: what to verify

`bt-spoof` edits `/Library/Bluetooth/Library/Preferences/com.apple.MobileBluetooth.devices.plist` (the same file
Sonoma uses). It only writes when it finds exactly one trackpad record with the expected field types and a matching
`SdpCache`. On Ventura the field layout may be slightly different; in that case the tool prints `STOP layout: ...`
**without changing anything** and the app shows the "layout is unexpected" message. That is the safe outcome.

To help adapt the tool, send the output of this *read-only* key/type dump with a compatibility report (it never prints
pairing keys or raw values):

```sh
sudo python3 - <<'PY'
import plistlib
p='/Library/Bluetooth/Library/Preferences/com.apple.MobileBluetooth.devices.plist'
root=plistlib.load(open(p,'rb'))
def walk(v,d=0):
    if isinstance(v,dict):
        for k,x in v.items():
            if any(s in k.lower() for s in ('key','secret','token','irk','ltk')): continue
            print('  '*d+str(k),type(x).__name__,len(x) if hasattr(x,'__len__') else '')
            if d<4: walk(x,d+1)
walk(root)
PY
```

If `apply` succeeds, the trackpad should reconnect on macOS's own driver (full gestures, battery in the menu bar).
If it reports `layout:`, USB gestures still work; only the native Bluetooth path needs a small format adjustment.
To undo: click **Restore** in the app, or plug the trackpad in with a cable once to re-pair it.

## Known limits

- Ventura + Intel is **built in CI** on an Intel `macos-15-intel` runner with deployment target 13.0 (GitHub retired the `macos-13` image), but it is not yet confirmed on real Ventura trackpad hardware.
- The USB gesture engine, shortcuts and settings mirror do not depend on the macOS version and need no changes.
- Force Click and haptics remain unavailable, same as on Sonoma.
