# HAPPY JARZ ESP32-S3 Mini + Pi Mini App

This is the single source repository for the HAPPY JARZ ESP32-S3 SuperMini firmware and its matching Raspberry Pi controller app.

## Current source pair

- Firmware: **0.18.2**
- Pi Mini app: **1.11.0**
- Board profile: **SIMPLE / ESP32-S3 SuperMini / 4 MB flash**

The repository contains the firmware, Mini app, USB watcher, controller, and art tools. It does not depend on BloomCircuit or another checkout.

## Run the Pi Mini app

On Raspberry Pi OS:

```bash
sudo apt-get install python3-tk python3-serial
python3 mini-app/app.py
```

The app includes LIGHTS + CONTROL, WIFI + CLOCK, DISPLAY + INPUT, PROFILE, ART, and SERVICE tabs. PROFILE stores the owner's name and annual birthday (month and day only) on the Pi, keyed to the Jar's unique ESP32 chip ID, and in the Jar's settings memory. It shows a live countdown. On the birthday, the Jar runs a colorful light pattern and shows an OLED greeting while its clock has the correct date. The app syncs the Jar clock whenever it connects over USB. Firmware 0.18.2 also adds an on-board PROFILE editor for standalone owners, manual date/time access, and a randomized birthday countdown at startup when the clock is set.

The ART tab has a 128×64 monochrome editor and frame maker. Create a still picture or two-frame animation, name it, and save it to **MY PICTURES**. Only saved pictures are included in a gallery transfer. MY PICTURES has a scrollbar for browsing the saved gallery.

Choose **Send gallery to Jar** to transfer the saved pictures. The board stores them in its dedicated SPIFFS art filesystem, so it can display them with USB disconnected. To start cycling through the transferred pictures, open **DISPLAY + INPUT** and select **ART GALLERY** in the screensaver controls. Two-frame pictures animate on the OLED.

The first gallery transfer asks before initializing the art filesystem. No partition table or erase-all setting is changed.

## Launch the app when a Jar is plugged in

Install exactly one HAPPY JARZ desktop autostart entry:

```bash
sh mini-app/install_autostart.sh
```

Then log out and back in, or start the watcher once for the current session:

```bash
pkill -f '[h]appyjarz_plug_watch.py' || true
/usr/bin/python3 "$PWD/mini-app/happyjarz_plug_watch.py" >/dev/null 2>&1 &
```

The watcher opens the controller only after a HAPPY JARZ identity handshake. The reported serial is derived from the chip ID, so different boards no longer all report the same placeholder serial. The watcher has a single-instance lock, and the controller keeps retrying if another USB watcher briefly owns the port. Check `~/.happyjarz/plug_watch.log` and `~/.happyjarz/controller.log` if the app does not connect.

## Compile the Mini firmware

```bash
arduino-cli core install esp32:esp32
arduino-cli lib install U8g2
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc firmware/HappyJarzMini
```

Build successfully before uploading. Firmware 0.18.2 has not yet passed a clean compile or board test; see `firmware/README.md` and `BUILD_STATUS.md`.

See [firmware/README.md](firmware/README.md) and [BUILD_STATUS.md](BUILD_STATUS.md) for the build steps and verification state.
