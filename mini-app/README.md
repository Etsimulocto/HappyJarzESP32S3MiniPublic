# HAPPY JARZ Pi Mini app

Current app version: **1.11.0** (see `VERSION`). Matching firmware source: **0.18.4**.

From the repository root, launch with `python3 mini-app/app.py`; the command below is for running inside `mini-app/`.

Launch the controller UI:

```bash
python3 app.py
```

Install one desktop autostart watcher from the repository root:

```bash
sh mini-app/install_autostart.sh
```

Log out and back in to start it, or start the watcher manually for this session:

```bash
/usr/bin/python3 mini-app/happyjarz_plug_watch.py
```

The watcher uses a single-instance lock. The controller and watcher request exclusive serial access on POSIX and retry discovery when another USB watcher has the port.

Dependencies are Python 3, Tkinter, and pyserial. On Raspberry Pi OS, install Tkinter and pyserial with:

```bash
sudo apt-get install python3-tk python3-serial
```

## Owner profile

The PROFILE tab stores an owner's name and annual birthday (month and day; no birth year or age) in `~/.happyjarz/device_profiles.json`, keyed by the Jar's unique ESP32 chip ID. The Jar stores the same profile in its settings memory, so it remains there if the Pi profile file is unavailable. Save Profile + Sync Clock writes the profile and syncs the Jar's time over USB. The app also syncs time on connection.

The app shows a countdown to the next birthday. A February 29 birthday is observed on March 1 in non-leap years. Firmware 0.18.4 runs all-day Birthday Mode when the clock has the correct local date: animated cake/balloon/confetti/present/wish screens mixed with the selected screensaver, gentle rainbow shimmer at the selected brightness, and normal color/pattern restoration at midnight. On the present screen, physical or Mini **A** opens a random wish for six seconds; **B** exits the screensaver. Selecting a color or pattern takes control of the lights for the rest of the powered birthday session. OFF is honored. If the Jar clock loses time, the saved profile remains; connecting the app synchronizes the clock again. CLEAR PROFILE removes the name and birthday from both the Pi and Jar.

## Standalone board controls

Owners can also use **HOME → A → PROFILE → A** to set name, birthday and date/time without a Pi or PC. The Jar's profile remains in Preferences/NVS after full power loss, but its clock requires setting or syncing again.

The OLED/lamp 3.3 V accessory switch leaves the ESP32 powered. Every OFF → ON restore shows the birthday countdown for five seconds when the birthday and clock are valid; on the birthday it shows the cake greeting.

To try Birthday Mode, temporarily save today's month/day in PROFILE, check the birthday screens and present, then restore the real birthday.

## Updates and logs

The USB watcher checks this Git checkout for safe fast-forward source/app updates. Firmware must be compiled and uploaded separately using the [firmware update workflow](../firmware/README.md#compile-and-upload-on-raspberry-pi).

Keep USB connected and the accessory switch steady until upload completes and the app opens. Use `~/.happyjarz/firmware_update.exit` and `firmware_update.log` to check the update; use `plug_watch.log`, `controller_launch.log` and `controller.log` for launch/connection diagnostics.

## Art gallery

The ART tab includes the 128×64 monochrome frame maker with one- or two-frame drawings. Name and save each drawing to add it to **MY PICTURES**; the list has a scrollbar. Only saved drawings are transferred. The gallery holds up to 50 pictures and can be exported as U8g2 C headers. Use **Send gallery to Jar** to transfer the saved pictures over USB. The first transfer formats the board's dedicated art filesystem after an explicit confirmation.

After transfer, select **ART GALLERY** in the screensaver controls under **DISPLAY + INPUT** to start cycling through the stored drawings. Two-frame pieces animate on the OLED. The board can keep the gallery and run it with USB disconnected.

Each one-bit frame uses 1,024 bytes. Fifty two-frame pieces are about 100 KiB of image data. They are saved in SPIFFS flash storage, not in the settings NVS partition.
