# HAPPY JARZ Mini build status

## November 22, 2026 — BLOOMPETZ + DND hardware-test handoff (latest)

**Build/source branch:** `feature/dnd-next-module`, `firmware/VERSION = 0.18.17`. **Published stable:** 0.18.15 on the public Web Studio release catalog; **0.18.16** was the first BLOOMPETZ test candidate, not a public release. Earlier 0.18.14 and 0.18.15 notes below are historical snapshots, not the current development status.

- **Verified on Board #3:** Windows Arduino ESP32 core 3.3.12 successfully compiled 0.18.17: **1,169,301 bytes (89% of 1,310,720 app limit)** and 58,344 bytes global RAM (17%). Default **4 MB dual-app partition map verified**. Candidate staged at `release-staging/candidate/happyjarz-0.18.17-app.bin`, SHA-256 **`3789820a0362a1799ee81531c5045735e5531cd63130fbef386a5ae8c8e26593`**. User's COM5 `esptool` log verified a **1,169,456-byte app-only flash at 0x10000**; no full-flash erase or partition reflash. User confirmed **DND gameplay works on hardware**, including the DND controls tested during the session. Detailed gameplay/save/reboot regression results were not provided; do not claim every path is certified.
- **Original source recovered and preserved:** Raspberry Pi `~/BloomCircuit/BloomPetz/firmware/bloompetz_v0_1/bloompetz_v0_1.ino` (144,753 bytes), verified SHA-256 **`110bbb5b17b4912364fe05f2824e755d2a29209a8d3d1af25449012b8733602d`** against a second Pi copy. Archived with nine historical `.pre_*` snapshots and the LED header: `Etsimulocto/BloomCircuit`, branch `archive/bloompetz-dnd-original-2026-11-22`, commit `d6f1deb`. Archive is deliberately **not** merged into BloomCircuit main. Other uncommitted Pi files were not staged or modified.
- **DND integration:** `firmware/HappyJarzMini/happyjarz_dnd.h` ports the original dungeon engine, 48 enemy species, extended equipment/item catalog, rooms, combat, shops, and inventory to HAPPY JARZ's shared 128x64 OLED (7-column HUD + 14-column view over four rows). D-pad moves; tap A then direction attacks; B opens menu; START exits back to HAPPY JARZ. DND has a separate menu visibility toggle and saves in **`hjdnd/state`**, isolated from **`hjpetz`**, **`bloomsys/state`**, existing `happyjarz` settings, and ART SPIFFS. **No original BloomCircuit save migration.**
- **Web Studio gamepad repair:** Xbox 360 pad `VID_045E PID_028E` enumerated normally on Windows and its buttons worked in the OS test. The Studio's browser-side gamepad relay had been limited to the **Lights + Control** tab and an opt-in checkbox; updated `web-controller/tabs/lights-control.js` to forward gamepad presses on **any tab** when the jar is connected, `Enable computer keyboard + USB gamepad` is checked and the Studio has focus. Keyboard shortcuts still work only on Lights + Control and pause while typing. The status label reports whether gamepad forwarding is ON/OFF. User confirmed **the local Web Studio/gamepad fix works**. No firmware reflash needed for this web-only fix. The **Pi desktop app's separate gamepad handler has not yet been fixed or re-tested**.
- **Local PC test server:** Windows PowerShell at `C:\Users\quart\Documents\HappyJarzESP32S3MiniPublic`: `py -3 -m http.server 8000 --directory .\web-controller`, then open `http://localhost:8000` in Edge; keep server open, connect the jar, enable host controls. This local test does **not** publish the changed Web Studio files to `jarz.spiralside.com`.
- **Preservation:** Never erase the device flash, NVS, or SPIFFS; use the verified app-only `0x10000` workflow. Verify the actual COM port after USB reconnect. The board may need an unplug/replug to recover USB enumeration after reset/flash; simply compiling on the PC does not reset it. Keep Pi Board #4 experiments and unrelated Git changes untouched. Firmware 0.18.17 is **physically tested but unpublished**; the production browser updater and complete persistence regression remain separate gates.

### Windows development commands (do not flash automatically)

```powershell
cd "$HOME\Documents\HappyJarzESP32S3MiniPublic"
git switch feature/dnd-next-module
git pull --ff-only origin feature/dnd-next-module
.\tools\build_release_windows.ps1
```



## November 22, 2026 - firmware 0.18.14 release and Board #3 field result

- **Current source/release:** `firmware/VERSION = 0.18.14` on `feature/mini-art-gallery-animation`. Windows build used Arduino ESP32 core **3.3.12**, SIMPLE ESP32-S3 V1, 4 MB flash, app-only image at `0x10000`, maximum app partition `0x140000`. Avoid core 4.0.0: a previous build produced unusable touch readings and unstable controls.
- **Physically confirmed on Board #3 (COM5):** flashing the staged image, firmware boot/connect, touch navigation, seven on-OLED arcade games, nine selectable screensavers including **BIG CLOCK**, correct local time/date *after manual re-sync*, Web Studio ART two-frame upload/playback, and **art persistence through power cycle**.
- **Art recovery:** `release-staging/board3-backup/spiffs.bin` was read locally and contained **1,441,792 bytes of 0xFF**, with no allocated/data blocks. The previously uninitialized SPIFFS partition had reported `ART STORAGE OFF` / `ready=0`. Explicit `ART FORMAT` was sent to initialize the **art-only** filesystem; later successful uploads, OLED playback, and persistence establish that storage is now working. Do **not** repeat format on a jar with saved art. NVS/profile was not targeted. Keep raw backups private (especially NVS).
- **Released artifact:** `web-controller/releases/happyjarz-0.18.14-app.bin`, **1,118,144 bytes**, SHA-256 `a92108a7acd697e761c9859d0a8b77f5a6eef2526fd19ff872b10fa251ddb712`. Release entry: `web-controller/releases.json`; publication commit `209fc11`. Live `https://jarz.spiralside.com` UPDATES page was photographed reporting **connected 0.18.14 / V1** and **published 0.18.14**, correctly disabling reinstallation of the same version.
- **Still unverified:** an actual *public website* browser firmware installation from an older version to 0.18.14, interruption/recovery, and general compatibility on other jars. The earlier **local** browser test of 0.18.13 succeeded in flashing, but that is not a production updater regression pass. Do not claim customer updater flashing is fully validated.
- **Post-flash USB note:** Board #3 sometimes hangs at the boot/blank screen after the flasher's RTS reset, yet runs on battery/phone charger, and plugging PC USB into an already-running unit is fine. A complete power-off, USB disconnect, brief wait, and normal battery startup restores it; root cause remains unconfirmed. Serial miniterm lost the COM interface during a reset. Keep serial clients disconnected while flashing.
- **Pi Board #4 is independent.** Do not overwrite its local touch-sensitivity experiments. The default `main` branch is historically older. The clock may need host/Wi-Fi time synchronization after full power loss; no battery-backed RTC is claimed.

**Board:** ESP32-S3 SuperMini (SIMPLE profile, 4 MB flash)  
**Firmware source and published release:** 0.18.14 (Board #3 physically tested; website installation from older firmware still pending)  
**Pi Mini app:** 1.11.2  
**Source of truth:** `Etsimulocto/HappyJarzESP32S3MiniPublic`

## What is in this repository

- The Happy Jarz Mini firmware sketch plus the arcade and art-gallery source files it includes.
- The matching Pi app, its controller layers, owner profile panel, art editor, single-instance USB watcher, and seven mini-games.
- One version file for firmware and one for the app.

The app and firmware source are together here. You do not need to pull app code or firmware patches from another checkout.

## Verification

- The two-frame art editor is in its own top-level ART tab; gallery transfer, 50-slot storage, and OLED playback are implemented.
- The PROFILE tab stores an owner's name and month/day birthday on the Pi keyed by chip ID and in Jar NVS. Firmware 0.18.2 adds an on-board editor using the six capacitive controls, manual date/time access, and a randomized startup countdown when a valid clock is available.
- The app synchronizes Jar time from the host when a board connects and when saving a profile.
- Birthday date validation and countdown tests have been added in `mini-app/test_profile.py`.
- The controller retries serial discovery while open and now requests exclusive access on POSIX, reducing conflicts with other USB watchers.
- The plug watcher uses a process lock so duplicate HAPPY JARZ autostart entries do not create duplicate watchers.
- The Pi serial reader shutdown race is handled without closing a newer connection from a stale reader.
- All eight pixel-art and transfer-protocol tests passed for the art implementation.
- Python syntax checks passed for the updated controller; the autostart installer passed shell syntax and temporary-home smoke checks.
- Firmware 0.18.1 compiled, uploaded, and reported `fw=0.18.1` on the Architect's 4 MB ESP32-S3 SuperMini.
- Firmware 0.18.2 compiled successfully on the Architect's Pi: 1,117,977 bytes of sketch space (85%) and 53,528 bytes of static RAM (16%). The Architect reported reconnecting after upload; detailed profile/countdown checks remain pending.
- Firmware 0.18.3 shows a five-second birthday countdown on every successful accessory OFF/ON restore, wakes software sleep and lets the overlay temporarily take priority over the arcade renderer. Host-side checks passed for repeated restores, fresh five-second timing, sleep wake, invalid birthday, delayed clock sync and birthday-day countdown. The Pi compiled this release, and the Architect reported reconnecting after retrying an interrupted upload; detailed physical switch checks remain pending.

- Firmware 0.18.4 adds animated cake/balloon/confetti/present/wish screens mixed with the selected saver, an A-to-open birthday wish, a slow rainbow shimmer that respects OFF and user color/pattern choices, and restoration at local midnight. Extracted C++ host tests passed for birthday activation, midnight restoration, lights override, gift timer, scene mixing, drawing bounds, accessory/sleep gating and Feb 29 behavior. The Architect gave positive feedback after the update instructions. A subsequent Windows/Edge browser log records a real `fw=0.18.4` identity and successful light/telemetry traffic. A complete 0.18.4 compile/upload log and detailed Birthday Mode checks have not been recorded, so full hardware verification remains pending.

## Field update lessons

- The Pi workflow runs compile and upload in a detached job; upload is gated on successful compilation.
- An interrupted 0.18.3 application write stopped around 16% and left the OLED blank. Compilation had succeeded. The Architect reported reconnecting after the upload retry.
- Use the documented 115200-baud upload override, keep the serial port free and wait for completion/app reopening before touching USB or the accessory switch.
- Exit `0`, final application hash verification and a fresh firmware identity establish upload/reconnection; they do not alone verify every feature.
- An absent completion marker must be checked against the process list and log before calling a build wedged.

This update does not change the partition table or erase-flash setting. The gallery uses the existing SPIFFS partition; the app asks before first-time formatting that filesystem. Owner profile values use Preferences/NVS and are separate from the art filesystem. Verify the firmware identity after upload before testing profile sync and birthday behavior.

## Release gate

Mark a firmware release verified only after a clean compile with the documented FQBN, a successful upload to the ESP32-S3 SuperMini, and checks for unique serial/chip identity, on-board profile editing and persistence across power cycles, manual/Wi-Fi time setting, randomized startup countdown, repeated accessory-switch countdown with return to the previous screen, birthday greeting/lights, A-to-open present, scene mixing with art, user light override and midnight restoration, OLED, touch controls, lamps, a 50-slot transfer, two-frame playback, and plug-in autolaunch.

## Web Studio draft 0.2

- Browser frontend includes LIGHTS + CONTROL, WIFI + CLOCK, DISPLAY + INPUT, ART, PROFILE, SERVICE and UPDATES panels; BLOOPETZ and DND panels are placeholders for future app/board work.
- Service provides timestamped telemetry, ping/RGB/touch routines, live touch readings, user observations and downloadable reports/logs. Firmware routine responses are separate from physical pass/fail observations.
- Local diagnostics capture serial TX/RX, request/device/USB errors, boot output, JS/unhandled/resource errors, console warnings and release fetch failures. Bounded history can persist across reloads; storage failures fall back to memory and are displayed. Known private fields/pixel payloads are redacted before persistence/export. Reports include board identity, browser/locale/timezone/USB context, readouts, test timings and user notes. Nothing is uploaded automatically.
- Six protocol/diagnostics tests passed, JavaScript syntax checks passed, and static UI validation found all nine tabs with no duplicate or missing wired controls. Browser smoke tests were extended for Service tests/report exports but remain unexecuted due to the unavailable Chromium download in this environment. Physical Pi/Windows USB verification remains pending; responsive layout does not establish phone/tablet USB support.
- Corrected the initial web draft's POSIX timezone selections: firmware 0.18.4 expects its supported IANA identifiers and otherwise falls back to US Central. UI now offers only supported US regions and UTC. Broader international timezone mapping remains future firmware work.
- No firmware/partition change, released binary, enabled flasher or deployed Pages site accompanies this draft.


## Web Studio draft 0.3 — Lights + Control

- Compared the web tab against the Pi app's controller core/UI/extensions and the current firmware commands. Added four Base/Live color cards with chooser/White/Off, 100-pattern live selection, debounced brightness with the confirmed 50% ceiling, save and refresh. LED_FRAME remains the source of live bulb colors.
- Live touch and OLED streams accompany identity/battery readouts. All twelve console inputs are visible, but only GET CAPS advertised keys are enabled. Keyboard and standard browser gamepad input are opt-in, focus/tab gated and pause while typing; unsupported gamepad mappings are not guessed.
- Captured pattern themes use a short live palette and readable text contrast. Explicit disconnect stops all three streams; a connection generation guard rejects old queued commands after reconnection. High-rate telemetry is sampled in the retained diagnostic log; errors are not sampled.
- Nine Node logic tests passed, including the existing protocol/report tests and new RGB, brightness, capability, gamepad and theme checks. Syntax and static UI wiring checks passed (92 unique HTML IDs). Simulated browser smoke coverage was extended, but remains unexecuted without a working Chromium installation. Real Pi/Windows USB and visual checks remain pending.
- No firmware change or flash is required. This update does not add a shared support inbox or automated remote repairs.

## Web Studio draft 0.4 — separate panel modules

- Each of the nine tabs has its own JavaScript file under `web-controller/tabs/`, including reserved Bloopetz/DND files. Shared page markup/CSS remain in `index.html`/`style.css`. App shell owns one USB port and queue; modules use injected shared requests and receive/reset/show hooks.
- Module load/init errors are logged and disable the affected tab; receive errors are isolated from the serial reader. Device controls are disabled while connecting/closing. Write failures and timeout callbacks are scoped to their own pending request.
- Ten Node tests passed: the nine existing logic tests plus actual module initialization and simulated serial integration covering all nine tab registrations, colors/pattern/50% clamp, capabilities/keyboard typing guard, Wi-Fi secret handling, alarms/timers, screensaver/marquee, profile/clock, complete two-frame upload, Service readout/ping/RGB/touch tests, redacted report export, rejection recovery, disconnect and reconnect.
- ES-module syntax checks passed. Integration uses a minimal DOM and simulated firmware, so it does not verify visual layout or real USB. Chromium browser smoke and physical Pi/Windows checks remain pending. No firmware change or flash is required.

## Web Studio draft 0.5 — Monday + BloomSaver

- Monday's host styling uses dark collage panels, pink/teal/yellow accents, expressive system headings and monospace readouts. The media corner accepts six locally compressed images and an HTTPS Suno song/playlist/share link. Published defaults live in monday-media.json; exact art/song assets remain empty pending selection. Local changes are not uploaded.
- Copied the original BloomCircuit BloomSaver Glitter engine into an isolated same-origin iframe background, with its 22 editable sliders, symbols, seeds, randomize/mutate and presets. Added edit/done, pause, current-setting persistence, reduced-motion default pause, hidden-page paint suppression, lower embedded defaults and about 30 FPS. Palette hooks are confined to this iframe.
- Twelve Node tests passed. Added original-engine VM checks for controls, pause/edit, message origin/source checks and stored settings, plus media URL validation and actual Monday module song-saving integration. Browser smoke coverage was extended for the iframe editor, slider and music link but remains unexecuted in this environment. Visual layout, iframe interactions and actual Pi/mobile rendering performance still require browser trials.
- No firmware change, paid hosting service, domain purchase, external font subscription or automatic support upload accompanies this update.

## Web Studio draft 0.6 — Architect-curated media

- Published the supplied glitter-jar PNG unchanged and Suno share link `https://suno.com/s/cuHjog2hpKZ3MOVF`. No visitor media upload/edit/clear UI remains, and legacy local media overrides are ignored.
- Content changes are committed through repository write permissions; the static site contains no admin password, write token or upload backend. Repository collaborators with write access can also publish, so the repository access list remains authoritative.
- Twelve Node checks passed, with integration coverage updated for published song/art loading and absent editing controls. Browser visual/USB verification remains pending. No firmware flash is needed.

## Web Studio draft 0.7 — Wi-Fi + Clock

- Ported Pi scan/select, saved Wi-Fi connect/test/clear, connection/IP/RSSI readout, host/internet time controls and shared supported time-zone selection.
- Separate alarm and timer board readouts; alarm validation and persistence, timer start/stop. Explicit refresh confirms asynchronous Wi-Fi/internet sync results.
- Expanded simulated USB integration coverage; all 12 Node tests pass. No physical board or browser layout validation in this change. No firmware change or flash.

## Web Studio draft 0.8 — Display + Input

- Added OLED preview and display readout, current saver mode/speed/idle/particle telemetry, control modes and six raw touch readouts.
- Added complete 50-slot marquee load/replace/clear, preserving slot gaps, validated before destructive replacement. Recalibration delegates to Service report history.
- All 12 Node tests pass with actual-module simulated USB coverage. Hardware and browser layout verification remain outstanding. No firmware change or flash.

## Web Studio draft 0.9 — Profile

- Added saved-profile/identity/date readouts, optional birthday removal, save-only and sync-save, confirmed load/clear and unsaved-edit protection.
- Added countdown from the jar’s last reported local date, leap-day observance and year rollover. Existing board birthday behavior is unchanged.
- All 13 Node tests pass with calendar and simulated USB profile coverage. No physical board or browser layout validation; no firmware flash.

## Web Studio draft 0.10 — Art stamps and shapes

- Added 24 original deterministic pixel icons, stamp scales, six shape tools, fill/erase, connected freehand brush strokes and per-frame undo.
- Added clipping, drag previews and cancellation rollback; existing XBM transfer format preserved.
- All 14 Node tests pass with raster and actual pointer-handler coverage. Hardware/browser layout validation remains outstanding. Web-only change.

## Web Studio draft 0.11 — Curated stamp library

- Expanded Art to 96 distinct original pixel masks, six categories, search and compact scrollable fixed-height rows.
- All 14 tests pass, including each mask’s erase/clipping and category/search integration. Browser appearance needs user verification. No firmware changes.

## Web Studio draft 0.12 — Pi Service parity

- Added fuel gauge (sensor, battery estimate, voltage, USB data, hardware charger indication, ADC), timestamp and optional five-second refresh.
- Added Input recalibration test, individual diagnostic reads and Save to Jar; recorded tests remain in support exports. Polling pauses when hidden/busy and avoids request buildup.
- All 14 Node tests pass with extended Service/fuel/report integration. Physical board/browser appearance checks remain outstanding. Web-only change.

## Initial Windows/Edge field test — 2026-10-10

- Real USB connection reported firmware 0.18.4, chip-derived serial/chip identity, SIMPLE capabilities, four lamps and six controls.
- The supplied log confirms profile/status/power/capability reads, ACKs for all four light-color commands, OLED/LED/input telemetry and explicit stream shutdown. The Architect reported that most controls worked.
- Clock initially reported unsynced; the Architect subsequently confirmed it synchronized after a delay. This was a user-observed result, not a supplied final TIME response.
- Duplicate identity lines during handshake remain an investigation item. A later chooser cancellation reported No jar selected.
- Chromium on Pi was reported to lag; Windows/Edge testing succeeded. Pi performance and full tab-by-tab physical checks remain pending.
- Reopening instructions now document the local server, keeping its terminal open, localhost and explicit Connect Jar. Public hosting, remembered-port automatic reconnection, optional Windows plug-in helper and packaging QR code are planned, not shipped.
- Documentation-only update; no firmware upload, runtime changes, enabled Pages deployment or installer release.

## Firmware 0.18.5 — L / R / Start

- Added GPIO11 L, GPIO12 R and GPIO13 Start; original pin assignments and touch sensitivity algorithm unchanged.
- Nine-pad calibration, raw telemetry, calibration fields, capabilities, host KEY mapping and extra edge events before UI early returns. No new game/menu aliases.
- Pi existing capability-driven buttons/readouts accept all nine keys; web Display + Input adds L/R/Start raw and calibration readouts while retaining six-pad compatibility.
- All 14 Node tests and two extracted firmware/Pi host tests pass. Python syntax check passed. Full ESP32 compile/upload, browser appearance and physical nine-pad sensitivity remain pending.
