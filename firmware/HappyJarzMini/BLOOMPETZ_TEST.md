# BLOOMPETZ - first HAPPY JARZ module bench candidate (0.18.16)

**UNRELEASED - DO NOT PUBLISH OR FLASH UNTIL COMPILE AND CONTROLLED BOARD #3 TESTS PASS.**

Source base: `Etsimulocto/BloomCircuit/BloomPetz/firmware/bloompetz_v0_1.ino` on main (early standalone BloomPetz v0.1). The BloomCircuit README describes a later unified BloomPetz + DND system, but the unified production sketch is not available at the verified repository path; this first slice **does not** include its editor, DND, Pi mirror, or unified `bloomsys/state` save migration. Original BloomCircuit code is **not overwritten**.

This candidate adds one ninth OLED entry **BLOOMPETZ** after the original eight, while maintaining the existing 8-bit menu mask and protected SYSTEM menu. This is a compiled option, not a dynamically downloaded module. Stable 0.18.15 is untouched on the other branch and on the website.

## Hardware features ported from original source

- Three pet slots, each containing 200 developmental float stats
- Eight daily actions: CHECK, CLEAN, PET, PLAY, REST, SCRATCH, SOCIALIZE, TRAIN
- Three-direction Simon sequence using the shared six known-good touch controls
- 25 weighted/random developmental stat rolls for successful first daily action
- 12.5 energy spent on first daily rewarded action, repeat = fun only
- FEED only when stomach empty (100 energy); three treats per day; daily resets at local midnight when clock is set
- View 200 stat values, cycle pet slots, create one default SPARK pet on-device, confirm before replacing
- Save/checksum fields based on original v0.1 record; stores these candidates only in **isolated `hjpetz` NVS namespace**, with no reads/writes to `bloomsys` or existing ART / HAPPY JARZ NVS
- All existing factory arcade and screensaver firmware remains compiled in

### Controls

On jar: **A** from HOME opens menu; scroll to BLOOMPETZ (at end) and A. In the pet view use UP/DOWN to select daily action; A starts the Simon sequence. B opens pet menu with FEED, TREAT, STATS, NEXT SLOT, CREATE/RESET and EXIT; pet menu A confirms choice. Stat browser uses LEFT/RIGHT for all 200. Empty pet slots use LEFT/RIGHT to change slot, A to create, B to exit to jar menu. Pet's internal EXIT returns to Happy Jarz menu. Press B during Simon to cancel.

In Web Studio EXPERIENCE + MAPPING (0.18.16+ connected) use Read / Show / Hide BLOOMPETZ. Menu visibility is an independent persisted boolean `happyjarz/pet_menu` (default true); disabling **does not delete the pet data**. `GET PET MODULE`, `SET PET MODULE ON`, `SET PET MODULE OFF`, `GET PET STATUS` commands query the candidate.

## Build and testing

On Windows in the **new development branch**, ensure the toolchain remains ESP32 core 3.3.12:

```powershell
cd $HOME\Documents\HappyJarzESP32S3MiniPublic
git fetch origin
git switch feature/bloompetz-first-module
git pull --ff-only
powershell -ExecutionPolicy Bypass -File .\tools\build_release_windows.ps1
```

Do not publish the resulting candidate. Review the compiler size; max application image is **1,310,720 bytes**.

**Before first app-only flash:** preserve Board #3 NVS, SPIFFS artwork, and working 0.18.15 app image. Do not erase flash, NVS or SPIFFS. Do not flash Board #4.

**Physical acceptance test** (one module only):

1. Boot 0.18.16, verify HJ|IDENTITY and stable lights, screensaver, artwork.
2. Confirm ninth BLOOMPETZ option appears; restore original 8 menu entries via Web Studio if 0.18.15 mask still hides several.
3. Enter BLOOMPETZ; select empty slot, create SPARK and power cycle. Pet still exists.
4. Complete one Simon daily action and observe energy drop by 12.5 and stat growth; replay same action and confirm no extra reward.
5. Explore STATS (1/200 through 200/200), feed/treat, slot changes, and exit.
6. Power cycle again; verify pet progress and artwork both persist.
7. Hide BLOOMPETZ from Web Studio, restart and verify it disappears from menu but pet data remains. Re-show and re-enter.
8. Launch existing seven factory arcade games, verify clock, lighting, touch, other menus, and art screensaver.
9. Confirm SYSTEM is available even if 8-bit menu mask = 0; no edit to released 0.18.15 catalog.
10. If anything fails, stop and restore the previous **app-only** image without wiping user partitions.

Known limitations: The older BloomPetz v0.1 source does not include the documented later full on-device free-text pet editor, richer animation, or unified BLOOM SYSTEM migration. This first candidate intentionally keeps existing saves separate and creates a default pet for controlled hardware validation. DND is not included.
