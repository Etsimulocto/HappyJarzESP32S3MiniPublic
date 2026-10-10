# HAPPY JARZ ESP32-S3 Mini Firmware

This is the public home for **HAPPY JARZ ESP32-S3 Mini firmware builds**. It is firmware only; the Raspberry Pi controller and other apps stay in their own repositories.

## Current status

The project hardware is the **ESP32-S3 SuperMini**. The known-good firmware reference is **v0.17.13**, tested on that Mini board. See [BUILD_STATUS.md](BUILD_STATUS.md) for the source location and the current boot-screen diagnostic status.

This repository is being kept to one clean, reproducible firmware build at a time. It does not yet contain the v0.17.13 sketch: that build currently lives in BloomCircuit as a base sketch plus a staging pipeline. We will move a self-contained, verified Mini build here before calling it the repository's current release.

## Release rule

Each release must include one compile-ready sketch, its exact firmware version, the ESP32-S3 Mini board settings, required libraries, reproducible build steps, and test notes. The default branch is for the current clean build; older releases belong in Git tags/releases. Pi applications and unverified firmware experiments do not belong here.

## Flashing

Do not flash based on a version number alone. Confirm the selected build is for the ESP32-S3 Mini, stop serial watchers, confirm the USB port, compile and verify, then upload. If the device is stuck on its boot screen, capture startup serial output before changing its firmware.
