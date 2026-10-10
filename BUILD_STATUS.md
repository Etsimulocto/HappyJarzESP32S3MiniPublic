# HAPPY JARZ Mini build status

**Board:** ESP32-S3 SuperMini (SIMPLE profile, 4 MB flash)  
**Firmware source:** 0.18.0 (art gallery added; compile/device check pending)  
**Pi Mini app:** 1.10.0  
**Source of truth:** `Etsimulocto/HappyJarzESP32S3MiniPublic`

## What is in this repository

- The Happy Jarz Mini firmware sketch plus the arcade and art-gallery source files it includes.
- The matching Pi app, its controller layers, art editor, watcher, and seven mini-games.
- One version file for firmware and one for the app.

The app and firmware source are together here. You do not need to pull app code or firmware patches from another checkout.

## Verification

- Python syntax checks passed for the Mini app and game modules.
- All eight pixel-art and transfer-protocol unit tests passed.
- Arduino CLI is not installed in this workspace, so a fresh firmware compile has not been run.
- No firmware upload or hardware gallery test has been run from this workspace.

This update does not change the partition table or erase-flash setting. The gallery uses the existing SPIFFS partition; the app asks before first-time formatting that filesystem. Verify the firmware identity after upload before testing gallery transfer.

## Release gate

Mark a firmware release verified only after a clean compile with the documented FQBN, a successful upload to the ESP32-S3 SuperMini, and checks for serial identity, OLED, touch controls, lamps, a 50-slot transfer, and two-frame playback.
