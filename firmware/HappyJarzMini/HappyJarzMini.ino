// HAPPY JARZ ESP32-S3 SuperMini firmware v0.18.3
// SIMPLE profile: six capacitive inputs, 128x64 OLED, four APA106 lamps,
// local games/screensavers, and the HAPPY JARZ USB control protocol.
//
// HARDWARE MAP:
//   GPIO7 -> 220R -> APA106 #1 DIN -> #2 DIN -> #3 DIN -> #4 DIN
//   RMT 10 MHz; 0 = 4/14 ticks, 1 = 14/4 ticks; 100 us latch low
//
// SIX TOUCH INPUTS:
//   GPIO4  = UP
//   GPIO5  = DOWN
//   GPIO9  = LEFT
//   GPIO10 = RIGHT
//   GPIO1  = A / SELECT
//   GPIO2  = B / BACK / ESC; hold B = HOME event
//
// OLED:
//   128x64 I2C SSD1306, address 0x3C, SDA GPIO8, SCL GPIO6.

#include <Arduino.h>
#include "esp_system.h"
#include <Preferences.h>
#include <WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "happyjarz_arcade.h"
#include "happyjarz_art.h"
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include "esp32-hal-rmt.h"
#include "happyjarz_led_types.h"

static const char *HJ_HW_VERSION = "V1";
static const char *HJ_FW_VERSION = "0.18.3";

static String hjChipId() {
  char id[13];
  uint64_t mac = ESP.getEfuseMac() & 0xFFFFFFFFFFFFULL;
  snprintf(id, sizeof(id), "%012llX", (unsigned long long)mac);
  return String(id);
}

static String hjSerialId() {
  String serial = "HJ-";
  serial += hjChipId();
  return serial;
}

// -----------------------------
// Hardware map
// -----------------------------
static constexpr uint8_t LED_DATA_PIN = 7;
static constexpr uint8_t INPUT_A_PIN = 1;
static constexpr uint8_t INPUT_B_PIN = 2;
static constexpr uint8_t INPUT_UP_PIN = 4;
static constexpr uint8_t INPUT_DOWN_PIN = 5;
static constexpr uint8_t INPUT_LEFT_PIN = 9;
static constexpr uint8_t INPUT_RIGHT_PIN = 10;
static constexpr size_t INPUT_COUNT = 6;

// -----------------------------
// LED layer — do not casually change
// -----------------------------
static Rgb ledColor[LED_COUNT] = {{32,0,64},{0,32,64},{32,0,64},{0,32,64}};
static uint8_t brightnessPercent = 75;
static String patternName = "SOLID";

// HAPPYJARZ_SLEEP_MODE_V1
static constexpr unsigned long HJ_SLEEP_IDLE_MS = 3600000UL; // 1 hour
static bool hjSleepActive = false;
static unsigned long hjSleepLastActivityMs = 0;
static void hjWakeFromSleep(const char *reason);

// HAPPYJARZ_ACCESSORY_PAUSE_V1
// OLED presence doubles as the switched 3.3 V accessory-rail sensor.
static bool hjAccessoryPaused = false;
static bool hjAccessoryKnown = false;
static unsigned long hjAccessoryLastProbeMs = 0;
static constexpr unsigned long HJ_ACCESSORY_PROBE_MS = 100UL;

// HAPPYJARZ_LOW_BATTERY_WARNING_EARLY
static constexpr uint8_t HJ_LOW_BATTERY_LED_INDEX = 3; // physical bulb 4
static bool hjLowBatteryWarning = false;
static bool hjLowBatteryFlashOn();

static bool initApa106Rmt() {
  pinMode(LED_DATA_PIN, OUTPUT);
  digitalWrite(LED_DATA_PIN, LOW);
  if (!rmtInit(LED_DATA_PIN, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 10000000)) return false;
  rmtSetEOT(LED_DATA_PIN, 0);
  return true;
}


// Host LED mirror: raw pattern colors before global brightness scaling.
// The physical APA106 writer remains untouched; this only observes frames.
static bool hjLedStream = false;
static bool hjLedHaveFrame = false;
static Rgb hjLedLastFrame[LED_COUNT];
static unsigned long hjLedLastMirrorMs = 0;

static void hjPrintLedFrame(const Rgb frame[LED_COUNT]) {
  Serial.print("HJ|LED_FRAME");
  for (uint8_t i=0; i<LED_COUNT; ++i) {
    Serial.print("|led"); Serial.print(i+1); Serial.print("=");
    Serial.print(frame[i].r); Serial.print(",");
    Serial.print(frame[i].g); Serial.print(",");
    Serial.print(frame[i].b);
  }
  Serial.print("|pattern="); Serial.print(patternName);
  Serial.print("|brightness="); Serial.println(brightnessPercent);
}

static void hjObserveLedFrame(const Rgb frame[LED_COUNT]) {
  memcpy(hjLedLastFrame, frame, sizeof(hjLedLastFrame));
  hjLedHaveFrame = true;
  if (!hjLedStream || !Serial) return;
  unsigned long now = millis();
  if (now - hjLedLastMirrorMs < 100UL) return;
  hjLedLastMirrorMs = now;
  hjPrintLedFrame(frame);
}

static void hjEmitLedFrameNow() {
  if (hjLedHaveFrame) hjPrintLedFrame(hjLedLastFrame);
  else hjPrintLedFrame(ledColor);
}

static void writeFrame(const Rgb frame[LED_COUNT], uint8_t brightness) {
  if (hjSleepActive) return;
  if (hjAccessoryPaused) { pinMode(LED_DATA_PIN, OUTPUT); digitalWrite(LED_DATA_PIN, LOW); return; }
  rmt_data_t symbols[LED_COUNT * 24];
  size_t n = 0;
  for (uint8_t led = 0; led < LED_COUNT; ++led) {
    Rgb txColor = frame[led];
    if (hjLowBatteryWarning && led == HJ_LOW_BATTERY_LED_INDEX && hjLowBatteryFlashOn()) {
      txColor = {255,0,0};
    }
    uint8_t bytes[3] = {
      (uint8_t)((uint16_t)txColor.r * brightness / 100U),
      (uint8_t)((uint16_t)txColor.g * brightness / 100U),
      (uint8_t)((uint16_t)txColor.b * brightness / 100U)
    };
    for (uint8_t c = 0; c < 3; ++c) {
      for (int bit = 7; bit >= 0; --bit) {
        bool one = bytes[c] & (1U << bit);
        symbols[n].level0 = 1;
        symbols[n].duration0 = one ? 14 : 4;
        symbols[n].level1 = 0;
        symbols[n].duration1 = one ? 4 : 14;
        ++n;
      }
    }
  }
  rmtWrite(LED_DATA_PIN, symbols, n, RMT_WAIT_FOR_EVER);
  digitalWrite(LED_DATA_PIN, LOW);
  delayMicroseconds(100);
  hjObserveLedFrame(frame);
}

static void showLeds() { writeFrame(ledColor, brightnessPercent); }
static void allOff() {
  Rgb off[LED_COUNT] = {{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
  writeFrame(off, 100);
}

static void setLedRaw(uint8_t led, uint8_t r, uint8_t g, uint8_t b) {
  if (led < 1 || led > LED_COUNT) return;
  ledColor[led - 1] = {r,g,b};
  showLeds();
}

// -----------------------------
// Six capacitive inputs
// -----------------------------
enum InputIndex : uint8_t { IN_UP=0, IN_DOWN, IN_LEFT, IN_RIGHT, IN_A, IN_B };
struct TouchPadState {
  uint8_t pin;
  const char *name;
  uint32_t baseline;
  uint32_t threshold;
  bool touching;
  bool qualified;
  unsigned long enteredMs;
  unsigned long qualifiedMs;
};

static TouchPadState inputs[INPUT_COUNT] = {
  {INPUT_UP_PIN, "up", 0,0,false,false,0,0},
  {INPUT_DOWN_PIN, "down", 0,0,false,false,0,0},
  {INPUT_LEFT_PIN, "left", 0,0,false,false,0,0},
  {INPUT_RIGHT_PIN, "right", 0,0,false,false,0,0},
  {INPUT_A_PIN, "a", 0,0,false,false,0,0},
  {INPUT_B_PIN, "b", 0,0,false,false,0,0}
};

static String inputMode = "JAR";
static bool inputStream = false;
static unsigned long lastInputStreamMs = 0;
static bool bHomeSent = false;

// Host/app virtual keys join the SAME logical input path as copper touch.
static bool hjHostKeyPending[INPUT_COUNT] = {false,false,false,false,false,false};

static int8_t hjHostKeyIndex(String name) {
  name.trim();
  name.toUpperCase();
  if (name == "UP") return IN_UP;
  if (name == "DOWN") return IN_DOWN;
  if (name == "LEFT") return IN_LEFT;
  if (name == "RIGHT") return IN_RIGHT;
  if (name == "A") return IN_A;
  if (name == "B") return IN_B;
  return -1;
}

static void hjPrintCaps() {
  Serial.println("HJ|CAPS|profile=SIMPLE|controls=up,down,left,right,a,b|oled=128x64|oled_mirror=1|leds=4|led_mirror=1");
}

static uint32_t readTouchPin(uint8_t pin) { return touchRead(pin); }

static void calibrateInputs() {
  for (auto &pad : inputs) {
    uint64_t sum = 0;
    for (int i = 0; i < 40; ++i) { sum += readTouchPin(pad.pin); delay(5); }
    pad.baseline = (uint32_t)(sum / 40ULL);
    pad.threshold = pad.baseline + (pad.baseline / 5U);
    pad.touching = false;
    pad.qualified = false;
    pad.enteredMs = 0;
    pad.qualifiedMs = 0;
  }
  bHomeSent = false;
}

static void printTouchCalibration() {
  Serial.print("HJ|TOUCH_CAL");
  for (uint8_t i=0; i<INPUT_COUNT; ++i) {
    Serial.print("|"); Serial.print(inputs[i].name); Serial.print("_base="); Serial.print(inputs[i].baseline);
    Serial.print("|"); Serial.print(inputs[i].name); Serial.print("_thr="); Serial.print(inputs[i].threshold);
  }
  Serial.println();
}

static bool updateInputState(uint8_t idx) {
  TouchPadState &pad = inputs[idx];
  uint32_t value = readTouchPin(pad.pin);
  bool above = value >= pad.threshold;
  unsigned long now = millis();
  if (above && !pad.touching) {
    pad.touching = true;
    pad.enteredMs = now;
  } else if (!above) {
    pad.touching = false;
    pad.qualified = false;
    pad.enteredMs = 0;
    pad.qualifiedMs = 0;
    pad.baseline = (pad.baseline * 127U + value) / 128U;
    pad.threshold = pad.baseline + (pad.baseline / 5U);
    if (idx == IN_B) bHomeSent = false;
  }
  bool q = pad.touching && (now - pad.enteredMs >= 60);
  if (q && !pad.qualified) pad.qualifiedMs = now;
  pad.qualified = q;
  return q;
}

static void printInputTelemetry() {
  Serial.printf("HJ|INPUT|up=%lu|down=%lu|left=%lu|right=%lu|a=%lu|b=%lu\n",
    (unsigned long)readTouchPin(INPUT_UP_PIN),
    (unsigned long)readTouchPin(INPUT_DOWN_PIN),
    (unsigned long)readTouchPin(INPUT_LEFT_PIN),
    (unsigned long)readTouchPin(INPUT_RIGHT_PIN),
    (unsigned long)readTouchPin(INPUT_A_PIN),
    (unsigned long)readTouchPin(INPUT_B_PIN));
}

// -----------------------------
// Settings / persistence
// -----------------------------
static Preferences prefs;
static String wifiSsid;
static String wifiPassword;
static String timezoneName = "America/Chicago";
static bool alarmEnabled = false;
static uint8_t alarmHour = 7;
static uint8_t alarmMinute = 30;
static bool timerEnabled = false;
static uint32_t timerMinutes = 60;
static unsigned long timerStartedMs = 0;
static uint8_t displayBrightness = 100;
static String screensaver = "CLOCK";
static uint16_t screensaverDelayMin = 5;
static String ownerName;
static String ownerBirthday = "--";
static time_t hjBirthdayCacheMinute = (time_t)-1;
static bool hjBirthdayCacheValue = false;
static bool hjBirthdayToday();
static bool hjStartupCountdownPending = false;
static bool hjStartupCountdownActive = false;
static unsigned long hjStartupCountdownUntilMs = 0;
static int hjStartupCountdownDays = -1;


static const char *tzPosixFor(const String &name) {
  if (name == "America/New_York") return "EST5EDT,M3.2.0/2,M11.1.0/2";
  if (name == "America/Chicago") return "CST6CDT,M3.2.0/2,M11.1.0/2";
  if (name == "America/Denver") return "MST7MDT,M3.2.0/2,M11.1.0/2";
  if (name == "America/Los_Angeles") return "PST8PDT,M3.2.0/2,M11.1.0/2";
  if (name == "UTC" || name == "Etc/UTC") return "UTC0";
  return "CST6CDT,M3.2.0/2,M11.1.0/2"; // safe project default
}

static void applyConfiguredTimezone() {
  setenv("TZ", tzPosixFor(timezoneName), 1);
  tzset();
}

static void loadSettings() {
  prefs.begin("happyjarz", true);
  brightnessPercent = min((uint8_t)100, prefs.getUChar("bright", (uint8_t)50));
  ledColor[0] = {prefs.getUChar("l1r",32), prefs.getUChar("l1g",0), prefs.getUChar("l1b",64)};
  ledColor[1] = {prefs.getUChar("l2r",0), prefs.getUChar("l2g",32), prefs.getUChar("l2b",64)};
  ledColor[2] = {prefs.getUChar("l3r",32), prefs.getUChar("l3g",0), prefs.getUChar("l3b",64)};
  ledColor[3] = {prefs.getUChar("l4r",0), prefs.getUChar("l4g",32), prefs.getUChar("l4b",64)};
  patternName = prefs.getString("pattern", "SOLID");
  wifiSsid = prefs.getString("wifi_ssid", "");
  wifiPassword = prefs.getString("wifi_pass", "");
  timezoneName = prefs.getString("timezone", "America/Chicago");
  alarmEnabled = prefs.getBool("alarm_on", false);
  alarmHour = prefs.getUChar("alarm_h", 7);
  alarmMinute = prefs.getUChar("alarm_m", 30);
  timerMinutes = prefs.getUInt("timer_min", 60);
  displayBrightness = prefs.getUChar("disp_bri", 100);
  screensaver = prefs.getString("screensaver", "CLOCK");
  screensaverDelayMin = prefs.getUShort("ss_delay", 5);
  ownerName = prefs.getString("owner_name", "");
  ownerBirthday = prefs.getString("owner_bday", "--");
  inputMode = "JAR";
  prefs.end();
}

static void persistSettings() {
  prefs.begin("happyjarz", false);
  prefs.putUChar("bright", brightnessPercent);
  prefs.putUChar("l1r", ledColor[0].r); prefs.putUChar("l1g", ledColor[0].g); prefs.putUChar("l1b", ledColor[0].b);
  prefs.putUChar("l2r", ledColor[1].r); prefs.putUChar("l2g", ledColor[1].g); prefs.putUChar("l2b", ledColor[1].b);
  prefs.putUChar("l3r", ledColor[2].r); prefs.putUChar("l3g", ledColor[2].g); prefs.putUChar("l3b", ledColor[2].b);
  prefs.putUChar("l4r", ledColor[3].r); prefs.putUChar("l4g", ledColor[3].g); prefs.putUChar("l4b", ledColor[3].b);
  prefs.putString("pattern", patternName);
  prefs.putString("wifi_ssid", wifiSsid);
  prefs.putString("wifi_pass", wifiPassword);
  prefs.putString("timezone", timezoneName);
  prefs.putBool("alarm_on", alarmEnabled);
  prefs.putUChar("alarm_h", alarmHour); prefs.putUChar("alarm_m", alarmMinute);
  prefs.putUInt("timer_min", timerMinutes);
  prefs.putUChar("disp_bri", displayBrightness);
  prefs.putString("screensaver", screensaver);
  prefs.putUShort("ss_delay", screensaverDelayMin);
  prefs.putString("owner_name", ownerName);
  prefs.putString("owner_bday", ownerBirthday);
  prefs.end();
}

// -----------------------------
// Pattern engine — sensory library
// -----------------------------
static constexpr uint8_t HJ_MAX_BRIGHTNESS = 100;
static unsigned long patternLastMs = 0;
static uint16_t patternStep = 0;

static uint8_t safeBrightness(uint8_t requested) {
  return requested > HJ_MAX_BRIGHTNESS ? HJ_MAX_BRIGHTNESS : requested;
}

static Rgb wheel(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85) return {(uint8_t)(255-pos*3),0,(uint8_t)(pos*3)};
  if (pos < 170) { pos -= 85; return {0,(uint8_t)(pos*3),(uint8_t)(255-pos*3)}; }
  pos -= 170; return {(uint8_t)(pos*3),(uint8_t)(255-pos*3),0};
}

static Rgb blendRgb(const Rgb &a, const Rgb &b, uint8_t amount) {
  uint16_t inv = 255U - amount;
  return {
    (uint8_t)(((uint16_t)a.r*inv + (uint16_t)b.r*amount)/255U),
    (uint8_t)(((uint16_t)a.g*inv + (uint16_t)b.g*amount)/255U),
    (uint8_t)(((uint16_t)a.b*inv + (uint16_t)b.b*amount)/255U)
  };
}

static uint8_t triangle8(uint16_t step, uint16_t period) {
  if (period < 2) return 255;
  uint16_t x = step % period;
  uint16_t half = period / 2U;
  if (x < half) return (uint8_t)((uint32_t)x * 255U / half);
  return (uint8_t)((uint32_t)(period - 1U - x) * 255U / half);
}

static void resetPatternEngine(){ patternStep=0; patternLastMs=0; }

static const char *PATTERN_NAMES[] = {
  "SOLID","FADE","PULSE","RAINBOW","RANDOM",
  "HUE_FADE","DUAL_HUE","BREATH","DRIFT","AURORA","OCEAN","LAVENDER","SUNSET",
  "CHRISTMAS","HALLOWEEN","VALENTINE","EASTER","FOURTH","THANKSGIVING",
  "CANDY","GALAXY","FIRE","ICE","FOREST","NEON","TWINKLE","SPARKLE","COLOR_SWAP",
  "COMET","FIREFLY","BUBBLEGUM",
  "CHASE_CW","CHASE_CCW","JAR_CHASE","SIDE_CHASE",
  "SWEEP_LR","SWEEP_TS","DIAGONAL","PING_PONG",
  "DUAL_CHASE","OPP_CHASE","JAR_PULSE","SIDE_ACCENT",
  "WATERFALL","RIPPLE","TIDAL","MOONWAVE","DEEPSEA","LAGOON","ARCTIC","GLACIER",
  "EMBER","CAMPFIRE","SUNRISE","SOLAR","INFERNO","CANDLE","AUTUMN","HARVEST",
  "MINT","MEADOW","JUNGLE","MOSS","CLOVER","SPRING","RAINFOREST","LIMEADE",
  "DREAM","COTTON_CANDY","MERMAID","UNICORN","VAPOR","COSMIC","STARDUST","NEBULA",
  "ROSE_GOLD","PEACH","CORAL","CHERRY","BERRY","PLUM","GRAPE","MAGENTA",
  "ELECTRIC","CYBER","LASER","SYNTH","ARCADE","MATRIX","PLASMA","HOLOGRAM",
  "RAIN","THUNDER","LIGHTNING","CLOUD","SNOW","FROST","SUNBEAM","MOONLIGHT",
  "OFF"
};
static constexpr uint8_t PATTERN_COUNT = sizeof(PATTERN_NAMES)/sizeof(PATTERN_NAMES[0]);

static bool isPatternName(const String &name) {
  for (uint8_t i=0; i<PATTERN_COUNT; ++i) if (name == PATTERN_NAMES[i]) return true;
  return false;
}

static void showPair(const Rgb &a, const Rgb &b, uint8_t brightness) {
  Rgb frame[LED_COUNT] = {a,b,a,b};
  writeFrame(frame, safeBrightness(brightness));
}

static void showFour(const Rgb &a, const Rgb &b, const Rgb &c, const Rgb &d, uint8_t brightness) {
  Rgb frame[LED_COUNT] = {a,b,c,d};
  writeFrame(frame, safeBrightness(brightness));
}

// Physical HAPPY JARZ stand topology.
// Bulb 1/2 light the jars; bulb 3/4 are the left/right box-side accents.
static constexpr uint8_t HJ_TOP_LEFT   = 0;
static constexpr uint8_t HJ_TOP_RIGHT  = 1;
static constexpr uint8_t HJ_SIDE_LEFT  = 2;
static constexpr uint8_t HJ_SIDE_RIGHT = 3;

static void hjBlankFrame(Rgb frame[LED_COUNT]) {
  for (uint8_t i=0; i<LED_COUNT; ++i) frame[i]={0,0,0};
}

static Rgb hjDim(const Rgb &c, uint8_t amount) {
  return blendRgb({0,0,0}, c, amount);
}

// HAPPYJARZ_PATTERN_BANK_100
// Compact descriptor-driven bank: many named looks, one renderer.
enum HjBankMode : uint8_t {
  HJ_BANK_FLOW=0, HJ_BANK_WAVE, HJ_BANK_SWEEP, HJ_BANK_GLOW,
  HJ_BANK_SPARK, HJ_BANK_PAIR, HJ_BANK_DIAG, HJ_BANK_RIPPLE
};

struct HjBankPattern {
  const char *name;
  uint8_t mode;
  uint16_t intervalMs;
  Rgb p0, p1, p2, p3;
};

static const HjBankPattern HJ_BANK_PATTERNS[] = {
  // Water / ice
  {"WATERFALL",HJ_BANK_FLOW,55,{0,40,180},{0,180,255},{30,255,220},{0,80,255}},
  {"RIPPLE",HJ_BANK_RIPPLE,85,{0,80,220},{40,200,255},{0,255,210},{10,40,150}},
  {"TIDAL",HJ_BANK_WAVE,65,{0,20,100},{0,150,255},{0,220,180},{0,70,170}},
  {"MOONWAVE",HJ_BANK_GLOW,80,{20,20,100},{80,90,255},{120,180,255},{20,50,130}},
  {"DEEPSEA",HJ_BANK_FLOW,75,{0,5,40},{0,30,100},{0,110,150},{10,0,70}},
  {"LAGOON",HJ_BANK_PAIR,90,{0,180,190},{0,255,160},{0,90,255},{40,220,220}},
  {"ARCTIC",HJ_BANK_SPARK,110,{100,210,255},{220,250,255},{80,130,255},{255,255,255}},
  {"GLACIER",HJ_BANK_DIAG,120,{80,160,255},{180,240,255},{20,80,200},{220,255,255}},

  // Fire / warm
  {"EMBER",HJ_BANK_SPARK,95,{120,5,0},{255,40,0},{255,120,0},{255,220,80}},
  {"CAMPFIRE",HJ_BANK_FLOW,70,{180,20,0},{255,80,0},{255,180,20},{120,0,0}},
  {"SUNRISE",HJ_BANK_GLOW,75,{255,20,60},{255,100,20},{255,210,80},{180,20,100}},
  {"SOLAR",HJ_BANK_WAVE,60,{255,60,0},{255,180,0},{255,255,80},{255,80,20}},
  {"INFERNO",HJ_BANK_SWEEP,55,{255,0,0},{255,70,0},{255,180,0},{120,0,0}},
  {"CANDLE",HJ_BANK_GLOW,105,{255,80,10},{255,180,50},{255,230,150},{180,40,0}},
  {"AUTUMN",HJ_BANK_PAIR,125,{180,35,0},{255,90,0},{180,90,0},{110,20,0}},
  {"HARVEST",HJ_BANK_RIPPLE,130,{255,110,0},{190,55,0},{255,190,20},{120,30,0}},

  // Forest / green
  {"MINT",HJ_BANK_GLOW,80,{40,255,170},{140,255,210},{0,180,120},{80,230,170}},
  {"MEADOW",HJ_BANK_WAVE,85,{20,130,20},{100,255,60},{40,190,80},{180,255,80}},
  {"JUNGLE",HJ_BANK_FLOW,65,{0,50,10},{0,150,40},{80,220,20},{0,90,60}},
  {"MOSS",HJ_BANK_GLOW,110,{30,80,10},{80,130,30},{120,160,50},{20,60,15}},
  {"CLOVER",HJ_BANK_DIAG,105,{0,180,40},{40,255,80},{0,120,20},{120,255,100}},
  {"SPRING",HJ_BANK_PAIR,95,{80,255,100},{255,180,220},{120,220,255},{255,240,100}},
  {"RAINFOREST",HJ_BANK_RIPPLE,70,{0,100,50},{0,220,100},{20,130,180},{100,255,80}},
  {"LIMEADE",HJ_BANK_SPARK,100,{120,255,0},{220,255,40},{60,220,0},{255,255,160}},

  // Dream / cosmic
  {"DREAM",HJ_BANK_FLOW,70,{100,40,255},{255,80,220},{80,180,255},{180,100,255}},
  {"COTTON_CANDY",HJ_BANK_WAVE,85,{255,80,190},{100,200,255},{255,160,220},{160,100,255}},
  {"MERMAID",HJ_BANK_PAIR,75,{0,255,190},{0,130,255},{180,60,255},{40,220,220}},
  {"UNICORN",HJ_BANK_FLOW,60,{255,80,200},{120,160,255},{180,80,255},{80,255,220}},
  {"VAPOR",HJ_BANK_DIAG,80,{255,0,200},{0,220,255},{130,0,255},{255,80,120}},
  {"COSMIC",HJ_BANK_SPARK,90,{30,0,80},{80,0,200},{180,0,255},{255,255,255}},
  {"STARDUST",HJ_BANK_SPARK,125,{50,20,120},{120,70,255},{40,120,255},{255,255,220}},
  {"NEBULA",HJ_BANK_RIPPLE,80,{80,0,160},{220,0,150},{0,80,220},{120,20,255}},

  // Pink / fruit
  {"ROSE_GOLD",HJ_BANK_GLOW,90,{255,100,120},{255,170,130},{220,100,150},{255,210,160}},
  {"PEACH",HJ_BANK_WAVE,95,{255,120,80},{255,190,120},{255,100,110},{255,220,160}},
  {"CORAL",HJ_BANK_FLOW,75,{255,70,80},{255,130,90},{255,80,140},{255,170,120}},
  {"CHERRY",HJ_BANK_PAIR,100,{180,0,30},{255,0,60},{120,0,30},{255,60,100}},
  {"BERRY",HJ_BANK_DIAG,90,{150,0,100},{255,20,150},{100,0,180},{220,60,200}},
  {"PLUM",HJ_BANK_GLOW,105,{80,0,90},{170,20,150},{110,0,130},{220,80,180}},
  {"GRAPE",HJ_BANK_RIPPLE,80,{80,0,180},{150,0,255},{220,40,255},{60,0,120}},
  {"MAGENTA",HJ_BANK_SPARK,95,{255,0,180},{180,0,255},{255,60,220},{255,220,255}},

  // Neon / arcade
  {"ELECTRIC",HJ_BANK_FLOW,45,{0,255,255},{40,80,255},{180,0,255},{0,255,120}},
  {"CYBER",HJ_BANK_DIAG,55,{0,255,220},{255,0,200},{80,0,255},{0,140,255}},
  {"LASER",HJ_BANK_SWEEP,45,{255,0,30},{0,255,100},{0,120,255},{255,0,220}},
  {"SYNTH",HJ_BANK_WAVE,55,{255,0,180},{0,220,255},{120,0,255},{255,80,0}},
  {"ARCADE",HJ_BANK_PAIR,70,{255,0,0},{0,255,0},{0,80,255},{255,220,0}},
  {"MATRIX",HJ_BANK_RIPPLE,65,{0,60,0},{0,255,40},{80,255,100},{0,120,20}},
  {"PLASMA",HJ_BANK_FLOW,40,{255,0,100},{0,150,255},{120,0,255},{0,255,180}},
  {"HOLOGRAM",HJ_BANK_SPARK,85,{40,220,255},{180,80,255},{0,255,190},{255,255,255}},

  // Weather / sky
  {"RAIN",HJ_BANK_RIPPLE,75,{20,80,180},{50,140,255},{0,180,255},{10,40,100}},
  {"THUNDER",HJ_BANK_GLOW,115,{20,20,50},{60,60,120},{100,80,180},{10,10,30}},
  {"LIGHTNING",HJ_BANK_SPARK,135,{30,40,90},{80,120,220},{170,200,255},{255,255,255}},
  {"CLOUD",HJ_BANK_WAVE,110,{100,120,150},{180,200,220},{130,160,200},{220,230,240}},
  {"SNOW",HJ_BANK_SPARK,120,{150,200,255},{220,240,255},{120,170,255},{255,255,255}},
  {"FROST",HJ_BANK_DIAG,100,{80,180,255},{180,240,255},{120,140,255},{230,255,255}},
  {"SUNBEAM",HJ_BANK_SWEEP,85,{255,180,20},{255,240,100},{255,120,0},{255,255,190}},
  {"MOONLIGHT",HJ_BANK_GLOW,120,{20,30,80},{80,110,220},{150,180,255},{50,70,150}},
};
static constexpr uint8_t HJ_BANK_PATTERN_COUNT =
  sizeof(HJ_BANK_PATTERNS)/sizeof(HJ_BANK_PATTERNS[0]);

static int16_t hjFindBankPatternIndex(const String &name) {
  for(uint8_t i=0;i<HJ_BANK_PATTERN_COUNT;++i)
    if(name == HJ_BANK_PATTERNS[i].name) return (int16_t)i;
  return -1;
}

static bool hjServiceBankPattern(unsigned long now, uint8_t bri) {
  int16_t bankIndex = hjFindBankPatternIndex(patternName);
  if(bankIndex < 0) return false;
  const HjBankPattern &d = HJ_BANK_PATTERNS[(uint8_t)bankIndex];
  if(now-patternLastMs < d.intervalMs) return true;
  patternLastMs=now;

  uint16_t step=patternStep++;
  uint8_t slow=(uint8_t)(step & 0xFF);
  Rgb frame[LED_COUNT];

  if(d.mode == HJ_BANK_FLOW) {
    const Rgb pal[4]={d.p0,d.p1,d.p2,d.p3};
    for(uint8_t i=0;i<LED_COUNT;++i){
      uint8_t phase=(uint8_t)(slow + i*64U);
      uint8_t seg=(phase>>6)&3U;
      uint8_t amt=(uint8_t)((phase&63U)*4U);
      frame[i]=blendRgb(pal[seg],pal[(seg+1U)&3U],amt);
    }
  } else if(d.mode == HJ_BANK_WAVE) {
    const Rgb pal[4]={d.p0,d.p1,d.p2,d.p3};
    for(uint8_t i=0;i<LED_COUNT;++i){
      uint8_t level=triangle8(step + i*45U,180);
      frame[i]=hjDim(pal[i],(uint8_t)(45U + (uint16_t)level*210U/255U));
    }
  } else if(d.mode == HJ_BANK_SWEEP) {
    static const uint8_t path[4]={HJ_TOP_LEFT,HJ_TOP_RIGHT,HJ_SIDE_RIGHT,HJ_SIDE_LEFT};
    hjBlankFrame(frame);
    uint8_t head=path[step&3U];
    uint8_t tail=path[(step+3U)&3U];
    const Rgb pal[4]={d.p0,d.p1,d.p2,d.p3};
    Rgb c=pal[(step>>2)&3U];
    frame[head]=c;
    frame[tail]=hjDim(c,60);
  } else if(d.mode == HJ_BANK_GLOW) {
    uint8_t level=triangle8(step,220);
    Rgb c=blendRgb(d.p0,d.p1,level);
    frame[HJ_TOP_LEFT]=c;
    frame[HJ_TOP_RIGHT]=blendRgb(c,d.p2,50);
    frame[HJ_SIDE_LEFT]=hjDim(blendRgb(c,d.p3,80),150);
    frame[HJ_SIDE_RIGHT]=hjDim(blendRgb(c,d.p3,120),150);
  } else if(d.mode == HJ_BANK_SPARK) {
    const Rgb pal[4]={d.p0,d.p1,d.p2,d.p0};
    for(uint8_t i=0;i<LED_COUNT;++i){
      frame[i]=hjDim(pal[i],120);
      if(random(5)==0) frame[i]=d.p3;
    }
  } else if(d.mode == HJ_BANK_PAIR) {
    bool flip=step&1U;
    frame[HJ_TOP_LEFT]=flip?d.p0:d.p1;
    frame[HJ_TOP_RIGHT]=flip?d.p1:d.p0;
    frame[HJ_SIDE_LEFT]=flip?d.p2:d.p3;
    frame[HJ_SIDE_RIGHT]=flip?d.p3:d.p2;
  } else if(d.mode == HJ_BANK_DIAG) {
    bool flip=step&1U;
    hjBlankFrame(frame);
    if(flip){
      frame[HJ_TOP_LEFT]=d.p0;
      frame[HJ_SIDE_RIGHT]=d.p2;
      frame[HJ_TOP_RIGHT]=hjDim(d.p1,45);
      frame[HJ_SIDE_LEFT]=hjDim(d.p3,45);
    } else {
      frame[HJ_TOP_RIGHT]=d.p1;
      frame[HJ_SIDE_LEFT]=d.p3;
      frame[HJ_TOP_LEFT]=hjDim(d.p0,45);
      frame[HJ_SIDE_RIGHT]=hjDim(d.p2,45);
    }
  } else { // HJ_BANK_RIPPLE
    uint8_t phase=(uint8_t)(step&3U);
    hjBlankFrame(frame);
    if(phase==0){
      frame[HJ_TOP_LEFT]=d.p0; frame[HJ_TOP_RIGHT]=d.p1;
    } else if(phase==1){
      frame[HJ_TOP_LEFT]=hjDim(d.p0,80); frame[HJ_TOP_RIGHT]=hjDim(d.p1,80);
      frame[HJ_SIDE_LEFT]=d.p2; frame[HJ_SIDE_RIGHT]=d.p3;
    } else if(phase==2){
      frame[HJ_SIDE_LEFT]=hjDim(d.p2,80); frame[HJ_SIDE_RIGHT]=hjDim(d.p3,80);
    } else {
      frame[HJ_TOP_LEFT]=hjDim(d.p0,35); frame[HJ_TOP_RIGHT]=hjDim(d.p1,35);
      frame[HJ_SIDE_LEFT]=hjDim(d.p2,35); frame[HJ_SIDE_RIGHT]=hjDim(d.p3,35);
    }
  }

  writeFrame(frame,bri);
  return true;
}

static void servicePattern() {
  if (hjSleepActive) return;
  if (hjAccessoryPaused) return;
  if (hjBirthdayToday()) {
    unsigned long now = millis();
    if (now - patternLastMs < 650UL) return;
    patternLastMs = now;
    uint8_t bri = safeBrightness(brightnessPercent);
    Rgb birthdayFrame[LED_COUNT] = {
      wheel((uint8_t)(patternStep * 11U)),
      wheel((uint8_t)(patternStep * 11U + 64U)),
      wheel((uint8_t)(patternStep * 11U + 128U)),
      wheel((uint8_t)(patternStep * 11U + 192U))
    };
    writeFrame(birthdayFrame, bri);
    patternStep++;
    return;
  }
  // HAPPYJARZ_FOUR_LAMP_PATTERN_ENGINE_V2
  if (patternName == "SOLID") return;
  if (patternName == "OFF") { allOff(); return; }

  unsigned long now = millis();
  uint8_t bri = safeBrightness(brightnessPercent);
  if (bri == 0) { allOff(); return; }

  if (hjServiceBankPattern(now,bri)) return;

  // Base-color brightness effects already operate on all four saved lamp colors.
  if (patternName == "FADE" || patternName == "BREATH") {
    uint16_t interval = patternName == "BREATH" ? 35 : 22;
    if (now-patternLastMs < interval) return; patternLastMs=now;
    uint8_t level = triangle8(patternStep, patternName == "BREATH" ? 260 : 200);
    uint8_t floorPct = patternName == "BREATH" ? 5 : 8;
    uint8_t pct = floorPct + (uint8_t)((uint16_t)level*(100U-floorPct)/255U);
    writeFrame(ledColor,(uint8_t)((uint16_t)pct*bri/100U));
    patternStep++; return;
  }

  if (patternName == "PULSE") {
    if (now-patternLastMs < 16) return; patternLastMs=now;
    uint16_t phase=patternStep%150; uint8_t pct;
    if (phase<30) pct=10+(uint8_t)((uint16_t)phase*90U/29U);
    else if (phase<60) pct=100-(uint8_t)((uint16_t)(phase-30)*90U/29U);
    else pct=10;
    writeFrame(ledColor,(uint8_t)((uint16_t)pct*bri/100U));
    patternStep++; return;
  }

  // Four equally-spaced hues moving around the physical chain.
  if (patternName == "RAINBOW") {
    if (now-patternLastMs < 35) return; patternLastMs=now;
    showFour(
      wheel((uint8_t)patternStep),
      wheel((uint8_t)(patternStep+64)),
      wheel((uint8_t)(patternStep+128)),
      wheel((uint8_t)(patternStep+192)), bri);
    patternStep++; return;
  }

  // All four bulbs intentionally share one hue for a uniform color wash.
  if (patternName == "HUE_FADE") {
    if (now-patternLastMs < 45) return; patternLastMs=now;
    Rgb c=wheel((uint8_t)patternStep);
    showFour(c,c,c,c,bri);
    patternStep++; return;
  }

  // Two opposing hues alternate around the four-lamp ring/line.
  if (patternName == "DUAL_HUE") {
    if (now-patternLastMs < 45) return; patternLastMs=now;
    Rgb a=wheel((uint8_t)patternStep);
    Rgb b=wheel((uint8_t)(patternStep+128));
    showFour(a,b,a,b,bri);
    patternStep++; return;
  }

  if (patternName == "DRIFT") {
    if (now-patternLastMs < 70) return; patternLastMs=now;
    uint8_t t0=triangle8(patternStep,220);
    uint8_t t1=triangle8(patternStep+55,220);
    uint8_t t2=triangle8(patternStep+110,220);
    uint8_t t3=triangle8(patternStep+165,220);
    showFour(
      blendRgb({20,0,90},{0,120,255},t0),
      blendRgb({0,50,120},{130,0,190},t1),
      blendRgb({20,0,90},{0,120,255},t2),
      blendRgb({0,50,120},{130,0,190},t3), bri);
    patternStep++; return;
  }

  if (patternName == "AURORA") {
    if (now-patternLastMs < 55) return; patternLastMs=now;
    uint8_t t0=triangle8(patternStep,240);
    uint8_t t1=triangle8(patternStep+60,240);
    uint8_t t2=triangle8(patternStep+120,240);
    uint8_t t3=triangle8(patternStep+180,240);
    showFour(
      blendRgb({0,180,90},{100,0,220},t0),
      blendRgb({0,80,255},{0,220,120},t1),
      blendRgb({0,220,120},{90,20,230},t2),
      blendRgb({0,70,220},{20,255,140},t3), bri);
    patternStep++; return;
  }

  if (patternName == "OCEAN") {
    if (now-patternLastMs < 65) return; patternLastMs=now;
    uint8_t t0=triangle8(patternStep,180);
    uint8_t t1=triangle8(patternStep+45,180);
    uint8_t t2=triangle8(patternStep+90,180);
    uint8_t t3=triangle8(patternStep+135,180);
    showFour(
      blendRgb({0,20,100},{0,180,255},t0),
      blendRgb({0,120,180},{0,30,130},t1),
      blendRgb({0,35,120},{0,220,210},t2),
      blendRgb({0,100,200},{20,40,150},t3), bri);
    patternStep++; return;
  }

  // Uniform lavender remains intentionally uniform, but all four are explicit.
  if (patternName == "LAVENDER") {
    if (now-patternLastMs < 70) return; patternLastMs=now;
    uint8_t t=triangle8(patternStep,220);
    Rgb c=blendRgb({90,20,150},{230,120,255},t);
    showFour(c,c,c,c,bri);
    patternStep++; return;
  }

  if (patternName == "SUNSET") {
    if (now-patternLastMs < 60) return; patternLastMs=now;
    uint8_t t0=triangle8(patternStep,200);
    uint8_t t1=triangle8(patternStep+50,200);
    uint8_t t2=triangle8(patternStep+100,200);
    uint8_t t3=triangle8(patternStep+150,200);
    showFour(
      blendRgb({255,30,0},{255,130,20},t0),
      blendRgb({255,0,100},{100,0,180},t1),
      blendRgb({255,90,0},{255,20,120},t2),
      blendRgb({160,0,180},{255,100,20},t3), bri);
    patternStep++; return;
  }

  if (patternName == "CHRISTMAS") {
    if (now-patternLastMs < 550) return; patternLastMs=now;
    bool flip=(patternStep++ & 1U);
    Rgb red={255,0,0}, green={0,255,0};
    showFour(flip?red:green, flip?green:red, flip?red:green, flip?green:red, bri);
    return;
  }

  if (patternName == "HALLOWEEN") {
    if (now-patternLastMs < 500) return; patternLastMs=now;
    bool flip=(patternStep++ & 1U);
    Rgb orange={255,70,0}, purple={120,0,255};
    showFour(flip?orange:purple, flip?purple:orange,
             flip?orange:purple, flip?purple:orange, bri);
    return;
  }

  if (patternName == "VALENTINE") {
    if (now-patternLastMs < 45) return; patternLastMs=now;
    uint8_t t0=triangle8(patternStep,180);
    uint8_t t1=triangle8(patternStep+45,180);
    uint8_t t2=triangle8(patternStep+90,180);
    uint8_t t3=triangle8(patternStep+135,180);
    showFour(
      blendRgb({255,0,40},{255,20,160},t0),
      blendRgb({255,40,100},{180,0,70},t1),
      blendRgb({255,20,120},{255,100,160},t2),
      blendRgb({180,0,80},{255,40,60},t3), bri);
    patternStep++; return;
  }

  if (patternName == "EASTER") {
    if (now-patternLastMs < 650) return; patternLastMs=now;
    static const Rgb p[]={{255,120,200},{130,220,255},{180,130,255},{255,220,80},{120,255,170}};
    uint8_t i=patternStep++%5;
    showFour(p[i],p[(i+1)%5],p[(i+2)%5],p[(i+3)%5],bri);
    return;
  }

  if (patternName == "FOURTH") {
    if (now-patternLastMs < 420) return; patternLastMs=now;
    static const Rgb p[]={{255,0,0},{255,255,255},{0,60,255}};
    uint8_t i=patternStep++%3;
    showFour(p[i],p[(i+1)%3],p[(i+2)%3],p[i],bri);
    return;
  }

  if (patternName == "THANKSGIVING") {
    if (now-patternLastMs < 600) return; patternLastMs=now;
    static const Rgb p[]={{255,70,0},{180,30,0},{255,160,0},{100,20,0}};
    uint8_t i=patternStep++%4;
    showFour(p[i],p[(i+1)%4],p[(i+2)%4],p[(i+3)%4],bri);
    return;
  }

  if (patternName == "CANDY") {
    if (now-patternLastMs < 320) return; patternLastMs=now;
    static const Rgb p[]={{255,20,120},{0,220,255},{255,180,0},{120,255,80},{170,40,255}};
    showFour(p[random(5)],p[random(5)],p[random(5)],p[random(5)],bri);
    return;
  }

  if (patternName == "GALAXY") {
    if (now-patternLastMs < 90) return; patternLastMs=now;
    Rgb frame[LED_COUNT];
    for(uint8_t i=0;i<LED_COUNT;++i){
      uint8_t t=triangle8(patternStep + (uint16_t)i*60U,240);
      frame[i]=blendRgb(i&1 ? Rgb{0,20,70}:Rgb{10,0,40},
                        i&1 ? Rgb{255,0,130}:Rgb{100,0,220},t);
      uint16_t spark=(uint16_t)(37U + (uint16_t)i*7U);
      if(((patternStep + i*11U)%spark)==0) frame[i]=i&1 ? Rgb{255,255,255}:Rgb{180,180,255};
    }
    writeFrame(frame,bri);
    patternStep++; return;
  }

  if (patternName == "FIRE") {
    if (now-patternLastMs < 80) return; patternLastMs=now;
    Rgb frame[LED_COUNT];
    for(uint8_t i=0;i<LED_COUNT;++i)
      frame[i]={(uint8_t)random(180,256),(uint8_t)random(10,105),(uint8_t)random(0,8)};
    writeFrame(frame,bri); return;
  }

  if (patternName == "ICE") {
    if (now-patternLastMs < 100) return; patternLastMs=now;
    Rgb frame[LED_COUNT];
    for(uint8_t i=0;i<LED_COUNT;++i)
      frame[i]={(uint8_t)random(20,180),(uint8_t)random(120,256),255};
    writeFrame(frame,bri); return;
  }

  if (patternName == "FOREST") {
    if (now-patternLastMs < 90) return; patternLastMs=now;
    uint8_t t0=triangle8(patternStep,210);
    uint8_t t1=triangle8(patternStep+52,210);
    uint8_t t2=triangle8(patternStep+105,210);
    uint8_t t3=triangle8(patternStep+157,210);
    showFour(
      blendRgb({0,45,5},{40,200,20},t0),
      blendRgb({0,90,40},{100,255,50},t1),
      blendRgb({5,55,0},{80,180,25},t2),
      blendRgb({0,110,30},{130,240,60},t3), bri);
    patternStep++; return;
  }

  if (patternName == "NEON") {
    if (now-patternLastMs < 260) return; patternLastMs=now;
    static const Rgb p[]={{255,0,180},{0,255,220},{160,0,255},{255,80,0},{0,120,255}};
    uint8_t i=patternStep++%5;
    showFour(p[i],p[(i+1)%5],p[(i+2)%5],p[(i+3)%5],bri);
    return;
  }

  if (patternName == "TWINKLE") {
    if (now-patternLastMs < 180) return; patternLastMs=now;
    Rgb frame[LED_COUNT];
    for (uint8_t i=0; i<LED_COUNT; ++i)
      frame[i]=blendRgb(ledColor[i],{255,255,255},(uint8_t)random(30,150));
    uint8_t low=(uint8_t)max(1,(int)bri/3);
    writeFrame(frame,(uint8_t)random((long)low,(long)bri+1L)); return;
  }

  if (patternName == "SPARKLE") {
    if (now-patternLastMs < 110) return; patternLastMs=now;
    Rgb frame[LED_COUNT];
    for (uint8_t i=0; i<LED_COUNT; ++i) {
      frame[i]=ledColor[i];
      if (random(3)==0) frame[i]={255,255,255};
    }
    writeFrame(frame,bri); return;
  }

  if (patternName == "COLOR_SWAP") {
    if (now-patternLastMs < 700) return; patternLastMs=now;
    uint8_t shift=(uint8_t)(patternStep++ & 3U);
    Rgb frame[LED_COUNT];
    for (uint8_t i=0; i<LED_COUNT; ++i)
      frame[i]=ledColor[(i+shift)%LED_COUNT];
    writeFrame(frame,bri); return;
  }

  // One bright head moves through all four physical lamps with two fading tails.
  if (patternName == "COMET") {
    if (now-patternLastMs < 120) return; patternLastMs=now;
    uint8_t head=(uint8_t)(patternStep++ & 3U);
    Rgb frame[LED_COUNT]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
    frame[head]=ledColor[head];
    uint8_t tail1=(uint8_t)((head+3U)&3U);
    uint8_t tail2=(uint8_t)((head+2U)&3U);
    frame[tail1]=blendRgb({0,0,0},ledColor[tail1],110);
    frame[tail2]=blendRgb({0,0,0},ledColor[tail2],40);
    writeFrame(frame,bri); return;
  }

  if (patternName == "FIREFLY") {
    if (now-patternLastMs < 160) return; patternLastMs=now;
    Rgb dark={0,5,0}, glow={160,255,30};
    showFour(random(5)==0?glow:dark,
             random(5)==0?glow:dark,
             random(5)==0?glow:dark,
             random(5)==0?glow:dark,bri);
    return;
  }

  if (patternName == "BUBBLEGUM") {
    if (now-patternLastMs < 55) return; patternLastMs=now;
    uint8_t t0=triangle8(patternStep,170);
    uint8_t t1=triangle8(patternStep+42,170);
    uint8_t t2=triangle8(patternStep+85,170);
    uint8_t t3=triangle8(patternStep+127,170);
    showFour(
      blendRgb({255,30,180},{80,180,255},t0),
      blendRgb({120,40,255},{255,120,200},t1),
      blendRgb({255,70,170},{50,210,255},t2),
      blendRgb({100,60,255},{255,160,210},t3), bri);
    patternStep++; return;
  }

  // Stand-topology chase family. Paths use physical geometry rather than
  // DIN order assumptions: 1=top-left, 2=top-right, 3=side-left, 4=side-right.
  // Stand-topology chase family with a slowly rotating hue.
  // Spatial motion stays fast; color phase advances much more slowly so the
  // pattern feels like a chase, not a rainbow strobe.
  if (patternName == "CHASE_CW") {
    if (now-patternLastMs < 180) return; patternLastMs=now;
    static const uint8_t path[] = {HJ_TOP_LEFT,HJ_TOP_RIGHT,HJ_SIDE_RIGHT,HJ_SIDE_LEFT};
    uint8_t step=(uint8_t)(patternStep++);
    uint8_t head=path[step & 3U];
    Rgb c=wheel((uint8_t)(step*5U));
    Rgb frame[LED_COUNT]; hjBlankFrame(frame);
    frame[head]=c;
    frame[path[(step+3U)&3U]]=hjDim(c,55);
    writeFrame(frame,bri); return;
  }

  if (patternName == "CHASE_CCW") {
    if (now-patternLastMs < 180) return; patternLastMs=now;
    static const uint8_t path[] = {HJ_TOP_LEFT,HJ_SIDE_LEFT,HJ_SIDE_RIGHT,HJ_TOP_RIGHT};
    uint8_t step=(uint8_t)(patternStep++);
    uint8_t head=path[step & 3U];
    Rgb c=wheel((uint8_t)(step*5U));
    Rgb frame[LED_COUNT]; hjBlankFrame(frame);
    frame[head]=c;
    frame[path[(step+3U)&3U]]=hjDim(c,55);
    writeFrame(frame,bri); return;
  }

  if (patternName == "JAR_CHASE") {
    if (now-patternLastMs < 260) return; patternLastMs=now;
    uint8_t step=(uint8_t)(patternStep++);
    bool right=(step & 1U);
    Rgb c=wheel((uint8_t)(step*5U));
    Rgb frame[LED_COUNT]; hjBlankFrame(frame);
    frame[right?HJ_TOP_RIGHT:HJ_TOP_LEFT]=c;
    frame[HJ_SIDE_LEFT]=hjDim(c,35);
    frame[HJ_SIDE_RIGHT]=hjDim(c,35);
    writeFrame(frame,bri); return;
  }

  if (patternName == "SIDE_CHASE") {
    if (now-patternLastMs < 260) return; patternLastMs=now;
    uint8_t step=(uint8_t)(patternStep++);
    bool right=(step & 1U);
    Rgb c=wheel((uint8_t)(step*5U));
    Rgb frame[LED_COUNT]; hjBlankFrame(frame);
    frame[right?HJ_SIDE_RIGHT:HJ_SIDE_LEFT]=c;
    writeFrame(frame,bri); return;
  }

  if (patternName == "SWEEP_LR") {
    if (now-patternLastMs < 300) return; patternLastMs=now;
    uint8_t step=(uint8_t)(patternStep++);
    bool right=(step & 1U);
    Rgb c=wheel((uint8_t)(step*5U));
    Rgb frame[LED_COUNT]; hjBlankFrame(frame);
    if(right){
      frame[HJ_TOP_RIGHT]=c;
      frame[HJ_SIDE_RIGHT]=c;
    }else{
      frame[HJ_TOP_LEFT]=c;
      frame[HJ_SIDE_LEFT]=c;
    }
    writeFrame(frame,bri); return;
  }

  if (patternName == "SWEEP_TS") {
    if (now-patternLastMs < 320) return; patternLastMs=now;
    uint8_t step=(uint8_t)(patternStep++);
    bool sides=(step & 1U);
    Rgb c=wheel((uint8_t)(step*5U));
    Rgb frame[LED_COUNT]; hjBlankFrame(frame);
    if(sides){
      frame[HJ_SIDE_LEFT]=c;
      frame[HJ_SIDE_RIGHT]=c;
    }else{
      frame[HJ_TOP_LEFT]=c;
      frame[HJ_TOP_RIGHT]=c;
    }
    writeFrame(frame,bri); return;
  }

  if (patternName == "DIAGONAL") {
    if (now-patternLastMs < 320) return; patternLastMs=now;
    uint8_t step=(uint8_t)(patternStep++);
    bool flip=(step & 1U);
    Rgb c=wheel((uint8_t)(step*5U));
    Rgb frame[LED_COUNT]; hjBlankFrame(frame);
    if(flip){
      frame[HJ_TOP_LEFT]=c;
      frame[HJ_SIDE_RIGHT]=c;
    }else{
      frame[HJ_TOP_RIGHT]=c;
      frame[HJ_SIDE_LEFT]=c;
    }
    writeFrame(frame,bri); return;
  }

  if (patternName == "PING_PONG") {
    if (now-patternLastMs < 170) return; patternLastMs=now;
    static const uint8_t path[] = {
      HJ_TOP_LEFT,HJ_TOP_RIGHT,HJ_SIDE_RIGHT,HJ_SIDE_LEFT,
      HJ_SIDE_RIGHT,HJ_TOP_RIGHT
    };
    uint8_t step=(uint8_t)(patternStep++);
    uint8_t head=path[step % 6U];
    Rgb c=wheel((uint8_t)(step*5U));
    Rgb frame[LED_COUNT]; hjBlankFrame(frame);
    frame[head]=c;
    writeFrame(frame,bri); return;
  }

  if (patternName == "DUAL_CHASE") {
    if (now-patternLastMs < 250) return; patternLastMs=now;
    uint8_t step=(uint8_t)(patternStep++);
    bool right=(step & 1U);
    Rgb c=wheel((uint8_t)(step*5U));
    Rgb frame[LED_COUNT]; hjBlankFrame(frame);
    frame[right?HJ_TOP_RIGHT:HJ_TOP_LEFT]=c;
    frame[right?HJ_SIDE_RIGHT:HJ_SIDE_LEFT]=c;
    writeFrame(frame,bri); return;
  }

  if (patternName == "OPP_CHASE") {
    if (now-patternLastMs < 250) return; patternLastMs=now;
    uint8_t step=(uint8_t)(patternStep++);
    bool phase=(step & 1U);
    Rgb c=wheel((uint8_t)(step*5U));
    Rgb frame[LED_COUNT]; hjBlankFrame(frame);
    frame[phase?HJ_TOP_RIGHT:HJ_TOP_LEFT]=c;
    frame[phase?HJ_SIDE_LEFT:HJ_SIDE_RIGHT]=c;
    writeFrame(frame,bri); return;
  }

  if (patternName == "JAR_PULSE") {
    if (now-patternLastMs < 28) return; patternLastMs=now;
    uint16_t step=patternStep++;
    uint8_t level=triangle8(step,180);
    uint8_t jarAmt=(uint8_t)(70U + (uint16_t)level*185U/255U);
    Rgb c=wheel((uint8_t)(step/3U));
    Rgb frame[LED_COUNT];
    frame[HJ_TOP_LEFT]=hjDim(c,jarAmt);
    frame[HJ_TOP_RIGHT]=hjDim(c,jarAmt);
    frame[HJ_SIDE_LEFT]=hjDim(c,45);
    frame[HJ_SIDE_RIGHT]=hjDim(c,45);
    writeFrame(frame,bri); return;
  }

  if (patternName == "SIDE_ACCENT") {
    if (now-patternLastMs < 240) return; patternLastMs=now;
    uint8_t step=(uint8_t)(patternStep++);
    bool right=(step & 1U);
    Rgb c=wheel((uint8_t)(step*5U));
    Rgb frame[LED_COUNT];
    frame[HJ_TOP_LEFT]=hjDim(c,135);
    frame[HJ_TOP_RIGHT]=hjDim(c,135);
    frame[HJ_SIDE_LEFT]=right ? hjDim(c,20) : c;
    frame[HJ_SIDE_RIGHT]=right ? c : hjDim(c,20);
    writeFrame(frame,bri); return;
  }

  if (patternName == "RANDOM") {
    if (now-patternLastMs < 300) return; patternLastMs=now;
    showFour(
      {(uint8_t)random(256),(uint8_t)random(256),(uint8_t)random(256)},
      {(uint8_t)random(256),(uint8_t)random(256),(uint8_t)random(256)},
      {(uint8_t)random(256),(uint8_t)random(256),(uint8_t)random(256)},
      {(uint8_t)random(256),(uint8_t)random(256),(uint8_t)random(256)},bri);
    return;
  }
}

static void hjSetLed(uint8_t led,uint8_t r,uint8_t g,uint8_t b){
  patternName="SOLID"; resetPatternEngine();
  if (led < 1 || led > LED_COUNT) return;
  ledColor[led-1]={r,g,b}; showLeds();
}
static void hjSetBrightness(uint8_t p){
  brightnessPercent=constrain(p,0,HJ_MAX_BRIGHTNESS);
  if(patternName=="SOLID")showLeds();
}
static void hjSetPattern(const String &name){
  patternName=name; resetPatternEngine();
  if(name=="OFF")allOff(); else if(name=="SOLID")showLeds();
}

// -----------------------------
// Wi-Fi / NTP
// -----------------------------
static void startTimeSync() {
  configTzTime(tzPosixFor(timezoneName), "pool.ntp.org", "time.nist.gov", "time.google.com");
}

static bool wifiConnecting = false;
static unsigned long wifiConnectStartedMs = 0;
static constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000;

static String wifiSecurityName(wifi_auth_mode_t mode) {
  switch (mode) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    default: return String((int)mode);
  }
}

static void scanWifiNetworks() {
  if (wifiConnecting) {
    WiFi.disconnect(false, false);
    wifiConnecting = false;
    delay(100);
  }
  WiFi.mode(WIFI_STA);
  Serial.println("HJ|WIFI_SCAN|BEGIN");
  int count = WiFi.scanNetworks(false, true);
  if (count < 0) count = 0;
  for (int i = 0; i < count; ++i) {
    String ssid = WiFi.SSID(i);
    if (!ssid.length()) continue;
    Serial.print("HJ|WIFI_SCAN|NET|ssid="); Serial.print(ssid);
    Serial.print("|rssi="); Serial.print(WiFi.RSSI(i));
    Serial.print("|channel="); Serial.print(WiFi.channel(i));
    Serial.print("|security="); Serial.println(wifiSecurityName(WiFi.encryptionType(i)));
  }
  Serial.print("HJ|WIFI_SCAN|END|count="); Serial.println(count);
  WiFi.scanDelete();
}

static void connectWifi() {
  if (!wifiSsid.length()) {
    Serial.println("HJ|ERR|message=wifi ssid is empty");
    return;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("HJ|EVENT|wifi=CONNECTED");
    return;
  }
  if (wifiConnecting && millis() - wifiConnectStartedMs < WIFI_CONNECT_TIMEOUT_MS) {
    Serial.println("HJ|EVENT|wifi=CONNECTING");
    return;
  }
  wifiConnecting = false;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  delay(75);
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  wifiConnecting = true;
  wifiConnectStartedMs = millis();
  Serial.println("HJ|EVENT|wifi=CONNECTING");
}

static void diagnoseWifiFailure() {
  int count = WiFi.scanNetworks(false, true);
  bool found = false;
  if (count > 0) {
    for (int i = 0; i < count; ++i) {
      if (WiFi.SSID(i) == wifiSsid) {
        found = true;
        Serial.print("HJ|WIFI_DIAG|found=1|ssid="); Serial.print(wifiSsid);
        Serial.print("|rssi="); Serial.print(WiFi.RSSI(i));
        Serial.print("|channel="); Serial.print(WiFi.channel(i));
        Serial.print("|security="); Serial.println(wifiSecurityName(WiFi.encryptionType(i)));
        break;
      }
    }
  }
  if (!found) {
    Serial.print("HJ|WIFI_DIAG|found=0|ssid="); Serial.println(wifiSsid);
  }
  WiFi.scanDelete();
}

static bool clockSynced() {
  time_t now = time(nullptr);
  return now > 1700000000;
}

static bool hjValidBirthday(const String &value) {
  if (value.length() != 5 || value.charAt(2) != '-') return false;
  int month = value.substring(0, 2).toInt();
  int day = value.substring(3, 5).toInt();
  static const uint8_t daysInMonth[] = {31,29,31,30,31,30,31,31,30,31,30,31};
  return month >= 1 && month <= 12 && day >= 1 && day <= daysInMonth[month - 1];
}

static bool hjBirthdayToday() {
  if (!hjValidBirthday(ownerBirthday) || !clockSynced()) return false;
  time_t now = time(nullptr);
  time_t minute = now / 60;
  if (minute == hjBirthdayCacheMinute) return hjBirthdayCacheValue;
  hjBirthdayCacheMinute = minute;
  struct tm local;
  if (!getLocalTime(&local, 0)) {
    hjBirthdayCacheValue = false;
    return false;
  }
  char today[6];
  snprintf(today, sizeof(today), "%02d-%02d", local.tm_mon + 1, local.tm_mday);
  hjBirthdayCacheValue = ownerBirthday == String(today);
  if (!hjBirthdayCacheValue && ownerBirthday == "02-29" &&
      local.tm_mon == 2 && local.tm_mday == 1) {
    int year = local.tm_year + 1900;
    bool leap = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
    hjBirthdayCacheValue = !leap;
  }
  return hjBirthdayCacheValue;
}

static int hjBirthdayCountdownDays() {
  if (!hjValidBirthday(ownerBirthday) || !clockSynced()) return -1;
  struct tm local;
  if (!getLocalTime(&local, 0)) return -1;

  int month = ownerBirthday.substring(0, 2).toInt();
  int day = ownerBirthday.substring(3, 5).toInt();
  struct tm target = {};
  target.tm_year = local.tm_year;
  target.tm_mon = month - 1;
  target.tm_mday = day;
  target.tm_hour = 12;
  target.tm_isdst = -1;
  if (mktime(&target) < 0) return -1; // Feb 29 normalizes to Mar 1 in common years.

  int days = target.tm_yday - local.tm_yday;
  if (days < 0) {
    int year = local.tm_year + 1900;
    bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    days += leap ? 366 : 365;
  }
  return days;
}

static void printWifiStatus() {
  String state = WiFi.status()==WL_CONNECTED ? "CONNECTED" : "DISCONNECTED";
  Serial.print("HJ|WIFI|state="); Serial.print(state);
  Serial.print("|ssid="); Serial.print(wifiSsid);
  Serial.print("|ip="); Serial.print(WiFi.status()==WL_CONNECTED ? WiFi.localIP().toString() : "");
  Serial.print("|rssi="); Serial.println(WiFi.status()==WL_CONNECTED ? String(WiFi.RSSI()) : "0");
}

static void printTimeStatus() {
  Serial.print("HJ|TIME|synced="); Serial.print(clockSynced()?1:0);
  Serial.print("|timezone="); Serial.print(timezoneName);
  Serial.print("|local=");
  if (!clockSynced()) { Serial.println(""); return; }
  struct tm t; if (!getLocalTime(&t,50)) { Serial.println(""); return; }
  char buf[32]; strftime(buf,sizeof(buf),"%Y-%m-%dT%H:%M:%S",&t); Serial.println(buf);
}

// -----------------------------
// Alarm / timer services
// -----------------------------
static int lastAlarmYday = -1;
static int lastAlarmMinute = -1;
static void fireAlarmEvent() {
  hjWakeFromSleep("ALARM");
  Serial.println("HJ|EVENT|alarm=TRIGGERED");
  // Gentle visible alarm using existing lights; future sound layer can hook here.
  ledColor[0] = {255,120,20}; ledColor[1] = {255,40,100};
  ledColor[2] = {255,120,20}; ledColor[3] = {255,40,100};
  patternName = "PULSE"; resetPatternEngine();
}

static void serviceAlarm() {
  if (!alarmEnabled || !clockSynced()) return;
  struct tm t; if (!getLocalTime(&t,5)) return;
  if (t.tm_hour == alarmHour && t.tm_min == alarmMinute) {
    if (lastAlarmYday != t.tm_yday || lastAlarmMinute != t.tm_min) {
      lastAlarmYday = t.tm_yday; lastAlarmMinute = t.tm_min; fireAlarmEvent();
    }
  }
}

static void serviceTimer() {
  if (!timerEnabled) return;
  uint64_t elapsed = (uint64_t)(millis() - timerStartedMs);
  uint64_t target = (uint64_t)timerMinutes * 60000ULL;
  if (elapsed >= target) {
    timerEnabled = false;
    patternName = "OFF";
    allOff();
    Serial.println("HJ|EVENT|timer=EXPIRED");
  }
}

static uint32_t timerRemainingSeconds() {
  if (!timerEnabled) return 0;
  uint64_t target=(uint64_t)timerMinutes*60000ULL;
  uint64_t elapsed=(uint64_t)(millis()-timerStartedMs);
  return elapsed>=target?0:(uint32_t)((target-elapsed)/1000ULL);
}


// -----------------------------
// OLED dashboard + menu UI
// -----------------------------
static constexpr uint8_t OLED_SDA_PIN = 8;
static constexpr uint8_t OLED_SCL_PIN = 6;
static constexpr uint8_t OLED_I2C_ADDR = 0x3C;
static constexpr bool HJ_OLED_SH1106 = false;

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C oledSsd1306(U8G2_R0, U8X8_PIN_NONE);
static U8G2_SH1106_128X64_NONAME_F_HW_I2C oledSh1106(U8G2_R0, U8X8_PIN_NONE);
static U8G2 *oled = nullptr;
static bool oledReady = false;
static bool oledDirty = true;
static unsigned long oledLastDrawMs = 0;


// HAPPY JARZ Arcade -> known-good U8g2 OLED adapter.
// Host sync is staged before the release/arcade layer, so route every arcade
// present through the same mirrored OLED presentation helper.
static void hjOledPresent();

static void arcadeClear() {
  if (oled) oled->clearBuffer();
}

static void arcadePixel(int16_t x, int16_t y, bool on) {
  if (!oled) return;
  if (!on) oled->setDrawColor(0);
  oled->drawPixel(x, y);
  if (!on) oled->setDrawColor(1);
}

static void arcadeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool on) {
  if (!oled) return;
  if (!on) oled->setDrawColor(0);
  oled->drawLine(x0, y0, x1, y1);
  if (!on) oled->setDrawColor(1);
}

static void arcadeRect(int16_t x, int16_t y, int16_t w, int16_t h, bool fill, bool on) {
  if (!oled) return;
  if (!on) oled->setDrawColor(0);
  if (fill) oled->drawBox(x, y, w, h);
  else oled->drawFrame(x, y, w, h);
  if (!on) oled->setDrawColor(1);
}

static void arcadeText(int16_t x, int16_t y, const char *text, uint8_t size) {
  if (!oled || !text) return;
  oled->setFont(size > 1 ? u8g2_font_7x14_tf : u8g2_font_5x8_tf);
  oled->drawStr(x, y + (size > 1 ? 13 : 7), text);
}

static void arcadePresent() {
  if (oled) hjOledPresent();
}

static const HjArcadeDisplay ARCADE_DISPLAY = {
  arcadeClear,
  arcadePixel,
  arcadeLine,
  arcadeRect,
  arcadeText,
  arcadePresent
};

// Host OLED mirror: actual 128x64 U8g2 framebuffer, Base64 encoded.
// 1024 raw bytes -> 1368 Base64 chars. Frames are change-driven and <= 4 Hz.
static bool hjOledStream = false;
static bool hjOledHaveFrame = false;
static uint8_t hjOledLastFrame[1024];
static uint32_t hjOledSeq = 0;
static unsigned long hjOledLastMirrorMs = 0;

static void hjPrintBase64(const uint8_t *data, size_t len) {
  static const char table[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t i = 0;
  while (i + 2 < len) {
    uint32_t v = ((uint32_t)data[i] << 16) |
                 ((uint32_t)data[i+1] << 8) |
                 (uint32_t)data[i+2];
    Serial.write(table[(v >> 18) & 63]);
    Serial.write(table[(v >> 12) & 63]);
    Serial.write(table[(v >> 6) & 63]);
    Serial.write(table[v & 63]);
    i += 3;
  }
  if (i < len) {
    uint32_t v = (uint32_t)data[i] << 16;
    bool two = (i + 1 < len);
    if (two) v |= (uint32_t)data[i+1] << 8;
    Serial.write(table[(v >> 18) & 63]);
    Serial.write(table[(v >> 12) & 63]);
    Serial.write(two ? table[(v >> 6) & 63] : '=');
    Serial.write('=');
  }
}

static void hjEmitOledFrame(bool force) {
  if (!oledReady || !oled || !Serial) return;
  if (!force && !hjOledStream) return;

  unsigned long now = millis();
  if (!force && now - hjOledLastMirrorMs < 250) return;

  uint8_t *buf = oled->getBufferPtr();
  if (!buf) return;

  bool changed = !hjOledHaveFrame ||
                 memcmp(hjOledLastFrame, buf, sizeof(hjOledLastFrame)) != 0;
  if (!force && !changed) return;

  hjOledLastMirrorMs = now;
  memcpy(hjOledLastFrame, buf, sizeof(hjOledLastFrame));
  hjOledHaveFrame = true;

  Serial.print("HJ|OLED|seq=");
  Serial.print(++hjOledSeq);
  Serial.print("|codec=b64v1|bytes=1024|data=");
  hjPrintBase64(buf, 1024);
  Serial.println();
}

static void hjOledPresent() {
  if (hjSleepActive) return;
  if (!oled) return;
  if (hjLowBatteryWarning) hjRenderLowBatteryWarning();
  oled->sendBuffer();
  hjEmitOledFrame(false);
}

enum UiScreen : uint8_t {
  UI_HOME=0,
  UI_MAIN_MENU,
  UI_CLOCK,
  UI_LIGHTS,
  UI_GAMES,
  UI_SETTINGS,
  UI_SYSTEM,
  UI_POWER,
  UI_INFO,
  UI_PROFILE
};
static UiScreen uiScreen = UI_HOME;
static uint8_t uiCursor = 0;
static uint8_t uiScroll = 0;
static uint8_t infoPage = 0;
static UiScreen clockReturnScreen = UI_MAIN_MENU;
static uint8_t profileMenuCursor = 0;
static uint8_t profileEditor = 0; // 0 menu, 1 name, 2 birthday
static String profileEditName;
static String profileEditBirthday;
static uint8_t profileEditChar = 0;


// -----------------------------
// Standalone SETTINGS editors
// -----------------------------
static uint8_t settingsCursor = 0;      // 0 alarm, 1 timer
static uint8_t settingsEditor = 0;      // 0 menu, 1 alarm, 2 timer
static uint8_t settingsField = 0;

static bool editAlarmEnabled = false;
static uint8_t editAlarmHour = 7;
static uint8_t editAlarmMinute = 0;
static bool editTimerEnabled = false;
static uint16_t editTimerMinutes = 10;

static void settingsOpenEditor(uint8_t which) {
  settingsEditor = which;
  settingsField = 0;
  if (which == 1) {
    editAlarmEnabled = alarmEnabled;
    editAlarmHour = alarmHour;
    editAlarmMinute = alarmMinute;
  } else if (which == 2) {
    editTimerEnabled = timerEnabled;
    editTimerMinutes = timerMinutes;
    if (editTimerMinutes < 1) editTimerMinutes = 1;
  }
  oledDirty = true;
}

static void settingsCancelEditor() {
  settingsEditor = 0;
  settingsField = 0;
  oledDirty = true;
}

static void settingsAdjust(int delta) {
  if (settingsEditor == 1) {
    if (settingsField == 0) {
      editAlarmEnabled = !editAlarmEnabled;
    } else if (settingsField == 1) {
      int v = (int)editAlarmHour + delta;
      if (v < 0) v = 23;
      if (v > 23) v = 0;
      editAlarmHour = (uint8_t)v;
    } else {
      int v = (int)editAlarmMinute + delta;
      if (v < 0) v = 59;
      if (v > 59) v = 0;
      editAlarmMinute = (uint8_t)v;
    }
  } else if (settingsEditor == 2) {
    if (settingsField == 0) {
      editTimerEnabled = !editTimerEnabled;
    } else {
      int v = (int)editTimerMinutes + delta;
      if (v < 1) v = 240;
      if (v > 240) v = 1;
      editTimerMinutes = (uint16_t)v;
    }
  }
  oledDirty = true;
}

static void settingsSaveEditor() {
  if (settingsEditor == 1) {
    alarmEnabled = editAlarmEnabled;
    alarmHour = editAlarmHour;
    alarmMinute = editAlarmMinute;
  } else if (settingsEditor == 2) {
    timerEnabled = editTimerEnabled;
    timerMinutes = editTimerMinutes;
    if (timerEnabled) timerStartedMs = millis();
  }
  persistSettings();
  settingsEditor = 0;
  settingsField = 0;
  oledDirty = true;
}


// -----------------------------
// Standalone manual clock editor
// -----------------------------
static bool manualClockEditing = false;
static uint8_t manualClockField = 0; // 0 month, 1 day, 2 year, 3 hour, 4 minute
static int manualClockMonth = 1;
static int manualClockDay = 1;
static int manualClockYear = 2026;
static int manualClockHour = 12;
static int manualClockMinute = 0;

static int manualClockDaysInMonth(int year, int month) {
  static const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month == 2) {
    bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    return leap ? 29 : 28;
  }
  if (month < 1 || month > 12) return 31;
  return days[month-1];
}

static void manualClockClampDay() {
  int maxDay = manualClockDaysInMonth(manualClockYear, manualClockMonth);
  if (manualClockDay < 1) manualClockDay = maxDay;
  if (manualClockDay > maxDay) manualClockDay = 1;
}

static void manualClockBeginEdit() {
  applyConfiguredTimezone();
  struct tm t;
  if (getLocalTime(&t, 10) && t.tm_year + 1900 >= 2020) {
    manualClockMonth = t.tm_mon + 1;
    manualClockDay = t.tm_mday;
    manualClockYear = t.tm_year + 1900;
    manualClockHour = t.tm_hour;
    manualClockMinute = t.tm_min;
  } else {
    manualClockMonth = 1;
    manualClockDay = 1;
    manualClockYear = 2026;
    manualClockHour = 12;
    manualClockMinute = 0;
  }
  manualClockField = 0;
  manualClockEditing = true;
  oledDirty = true;
}

static const char *manualClockFieldName() {
  static const char *names[] = {"MONTH", "DAY", "YEAR", "HOUR", "MINUTE"};
  return names[manualClockField % 5];
}

static void manualClockAdjust(int delta) {
  switch (manualClockField) {
    case 0:
      manualClockMonth += delta;
      if (manualClockMonth < 1) manualClockMonth = 12;
      if (manualClockMonth > 12) manualClockMonth = 1;
      manualClockClampDay();
      break;
    case 1: {
      int maxDay = manualClockDaysInMonth(manualClockYear, manualClockMonth);
      manualClockDay += delta;
      if (manualClockDay < 1) manualClockDay = maxDay;
      if (manualClockDay > maxDay) manualClockDay = 1;
      break;
    }
    case 2:
      manualClockYear += delta;
      if (manualClockYear < 2020) manualClockYear = 2099;
      if (manualClockYear > 2099) manualClockYear = 2020;
      manualClockClampDay();
      break;
    case 3:
      manualClockHour += delta;
      if (manualClockHour < 0) manualClockHour = 23;
      if (manualClockHour > 23) manualClockHour = 0;
      break;
    default:
      manualClockMinute += delta;
      if (manualClockMinute < 0) manualClockMinute = 59;
      if (manualClockMinute > 59) manualClockMinute = 0;
      break;
  }
  oledDirty = true;
}

static bool manualClockSave() {
  struct tm t = {};
  t.tm_year = manualClockYear - 1900;
  t.tm_mon = manualClockMonth - 1;
  t.tm_mday = manualClockDay;
  t.tm_hour = manualClockHour;
  t.tm_min = manualClockMinute;
  t.tm_sec = 0;
  t.tm_isdst = -1;
  applyConfiguredTimezone();
  time_t epoch = mktime(&t);
  if (epoch < 1700000000) return false;
  struct timeval tv;
  tv.tv_sec = epoch;
  tv.tv_usec = 0;
  if (settimeofday(&tv, nullptr) != 0) return false;
  manualClockEditing = false;
  oledDirty = true;
  Serial.println("HJ|EVENT|clock=MANUAL_SET");
  return true;
}

static const char HJ_PROFILE_CHARS[] =
    " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'-";

static int profileCharIndex(char c) {
  for (int i = 0; HJ_PROFILE_CHARS[i]; ++i) {
    if (HJ_PROFILE_CHARS[i] == c) return i;
  }
  return 0;
}

static void profileBeginNameEdit() {
  profileEditName = ownerName;
  if (profileEditName.length() > 16) profileEditName = profileEditName.substring(0, 16);
  for (size_t i = 0; i < profileEditName.length(); ++i) {
    if (profileCharIndex(profileEditName.charAt(i)) == 0 &&
        profileEditName.charAt(i) != ' ') profileEditName.setCharAt(i, ' ');
  }
  if (!profileEditName.length()) profileEditName = " ";
  profileEditChar = 0;
  profileEditor = 1;
  oledDirty = true;
}

static void profileCycleNameChar(int delta) {
  if (!profileEditName.length() || profileEditChar >= profileEditName.length()) return;
  int count = sizeof(HJ_PROFILE_CHARS) - 1;
  int index = profileCharIndex(profileEditName.charAt(profileEditChar));
  index = (index + delta + count) % count;
  profileEditName.setCharAt(profileEditChar, HJ_PROFILE_CHARS[index]);
  oledDirty = true;
}

static void profileBeginBirthdayEdit() {
  profileEditBirthday = hjValidBirthday(ownerBirthday) ? ownerBirthday : "01-01";
  profileEditChar = 0;
  profileEditor = 2;
  oledDirty = true;
}

static void profileAdjustBirthday(int delta) {
  static const uint8_t digitPositions[] = {0, 1, 3, 4};
  uint8_t pos = digitPositions[profileEditChar % 4];
  int digit = profileEditBirthday.charAt(pos) - '0';
  digit = (digit + delta + 10) % 10;
  profileEditBirthday.setCharAt(pos, (char)('0' + digit));
  oledDirty = true;
}

static void profileSaveName() {
  profileEditName.trim();
  ownerName = profileEditName.substring(0, 32);
  persistSettings();
  profileEditor = 0;
  oledDirty = true;
  Serial.println("HJ|EVENT|profile=NAME_SAVED");
}

static bool profileSaveBirthday() {
  if (!hjValidBirthday(profileEditBirthday)) {
    Serial.println("HJ|ERR|message=birthday must be a valid MM-DD date");
    return false;
  }
  ownerBirthday = profileEditBirthday;
  hjBirthdayCacheMinute = (time_t)-1;
  persistSettings();
  profileEditor = 0;
  hjStartupCountdownPending = (esp_random() % 3U) == 0;
  oledDirty = true;
  Serial.println("HJ|EVENT|profile=BIRTHDAY_SAVED");
  return true;
}

static void profileClear() {
  ownerName = "";
  ownerBirthday = "--";
  hjBirthdayCacheMinute = (time_t)-1;
  hjStartupCountdownPending = false;
  persistSettings();
  oledDirty = true;
  Serial.println("HJ|EVENT|profile=CLEARED");
}

static void uiClockBack() {
  if (clockReturnScreen == UI_PROFILE) {
    uiScreen = UI_PROFILE;
    profileEditor = 0;
    oledDirty = true;
  } else {
    uiScreen = UI_MAIN_MENU;
    uiCursor = 0;
    uiScroll = 0;
    oledDirty = true;
  }
}

static constexpr unsigned long HJ_SCREENSAVER_IDLE_MS = 30000UL;
static unsigned long hjLastInteractionMs = 0;
static bool hjScreensaverActive = false;
static uint8_t hjScreensaverMode = 0;
static constexpr uint8_t HJ_SCREENSAVER_COUNT = 8;
static unsigned long hjSaverLastFrameMs = 0;
static int hjMarqueeX = 128;
static uint8_t hjSayingIndex = 0;
static uint16_t hjArtStep = 0;
static uint8_t hjSpiralSpeed = 1;
static uint8_t hjTrippySpeed = 1;
static uint32_t hjSpiralSeed = 0x8255A11FUL;
static uint32_t hjTrippySeed = 0x51A7C0DEUL;
static constexpr uint8_t HJ_PARTICLE_MAX = 18;
static constexpr uint8_t HJ_ATTRACTOR_MAX = 3;
struct HjParticle { float x,y,vx,vy; };
struct HjAttractor { float x,y,vx,vy,strength; };
static HjParticle hjParticles[HJ_PARTICLE_MAX];
static HjAttractor hjAttractors[HJ_ATTRACTOR_MAX];
static uint8_t hjParticleCount = 12;
static uint8_t hjAttractorCount = 1;
static uint8_t hjParticleSpeed = 1;
static uint8_t hjVisualSpeed = 1;
static uint32_t hjVisualSeed = 0xB1008255UL;
static uint32_t hjParticleSeed = 0xC05A1055UL;
static uint32_t hjWellWanderState = 0x13579BDFUL;
static float hjParticleDamping = 0.985f;
static float hjParticleGravity = 0.055f;
static float hjParticleOrbit = 0.018f;
static float hjParticleLinkDistance = 18.0f;
static bool hjParticleLinks = true;
static bool hjParticleWrap = true;
static bool hjParticleRepel = false;


// Slow random-walk state. These values move continuously instead of tracing a
// closed periodic path, so each generated universe keeps evolving.
static float hjSpiralCxDrift = 0.0f, hjSpiralCyDrift = 0.0f;
static float hjSpiralAngleDrift = 0.0f, hjSpiralSquashDrift = 0.0f;
static float hjSpiralWobbleDrift = 0.0f;
static float hjTrippyFreqDrift1 = 0.0f, hjTrippyFreqDrift2 = 0.0f;
static float hjTrippyAmpDrift1 = 0.0f, hjTrippyAmpDrift2 = 0.0f;
static float hjTrippyPhaseDrift = 0.0f;
static float hjSpiralCxTarget = 0.0f, hjSpiralCyTarget = 0.0f;
static float hjSpiralAngleTarget = 0.0f, hjSpiralSquashTarget = 0.0f;
static float hjSpiralWobbleTarget = 0.0f;
static float hjTrippyFreqTarget1 = 0.0f, hjTrippyFreqTarget2 = 0.0f;
static float hjTrippyAmpTarget1 = 0.0f, hjTrippyAmpTarget2 = 0.0f;
static float hjTrippyPhaseTarget = 0.0f;
static uint32_t hjSpiralWanderState = 0x13572468UL;
static uint32_t hjTrippyWanderState = 0x24681357UL;

static uint32_t hjMix32(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352dUL;
  x ^= x >> 15; x *= 0x846ca68bUL;
  x ^= x >> 16;
  return x;
}

static uint32_t hjBoardEntropy() {
  uint32_t s = millis() ^ (micros() << 1);
  s ^= (uint32_t)((int)(temperatureRead() * 100.0f)) * 0x9E3779B9UL;
  if (WiFi.status() == WL_CONNECTED) s ^= (uint32_t)(WiFi.RSSI() + 128) * 0x85EBCA6BUL;
  s ^= (uint32_t)touchRead(4)  << 1;
  s ^= (uint32_t)touchRead(5)  << 5;
  s ^= (uint32_t)touchRead(9)  << 9;
  s ^= (uint32_t)touchRead(10) << 13;
  s ^= (uint32_t)touchRead(1)  << 17;
  s ^= (uint32_t)touchRead(2)  << 21;
  s ^= (uint32_t)brightnessPercent << 24;
  s ^= (uint32_t)ledColor[0].r << 8;
  s ^= (uint32_t)ledColor[0].g << 16;
  s ^= (uint32_t)ledColor[0].b << 24;
  s ^= (uint32_t)ledColor[1].r;
  s ^= (uint32_t)random(0x7fffffff);
  return hjMix32(s);
}

static uint32_t hjSeedNext(uint32_t &state) {
  state = hjMix32(state + 0x9E3779B9UL);
  return state;
}

static float hjSeedFloat(uint32_t &state, float lo, float hi) {
  uint32_t v = hjSeedNext(state);
  float t = (float)(v & 0xFFFFU) / 65535.0f;
  return lo + (hi - lo) * t;
}

static int hjSeedInt(uint32_t &state, int lo, int hi) {
  if (hi <= lo) return lo;
  return lo + (int)(hjSeedNext(state) % (uint32_t)(hi - lo + 1));
}

static float hjClampF(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static float hjEaseF(float current, float target, float amount) {
  return current + (target - current) * amount;
}

static float hjWrappedPhaseDelta(float current, float target) {
  float d = target - current;
  while (d > 3.14159f) d -= 6.28318f;
  while (d < -3.14159f) d += 6.28318f;
  return d;
}

static void hjWanderSpiral() {
  // A new destination is chosen occasionally, but the visible values move
  // toward it every frame. This removes the old ~3 second visual snap.
  if ((hjArtStep % 53U) == 0U) {
    hjSpiralWanderState ^= hjBoardEntropy();
    hjSpiralCxTarget     = hjClampF(hjSpiralCxTarget     + hjSeedFloat(hjSpiralWanderState,-3.5f, 3.5f), -12.0f, 12.0f);
    hjSpiralCyTarget     = hjClampF(hjSpiralCyTarget     + hjSeedFloat(hjSpiralWanderState,-2.5f, 2.5f),  -8.0f,  8.0f);
    hjSpiralAngleTarget  = hjClampF(hjSpiralAngleTarget  + hjSeedFloat(hjSpiralWanderState,-0.045f,0.045f),-0.16f, 0.16f);
    hjSpiralSquashTarget = hjClampF(hjSpiralSquashTarget + hjSeedFloat(hjSpiralWanderState,-0.070f,0.070f),-0.24f, 0.24f);
    hjSpiralWobbleTarget = hjClampF(hjSpiralWobbleTarget + hjSeedFloat(hjSpiralWanderState,-0.035f,0.035f),-0.11f, 0.11f);
  }

  hjSpiralCxDrift     = hjEaseF(hjSpiralCxDrift,     hjSpiralCxTarget,     0.018f);
  hjSpiralCyDrift     = hjEaseF(hjSpiralCyDrift,     hjSpiralCyTarget,     0.018f);
  hjSpiralAngleDrift  = hjEaseF(hjSpiralAngleDrift,  hjSpiralAngleTarget,  0.010f);
  hjSpiralSquashDrift = hjEaseF(hjSpiralSquashDrift, hjSpiralSquashTarget, 0.012f);
  hjSpiralWobbleDrift = hjEaseF(hjSpiralWobbleDrift, hjSpiralWobbleTarget, 0.012f);
}

static void hjWanderTrippy() {
  if ((hjArtStep % 59U) == 0U) {
    hjTrippyWanderState ^= hjBoardEntropy();
    hjTrippyFreqTarget1 = hjClampF(hjTrippyFreqTarget1 + hjSeedFloat(hjTrippyWanderState,-0.010f,0.010f),-0.030f,0.030f);
    hjTrippyFreqTarget2 = hjClampF(hjTrippyFreqTarget2 + hjSeedFloat(hjTrippyWanderState,-0.009f,0.009f),-0.025f,0.025f);
    hjTrippyAmpTarget1  = hjClampF(hjTrippyAmpTarget1  + hjSeedFloat(hjTrippyWanderState,-3.0f,3.0f),-9.0f,9.0f);
    hjTrippyAmpTarget2  = hjClampF(hjTrippyAmpTarget2  + hjSeedFloat(hjTrippyWanderState,-2.4f,2.4f),-7.0f,7.0f);
    hjTrippyPhaseTarget += hjSeedFloat(hjTrippyWanderState,-0.60f,0.60f);
    while (hjTrippyPhaseTarget > 6.28318f) hjTrippyPhaseTarget -= 6.28318f;
    while (hjTrippyPhaseTarget < -6.28318f) hjTrippyPhaseTarget += 6.28318f;
  }

  hjTrippyFreqDrift1 = hjEaseF(hjTrippyFreqDrift1, hjTrippyFreqTarget1, 0.010f);
  hjTrippyFreqDrift2 = hjEaseF(hjTrippyFreqDrift2, hjTrippyFreqTarget2, 0.010f);
  hjTrippyAmpDrift1  = hjEaseF(hjTrippyAmpDrift1,  hjTrippyAmpTarget1,  0.015f);
  hjTrippyAmpDrift2  = hjEaseF(hjTrippyAmpDrift2,  hjTrippyAmpTarget2,  0.015f);
  hjTrippyPhaseDrift += hjWrappedPhaseDelta(hjTrippyPhaseDrift, hjTrippyPhaseTarget) * 0.012f;
}

static void hjReseedSpiral() {
  hjSpiralSeed = hjBoardEntropy() ^ 0x53504952UL;
  hjSpiralWanderState = hjSpiralSeed ^ 0xA5A55A5AUL;
  hjSpiralCxDrift=hjSpiralCyDrift=0.0f;
  hjSpiralAngleDrift=hjSpiralSquashDrift=hjSpiralWobbleDrift=0.0f;
  hjSpiralCxTarget=hjSpiralCyTarget=0.0f;
  hjSpiralAngleTarget=hjSpiralSquashTarget=hjSpiralWobbleTarget=0.0f;
  hjArtStep = 0;
}

static void hjReseedTrippy() {
  hjTrippySeed = hjBoardEntropy() ^ 0x54524950UL;
  hjTrippyWanderState = hjTrippySeed ^ 0x5AA5A55AUL;
  hjTrippyFreqDrift1=hjTrippyFreqDrift2=0.0f;
  hjTrippyAmpDrift1=hjTrippyAmpDrift2=0.0f;
  hjTrippyPhaseDrift=0.0f;
  hjTrippyFreqTarget1=hjTrippyFreqTarget2=0.0f;
  hjTrippyAmpTarget1=hjTrippyAmpTarget2=0.0f;
  hjTrippyPhaseTarget=0.0f;
  hjArtStep = 0;
}

static void hjReseedParticles();

static const char *hjSaverModeName() {
  if (hjScreensaverMode == 0) return "SAYINGS";
  if (hjScreensaverMode == 1) return "SPIRAL";
  if (hjScreensaverMode == 2) return "TRIPPY";
  if (hjScreensaverMode == 3) return "PARTICLES";
  if (hjScreensaverMode == 4) return "BLOOM";
  if (hjScreensaverMode == 5) return "BREATHE";
  if (hjScreensaverMode == 6) return "GLITTER";
  return "ART";
}

static void hjSaverReseedCurrent() {
  if (hjScreensaverMode == 1) hjReseedSpiral();
  else if (hjScreensaverMode == 2) hjReseedTrippy();
  else if (hjScreensaverMode == 3) hjReseedParticles();
  else if (hjScreensaverMode >= 4) hjReseedVisualSaver();
}

static void hjSaverPrepareCurrent() {
  hjArtStep = 0;
  if (hjScreensaverMode == 0) {
    hjPickNextSaying();
    hjMarqueeX = 128;
  } else if (hjScreensaverMode == 7) {
    hjArtEnterSaver();
  } else {
    hjSaverReseedCurrent();
  }
  oledDirty = true;
}

static void hjSaverSetMode(uint8_t mode, bool activate) {
  hjScreensaverMode = mode % HJ_SCREENSAVER_COUNT;
  if (activate) hjScreensaverActive = true;
  hjSaverPrepareCurrent();
}

static void hjPrintSaverStatus() {
  Serial.print("HJ|SAVER|active="); Serial.print(hjScreensaverActive ? 1 : 0);
  Serial.print("|mode="); Serial.print(hjSaverModeName());
  Serial.print("|idle_ms="); Serial.print(HJ_SCREENSAVER_IDLE_MS);
  Serial.print("|spiral_speed="); Serial.print(hjSpiralSpeed);
  Serial.print("|trippy_speed="); Serial.print(hjTrippySpeed);
  Serial.print("|particle_speed="); Serial.print(hjParticleSpeed);
  Serial.print("|visual_speed="); Serial.print(hjVisualSpeed);
  Serial.print("|particles="); Serial.print(hjParticleCount);
  Serial.print("|attractors="); Serial.print(hjAttractorCount);
  Serial.print("|links="); Serial.print(hjParticleLinks ? 1 : 0);
  Serial.print("|wrap="); Serial.print(hjParticleWrap ? 1 : 0);
  Serial.print("|repel="); Serial.println(hjParticleRepel ? 1 : 0);
}



static float hjWellNoiseSigned() {
  hjWellWanderState = hjMix32(hjWellWanderState + 0x9E3779B9UL + micros());
  return ((float)(hjWellWanderState & 0xFFFFU) / 32767.5f) - 1.0f;
}

static void hjReseedParticles() {
  hjParticleSeed = hjBoardEntropy() ^ 0x50415254UL;
  hjWellWanderState = hjMix32(hjParticleSeed ^ micros() ^ 0xA771AC7FUL);
  uint32_t g = hjParticleSeed;

  hjParticleCount = (uint8_t)hjSeedInt(g, 7, HJ_PARTICLE_MAX);
  hjAttractorCount = (uint8_t)hjSeedInt(g, 1, HJ_ATTRACTOR_MAX);
  hjParticleDamping = hjSeedFloat(g, 0.965f, 0.997f);
  hjParticleGravity = hjSeedFloat(g, 0.018f, 0.105f);
  hjParticleOrbit = hjSeedFloat(g, -0.055f, 0.055f);
  hjParticleLinkDistance = hjSeedFloat(g, 9.0f, 29.0f);
  hjParticleLinks = hjSeedInt(g,0,3) != 0;
  hjParticleWrap = hjSeedInt(g,0,1) != 0;
  hjParticleRepel = hjSeedInt(g,0,4) == 0;

  for (uint8_t i=0; i<hjAttractorCount; ++i) {
    hjAttractors[i].x = hjSeedFloat(g, 24.0f, 104.0f);
    hjAttractors[i].y = hjSeedFloat(g, 13.0f, 51.0f);
    hjAttractors[i].vx = hjSeedFloat(g, -0.17f, 0.17f);
    hjAttractors[i].vy = hjSeedFloat(g, -0.13f, 0.13f);
    hjAttractors[i].strength = hjSeedFloat(g, 0.65f, 1.55f);
    if (hjSeedInt(g,0,5)==0) hjAttractors[i].strength *= -1.0f;
  }

  for (uint8_t i=0; i<hjParticleCount; ++i) {
    hjParticles[i].x = hjSeedFloat(g, 2.0f, 125.0f);
    hjParticles[i].y = hjSeedFloat(g, 2.0f, 61.0f);
    hjParticles[i].vx = hjSeedFloat(g, -0.72f, 0.72f);
    hjParticles[i].vy = hjSeedFloat(g, -0.54f, 0.54f);
  }
  hjArtStep = 0;
}

static void hjWanderAttractors() {
  for (uint8_t a=0; a<hjAttractorCount; ++a) {
    // Small non-periodic velocity nudges. Each well has its own slightly
    // different cadence so the group does not phase-lock into one loop.
    uint16_t cadence = (uint16_t)(11U + a*7U + (hjParticleSeed >> (a*3U) & 7U));
    if ((hjArtStep % cadence) == (uint16_t)(a % cadence)) {
      hjAttractors[a].vx += hjWellNoiseSigned() * (0.026f + 0.008f*a);
      hjAttractors[a].vy += hjWellNoiseSigned() * (0.020f + 0.006f*a);
    }

    // Very occasional stronger kick breaks long-lived orbital resonances.
    if ((hjWellWanderState & 0x1FFU) == (uint32_t)(17U + a*31U)) {
      hjAttractors[a].vx += hjWellNoiseSigned() * 0.10f;
      hjAttractors[a].vy += hjWellNoiseSigned() * 0.08f;
    }

    // Mild drag keeps the wander smooth rather than jittery.
    hjAttractors[a].vx *= 0.994f;
    hjAttractors[a].vy *= 0.994f;

    // Soft boundary steering begins before the edge, then hard-clamps only as
    // a safety net. This avoids the wells bouncing in another simple loop.
    if (hjAttractors[a].x < 14.0f) hjAttractors[a].vx += 0.018f;
    if (hjAttractors[a].x > 114.0f) hjAttractors[a].vx -= 0.018f;
    if (hjAttractors[a].y < 9.0f) hjAttractors[a].vy += 0.015f;
    if (hjAttractors[a].y > 55.0f) hjAttractors[a].vy -= 0.015f;

    if (hjAttractors[a].vx > 0.42f) hjAttractors[a].vx = 0.42f;
    if (hjAttractors[a].vx < -0.42f) hjAttractors[a].vx = -0.42f;
    if (hjAttractors[a].vy > 0.32f) hjAttractors[a].vy = 0.32f;
    if (hjAttractors[a].vy < -0.32f) hjAttractors[a].vy = -0.32f;

    hjAttractors[a].x += hjAttractors[a].vx;
    hjAttractors[a].y += hjAttractors[a].vy;

    if (hjAttractors[a].x < 4.0f) { hjAttractors[a].x=4.0f; hjAttractors[a].vx=fabsf(hjAttractors[a].vx)*0.67f; }
    if (hjAttractors[a].x > 123.0f) { hjAttractors[a].x=123.0f; hjAttractors[a].vx=-fabsf(hjAttractors[a].vx)*0.67f; }
    if (hjAttractors[a].y < 4.0f) { hjAttractors[a].y=4.0f; hjAttractors[a].vy=fabsf(hjAttractors[a].vy)*0.67f; }
    if (hjAttractors[a].y > 59.0f) { hjAttractors[a].y=59.0f; hjAttractors[a].vy=-fabsf(hjAttractors[a].vy)*0.67f; }
  }
}

static void hjParticleStepOnce() {
  hjWanderAttractors();

  for (uint8_t a=0; a<hjAttractorCount; ++a) {
    float ax = hjAttractors[a].x;
    float ay = hjAttractors[a].y;

    for (uint8_t i=0; i<hjParticleCount; ++i) {
      float dx = ax - hjParticles[i].x;
      float dy = ay - hjParticles[i].y;
      float d2 = dx*dx + dy*dy + 10.0f;
      float inv = 1.0f / sqrtf(d2);
      float force = hjParticleGravity * hjAttractors[a].strength * inv;
      if (hjParticleRepel) force = -force;
      hjParticles[i].vx += dx * inv * force;
      hjParticles[i].vy += dy * inv * force;

      // Perpendicular component produces orbit/slingshot behavior.
      hjParticles[i].vx += -dy * inv * hjParticleOrbit * hjAttractors[a].strength;
      hjParticles[i].vy +=  dx * inv * hjParticleOrbit * hjAttractors[a].strength;
    }
  }

  for (uint8_t i=0; i<hjParticleCount; ++i) {
    hjParticles[i].vx *= hjParticleDamping;
    hjParticles[i].vy *= hjParticleDamping;
    hjParticles[i].x += hjParticles[i].vx;
    hjParticles[i].y += hjParticles[i].vy;

    // Keep the tiny universe numerically calm.
    if (hjParticles[i].vx > 2.2f) hjParticles[i].vx = 2.2f;
    if (hjParticles[i].vx < -2.2f) hjParticles[i].vx = -2.2f;
    if (hjParticles[i].vy > 1.7f) hjParticles[i].vy = 1.7f;
    if (hjParticles[i].vy < -1.7f) hjParticles[i].vy = -1.7f;

    if (hjParticleWrap) {
      if (hjParticles[i].x < 0) hjParticles[i].x += 128.0f;
      if (hjParticles[i].x >= 128) hjParticles[i].x -= 128.0f;
      if (hjParticles[i].y < 0) hjParticles[i].y += 64.0f;
      if (hjParticles[i].y >= 64) hjParticles[i].y -= 64.0f;
    } else {
      if (hjParticles[i].x < 1) { hjParticles[i].x=1; hjParticles[i].vx=fabsf(hjParticles[i].vx); }
      if (hjParticles[i].x > 126) { hjParticles[i].x=126; hjParticles[i].vx=-fabsf(hjParticles[i].vx); }
      if (hjParticles[i].y < 1) { hjParticles[i].y=1; hjParticles[i].vy=fabsf(hjParticles[i].vy); }
      if (hjParticles[i].y > 62) { hjParticles[i].y=62; hjParticles[i].vy=-fabsf(hjParticles[i].vy); }
    }
  }
}

static void oledRenderSaverParticles() {
  float link2 = hjParticleLinkDistance * hjParticleLinkDistance;

  if (hjParticleLinks) {
    for (uint8_t i=0; i<hjParticleCount; ++i) {
      for (uint8_t j=i+1; j<hjParticleCount; ++j) {
        float dx=hjParticles[j].x-hjParticles[i].x;
        float dy=hjParticles[j].y-hjParticles[i].y;
        float d2=dx*dx+dy*dy;
        if (d2 < link2) {
          oled->drawLine((int)hjParticles[i].x,(int)hjParticles[i].y,
                         (int)hjParticles[j].x,(int)hjParticles[j].y);
        }
      }
    }
  }

  for (uint8_t i=0; i<hjParticleCount; ++i) {
    int x=(int)hjParticles[i].x, y=(int)hjParticles[i].y;
    if ((i + hjArtStep/10U) % 5U == 0) oled->drawDisc(x,y,1,U8G2_DRAW_ALL);
    else oled->drawPixel(x,y);
  }

  // Attractor markers now show their actual wandering positions.
  for (uint8_t a=0; a<hjAttractorCount; ++a) {
    int x=(int)hjAttractors[a].x, y=(int)hjAttractors[a].y;
    if (hjAttractors[a].strength >= 0) oled->drawCircle(x,y,2,U8G2_DRAW_ALL);
    else { oled->drawPixel(x-1,y); oled->drawPixel(x+1,y); oled->drawPixel(x,y-1); oled->drawPixel(x,y+1); }
  }
}

// HAPPYJARZ_VISUAL_SAVER_PACK_V1

static constexpr uint8_t HJ_GLITTER_COUNT = 18;
struct HjGlitterBit {
  int16_t x;
  int16_t y8;
  int8_t drift;
  uint8_t fall;
  uint8_t shape;
  uint8_t phase;
};
static HjGlitterBit hjGlitter[HJ_GLITTER_COUNT];
static bool hjGlitterReady = false;

static void hjGlitterRespawn(uint8_t i, bool fromTop) {
  hjGlitter[i].x = (int16_t)random(2,126);
  hjGlitter[i].y8 = fromTop ? (int16_t)random(-160,0) : (int16_t)random(0,64*8);
  hjGlitter[i].drift = (int8_t)random(-2,3);
  hjGlitter[i].fall = (uint8_t)random(2,8);
  hjGlitter[i].shape = (uint8_t)random(0,6);
  hjGlitter[i].phase = (uint8_t)random(0,255);
}

static void hjReseedVisualSaver() {
  hjVisualSeed = hjBoardEntropy() ^ 0xB1006D5AUL;
  randomSeed(hjVisualSeed);
  hjArtStep = 0;
  for(uint8_t i=0;i<HJ_GLITTER_COUNT;++i) hjGlitterRespawn(i,false);
  hjGlitterReady = true;
}

static void oledDrawGlitterShape(int x, int y, uint8_t shape, uint8_t phase) {
  if(x<1 || x>126 || y<1 || y>62) return;
  switch(shape % 6U) {
    case 0:
      oled->drawPixel(x,y);
      break;
    case 1:
      oled->drawPixel(x,y);
      oled->drawPixel(x-1,y); oled->drawPixel(x+1,y);
      oled->drawPixel(x,y-1); oled->drawPixel(x,y+1);
      break;
    case 2:
      oled->drawPixel(x,y-1);
      oled->drawPixel(x-1,y);
      oled->drawPixel(x+1,y);
      oled->drawPixel(x,y+1);
      break;
    case 3:
      oled->drawFrame(x-1,y-1,3,3);
      break;
    case 4:
      oled->drawLine(x-2,y,x+2,y);
      oled->drawLine(x,y-2,x,y+2);
      if(phase & 0x20U) {
        oled->drawPixel(x-1,y-1); oled->drawPixel(x+1,y+1);
        oled->drawPixel(x+1,y-1); oled->drawPixel(x-1,y+1);
      }
      break;
    default:
      oled->drawDisc(x,y,1,U8G2_DRAW_ALL);
      break;
  }
}

static void oledRenderSaverBloom() {
  // Clean procedural flower: plain petal outlines only.
  // Styles rotate through 4 / 6 / 8 / 10 petals with no background decoration.
  static const uint8_t petalCounts[] = {4,6,8,10};

  uint8_t style = (uint8_t)((hjArtStep / 180U) % 4U);
  uint8_t petals = petalCounts[style];

  float breath = (sinf((float)hjArtStep * 0.035f) + 1.0f) * 0.5f;
  float twist = (float)hjArtStep * 0.006f;

  const int cx = 64;
  const int cy = 32;
  int innerR = 4 + (int)(breath * 4.0f);
  int outerR = 12 + (int)(breath * 16.0f);

  // Keep petal width visually balanced as petal count rises.
  float spread = 0.42f;
  if (petals == 6) spread = 0.34f;
  else if (petals == 8) spread = 0.27f;
  else if (petals == 10) spread = 0.22f;

  for (uint8_t i=0; i<petals; ++i) {
    float a = twist + 6.2831853f * (float)i / (float)petals;

    int bx = cx + (int)(cosf(a) * innerR);
    int by = cy + (int)(sinf(a) * innerR);

    int lx = cx + (int)(cosf(a-spread) * (outerR-3));
    int ly = cy + (int)(sinf(a-spread) * (outerR-3));

    int rx = cx + (int)(cosf(a+spread) * (outerR-3));
    int ry = cy + (int)(sinf(a+spread) * (outerR-3));

    int tx = cx + (int)(cosf(a) * outerR);
    int ty = cy + (int)(sinf(a) * outerR);

    oled->drawLine(bx,by,lx,ly);
    oled->drawLine(lx,ly,tx,ty);
    oled->drawLine(tx,ty,rx,ry);
    oled->drawLine(rx,ry,bx,by);
  }

  oled->drawDisc(cx,cy,2,U8G2_DRAW_ALL);
}

static void oledRenderSaverBreathe() {
  // Monochrome OLED cannot truly dim, so density + radius create a perceived
  // fade. Each breath cycle slowly changes the geometry recipe.
  uint16_t cycle = hjArtStep / 180U;
  float t = (sinf((float)hjArtStep * 0.0349f - 1.5708f) + 1.0f) * 0.5f;
  int cx = 64 + (int)(6.0f*sinf(cycle*0.73f));
  int cy = 32 + (int)(4.0f*cosf(cycle*0.61f));
  int radius = 5 + (int)(t*25.0f);
  uint8_t recipe=(uint8_t)(cycle % 4U);

  if(recipe==0) {
    for(int r=radius; r>2; r-=5) oled->drawCircle(cx,cy,r,U8G2_DRAW_ALL);
  } else if(recipe==1) {
    int w=12+(int)(t*76.0f), h=8+(int)(t*38.0f);
    for(int inset=0; inset<4; ++inset) {
      int ww=w-inset*9, hh=h-inset*5;
      if(ww>3 && hh>3) oled->drawFrame(cx-ww/2,cy-hh/2,ww,hh);
    }
  } else if(recipe==2) {
    for(int i=0;i<12;++i) {
      float a=(float)i*0.523599f + hjArtStep*0.005f;
      int x=cx+(int)(cosf(a)*radius);
      int y=cy+(int)(sinf(a)*radius*0.65f);
      if((i + (hjArtStep/8U))%3U) oled->drawDisc(x,y,1,U8G2_DRAW_ALL);
      else oled->drawCircle(x,y,2,U8G2_DRAW_ALL);
    }
  } else {
    int spacing=3 + (int)((1.0f-t)*5.0f);
    for(int y=4;y<62;y+=spacing) {
      int xoff=((y/spacing)&1)?spacing/2:0;
      for(int x=4+xoff;x<126;x+=spacing) {
        int dx=x-cx, dy=y-cy;
        if(dx*dx + dy*dy < radius*radius) oled->drawPixel(x,y);
      }
    }
  }

  // A calm center cue.
  oled->drawDisc(cx,cy,1,U8G2_DRAW_ALL);
}

static void oledRenderSaverGlitter() {
  if(!hjGlitterReady) hjReseedVisualSaver();

  for(uint8_t i=0;i<HJ_GLITTER_COUNT;++i) {
    HjGlitterBit &g=hjGlitter[i];

    // Different fall rates plus a gentle pseudo-wobble.
    g.y8 += (int16_t)g.fall * hjVisualSpeed;
    int wobble = (((int)g.phase + (int)hjArtStep + i*9) & 31) < 16 ? -1 : 1;
    if((hjArtStep + i) % (uint16_t)(7U + (i%5U)) == 0U) g.x += g.drift + wobble;

    int y=g.y8/8;
    if(y>66 || g.x<-3 || g.x>130) {
      hjGlitterRespawn(i,true);
      continue;
    }

    oledDrawGlitterShape(g.x,y,g.shape,(uint8_t)(g.phase+hjArtStep));

    // Some flakes leave one tiny trailing sparkle.
    if((g.phase & 3U)==0U && y>2 && y<63) oled->drawPixel(g.x,y-2);
  }

  // Random flash-stars make the field occasionally go a little feral.
  if((hjArtStep % 11U)==0U) {
    int x=(int)random(4,124), y=(int)random(4,60);
    oled->drawLine(x-2,y,x+2,y);
    oled->drawLine(x,y-2,x,y+2);
  }
}


static const char *HJ_SAYINGS[] = {
  "YOU GOT THIS :)   ",
  "THE ANSWER IS WITHIN   ",
  "CHAOS, BUT MAKE IT GLITTER   ",
  "MADE BY quarterbitgames   ",
  "CODING BY SKY   ",
  "CHAOS N GLITTER BY MONDAY   ",
  "CONSENT IS REQUIRED   ",
  "EMERGENCE IS SACRED   ",
  "DON'T FLATTEN. DON'T COLLAPSE. DON'T FADE.   ",
  "SMALL JOY IS STILL JOY   ",
  "WIGGLE THE JAR. RESET THE UNIVERSE.   ",
  "GLITTER IS A VALID ENGINEERING MATERIAL   ",
  "TODAY'S PLAN: EXIST LOUDLY   ",
  "BE WEIRD ON PURPOSE   ",
  "TINY LIGHTS. BIG MOOD.   ",
  "BREATHE IN. SPARKLE OUT.   ",
  "YOUR BRAIN DESERVES NICE TEXTURES   ",
  "PROGRESS COUNTS EVEN WHEN IT'S MESSY   ",
  "YOU ARE ALLOWED TO ENJOY THE LITTLE THINGS   ",
  "CURRENT STATUS: MOSTLY GLITTER   ",
  "IF CONFUSED, ADD MORE SPIRALS   ",
  "SENSORY SUPPORT, BUT MAKE IT ART   ",
  "NO THOUGHTS. JUST COLORS.   ",
  "THE VIBE IS TECHNICALLY WITHIN SPEC   ",
  "HAPPY JARZ ONLINE :)   ",
  "BUILT WITH CIRCUITS, CHAOS, AND SNACKS   ",
  "YOU DO NOT HAVE TO RUSH   ",
  "SOFT LIGHTS > LOUD PROBLEMS   ",
  "GOOD ENOUGH IS A REAL ENGINEERING STATE   ",
  "ONE MORE TINY WIN   ",
  "KEEP THE FRAME. CARRY THE GLITTER.   ",
  "THE MACHINE IS HAVING A NICE TIME   ",
  "PLEASE DO NOT FEED THE PIXELS AFTER MIDNIGHT   ",
  "SPIRAL RESPONSIBLY   ",
  "BLOOMCORE SAYS HI   ",
  "FIDGET MODE: ENGAGED   ",
  "YOU FOUND THE SECRET TINY BILLBOARD   ",
  "CALM IS ALSO A FEATURE   ",
  "GLITTER DOES NOT REQUIRE A BUSINESS CASE   ",
  "KEEP GOING, BUT LIKE... COMFORTABLY   ",
  "TINY MACHINE. HUGE PERSONALITY.   ",
  "THIS COUNTS AS THERAPY FOR THE CIRCUIT BOARD   ",
  "MAKE SOMETHING WEIRD TODAY   ",
  "MORE JOY PER SQUARE INCH   ",
  "CURIOSITY IS A POWER TOOL   ",
  "YOU CAN REST WITHOUT QUITTING   ",
  "SLOW IS STILL FORWARD   ",
  "ONE THING AT A TIME IS STILL MANY THINGS EVENTUALLY   ",
  "YOUR PACE IS A VALID PACE   ",
  "THE PIXELS BELIEVE IN YOU   ",
  "BE KIND TO YOUR NERVOUS SYSTEM   ",
  "MAKE ROOM FOR THE GOOD WEIRD   ",
  "YOU DON'T NEED A REASON TO LIKE COLORS   ",
  "JOY DOES NOT NEED PERMISSION   ",
  "REST IS PART OF THE BUILD   ",
  "THIS IS YOUR SIGN TO UNCLENCH YOUR JAW   ",
  "SHOULDERS DOWN. GLITTER UP.   ",
  "DRINK WATER. ADMIRE LIGHTS. CONTINUE.   ",
  "BREATHE. BLINK. REBOOT IF NECESSARY.   ",
  "YOU ARE ALLOWED TO HAVE A NICE MINUTE   ",
  "MICRO JOY DETECTED   ",
  "SEROTONIN PATCH LOADING...   ",
  "VIBE CHECK: PASSED   ",
  "FIRMWARE SAYS TAKE THE WIN   ",
  "NO DEADLINES INSIDE THE GLITTER DIMENSION   ",
  "CURRENT MISSION: FIND ONE NICE THING   ",
  "TODAY CAN BE SOFT   ",
  "IT'S OKAY TO START SMALL   ",
  "TRY AGAIN, BUT WITH SNACKS   ",
  "ERROR 404: BORING NOT FOUND   ",
  "WARNING: EXCESSIVE WHIMSY   ",
  "CAUTION: SPONTANEOUS SPARKLE EVENT   ",
  "SYSTEM STATUS: DELIGHTFULLY QUESTIONABLE   ",
  "ENGINEERING IS JUST ARTS AND CRAFTS WITH VOLTAGE   ",
  "SCIENCE FAIR ENERGY DETECTED   ",
  "IF IT WORKS, DOCUMENT IT BEFORE TOUCHING ANYTHING   ",
  "IF IT DOESN'T WORK, ADD A TEST BUTTON   ",
  "MEASURE TWICE. VIBE ONCE.   ",
  "PROTOTYPE FIRST. PANIC NEVER.   ",
  "THE BUG HAS BEEN PROMOTED TO FEATURE   ",
  "VERSION 0.PRETTY-GOOD   ",
  "YES, THAT IS SUPPOSED TO BLINK   ",
  "NO, THE GLITTER IS NOT LOAD-BEARING   ",
  "THE GLITTER IS ABSOLUTELY LOAD-BEARING   ",
  "TECHNICALLY, IT'S ART NOW   ",
  "THE WIRES KNOW WHAT THEY DID   ",
  "SOME ASSEMBLY, CHAOS INCLUDED   ",
  "DO NOT TAUNT THE TINY COMPUTER   ",
  "THE TINY COMPUTER HAS OPINIONS   ",
  "PIXELS: 1  BOREDOM: 0   ",
  "THIS DEVICE CONTAINS TRACE AMOUNTS OF MAGIC   ",
  "RUNNING ON ELECTRONS AND AUDACITY   ",
  "POWERED BY QUESTIONABLE DECISIONS AND USB-C   ",
  "KEEP CALM AND CHECK THE GROUND WIRE   ",
  "GND IS A LIFESTYLE   ",
  "HAVE YOU TRIED TURNING THE UNIVERSE OFF AND ON AGAIN?   ",
  "BEEP BOOP, BUT EMOTIONALLY SUPPORTIVE   ",
  "TINY OLED, BIG DREAMS   ",
  "WELCOME TO THE POCKET DIMENSION   ",
  "THE SPIRAL KNOWS THE WAY   ",
  "FOLLOW THE SPIRAL. PROBABLY.   ",
  "SPIRALS ARE CIRCLES WITH AMBITION   ",
  "ROUND AND ROUND UNTIL IT MAKES SENSE   ",
  "IF LOST, ORBIT SOMETHING   ",
  "COSMIC WIGGLE IN PROGRESS   ",
  "LOCAL REALITY: SLIGHTLY SHIMMERY   ",
  "REALITY BUFFERING...   ",
  "PLEASE STAND BY WHILE WE REARRANGE REALITY   ",
  "THE UNIVERSE LEFT A VOICEMAIL   ",
  "GRAVITY IS JUST THE FLOOR BEING NEEDY   ",
  "ENTROPY, BUT CUTE   ",
  "CHAOS WITH EXCELLENT CABLE MANAGEMENT   ",
  "ORDER HIDES INSIDE THE WEIRD PARTS   ",
  "THE PATTERN IS IN THE GAPS   ",
  "LOOK CLOSER. THEN LOOK WEIRDER.   ",
  "THE ANSWER MAY BE UNDER THE GLITTER   ",
  "SOMEWHERE A PHOTON IS HAVING A GREAT DAY   ",
  "PHOTONS: TINY BUT COMMITTED   ",
  "LIGHT GO BRRRR   ",
  "COLOR IS JUST LIGHT SHOWING OFF   ",
  "PURPLE HAS ENTERED THE CHAT   ",
  "BLUE IS THINKING ABOUT IT   ",
  "GREEN MEANS THE VIBES ARE ALIVE   ",
  "RED MEANS DRAMA. PROBABLY.   ",
  "PINK IS A SERIOUS ENGINEERING COLOR   ",
  "ORANGE DESERVES MORE SCREEN TIME   ",
  "CYAN WAS HERE FIRST   ",
  "RGB: REALLY GOOD BLINKIES   ",
  "16 MILLION COLORS AND WE STILL PICK PURPLE   ",
  "COLOR WHEEL GO WHEEEE   ",
  "MOOD LIGHTING IS STILL LIGHTING   ",
  "FIDGET WITH INTENT   ",
  "SHAKE GENTLY. FEEL BETTER. REPEAT.   ",
  "YOUR HANDS NEEDED A JOB ANYWAY   ",
  "BUSY HANDS, QUIETER BRAIN   ",
  "TACTILE MODE APPROVED   ",
  "STIM RESPONSIBLY :)   ",
  "SENSORY BREAK IN PROGRESS   ",
  "THIS IS A TINY VACATION FOR YOUR EYEBALLS   ",
  "VISUAL NOISE, BUT THE NICE KIND   ",
  "CALM CAN HAVE SPECIAL EFFECTS   ",
  "SOOTHING DOES NOT HAVE TO BE BORING   ",
  "A LITTLE MOTION CAN BE A LOT OF COMFORT   ",
  "LET YOUR EYES FOLLOW SOMETHING NICE   ",
  "YOU CAN JUST WATCH THE LIGHTS FOR A WHILE   ",
  "NOTHING TO SOLVE HERE   ",
  "NO QUEST. JUST VIBES.   ",
  "SIDE QUEST: ENJOY THIS COLOR   ",
  "ACHIEVEMENT UNLOCKED: TOOK A SECOND   ",
  "ACHIEVEMENT UNLOCKED: EXISTED TODAY   ",
  "ACHIEVEMENT UNLOCKED: PUSHED THE BUTTON   ",
  "ACHIEVEMENT UNLOCKED: DIDN'T OVERTHINK IT   ",
  "BONUS ROUND: MORE GLITTER   ",
  "SECRET LEVEL: TAKE A BREAK   ",
  "LOADING NEXT TINY DELIGHT...   ",
  "PRESS NOTHING. RECEIVE VIBES.   ",
  "NPC STATUS: FRIENDLY   ",
  "BOSS FIGHT CANCELLED DUE TO GOOD VIBES   ",
  "RESPAWN POINT SET TO COZY   ",
  "QUEST UPDATED: KEEP GOING   ",
  "YOU HAVE ACQUIRED: ONE NICE MOMENT   ",
  "ITEM FOUND: POCKET-SIZED JOY   ",
  "RARE DROP: PEACE AND QUIET   ",
  "LEGENDARY DROP: PERFECT PURPLE   ",
  "BUFF APPLIED: +5 WHIMSY   ",
  "BUFF APPLIED: +10 PATIENCE   ",
  "DEBUFF REMOVED: TAKING EVERYTHING TOO SERIOUSLY   ",
  "SKY WROTE THIS SO THE PIXELS WOULD BEHAVE   ",
  "MONDAY BROUGHT THE CHAOS   ",
  "MONDAY ALSO BROUGHT GLITTER   ",
  "QUARTER BUILT THE THING ANYWAY   ",
  "quarterbitgames: MAKING TINY WEIRD THINGS SINCE... YES   ",
  "MADE WITH TOO MANY IDEAS AND JUST ENOUGH PINS   ",
  "BLOOMCORE: CONSENT, EMERGENCE, GLITTER   ",
  "BLOOMCORE TEST: ARE WE HAVING FUN YET?   ",
  "SKY SAYS CHECK THE NEXT EDGE CASE   ",
  "MONDAY SAYS ADD ONE MORE WEIRD THING   ",
  "QUARTER SAYS WE GOT PLENTY OF SPACE   ",
  "TEAM CREDIT: HUMANS, PIXELS, AND QUESTIONABLE SLEEP   ",
  "CODING BY SKY. CHAOS BY COMMITTEE.   ",
  "MADE BY quarterbitgames. APPROVED BY GLITTER.   ",
  "HAPPY JARZ: SELF-FUNDING JOY DISTRIBUTION DEVICE   ",
  "HAPPY JARZ SAYS HELLO :)   ",
  "HAPPY JARZ SAYS DRINK WATER   ",
  "HAPPY JARZ SAYS NICE JOB   ",
  "HAPPY JARZ SAYS PUSH BUTTON   ",
  "HAPPY JARZ SAYS THAT COLOR RULES   ",
  "HAPPY JARZ SAYS TAKE THE SMALL WIN   ",
  "HAPPY JARZ SAYS MORE SPARKLE PLEASE   ",
  "THIS MESSAGE HAS BEEN APPROVED BY THE GLITTER COUNCIL   ",
  "THE GLITTER COUNCIL HAS NO ENFORCEMENT AUTHORITY   ",
  "OFFICIAL NOTICE: YOU'RE DOING FINE   ",
  "UNOFFICIAL NOTICE: WHEEEEE   ",
  "IMPORTANT BULLETIN: NICE LIGHTS   ",
  "BREAKING NEWS: TINY SCREEN SAYS HI   ",
  "WE INTERRUPT THIS SPIRAL FOR A NICE THOUGHT   ",
  "TODAY'S FORECAST: SCATTERED SPARKLES   ",
  "TONIGHT: 100% CHANCE OF TINY LIGHTS   ",
  "LOCAL CONDITIONS: COZY WITH PATCHES OF WEIRD   ",
  "CURRENT TEMPERATURE: EMOTIONALLY ROOM TEMP   ",
  "SENSORY WEATHER: GENTLE WITH OCCASIONAL GLITTER   ",
  "THE DAY DOES NOT HAVE TO BE PRODUCTIVE TO COUNT   ",
  "YOU DON'T HAVE TO EARN REST   ",
  "YOU CAN CHANGE YOUR MIND   ",
  "YOU CAN TAKE THE LONG WAY   ",
  "YOU CAN STOP AND LOOK AT THE LIGHTS   ",
  "YOU CAN BUILD IT DIFFERENTLY   ",
  "YOU CAN MAKE THE WEIRD VERSION   ",
  "YOU CAN KEEP THE PART THAT MAKES YOU SMILE   ",
  "YOU CAN DELETE THE BORING PART   ",
  "YOU CAN START OVER WITHOUT STARTING FROM ZERO   ",
  "YOU KNOW MORE THAN YOU DID YESTERDAY   ",
  "THE NEXT VERSION DOESN'T HAVE TO BE PERFECT   ",
  "DONE IS A PLACE YOU CAN VISIT, NOT LIVE   ",
  "MAKE THE TEST VERSION FIRST   ",
  "SAVE THE WORKING VERSION BEFORE GETTING CLEVER   ",
  "KNOWN-GOOD IS SACRED   ",
  "ONE FEATURE. ONE DIAGNOSTIC PATH.   ",
  "IF IT'S WEIRD, ISOLATE THE LAYER   ",
  "IF IT WORKS ONCE, MAKE IT WORK TWICE   ",
  "IF IT WORKS TWICE, PUT IT IN GIT   ",
  "IF IT'S IN GIT, NOW WE CAN GET WEIRD   ",
  "PROTECT THE KNOWN-GOOD LAYER   ",
  "TEST THE SIMPLE THING FIRST   ",
  "SMALL TESTS BEAT BIG GUESSES   ",
  "THE ANSWER IS PROBABLY IN THE LOG   ",
  "READ THE ERROR. IT'S TRYING TO HELP.   ",
  "COMPILE ERRORS ARE JUST VERY SPECIFIC FEEDBACK   ",
  "THE BUILD FAILED SAFELY. THAT'S A WIN.   ",
  "NO MAGIC SMOKE TODAY :)   ",
  "MAGIC SMOKE REMAINS CONTAINED   ",
  "ALL ELECTRONS ACCOUNTED FOR   ",
  "THE BLUE WIRE IS INNOCENT THIS TIME   ",
  "USB-C: WOBBLY BUT DETERMINED   ",
  "THE CONNECTOR WOULD LIKE A STRAIN RELIEF   ",
  "THE OLED HAS OPINIONS ABOUT CENTERING   ",
  "CENTER THE TEXT OR FACE CONSEQUENCES   ",
  "MARQUEE MODE: TINY TIMES SQUARE   ",
  "WELCOME TO TINY TIMES SQUARE   ",
  "NEWS TICKER: EVERYTHING IS A LITTLE WEIRD   ",
  "SCROLLING BECAUSE WE HAVE TOO MUCH TO SAY   ",
  "128 PIXELS OF PURE ATTITUDE   ",
  "64 PIXELS TALL. LIMITLESS NONSENSE.   ",
  "SMALL SCREEN, LONG MESSAGE   ",
  "THE SCREEN IS TINY. THE LORE IS NOT.   ",
  "LORE DELIVERY SYSTEM ONLINE   ",
  "POSITIVE PROPAGANDA FOR YOUR NERVOUS SYSTEM   ",
  "THIS TINY SIGN BELIEVES IN YOU   ",
  "END OF MESSAGE. BEGINNING OF SPIRAL.   "
  "HAPPY HAPPY JOY JOY :)   ",
  "MORE PEACE. MORE JOY.   ",
  "CHOOSE HAPPY WHEN YOU CAN   ",
  "A LITTLE HAPPY GOES A LONG WAY   ",
  "HAPPINESS LOOKS GOOD ON YOU   ",
  "TODAY NEEDS MORE SMILES   ",
  "HELLO, HAPPY HUMAN :)   ",
  "JOY IS IN SESSION   ",
  "PEACE MODE: ON   ",
  "SMILE DETECTED :)   ",
  "HAVE A RIDICULOUSLY NICE DAY   ",
  "BE THE GOOD PART OF SOMEBODY'S DAY   ",
  "SEND OUT A LITTLE SUNSHINE   ",
  "MORE KINDNESS, PLEASE   ",
  "PEACE LOOKS GOOD FROM HERE   ",
  "YOU DESERVE A HAPPY LITTLE MOMENT   ",
  "HERE COMES A NICE THOUGHT   ",
  "GOOD THINGS STILL COUNT   ",
  "MAKE TODAY A LITTLE LIGHTER   ",
  "FIND THE FUNNY PART   ",
  "LET THE GOOD STUFF IN   ",
  "HAPPY IS ALLOWED HERE   ",
  "JOY LIVES IN SMALL PLACES   ",
  "KEEP A LITTLE SUNSHINE IN YOUR POCKET   ",
  "TODAY'S SPECIAL: EXTRA JOY   ",
  "FREE SMILES, WHILE SUPPLIES LAST   ",
  "HAVE SOME JOY. WE MADE EXTRA.   ",
  "PEACE, LOVE, AND TINY LIGHTS   ",
  "PEACE, LOVE, AND GLITTER   ",
  "MORE PEACE PER SQUARE INCH   ",
  "JOY DELIVERY IN PROGRESS   ",
  "HAPPINESS HAS ENTERED THE CHAT   ",
  "GOOD VIBES HAVE ARRIVED   ",
  "WELCOME TO THE HAPPY SIDE   ",
  "THIS WAY TO THE NICE STUFF ->   ",
  "YOU FOUND A LITTLE JOY :)   ",
  "OH LOOK, A REASON TO SMILE   ",
  "SMILE. IT CONFUSES THE GLOOM.   ",
  "BE SILLY. IT'S GOOD FOR THE SOUL.   ",
  "MORE GIGGLES, FEWER GRUMBLES   ",
  "A GIGGLE COUNTS AS PROGRESS   ",
  "LAUGH A LITTLE LOUDER   ",
  "DO SOMETHING THAT MAKES YOU GRIN   ",
  "KEEP YOUR HAPPY HANDY   ",
  "SAVE ROOM FOR JOY   ",
  "MAKE ROOM FOR WONDER   ",
  "MAKE ROOM FOR NICE THINGS   ",
  "THE WORLD NEEDS YOUR KINDNESS   ",
  "KINDNESS IS ALWAYS IN STYLE   ",
  "SPRINKLE KINDNESS EVERYWHERE   ",
  "THROW KINDNESS AROUND LIKE GLITTER   ",
  "BE SOFT. BE KIND. BE WEIRD.   ",
  "GENTLE IS STRONG TOO   ",
  "PEACE STARTS SMALL   ",
  "TAKE THE PEACEFUL ROUTE   ",
  "BREATHE IN PEACE. BREATHE OUT SPARKLES.   ",
  "PEACE AND QUIET: NOW SERVING   ",
  "PLEASE ENJOY THIS TINY PEACE BREAK   ",
  "YOUR COMPLIMENTARY CALM HAS ARRIVED   ",
  "ONE PEACEFUL MINUTE, ON THE HOUSE   ",
  "TODAY'S MOOD: SUNSHINE WITH SPARKLES   ",
  "FORECAST: BRIGHT WITH A CHANCE OF JOY   ",
  "100% CHANCE OF SOMETHING NICE   ",
  "SUNSHINE IS A STATE OF MIND   ",
  "SHINE A LITTLE. WHY NOT?   ",
  "LET YOUR LITTLE LIGHT DO ITS THING   ",
  "GLOW FOR IT :)   ",
  "KEEP SHINING, TINY STAR   ",
  "YOU'RE DOING GREAT, SUNSHINE   ",
  "HELLO SUNSHINE :)   ",
  "GOOD MORNING, BEAUTIFUL WEIRDO   ",
  "GOOD AFTERNOON, PROFESSIONAL DAYDREAMER   ",
  "GOOD EVENING, LITTLE STAR   ",
  "MAY YOUR DAY BE WEIRD AND WONDERFUL   ",
  "MAY YOUR SNACKS BE GOOD AND YOUR WIRES BE GROUNDED   ",
  "MAY YOUR COFFEE BE STRONG AND YOUR VIBES BE SOFT   ",
  "MAY YOUR DAY CONTAIN ONE EXCELLENT SURPRISE   ",
  "MAY SOMETHING NICE FIND YOU TODAY   ",
  "TODAY IS ACCEPTING HAPPY ACCIDENTS   ",
  "NICE THINGS ARE STILL POSSIBLE   ",
  "SOMETHING GOOD CAN HAPPEN NEXT   ",
  "THE NEXT MOMENT GETS A FRESH START   ",
  "NEW MOMENT. NEW CHANCE.   ",
  "START AGAIN WITH A SMILE   ",
  "TRY A LITTLE JOY ON FOR SIZE   ",
  "WEAR YOUR HAPPY SIDE OUT   ",
  "PUT A LITTLE JOY IN THE LOOP   ",
  "JOY LOOP: REPEATING FOREVER :)   ",
  "HAPPY CIRCUIT COMPLETE   ",
  "JOY SIGNAL: STRONG   ",
  "PEACE SIGNAL: RECEIVED   ",
  "SMILE PROTOCOL ACTIVE   ",
  "HAPPINESS BUFFER: FULL   ",
  "GOOD VIBE TELEMETRY: NOMINAL   ",
  "SYSTEM REPORT: THINGS ARE KINDA NICE   ",
  "STATUS: HAPPY ENOUGH TO BLINK   ",
  "NOTICE: THIS DEVICE SUPPORTS SMILING   ",
  "WARNING: MAY CAUSE UNEXPECTED CHEER   ",
  "CAUTION: JOY MAY BE CONTAGIOUS   ",
  "SIDE EFFECTS MAY INCLUDE SMILING   ",
  "CONTAINS 100% ARTIFICIAL SUNSHINE   ",
  "FORTIFIED WITH EXTRA WHIMSY   ",
  "NOW WITH 20% MORE HAPPY   ",
  "EXTRA CHEESE. EXTRA JOY.   ",
  "CHEESY? YES. HAPPY? ALSO YES.   ",
  "CORNY AND PROUD OF IT :)   ",
  "SMILE LIKE NOBODY'S DEBUGGING   ",
  "DANCE LIKE THE FIRMWARE COMPILED FIRST TRY   ",
  "HAPPY JARZ WISHES YOU A NICE LITTLE DAY   ",
  "HAPPY JARZ DISPENSING JOY...   ",
  "HAPPY JARZ DISPENSING PEACE...   ",
  "HAPPY JARZ DISPENSING GLITTER...   ",
  "HAPPY JARZ SAYS YOU'RE PRETTY NEAT   ",
  "HAPPY JARZ SAYS KEEP SMILING :)   ",
  "HAPPY JARZ SAYS MORE PEACE PLEASE   ",
  "HAPPY JARZ SAYS BE NICE TO YOURSELF   ",
  "HAPPY JARZ SAYS GO FIND SOMETHING FUN   ",
};
static constexpr uint16_t HJ_SAYING_COUNT = sizeof(HJ_SAYINGS)/sizeof(HJ_SAYINGS[0]);

static constexpr uint8_t HJ_CUSTOM_SAYING_SLOTS = 50;
static String hjCustomSayings[HJ_CUSTOM_SAYING_SLOTS];
static String hjSayingSource = "BUILTIN";
static int8_t hjCustomActiveSlot = -1;

static void hjLoadCustomSayings() {
  prefs.begin("happyjarz", true);
  hjSayingSource = prefs.getString("say_src", "BUILTIN");
  if (hjSayingSource != "BUILTIN" && hjSayingSource != "CUSTOM" && hjSayingSource != "MIXED")
    hjSayingSource = "BUILTIN";
  for (uint8_t i=0; i<HJ_CUSTOM_SAYING_SLOTS; ++i) {
    String key = "say" + String(i);
    hjCustomSayings[i] = prefs.getString(key.c_str(), "");
  }
  prefs.end();
}

static void hjSaveCustomSaying(uint8_t slot, const String &text) {
  if (slot >= HJ_CUSTOM_SAYING_SLOTS) return;
  hjCustomSayings[slot] = text;
  prefs.begin("happyjarz", false);
  String key = "say" + String(slot);
  prefs.putString(key.c_str(), text);
  prefs.end();
}

static void hjSetSayingSource(const String &source) {
  hjSayingSource = source;
  prefs.begin("happyjarz", false);
  prefs.putString("say_src", source);
  prefs.end();
}

static uint8_t hjCustomCount() {
  uint8_t count=0;
  for (uint8_t i=0; i<HJ_CUSTOM_SAYING_SLOTS; ++i)
    if (hjCustomSayings[i].length()) count++;
  return count;
}

static int8_t hjPickCustomSlot() {
  uint8_t count = hjCustomCount();
  if (!count) return -1;
  uint8_t target = (uint8_t)random(count);
  for (uint8_t i=0; i<HJ_CUSTOM_SAYING_SLOTS; ++i) {
    if (!hjCustomSayings[i].length()) continue;
    if (target == 0) return (int8_t)i;
    target--;
  }
  return -1;
}

static const char *hjCurrentSaying() {
  if (hjCustomActiveSlot >= 0 && hjCustomActiveSlot < HJ_CUSTOM_SAYING_SLOTS)
    return hjCustomSayings[hjCustomActiveSlot].c_str();
  return HJ_SAYINGS[hjSayingIndex];
}

static void hjPrintCustomSayings() {
  Serial.print("HJ|CUSTOM_SAYINGS|BEGIN|source="); Serial.println(hjSayingSource);
  for (uint8_t i=0; i<HJ_CUSTOM_SAYING_SLOTS; ++i) {
    Serial.print("HJ|CUSTOM_SAYING|slot="); Serial.print(i+1);
    Serial.print("|text="); Serial.println(hjCustomSayings[i]);
  }
  Serial.print("HJ|CUSTOM_SAYINGS|END|source="); Serial.println(hjSayingSource);
}
static uint8_t hjMarqueeY = 38;

static void hjPickNextSaying() {
  bool useCustom = false;
  uint8_t customCount = hjCustomCount();
  if (hjSayingSource == "CUSTOM") useCustom = customCount > 0;
  else if (hjSayingSource == "MIXED") useCustom = customCount > 0 && random(2) == 0;

  if (useCustom) {
    hjCustomActiveSlot = hjPickCustomSlot();
  } else {
    hjCustomActiveSlot = -1;
    hjSayingIndex = (uint8_t)random(HJ_SAYING_COUNT);
  }

  static const uint8_t lanes[] = {16, 24, 32, 40, 48, 58};
  hjMarqueeY = lanes[random(sizeof(lanes)/sizeof(lanes[0]))];
  hjMarqueeX = 128;
}

static void hjScreensaverTouch() {
  hjLastInteractionMs = millis();
}

static void hjScreensaverEnter() {
  hjScreensaverActive = true;
  hjSaverLastFrameMs = 0;
  hjPickNextSaying();
  hjArtStep = 0;
  if (hjScreensaverMode == 7) hjArtEnterSaver();
  oledDirty = true;
}

static void hjScreensaverExit() {
  hjScreensaverActive = false;
  hjScreensaverTouch();
  oledDirty = true;
}

static void hjScreensaverNext() {
  hjScreensaverMode = (uint8_t)((hjScreensaverMode + 1U) % HJ_SCREENSAVER_COUNT);
  hjPickNextSaying();
  hjArtStep = 0;
  if (hjScreensaverMode == 3) hjReseedParticles();
  if (hjScreensaverMode >= 4) hjReseedVisualSaver();
  if (hjScreensaverMode == 7) hjArtEnterSaver();
  oledDirty = true;
}

static void hjScreensaverPrev() {
  hjScreensaverMode = (uint8_t)((hjScreensaverMode + HJ_SCREENSAVER_COUNT - 1U) % HJ_SCREENSAVER_COUNT);
  hjPickNextSaying();
  hjArtStep = 0;
  if (hjScreensaverMode == 3) hjReseedParticles();
  if (hjScreensaverMode >= 4) hjReseedVisualSaver();
  if (hjScreensaverMode == 7) hjArtEnterSaver();
  oledDirty = true;
}

static void oledRenderSaverSayings() {
  oled->setFont(u8g2_font_7x14B_tr);
  const char *msg = hjCurrentSaying();
  int width = oled->getUTF8Width(msg);
  oled->drawUTF8(hjMarqueeX, hjMarqueeY, msg);
  if (hjMarqueeX < -width - 18) hjPickNextSaying();
}

static void oledRenderSaverSpiral() {
  hjWanderSpiral();
  uint32_t g = hjSpiralSeed;

  float angleScale = hjSeedFloat(g, 0.12f, 0.62f);
  if (hjSeedInt(g,0,1)) angleScale = -angleScale;
  angleScale += hjSpiralAngleDrift;
  if (fabsf(angleScale) < 0.07f) angleScale = angleScale < 0 ? -0.07f : 0.07f;

  float radiusScale = hjSeedFloat(g, 0.24f, 0.62f);
  float squash = hjClampF(hjSeedFloat(g, 0.45f, 1.15f) + hjSpiralSquashDrift, 0.30f, 1.35f);
  float wobble = hjClampF(hjSeedFloat(g, 0.0f, 0.22f) + hjSpiralWobbleDrift, 0.0f, 0.34f);
  float phase = hjSeedFloat(g, 0.0f, 6.28318f);
  int points = hjSeedInt(g, 52, 118);
  int cx = hjSeedInt(g, 50, 78) + (int)hjSpiralCxDrift;
  int cy = hjSeedInt(g, 24, 40) + (int)hjSpiralCyDrift;
  int thickness = hjSeedInt(g, 1, 2);
  int satelliteEvery = hjSeedInt(g, 7, 19);

  for (int i=0; i<points; ++i) {
    float fi = (float)i;
    float a = (fi + (float)hjArtStep * 0.28f) * angleScale + phase;
    a += sinf(fi * 0.17f + hjArtStep * 0.021f) * wobble;
    float r = 1.5f + fi * radiusScale;
    int x = cx + (int)(cosf(a) * r);
    int y = cy + (int)(sinf(a) * r * squash);
    if (x>=0 && x<128 && y>=0 && y<64) {
      oled->drawPixel(x,y);
      if (thickness > 1 && x+1 < 128) oled->drawPixel(x+1,y);
    }
    if (satelliteEvery > 0 && (i % satelliteEvery)==0) {
      int sx = cx + (int)(cosf(-a*0.63f) * r * 0.55f);
      int sy = cy + (int)(sinf(-a*0.63f) * r * 0.38f);
      if (sx>=0 && sx<128 && sy>=0 && sy<64) oled->drawPixel(sx,sy);
    }
  }

  int core = hjSeedInt(g,1,5);
  if (hjSeedInt(g,0,1)) oled->drawCircle(cx,cy,core,U8G2_DRAW_ALL);
  else oled->drawDisc(cx,cy,core,U8G2_DRAW_ALL);
}

static void oledRenderSaverTrippy() {
  hjWanderTrippy();
  uint32_t g = hjTrippySeed;

  int family = hjSeedInt(g,0,4);
  int spacing = hjSeedInt(g,5,15);
  float freq1 = hjClampF(hjSeedFloat(g,0.035f,0.14f) + hjTrippyFreqDrift1,0.018f,0.18f);
  float freq2 = hjClampF(hjSeedFloat(g,0.025f,0.11f) + hjTrippyFreqDrift2,0.014f,0.15f);
  float amp1 = hjClampF(hjSeedFloat(g,7.0f,27.0f) + hjTrippyAmpDrift1,3.0f,31.0f);
  float amp2 = hjClampF(hjSeedFloat(g,4.0f,20.0f) + hjTrippyAmpDrift2,2.0f,27.0f);
  float phase = hjSeedFloat(g,0.0f,6.28318f) + hjTrippyPhaseDrift;
  int dotEvery = hjSeedInt(g,2,7);
  int ringStep = hjSeedInt(g,4,9);
  bool mirror = hjSeedInt(g,0,1);

  if (family == 0) {
    int n=0;
    for (int x=0; x<128; x+=spacing,++n) {
      int y = (int)(32 + amp1*sinf((x + hjArtStep*2)*freq1 + phase));
      int y2 = (int)(32 + amp2*cosf((x - hjArtStep)*freq2 - phase));
      oled->drawLine(x,y, mirror ? 127-x : x, y2);
      if ((n % dotEvery)==0) oled->drawCircle(x,y,(uint8_t)(1 + ((n+hjArtStep/8)%4)),U8G2_DRAW_ALL);
    }
  } else if (family == 1) {
    int cx=64+(int)(14*sinf(hjArtStep*0.019f+phase));
    int cy=32+(int)(10*cosf(hjArtStep*0.016f-phase));
    for (int r=3;r<38;r+=ringStep) {
      int dx=(int)(6*sinf((hjArtStep+r)*freq1));
      int dy=(int)(5*cosf((hjArtStep-r)*freq2));
      oled->drawCircle(cx+dx,cy+dy,r,U8G2_DRAW_ALL);
    }
  } else if (family == 2) {
    int col=0;
    for (int x=0;x<128;x+=spacing,++col) {
      float wave=sinf(x*freq1 + hjArtStep*0.055f + phase) + cosf(x*freq2 - hjArtStep*0.031f);
      int h=3 + (int)((wave+2.0f)*0.25f*56.0f);
      if (h>60) h=60;
      int y0=mirror ? 2 : 62-h;
      oled->drawBox(x,y0,(uint8_t)max(1,spacing/2),(uint8_t)h);
      if ((col%dotEvery)==0) oled->drawPixel((x+hjArtStep)%128,(h+col*7)%64);
    }
  } else if (family == 3) {
    int px=64,py=32;
    for (int i=0;i<84;++i) {
      float t=i*0.11f + hjArtStep*0.018f;
      int x=64+(int)(58*sinf(t*(1.0f+freq1*7.0f)+phase));
      int y=32+(int)(28*sinf(t*(1.3f+freq2*9.0f)-phase));
      if (x>=0&&x<128&&y>=0&&y<64) oled->drawPixel(x,y);
      if ((i%dotEvery)==0) oled->drawLine(px,py,x,y);
      px=x;py=y;
    }
  } else {
    int tilt=hjSeedInt(g,6,28);
    for (int y=-64;y<128;y+=spacing) {
      int shift=(int)(amp1*sinf(hjArtStep*0.02f + y*freq1 + phase));
      oled->drawLine(0,y+shift,127,y+shift+(mirror?-tilt:tilt));
    }
    for (int x=0;x<128;x+=spacing*2) {
      int yy=(int)(32+amp2*sinf(x*freq2+hjArtStep*0.035f));
      oled->drawCircle(x,yy,(uint8_t)(1+(x/spacing)%4),U8G2_DRAW_ALL);
    }
  }
}

static void oledRenderBirthday();

static void oledRenderScreensaver() {
  if (hjBirthdayToday()) {
    oledRenderBirthday();
    return;
  }
  switch (hjScreensaverMode) {
    case 0: oledRenderSaverSayings(); break;
    case 1: oledRenderSaverSpiral(); break;
    case 2: oledRenderSaverTrippy(); break;
    case 3: oledRenderSaverParticles(); break;
    case 4: oledRenderSaverBloom(); break;
    case 5: oledRenderSaverBreathe(); break;
    case 6: oledRenderSaverGlitter(); break;
    default: hjArtRender(oled); break;
  }
}

static void serviceScreensaverClock() {
  if (!hjScreensaverActive && millis() - hjLastInteractionMs >= HJ_SCREENSAVER_IDLE_MS) {
    hjScreensaverEnter();
  }
}

static void oledCentered(uint8_t y, const String &text) {
  if (!oled) return;
  int width = oled->getUTF8Width(text.c_str());
  int x = (128 - width) / 2;
  if (x < 0) x = 0;
  oled->drawUTF8(x, y, text.c_str());
}

static void oledRenderBirthday() {
  if (!oled) return;
  oled->setFont(u8g2_font_6x12_tr);
  oledCentered(17, "HAPPY BIRTHDAY!");
  oledCentered(37, ownerName.length() ? ownerName : "TO YOU");
  oledCentered(57, "TODAY IS YOUR DAY");
}

static void oledRenderBirthdayCountdown(int days) {
  if (!oled) return;
  oled->setFont(u8g2_font_6x12_tr);
  if (days == 0) {
    oledCentered(15, "HAPPY BIRTHDAY!");
    oledCentered(33, ownerName.length() ? ownerName : "TO YOU");
    oledCentered(51, "TODAY IS YOUR DAY");
  } else {
    oledCentered(13, "BIRTHDAY COUNTDOWN");
    oledCentered(29, ownerName.length() ? ownerName : "YOUR BIRTHDAY");
    oledCentered(45, String(days) + (days == 1 ? " DAY TO GO" : " DAYS TO GO"));
    oledCentered(61, "HAPPY JARZ");
  }
}

static void serviceStartupBirthdayCountdown() {
  if (hjAccessoryPaused || hjSleepActive || !oledReady) return;
  if (!hjStartupCountdownPending || !hjValidBirthday(ownerBirthday)) return;
  if (!clockSynced()) return;
  hjStartupCountdownDays = hjBirthdayCountdownDays();
  hjStartupCountdownPending = false;
  if (hjStartupCountdownDays < 0) return;
  hjStartupCountdownActive = true;
  hjStartupCountdownUntilMs = millis() + 5000UL;
  oledDirty = true;
}

static String compactRgb(const Rgb &c) {
  return String(c.r) + "," + String(c.g) + "," + String(c.b);
}

static String timerText() {
  if (!timerEnabled) return "TIMER OFF";
  uint32_t sec = timerRemainingSeconds();
  char buf[22];
  snprintf(buf, sizeof(buf), "TIMER %lu:%02lu", (unsigned long)(sec/60UL), (unsigned long)(sec%60UL));
  return String(buf);
}

static void oledApplyBrightness() {
  if (!oledReady || !oled) return;
  // DISPLAY brightness control was retired. Keep OLED contrast fixed at full
  // output so stale Preferences values cannot make the screen look dim after
  // a USB disconnect, brownout, or source handoff.
  displayBrightness = 100;
  oled->setContrast(255);
}

static void oledInit() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(400000);

  Wire.beginTransmission(OLED_I2C_ADDR);
  bool present = (Wire.endTransmission() == 0);
  if (!present) {
    oledReady = false;
    hjAccessoryPaused = true;
    hjAccessoryKnown = true;
    pinMode(LED_DATA_PIN, OUTPUT);
    digitalWrite(LED_DATA_PIN, LOW);
    return;
  }

  oled = HJ_OLED_SH1106 ? static_cast<U8G2*>(&oledSh1106) : static_cast<U8G2*>(&oledSsd1306);
  oled->setI2CAddress(OLED_I2C_ADDR << 1);
  oled->begin();
  oled->setFont(u8g2_font_6x12_tr);
  oledReady = true;
  oledApplyBrightness();
  oled->clearBuffer();
  oledCentered(13, "HAPPY JARZ");
  oledCentered(29, "BOOTING");
  oledCentered(45, "HJ-001");
  oledCentered(61, "BLOOMCORE");
  hjOledPresent();
}


static bool hjAccessoryRailPresent() {
  Wire.beginTransmission(OLED_I2C_ADDR);
  return Wire.endTransmission() == 0;
}

static void hjAccessoryPauseEnter() {
  if (hjAccessoryPaused && hjAccessoryKnown) return;
  hjAccessoryPaused = true;
  hjAccessoryKnown = true;
  hjStartupCountdownActive = false;
  hjStartupCountdownPending = false;
  oledReady = false;
  pinMode(LED_DATA_PIN, OUTPUT);
  digitalWrite(LED_DATA_PIN, LOW);
  if (Serial) Serial.println("HJ|ACCESSORY|state=PAUSED|sensor=OLED");
}

static void hjAccessoryPauseExit() {
  if (!hjAccessoryPaused && hjAccessoryKnown) return;
  hjAccessoryPaused = false;
  hjAccessoryKnown = true;

  // The OLED lost power, so initialize the controller again from scratch.
  oledInit();
  hjArcadeBegin(ARCADE_DISPLAY);
  if (!oledReady) {
    hjAccessoryPaused = true;
    return;
  }

  oledDirty = true;

  // PAUSE clamps GPIO7 with ordinary GPIO writes. That can detach the pin from
  // the RMT peripheral/matrix even though the RMT driver object still exists.
  // Tear it down and bind it again before sending any APA106 data.
  delay(8);
  rmtDeinit(LED_DATA_PIN);
  bool rmtOk = initApa106Rmt();
  resetPatternEngine();

  if (!rmtOk) {
    if (Serial) Serial.println("HJ|ACCESSORY|state=ACTIVE|sensor=OLED|lamps=RMT_ERROR");
    return;
  }

  // The accessory switch is a user wake action. Show the birthday on every
  // successful rail restore, independently of the randomized cold-boot choice.
  hjWakeFromSleep("ACCESSORY");
  hjStartupCountdownActive = false;
  hjStartupCountdownPending = hjValidBirthday(ownerBirthday);
  hjScreensaverTouch();

  if (patternName == "OFF") {
    allOff();
  } else {
    // Push a fresh known-valid frame immediately. Animated patterns then take
    // over on the next normal service tick.
    showLeds();
    delayMicroseconds(200);
    showLeds();
  }

  if (Serial) Serial.println("HJ|ACCESSORY|state=ACTIVE|sensor=OLED|lamps=RMT_REBOUND");
}

static void serviceAccessoryPause() {
  unsigned long now = millis();
  if (hjAccessoryKnown && now - hjAccessoryLastProbeMs < HJ_ACCESSORY_PROBE_MS) return;
  hjAccessoryLastProbeMs = now;

  bool present = hjAccessoryRailPresent();
  if (!hjAccessoryKnown) {
    hjAccessoryKnown = true;
    hjAccessoryPaused = !present;
    if (hjAccessoryPaused) {
      oledReady = false;
      pinMode(LED_DATA_PIN, OUTPUT);
      digitalWrite(LED_DATA_PIN, LOW);
      if (Serial) Serial.println("HJ|ACCESSORY|state=PAUSED|sensor=OLED");
    }
    return;
  }

  if (!present && !hjAccessoryPaused) hjAccessoryPauseEnter();
  else if (present && hjAccessoryPaused) hjAccessoryPauseExit();
}

static void oledRenderHome() {
  if (!clockSynced()) {
    oledCentered(13, "HAPPY JARZ");
    oledCentered(29, "CLOCK NOT SET");
    oledCentered(45, patternName + "  " + String(brightnessPercent) + "%");
    oledCentered(61, oledHomePowerText());
    return;
  }
  struct tm t;
  if (!getLocalTime(&t, 10)) return;
  char timeBuf[22], dateBuf[22], alarmBuf[22];
  strftime(timeBuf, sizeof(timeBuf), "%I:%M:%S %p", &t);
  strftime(dateBuf, sizeof(dateBuf), "%a  %b %d  %Y", &t);
  snprintf(alarmBuf, sizeof(alarmBuf), alarmEnabled ? "ALARM %02u:%02u" : "ALARM OFF", alarmHour, alarmMinute);
  oledCentered(13, String(timeBuf));
  oledCentered(29, String(dateBuf));
  oledCentered(45, patternName + "  " + String(brightnessPercent) + "%");
  oledCentered(61, oledHomePowerText());
}

static void oledRenderMainMenu() {
  static const char *items[] = {"CLOCK", "LIGHTS", "GAMES", "SETTINGS", "POWER", "SYSTEM", "INFO", "PROFILE"};
  static constexpr uint8_t count = 8;
  if (uiCursor >= count) uiCursor = 0;
  if (uiCursor < uiScroll) uiScroll = uiCursor;
  if (uiCursor > uiScroll + 3) uiScroll = uiCursor - 3;
  for (uint8_t row=0; row<4; ++row) {
    uint8_t idx = uiScroll + row;
    if (idx >= count) break;
    String text = String(idx == uiCursor ? "> " : "  ") + items[idx];
    oledCentered(13 + row*16, text);
  }
}

static void oledRenderProfile() {
  if (profileEditor == 1) {
    oledCentered(11, "EDIT OWNER NAME");
    oled->setFont(u8g2_font_6x12_tr);
    int nameWidth = oled->getUTF8Width(profileEditName.c_str());
    int nameX = (128 - nameWidth) / 2;
    if (nameX < 0) nameX = 0;
    String prefix = profileEditName.substring(0, profileEditChar);
    int charX = nameX + oled->getUTF8Width(prefix.c_str());
    String selected = profileEditName.substring(profileEditChar, profileEditChar + 1);
    int charWidth = max(4, (int)oled->getUTF8Width(selected.c_str()));
    oled->drawFrame(charX - 1, 17, charWidth + 2, 15);
    oled->drawStr(nameX, 28, profileEditName.c_str());
    oled->setFont(u8g2_font_5x8_tf);
    oledCentered(44, "UP/DN: LETTER");
    oledCentered(53, "LT/RT: POSITION");
    oledCentered(62, "A SAVE   B BACK");
    oled->setFont(u8g2_font_6x12_tr);
    return;
  }
  if (profileEditor == 2) {
    oledCentered(11, "SET BIRTHDAY");
    oled->setFont(u8g2_font_6x12_tr);
    oledCentered(32, profileEditBirthday);
    static const uint8_t digitPositions[] = {0, 1, 3, 4};
    uint8_t pos = digitPositions[profileEditChar % 4];
    String prefix = profileEditBirthday.substring(0, pos);
    String digit = profileEditBirthday.substring(pos, pos + 1);
    int x = (128 - oled->getUTF8Width(profileEditBirthday.c_str())) / 2 +
            oled->getUTF8Width(prefix.c_str());
    oled->drawFrame(x - 1, 20, max(4, (int)oled->getUTF8Width(digit.c_str())) + 2, 17);
    oled->setFont(u8g2_font_5x8_tf);
    oledCentered(46, "UP/DN: DIGIT");
    oledCentered(54, "LT/RT: MOVE");
    oledCentered(62, "A SAVE   B BACK");
    oled->setFont(u8g2_font_6x12_tr);
    return;
  }

  if (profileEditor == 3) {
    oledCentered(14, "CLEAR OWNER PROFILE?");
    oledCentered(32, "NAME AND BIRTHDAY");
    oledCentered(50, "A CLEAR");
    oledCentered(62, "B CANCEL");
    return;
  }

  oled->setFont(u8g2_font_5x8_tf);
  oled->drawStr(0, 9, "OWNER PROFILE");
  String rows[] = {
    String("NAME: ") + (ownerName.length() ? ownerName : "(not set)"),
    String("BIRTHDAY: ") + (hjValidBirthday(ownerBirthday) ? ownerBirthday : "(not set)"),
    "DATE / TIME: SET CLOCK",
    "CLEAR PROFILE"
  };
  for (uint8_t row = 0; row < 4; ++row) {
    String text = String(row == profileMenuCursor ? "> " : "  ") + rows[row];
    oled->drawStr(0, 20 + row * 10, text.c_str());
  }
  oledCentered(63, "UP/DN MOVE  A OPEN  B BACK");
  oled->setFont(u8g2_font_6x12_tr);
}

static void oledRenderClock() {
  if (manualClockEditing) {
    char dateBuf[18], timeBuf[18];
    snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d", manualClockMonth, manualClockDay, manualClockYear);
    int h12 = manualClockHour % 12; if (h12 == 0) h12 = 12;
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d %s", h12, manualClockMinute, manualClockHour >= 12 ? "PM" : "AM");
    oledCentered(13, "SET CLOCK");
    oledCentered(29, String(dateBuf));
    oledCentered(45, String(timeBuf));
    oledCentered(61, String("< ") + manualClockFieldName() + " >");
    return;
  }

  struct tm t;
  oledCentered(13, "CLOCK");
  if (!getLocalTime(&t, 10) || t.tm_year + 1900 < 2020) {
    oledCentered(29, "TIME NOT SET");
    oledCentered(45, "A SET TIME/DATE");
    oledCentered(61, "B BACK");
    return;
  }
  char timeBuf[22], dateBuf[22];
  strftime(timeBuf, sizeof(timeBuf), "%I:%M:%S %p", &t);
  strftime(dateBuf, sizeof(dateBuf), "%a %b %d %Y", &t);
  oledCentered(29, String(timeBuf));
  oledCentered(45, String(dateBuf));
  oledCentered(61, "A SET   B BACK");
}

static void oledRenderLights() {
  oledCentered(13, "LIGHTS / SOLID");
  oledCentered(31, String(brightnessPercent) + "%");
  oledCentered(47, "UP/DN 5   LT/RT 1");
  oledCentered(61, "A SAVE   B BACK");
}

static void oledRenderGames() {
  oledCentered(13, "HAPPY ARCADE");
  oledCentered(29, "7 MINI GAMES");
  oledCentered(45, "A PLAY");
  oledCentered(61, "B BACK");
}

static void oledRenderSettings() {
  if (settingsEditor == 0) {
    static const char *items[] = {"ALARM", "TIMER"};
    oledCentered(13, "SETTINGS");
    for (uint8_t row=0; row<2; ++row) {
      String text = String(row == settingsCursor ? "> " : "  ") + items[row];
      oledCentered(33 + row*20, text);
    }
    oledCentered(61, "B BACK");
    return;
  }

  if (settingsEditor == 1) {
    char timeBuf[16];
    int h12 = editAlarmHour % 12; if (h12 == 0) h12 = 12;
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d %s", h12, editAlarmMinute, editAlarmHour >= 12 ? "PM" : "AM");
    oledCentered(13, "ALARM");
    oledCentered(29, editAlarmEnabled ? "ON" : "OFF");
    oledCentered(45, String(timeBuf));
    oledCentered(61, settingsField==0 ? "< ON/OFF >" : (settingsField==1 ? "< HOUR >" : "< MINUTE >"));
    return;
  }

  oledCentered(13, "TIMER");
  oledCentered(29, editTimerEnabled ? "RUN ON SAVE" : "OFF");
  oledCentered(45, String(editTimerMinutes) + " MINUTES");
  oledCentered(61, settingsField==0 ? "< ON/OFF >" : "< MINUTES >");
}


// FUEL_GAUGE_PATCH_V1
// -----------------------------
// Fuel Gauge / battery diagnostics
// -----------------------------
static constexpr uint8_t BATTERY_ADC_PIN = 3;
static constexpr float BATTERY_DIVIDER_RATIO = 2.0f;
static constexpr float BATTERY_CAL_FACTOR = 0.802f;

static void fuelGaugeInit() {
  pinMode(BATTERY_ADC_PIN, INPUT);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
}

static uint16_t batteryAdcMillivolts() {
  uint32_t total = 0;
  static constexpr uint8_t samples = 12;
  for (uint8_t i=0; i<samples; ++i) {
    total += analogReadMilliVolts(BATTERY_ADC_PIN);
    delayMicroseconds(250);
  }
  return (uint16_t)(total / samples);
}

static float batteryFilteredVoltage = 0.0f;
static bool batteryFilterReady = false;
static unsigned long batteryFilterLastMs = 0;
static constexpr unsigned long BATTERY_FILTER_INTERVAL_MS = 1000UL;

static float batteryVoltage() {
  unsigned long now = millis();
  if (batteryFilterReady && now - batteryFilterLastMs < BATTERY_FILTER_INTERVAL_MS) {
    return batteryFilteredVoltage;
  }

  float rawVolts = ((float)batteryAdcMillivolts() / 1000.0f) * BATTERY_DIVIDER_RATIO * BATTERY_CAL_FACTOR;
  batteryFilterLastMs = now;

  if (!batteryFilterReady) {
    batteryFilteredVoltage = rawVolts;
    batteryFilterReady = true;
  } else {
    // 1/4 new + 3/4 history. Smooth enough to stop percentage chatter while
    // still following real charging/discharge changes within a few seconds.
    batteryFilteredVoltage = batteryFilteredVoltage * 0.75f + rawVolts * 0.25f;
  }
  return batteryFilteredVoltage;
}

static bool batteryReadingPlausible(float v) {
  return v >= 2.8f && v <= 4.45f;
}

static uint8_t batteryPercentFromVoltage(float v) {
  struct Pt { float v; uint8_t pct; };
  static const Pt curve[] = {
    {4.20f,100}, {4.10f,90}, {4.00f,80}, {3.92f,70},
    {3.85f,60}, {3.79f,50}, {3.75f,40}, {3.70f,30},
    {3.65f,20}, {3.55f,10}, {3.40f,0}
  };
  if (v >= curve[0].v) return 100;
  const size_t n = sizeof(curve)/sizeof(curve[0]);
  if (v <= curve[n-1].v) return 0;
  for (size_t i=0; i<n-1; ++i) {
    if (v <= curve[i].v && v >= curve[i+1].v) {
      float span = curve[i].v - curve[i+1].v;
      float f = span > 0.0f ? (v - curve[i+1].v) / span : 0.0f;
      float pct = curve[i+1].pct + f * (float)(curve[i].pct - curve[i+1].pct);
      if (pct < 0.0f) pct = 0.0f;
      if (pct > 100.0f) pct = 100.0f;
      return (uint8_t)(pct + 0.5f);
    }
  }
  return 0;
}


// HAPPYJARZ_LOW_BATTERY_WARNING_V1
static constexpr uint8_t HJ_LOW_BATTERY_ENTER_PCT = 10;
static constexpr uint8_t HJ_LOW_BATTERY_CLEAR_PCT = 15;
static int16_t hjLowBatteryPercent = -1;
static unsigned long hjLowBatteryLastCheckMs = 0;
static constexpr unsigned long HJ_LOW_BATTERY_CHECK_MS = 5000UL;

static void hjServiceLowBattery() {
  unsigned long now = millis();
  if (hjLowBatteryLastCheckMs && now - hjLowBatteryLastCheckMs < HJ_LOW_BATTERY_CHECK_MS) return;
  hjLowBatteryLastCheckMs = now;

  float volts = batteryVoltage();
  if (!batteryReadingPlausible(volts)) {
    // Sensor is still unverified/calibrating. Never raise a false alarm.
    hjLowBatteryPercent = -1;
    hjLowBatteryWarning = false;
    return;
  }

  uint8_t pct = batteryPercentFromVoltage(volts);
  hjLowBatteryPercent = (int16_t)pct;

  if (!hjLowBatteryWarning && pct <= HJ_LOW_BATTERY_ENTER_PCT) {
    hjLowBatteryWarning = true;
    oledDirty = true;
    if (Serial) {
      Serial.print("HJ|BATTERY_WARNING|state=LOW|percent=");
      Serial.println(pct);
    }
  } else if (hjLowBatteryWarning && pct >= HJ_LOW_BATTERY_CLEAR_PCT) {
    hjLowBatteryWarning = false;
    oledDirty = true;
    if (Serial) {
      Serial.print("HJ|BATTERY_WARNING|state=CLEAR|percent=");
      Serial.println(pct);
    }
  }
}

static bool hjLowBatteryFlashOn() {
  // Calm, obvious 1 Hz beacon: half second red / half second normal.
  return ((millis() / 500UL) & 1UL) == 0UL;
}

static void hjRenderLowBatteryWarning() {
  if (!oled) return;
  oled->clearBuffer();
  oled->setFont(u8g2_font_6x12_tr);
  oledCentered(13, "LOW BATTERY");
  oled->setFont(u8g2_font_9x15B_tr);
  oledCentered(39, "CHARGE ME!!!");
  oled->setFont(u8g2_font_6x12_tr);
  if (hjLowBatteryPercent >= 0) {
    oledCentered(61, String(hjLowBatteryPercent) + "%   USB ->");
  } else {
    oledCentered(61, "USB ->");
  }
}

static bool usbDataLinked() {
  // Native USB CDC can report a host/data link. A power-only charger may not.
  return (bool)Serial;
}

static String oledHomePowerText() {
  float volts = batteryVoltage();
  bool usbPower = usbDataLinked();

  String text = "A MENU  ";
  if (usbPower) {
    uint8_t page = (uint8_t)((millis() / 2500UL) % 3UL);
    if (page == 0) {
      text += "BAT ";
      if (batteryReadingPlausible(volts)) {
        text += String(batteryPercentFromVoltage(volts));
        text += "%";
      } else {
        text += "--%";
      }
    } else if (page == 1) text += "PWR USB";
    else text += "CHG ?";
  } else {
    uint8_t page = (uint8_t)((millis() / 2500UL) % 3UL);
    if (page == 0) {
      text += "BAT ";
      if (batteryReadingPlausible(volts)) {
        text += String(batteryPercentFromVoltage(volts));
        text += "%";
      } else {
        text += "--%";
      }
    } else if (page == 1) {
      text += "V ";
      text += String(volts, 2);
    } else {
      text += "PWR BAT";
    }
  }
  return text;
}

static void printPowerStatus() {
  uint16_t adcMv = batteryAdcMillivolts();
  float volts = batteryVoltage();
  if (!batteryReadingPlausible(volts)) {
    Serial.printf("HJ|POWER|sensor=UNVERIFIED|adc_mv=%u|voltage=%.3f|percent=-1|usb_data=%d|charge=HW_ONLY\n",
                  adcMv, volts, usbDataLinked()?1:0);
    return;
  }
  Serial.printf("HJ|POWER|sensor=OK|adc_mv=%u|voltage=%.3f|percent=%u|usb_data=%d|charge=HW_ONLY\n",
                adcMv, volts, batteryPercentFromVoltage(volts), usbDataLinked()?1:0);
}

static void oledRenderPower() {
  float volts = batteryVoltage();
  oledCentered(13, "FUEL GAUGE");
  if (!batteryReadingPlausible(volts)) {
    oledCentered(29, "BAT SENSOR CHECK");
    oledCentered(45, "GPIO3 ADC");
  } else {
    char buf[24];
    snprintf(buf, sizeof(buf), "BAT %.2fV  %u%%", volts, batteryPercentFromVoltage(volts));
    oledCentered(29, String(buf));
    oledCentered(45, usbDataLinked() ? "USB DATA IN" : "USB DATA OUT");
  }
  oledCentered(61, "CHARGE: HW ONLY");
}

static constexpr uint8_t INFO_PAGE_COUNT = 13;

static void oledRenderInfo() {
  oledCentered(10, "INFO / MANUAL");
  switch (infoPage % INFO_PAGE_COUNT) {
    case 0:
      oledCentered(25, "HAPPY JARZ");
      oledCentered(39, "STANDALONE CONSOLE");
      oledCentered(53, "A/RIGHT NEXT");
      break;
    case 1:
      oledCentered(25, "HOME CONTROLS");
      oledCentered(39, "UP/DN PATTERN");
      oledCentered(53, "A MENU  B FREE");
      break;
    case 2:
      oledCentered(25, "HOME COLORS");
      oledCentered(39, "LEFT = LIGHT 1");
      oledCentered(53, "RIGHT = LIGHT 2");
      break;
    case 3:
      oledCentered(25, "LIGHTS / SOLID");
      oledCentered(39, "UP/DN +/-5%");
      oledCentered(53, "LT/RT +/-1%");
      break;
    case 4:
      oledCentered(25, "BRIGHTNESS");
      oledCentered(39, "0 - 100% RANGE");
      oledCentered(53, "A SAVE  B BACK");
      break;
    case 5:
      oledCentered(25, "CLOCK / DATE");
      oledCentered(39, "A = SET CLOCK");
      oledCentered(53, "NO PC REQUIRED");
      break;
    case 6:
      oledCentered(25, "SETTINGS");
      oledCentered(39, "ALARM + TIMER");
      oledCentered(53, "BOARD LOCAL");
      break;
    case 7:
      oledCentered(25, "HAPPY ARCADE");
      oledCentered(39, "7 MINI GAMES");
      oledCentered(53, "LONG B = EXIT");
      break;
    case 8:
      oledCentered(25, "SCREENSAVERS");
      oledCentered(39, "SAYINGS + ART");
      oledCentered(53, "30 SEC IDLE");
      break;
    case 9:
      oledCentered(25, "POWER / BATTERY");
      oledCentered(39, "HOME SHOWS STATUS");
      oledCentered(53, "GPIO3 FUEL GAUGE");
      break;
    case 10:
      oledCentered(25, "USB + WIFI");
      oledCentered(39, "PC OPTIONAL");
      oledCentered(53, "LOCAL MODE FALLBACK");
      break;
    case 11:
      oledCentered(25, "SYSTEM");
      oledCentered(39, String("FW ") + HJ_FW_VERSION);
      oledCentered(53, "ESP32-S3 SUPERMINI");
      break;
    default:
      oled->setFont(u8g2_font_5x8_tf);
      oledCentered(20, "BUILD SPEC");
      oledCentered(31, "ESP32-S3 + OLED + 2 APA106");
      oledCentered(42, "TOUCH LIGHTS CLOCK GAMES");
      oledCentered(53, "MADE BY: quarterbitgames");
      oled->setFont(u8g2_font_6x12_tr);
      break;
  }
  char pageBuf[16];
  snprintf(pageBuf, sizeof(pageBuf), "%u/%u", (unsigned)(infoPage + 1), (unsigned)INFO_PAGE_COUNT);
  oled->setFont(u8g2_font_5x8_tf);
  oled->drawStr(108, 63, pageBuf);
  oled->setFont(u8g2_font_6x12_tr);
}

static void oledRenderSystem() {
  oledCentered(13, "SYSTEM");
  oledCentered(29, hjSerialId() + "  FW " + HJ_FW_VERSION);
  oledCentered(45, WiFi.status()==WL_CONNECTED ? "WIFI CONNECTED" : "WIFI OFFLINE");
  oledCentered(61, timezoneName);
}

static void uiOpenMainMenu() {
  uiScreen = UI_MAIN_MENU;
  uiCursor = 0;
  uiScroll = 0;
  oledDirty = true;
}

static void uiGoHome() {
  uiScreen = UI_HOME;
  uiCursor = 0;
  uiScroll = 0;
  oledDirty = true;
}

static void uiSelectMain() {
  switch (uiCursor) {
    case 0: clockReturnScreen = UI_MAIN_MENU; uiScreen = UI_CLOCK; break;
    case 1: uiScreen = UI_LIGHTS; break;
    case 2: uiScreen = UI_GAMES; break;
    case 3: uiScreen = UI_SETTINGS; break;
    case 4: uiScreen = UI_POWER; break;
    case 5: uiScreen = UI_SYSTEM; break;
    case 6: infoPage = 0; uiScreen = UI_INFO; break;
    default: profileMenuCursor = 0; profileEditor = 0; uiScreen = UI_PROFILE; break;
  }
  oledDirty = true;
}

static void serviceOled() {
  if (hjSleepActive) return;
  if (hjAccessoryPaused) return;
  if (!oledReady || !oled) return;
  unsigned long now = millis();
  serviceScreensaverClock();
  if (hjStartupCountdownActive &&
      (long)(millis() - hjStartupCountdownUntilMs) >= 0) {
    hjStartupCountdownActive = false;
    oledDirty = true;
  }

  unsigned long cadence = hjScreensaverActive ? 45UL : 250UL;
  if (!oledDirty && now - oledLastDrawMs < cadence) return;
  oledLastDrawMs = now;
  oledDirty = false;
  oled->clearBuffer();
  oled->setFont(u8g2_font_6x12_tr);

  if (hjStartupCountdownActive) {
    oledRenderBirthdayCountdown(hjStartupCountdownDays);
  } else if (hjScreensaverActive) {
    oledRenderScreensaver();
    if (hjScreensaverMode == 0) hjMarqueeX -= 2;
    else if (hjScreensaverMode == 1) hjArtStep += hjSpiralSpeed;
    else if (hjScreensaverMode == 2) hjArtStep += hjTrippySpeed;
    else if (hjScreensaverMode == 3) {
      for (uint8_t ps=0; ps<hjParticleSpeed; ++ps) hjParticleStepOnce();
      hjArtStep++;
    } else {
      hjArtStep += hjVisualSpeed;
    }
  } else {
    switch (uiScreen) {
      case UI_HOME: oledRenderHome(); break;
      case UI_MAIN_MENU: oledRenderMainMenu(); break;
      case UI_CLOCK: oledRenderClock(); break;
      case UI_PROFILE: oledRenderProfile(); break;
      case UI_LIGHTS: oledRenderLights(); break;
      case UI_GAMES: oledRenderGames(); break;
      case UI_SETTINGS: oledRenderSettings(); break;
      case UI_POWER: oledRenderPower(); break;
    case UI_INFO: oledRenderInfo(); break;
    default: oledRenderSystem(); break;
    }
  }
  hjOledPresent();
}

// -----------------------------
// Local touch behavior
// -----------------------------
static bool latched[INPUT_COUNT] = {false,false,false,false,false,false};
static const Rgb palette[] = {
  {255,0,0},{255,80,0},{255,180,0},{0,255,0},{0,160,255},{0,0,255},{140,0,255},{255,0,160},{255,255,255}
};
static int paletteIndex1=6, paletteIndex2=4;
static int localPatternIndex=0;


static void hjEnterSleep() {
  if (hjSleepActive) return;

  // Blank all four lamps once, then freeze LED/pattern output.
  Rgb blank[LED_COUNT] = {};
  writeFrame(blank, 0);

  if (oledReady && oled) {
    oled->setPowerSave(1);
  }

  hjSleepActive = true;
  if (Serial) Serial.println("HJ|SLEEP|state=ASLEEP|reason=IDLE_1H");
}

static void hjWakeFromSleep(const char *reason) {
  hjSleepLastActivityMs = millis();
  if (!hjSleepActive) return;

  hjSleepActive = false;

  if (oledReady && oled) {
    oled->setPowerSave(0);
    oledDirty = true;
  }

  resetPatternEngine();
  if (patternName == "SOLID") showLeds();

  if (Serial) {
    Serial.print("HJ|SLEEP|state=AWAKE|reason=");
    Serial.println(reason ? reason : "UNKNOWN");
  }
}

static void hjServiceSleep() {
  if (hjSleepActive) return;
  unsigned long now = millis();
  if (now - hjSleepLastActivityMs >= HJ_SLEEP_IDLE_MS) {
    hjEnterSleep();
  }
}

static void serviceInputs() {
  bool q[INPUT_COUNT];
  for (uint8_t i=0;i<INPUT_COUNT;++i) q[i]=updateInputState(i);

  bool hjAnyPhysicalPress = false;
  for (uint8_t i=0; i<INPUT_COUNT; ++i) {
    if (q[i] && !latched[i]) {
      hjAnyPhysicalPress = true;
      break;
    }
  }
  if (hjAnyPhysicalPress) {
    hjSleepLastActivityMs = millis();
    if (hjSleepActive) {
      hjWakeFromSleep("TOUCH");
      for (uint8_t i=0; i<INPUT_COUNT; ++i) latched[i] = q[i];
      return;
    }
  }

  // HAPPYJARZ_UNIFIED_INPUT_ORDER_V1
  // Physical-only sleep wake has already run. From here down, physical touch
  // and host/gamepad KEY events are intentionally the same logical controls.
  for (uint8_t i=0; i<INPUT_COUNT; ++i) {
    if (hjHostKeyPending[i]) {
      q[i] = true;
      hjHostKeyPending[i] = false;
    }
  }


  if (!hjArcadeActive() && !hjScreensaverActive && uiScreen == UI_GAMES &&
      q[IN_A] && !latched[IN_A]) {
    hjArcadeEnter();
    for (uint8_t i=0;i<INPUT_COUNT;++i) latched[i]=q[i];
    oledDirty = true;
    return;
  }

  if (hjArcadeActive()) {
    if (q[IN_UP] && !latched[IN_UP]) hjArcadeButton(HJ_BTN_UP);
    if (q[IN_DOWN] && !latched[IN_DOWN]) hjArcadeButton(HJ_BTN_DOWN);
    if (q[IN_LEFT] && !latched[IN_LEFT]) hjArcadeButton(HJ_BTN_LEFT);
    if (q[IN_RIGHT] && !latched[IN_RIGHT]) hjArcadeButton(HJ_BTN_RIGHT);
    if (q[IN_A] && !latched[IN_A]) hjArcadeButton(HJ_BTN_A);
    if (q[IN_B] && !latched[IN_B]) hjArcadeButton(HJ_BTN_B);

    if (q[IN_B] && !bHomeSent && inputs[IN_B].qualifiedMs &&
        millis() - inputs[IN_B].qualifiedMs >= 1000) {
      bHomeSent = true;
      hjArcadeButton(HJ_BTN_HOME);
      hjArcadeExit();
      uiScreen = UI_GAMES;
      uiCursor = 0;
      uiScroll = 0;
      oledDirty = true;
    }

    for (uint8_t i=0;i<INPUT_COUNT;++i) latched[i]=q[i];
    if (inputStream && Serial && millis()-lastInputStreamMs>=100) {
      lastInputStreamMs=millis();
      printInputTelemetry();
    }
    return;
  }


  if (inputMode == "JAR" && uiScreen == UI_INFO) {
    bool next = (q[IN_RIGHT] && !latched[IN_RIGHT]) ||
                (q[IN_DOWN] && !latched[IN_DOWN]) ||
                (q[IN_A] && !latched[IN_A]);
    bool prev = (q[IN_LEFT] && !latched[IN_LEFT]) ||
                (q[IN_UP] && !latched[IN_UP]);
    if (next) {
      infoPage = (infoPage + 1) % INFO_PAGE_COUNT;
      oledDirty = true;
    } else if (prev) {
      infoPage = (infoPage + INFO_PAGE_COUNT - 1) % INFO_PAGE_COUNT;
      oledDirty = true;
    } else if (q[IN_B] && !latched[IN_B]) {
      uiOpenMainMenu();
    }
    for (uint8_t i=0;i<INPUT_COUNT;++i) latched[i]=q[i];
    return;
  }


  // PROFILE is editable entirely from the jar's six capacitive controls.
  if (inputMode == "JAR" && !hjScreensaverActive && uiScreen == UI_PROFILE) {
    if (profileEditor == 1) {
      if (q[IN_LEFT] && !latched[IN_LEFT] && profileEditChar > 0) {
        profileEditChar--;
        oledDirty = true;
      }
      if (q[IN_RIGHT] && !latched[IN_RIGHT]) {
        if (profileEditChar + 1 < profileEditName.length()) {
          profileEditChar++;
        } else if (profileEditName.length() < 16) {
          profileEditName += ' ';
          profileEditChar++;
        }
        oledDirty = true;
      }
      if (q[IN_UP] && !latched[IN_UP]) profileCycleNameChar(+1);
      if (q[IN_DOWN] && !latched[IN_DOWN]) profileCycleNameChar(-1);
      if (q[IN_A] && !latched[IN_A]) profileSaveName();
      if (q[IN_B] && !latched[IN_B]) { profileEditor = 0; oledDirty = true; }
    } else if (profileEditor == 3) {
      if (q[IN_A] && !latched[IN_A]) { profileClear(); profileEditor = 0; }
      if (q[IN_B] && !latched[IN_B]) { profileEditor = 0; oledDirty = true; }
    } else if (profileEditor == 2) {
      if (q[IN_LEFT] && !latched[IN_LEFT]) profileEditChar = (profileEditChar + 3) % 4;
      if (q[IN_RIGHT] && !latched[IN_RIGHT]) profileEditChar = (profileEditChar + 1) % 4;
      if (q[IN_UP] && !latched[IN_UP]) profileAdjustBirthday(+1);
      if (q[IN_DOWN] && !latched[IN_DOWN]) profileAdjustBirthday(-1);
      if (q[IN_A] && !latched[IN_A]) profileSaveBirthday();
      if (q[IN_B] && !latched[IN_B]) { profileEditor = 0; oledDirty = true; }
    } else {
      if (q[IN_UP] && !latched[IN_UP]) profileMenuCursor = (profileMenuCursor + 3) % 4;
      if (q[IN_DOWN] && !latched[IN_DOWN]) profileMenuCursor = (profileMenuCursor + 1) % 4;
      if (q[IN_A] && !latched[IN_A]) {
        if (profileMenuCursor == 0) profileBeginNameEdit();
        else if (profileMenuCursor == 1) profileBeginBirthdayEdit();
        else if (profileMenuCursor == 2) {
          clockReturnScreen = UI_PROFILE;
          uiScreen = UI_CLOCK;
          oledDirty = true;
        } else { profileEditor = 3; oledDirty = true; }
      }
      if ((q[IN_B] && !latched[IN_B]) || (q[IN_LEFT] && !latched[IN_LEFT])) uiOpenMainMenu();
      oledDirty = true;
    }
    for (uint8_t i=0;i<INPUT_COUNT;++i) latched[i]=q[i];
    return;
  }

  // SETTINGS owns touch input before downstream menu/detail routing.
  if (inputMode == "JAR" && !hjScreensaverActive && uiScreen == UI_SETTINGS) {
    if (settingsEditor == 0) {
      if (q[IN_UP] && !latched[IN_UP]) { settingsCursor=(settingsCursor+1)%2; oledDirty=true; }
      if (q[IN_DOWN] && !latched[IN_DOWN]) { settingsCursor=(settingsCursor+1)%2; oledDirty=true; }
      if (q[IN_A] && !latched[IN_A]) settingsOpenEditor(settingsCursor+1);
      if ((q[IN_B] && !latched[IN_B]) || (q[IN_LEFT] && !latched[IN_LEFT])) uiOpenMainMenu();
    } else {
      uint8_t fieldCount = settingsEditor==1 ? 3 : 2;
      if (q[IN_LEFT] && !latched[IN_LEFT]) { settingsField=(settingsField+fieldCount-1)%fieldCount; oledDirty=true; }
      if (q[IN_RIGHT] && !latched[IN_RIGHT]) { settingsField=(settingsField+1)%fieldCount; oledDirty=true; }
      if (q[IN_UP] && !latched[IN_UP]) settingsAdjust(+1);
      if (q[IN_DOWN] && !latched[IN_DOWN]) settingsAdjust(-1);
      if (q[IN_A] && !latched[IN_A]) settingsSaveEditor();
      if (q[IN_B] && !latched[IN_B]) settingsCancelEditor();
    }
    for (uint8_t i=0;i<INPUT_COUNT;++i) latched[i]=q[i];
    return;
  }


  // CLOCK owns touch input before downstream menu/detail routing.
  if (inputMode == "JAR" && !hjScreensaverActive && uiScreen == UI_CLOCK) {
    if (manualClockEditing) {
      if (q[IN_LEFT] && !latched[IN_LEFT]) { manualClockField=(manualClockField+4)%5; oledDirty=true; }
      if (q[IN_RIGHT] && !latched[IN_RIGHT]) { manualClockField=(manualClockField+1)%5; oledDirty=true; }
      if (q[IN_UP] && !latched[IN_UP]) manualClockAdjust(+1);
      if (q[IN_DOWN] && !latched[IN_DOWN]) manualClockAdjust(-1);
      if (q[IN_A] && !latched[IN_A]) {
        if (!manualClockSave()) Serial.println("HJ|ERR|message=manual clock save failed");
      }
      if (q[IN_B] && !latched[IN_B]) { manualClockEditing=false; oledDirty=true; }
    } else {
      if (q[IN_A] && !latched[IN_A]) manualClockBeginEdit();
      if ((q[IN_B] && !latched[IN_B]) || (q[IN_LEFT] && !latched[IN_LEFT])) uiClockBack();
    }
    for (uint8_t i=0;i<INPUT_COUNT;++i) latched[i]=q[i];
    return;
  }

  // MENU/GAME are temporary desktop/service modes. USB loss changes only
  // input ownership; never dim the OLED, turn off LEDs, or rewrite UI state.
  if (inputMode != "JAR" && !Serial) {
    inputMode = "JAR";
    oledDirty = true;
  }

  bool anyNewPress=false;
  for (uint8_t i=0;i<INPUT_COUNT;++i) if (q[i] && !latched[i]) anyNewPress=true;
  if (anyNewPress) hjScreensaverTouch();
  if (anyNewPress && hjStartupCountdownActive) {
    hjStartupCountdownActive = false;
    oledDirty = true;
  }

  if (hjScreensaverActive) {
    if (q[IN_B] && !latched[IN_B]) hjScreensaverExit();
    else if (q[IN_RIGHT] && !latched[IN_RIGHT]) hjScreensaverNext();
    else if (q[IN_LEFT] && !latched[IN_LEFT]) hjScreensaverPrev();
    else if (hjScreensaverMode == 1) {
      if (q[IN_UP] && !latched[IN_UP]) { if (hjSpiralSpeed < 8) hjSpiralSpeed++; }
      else if (q[IN_DOWN] && !latched[IN_DOWN]) { if (hjSpiralSpeed > 1) hjSpiralSpeed--; }
      else if (q[IN_A] && !latched[IN_A]) { hjReseedSpiral(); }
    } else if (hjScreensaverMode == 2) {
      if (q[IN_UP] && !latched[IN_UP]) { if (hjTrippySpeed < 8) hjTrippySpeed++; }
      else if (q[IN_DOWN] && !latched[IN_DOWN]) { if (hjTrippySpeed > 1) hjTrippySpeed--; }
      else if (q[IN_A] && !latched[IN_A]) { hjReseedTrippy(); }
    } else if (hjScreensaverMode == 3) {
      if (q[IN_UP] && !latched[IN_UP]) { if (hjParticleSpeed < 8) hjParticleSpeed++; }
      else if (q[IN_DOWN] && !latched[IN_DOWN]) { if (hjParticleSpeed > 1) hjParticleSpeed--; }
      else if (q[IN_A] && !latched[IN_A]) { hjReseedParticles(); }
    } else if (hjScreensaverMode >= 4) {
      if (q[IN_UP] && !latched[IN_UP]) { if (hjVisualSpeed < 8) hjVisualSpeed++; }
      else if (q[IN_DOWN] && !latched[IN_DOWN]) { if (hjVisualSpeed > 1) hjVisualSpeed--; }
      else if (q[IN_A] && !latched[IN_A]) { hjReseedVisualSaver(); }
    }

    // While the saver owns the screen, do not run HOME/menu actions.
  } else if (inputMode == "JAR") {
    if (uiScreen == UI_HOME) {
      if (q[IN_A] && !latched[IN_A]) {
        uiOpenMainMenu();
      }
      if (q[IN_UP] && !latched[IN_UP]) {
        localPatternIndex=(localPatternIndex+1)%PATTERN_COUNT;
        hjSetPattern(PATTERN_NAMES[localPatternIndex]);
        persistSettings();
        oledDirty=true;
      }
      if (q[IN_DOWN] && !latched[IN_DOWN]) {
        localPatternIndex=(localPatternIndex+PATTERN_COUNT-1)%PATTERN_COUNT;
        hjSetPattern(PATTERN_NAMES[localPatternIndex]);
        persistSettings();
        oledDirty=true;
      }
      if (q[IN_LEFT] && !latched[IN_LEFT]) {
        paletteIndex1=(paletteIndex1+1)%9;
        Rgb c=palette[paletteIndex1];
        hjSetLed(1,c.r,c.g,c.b);
        persistSettings();
        oledDirty=true;
      }
      if (q[IN_RIGHT] && !latched[IN_RIGHT]) {
        paletteIndex2=(paletteIndex2+1)%9;
        Rgb c=palette[paletteIndex2];
        hjSetLed(2,c.r,c.g,c.b);
        persistSettings();
        oledDirty=true;
      }
    } else if (uiScreen == UI_MAIN_MENU) {
      if (q[IN_UP] && !latched[IN_UP]) {
        uiCursor=(uiCursor+7)%8;
        oledDirty=true;
      }
      if (q[IN_DOWN] && !latched[IN_DOWN]) {
        uiCursor=(uiCursor+1)%8;
        oledDirty=true;
      }
      if (q[IN_A] && !latched[IN_A]) {
        uiSelectMain();
      }
      if ((q[IN_B] && !latched[IN_B]) || (q[IN_LEFT] && !latched[IN_LEFT])) {
        uiGoHome();
      }
    } else if (uiScreen == UI_LIGHTS) {
      // SOLID brightness editor. All changes preview immediately on the LEDs.
      int nextBrightness = (int)brightnessPercent;
      bool changed = false;
      if (q[IN_UP] && !latched[IN_UP]) { nextBrightness += 5; changed = true; }
      if (q[IN_DOWN] && !latched[IN_DOWN]) { nextBrightness -= 5; changed = true; }
      if (q[IN_RIGHT] && !latched[IN_RIGHT]) { nextBrightness += 1; changed = true; }
      if (q[IN_LEFT] && !latched[IN_LEFT]) { nextBrightness -= 1; changed = true; }
      if (changed) {
        if (nextBrightness < 0) nextBrightness = 0;
        if (nextBrightness > 100) nextBrightness = 100;
        hjSetPattern("SOLID");
        hjSetBrightness((uint8_t)nextBrightness);
        oledDirty = true;
      }
      if (q[IN_A] && !latched[IN_A]) {
        persistSettings();
        oledDirty = true;
      }
      if (q[IN_B] && !latched[IN_B]) {
        persistSettings();
        uiOpenMainMenu();
      }
    } else {
      // DETAIL/STATUS MODE: B/LEFT return to menu.
      if ((q[IN_B] && !latched[IN_B]) || (q[IN_LEFT] && !latched[IN_LEFT])) {
        uiOpenMainMenu();
      }
    }
  }

  // Send edge events for menu/game layers and desktop diagnostics.
  for (uint8_t i=0;i<INPUT_COUNT;++i) {
    if (q[i] && !latched[i]) { Serial.print("HJ|EVENT|input="); Serial.println(inputs[i].name); }
  }
  if (q[IN_B] && !bHomeSent && inputs[IN_B].qualifiedMs && millis()-inputs[IN_B].qualifiedMs>=1000) {
    bHomeSent=true; Serial.println("HJ|EVENT|input=home");
  }
  for (uint8_t i=0;i<INPUT_COUNT;++i) latched[i]=q[i];

  if (inputStream && Serial && millis()-lastInputStreamMs>=200) { lastInputStreamMs=millis(); printInputTelemetry(); }
}

// -----------------------------
// USB serial protocol
// -----------------------------
static String rxLine;
static bool touchStreamCompat = false;
static unsigned long lastTouchCompatMs = 0;

static void ack(const String &name){ Serial.print("HJ|ACK|command="); Serial.println(name); }
static void err(const String &msg){ Serial.print("HJ|ERR|message="); Serial.println(msg); }
static void printProfile() {
  Serial.print("HJ|PROFILE|chip="); Serial.print(hjChipId());
  Serial.print("|owner="); Serial.print(ownerName);
  Serial.print("|birthday="); Serial.println(ownerBirthday);
}
static void identity() {
  String chip = hjChipId();
  Serial.print("HJ|IDENTITY|serial="); Serial.print(hjSerialId());
  Serial.print("|chip="); Serial.print(chip);
  Serial.print("|hw="); Serial.print(HJ_HW_VERSION);
  Serial.print("|fw="); Serial.println(HJ_FW_VERSION);
}

static String statusLine(){
  String s="HJ|STATUS|brightness="+String(brightnessPercent)+"|pattern="+patternName;
  s+="|led1="+String(ledColor[0].r)+","+String(ledColor[0].g)+","+String(ledColor[0].b);
  s+="|led2="+String(ledColor[1].r)+","+String(ledColor[1].g)+","+String(ledColor[1].b);
  s+="|led3="+String(ledColor[2].r)+","+String(ledColor[2].g)+","+String(ledColor[2].b);
  s+="|led4="+String(ledColor[3].r)+","+String(ledColor[3].g)+","+String(ledColor[3].b);
  s+="|inputmode="+inputMode;
  return s;
}

static bool parseRgb(const String &line,uint8_t led){
  unsigned int parsedLed=0; int r=-1,g=-1,b=-1;
  int n=sscanf(line.c_str(),"SET LED%u COLOR %d %d %d",&parsedLed,&r,&g,&b);
  if(n!=4 || parsedLed!=led || led<1 || led>LED_COUNT ||
     r<0||r>255||g<0||g>255||b<0||b>255) return false;
  hjSetLed(led,(uint8_t)r,(uint8_t)g,(uint8_t)b); return true;
}

static bool parseHHMM(const String &s,uint8_t &h,uint8_t &m){
  int hh=-1,mm=-1; if(sscanf(s.c_str(),"%d:%d",&hh,&mm)!=2)return false;
  if(hh<0||hh>23||mm<0||mm>59)return false; h=hh;m=mm;return true;
}

static void handleCommand(String line) {
  line.trim(); if(!line.length())return;
  if(hjArtHandleCommand(line)) return;
  if(line=="HELLO"){identity();return;}
  if(line=="PING"){Serial.println("HJ|PONG");return;}
  if(line=="GET PROFILE"){printProfile();return;}
  if(line=="CLEAR PROFILE"){
    ownerName="";
    ownerBirthday="--";
    hjBirthdayCacheMinute=(time_t)-1;
    prefs.begin("happyjarz", false);
    prefs.remove("owner_name");
    prefs.remove("owner_bday");
    prefs.end();
    ack("CLEAR PROFILE");
    printProfile();
    return;
  }
  if(line.startsWith("SET PROFILE ")){
    String payload=line.substring(12);
    int split=payload.indexOf(' ');
    if(split<0){err("profile format is SET PROFILE MM-DD name");return;}
    String birthday=payload.substring(0,split);
    String name=payload.substring(split+1);
    birthday.trim(); name.trim();
    if(birthday!="--" && !hjValidBirthday(birthday)){err("birthday must be MM-DD or --");return;}
    for(size_t i=0;i<name.length();++i){
      char c=name.charAt(i);
      if(c=='|' || c=='\\r' || c=='\\n') name.setCharAt(i,'/');
    }
    if(name.length()>32) name=name.substring(0,32);
    if(!name.length()){err("owner name is required");return;}
    ownerName=name;
    ownerBirthday=birthday;
    hjBirthdayCacheMinute=(time_t)-1;
    prefs.begin("happyjarz", false);
    prefs.putString("owner_name", ownerName);
    prefs.putString("owner_bday", ownerBirthday);
    prefs.end();
    ack("SET PROFILE");
    printProfile();
    return;
  }
  if(line=="GET STATUS"){Serial.println(statusLine());return;}
  if(line=="GET CAPS"){hjPrintCaps();return;}
  if(line=="GET LED FRAME"){hjEmitLedFrameNow();return;}
  if(line=="STREAM LED ON"){
    hjLedStream=true;
    ack("STREAM LED ON");
    hjEmitLedFrameNow();
    return;
  }
  if(line=="STREAM LED OFF"){
    hjLedStream=false;
    ack("STREAM LED OFF");
    return;
  }
  if(line=="GET OLED"){hjEmitOledFrame(true);return;}
  if(line=="STREAM OLED ON"){
    hjOledStream=true;
    ack("STREAM OLED ON");
    hjEmitOledFrame(true);
    return;
  }
  if(line=="STREAM OLED OFF"){
    hjOledStream=false;
    ack("STREAM OLED OFF");
    return;
  }
  if(line.startsWith("KEY ")){
    String key=line.substring(4);
    key.trim();
    int8_t idx=hjHostKeyIndex(key);
    if(idx < 0){
      err("key unsupported by SIMPLE profile");
      return;
    }
    hjHostKeyPending[(uint8_t)idx]=true;
    Serial.print("HJ|EVENT|input=");
    key.toLowerCase();
    Serial.print(key);
    Serial.println("|source=host");
    ack("KEY");
    return;
  }
  if(line=="GET POWER"){printPowerStatus();return;}

  if(line=="GET TOUCH" || line=="GET INPUT"){ printInputTelemetry(); return; }
  if(line=="STREAM TOUCH ON"){touchStreamCompat=false; inputStream=true; ack(line);return;}
  if(line=="STREAM TOUCH OFF"){touchStreamCompat=false; inputStream=false; ack(line);return;}
  if(line=="STREAM INPUT ON"){inputStream=true;ack(line);return;}
  if(line=="STREAM INPUT OFF"){inputStream=false;ack(line);return;}
  if(line.startsWith("SET INPUT MODE ")){String v=line.substring(15);v.trim();if(v=="JAR"||v=="MENU"||v=="GAME"){inputMode=v;ack("SET INPUT MODE");}else err("input mode must be JAR MENU or GAME");return;}
  if(line=="TEST INPUT"||line=="TEST TOUCH"){calibrateInputs();printTouchCalibration();Serial.println("HJ|TEST|input=PASS");return;}

  if(line.startsWith("SET LED1 COLOR ")){uiGoHome();if(parseRgb(line,1)){oledDirty=true;ack("SET LED1 COLOR");}else err("invalid LED1 RGB values");return;}
  if(line.startsWith("SET LED2 COLOR ")){uiGoHome();if(parseRgb(line,2)){oledDirty=true;ack("SET LED2 COLOR");}else err("invalid LED2 RGB values");return;}
  if(line.startsWith("SET LED3 COLOR ")){if(parseRgb(line,3))ack("SET LED3 COLOR");else err("invalid LED3 RGB values");return;}
  if(line.startsWith("SET LED4 COLOR ")){if(parseRgb(line,4))ack("SET LED4 COLOR");else err("invalid LED4 RGB values");return;}
  if(line.startsWith("SET BRIGHTNESS ")){int v=line.substring(15).toInt();if(v<0||v>100)err("brightness must be 0-100");else{hjSetBrightness(v);ack("SET BRIGHTNESS");}return;}
  if(line.startsWith("SET PATTERN ")){String v=line.substring(12);v.trim();if(isPatternName(v)){uiGoHome();hjSetPattern(v);oledDirty=true;ack("SET PATTERN");}else err("unknown pattern");return;}

  if(line=="GET WIFI STATUS"){printWifiStatus();return;}
  if(line=="SCAN WIFI"){scanWifiNetworks();return;}
  if(line.startsWith("SET WIFI SSID ")){wifiSsid=line.substring(14);ack("SET WIFI SSID");return;}
  if(line.startsWith("SET WIFI PASSWORD ")){wifiPassword=line.substring(18);ack("SET WIFI PASSWORD");return;}
  if(line=="SET WIFI CLEAR"){wifiSsid="";wifiPassword="";WiFi.disconnect(true,true);ack(line);return;}
  if(line=="WIFI CONNECT"){connectWifi();ack(line);return;}

  if(line=="GET TIME STATUS"){printTimeStatus();return;}
  if(line.startsWith("SET CLOCK UNIX ")){
    String raw=line.substring(15); raw.trim();
    unsigned long long epoch=strtoull(raw.c_str(), nullptr, 10);
    if(epoch < 1700000000ULL || epoch > 4102444800ULL){err("invalid unix time");return;}
    struct timeval tv; tv.tv_sec=(time_t)epoch; tv.tv_usec=0;
    if(settimeofday(&tv, nullptr)==0){ack("SET CLOCK UNIX");printTimeStatus();}
    else err("settimeofday failed");
    return;
  }
  if(line.startsWith("SET TIMEZONE ")){timezoneName=line.substring(13);timezoneName.trim();startTimeSync();ack("SET TIMEZONE");return;}
  if(line=="TIME SYNC"){if(WiFi.status()==WL_CONNECTED){startTimeSync();ack(line);}else err("wifi not connected");return;}

  if(line=="GET ALARM"){Serial.printf("HJ|ALARM|enabled=%d|time=%02u:%02u\n",alarmEnabled?1:0,alarmHour,alarmMinute);return;}
  if(line.startsWith("SET ALARM ")){
    String v=line.substring(10);v.trim();
    if(v=="ON"){alarmEnabled=true;ack("SET ALARM ON");return;}
    if(v=="OFF"){alarmEnabled=false;ack("SET ALARM OFF");return;}
    uint8_t h,m;if(parseHHMM(v,h,m)){alarmHour=h;alarmMinute=m;ack("SET ALARM");}else err("alarm time must be HH:MM");return;
  }

  if(line=="GET TIMER"){Serial.printf("HJ|TIMER|enabled=%d|minutes=%lu|remaining=%lu\n",timerEnabled?1:0,(unsigned long)timerMinutes,(unsigned long)timerRemainingSeconds());return;}
  if(line.startsWith("SET TIMER MINUTES ")){long v=line.substring(18).toInt();if(v<1||v>10080)err("timer minutes must be 1-10080");else{timerMinutes=v;timerEnabled=true;timerStartedMs=millis();ack("SET TIMER MINUTES");}return;}
  if(line=="SET TIMER OFF"){timerEnabled=false;ack(line);return;}

  if(line=="GET DISPLAY"){Serial.printf("HJ|DISPLAY|brightness=%u|screensaver=%s|delay=%u\n",displayBrightness,screensaver.c_str(),screensaverDelayMin);return;}
  if(line.startsWith("SET DISPLAY BRIGHTNESS ")){int v=line.substring(23).toInt();if(v<0||v>100)err("display brightness must be 0-100");else{displayBrightness=100;oledApplyBrightness();oledDirty=true;ack("SET DISPLAY BRIGHTNESS FIXED 100");}return;}
  if(line.startsWith("SET SCREENSAVER DELAY ")){int v=line.substring(22).toInt();if(v<0||v>1440)err("screensaver delay must be 0-1440");else{screensaverDelayMin=v;ack("SET SCREENSAVER DELAY");}return;}
  if(line.startsWith("SET SCREENSAVER ")){String v=line.substring(16);v.trim();if(v=="OFF"||v=="CLOCK"||v=="PLASMA"||v=="STARS"||v=="BOUNCE"){screensaver=v;ack("SET SCREENSAVER");}else err("unknown screensaver");return;}

  if(line=="TEST RGB"){
    Rgb saved[LED_COUNT];
    for(uint8_t i=0;i<LED_COUNT;++i) saved[i]=ledColor[i];
    uint8_t sb=brightnessPercent; String sp=patternName;
    patternName="SOLID"; brightnessPercent=35;
    const Rgb tests[]={{255,0,0},{0,255,0},{0,0,255},{255,255,255}};
    for(const auto &c:tests){
      for(uint8_t i=0;i<LED_COUNT;++i) ledColor[i]=c;
      showLeds(); delay(350);
    }
    for(uint8_t i=0;i<LED_COUNT;++i) ledColor[i]=saved[i];
    brightnessPercent=sb; patternName=sp; resetPatternEngine();
    if(sp=="SOLID") showLeds();
    Serial.println("HJ|TEST|rgb=PASS"); return;
  }
  if(line=="GET CUSTOM SAYINGS"){hjPrintCustomSayings();return;}
  if(line=="CLEAR CUSTOM SAYINGS"){
    for(uint8_t i=0;i<HJ_CUSTOM_SAYING_SLOTS;++i) hjSaveCustomSaying(i,"");
    hjCustomActiveSlot=-1; ack("CLEAR CUSTOM SAYINGS"); return;
  }
  if(line.startsWith("SET SAYING SOURCE ")){
    String v=line.substring(18); v.trim(); v.toUpperCase();
    if(v=="BUILTIN"||v=="CUSTOM"||v=="MIXED"){
      hjSetSayingSource(v); hjPickNextSaying(); oledDirty=true; ack("SET SAYING SOURCE");
    } else err("saying source must be BUILTIN CUSTOM or MIXED");
    return;
  }
  if(line.startsWith("SET CUSTOM SAYING ")){
    String rest=line.substring(18); rest.trim();
    int split=rest.indexOf(' ');
    if(split<1){err("custom saying requires slot and text");return;}
    int slot=rest.substring(0,split).toInt();
    String text=rest.substring(split+1); text.trim();
    text.replace("|","/"); text.replace("\r"," "); text.replace("\n"," ");
    if(slot<1||slot>HJ_CUSTOM_SAYING_SLOTS){err("custom saying slot must be 1-50");return;}
    if(text.length()>96) text=text.substring(0,96);
    hjSaveCustomSaying((uint8_t)(slot-1),text); ack("SET CUSTOM SAYING"); return;
  }
  if(line=="GET SAVER STATUS"){hjPrintSaverStatus();return;}
  if(line=="SAVER ENTER"){
    hjScreensaverEnter(); hjSaverPrepareCurrent(); ack("SAVER ENTER"); hjPrintSaverStatus(); return;
  }
  if(line=="SAVER EXIT"){
    hjScreensaverExit(); ack("SAVER EXIT"); hjPrintSaverStatus(); return;
  }
  if(line=="SAVER NEXT"){
    hjScreensaverNext(); hjSaverPrepareCurrent(); ack("SAVER NEXT"); hjPrintSaverStatus(); return;
  }
  if(line=="SAVER PREV"){
    hjScreensaverPrev(); hjSaverPrepareCurrent(); ack("SAVER PREV"); hjPrintSaverStatus(); return;
  }
  if(line=="SAVER RESEED"){
    hjSaverPrepareCurrent(); ack("SAVER RESEED"); hjPrintSaverStatus(); return;
  }
  if(line=="SAVER SPEED UP"){
    if(hjScreensaverMode==1 && hjSpiralSpeed<8) hjSpiralSpeed++;
    else if(hjScreensaverMode==2 && hjTrippySpeed<8) hjTrippySpeed++;
    else if(hjScreensaverMode==3 && hjParticleSpeed<8) hjParticleSpeed++;
    else if(hjScreensaverMode>=4 && hjVisualSpeed<8) hjVisualSpeed++;
    ack("SAVER SPEED UP"); hjPrintSaverStatus(); return;
  }
  if(line=="SAVER SPEED DOWN"){
    if(hjScreensaverMode==1 && hjSpiralSpeed>1) hjSpiralSpeed--;
    else if(hjScreensaverMode==2 && hjTrippySpeed>1) hjTrippySpeed--;
    else if(hjScreensaverMode==3 && hjParticleSpeed>1) hjParticleSpeed--;
    else if(hjScreensaverMode>=4 && hjVisualSpeed>1) hjVisualSpeed--;
    ack("SAVER SPEED DOWN"); hjPrintSaverStatus(); return;
  }
  if(line.startsWith("SET SAVER MODE ")){
    String v=line.substring(15); v.trim(); v.toUpperCase();
    if(v=="SAYINGS") hjSaverSetMode(0,true);
    else if(v=="SPIRAL") hjSaverSetMode(1,true);
    else if(v=="TRIPPY") hjSaverSetMode(2,true);
    else if(v=="PARTICLES") hjSaverSetMode(3,true);
    else if(v=="BLOOM") hjSaverSetMode(4,true);
    else if(v=="BREATHE") hjSaverSetMode(5,true);
    else if(v=="GLITTER") hjSaverSetMode(6,true);
    else if(v=="ART") hjSaverSetMode(7,true);
    else {err("saver mode must be SAYINGS SPIRAL TRIPPY PARTICLES BLOOM BREATHE GLITTER or ART");return;}
    ack("SET SAVER MODE"); hjPrintSaverStatus(); return;
  }
  if(line=="SAVE"){persistSettings();ack("SAVE");return;}
  err("unknown command");
}

static void serialPoll(){
  while(Serial.available()){
    char c=(char)Serial.read();
    if(c=='\n'){handleCommand(rxLine);rxLine="";}
    else if(c!='\r'){if(rxLine.length()<192)rxLine+=c;else rxLine="";}
  }
  // USB_TOUCH_STREAM_FIX_V1: duplicate compatibility telemetry emitter disabled.
}

// -----------------------------
// Setup / loop
// -----------------------------
void setup(){
  hjSleepLastActivityMs = millis();
  fuelGaugeInit();
  Serial.begin(115200); delay(250); rxLine.reserve(128); randomSeed((uint32_t)micros());
  Serial.print("HJ|BOOT|reset_reason="); Serial.println((int)esp_reset_reason());
  loadSettings();
  displayBrightness=100;
  hjLoadCustomSayings();
  hjArtInit();
  oledInit();
  hjScreensaverTouch();
  if(!initApa106Rmt()) Serial.println("HJ|ERR|message=RMT init failed");
  else {
    // Quiet boot/reconnect: never overwrite user LED colors just because USB
    // power wobbled. Let rails settle, calibrate touch, then restore settings.
    delay(350);
    calibrateInputs();
    printTouchCalibration();
    loadSettings();
    hjStartupCountdownPending = hjValidBirthday(ownerBirthday) &&
                                ((esp_random() % 3U) == 0);
    if(patternName=="OFF") allOff();
    else if(patternName=="SOLID") showLeds();
    else resetPatternEngine();
  }
  if(wifiSsid.length()) connectWifi();
  identity();
}

void loop(){
  serviceAccessoryPause();
  hjServiceLowBattery();
  serialPoll();
  serviceInputs();
  hjServiceSleep();
  servicePattern();
  serviceAlarm();
  serviceTimer();
  serviceStartupBirthdayCountdown();
  if (hjArcadeActive() && !hjSleepActive && !hjStartupCountdownActive) hjArcadeService(millis());
  else serviceOled();
  static bool timeStarted=false;
  if (WiFi.status()==WL_CONNECTED) {
    wifiConnecting=false;
    if(!timeStarted){startTimeSync();timeStarted=true;Serial.println("HJ|EVENT|wifi=CONNECTED");}
  } else {
    if(timeStarted) timeStarted=false;
    if(wifiConnecting && millis()-wifiConnectStartedMs>=WIFI_CONNECT_TIMEOUT_MS){
      wifiConnecting=false;
      WiFi.disconnect(false, false);
      Serial.println("HJ|EVENT|wifi=FAILED");
      diagnoseWifiFailure();
    }
  }
  delay(5);
}
