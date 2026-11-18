# Testing and recovery

## Automated tests

```bash
bash scripts/test.sh
```

| Suite | What it covers |
|---|---|
| `tests/EngineTests.c` | Gesture engine: pointer gains, Spaces/Mission Control swipes, zoom, rotation, pages, cancellation, option validation. Events are captured, never posted. |
| `tests/SettingsTests.m` | Settings round trip, mutually exclusive gestures, malformed values, write failure. |
| `tests/bluetooth_setup_tests.py` | `bt-spoof` on fixture caches: apply, restore, repeated apply/restore, and refusal (no write, no backup) on unexpected layouts: no trackpad, two trackpads, wrong field sizes, unknown product ID, missing or ambiguous `SdpCache`, a non-plist file, a missing file. |

CI (`.github/workflows/ci.yml`) runs the tests and builds the release zip on an **Intel `macos-15-intel` runner** for
every push and pull request. GitHub retired the `macos-13` runner image in December 2025, so CI no longer runs on
Ventura itself; instead it cross-targets Ventura with deployment target 13.0 and checks that the binaries are x86_64.
Hardware behavior (Bluetooth, USB, gestures) can only be checked on a real trackpad; see
[Ventura + Intel](MACOS-13-INTEL.md) for the build options and the one cache-layout detail still needing on-machine
confirmation.

## Bluetooth change: what it does and what to expect

`bt-spoof` (run by the app with administrator rights) edits one record in
`/Library/Bluetooth/Library/Preferences/com.apple.MobileBluetooth.devices.plist` (the same file on Ventura, Sonoma and
Sequoia — the `SdpCache` field layout was reverse-engineered on Sonoma and is not hardware-verified on Ventura):

- It only proceeds when it finds **exactly one** trackpad record whose fields have the expected layout. Otherwise it
  stops before writing anything. With two Magic Trackpads paired it refuses; unpair one first.
- It validates and converts the record in memory, then backs up the original cache once (`….tc-before-pid-spoof`,
  mode 0600), checks that the file did not change meanwhile, and replaces it atomically with a temporary file in
  the same root-owned folder.
- It restarts `bluetoothd` (and `cfprefsd`) so macOS reloads the cache. **All Bluetooth devices disconnect for a
  few seconds** and reconnect by themselves: keyboards, mice, headphones.
- It reads the cache back and reports failure if macOS rewrote it.

## Recovery

| Situation | What to do |
|---|---|
| Gestures stopped after plugging the trackpad into the Mac (cable re-pairs it) | Unplug, let it reconnect over Bluetooth, click **Enable Bluetooth gestures** again. |
| Gestures stopped after a macOS update | Click **Enable Bluetooth gestures** again. If the tool says the layout is unexpected, it changed nothing; please open an issue. |
| Undo the change | Plug the trackpad in with a cable once (re-pairing writes the original ID), or remove it in System Settings > Bluetooth and pair it again. |
| Last resort | The untouched original is at `/Library/Bluetooth/Library/Preferences/com.apple.MobileBluetooth.devices.plist.tc-before-pid-spoof`. Copying it back (as root) and restarting the Mac restores the cache as it was before the first change; devices paired since then must be paired again. |

## Hardware test matrix

| Check | M1 Pro, macOS 14.7.2 |
|---|:-:|
| Bluetooth: enable, gestures work | ✅ |
| Bluetooth: survives app quit and reboot | ✅ |
| Bluetooth: cable re-pair undoes the change | ✅ |
| Bluetooth: enable again after a cable re-pair | not recorded |
| Bluetooth: minor macOS update | not tested |
| USB: pointer, scroll, Spaces, Mission Control | ✅ |
| Keyboard desktop shortcuts | ✅ |
| Other Apple Silicon models | not tested |
| Other macOS 14.x versions | not tested |
| Intel Mac, macOS 13 Ventura | build verified in CI; not hardware-tested — [notes](MACOS-13-INTEL.md) |
| Two trackpads paired | refuses by design (fixture-tested, not on hardware) |

Reports from other Macs are welcome: use the *Bug or compatibility report* issue template.

## Permissions and signing

- **Administrator password**: asked for when enabling or undoing Bluetooth gestures (`bt-spoof`) and when
  starting the USB helper (`usb-helper`, which needs root to claim the trackpad's USB interface). Nothing else runs
  with elevated rights, and nothing is installed as a background service.
- **Accessibility**: needed to post pointer, scroll and gesture events and to catch the desktop shortcuts.
- **Unsigned release**: release builds are ad-hoc signed, not notarized by Apple, so macOS shows a warning on first
  launch. Open it with right-click > **Open**, or build from source.
