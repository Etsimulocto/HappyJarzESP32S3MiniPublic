# HAPPY JARZ Mini build status

**Board:** ESP32-S3 SuperMini (SIMPLE profile, 4 MB flash)  
**Firmware source:** 0.18.0 (art gallery added; compile/device check pending)  
**Pi Mini app:** 1.10.1  
**Source of truth:** `Etsimulocto/HappyJarzESP32S3MiniPublic`

## What is in this repository

- The Happy Jarz Mini firmware sketch plus the arcade and art-gallery source files it includes.
- The matching Pi app, its controller layers, art editor, single-instance USB watcher, and seven mini-games.
- One version file for firmware and one for the app.

The app and firmware source are together here. You do not need to pull app code or firmware patches from another checkout.

## Verification

- The two-frame art editor, gallery transfer, 50-slot storage, and OLED playback are implemented.
- The controller retries serial discovery while open and now requests exclusive access on POSIX, reducing conflicts with other USB watchers.
- The plug watcher uses a process lock so duplicate HAPPY JARZ autostart entries do not create duplicate watchers.
- The Pi serial reader shutdown race is handled without closing a newer connection from a stale reader.
- Python syntax checks and all eight pixel-art/transfer-protocol tests passed for the art implementation. The latest watcher/controller changes still need a syntax check on the release candidate.
- Arduino CLI is not installed in this workspace, so a fresh firmware compile has not been run.
- No firmware upload or hardware gallery/autolaunch test has been run from this workspace.

This update does not change the partition table or erase-flash setting. The gallery uses the existing SPIFFS partition; the app asks before first-time formatting that filesystem. Verify the firmware identity after upload before testing gallery transfer.

## Release gate

Mark a firmware release verified only after a clean compile with the documented FQBN, a successful upload to the ESP32-S3 SuperMini, and checks for serial identity, OLED, touch controls, lamps, a 50-slot transfer, two-frame playback, and plug-in autolaunch.
