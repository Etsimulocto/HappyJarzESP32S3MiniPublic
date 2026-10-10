# HAPPY JARZ Mini build status

**Board:** ESP32-S3 SuperMini (SIMPLE profile, 4 MB flash)  
**Firmware source:** 0.18.4 (all-day Birthday Mode; ESP32 compile/device check pending)  
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
- Firmware 0.18.2 compiled successfully on the Architect's Pi: 1,117,977 bytes of sketch space (85%) and 53,528 bytes of static RAM (16%). The Architect reported reconnecting after upload; detailed profile/countdown checks remain pending.
- Firmware 0.18.3 shows a five-second birthday countdown on every successful accessory OFF/ON restore, wakes software sleep and lets the overlay temporarily take priority over the arcade renderer. Host-side checks passed for repeated restores, fresh five-second timing, sleep wake, invalid birthday, delayed clock sync and birthday-day countdown. The Pi compiled this release, and the Architect reported reconnecting after retrying an interrupted upload; detailed physical switch checks remain pending.

- Firmware 0.18.4 adds animated cake/balloon/confetti/present/wish screens mixed with the selected saver, an A-to-open birthday wish, a slow rainbow shimmer that respects OFF and user color/pattern choices, and restoration at local midnight. Extracted C++ host tests passed for birthday activation, midnight restoration, lights override, gift timer, scene mixing, drawing bounds, accessory/sleep gating and Feb 29 behavior. The complete ESP32 build and board Birthday Mode checks remain pending.

This update does not change the partition table or erase-flash setting. The gallery uses the existing SPIFFS partition; the app asks before first-time formatting that filesystem. Owner profile values use Preferences/NVS and are separate from the art filesystem. Verify the firmware identity after upload before testing profile sync and birthday behavior.

## Release gate

Mark a firmware release verified only after a clean compile with the documented FQBN, a successful upload to the ESP32-S3 SuperMini, and checks for unique serial/chip identity, on-board profile editing and persistence across power cycles, manual/Wi-Fi time setting, randomized startup countdown, repeated accessory-switch countdown with return to the previous screen, birthday greeting/lights, A-to-open present, scene mixing with art, user light override and midnight restoration, OLED, touch controls, lamps, a 50-slot transfer, two-frame playback, and plug-in autolaunch.
