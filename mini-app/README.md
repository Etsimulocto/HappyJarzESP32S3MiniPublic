# HAPPY JARZ Pi Mini app

Current app version: `1.9.3` (see `VERSION`).

Launch the controller UI:

```bash
python3 app.py
```

The USB plug watcher sends the board a newline-terminated `HELLO` and opens this controller after the HAPPY JARZ identity reply.

Launch the USB plug watcher:

```bash
python3 happyjarz_plug_watch.py
```

Dependencies are Python 3, Tkinter, and pyserial. On Raspberry Pi OS, install Tkinter and pyserial with:

```bash
sudo apt-get install python3-tk python3-serial
```

The app includes the 128×64 monochrome art editor. Artwork is stored in `~/.happyjarz/art_library.json` and can be exported as U8g2 C headers. Firmware 0.17.13 has no art-transfer command, so saved art remains on the Pi until that firmware feature is designed and added.
