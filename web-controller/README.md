# HAPPY JARZ Web Studio — draft 0.11

A static USB browser controller for the existing SuperMini SIMPLE protocol. No backend, account, Python installation, or paid domain required for website users. This is an initial implementation, not a hardware-verified replacement for the Mini app.

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
- Release catalog check with explicit “not published” state. Firmware installation remains disabled. No firmware or partition changes accompany this draft.

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

## Try locally

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

## Firmware release/update design

`releases.json` deliberately contains no installable releases. Do not put development HEAD into an automatic updater. A future tested entry needs a unique hardware model (`happyjarz-supermini-simple-4mb`), version, protocol compatibility, exact partition layout/build settings, artifact URLs, sizes and SHA-256 hashes. HTTPS + hashes detect transfer errors; hashes in the same catalog do not establish independent authenticity. Unattended updates need an authenticated release policy.

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

## Validation and remaining work

Twelve Node tests passed: nine protocol/diagnostics/light-control tests plus an integration test executing the actual panel modules against a minimal DOM and simulated serial firmware. The logic tests cover birthday/name validation, two-frame packing, chunk limits, import validation, protocol/version parsing, diagnostic redaction/reload, bounded retention, blocked/full storage and support report contents, RGB validation, the 50% brightness ceiling, capability filtering, standard gamepad mapping and theme contrast. JavaScript syntax checks passed. Browser smoke tests with simulated serial hardware have been extended for Base/Live readouts, capability-gated keys, White/Off actions, opt-in keyboard input, ACK sequencing, rejected storage, disconnect cleanup and UI operation, but could not be executed in the development environment because the Chromium download failed. The module integration test passed for nine-tab mounting, shared USB connect/reconnect, capability-gated keys, typing/opt-in keyboard behavior, White/Off/pattern/brightness, Wi-Fi credential handling, alarm/timer, display/marquee, clock/profile saving, complete art transfer, Service readout/RGB/touch/ping routines, redacted report export and device-error recovery. Published Monday song/art loading and absence of visitor media editor controls also passed. Old browser media customizations are ignored; validation still rejects unsafe links. A separate VM test executed the copied BloomSaver engine with all 22 controls and verified pause/resume, editing messages, foreign-message rejection and state saving; media validation checks cover Suno links and image sources. These simulated environments do not render the page. Browser rendering and real hardware behavior remain unverified. These tests do not prove real USB reset behavior, drivers, touch timing, clock behavior, screen byte layout or flash preservation. Hardware trials on Pi and Windows are required. The Mini app remains the established controller.

To run the logic and module integration tests from the repository root:

```bash
node --test tests/web-studio.test.mjs tests/web-panels.test.mjs tests/bloomsaver.test.mjs
```

For the browser test, install Playwright and its Chromium browser in `tests/`, then run `node tests/browser-smoke.mjs` from the repository root. The browser test simulates the serial device and never accesses a real jar.

Sources: [ESP Web Tools](https://esphome.github.io/esp-web-tools/), [Espressif esptool-js](https://github.com/espressif/esptool-js), [WLED installer](https://install.wled.me/), [Web Serial](https://developer.chrome.com/docs/capabilities/serial), [ESP32-S3 OTA](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/system/ota.html).

## International and mobile limitations

All diagnostic timestamps use UTC. Reports also record browser locale, browser timezone and offset, and the jar's reported timezone/local clock when available. Firmware 0.18.4 maps only America/New_York, America/Chicago, America/Denver, America/Los_Angeles and UTC; other values silently fall back to Central. The first web draft used POSIX values incompatible with that mapping. Draft 0.2 fixes the controls to use supported IANA identifiers and removes the unsupported UK option. More country time zones need a board firmware extension and daylight-saving tests before they can be offered.

The layout adapts to phone/tablet widths, including Service cards and wrapped tabs. This does not imply universal phone/tablet USB support. USB permission, browser support and secure-context checks still apply. Wi-Fi configuration currently requires a nonempty password because the firmware's trimmed command parser cannot reliably clear a password with an empty SET command. No board firmware changed in this web update.

## Wi-Fi + Clock — draft 0.11

The separate `tabs/wifi-clock.js` module now carries the Pi setup controls: scan the jar’s visible 2.4 GHz networks, select an SSID, save credentials and connect, retry a saved network, read connection/IP/signal status, and confirm clearing saved Wi-Fi. Scan results are rendered as text and network/password fields remain redacted in diagnostics. Password visibility is optional and the input is cleared after saving and on disconnect.

Clock controls include a shared Profile/Clock time-zone selection, saved time zone, USB host time without internet, an internet-time request and explicit clock refresh. Internet requests and Wi-Fi connection ACKs indicate requests accepted, not successful synchronization or connection; refresh afterward to confirm. Firmware still supports only four US zones and UTC. Password-free networks and captive-portal login are unsupported by this setup.

Alarm controls validate HH:MM, apply enable/disable and save settings. Auto-off controls start/stop a 1–10080 minute timer with separate remaining-time readout. All readouts show last board readings; they are not a locally fabricated countdown. Disconnect clears readouts and scan results. No firmware flash is needed.

Validation: all 12 Node tests pass with expanded actual-module simulated USB coverage for scanning/selecting, time-zone and host/internet sync commands, invalid alarms, timer stop, credential clearing and redacted reports. Browser layout and physical Wi-Fi operation still need testing on a real jar.

## Display + Input — draft 0.11

The separate `tabs/display-input.js` module now includes a 128×64 OLED mirror, manual screen/display refresh, the eight current saver modes (SAYINGS, SPIRAL, TRIPPY, PARTICLES, BLOOM, BREATHE, GLITTER, ART), start/exit, previous/next, reseed and speed controls. Saver telemetry selects the current mode and shows its activity, speed, idle delay and particle settings. Screen brightness remains fixed at 100% by firmware; obsolete brightness/idle sliders are not exposed. The web page background is a separate personal BloomSaver, not the jar OLED.

Touch controls include Jar/Menu/Game mode, six raw pad readings and recalibration. The test delegates to Service so its response, timing and observation workflow remain in downloadable support reports. Keep all fingers off the pads during calibration. Raw touch values do not imply a qualified press.

The marquee editor loads all 50 slots, preserving blank slots, and can replace/save the list, edit one slot, choose BUILTIN/CUSTOM/MIXED, or clear custom messages with confirmation. Firmware persists custom slots and source directly. Replacement validates the whole draft before clearing old messages; an interrupted write can leave a partial list on the jar. Load to check before retrying. Text must fit 96 UTF-8 bytes per slot without protocol separators. Empty CUSTOM lists fall back to built-in sayings. Diagnostic exports redact message text. Disconnect pauses readouts without discarding the local editor draft.

Validation: 12 Node tests pass, including expanded simulated USB coverage for complete marquee responses, slot gaps, save/clear, invalid messages, OLED frame rendering, mode commands and Service-routed calibration. Physical OLED/touch behavior and browser layout still need checking on the jar. No firmware flash required.

## Profile — draft 0.11

The separate `tabs/profile.js` module now includes jar identity/firmware, saved name/birthday, save-only and save-plus-host-clock options, confirmed profile loading/clearing, optional birthday removal and name-byte validation. Loading asks before replacing unsaved edits; unsolicited profile telemetry updates the saved readout without overwriting a dirty editor. Disconnect clears the editor so one jar’s private profile is not carried into the next connection. Profiles persist on the jar; the web app does not maintain the Pi app’s separate local profile database.

Birthday countdowns use the saved profile and the last `GET TIME STATUS` local date. They do not use the computer’s timezone or an unsaved birthday. Refresh the date/countdown for a new board reading. An unset/unsynced clock suppresses the countdown. Calendar arithmetic handles year rollover and observes February 29 on March 1 in non-leap years, matching the board birthday celebration rule. Existing firmware provides startup/wake countdowns and all-day birthday visuals; this change does not alter them.

If profile saving succeeds but subsequent clock sync fails, the UI reports that distinction. Time-zone options remain the four US firmware regions and UTC. Profile name/birthday are redacted from logs and support exports.

Validation: all 13 Node tests pass, including calendar boundaries/leap days, profile save/load/clear, dirty-edit protection, removing a birthday and saving without clock writes. Physical jar and browser layout testing remain outstanding. No firmware change or flash.

## Art stamps + shapes — draft 0.11

Art now includes 24 original monochrome pixel stamps: heart, star, smile, wink, sunglasses, ghost, cat, dog, paw, alien, robot, skull, flower, mushroom, butterfly, moon, sun, lightning, music note, cake, gift, rocket, jar and sparkles. Browser emoji label the buttons; the jar receives deterministic pixel art, not font-rendered emoji. Choose a stamp, set scale 1–4 and tap/drag to place it. Edge placement clips to the 128×64 drawing.

Drag line, rectangle, circle/oval, triangle, diamond or star tools. Fill applies to closed shapes; Eraser removes pixels with the chosen tool. Freehand has a 1–6 pixel brush and connected stroke segments. Shape/stamp drags preview from the original frame and commit on release; cancelled gestures restore it. Undo retains the last 40 edits per frame, including clear/copy/import. These tools use the existing two-frame export/upload format and do not change firmware. Download drawings before leaving the page.

Validation: all 14 Node tests pass, including stamp masks/scales/erase/clipping, shape fill, actual pointer-handler placement/undo and upload integration. Physical display and browser layout testing remain outstanding.

## Curated stamp library — draft 0.11

Art now offers 96 distinct original pixel masks with searchable labels and six categories: Faces, Animals, Nature, Food, Things, Symbols. The tray uses compact fixed-height rows and scrolls after five rows rather than stretching its cells or leaving a large blank area. The collection is stored in the browser app, not the ESP32; adding stamps does not increase firmware or drawing size. Finished pictures still use the same 128×64 two-frame data. This is a curated emoji-style selection, not the full Unicode emoji catalogue.

All 14 tests pass, with all 96 stamps tested for placement, erase and edge clipping, unique masks/IDs, category/search filtering and existing upload integration. Browser appearance still needs user verification. No firmware flash.
