#!/usr/bin/env bash
# Builds LLM-gui from source and installs it for the current user, without sudo:
#   ~/.local/bin/LLM-gui                                               the app
#   ~/.local/share/icons/hicolor/scalable/apps/<app id>.svg            its icon
#   ~/.local/share/applications/<app id>.desktop                       its launcher entry
# PREFIX=/some/folder ./install.sh installs under that folder instead of ~/.local.
# Needs cmake, a C++20 compiler and Qt 6 (Quick, Network, Concurrent) to build; Qt 6 from the
# system at run time; and Ollama (https://ollama.com) installed and running to chat.
# Your chats, characters and settings (~/.local/share/LLM-gui, ~/.config/LLM-gui) are not touched.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$here"
prefix="${PREFIX:-$HOME/.local}"
id="io.github.chandandasgit.LLMgui"
build="$root/build-release"

echo "Building LLM-gui (Release) in $build ..."
cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Release > /dev/null
cmake --build "$build" --target appLLM-gui --parallel

echo "Installing under $prefix ..."
# A copy from install-app.sh leaves a link here; it is replaced by this build.
rm -f "$prefix/bin/LLM-gui"
install -Dm755 "$build/appLLM-gui" "$prefix/bin/LLM-gui"
install -Dm644 "$root/resources/icons/icon.svg" "$prefix/share/icons/hicolor/scalable/apps/$id.svg"
# The launcher runs the installed app by its full path, so ~/.local/bin need not be on PATH.
entry="$(mktemp)"
trap 'rm -f "$entry"' EXIT
sed "s|^Exec=.*|Exec=$prefix/bin/LLM-gui|" "$root/packaging/linux/$id.desktop" > "$entry"
install -Dm644 "$entry" "$prefix/share/applications/$id.desktop"

if command -v update-desktop-database > /dev/null; then
    update-desktop-database -q "$prefix/share/applications" || true
fi
if command -v gtk-update-icon-cache > /dev/null; then
    gtk-update-icon-cache -q -t "$prefix/share/icons/hicolor" || true
fi

if [ -d "$HOME/.local/opt/LLM-gui" ]; then
    echo "Note: a copy from install-app.sh is still in ~/.local/opt/LLM-gui. Its uninstall-app.sh removes it."
fi
echo "Installed. Start it from your launcher (LLM-gui) or run $prefix/bin/LLM-gui"
