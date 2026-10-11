# HAPPY JARZ ESP32-S3 Mini + Pi Mini App

This repository contains the HAPPY JARZ ESP32-S3 SuperMini firmware and its matching Raspberry Pi controller app.

## Current release

| Component | Version / target |
| --- | --- |
| Firmware | **0.18.4** |
| Pi Mini app | **1.11.0** |
| Board | SIMPLE / ESP32-S3 SuperMini / 4 MB flash |

Firmware, app, USB watcher, controller and art tools are all here. This checkout builds directly from `firmware/HappyJarzMini`; the BloomCircuit staging/patch flasher belongs to the older repository. See [firmware instructions](firmware/README.md) and [recorded verification](BUILD_STATUS.md).

## Raspberry Pi app

From the repository root:

```bash
sudo apt-get install python3-tk python3-serial
python3 mini-app/app.py
```

The app includes LIGHTS + CONTROL, WIFI + CLOCK, DISPLAY + INPUT, PROFILE, ART and SERVICE tabs. See [Mini app instructions](mini-app/README.md).

Install one HAPPY JARZ desktop autostart entry:

```bash
sh mini-app/install_autostart.sh
```

Log out and back in, or start the watcher for this session:

```bash
pkill -f '[h]appyjarz_plug_watch.py' || true
nohup /usr/bin/python3 "$PWD/mini-app/happyjarz_plug_watch.py" >/dev/null 2>&1 </dev/null &
```

The watcher launches the app only after receiving a HAPPY JARZ identity. It uses a single-instance lock, checks this checkout's Git upstream for fast-forward app/source updates, and leaves firmware uploading to the separate update process. The board's serial is derived from its ESP32 chip ID. Logs are in `~/.happyjarz/`.

## Standalone profile and birthday countdown

Press **A** from HOME, scroll to **PROFILE**, then press **A**. Owners can save their name and annual birthday, set date/time, or clear the profile using the six touch controls. No Pi or PC is needed for these board controls.

The birthday stores month/day only. A February 29 birthday is observed on March 1 in non-leap years. Profile values remain in Preferences/NVS after power loss.

With a saved birthday and valid clock:
- Restoring the OLED/lamp **3.3 V accessory switch** shows the countdown for **five seconds**, then returns to the previous screen.
- A full ESP32 startup has a one-in-three countdown chance on ordinary days.
- On the birthday, the Jar always queues an animated cake greeting when the date becomes available.

The accessory switch powers the screen and lamps while the ESP32 continues running. After full ESP32 power loss, its clock needs manual setting, Wi-Fi time sync or app USB time sync; there is no battery-backed RTC.

## All-day Birthday Mode

Birthday Mode follows the owner's local date:
- A five-second animated cake greeting at startup or accessory switch-on.
- Slow rainbow shimmer at the selected brightness. OFF remains available; choosing a lamp color or pattern takes control of the lights for the rest of that powered birthday session.
- Cake, balloons, confetti, present and wish screens mixed with the chosen screensaver/art gallery.
- **A** on the present reveals a random wish for six seconds; **B** exits the screensaver.
- At midnight, birthday visuals end and the normal color/pattern choices resume.

Games, menus, accessory pause and the existing one-hour sleep behavior remain usable. To test, temporarily save today's birthday in PROFILE and restore the real birthday afterward.

## Art gallery

The ART tab provides a 128×64 monochrome editor for still pictures and two-frame animations. Name and save drawings to **MY PICTURES**; only saved pictures are transferred.

**Send gallery to Jar** transfers up to 50 pictures into the existing SPIFFS art filesystem. The first gallery initialization asks for confirmation. Select **ART GALLERY** under DISPLAY + INPUT to play the pictures on the OLED. Stored art works with USB disconnected.

## Firmware updates

Follow the [compile and upload workflow](firmware/README.md#compile-and-upload-on-raspberry-pi). It runs detached, compiles before uploading, uploads at 115200 baud and restarts the watcher after success.

**Keep USB connected and the accessory switch steady until the upload finishes and the app opens.** A blank OLED during flashing does not mean the update is finished. Check the exit marker, upload log and fresh `fw=0.18.4` identity.

See [BUILD_STATUS.md](BUILD_STATUS.md) for the distinction between host tests, reported board results and remaining release checks.

## Browser controller draft

[HAPPY JARZ Web Studio](web-controller/README.md) is an initial USB browser frontend: profile/time sync, lights, live OLED, jar keys and a two-frame art editor. Firmware installation and automatic plug-in launching are not enabled. Protocol tests passed; browser and physical Pi/Windows checks remain pending. See its README for local preview, optional Pages deployment and the release-update design. The existing Mini app remains available.

Web Studio draft 0.2 adds the Pi app's main panel tabs, Service telemetry/tests, persistent bounded diagnostics and downloadable redacted support reports. BLOOPETZ and DND are reserved future panels. Firmware 0.18.4 timezone controls now use its supported IANA identifiers; broader international timezone support still needs a firmware extension. No firmware flash is required for this web-only change.


Web Studio draft 0.3 expands LIGHTS + CONTROL to follow the Pi panel: four Base/Live light cards, White/Off and individual color controls, 100 patterns, the 50% brightness ceiling, live touch tiles, capability-gated console keys, OLED mirror, optional keyboard/gamepad input and a captured pattern theme. Service reports remain downloadable locally; there is no central report inbox or automatic repair service yet. Nine logic tests and static wiring checks passed; browser/real USB checks remain pending.

Web Studio draft 0.4 moves each tab's JavaScript into its own module under `web-controller/tabs/`. USB, command sequencing and global diagnostics stay shared in `app.js`; page markup remains in `index.html`. Ten Node tests passed, including simulated panel integration covering controls, profile/clock, Wi-Fi, display, art upload, Service reports and reconnect. Actual browser/board checks are still required.

Web Studio draft 0.5 adds Monday's pink/teal collage identity, a six-image local gallery and click-to-open Suno link. The actual BloomSaver Glitter engine from the old BloomCircuit repo now fills the window behind the controller, with a live editor, sliders, symbols, seeds and local presets. All files are hosted locally with the web app. Twelve simulated/logic tests passed; visual browser and physical Pi performance checks remain pending.

Web Studio draft 0.6 publishes the Architect's glitter-jar image and selected Suno song. Monday's Corner is now read-only for visitors; media publishing uses GitHub repository write permissions. Public upload/edit/clear controls and browser media overrides were removed. Personal background tuning remains available.

Web Studio draft 0.7 completes the Wi-Fi + Clock tab from the Pi reference: 2.4 GHz scan/select, saved network setup and clearing, USB/internet clock controls, saved alarm and auto-off timer readouts. Time zones remain limited by firmware to four US regions and UTC. This is a web-only update.
