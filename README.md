# HAPPY JARZ ESP32-S3 Mini + Pi Mini App

This is the single source repository for the HAPPY JARZ ESP32-S3 SuperMini firmware and its matching Raspberry Pi controller app.

## Current source pair

- Firmware: **0.18.0**
- Pi Mini app: **1.10.0**
- Board profile: **SIMPLE / ESP32-S3 SuperMini / 4 MB flash**

The repository contains the firmware, Mini app, USB watcher, controller, and art tools. It does not depend on BloomCircuit or another checkout.

## Run the Pi Mini app

On Raspberry Pi OS:

```bash
sudo apt-get install python3-tk python3-serial
python3 mini-app/app.py
```

The app includes LIGHTS + CONTROL, DISPLAY + INPUT, custom marquee sayings, and the 128×64 monochrome art editor. Create a still image or two-frame animation, save up to 50 pieces in the Pi library, then choose **Send gallery to Jar**. The board stores the gallery in SPIFFS and can rotate through it while disconnected from USB. Two-frame art animates on the OLED.

The first gallery transfer asks before initializing the art filesystem. No partition table or erase-all setting is changed.

To run the USB plug watcher:

```bash
python3 mini-app/happyjarz_plug_watch.py
```

The watcher opens the controller only after a HAPPY JARZ identity handshake.

## Compile the Mini firmware

```bash
arduino-cli core install esp32:esp32
arduino-cli lib install U8g2
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc firmware/HappyJarzMini
```

Build and verify before uploading. The art gallery adds a file-transfer protocol and filesystem storage, so verify serial identity, a one-frame transfer, a two-frame animation, touch inputs, OLED, and lamps before treating this firmware version as hardware-verified.

See [firmware/README.md](firmware/README.md) and [BUILD_STATUS.md](BUILD_STATUS.md) for the build steps and verification state.
