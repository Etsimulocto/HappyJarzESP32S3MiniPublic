# HAPPY JARZ Pi Mini app

Current app version: `1.10.0` (see `VERSION`).

Launch the controller UI:

```bash
python3 app.py
```

Launch the optional USB plug watcher:

```bash
python3 happyjarz_plug_watch.py
```

Dependencies are Python 3, Tkinter, and pyserial. On Raspberry Pi OS, install Tkinter and pyserial with:

```bash
sudo apt-get install python3-tk python3-serial
```

The app includes the 128×64 monochrome art editor with one- or two-frame drawings. Artwork is stored in `~/.happyjarz/art_library.json`; the gallery holds up to 50 pictures and can be exported as U8g2 C headers. Use **Send gallery to Jar** to transfer the saved pictures over USB. The first transfer formats the board's dedicated art filesystem after an explicit confirmation. Select **ART GALLERY** in the screensaver controls to rotate through the stored drawings; two-frame pieces animate on the OLED.

Each one-bit frame uses 1,024 bytes. Fifty two-frame pieces are about 100 KiB of image data. They are saved in SPIFFS flash storage, not in the settings NVS partition. The board can keep the gallery and run it with USB disconnected.
