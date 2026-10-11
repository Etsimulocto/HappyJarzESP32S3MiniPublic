# HAPPY JARZ Mini firmware

| Item | Current value |
| --- | --- |
| Firmware | **0.18.5** |
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

The absent exit marker alone does not prove a process is still running; use the process list and log together. Success requires exit **0**, final application hash verification and a fresh `HJ|IDENTITY|...|fw=0.18.5` line.

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
