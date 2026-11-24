# LLM-gui

A desktop chat app for language models that run on your own computer through
[Ollama](https://ollama.com). Dark, keyboard-first and terminal-styled: your prompt
as `> prompt`, the reply streaming in below it. Nothing leaves your machine except
model downloads.

## Before you start: Ollama

LLM-gui talks to Ollama, which runs the models. Install it from
[ollama.com/download](https://ollama.com/download) and make sure it is running
(`ollama serve`, or the service your install set up). LLM-gui looks for it at
`http://localhost:11434`; the status strip at the bottom shows whether it answers.

You can install models from inside LLM-gui (Models → Recommended, or Manual).

## Installation

### Install from the release (recommended)

1. Download `LLM-gui-<version>-linux-x86_64.tar.xz` from the
   [latest release](https://github.com/ChandanDasGIT/LLM-gui/releases/latest).
2. Unpack it and run the installer. No administrator rights are needed:

   ```bash
   tar -xf LLM-gui-*-linux-x86_64.tar.xz
   cd LLM-gui-*-linux-x86_64
   ./install-app.sh
   ```

This puts the app in `~/.local/opt/LLM-gui`, an `LLM-gui` command in `~/.local/bin`,
and a launcher entry with its icon in your app menu. Run `./install-app.sh` from a newer
release to update. The release carries its own copy of Qt, so nothing else is needed.

### Use the AppImage

Download `LLM-gui-<version>-x86_64.AppImage` from the
[latest release](https://github.com/ChandanDasGIT/LLM-gui/releases/latest), then:

```bash
chmod +x LLM-gui-*-x86_64.AppImage
./LLM-gui-*-x86_64.AppImage
```

### Build and install from source

Building needs **Qt 6.10 or newer**, CMake and a C++20 compiler.

1. Install the build tools and Qt.

   **Fedora** (tested on Fedora 44):

   ```bash
   sudo dnf install git cmake gcc-c++ qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg
   ```

   **Arch Linux** (package names not tested on Arch):

   ```bash
   sudo pacman -S --needed git cmake gcc qt6-base qt6-declarative qt6-svg
   ```

   **Ubuntu and Debian**: first check that your release has Qt 6.10 or newer with
   `apt-cache policy qt6-base-dev`. If it is older, use the release above, which brings its
   own Qt. If it is new enough (package names not tested on Ubuntu or Debian):

   ```bash
   sudo apt install git cmake g++ qt6-base-dev qt6-declarative-dev qt6-svg-plugins \
     qml6-module-qtquick-controls qml6-module-qtquick-layouts \
     qml6-module-qtquick-dialogs qml6-module-qtqml-workerscript
   ```

2. Get the source, build it and install it for your user (no sudo):

   ```bash
   git clone https://github.com/ChandanDasGIT/LLM-gui.git
   cd LLM-gui
   ./packaging/linux/install.sh
   ```

   This builds a Release version in `build-release/` and installs the app as
   `~/.local/bin/LLM-gui`, with its icon and launcher entry under `~/.local/share`.
   To install somewhere else, set `PREFIX`, for example
   `PREFIX=/opt/llm-gui ./packaging/linux/install.sh`.

   A source build uses the Qt from your system, so keep those Qt packages installed.

To run it from the build folder without installing:

```bash
cmake --preset default
cmake --build build
./build/appLLM-gui
```

## Uninstallation

Your chats, characters and settings are kept by both uninstallers (see below), and your
Ollama models are never touched.

- Installed from the release: run `./uninstall-app.sh` from the unpacked folder.
- Installed from source: run `./packaging/linux/uninstall.sh` from the source folder
  (with the same `PREFIX` if you used one).
- AppImage: delete the file.

## Where it keeps your data

The same places whichever way you installed it:

| What | Where |
|---|---|
| Chats | `~/.local/share/LLM-gui/chats/` (one JSON file per chat) |
| Characters | `~/.local/share/LLM-gui/characters/` (one JSON file each) |
| Settings (Ollama address, current model and chat) | `~/.config/LLM-gui/LLM-gui.conf` |
| Models | Ollama's own folder; LLM-gui does not store models |

Images you attach are saved in a chat as file paths, so an image that is moved or deleted
afterwards shows as "no longer on disk" and is not sent again.

## Features

- **Chat**: replies stream in as Markdown; thinking models show their thinking dimmed
  above the reply. Enter sends, Shift+Enter adds a line, Esc stops a reply.
- **Images**: attach or drop PNG, JPEG or WebP images when the model can see them.
- **History**: every chat is saved and listed in the sidebar by day; reopen or delete it there.
- **New chat**: pick a model and a character to start with; switch either later from the
  prompt box.
- **Slash commands**: `/clear` (new chat), `/model <name>`, `/models`, `/characters`.
- **Model manager**:
  - *Recommended*: models from ollama.com sorted by category (general, coding, reasoning,
    writing, vision, tiny), each rated for your graphics card and memory
    (fits the GPU, partly on the CPU, or too big). Install with progress and Cancel.
  - *Installed*: size, parameters and quantization, with Use and Remove.
  - *Manual*: install any Ollama tag (also `hf.co/user/repo:quant`), or import a local
    `.gguf` file under a name you choose.
- **Characters**: personalities with a system prompt, an optional greeting, a preferred
  model and a temperature. The built-in Default has none.
- **Status strip**: model, character, Ollama connection and the speed of the last reply.

Limits, said plainly:

- The fit rating is an estimate from download size and memory, not a speed promise.
- The recommended list is a snapshot of ollama.com (2026-11-24); it goes stale until updated.
- Graphics memory is read for NVIDIA (`nvidia-smi`) and AMD cards; others count as CPU only.
- An imported `.gguf` takes twice the disk space until you delete the original, and a file
  without a chat template can answer badly.

## How it is built

C++20 and Qt 6 (QML), in three tiers:

- `backend/`: plain C++ with no Qt (the Ollama formats, JSON, chats, characters,
  hardware rating, SHA-256).
- `wrapper/`: Qt classes that connect the backend to the QML.
- `frontend/`: the QML views.

Tests: `build/backendtests` (backend) and `build/wrappertests` (wrappers, against a fake
Ollama server; run with `QT_QPA_PLATFORM=offscreen`).

## License

GPL-3.0. See [LICENSE](LICENSE).
