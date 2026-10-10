#!/bin/sh
set -eu

APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
AUTOSTART_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/autostart"
DESKTOP_FILE="$AUTOSTART_DIR/happyjarz-plug-watch.desktop"

mkdir -p "$AUTOSTART_DIR"
cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Type=Application
Name=HAPPY JARZ Mini plug watcher
Exec=/usr/bin/python3 "$APP_DIR/happyjarz_plug_watch.py"
Path=$APP_DIR
Terminal=false
X-GNOME-Autostart-enabled=true
EOF
chmod 644 "$DESKTOP_FILE"
printf 'Installed HAPPY JARZ autostart: %s\n' "$DESKTOP_FILE"
printf 'It starts at the next login. Start it now with /usr/bin/python3 "%s/happyjarz_plug_watch.py"\n' "$APP_DIR"
