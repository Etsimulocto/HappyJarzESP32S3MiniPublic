# HAPPY JARZ firmware build status

**Updated:** 2026-10-10  
**Repository role:** public firmware source of truth  
**Current main-branch firmware release:** none published here yet

## Known-good reference

- **Firmware:** v0.17.13
- **Source currently recorded in:** `Etsimulocto/BloomCircuit`, branch `feature/happyjarz-platform`, commit `7412f7ef0c5faf84310b7c7fe1612f29f6bb84ba`
- **Build path there:** a base sketch plus a multi-script staging/patch pipeline
- **Verified target:** ESP32-S3 SuperMini prototype
- **Not verified for:** full-size ESP32-S3 N16R8 board

This is a historical reference, not a flash instruction. The patch-based build is not yet copied here as one self-contained, reproducible release artifact. Do not treat the old sketch filename or app version as proof of the firmware target.

## Full-size N16R8 migration

The hardware reference recorded October 9 says the N16R8 migration is incomplete: OLED operation and pin behavior have not been verified on that board, the battery path is unresolved, and the 16 MB flash / 8 MB PSRAM Arduino settings need deliberate selection. Therefore there is no approved N16R8 firmware release in this repository yet.

## Boot-screen incident

The board has been reported stuck on its boot screen. The Pi app split/refresh process updates host files and watchers; it does not flash the ESP32. No firmware flash should be attempted until the physical board model, serial port, and startup output are identified.

For diagnosis, capture:

- the board model printed on the PCB (SuperMini or full-size N16R8)
- which USB-C connector is connected (UART or USB)
- the exact screen text/pattern and whether it changes after reset
- serial output at 115200 baud from reset, with Pi app watchers stopped
- whether the board still responds to the ROM bootloader / `esptool chip-id`

Do not run `erase_flash`. Preserve the current flash until its state is known.

## Promotion checklist

A firmware build becomes the single current build on `main` only after it has:

1. one self-contained sketch (no hidden version-injection or patch chain);
2. explicit target board and Arduino settings;
3. a clean compile from documented steps;
4. a successful upload and reboot on the named physical board;
5. a serial identity/version check and basic OLED, touch, and lamp tests;
6. a tagged release and matching build notes.

Until that checklist is complete, this repository intentionally has no flashable "latest" build.
