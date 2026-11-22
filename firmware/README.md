# HAPPY JARZ Mini firmware

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

| Item | Current value |
| --- | --- |
| Firmware | **0.18.14** |
| Target | ESP32-S3 SuperMini, SIMPLE profile, 4 MB flash |
| Sketch | `HappyJarzMini/HappyJarzMini.ino` |
| Additional sources | `happyjarz_arcade.cpp/.h`, `happyjarz_art.cpp/.h`, `happyjarz_led_types.h` |
| FQBN | `esp32:esp32:esp32s3:CDCOnBoot=cdc` |
| Pi-tested dependencies | ESP32 core 3.3.12, U8g2 2.36.19 |
| Serial / recovery upload speed | 115200 baud |

This repository contains the complete Mini sketch. Build it directly; BloomCircuit's compatibility staging and patch chain target its older source tree.

## Preserve the proven hardware layer

The four APA106 lamps use GPIO7, RGB byte order, the ESP32 HAL RMT driver at 10 MHz, 4/14 ticks for zero, 14/4 ticks for one and approximately 100 µs latch low. OLED uses 128×64 U8g2 on I²C address 0x3C, SDA GPIO8 and SCL GPIO6.

Touch controls: UP GPIO4, DOWN GPIO5, LEFT GPIO9, RIGHT GPIO10, A GPIO1, B GPIO2, L GPIO11, R GPIO12 and Start GPIO13. The 3.3 V accessory switch powers the OLED/lamps; OLED presence is the existing rail detector. On resume the firmware reinitializes the OLED and rebinds GPIO7 to RMT before restoring lamp output.

## Board #4 long-wire capacitive-touch tuning — November 20, 2026

Board #4's long copper touch leads were reading approximately **50,000 untouched** and **62,000 when touched**. With the original firmware logic (`threshold = baseline + 20%`), a 50,000 baseline required about **60,000** to qualify as a press, leaving little margin for shorter/faster touches.

The Architect tested a **local, uncommitted 0.18.12 sensitivity patch** on the Raspberry Pi production worktree (`~/HappyJarzESP32S3MiniPublic-art-test`) and reported that buttons now accept **faster successive presses without the previous noticeable delay**. The patch changes only threshold selection:

```cpp
static uint32_t hjTouchThreshold(uint32_t baseline) {
  if (baseline >= 45000U) return baseline + 2000U;
  return baseline + (baseline / 5U);
}
```

Use `pad.threshold = hjTouchThreshold(pad.baseline);` in **both** `calibrateInputs()` and the idle baseline update in `updateInputState()`. At a baseline of 50,000, this yields a **52,000 trigger**. Lower-baseline pads retain the original +20% behavior. **The 60 ms touch qualification, press/release semantics, drift tracking, pin map, OLED and APA106 RMT driver are unchanged**; this is a sensitivity improvement, not a removal of debounce or a new auto-repeat feature.

**Source-of-truth warning:** This documents the tested **local** modification and user observation; updating this README does **not** apply the patch to the GitHub firmware source. Before flashing another board, inspect `HappyJarzMini.ino` for `hjTouchThreshold`, preserve a backup, compile successfully and test the actual touch pads. No separate timing/latency benchmark was recorded. Calibrate hands-off; verify each button's idle, touched and threshold telemetry, rapid individual taps, and absence of ghost presses. If idle readings shift or ghost presses appear, retune based on the installed wiring rather than assuming 52,000 is universal.

## L/R brightness controls — firmware 0.18.6

L decreases and R increases lamp brightness by 5 percentage points per press, clamped to 0–50%. The handler runs before page/game-specific returns and shares physical/host qualified edges. It preserves the current pattern and lamp colors, saves actual changes, updates the OLED and emits STATUS for the app. Holding a pad does not auto-repeat; simultaneous L/R press edges cancel. As with other pads, the first physical press while asleep wakes the device.

Raw INPUT telemetry is now sent before any page-specific return, retaining the existing 100 ms arcade / 200 ms normal cadence. Start is still an independent event; the three-mode selector is not yet implemented.

Host tests exercise the actual brightness handler for steps, limits, held presses, simultaneous shoulders, Start isolation, saved changes and STATUS output. Full Arduino compilation and real pad tests remain to be performed on the Pi.

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

Host verification: 14 Node tests pass, including new/old capability transitions and L/R/Start commands/readouts. Two Python tests execute the extracted firmware touch routines in C++ and the actual Pi capability/readout handlers. These do not compile the full ESP32 sketch or validate physical touch sensitivity. Run from the repository root:

```bash
python3 tests/test_touch_inputs.py
```

The firmware 0.18.5 full Arduino compile/upload and physical nine-pad test remain pending. After upload confirm fw=0.18.5, recalibrate with fingers off, check each new pad's baseline/threshold/raw values, and touch/release each pad to verify an isolated event without ghost presses. Repeat while lights and Wi-Fi are active.

## On-board PROFILE

From HOME, **A → PROFILE → A**.

| Editor | Controls |
| --- | --- |
| Name | UP/DOWN change character; LEFT/RIGHT select position; A saves; B cancels |
| Birthday | UP/DOWN change digit; LEFT/RIGHT select digit; A validates/saves MM-DD; B cancels |
| Date/time | Open the CLOCK editor; LEFT/RIGHT select month/day/year/hour/minute; UP/DOWN change value; A saves; B backs out |
| Clear profile | Select clear, then A confirms or B cancels |

Name and annual birthday are stored in Preferences/NVS. Birthday has no birth year or age. February 29 is observed on March 1 in non-leap years.

The clock must be valid for date-based behavior. The ESP32 keeps time while powered; full power loss requires manual date/time entry, Wi-Fi time sync or app USB time sync. The accessory switch leaves the ESP32 powered.

## Countdown and accessory switch

With a valid birthday and clock, every successful accessory OFF → ON restore shows a countdown for five seconds before returning to the previous screen. It also wakes software sleep. If time is unavailable, the countdown waits for time sync.

On ordinary days, a full ESP32 startup has a one-in-three countdown chance. On the birthday, Birthday Mode always queues the cake greeting as soon as the date becomes available, including after USB/Wi-Fi time sync.

## All-day Birthday Mode

Firmware 0.18.4 adds:
- Animated cake/candles at birthday startup and accessory switch-on.
- Slow rainbow shimmer at the user's brightness, without overwriting normal colors/patterns.
- A 90-second screensaver cycle: ten seconds each of cake, balloons, confetti, present and wishes, then forty seconds of the chosen saver/art gallery.
- **A** on the present opens a random two-line wish for six seconds. **B** exits the screensaver.
- Normal games/menu controls throughout the day.
- Automatic return to normal color/pattern choices at local midnight.

OFF is honored. Selecting a color or pattern takes priority over celebration lighting for the rest of that powered birthday session; a full reboot resets this temporary override. Existing accessory pause and one-hour software sleep remain active.

For a quick board test, temporarily save today's month/day as the birthday. Check cake greeting, present/A, B exit, normal light controls and accessory OFF/ON. Restore the real birthday afterward. Midnight restoration remains a separate test.

## Art gallery

The existing SPIFFS partition holds up to 50 one- or two-frame monochrome pictures. Each frame is 1,024 bytes; fifty two-frame pictures use approximately 100 KiB of image data. Gallery playback rotates pictures every five seconds and changes animation frames every 400 ms.

The app asks before first-time art filesystem initialization. Normal firmware updates use the existing partition layout; profile values are in separate Preferences/NVS settings.

## Compile and upload on Raspberry Pi

From this repository's root, update the checkout and install dependencies if needed:

```bash
git pull --ff-only
arduino-cli core install esp32:esp32
arduino-cli lib install U8g2
```

Confirm the connected board is the 4 MB SuperMini on `/dev/ttyACM0`; use its actual port if different. The detached update below survives closing the terminal and only uploads after compilation succeeds:

```bash
pkill -f '[h]appyjarz_plug_watch.py' || true
pkill -f '[m]ini-app/app.py' || true
mkdir -p "$HOME/.happyjarz"
rm -f "$HOME/.happyjarz/firmware_update.exit"

HJ_UPDATE_REPO="$PWD" nohup bash -c '
cd "$HJ_UPDATE_REPO" || exit 1
if arduino-cli compile \
  --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc \
  firmware/HappyJarzMini > "$HOME/.happyjarz/firmware_update.log" 2>&1 &&
  arduino-cli upload \
  --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc \
  --upload-property upload.speed=115200 \
  --port /dev/ttyACM0 \
  firmware/HappyJarzMini >> "$HOME/.happyjarz/firmware_update.log" 2>&1
then
  result=0
else
  result=$?
fi
echo "$result" > "$HOME/.happyjarz/firmware_update.exit"
if [ "$result" -eq 0 ]; then
  nohup /usr/bin/python3 "$PWD/mini-app/happyjarz_plug_watch.py" >/dev/null 2>&1 </dev/null &
fi
exit "$result"
' >/dev/null 2>&1 </dev/null &

echo "Update started; PID $!"
```

**Keep USB connected and the accessory switch steady until the upload finishes and the app opens.** The OLED may be blank during flashing. The app opens only after successful upload, watcher restart and a HAPPY JARZ identity handshake.

Check progress or failure:

```bash
cat "$HOME/.happyjarz/firmware_update.exit" 2>/dev/null || echo "No completion marker yet"
ps -eo pid,stat,etime,args | grep -E '[a]rduino-cli|[e]sptool|[h]appyjarz_plug_watch.py'
tail -n 12 "$HOME/.happyjarz/firmware_update.log"
tail -n 8 "$HOME/.happyjarz/plug_watch.log"
```

The absent exit marker alone does not prove a process is still running; use the process list and log together. Success requires exit **0**, final application hash verification and a fresh `HJ|IDENTITY|...|fw=0.18.8` line.

### Interrupted upload recovery

An interrupted write can leave an incomplete application and blank OLED. The compile may have succeeded even though upload failed. Once the failed upload process has exited, retry the built sketch:

```bash
arduino-cli upload \
  --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc \
  --upload-property upload.speed=115200 \
  --port /dev/ttyACM0 \
  firmware/HappyJarzMini
```

Keep the watcher/controller stopped during the retry. After successful upload, restart the watcher from the repository root:

```bash
nohup /usr/bin/python3 "$PWD/mini-app/happyjarz_plug_watch.py" >/dev/null 2>&1 </dev/null &
```

## Verification record

Firmware 0.18.2 passed the Pi compile and reported its identity after upload. Firmware 0.18.3 passed compile and the Architect reported reconnecting after retrying an interrupted upload.

Firmware 0.18.4 passed extracted C++ host tests for birthday activation, midnight restoration, light override/OFF, gift expiry, scene mixing, drawing bounds, accessory/sleep gating and February 29 behavior. Host tests do not compile the complete ESP32 sketch. The update instructions were provided and the Architect gave positive feedback; a 0.18.4 upload/identity log and detailed feature results have not been recorded in this document.

See [BUILD_STATUS.md](../BUILD_STATUS.md) for release checks.

### 0.18.7 — prepare all nine touch channels before calibration

Startup and `TEST INPUT` / `TEST TOUCH` now initialize every mapped touch
channel (Up, Down, Left, Right, A, B, L, R, Start), then discard one second
of round-robin readings before recording baselines. This removes the reliance
on the screensaver's early reads of only the original six pins. All nine pads
retain direct `touchRead()`, 40 samples per baseline, the +20% threshold,
60 ms qualification, and slow baseline drift. Idle counts are not rescaled.

This addresses startup calibration preparation; physical L/R/Start response
still needs a board test. A nonzero count or `TEST INPUT` PASS means the
calibration routine ran, not that somebody touched every pad successfully.
After flashing, leave all pads untouched until startup finishes. Check
`HJ|TOUCH_CAL` against steady `HJ|INPUT` readings, then touch L/R/Start
separately. L/R should adjust brightness and emit physical input events.


## 0.18.8 — correct SuperMini battery / charging model

Production hardware uses the ESP32-S3 SuperMini B+/B- battery pads with a protected 1-cell LiPo through a JST connector. The OLED and APA106 lamps are powered from the board's regulated 3V3 rail.

GPIO3 is treated as the SuperMini onboard VBUS-sense divider, not as a direct battery-positive measurement. The previous firmware multiplied that reading by a calibration factor and converted it into a LiPo percentage; that could produce believable but false percentages and false LOW BATTERY / CHARGE ME warnings.

Firmware 0.18.8 therefore:
- removes the old 0.802 battery calibration fudge factor
- uses GPIO3 only as a VBUS / power-source monitor
- distinguishes USB power from battery operation using the VBUS rail
- reports USB charging as hardware-controlled
- disables battery percentage and low-battery warnings until a direct B+ sense circuit is physically installed and calibrated
- keeps the OLED and four APA106 lamps on the existing regulated 3V3 production wiring

The POWER display now shows USB/charging or battery power plus the measured VBUS rail and reports battery percentage as N/A. A future direct B+ ADC divider can restore a calibrated fuel gauge without changing the working charging hardware.


## 0.18.9 — onboard RGB coarse battery indicator

The SuperMini onboard addressable RGB LED on GPIO48 now gives a low-power coarse battery indication while running from the LiPo:

- green: high battery band
- yellow: middle battery band
- orange: low battery band
- red: critical battery band, approximately the bottom ~10% zone

Above critical, the onboard RGB gives one short pulse every eight seconds to minimize battery draw. In the critical band it strobes red rapidly as an obvious charge-now warning. The four external APA106 jar lamps are not used for this alert.

While USB power is present, the onboard RGB stays off because the board's separate blue charger LED already indicates charging state and GPIO3/VBUS cannot provide a valid battery-level estimate during USB charging.

These are coarse VBUS-derived bands, not a calibrated fuel-gauge percentage. OLED battery percentage remains N/A until a direct B+ sensor is installed and calibrated.


## 0.18.10 — production SuperMini LED correction

Physical production testing confirmed this SuperMini revision has only the red board LED and blue charger-status LED; no separate addressable third RGB LED was confirmed. The provisional GPIO48 battery-band indicator added in 0.18.9 has therefore been removed.

Production firmware does not drive GPIO48 for battery indication. Battery/charging status remains:
- blue charger LED: hardware-controlled by the SuperMini charger circuit
- OLED: USB/battery power state and VBUS monitor
- battery percentage: intentionally unavailable until a direct B+ sensor is installed and calibrated

The physical production board is authoritative over clone-family pinout assumptions.


## 0.18.11 — production VBUS rail calibration

Physical DMM comparison on the production SuperMini showed:

- external meter: 3.67 V
- firmware/OLED rail reading: 3.28 V

The VBUS/power-rail calibration factor is therefore set to **1.119**, calculated from 3.67 / 3.28. This is a one-point calibration for the current production board and corrects the displayed rail voltage.

This does not convert GPIO3 into a true battery fuel gauge. Battery percentage remains disabled and the OLED continues to treat this as a power-rail measurement rather than direct B+ cell voltage.


## 0.18.12 — OLED is optional; controls stay live

Physical testing showed that unplugging OLED VCC caused the touch controls to stop working. The cause was architectural: OLED presence was being used as the accessory-rail detector, and losing the OLED forced the whole accessory layer into a paused state.

Firmware 0.18.12 removes that coupling:

- OLED presence no longer pauses touch inputs
- OLED absence no longer clamps the APA106 lamp data output
- buttons/touch controls continue working with OLED unplugged
- lamps/pattern engine continue operating without OLED
- serial, timers, games and other core logic remain active
- OLED is now treated as an optional hot-pluggable display
- when the OLED reappears, firmware reinitializes it automatically

GPIO3 remains the calibrated rail/VBUS monitor, not a direct battery fuel gauge.
