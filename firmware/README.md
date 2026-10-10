# HAPPY JARZ Mini firmware

- Firmware version: `0.18.2`
- Target: ESP32-S3 SuperMini, SIMPLE profile, 4 MB flash
- Sketch: `HappyJarzMini/HappyJarzMini.ino`
- Additional Arduino source: `happyjarz_arcade.cpp`/`.h` and `happyjarz_art.cpp`/`.h`
- Board FQBN used by the source flasher: `esp32:esp32:esp32s3:CDCOnBoot=cdc`
- Required library: U8g2 (the source notes identify 2.36.19)
- Serial speed: 115200 baud

Firmware 0.18.0 adds a 50-slot, one- or two-frame monochrome art gallery in the existing SPIFFS data partition. USB commands receive checked 48-byte chunks, write each image to a temporary file, validate its exact size, and then replace the prior slot. The ART screensaver rotates images every five seconds and animates two-frame art every 400 ms. First use is initialized by the app after the user confirms formatting the art filesystem.

Firmware 0.18.1 reports a stable chip-derived board ID instead of the shared `HJ-001` placeholder. The PROFILE protocol stores the owner's name and annual birthday (month and day only) in ESP32 Preferences/NVS. When the clock is set to that date, the lights run a colorful pattern and the OLED shows a birthday greeting during the screensaver. The Pi app syncs the clock on USB connection and when saving a profile.

Firmware 0.18.2 adds an on-board PROFILE menu. The six capacitive controls edit and save the owner's name and annual birthday directly to Preferences/NVS, open the manual date/time editor, and clear a profile with confirmation. If a birthday and valid clock are set, the OLED has a one-in-three chance to show the countdown at startup; a birthday-day boot shows the greeting. The clock can be set without a computer, but it needs Wi-Fi time sync or a manual reset after full power loss because this board has no battery-backed clock.

Build from the repository root:

```bash
arduino-cli core install esp32:esp32
arduino-cli lib install U8g2
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc firmware/HappyJarzMini
```

Firmware 0.18.2 has not yet been compiled with Arduino CLI or uploaded to a board. Treat successful compilation and post-upload profile editing/persistence, date/time entry, birthday countdown, gallery transfer, OLED animation, touch, lamp, and `HELLO`/identity checks as required before marking the release verified. Treat successful compilation and post-upload profile persistence, birthday behavior, gallery transfer, OLED animation, touch, lamp, and `HELLO`/identity checks as required before marking the release verified.

The board responds to `HELLO` with an `HJ|IDENTITY|...` line when the application is running. Firmware 0.18.2 reports a unique `chip=` value and a serial derived from that ID.
