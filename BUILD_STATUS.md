# HAPPY JARZ Mini build status

**Board:** ESP32-S3 SuperMini (SIMPLE profile, 4 MB flash)  
**Firmware source:** 0.17.13  
**Pi Mini app:** 1.9.3  
**Source reference:** `Etsimulocto/BloomCircuit` commit `7412f7ef0c5faf84310b7c7fe1612f29f6bb84ba`

## What is in this repository

- A final staged, compile-ready firmware sketch plus the arcade source/header it includes.
- The matching Pi app, its controller layers, art editor, watcher, and seven mini-games.
- One version file for firmware and one for the app.

The app and firmware source are together here. You do not need to pull app code or firmware patches from another checkout.

## Verification

- Mini app 1.9.3 fixes the USB plug watcher's `HELLO` line terminator, which prevented reliable identity detection and auto-launch.

- The original v0.17.13 staging verifier passed while assembling this package.
- Python syntax checks passed for the Mini app and game modules.
- All four pixel-art unit tests passed.
- A fresh Arduino CLI compile was not run in this workspace.
- The currently connected Mini has not returned a valid `HJ|IDENTITY` line during the present boot-screen diagnosis. Its installed firmware is therefore unknown.

The connected board’s boot-screen issue remains open. Do not infer the installed version from Pi app files. Do not run `erase_flash` as a troubleshooting shortcut. Close serial watchers before any later port test or firmware upload.

## Release gate

Mark a firmware release verified only after a clean compile with the documented FQBN, a successful upload to the ESP32-S3 SuperMini, and checks for serial identity, OLED, touch controls, and lamps.
