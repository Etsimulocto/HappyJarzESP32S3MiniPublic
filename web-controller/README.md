# HAPPY JARZ Web Studio — draft 0.13

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

### Publish procedure (verified exact binary)

Use Windows PowerShell in the feature-branch repository after the release candidate was built and **physically verified**:

```powershell
git pull --ff-only origin feature/mini-art-gallery-animation
powershell -ExecutionPolicy Bypass -File .\tools\publish_tested_01814_windows.ps1 -Push
```

The publisher checks the staged **exact known 0.18.14 SHA-256 and size** before copying the binary into `web-controller/releases/` and writing `releases.json`; then commits and pushes. The published website must deploy that commit separately. The live page has been verified to see the manifest. On a jar **already on 0.18.14**, the **Install verified update** button correctly stays disabled. The updater supports only a *newer* released version. A live browser flash from an older installed version is still a pending validation.

A static USB browser controller for the existing SuperMini SIMPLE protocol. No backend, account, Python installation, or paid domain required for website users. This is a development implementation with initial PC hardware results, not a fully verified replacement for the Mini app.

## Unreleased 0.18.15 candidate - real OLED menu visibility

**Not published, flashed or compiled yet.** The latest feature branch now contains an experimental 0.18.15 firmware source and matching Web Studio controls. The stable public release entry and exact `0.18.14` application binary are unchanged.

Firmware menu: eight original OLED entries map to bits **0 CLOCK, 1 LIGHTS, 2 GAMES, 3 SETTINGS, 4 POWER, 5 SYSTEM, 6 INFO, 7 PROFILE**. `SYSTEM` bit 5 is enforced ON in RAM, reads and writes; the menu list dynamically skips hidden entries while selection preserves the original item-to-screen mapping. It persists one byte in existing `happyjarz` Preferences under `menu_mask`; existing NVS, art, Wi-Fi and owner values are not intentionally cleared. On first upgrade with no `menu_mask`, all original menu entries remain visible.

Commands supported by **candidate firmware only**:

```text
GET MODULES
HJ|MODULES|menu_mask=255|system_locked=1|arcade=1|bloompetz=0|dnd=0|hid=0
SET MODULES 36
HJ|ACK|command=SET MODULES
HJ|MODULES|menu_mask=36|system_locked=1|arcade=1|bloompetz=0|dnd=0|hid=0
RESET MODULES
HJ|ACK|command=RESET MODULES
```

Example mask 36 shows GAMES + SYSTEM. This is **menu visibility only** and does not reclaim bytes, install/remove modules, or suppress underlying physical HOME actions. Existing touch/USB interface behavior is unchanged. Web Studio's new On-device OLED menu section is version gated to **0.18.15+**, supports read/apply/restore and demands an explicit confirmation before applying.

Before any board installation: compile on **ESP32 Arduino core 3.3.12** with the usual 4MB partition settings, run host tests, back up Board #3 NVS/SPIFFS again, and test menu navigation at masks 255, 32, 36, 128, 0 (SYSTEM remains), full power cycle/persistence and RESET MODULES, as well as normal art, seven arcade games, lights, clock, USB Web Studio and update compatibility. Do not alter the released catalog until a **verified** app build has passed hardware testing.

## Experience Manager + Control Mapper (November 22, 2026)

### Experience storage meter (November 22, 2026)

The Experience Manager now has two deliberately distinct displays:

- **Application flash:** exact 0.18.14 verified binary is **1,118,144 bytes** of the **1,310,720-byte app partition** (about **85.3% used**, **192,576 bytes free**). This value is displayed as board-specific only when the connected jar identifies as 0.18.14; other versions are labelled *unmeasured*, not assigned 0.18.14's size. The app is a partition image; it does not include whole-chip bootloader/other partitions.
- **SPIFFS/Art:** partition capacity **1,441,792 bytes (1,408 KiB)**. The **Refresh artwork slots** button runs the existing `ART COUNT` serial command and displays occupied slots out of 50, plus the known 1,024–2,048 bytes *picture payload* per occupied slot. Firmware 0.18.14 does **not** expose real SPIFFS used/free bytes; that number remains explicitly unknown and cannot be inferred safely from slot count alone.
- **Per-feature cards:** already compiled features read `Already included · 0 KiB freed if hidden`. BLOOMPETZ/DND/HID read `Not installed · 0 KiB used` on the current edition. This is not a per-module size breakdown: individual linked section footprints require controlled comparison builds with the exact Arduino toolchain and linker settings.
- The storage read operation is read-only, does not format SPIFFS, erase artwork, or modify NVS. No firmware changes accompany this UI addition.



**New Web Studio tab: EXPERIENCES + MAPPING.** First-stage browser implementation, not an ESP32 firmware upgrade.

- **Experience presets:** factory Classic, Pet Only, DND player, Business Display, and Everything/Maker. The full list of feature switches is editable. **Simplified navigation** hides deselected *Web Studio tabs*, with the Experiences tab always reachable for recovery. This does not remove compiled firmware, change the on-device menu or uninstall apps.
- **Nine editable physical touch assignments:** UP GPIO4, DOWN GPIO5, LEFT GPIO9, RIGHT GPIO10, A GPIO1, B GPIO2, L GPIO11, R GPIO12, START GPIO13. Includes Spiralside gamepad, DAW and display-controller templates; explicit per-button selection and value fields.
- **Live opt-in actions on current 0.18.14:** Physical `HJ|EVENT|input=...` lines can trigger safe existing commands (`SAVER NEXT/PREV/ENTER/EXIT/RESEED`, `SAVE`) **while connected in Web Studio**. Host `source=host` echoes are rejected, actions are serialized with a small debounce and commands require advertised controls. Board controls **still perform their normal physical functions**; these are added actions, not exclusive remaps.
- **Not yet active:** A physical USB HID keyboard/gamepad or USB MIDI device, OS-level shortcuts (e.g., Ctrl+Z), and true on-device macro/remapping. Assignments for these are **editor presets only** until composite HID/MIDI firmware and an explicit device Control Mode are built and tested. Browsers cannot inject trusted system-wide OS input from serial events. BLOOMPETZ/DND firmware installation is likewise not implemented.
- **Persistence:** Browser `localStorage` only (same computer/browser, not synced to the jar). JSON import/export and factory restore are provided. Restore never writes jar flash, NVS or SPIFFS. The web updater is unaffected.
- **Safety to address before eventual device support:** preserve serial firmware recovery and protected exit from Control Mode; avoid accidental macros during ordinary jar navigation; version and validate device mappings; isolate saved game/pet state and require explicit confirmation before any firmware edition install.

Source files: `tabs/experience.js`, `app.js`, `index.html`, `style.css`. This UI must be deployed by the site's normal publishing process; committing a feature branch is not proof the public site has updated. No physical board test of the new tab has been recorded.

## Implemented

- Main Pi panel tabs: LIGHTS + CONTROL, WIFI + CLOCK, DISPLAY + INPUT, ART, PROFILE and SERVICE; UPDATES remains separate. BLOOPETZ and DND are reserved future panels, not implemented games or board features.
- Wi-Fi credentials/connect/status, alarm/timer controls, current screensaver selection/speed/variation, input mode and custom marquee slot/source controls.
- Service readouts include capabilities, fuel gauge/ADC/USB/charger hardware state, LED/OLED telemetry, touch input/calibration, Wi-Fi, clock, alarm/timer, screensaver and last accessory/sleep events. Readouts carry UTC receive timestamps and become last-known readings after disconnect. They do not infer values the firmware never reported.
- Ping, RGB and touch calibration routines plus optional live touch polling. RGB/calibration require explicit confirmation. Firmware PASS indicates routine completion; physical light/screen/touch observations are recorded separately.
- Local diagnostics record TX/RX (OLED/LED/input telemetry sampled at most once per second per type; all device errors are recorded), boot output/panic lines, device errors, request timeouts, connection/cleanup errors, release fetch failures, JS/unhandled/resource errors and console warnings/errors. Logs survive page reloads when browser storage permits, with memory fallback. Retention is bounded to 2,000 events / approximately 700,000 serialized characters, with drop count and storage health shown. Each event is capped at 4,000 characters. This is not an unlimited device-side crash recorder, and failures before the logging module starts cannot be captured.
- Downloadable JSON support report and readable log; no automatic upload. Report includes browser/language/timezone/USB identifiers, firmware/board ID, readouts, test timings/results and user notes/observations. Known passwords/profile/network fields and screen/art payloads are redacted before local persistence and report export. Review user-written notes before sharing. No precise location or automatic country lookup is collected.

- Explicit USB port selection, repeated HELLO handshake, bounded line buffering, serial command queue, timeout/error messages and disconnect cleanup.
- Lights + Control follows the Pi panel: four independent Base/Live swatches with color chooser, White and Off; 100 patterns; live pattern selection; debounced brightness capped at the board/Pi 50% ceiling; SAVE and refresh. Base colors come from STATUS; live colors come from LED_FRAME. Per-bulb Off changes its base color, so animated patterns can still illuminate that bulb. Choose SOLID to display the four base colors continuously or OFF to stop the whole scene.
- Twelve touch tiles and console keys, enabled only when GET CAPS advertises the corresponding input. SIMPLE boards enable up/down/left/right/A/B; the other six remain visible as N/A. Raw touch values, input-event flashes, live 128×64 OLED, identity and battery telemetry are shown together.
- Optional focused-tab keyboard and standard-mapped browser gamepad input sends press edges through the same KEY protocol. Typing, other tabs, disabled host input and unsupported keys do not send commands. Nonstandard gamepads need an explicit mapping before use; none is guessed. The browser may require a controller button press before exposing the gamepad.
- Optional pattern-derived app theme captures a short palette from live LED frames, then stays fixed until the pattern changes. Text contrast is adjusted for readability; dark/OFF frames retain the previous theme. Reset theme restores the default palette.
- OLED, LED and touch streams start on connection and are stopped on explicit disconnect. Old queued requests cannot execute on a replacement USB connection.
- Saved owner name/birthday and selected firmware-supported IANA time zone; host-time sync.
- Existing screensaver enter/next/exit commands.
- Two-frame 128×64 XBM pixel editor, preview, validated JSON import/export and ACK-gated upload to one of 50 slots. Upload replaces the selected slot after confirmation. The draft never formats or trims the gallery.
- The UPDATES panel now includes a gated Web Serial application-only flasher. The release catalog now contains the hardware-tested 0.18.14 binary and manifest. The installer validates board metadata and SHA-256, and only offers a version newer than the jar's installed version; live public browser flashing from an older version is not yet validated. No firmware or partition changes accompany this website build.

## Monday + live BloomSaver background

Monday hosts the studio with a dark pink/teal/yellow collage skin, bold system-font headings, monospace instrument readouts and a welcome sticker. The optional pattern-tinted control theme now starts off so Monday's default branding remains visible.

The full-window backdrop is the actual **BloomSaver — Glitter** engine from `Etsimulocto/BloomCircuit/BloomSaver`, copied locally under `bloomsaver/`. It fills the margins around the responsive controller. **Edit live background** opens the existing 22-slider laboratory with symbols, moving emitters, attractors/repulsors, drift, swirl, noise, bursts, glow, trails, color/background fade, seed, randomize/mutate and saved presets. **Done editing** or Escape returns to the controller without restarting the simulation. The engine runs in a same-origin iframe; its canvas palette override cannot affect the controller OLED/art canvases, and its keyboard shortcuts do not intercept main-page typing or gamepad controls.

Embedded defaults are 180 particles, reduced glow, capped pixel density and approximately 30 FPS. Pause/resume is available. Reduced-motion preference starts paused unless the user has explicitly saved a setting; hidden pages skip painting. Current slider settings, randomize/mutate/seed/load results and named presets persist locally when storage permits. Saved settings are range-clamped on load. Controls remain editable and do not auto-cycle presets. Performance with maximum particle/glow settings still needs testing on actual Pi/mobile hardware.

**MATCH JAR COLORS ON START** retains the original optional startup palette handoff from live LED_FRAME colors; it does not continuously overwrite user edits. The palette bridge accepts only same-origin parent messages. Background errors are forwarded into Service diagnostics.

**Monday's Corner** is a public, read-only gallery and click-to-open Suno song/playlist/share link. The Architect's supplied glitter-jar PNG and `https://suno.com/s/cuHjog2hpKZ3MOVF` are now the published media. Music opens Suno in a new tab; it does not autoplay or store audio in GitHub.

There are no visitor uploads, music editing forms, media clear buttons, local media overrides, client-side admin passwords or exposed write credentials. Published additions require GitHub repository write access. This uses GitHub's existing account permissions rather than pretending a hidden browser button is authorization. Visitors may still change their own background/glitter settings and control their connected jar.

To publish new media, maintainers add PNG/JPEG/WebP files under `web-controller/assets/`, then edit `monday-media.json` with up to six `./assets/filename` paths, a title and an HTTPS Suno link. Commit to the serving branch and deploy/pull as usual. Authorized repository collaborators can publish too; restricting that list is a repository access setting. Ordinary visitors cannot modify the published repository through this app.

No external font service, paid asset service, backend or new subscription was added. The uploaded artwork is stored unchanged; future web-size copies can reduce download size if requested.

## Panel code layout

Each tab has its own JavaScript module under `web-controller/tabs/`. `index.html` retains the shared page markup and `style.css` retains the responsive styles.

| Tab | Code file |
| --- | --- |
| LIGHTS + CONTROL | `tabs/lights-control.js` |
| WIFI + CLOCK | `tabs/wifi-clock.js` |
| DISPLAY + INPUT | `tabs/display-input.js` |
| PROFILE | `tabs/profile.js` |
| ART | `tabs/art.js` |
| SERVICE | `tabs/service.js` |
| UPDATES | `tabs/updates.js` |
| BLOOPETZ | `tabs/bloopetz.js` (reserved) |
| DND | `tabs/dnd.js` (reserved) |

`app.js` owns the single USB session, command queue, connection state, navigation and global error capture. Each panel exports `init(ctx)` and optional `receive`, `reset` and `onShow` hooks. Panels use shared request/ACK/query methods; they never open their own serial port. Profile saving calls the Clock panel through the shared context. Service owns the report UI and diagnostic display; persistent log storage/redaction stays in `diagnostics.mjs`.

Modules load independently. Load/initialization failures disable the affected tab and produce a PANEL error in the retained log; receive-hook failures are logged without ending the USB reader. UI controls remain disabled during initial connection/disconnect. Timers and failed writes reject only the request they belong to, avoiding interference with later commands.

## Open or reopen Web Studio on Windows

Open the extracted repository folder containing `web-controller`, right-click inside it and choose **Open in Terminal**. Run:

```powershell
py -m http.server 8080 --directory web-controller
```

Leave that terminal open while using the local app. In Edge or a desktop browser supporting Web Serial, open [http://localhost:8080](http://localhost:8080), plug the jar directly into this computer, click **Connect Jar**, and select its USB serial port.

If you closed everything, repeat those steps. If the server is still running, just reopen the link. If port 8080 is already in use, check whether the existing server is still serving the app before starting another one. A localhost link refers to the computer opening it, not the Pi or another computer on the network.

Close other controllers using the jar. On Pi, stop the Mini app and its watcher using the commands below. The current browser draft requires explicit connection after reopening; remembered-jar automatic reconnection is planned, not implemented. Export unsaved drawings before closing the page.

If the app opens but cannot connect, confirm that the jar is attached to this computer with a USB data cable and select the jar's USB port. Pi `ttyAMA0` / `ttyAMA10` are UARTs, not the jar's tested `ttyACM0` USB port. Cancelling the chooser produces **No jar selected**.

## Customer setup plan — not yet shipped

- Publish the controller and a simple setup page over HTTPS. Public website visitors will not need a GitHub account, Python or terminal commands. Repository write access is needed only by maintainers publishing changes.
- Print a readable website address and a QR code on the instructions/packaging. The customer's phone camera scans the printed code; the jar needs no camera. Generate the code after the permanent address is verified. A QR code only opens a link; it does not grant USB permission or install software.
- Explain first-time **Connect Jar** permission, compatible devices/browsers, reconnecting and common cable/port problems. A phone-friendly layout does not establish phone/tablet USB support.
- Add remembered-port reconnection while the page is open. A website cannot launch itself from a closed browser when USB is inserted.
- Build and test an optional one-time Windows helper installation for plug-in launching, similar in purpose to the Pi watcher. It must release the port before browser connection or firmware flashing.
- Keep website updates separate from board firmware updates. The browser flasher is guarded by a published verified release manifest and explicit confirmation; plugging in a jar must never silently flash development firmware.
- Test the full customer flow with someone following only the supplied instructions.

GitHub Pages is the proposed hosting option; a paid domain is optional. The deployment has not been enabled or verified, so the expected URL below is not advertised as a working customer link.

## Try locally on Pi / Linux

From the repository root:

```bash
python3 -m http.server 8080 --directory web-controller
```

Open `http://localhost:8080` on that same computer. USB requires a secure context: localhost works for development; a published site must use HTTPS. An ordinary HTTP address on another machine is not sufficient.

Use a browser exposing `navigator.serial`, initially test desktop Chrome/Chromium or Edge. Feature detection is used rather than a browser-name promise. Current support varies by version/platform; iOS USB installation is not supported by ESP Web Tools. Close the Mini controller and stop its watcher before using the browser on Pi, otherwise they may compete for the port:

```bash
pkill -f '[h]appyjarz_plug_watch.py' || true
pkill -f '[m]ini-app/app.py' || true
```

After closing/disconnecting the browser, restore the existing watcher from the repository root:

```bash
nohup /usr/bin/python3 "$PWD/mini-app/happyjarz_plug_watch.py" >/dev/null 2>&1 </dev/null &
```

The first connection requires the browser's device chooser. No automatic website opening or background Windows helper is implemented. Unexpected disconnect clears the session; reconnect is explicit in this draft. Drawings remain in memory until exported. Name length is measured in UTF-8 bytes to match firmware storage. Clock accuracy depends on the computer; confirm the selected time zone before syncing.

## Optional GitHub Pages deployment

The manual `.github/workflows/web-studio-pages.yml` workflow publishes only `web-controller/`. The workflow must first exist on the repository's default branch for manual dispatch to be offered. In repository Settings → Pages choose GitHub Actions, then run **Publish HAPPY JARZ Web Studio** with the desired source branch selected. This setup has not been enabled or run as part of the draft. GitHub may require repository settings access.

Expected project URL if Pages is enabled for this repository:
`https://etsimulocto.github.io/HappyJarzESP32S3MiniPublic/`

All assets use relative paths, so project subdirectory hosting works. No custom domain is necessary. The website does not itself authorize or launch on USB insertion.


## Board #3: Windows candidate build and local browser test (0.18.13)

**Historical 0.18.13 candidate procedure (not today's release).** This procedure serves a
candidate from `http://localhost:8000` only. It never publishes an update to
customers, pushes firmware to GitHub, or silently flashes Board #3.

1. On Windows install Git, Arduino CLI, Python 3 and the ESP32 core. Plug
   Board #3 into the PC and verify its COM port. If the Arduino core/U8g2 are missing, install them explicitly (or run the builder with -InstallDependencies). The official site should
   report `V1`, `0.18.12`. Close the website serial session afterwards.
2. Open PowerShell in a checkout of
   `feature/mini-art-gallery-animation`, then run:

   ```powershell
   git pull --ff-only
   py -3 -m pip install esptool
   powershell -ExecutionPolicy Bypass -File .\tools\build_release_windows.ps1
   ```

   The builder reuses the installed Arduino toolchain and compiles with `FlashSize=4M,PartitionScheme=default,PSRAM=disabled,CDCOnBoot=cdc`, verifies that
   firmware source/version match, inspects the **compiled** partition table,
   checks for a 0x10000 main application region of length 0x140000 and
   separate NVS/SPIFFS, then stages the binary and release-entry SHA-256 in
   `release-staging/candidate/`. A candidate is **not** a tested release.
3. Back up any irreplaceable art and settings and record the current profile.
   First-time flash testing can fail and may require bootloader recovery.
4. To test in Edge against the **same physical Board #3** only, close all
   other USB controllers, then substitute its actual COM port:

   ```powershell
   powershell -ExecutionPolicy Bypass -File .\tools\prepare_local_browser_test.ps1 -Port COM4 -IUnderstandFlashRisk -IHaveBackedUpData
   ```

   This reads the board's partition table from flash offset 0x8000 without
   writing to it, checks that its partitions exactly match the candidate
   build, creates a locally served copy of the website with a **local-only**
   update manifest, and runs a localhost HTTP server. If this verification
   fails, **do not install or bypass the check**.
5. Open `http://localhost:8000` in Edge on the PC, connect the jar, select
   UPDATES, and use the new explicit confirmation to install the staged
   **test candidate**. Keep USB connected throughout. Afterward reconnect
   and check `fw=0.18.13`, OLED clock, touch pads, seven arcade games,
   saved owner/profile, Wi-Fi and saved art.

`localhost` is an allowed secure origin for Web Serial, but this relies on
browser support and a physical device test. The website flasher has not
yet been proven on Board #3. An interrupted write can require Arduino
CLI/esptool recovery. **Do not publish `release-entry.json` to the public
`releases.json` until this test and data-preservation checks succeed.**

Notes: the candidate source does not include Board #4's uncommitted 40K
touch experiment; that remains isolated on the Pi. The website has no
firmware rollback or automatic recovery. Only the app partition is written;
that is necessary but not sufficient to guarantee preservation unless the
existing board's actual NVS/SPIFFS partition map is confirmed.

## Website firmware update center (guarded installer, November 2026)

The UPDATES tab now implements a gated Web Serial ESP32-S3 application flasher using
the pinned `esptool-js@0.7.1` browser bundle. Firmware 0.18.14 is now published in `releases.json`. Installation is disabled when no newer compatible release exists. **Never release from branch HEAD alone:** publish only the verified binary; Board #4 has independent local touch calibration work.

A production release entry has this shape (published 0.18.14 metadata):

```json
{
  "schema": 1,
  "releases": [{
    "channel": "stable",
    "model": "happyjarz-supermini-simple-4mb",
    "hw": "V1",
    "chip": "ESP32-S3",
    "layout": "arduino-4mb-default-app-0x10000-0x140000",
    "flashSize": 4194304,
    "version": "0.18.14",
    "images": [{
      "url": "./releases/happyjarz-0.18.14-app.bin",
      "address": 65536,
      "size": 1118144,
      "sha256": "a92108a7acd697e761c9859d0a8b77f5a6eef2526fd19ff872b10fa251ddb712"
    }]
  }]
}
```

Replace the example size with the **actual** build binary size and SHA-256
(and publish that exact `.bin` under the site origin). No placeholder is accepted.
The updater rejects entries with missing / incompatible metadata, other flash offsets,
more than one image, or binaries over the confirmed 0x140000 app budget. It checks
the binary SHA-256 before releasing the existing browser serial connection. It uses
the already-selected same port, detects ESP32-S3 in ROM mode, flashes **only**
the application image at 0x10000, never sets eraseAll, and requests reset. Users
reconnect using the site's Connect button; firmware version is then checked.

**Remaining customer-wide release/flash validation gates (0.18.14 already published for this tested V1 target):** confirm exact production Arduino partition
table and app offset against Board #4, compile and test on a sacrificial/test board,
confirm native USB ROM bootloader entry works with esptool-js, verify browser
downloads and the printed SHA-256, preserve NVS and SPIFFS including owner profile
and art, and test interrupted-flash recovery. Firmware version from HJ identity
plus hw=V1 is not a cryptographic board attestation; do not extend this release to other board layouts or treat version strings as secure board attestation. Normal browser usage uses `Disconnect` to close the session first.
The installer currently requires an existing *connected and running* jar; it is
not a recovery flasher for an already bricked device.

For a newer release offered to a compatible board, use the website in a secure context in
desktop Chrome/Edge, connect your jar, Check for updates, read the warning,
check the confirmation, and install. Do not use a PC controller and the
website against the same USB port simultaneously.

## Firmware release/update design

`releases.json` now contains the tested 0.18.14 installable artifact (for older, compatible V1 units). Do not put development HEAD into an automatic updater. A future tested entry needs a unique hardware model (`happyjarz-supermini-simple-4mb`), version, protocol compatibility, exact partition layout/build settings, artifact URLs, sizes and SHA-256 hashes. HTTPS + hashes detect transfer errors; hashes in the same catalog do not establish independent authenticity. Unattended updates need an authenticated release policy.

Proposed pipeline:

1. Compile a tagged candidate with the established ESP32-S3 / CDCOnBoot=cdc settings and pinned toolchain/library versions. Produce bootloader, partition and application artifacts with build metadata.
2. Test on the actual 4 MB SuperMini: GPIO/RMT timing, OLED, touch, profiles, birthday modes and artwork retention. Only then promote to stable.
3. Publish immutable binaries and manifest together. Serve browser downloads from the same HTTPS origin where practical; test cross-origin downloads/redirects if using release assets. Never begin erasing until all needed bytes and hashes have been validated.
4. Read jar identity before resetting; verify exact model/flash/partition compatibility. Current `hw=V1` and chip-family detection alone cannot identify all S3 variants. Add explicit model/protocol/layout identity before unattended flashing.
5. Coordinate exclusive port ownership with desktop app/watcher, then flash through ESP Web Tools or esptool-js. Preserve Preferences/NVS and SPIFFS. Inspect merged-image padding and actual written ranges; a “do not erase all” option alone is insufficient to prove data retention.
6. Reconnect and verify reported version plus basic health. Report success only then. Offer a recovery path for interrupted writes.

No Improv Serial support is assumed: the jar currently uses HJ|IDENTITY. ESP Web Tools' default detection/new-install behavior must be adapted so an unknown Improv device is not silently treated as an erase-and-install case.

USB ROM flashing overwrites the application and does not provide automatic rollback. ESP-IDF dual-slot application updates and rollback are a separate future firmware design, requiring partition-space analysis and bootloader support. Retaining a previous binary provides a retry option, not automatic rollback.

An optional Windows/Pi helper can later launch this site on insertion. It must release its probing port before the browser takes ownership, and must not race a firmware installer. The existing Python/Tkinter UI is not executable as a GitHub Pages website; this UI is a new frontend using the existing protocol.

## Initial PC hardware results — 2026-10-10

The Architect tested the controller on a Windows PC in Edge with the real SuperMini. The supplied serial log identifies firmware **0.18.4** and confirms successful USB handshake, profile/status/capability reads, four light-color commands with ACKs, live OLED/LED frames, six-pad input readings, power telemetry and explicit stream shutdown/disconnection. The Architect reported that most controls worked and subsequently confirmed clock synchronization after a delay. The earlier `synced=0` snapshot is not the final clock result.

Many identity replies appeared during initial connection; investigate handshake retries/buffering before calling this a diagnosed fault. A later **No jar selected** message followed a chooser cancellation and does not establish a board failure. Chromium on the Pi was reported to lag; PC testing succeeded, but Pi/background performance still needs measurement.

These are initial field observations, not certification of every tab. Art transfers/persistence, marquee replacement, alarms/timers, profile persistence, birthday behaviors, repeated reconnects and Service physical observations still need recorded checks.

## Validation and remaining work

Fourteen Node tests passed across protocol/diagnostics, calendar/raster tools, panel integration and BloomSaver checks. Panel integration executes the actual modules against a minimal DOM and simulated serial firmware. The logic tests cover birthday/name validation, two-frame packing, chunk limits, import validation, protocol/version parsing, diagnostic redaction/reload, bounded retention, blocked/full storage and support report contents, RGB validation, the 50% brightness ceiling, capability filtering, standard gamepad mapping and theme contrast. JavaScript syntax checks passed. Browser smoke tests with simulated serial hardware have been extended for Base/Live readouts, capability-gated keys, White/Off actions, opt-in keyboard input, ACK sequencing, rejected storage, disconnect cleanup and UI operation, but could not be executed in the development environment because the Chromium download failed. The module integration test passed for nine-tab mounting, shared USB connect/reconnect, capability-gated keys, typing/opt-in keyboard behavior, White/Off/pattern/brightness, Wi-Fi credential handling, alarm/timer, display/marquee, clock/profile saving, complete art transfer, Service readout/RGB/touch/ping routines, redacted report export and device-error recovery. Published Monday song/art loading and absence of visitor media editor controls also passed. Old browser media customizations are ignored; validation still rejects unsafe links. A separate VM test executed the copied BloomSaver engine with all 22 controls and verified pause/resume, editing messages, foreign-message rejection and state saving; media validation checks cover Suno links and image sources. These simulated environments do not render the page. Automated browser rendering remains unverified. Initial manual PC USB results are recorded above; full tab-by-tab hardware verification remains outstanding. These tests do not prove real USB reset behavior, drivers, touch timing, clock behavior, screen byte layout or flash preservation. Further hardware trials on Pi and Windows are required. The Mini app remains the established controller.

To run the logic and module integration tests from the repository root:

```bash
node --test tests/web-studio.test.mjs tests/web-panels.test.mjs tests/bloomsaver.test.mjs
```

For the browser test, install Playwright and its Chromium browser in `tests/`, then run `node tests/browser-smoke.mjs` from the repository root. The browser test simulates the serial device and never accesses a real jar.

Sources: [ESP Web Tools](https://esphome.github.io/esp-web-tools/), [Espressif esptool-js](https://github.com/espressif/esptool-js), [WLED installer](https://install.wled.me/), [Web Serial](https://developer.chrome.com/docs/capabilities/serial), [ESP32-S3 OTA](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/system/ota.html).

## International and mobile limitations

All diagnostic timestamps use UTC. Reports also record browser locale, browser timezone and offset, and the jar's reported timezone/local clock when available. Firmware 0.18.4 maps only America/New_York, America/Chicago, America/Denver, America/Los_Angeles and UTC; other values silently fall back to Central. The first web draft used POSIX values incompatible with that mapping. Draft 0.2 fixes the controls to use supported IANA identifiers and removes the unsupported UK option. More country time zones need a board firmware extension and daylight-saving tests before they can be offered.

The layout adapts to phone/tablet widths, including Service cards and wrapped tabs. This does not imply universal phone/tablet USB support. USB permission, browser support and secure-context checks still apply. Wi-Fi configuration currently requires a nonempty password because the firmware's trimmed command parser cannot reliably clear a password with an empty SET command. No board firmware changed in this web update.

## Wi-Fi + Clock — draft 0.12

The separate `tabs/wifi-clock.js` module now carries the Pi setup controls: scan the jar’s visible 2.4 GHz networks, select an SSID, save credentials and connect, retry a saved network, read connection/IP/signal status, and confirm clearing saved Wi-Fi. Scan results are rendered as text and network/password fields remain redacted in diagnostics. Password visibility is optional and the input is cleared after saving and on disconnect.

Clock controls include a shared Profile/Clock time-zone selection, saved time zone, USB host time without internet, an internet-time request and explicit clock refresh. Internet requests and Wi-Fi connection ACKs indicate requests accepted, not successful synchronization or connection; refresh afterward to confirm. Firmware still supports only four US zones and UTC. Password-free networks and captive-portal login are unsupported by this setup.

Alarm controls validate HH:MM, apply enable/disable and save settings. Auto-off controls start/stop a 1–10080 minute timer with separate remaining-time readout. All readouts show last board readings; they are not a locally fabricated countdown. Disconnect clears readouts and scan results. No firmware flash is needed.

Validation: all 12 Node tests pass with expanded actual-module simulated USB coverage for scanning/selecting, time-zone and host/internet sync commands, invalid alarms, timer stop, credential clearing and redacted reports. Browser layout and physical Wi-Fi operation still need testing on a real jar.

## Display + Input — draft 0.12

The separate `tabs/display-input.js` module now includes a 128×64 OLED mirror, manual screen/display refresh, the eight current saver modes (SAYINGS, SPIRAL, TRIPPY, PARTICLES, BLOOM, BREATHE, GLITTER, ART), start/exit, previous/next, reseed and speed controls. Saver telemetry selects the current mode and shows its activity, speed, idle delay and particle settings. Screen brightness remains fixed at 100% by firmware; obsolete brightness/idle sliders are not exposed. The web page background is a separate personal BloomSaver, not the jar OLED.

Touch controls include Jar/Menu/Game mode, six raw pad readings and recalibration. The test delegates to Service so its response, timing and observation workflow remain in downloadable support reports. Keep all fingers off the pads during calibration. Raw touch values do not imply a qualified press.

The marquee editor loads all 50 slots, preserving blank slots, and can replace/save the list, edit one slot, choose BUILTIN/CUSTOM/MIXED, or clear custom messages with confirmation. Firmware persists custom slots and source directly. Replacement validates the whole draft before clearing old messages; an interrupted write can leave a partial list on the jar. Load to check before retrying. Text must fit 96 UTF-8 bytes per slot without protocol separators. Empty CUSTOM lists fall back to built-in sayings. Diagnostic exports redact message text. Disconnect pauses readouts without discarding the local editor draft.

Validation: 12 Node tests pass, including expanded simulated USB coverage for complete marquee responses, slot gaps, save/clear, invalid messages, OLED frame rendering, mode commands and Service-routed calibration. Physical OLED/touch behavior and browser layout still need checking on the jar. No firmware flash required.

## Profile — draft 0.12

The separate `tabs/profile.js` module now includes jar identity/firmware, saved name/birthday, save-only and save-plus-host-clock options, confirmed profile loading/clearing, optional birthday removal and name-byte validation. Loading asks before replacing unsaved edits; unsolicited profile telemetry updates the saved readout without overwriting a dirty editor. Disconnect clears the editor so one jar’s private profile is not carried into the next connection. Profiles persist on the jar; the web app does not maintain the Pi app’s separate local profile database.

Birthday countdowns use the saved profile and the last `GET TIME STATUS` local date. They do not use the computer’s timezone or an unsaved birthday. Refresh the date/countdown for a new board reading. An unset/unsynced clock suppresses the countdown. Calendar arithmetic handles year rollover and observes February 29 on March 1 in non-leap years, matching the board birthday celebration rule. Existing firmware provides startup/wake countdowns and all-day birthday visuals; this change does not alter them.

If profile saving succeeds but subsequent clock sync fails, the UI reports that distinction. Time-zone options remain the four US firmware regions and UTC. Profile name/birthday are redacted from logs and support exports.

Validation: all 13 Node tests pass, including calendar boundaries/leap days, profile save/load/clear, dirty-edit protection, removing a birthday and saving without clock writes. Physical jar and browser layout testing remain outstanding. No firmware change or flash.

## Art stamps + shapes — draft 0.12

Art now includes 24 original monochrome pixel stamps: heart, star, smile, wink, sunglasses, ghost, cat, dog, paw, alien, robot, skull, flower, mushroom, butterfly, moon, sun, lightning, music note, cake, gift, rocket, jar and sparkles. Browser emoji label the buttons; the jar receives deterministic pixel art, not font-rendered emoji. Choose a stamp, set scale 1–4 and tap/drag to place it. Edge placement clips to the 128×64 drawing.

Drag line, rectangle, circle/oval, triangle, diamond or star tools. Fill applies to closed shapes; Eraser removes pixels with the chosen tool. Freehand has a 1–6 pixel brush and connected stroke segments. Shape/stamp drags preview from the original frame and commit on release; cancelled gestures restore it. Undo retains the last 40 edits per frame, including clear/copy/import. These tools use the existing two-frame export/upload format and do not change firmware. Download drawings before leaving the page.

Validation: all 14 Node tests pass, including stamp masks/scales/erase/clipping, shape fill, actual pointer-handler placement/undo and upload integration. Physical display and browser layout testing remain outstanding.

## Curated stamp library — draft 0.12

Art now offers 96 distinct original pixel masks with searchable labels and six categories: Faces, Animals, Nature, Food, Things, Symbols. The tray uses compact fixed-height rows and scrolls after five rows rather than stretching its cells or leaving a large blank area. The collection is stored in the browser app, not the ESP32; adding stamps does not increase firmware or drawing size. Finished pictures still use the same 128×64 two-frame data. This is a curated emoji-style selection, not the full Unicode emoji catalogue.

All 14 tests pass, with all 96 stamps tested for placement, erase and edge clipping, unique masks/IDs, category/search filtering and existing upload integration. Browser appearance still needs user verification. No firmware flash.

The stamp library also includes all 12 zodiac glyphs in a dedicated **Zodiac** category: Aries, Taurus, Gemini, Cancer, Leo, Virgo, Libra, Scorpio, Sagittarius, Capricorn, Aquarius and Pisces. Total: 108 stamps. Search by sign name, then place/scale like any other stamp. Zodiac is an art category; it does not derive or change the saved owner profile.

## Service — draft 0.12

Service now includes the Pi app’s fuel-gauge panel: voltage-estimated battery percentage, voltage, USB-data presence, charger indication, ADC millivolts and sensor state, with reading timestamp. Unknown/missing fields stay unknown; charging/full remains a hardware LED indication (`HW_ONLY`) rather than a software charge measurement. Optional five-second power refresh and one-second input refresh operate only while Service is visible, pause during transfers/tests/other busy operations, avoid accumulating pending reads and stop on disconnect.

Added the Pi’s separate `TEST INPUT` recalibration routine, individual Jar Status / Wi-Fi / Clock / Display / Power / Capabilities / Touch / Screensaver reads, and Save to Jar. Both input and touch tests recalibrate baselines and require fingers off all pads. Tests record firmware responses and elapsed times in the support report; routine PASS does not certify physical hardware, so users still record observations. Existing logs, error filters, readable downloads and redacted support reports remain local. No automatic sending or remote repair is implied.

Validation: all 14 Node tests pass, including individual reads, save, Input test/report entries, fuel-gauge field mapping and optional polling wiring. Physical board and browser appearance checks remain outstanding. No firmware flash.

## Nine-pad firmware support

Firmware 0.18.5 adds L/R/Start on GPIO11/12/13. Lights + Control virtual keys and raw tiles follow advertised capabilities; Display + Input now includes all nine readings and supported calibration fields. Older firmware leaves extras disabled/N/A. X/Y/Select remain unsupported. Existing firmware game/menu bindings are unchanged. All 14 Node tests pass, including capability upgrades/downgrades and extra commands/telemetry. A firmware flash is required to enable the new physical controls; see [firmware wiring/build instructions](../firmware/README.md). Physical pad and full ESP32 build checks remain pending.

## Nine-button controller test — draft 0.13

The nine-pad SuperMini mapping is Up GPIO4, Down GPIO5, Left GPIO9,
Right GPIO10, A GPIO1, B GPIO2, L GPIO11, R GPIO12, Start GPIO13.
Display + Input labels each pad with its GPIO and flashes the tile on
board input events. Lights + Control labels shoulders L − / R + and
shows their brightness role in button tooltips. Firmware 0.18.6+ applies
one saved 5% brightness step per qualified press, within 0–50%.
The web slider follows the board's STATUS response. Start is an independent
event; Happy Jarz/BloomPetz/DND mode selection remains planned.

On October 11 the Architect confirmed that L/R/Start did not detect touch
because their GPIO header pins had not been soldered to the board.
Soldering those joints restored all three physical controls on firmware
0.18.7. The startup warmup change alone had not solved the fault.
Different idle counts alone do not diagnose this failure. Before reflashing,
check the soldered header connection and continuity with power disconnected.
Calibration PASS means the routine completed, not proof of physical touch.
Host commands and board presses share the event path; events without a source
field are labelled source not specified, not certified physical touches.

For a new test controller, connect in Edge/Chrome with other USB controllers
closed. Check all nine live values and press events, then L/R brightness
and Start. Existing six-pad firmware still disables unsupported extras.
After updating the site, use Ctrl+F5 to refresh cached modules.
No board flash is required for this web update.

Validation: all 14 Node tests passed, including GPIO labels, L/R button roles,
L/R/Start event feedback, host-source labels, board-reported brightness updates,
and six-pad capability compatibility. Tests simulate USB; the new controller
still needs a physical PC trial. No additional firmware changes were made.
