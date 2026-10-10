# HAPPY JARZ ESP32-S3 Mini build status

**Updated:** 2026-10-10  
**Board:** ESP32-S3 SuperMini  
**Current release in this repository:** none published yet

## Known-good Mini firmware reference

- **Firmware:** v0.17.13
- **Source location:** `Etsimulocto/BloomCircuit`, branch `feature/happyjarz-platform`
- **Source commit:** `7412f7ef0c5faf84310b7c7fe1612f29f6bb84ba`
- **Build method:** base sketch plus the Happy Jarz staging/patch scripts
- **Target verified:** ESP32-S3 SuperMini

The firmware was tested on the Mini. This new repo is the place for its clean, self-contained release copy; it has not been copied here yet, so there is no flashable "latest" sketch here today. The Pi controller app version is tracked separately and is not the ESP32 firmware version.

## Boot-screen report

The ESP32-S3 Mini is reported stuck on its boot screen. The Pi app split/refresh command updates host files and watchers; it does not flash the ESP32. The reported split did not upload firmware.

Before changing firmware, capture the Mini's serial output at 115200 baud from reset with the HAPPY JARZ and BloomPetz watchers stopped. Also note the exact screen text/pattern and which USB-C port is in use. Keep the current flash intact; do not run `erase_flash`.

## Clean release checklist

A build becomes the single current release here after it has:

1. one self-contained sketch with the version embedded;
2. documented ESP32-S3 Mini board and Arduino settings;
3. a clean compile from the documented steps;
4. a successful upload and reboot on the Mini;
5. a serial identity/version check plus OLED, touch, and lamp smoke tests;
6. matching release notes and a Git tag.

Until then, v0.17.13 remains a verified reference build in the controller repository and this repo intentionally publishes no flashable release.
