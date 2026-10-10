# HAPPY JARZ ESP32-S3 Mini + Pi Mini App

This repository is the single source home for the HAPPY JARZ **ESP32-S3 SuperMini** firmware and its matching Raspberry Pi controller app.

## Current source pair

- Firmware: **0.17.13**
- Pi Mini app: **1.9.3**
- Board profile: **SIMPLE / ESP32-S3 SuperMini / 4 MB flash**
- Source import: BloomCircuit commit `7412f7ef0c5faf84310b7c7fe1612f29f6bb84ba`

The files needed to build and run this pair are in this repository. The firmware folder contains the final staged sketch and its arcade translation units; it does not require a patch-chain from another checkout.

## Run the Pi Mini app

On Raspberry Pi OS:

```bash
sudo apt-get install python3-tk python3-serial
python3 mini-app/app.py
```

The app includes LIGHTS + CONTROL, DISPLAY + INPUT, custom marquee sayings, the 128×64 pixel-art editor, and the service panel. The art editor saves its library on the Pi and exports U8g2 C headers. **Sending saved art to the Jar is not implemented in firmware 0.17.13.**

To run the USB plug watcher instead:

```bash
python3 mini-app/happyjarz_plug_watch.py
```

The watcher launches the app only after a HAPPY JARZ `HELLO` identity handshake. Mini app 1.9.3 fixes the watcher command terminator so the board receives the complete line.

## Compile the Mini firmware

Install the ESP32 Arduino core and U8g2, then compile the included sketch:

```bash
arduino-cli core install esp32:esp32
arduino-cli lib install U8g2
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc firmware/HappyJarzMini
```

The board settings and serial protocol are documented in [firmware/README.md](firmware/README.md). Compile and verify the target before uploading. Do not erase flash as a routine update; that can remove saved Jar settings.

## Verification status

The v0.17.13 staging verifier passed while this package was assembled. A fresh Arduino compile and smoke test on the currently connected Mini have **not** been completed in this packaging pass. The live board’s boot-screen problem remains unresolved, so this repository does not claim that the current connected unit has passed the release checklist.

See [BUILD_STATUS.md](BUILD_STATUS.md) for the exact known/unknown state.
