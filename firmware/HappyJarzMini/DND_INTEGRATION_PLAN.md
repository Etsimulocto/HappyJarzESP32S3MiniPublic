# DND hardware-test candidate — 0.18.17

## Source and status
- **Source recovered** from Pi, committed to `Etsimulocto/BloomCircuit` branch `archive/bloompetz-dnd-original-2026-11-22` (commit `d6f1deb`).
- The authoritative engine is `BloomPetz/firmware/bloompetz_v0_1/bloompetz_v0_1.ino`, dated 2026-10-05, 144753 bytes. Original archive was SHA256 checked against Pi backup.
- `happyjarz_dnd.h` is a **port of the archived game engine and original 48-species/item/equipment/room logic**, not a rewrite. UI, input, save, and lifecycle are adapters.
- **0.18.17 test candidate is source-only until compiled on Windows. Not flashed, not verified on hardware, not published.** Public 0.18.15 remains stable; 0.18.16 remains the previous pet test candidate.

## Adapters and boundaries
- The original 21-character x four-row display uses HAPPY JARZ shared OLED service (U8g2 6x12 font); native 7 HUD + 14 map columns preserved.
- Six touch/USB KEY actions: UP/DOWN/LEFT/RIGHT/A/B. A then direction within 700ms attacks; B opens original game menu. **START exits DND to HAPPY JARZ menu**, with save.
- Menu item `DND` is independently show/hide configurable, saved at `happyjarz/dnd_menu`, separate from original 8-bit `menu_mask`. Pet menu is independent too.
- DND durable state is written to **new namespace `hjdnd`, key `state`** with magic/version/length/FNV checksum; does **not** touch original `bloomsys/state`, current `hjpetz` pet data, profile, or ART SPIFFS. This means automatic migration from original Pi `bloomsys` is deliberately **not implemented**.
- Original game durable state, 288-inventory capacity, equipment slots, stats, gold, experience, room and depth survive re-entry/reboot; room map and transient enemies regenerate on resume, consistent with original engine.
- `GET DND MODULE`, `SET DND MODULE ON/OFF`, and `GET MODULES` expose module status. `RESET MODULES` shows DND again without resetting its saved game.

## Build / validation on Windows PowerShell
```powershell
cd "$HOME\Documents\HappyJarzESP32S3MiniPublic"
git fetch origin
git switch feature/dnd-next-module
git pull --ff-only origin feature/dnd-next-module
.\tools\build_release_windows.ps1
```
Do not use `-InstallDependencies` unless specifically requested: preserve ESP32 Arduino core 3.3.12.
The builder compiles for 4MB default dual-app layout, fails closed if it exceeds `0x140000`, stages app-only binary and logs at `release-staging/candidate`. No flashing/publishing.

## Test gates
1. Successful Arduino build with memory headroom; check release-staging candidate checksum.
2. App-only flash to intended test board (COM port confirmed), **never erase flash** or flash partitions/bootloader.
3. OLED view, D-pad movement, A+direction attack, B menu, START exit.
4. Inventory, shop, progression, random rooms, enemies and autosaves.
5. Power cycle, resume persistent DND state, verify existing BloomPetz pet, ART, settings, profiles, lights, arcade.
6. Only after physical verification consider Web Studio controls and publishing.

## Caveat
The original game engine was ported with limited interface edits, but **the new integration has not yet been compiled or physically validated**. Fix compilation/regressions on this feature branch; do not modify stable release until proven.
