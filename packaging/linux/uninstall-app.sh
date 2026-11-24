#!/bin/bash
# Removes what install-app.sh installed. Your chats, characters and settings
# (~/.local/share/LLM-gui and ~/.config/LLM-gui) are kept, and so are your
# Ollama models.
set -euo pipefail

id=io.github.chandandasgit.LLMgui
data="${XDG_DATA_HOME:-$HOME/.local/share}"
target="$HOME/.local/opt/LLM-gui"

for file in "$data/applications/$id.desktop" "$data/icons/hicolor/scalable/apps/$id.svg" \
            "$HOME/.local/bin/LLM-gui" "$target"; do
    if [ -e "$file" ] || [ -L "$file" ]; then
        rm -rf -- "$file"
        echo "Removed $file"
    fi
done
command -v update-desktop-database >/dev/null && update-desktop-database -q "$data/applications" || true
echo "Your chats, characters and settings are still in ~/.local/share/LLM-gui and ~/.config/LLM-gui."
