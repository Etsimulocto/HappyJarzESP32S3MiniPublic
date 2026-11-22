# DND - next isolated module

**Status:** Integration branch and source-location tooling only. No DND firmware has been added, built, flashed, or published.

## Source fidelity and preservation

We are porting the original **BloomCircuit/BloomPetz DND** subsystem, not replacing it with a different role-playing game. The verifiable GitHub file `BloomPetz/firmware/bloompetz_v0_1.ino` is an early standalone BloomPetz vertical slice with no DND. The BloomCircuit README describes the *later*, working unified BloomSystem build, but its path `BloomPetz/firmware/bloompetz_v0_1/bloompetz_v0_1.ino` returned 404 via connected GitHub. This is a genuine source gap.

Before writing a functional DND module, locate the **authoritative** latest source on the Pi (or any Windows backup). A read-only search script is in `tools/find_bloompetz_dnd_source_pi.sh`. It does **not** modify any files.

## Required original game mechanics (from BloomCircuit README)

- Physical **21-character** four-row OLED: 7-column HUD + 14-column map; active DND owns display and blocks BloomPetz side-art refresh
- `@` player, D-pad move, tap A then direction within approx. 700 ms for attack, B opens game menu
- Eight geometry archetypes, 48 room themes, 48 enemy species, 48 items, 48 symbols
- Chests, traps, gold, potions, doors and combat; d20-style roll feed
- Inventory/shop with four offers, rerolled **on room entry**; purchasing deducts gold
- XP, level, HP/max HP, STR, DEX, AC, weapon progression and HP growth beyond 18
- Durable DND fields in BloomCircuit `bloomsys/state` unified save with version/checksum; temporary enemies/maps regenerated on load
- Resume the session across mode changes, checkpoint as documented
- Returning to HAPPY JARZ menu must preserve lights, clock, ART, arcade, and existing BloomPetz functionality

## Integration boundaries

- Preserve stable published **0.18.15**; preserve unpublished BLOOMPETZ 0.18.16 candidate and all saved NVS / SPIFFS.
- First develop on `feature/dnd-next-module`, a branch from `feature/bloompetz-first-module`.
- Obtain the latest, verified BloomCircuit DND code and its save-format declarations **before** porting.
- Do not silently migrate/overwrite early `hjpetz` PetSave or any existing BloomCircuit `bloomsys/state` data.
- DND should have a separate optional menu entry/visibility toggle, not be bundled into arcade or forced on customers.
- Compile under ESP32 Arduino core **3.3.12**; 4MB default application partition max **0x140000 / 1,310,720 bytes**.
- One test stage at a time: compile → app-only flash → OLED/input → gameplay → persistence → regressions. Publish only after explicit verification.

## Search on Raspberry Pi

From a shell inside the HappyJarz project checkout:

```bash
bash tools/find_bloompetz_dnd_source_pi.sh
```

Or run directly on the Pi if the HappyJarz checkout is not available:

```bash
find ~/BloomCircuit -type f \( -name '*.ino' -o -name '*.cpp' -o -name '*.h' \) -printf '%TY-%Tm-%Td %TH:%TM %s %p\n' | sort -r | head -80
```

Inspect matching file names and modified dates; avoid confusing the documented later implementation with the older 2026 BloomPetz-only source. A filename alone is not proof a file has the current DND logic.

