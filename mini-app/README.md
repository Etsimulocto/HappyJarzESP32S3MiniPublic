# HAPPY JARZ Pi Mini app

Current app version: `1.11.0` (see `VERSION`).

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

The app shows a countdown to the next birthday. A February 29 birthday is observed on March 1 in non-leap years. On the birthday, the Jar runs a colorful light pattern and displays an OLED greeting while the clock has the correct date. If the Jar clock loses time, the saved profile remains; connecting the app synchronizes the clock again. CLEAR PROFILE removes the name and birthday from both the Pi and Jar.

## Art gallery

The ART tab includes the 128×64 monochrome frame maker with one- or two-frame drawings. Name and save each drawing to add it to **MY PICTURES**; the list has a scrollbar. Only saved drawings are transferred. The gallery holds up to 50 pictures and can be exported as U8g2 C headers. Use **Send gallery to Jar** to transfer the saved pictures over USB. The first transfer formats the board's dedicated art filesystem after an explicit confirmation.

After transfer, select **ART GALLERY** in the screensaver controls under **DISPLAY + INPUT** to start cycling through the stored drawings. Two-frame pieces animate on the OLED. The board can keep the gallery and run it with USB disconnected.

Each one-bit frame uses 1,024 bytes. Fifty two-frame pieces are about 100 KiB of image data. They are saved in SPIFFS flash storage, not in the settings NVS partition.
