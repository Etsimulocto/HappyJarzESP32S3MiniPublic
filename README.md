# HAPPY JARZ ESP32-S3 Mini Firmware

This is the public, canonical repository for **released HAPPY JARZ ESP32-S3 firmware builds**.

It contains device firmware only. The Raspberry Pi controller, BloomPetz, BloomCircuit editor, and other applications belong in their own repositories.

## Current build status

See [BUILD_STATUS.md](BUILD_STATUS.md) before compiling or flashing. A version number alone does not mean a build is verified for every ESP32-S3 board revision.

At the moment, the known-good firmware release **v0.17.13** was verified on the ESP32-S3 SuperMini prototype. The full-size ESP32-S3 N16R8 board migration is not yet verified. Do not flash the SuperMini build onto the N16R8 board based on the shared chip name.

## Release rule

The default branch should contain one current, self-contained firmware build at a time. A release must include:

- one compile-ready sketch and its exact version
- the exact ESP32-S3 board and Arduino build settings
- required libraries and a repeatable build command
- compile/upload result and board test notes
- a recovery procedure if startup fails

Older releases should remain available as Git tags/releases. Do not put Pi applications, patch-injection pipelines, alternate board builds, or unverified experiments in the current release folder.

## Before flashing

1. Identify the physical board (SuperMini or full-size N16R8).
2. Read [BUILD_STATUS.md](BUILD_STATUS.md) and confirm the firmware target matches.
3. Stop serial watchers and confirm the selected port belongs to this device.
4. Compile and verify first; flash only the matching, tested build.
5. Record the firmware identity reported by the device after reboot.

If the board is stuck at a boot screen, pause flashing and diagnose the USB/serial output and exact board first.
