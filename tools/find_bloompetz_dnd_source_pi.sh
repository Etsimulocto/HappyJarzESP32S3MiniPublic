# Locate the authoritative BloomCircuit DND implementation (READ ONLY)
# Execute on the Raspberry Pi that still has the tested BloomPetz+DND firmware.
# This tool never copies, modifies, uploads, flashes or deletes any files.
# Example: bash tools/find_bloompetz_dnd_source_pi.sh
set -eu
echo "Searching for unified BloomPetz + DND sources under common home paths..."
for root in "$HOME/BloomCircuit" "$HOME/Desktop" "$HOME/Documents" "$HOME/Downloads"; do
  if [ ! -d "$root" ]; then continue; fi
  echo "== $root =="
  find "$root" -type f \( -iname '*.ino' -o -iname '*.cpp' -o -iname '*.h' -o -iname '*.py' \) \
    \( -iname '*bloom*' -o -iname '*dnd*' -o -iname '*dungeon*' -o -iname '*rpg*' \) \
    -printf '%TY-%Tm-%Td %TH:%TM %10s bytes  %p\n' 2>/dev/null | sort -r | head -60 || true
done
echo
echo "Checking for distinctive unified-save and gameplay markers..."
for root in "$HOME/BloomCircuit" "$HOME/Desktop" "$HOME/Documents" "$HOME/Downloads"; do
  if [ ! -d "$root" ]; then continue; fi
  grep -RIlE --include='*.ino' --include='*.h' --include='*.cpp' \
    'BloomSystemSaveV1|dndRoom|DND_ROOM|bloomsys' "$root" 2>/dev/null | head -60 || true
done
echo "Locate the newest *working* complete source; do not replace your Pi firmware."
