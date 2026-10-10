# HAPPY JARZ Mini firmware

- Firmware version: `0.18.0`
- Target: ESP32-S3 SuperMini, SIMPLE profile, 4 MB flash
- Sketch: `HappyJarzMini/HappyJarzMini.ino`
- Additional Arduino source: `happyjarz_arcade.cpp`/`.h` and `happyjarz_art.cpp`/`.h`
- Board FQBN used by the source flasher: `esp32:esp32:esp32s3:CDCOnBoot=cdc`
- Required library: U8g2 (the source notes identify 2.36.19)
- Serial speed: 115200 baud

Firmware 0.18.0 adds a 50-slot, one- or two-frame monochrome art gallery in the existing SPIFFS data partition. USB commands receive checked 48-byte chunks, write each image to a temporary file, validate its exact size, and then replace the prior slot. The ART screensaver rotates images every five seconds and animates two-frame art every 400 ms. First use is initialized by the app after the user confirms formatting the art filesystem.

Build from the repository root:

```bash
arduino-cli core install esp32:esp32
arduino-cli lib install U8g2
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc firmware/HappyJarzMini
```

The FQBN is the exact setting in the known source flasher. This source update has not yet been compiled with Arduino CLI, uploaded, or validated on the currently connected board. Treat successful compilation and post-upload gallery transfer, OLED animation, touch, lamp, and `HELLO`/identity checks as required before marking the release verified.

The board responds to `HELLO` with an `HJ|IDENTITY|...` line when the application is running. This source version should identify itself as `fw=0.18.0` after a successful build and upload.
