#!/bin/bash
# Installs LLM-gui for your user (no administrator rights needed):
#   the app in ~/.local/opt/LLM-gui, the LLM-gui command in ~/.local/bin,
#   a launcher entry and its icon. (Not in ~/.local/share/LLM-gui: that holds
#   your chats and characters.)
# Run it from the unpacked folder: ./install-app.sh. Running it again updates
# the app. Remove it with ./uninstall-app.sh (your chats, characters and settings stay).
set -euo pipefail

id=io.github.chandandasgit.LLMgui
here="$(cd "$(dirname "$0")" && pwd)"
target="$HOME/.local/opt/LLM-gui"
data="${XDG_DATA_HOME:-$HOME/.local/share}"
bin="$HOME/.local/bin"
apps="$data/applications"
icons="$data/icons/hicolor/scalable/apps"

if [ ! -x "$here/app/AppRun" ]; then
    echo "Run this from the unpacked LLM-gui folder (app/AppRun is missing)." >&2
    exit 1
fi

# The app (an older copy is replaced)
rm -rf "$target"
mkdir -p "$target"
cp -a "$here/app/." "$target/"

# Launcher entry and icon. Desktop files quote with "…"; inside, \ " ` $ are escaped.
quoted="\"$(printf '%s' "$target/AppRun" | sed -e 's/[\\"`$]/\\&/g')\""
sedEscape() { printf '%s' "$1" | sed -e 's/[\\&|]/\\&/g'; }
mkdir -p "$apps" "$icons"
sed -e "s|^Exec=.*|Exec=$(sedEscape "$quoted")|" "$target/usr/share/applications/$id.desktop" > "$apps/$id.desktop"
chmod 644 "$apps/$id.desktop"
install -m644 "$target/usr/share/icons/hicolor/scalable/apps/$id.svg" "$icons/$id.svg"
command -v update-desktop-database >/dev/null && update-desktop-database -q "$apps" || true

# The LLM-gui command
mkdir -p "$bin"
ln -sf "$target/AppRun" "$bin/LLM-gui"

echo "LLM-gui is installed."
echo "  Program : $target"
echo "  Command : $bin/LLM-gui"
echo "  Menu    : $apps/$id.desktop"
case ":$PATH:" in
    *":$bin:"*) ;;
    *) echo "Note: $bin is not on your PATH, so the LLM-gui command works only once you add it." ;;
esac
echo "LLM-gui talks to Ollama, which must be installed and running (https://ollama.com)."
