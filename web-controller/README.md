# HAPPY JARZ Web Studio — draft 0.4

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

Ten Node tests passed: nine protocol/diagnostics/light-control tests plus an integration test executing the actual panel modules against a minimal DOM and simulated serial firmware. The logic tests cover birthday/name validation, two-frame packing, chunk limits, import validation, protocol/version parsing, diagnostic redaction/reload, bounded retention, blocked/full storage and support report contents, RGB validation, the 50% brightness ceiling, capability filtering, standard gamepad mapping and theme contrast. JavaScript syntax checks passed. Browser smoke tests with simulated serial hardware have been extended for Base/Live readouts, capability-gated keys, White/Off actions, opt-in keyboard input, ACK sequencing, rejected storage, disconnect cleanup and UI operation, but could not be executed in the development environment because the Chromium download failed. The module integration test passed for nine-tab mounting, shared USB connect/reconnect, capability-gated keys, typing/opt-in keyboard behavior, White/Off/pattern/brightness, Wi-Fi credential handling, alarm/timer, display/marquee, clock/profile saving, complete art transfer, Service readout/RGB/touch/ping routines, redacted report export and device-error recovery. This minimal DOM does not render the page. Browser rendering and real hardware behavior remain unverified. These tests do not prove real USB reset behavior, drivers, touch timing, clock behavior, screen byte layout or flash preservation. Hardware trials on Pi and Windows are required. The Mini app remains the established controller.

To run the logic and module integration tests from the repository root:

```bash
node --test tests/web-studio.test.mjs tests/web-panels.test.mjs
```

For the browser test, install Playwright and its Chromium browser in `tests/`, then run `node tests/browser-smoke.mjs` from the repository root. The browser test simulates the serial device and never accesses a real jar.

Sources: [ESP Web Tools](https://esphome.github.io/esp-web-tools/), [Espressif esptool-js](https://github.com/espressif/esptool-js), [WLED installer](https://install.wled.me/), [Web Serial](https://developer.chrome.com/docs/capabilities/serial), [ESP32-S3 OTA](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/system/ota.html).

## International and mobile limitations

All diagnostic timestamps use UTC. Reports also record browser locale, browser timezone and offset, and the jar's reported timezone/local clock when available. Firmware 0.18.4 maps only America/New_York, America/Chicago, America/Denver, America/Los_Angeles and UTC; other values silently fall back to Central. The first web draft used POSIX values incompatible with that mapping. Draft 0.2 fixes the controls to use supported IANA identifiers and removes the unsupported UK option. More country time zones need a board firmware extension and daylight-saving tests before they can be offered.

The layout adapts to phone/tablet widths, including Service cards and wrapped tabs. This does not imply universal phone/tablet USB support. USB permission, browser support and secure-context checks still apply. Wi-Fi configuration currently requires a nonempty password because the firmware's trimmed command parser cannot reliably clear a password with an empty SET command. No board firmware changed in this web update.
