# HAPPY JARZ ESP32-S3 Mini + Pi Mini App

> ## ⚠️ LEGACY SNAPSHOT — DO NOT USE FOR CURRENT PRODUCTION BUILDS
>
> The `main` branch is an older packaged snapshot (**Pi Mini app 1.9.3 / firmware 0.17.13**).
> It is **not** the source used for the October 23, 2026 physical production build.
>
> **Current production-development source:**
>
> ```text
> branch: feature/mini-art-gallery-animation
> Pi Mini app: 1.11.2
> firmware: 0.18.7
> board: ESP32-S3 SuperMini / SIMPLE / 4 MB flash
> ```
>
> On the Raspberry Pi used for the production build, that branch is checked out as:
>
> ```text
> ~/HappyJarzESP32S3MiniPublic-art-test
> ```
>
> The `-art-test` directory name is historical. It currently contains the newer HAPPY JARZ Mini firmware/app stack. Do **not** flash from `~/HappyJarzESP32S3MiniPublic` while its `main` branch remains on this legacy snapshot.
>
> Production validation on October 23, 2026 confirmed firmware **0.18.7** and Pi Mini app **1.11.2** with the expanded gamepad controls working in the app.
>
> The intent is to promote the validated production branch back to `main`; until that happens, treat this branch as archival/reference only.

## Legacy snapshot details

This branch contains the earlier standalone HAPPY JARZ ESP32-S3 SuperMini package.

### Snapshot source pair

- Firmware: **0.17.13**
- Pi Mini app: **1.9.3**
- Board profile: **SIMPLE / ESP32-S3 SuperMini / 4 MB flash**
- Source import: BloomCircuit commit `7412f7ef0c5faf84310b7c7fe1612f29f6bb84ba`

The files below remain useful for historical reference, but they are not the current production source.

## Run this legacy Pi Mini app

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

The watcher launches the app only after a HAPPY JARZ `HELLO` identity handshake.

## Compile this legacy Mini firmware

Install the ESP32 Arduino core and U8g2, then compile the included sketch:

```bash
arduino-cli core install esp32:esp32
arduino-cli lib install U8g2
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc firmware/HappyJarzMini
```

Do not erase flash as a routine update; that can remove saved Jar settings.

## Historical verification status

The v0.17.13 staging verifier passed when this package was assembled, but this branch is superseded by the current production-development branch described above.

See [BUILD_STATUS.md](BUILD_STATUS.md) for this snapshot's historical verification record.
