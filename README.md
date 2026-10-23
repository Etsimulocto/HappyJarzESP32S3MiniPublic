# HAPPY JARZ ESP32-S3 Mini + Pi Mini App

This repository contains the HAPPY JARZ ESP32-S3 SuperMini firmware and its matching Raspberry Pi controller app.

> ## PRODUCTION SOURCE — October 23, 2026
>
> This branch is the current source used for the physical HAPPY JARZ production build.
>
> ```text
> branch: feature/mini-art-gallery-animation
> Pi Mini app: 1.11.2
> firmware: 0.18.7
> board: ESP32-S3 SuperMini / SIMPLE / 4 MB flash
> ```
>
> On the Raspberry Pi production machine this branch may appear in the historical worktree path:
>
> ```text
> ~/HappyJarzESP32S3MiniPublic-art-test
> ```
>
> Despite the `-art-test` name, this is the newer production-development stack. The default `main` branch is still an older 1.9.3 / 0.17.13 snapshot and must not be used for current production flashing until this branch is promoted.
>
> Physical validation on October 23, 2026 confirmed firmware **0.18.7** running with Pi Mini app **1.11.2**, including the expanded gamepad controls.

## Current release

| Component | Version / target |
| --- | --- |
| Firmware source | **0.18.7** |
| Pi Mini app | **1.11.2** |
| Board | SIMPLE / ESP32-S3 SuperMini / 4 MB flash |

Firmware, app, USB watcher, controller and art tools are all here. This checkout builds directly from `firmware/HappyJarzMini`; the BloomCircuit staging/patch flasher belongs to the older repository. See [firmware instructions](firmware/README.md) and [recorded verification](BUILD_STATUS.md).

## Shoulder brightness — firmware 0.18.6

L lowers lamp brightness and R raises it by five percentage points per new press, from 0% to the existing Pi app ceiling of 50%. Physical touch, app KEY commands and gamepad shoulder keys use the same qualified edge path. Changes are saved without changing the selected pattern, colors or screensaver. Holding a pad does not repeat; pressing both shoulders together leaves brightness unchanged. Start remains an independent input pending mode-selector/game integration.

Input telemetry now runs before OLED page handlers return, so live raw readings continue on Profile, Clock and Info pages as well as normal screens/games. Host regression tests passed; the full ESP32 compile/upload and physical response still require Pi verification.

## L / R / Start touch expansion — firmware 0.18.5

| Control | GPIO | Touch channel |
| --- | --- | --- |
| L shoulder | 11 | TOUCH11 |
| R shoulder | 12 | TOUCH12 |
| Start | 13 | TOUCH13 |

The existing six pads keep their assignments. Each new pad uses one signal wire to its own copper touch surface, with no pad connection to power or ground. Keep signal wires short and away from lamp/power wiring; install the pads before calibration and leave all pads untouched during boot or recalibration.

All nine pads share the proven algorithm: 40 samples with a 5 ms delay per pad, individual baseline, threshold 20% above baseline, 60 ms qualification and idle baseline tracking. Nine-pad calibration takes about 1.8 seconds plus read overhead. Physical sensitivity must be verified after assembly; thresholds are not copied from another pad's numeric reading.

GET CAPS advertises up,down,left,right,a,b,l,r,start. GET INPUT / GET TOUCH, streams and TOUCH_CAL include the extra fields. KEY L, KEY R and KEY START use the same logical input path. Extra press-edge events are emitted before UI/game early returns. L/R/Start remain independent controls: existing six-button menu/game actions are unchanged; new gameplay bindings are future work. Any physical pad can wake software sleep.

Pi controls already follow GET CAPS, so the extra buttons/readings become available after firmware connection. Web Display + Input now shows all nine raw readings and supported calibration values; older six-pad firmware leaves the extra controls disabled/N/A. X, Y and Select remain unsupported.

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

### Physical-input readouts — Pi app 1.11.2

The app refreshes INPUT readings once per second when firmware live telemetry stalls on an OLED page, including L/R/Start readings. It follows supported capabilities and leaves touch sensitivity unchanged. This app-only change requires restarting the Pi app, with no firmware upload. Physical extra-pad response still needs verification.

### USB startup fix — Pi app 1.11.1

Both controller and watcher now allow a 12-second identity handshake, retaining the 1.50-second boot settling delay and inactive DTR/RTS. The Pi traced the previous probe closing at 5.59 seconds while firmware 0.18.5 was still starting. This is an app-only change; no firmware compile/upload is required. Actual reconnect verification remains pending.

BloomScope and BRO login watchers can probe the same Espressif USB device with default serial control lines and interfere before identifying its firmware. Disable their competing login entries and stop those watchers when using this controller; see [Mini app startup notes](mini-app/README.md). Their applications can remain installed for manual use.

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

**Keep USB connected and the accessory switch steady until the upload finishes and the app opens.** A blank OLED during flashing does not mean the update is finished. Check the exit marker, upload log and fresh `fw=0.18.7` identity.

See [BUILD_STATUS.md](BUILD_STATUS.md) for the distinction between host tests, reported board results and remaining release checks.

## Browser controller draft

[HAPPY JARZ Web Studio](web-controller/README.md) is an initial USB browser frontend: profile/time sync, lights, live OLED, jar keys and a two-frame art editor. Firmware installation and automatic plug-in launching are not enabled. Fourteen Node tests passed. Initial Windows/Edge USB testing confirmed firmware 0.18.4, light commands and live telemetry; the Architect also confirmed delayed clock sync. Full feature checks and Pi browser performance remain pending. See its README for local preview, optional Pages deployment and the release-update design. The existing Mini app remains available.

### Reopen on your Windows PC

From the extracted repository folder containing `web-controller`, choose **Open in Terminal** and run:

```powershell
py -m http.server 8080 --directory web-controller
```

Keep the terminal open. Open [http://localhost:8080](http://localhost:8080) in Edge, attach the jar to this PC, click **Connect Jar**, and select its USB port. If everything was closed, restart the server with the same command. Export unsaved artwork before closing.

See [reopening and customer setup](web-controller/README.md#open-or-reopen-web-studio-on-windows) for connection help and the planned public website/optional Windows helper. Public website users will not need a GitHub account. Pages hosting, automatic browser reconnection, Windows plug-in launching and browser firmware installation are not yet enabled.

### Draft development history

Web Studio draft 0.2 adds the Pi app's main panel tabs, Service telemetry/tests, persistent bounded diagnostics and downloadable redacted support reports. BLOOPETZ and DND are reserved future panels. Firmware 0.18.4 timezone controls now use its supported IANA identifiers; broader international timezone support still needs a firmware extension. No firmware flash is required for this web-only change.


Web Studio draft 0.3 expands LIGHTS + CONTROL to follow the Pi panel: four Base/Live light cards, White/Off and individual color controls, 100 patterns, the 50% brightness ceiling, live touch tiles, capability-gated console keys, OLED mirror, optional keyboard/gamepad input and a captured pattern theme. Service reports remain downloadable locally; there is no central report inbox or automatic repair service yet. Nine logic tests and static wiring checks passed; browser/real USB checks remain pending.

Web Studio draft 0.4 moves each tab's JavaScript into its own module under `web-controller/tabs/`. USB, command sequencing and global diagnostics stay shared in `app.js`; page markup remains in `index.html`. Ten Node tests passed, including simulated panel integration covering controls, profile/clock, Wi-Fi, display, art upload, Service reports and reconnect. Actual browser/board checks are still required.

Web Studio draft 0.5 adds Monday's pink/teal collage identity, a six-image local gallery and click-to-open Suno link. The actual BloomSaver Glitter engine from the old BloomCircuit repo now fills the window behind the controller, with a live editor, sliders, symbols, seeds and local presets. All files are hosted locally with the web app. Twelve simulated/logic tests passed; visual browser and physical Pi performance checks remain pending.

Web Studio draft 0.6 publishes the Architect's glitter-jar image and selected Suno song. Monday's Corner is now read-only for visitors; media publishing uses GitHub repository write permissions. Public upload/edit/clear controls and browser media overrides were removed. Personal background tuning remains available.

Web Studio draft 0.7 completes the Wi-Fi + Clock tab from the Pi reference: 2.4 GHz scan/select, saved network setup and clearing, USB/internet clock controls, saved alarm and auto-off timer readouts. Time zones remain limited by firmware to four US regions and UTC. This is a web-only update.

Web Studio draft 0.8 completes Display + Input with the OLED mirror, eight screensavers and their controls/readouts, six-pad raw telemetry, Service-recorded calibration, and the Pi-style 50-slot marquee editor. This is a web-only update.

Web Studio draft 0.9 completes Profile: jar identity, confirmed name/birthday save/load/clear, save without changing the clock, protected unsaved edits, and birthday countdowns based on the jar’s reported local date. Profiles remain on the jar; this web-only update needs no firmware flash.

Web Studio draft 0.10 adds 24 monochrome emoji-style stamps, six draggable shape tools, fill/erase, adjustable brush and stamp sizes, and 40-step per-frame undo to Art. Existing two-frame drawing/export/upload formats remain compatible. No firmware flash.

Web Studio draft 0.11 expands Art to 96 curated emoji-style pixel stamps with search, six categories and a compact scrolling tray. Stamps remain web assets; board storage use is unchanged.

Web Studio draft 0.12 completes the Pi Service controls with a readable fuel gauge, optional live power refresh, separate Input test, individual diagnostic reads and Save to Jar. Downloadable test/log reports remain local and redacted. No firmware flash required.
