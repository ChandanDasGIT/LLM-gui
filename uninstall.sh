#!/usr/bin/env bash
# Removes what install.sh put in place: the app, its icon and its launcher entry.
# Your chats, characters and settings are kept:
#   ~/.local/share/LLM-gui and ~/.config/LLM-gui (delete those yourself to remove them too).
# Your Ollama models are not touched.
# PREFIX=/some/folder ./uninstall.sh removes an install made with the same PREFIX.
set -euo pipefail

prefix="${PREFIX:-$HOME/.local}"
id="io.github.chandandasgit.LLMgui"

for file in "$prefix/bin/LLM-gui" \
            "$prefix/share/icons/hicolor/scalable/apps/$id.svg" \
            "$prefix/share/applications/$id.desktop"; do
    if [ -e "$file" ] || [ -L "$file" ]; then
        rm -f "$file"
        echo "Removed $file"
    fi
done

if command -v update-desktop-database > /dev/null && [ -d "$prefix/share/applications" ]; then
    update-desktop-database -q "$prefix/share/applications" || true
fi
if command -v gtk-update-icon-cache > /dev/null && [ -d "$prefix/share/icons/hicolor" ]; then
    gtk-update-icon-cache -q -t "$prefix/share/icons/hicolor" || true
fi

echo "LLM-gui is uninstalled. Your chats and characters are still in ~/.local/share/LLM-gui."
