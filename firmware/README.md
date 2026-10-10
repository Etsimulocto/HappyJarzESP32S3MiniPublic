# HAPPY JARZ Mini firmware

- Firmware version: `0.17.13`
- Target: ESP32-S3 SuperMini, SIMPLE profile, 4 MB flash
- Sketch: `HappyJarzMini/HappyJarzMini.ino`
- Additional Arduino source: `happyjarz_arcade.cpp` and `happyjarz_arcade.h`
- Board FQBN used by the source flasher: `esp32:esp32:esp32s3:CDCOnBoot=cdc`
- Required library: U8g2 (the source notes identify 2.36.19)
- Serial speed: 115200 baud

The sketch is the final staged output of the verified v0.17.13 source pipeline from commit `7412f7ef0c5faf84310b7c7fe1612f29f6bb84ba`. The pipeline verifier reported PASS for the 100-pattern bank, OLED/menu, six touch inputs, four APA106 lamps, gamepad/host protocol, OLED mirror, art/saying support, and power/sleep features.

Build from the repository root:

```bash
arduino-cli core install esp32:esp32
arduino-cli lib install U8g2
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc firmware/HappyJarzMini
```

The FQBN is the exact setting in the known source flasher. This package pass did not run Arduino CLI, upload the sketch, or validate the currently connected board. Treat successful compilation and the post-upload OLED, touch, lamp, and `HELLO`/identity checks as still required before marking a new release as verified.

The board responds to `HELLO` with an `HJ|IDENTITY|...` line when the application is running. Firmware 0.17.13 should identify itself as `fw=0.17.13`.
