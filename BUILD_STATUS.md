# HAPPY JARZ Mini build status

**Board:** ESP32-S3 SuperMini (SIMPLE profile, 4 MB flash)  
**Firmware source:** 0.18.2 (on-board profile editor and birthday countdown added; compile/device check pending)  
**Pi Mini app:** 1.11.0  
**Source of truth:** `Etsimulocto/HappyJarzESP32S3MiniPublic`

## What is in this repository

- The Happy Jarz Mini firmware sketch plus the arcade and art-gallery source files it includes.
- The matching Pi app, its controller layers, owner profile panel, art editor, single-instance USB watcher, and seven mini-games.
- One version file for firmware and one for the app.

The app and firmware source are together here. You do not need to pull app code or firmware patches from another checkout.

## Verification

- The two-frame art editor is in its own top-level ART tab; gallery transfer, 50-slot storage, and OLED playback are implemented.
- The PROFILE tab stores an owner's name and month/day birthday on the Pi keyed by chip ID and in Jar NVS. Firmware 0.18.2 adds an on-board editor using the six capacitive controls, manual date/time access, and a randomized startup countdown when a valid clock is available.
- The app synchronizes Jar time from the host when a board connects and when saving a profile.
- Birthday date validation and countdown tests have been added in `mini-app/test_profile.py`.
- The controller retries serial discovery while open and now requests exclusive access on POSIX, reducing conflicts with other USB watchers.
- The plug watcher uses a process lock so duplicate HAPPY JARZ autostart entries do not create duplicate watchers.
- The Pi serial reader shutdown race is handled without closing a newer connection from a stale reader.
- All eight pixel-art and transfer-protocol tests passed for the art implementation.
- Python syntax checks passed for the updated controller; the autostart installer passed shell syntax and temporary-home smoke checks.
- Firmware 0.18.1 compiled, uploaded, and reported `fw=0.18.1` on the Architect's 4 MB ESP32-S3 SuperMini.
- Firmware 0.18.2 has not yet been compiled or uploaded; its board editor/countdown behavior remains to be tested.

This update does not change the partition table or erase-flash setting. The gallery uses the existing SPIFFS partition; the app asks before first-time formatting that filesystem. Owner profile values use Preferences/NVS and are separate from the art filesystem. Verify the firmware identity after upload before testing profile sync and birthday behavior.

## Release gate

Mark a firmware release verified only after a clean compile with the documented FQBN, a successful upload to the ESP32-S3 SuperMini, and checks for unique serial/chip identity, on-board profile editing and persistence across power cycles, manual/Wi-Fi time setting, randomized birthday countdown and birthday greeting/lights, OLED, touch controls, lamps, a 50-slot transfer, two-frame playback, and plug-in autolaunch.
