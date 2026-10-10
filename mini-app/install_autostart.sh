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
printf 'The watcher starts at next login. For this session, restart it with:\n'
printf '  pkill -f \'[h]appyjarz_plug_watch.py\' || true\n'
printf '  /usr/bin/python3 "%s/happyjarz_plug_watch.py" >/dev/null 2>&1 &\n' "$APP_DIR"
