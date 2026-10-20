<p align="center">
  <img src="https://raw.githubusercontent.com/OpenRCT2/OpenRCT2/develop/resources/logo/icon_x128.png" style="width: 128px;" alt="NeXTycoon Logo"/>
</p>

<h1 align="center">NeXTycoon</h1>

<h3 align="center">The Next-Generation Universal Tycoon Simulation Engine</h3>

<p align="center">
  <b>A unified, cross-platform C++20 engine combining RollerCoaster Tycoon 1 & 2 with Chris Sawyer's Locomotion under a modern Vulkan & OpenGL hardware presentation pipeline, multi-language plugin architecture (JS/TS/Lua/Native C-ABI), and multi-platform native support (Windows 64/32-bit, Linux, macOS).</b>
</p>

<p align="center">
  <a href="https://github.com/KTMGv5/NeXTycoon"><img src="https://img.shields.io/badge/version-1.1.0-blue.svg?style=flat-square" alt="Version 1.1.0"/></a>
  <a href="https://github.com/KTMGv5/NeXTycoon"><img src="https://img.shields.io/badge/platforms-Windows%20%7C%20Linux%20%7C%20macOS-informational.svg?style=flat-square" alt="Platforms: Windows, Linux, macOS"/></a>
  <a href="https://github.com/KTMGv5/NeXTycoon"><img src="https://img.shields.io/badge/graphics-Vulkan%20%7C%20OpenGL%20%7C%20Software-orange.svg?style=flat-square" alt="Vulkan, OpenGL, Software"/></a>
  <a href="https://github.com/KTMGv5/NeXTycoon"><img src="https://img.shields.io/badge/license-GPLv3-lightgrey.svg?style=flat-square" alt="License GPLv3"/></a>
</p>

---

## Overview

**NeXTycoon** is an open-source evolution of the classic isometric tycoon simulation genre. Originally founded on the decompiled assembly of Chris Sawyer's masterpiece *RollerCoaster Tycoon 2*, NeXTycoon bridges the simulation engines of **RCT1**, **RCT2**, and **Chris Sawyer's Locomotion** into a single universal platform.

Equipped with hardware-accelerated **Vulkan** and **OpenGL 3.3+** drawing engines, an ultra-compatible **Software** renderer, live telemetry coaster cameras, and a multi-language plugin architecture (JavaScript, TypeScript, Lua 5.4, and C-ABI Native DLLs), NeXTycoon delivers high framerates and pixel fidelity on modern displays while retaining 100% backward compatibility with classic parks, track designs, and scenarios.

---

## Key Features

### 🎮 Universal Tycoon Architecture
- **RollerCoaster Tycoon 2 Core**: Complete park mechanics, multi-threaded pathfinding, guest AI, advanced finance, and co-op multiplayer.
- **RollerCoaster Tycoon 1 Direct Importer**: Native, zero-conversion loading of `.SV4` parks, `.SC4` scenarios, and `.CSG` landscape files.
- **Locomotion Transport Bridge**: Architectural foundation for Chris Sawyer's *Locomotion* (`.SV5`, `.SC5`, `.DAT`), enabling multi-consist train networks, freight delivery cycles, and road vehicle transport.

### ⚡ Modern Cross-Platform Graphics Engine
Selectable under *Options > Display > Drawing Engine*:
- **Vulkan (Hardware Accelerated)**: Modern low-overhead graphics API delivering uncapped framerates (via `VK_PRESENT_MODE_MAILBOX_KHR` and immediate mode), hardware blit scaling, and cross-platform support across Windows, Linux, and macOS (via MoltenVK).
- **OpenGL (Hardware Accelerated)**: Full OpenGL 3.3+ pipeline with programmable shader programs, custom framebuffers, and texture caches.
- **Software (Hardware Display Blit)**: 100% compatible dirty-rectangle software rasterizer with hardware blitting.
- **Direct3D 11 & Direct3D 9**: Optional fallbacks available on Windows.

### 🔌 Multi-Language Plugin & Universal Module Manager
- Accessible directly via the top toolbar or Options menu.
- **Polyglot Scripting**: Native support for **JavaScript** and **TypeScript** (QuickJS), embedded **Lua 5.4.7**, and compiled **C-ABI Native Dynamic Libraries** (`.dll`, `.so`, `.dylib`).
- **Live Hot-Reload**: Reload all scripts instantly with zero game downtime.
- **One-Click Folder Access**: Open the local `plugins/` directory in File Explorer with one button.

### 🎢 Live Coaster-Cam (Picture-in-Picture)
- Dedicated broadcast-style viewport window tracking any roller coaster train or vehicle in real time.
- **Live Broadcast Telemetry HUD**: Displays real-time G-forces (Vertical & Lateral), velocity (mph / km/h), altitude, train index, and live POV status.
- Smear-free dragging: Secondary floating viewports drag across the screen smoothly without ghosting or tearing.

---

## Supported Platforms & Pre-built Binaries

NeXTycoon distributes official binaries for all major desktop platforms:

| Platform | Architecture | Binary Package | Rendering Backends |
| :--- | :--- | :--- | :--- |
| **Windows** | 64-bit (`x64`) | `NeXTycoon-v1.1.0-windows-x64.zip` | Vulkan, OpenGL, Software, Direct3D 11/9 |
| **Windows** | 32-bit (`x86`) | `NeXTycoon-v1.1.0-windows-x86.zip` | Vulkan, OpenGL, Software, Direct3D 11/9 |
| **Linux** | 64-bit (`x86_64`) | `NeXTycoon-v1.1.0-linux-x86_64.tar.gz` | Vulkan, OpenGL, Software |
| **macOS** | Universal (`arm64` + `x86_64`) | `NeXTycoon-v1.1.0-macos-universal.zip` | Vulkan (MoltenVK), OpenGL, Software |

---

## Quick Start

1. Download the latest archive for your platform from the [Releases](https://github.com/KTMGv5/NeXTycoon/releases) page.
2. Extract the archive to any directory.
3. Ensure you have your original *RollerCoaster Tycoon 2* game files available (Steam, GOG, or CD).
4. Run `openrct2` (or `openrct2.exe` on Windows).
5. Enjoy smooth, high-resolution simulation gameplay!

---

## Building from Source

### Windows (Visual Studio / MSBuild)
```powershell
# Clone the repository
git clone https://github.com/KTMGv5/NeXTycoon.git
cd NeXTycoon

# Build 64-bit Release
msbuild openrct2.sln /p:Configuration=Release /p:Platform=x64 -m

# Build 32-bit Release
msbuild openrct2.sln /p:Configuration=Release /p:Platform=Win32 -m
```

### Linux (CMake + Ninja)
```bash
# Install dependencies (Ubuntu / Debian)
sudo apt-get update
sudo apt-get install -y cmake ninja-build libsdl2-dev libvulkan-dev \
    libcurl4-openssl-dev libspeexdsp-dev libfontconfig1-dev libfreetype6-dev \
    libpng-dev libzip-dev libssl-dev zlib1g-dev libflac-dev libvorbis-dev libicu-dev libgl1-mesa-dev

# Build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DDISABLE_DISCORD=ON
ninja -C build
```

### macOS (Homebrew + CMake)
```bash
# Install dependencies
brew install cmake ninja sdl2 libpng freetype fontconfig speexdsp libzip flac libvorbis icu4c openssl molten-vk

# Build Universal Binary
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DDISABLE_DISCORD=ON \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DOPENSSL_ROOT_DIR="$(brew --prefix openssl)" \
    -DICU_ROOT="$(brew --prefix icu4c)"
ninja -C build
```

---

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                       NeXTycoon UI                           │
│   Options • Plugin Manager • Top Toolbar • Coaster-Cam PIP   │
└──────────────────────────────┬───────────────────────────────┘
                               │
┌──────────────────────────────▼───────────────────────────────┐
│                 Universal Tycoon Sim Core                    │
│   RCT2 Core Engine  │  RCT1 S4/CSG Importer  │  Loco Bridge  │
│   Multi-Language VM │  Pathfinding           │  Multiplayer  │
│   (JS / TS / Lua)   │  Guest / Ride AI       │  Zstd Stream  │
└──────────────────────────────┬───────────────────────────────┘
                               │
┌──────────────────────────────▼───────────────────────────────┐
│              Hardware Presentation Pipeline                  │
│   Vulkan (Primary) ◄─── Auto-Fallback ───► OpenGL 3.3+       │
│   Software (Blit)  ◄─── Windows Only  ───► Direct3D 11 / 9   │
└──────────────────────────────────────────────────────────────┘
```

---

## Contributing

Contributions, issues, and feature proposals are welcome! Whether you are interested in:
- Expanding Locomotion track and vehicle transport features
- Writing new JavaScript, TypeScript, or Lua plugins
- Creating native C-ABI high-performance simulation modules
- Enhancing rendering performance across Vulkan and OpenGL
- Improving simulation mechanics and multiplayer sync

Feel free to open an issue or submit a Pull Request.

---

## License

NeXTycoon is licensed under the **GNU General Public License version 3 (GPLv3)**. See [contributors.md](contributors.md) for full historical acknowledgments of Chris Sawyer, OpenRCT2 developers, and the OpenLoco project.
