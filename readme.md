<p align="center">
  <img src="https://raw.githubusercontent.com/OpenRCT2/OpenRCT2/develop/resources/logo/icon_x128.png" style="width: 128px;" alt="NeXTycoon Logo"/>
</p>

<h1 align="center">NeXTycoon</h1>

<h3 align="center">The Next-Generation Universal Tycoon Simulation Engine</h3>

<p align="center">
  <b>A unified, high-performance C++20 engine combining RollerCoaster Tycoon 1 & 2 with Chris Sawyer's Locomotion under a modern DirectX 11 hardware presentation pipeline, post-processing pixel shaders, and hot-reloadable plugin architecture.</b>
</p>

<p align="center">
  <a href="https://github.com/KTMGv5/NeXTycoon"><img src="https://img.shields.io/badge/version-1.0.0-blue.svg?style=flat-square" alt="Version 1.0.0"/></a>
  <a href="https://github.com/KTMGv5/NeXTycoon"><img src="https://img.shields.io/badge/arch-x86--64-green.svg?style=flat-square" alt="x86-64 Architecture"/></a>
  <a href="https://github.com/KTMGv5/NeXTycoon"><img src="https://img.shields.io/badge/graphics-DirectX%2011%20%7C%20DirectX%209-orange.svg?style=flat-square" alt="DirectX 11 / 9"/></a>
  <a href="https://github.com/KTMGv5/NeXTycoon"><img src="https://img.shields.io/badge/license-GPLv3-lightgrey.svg?style=flat-square" alt="License GPLv3"/></a>
</p>

---

## Overview

**NeXTycoon** is an ambitious open-source evolution of the classic isometric tycoon simulation genre. Originally founded on the decompiled assembly of Chris Sawyer's masterpiece *RollerCoaster Tycoon 2*, NeXTycoon bridges the engines of **RCT1**, **RCT2**, and **Chris Sawyer's Locomotion** into a single modular simulation platform.

Equipped with a modern Direct3D 11 hardware pipeline, multi-layer post-processing shaders, live telemetry cameras, and an integrated JavaScript/TypeScript plugin manager, NeXTycoon delivers high framerates and pixel fidelity on modern displays while retaining 100% backward compatibility with classic parks, track designs, and scenarios.

---

## Key Features

### 🎮 Universal Tycoon Architecture
- **RollerCoaster Tycoon 2 Core**: Complete park mechanics, multi-threaded pathfinding, guest AI, advanced finance, and co-op multiplayer.
- **RollerCoaster Tycoon 1 Direct Importer**: Native, zero-conversion loading of `.SV4` parks, `.SC4` scenarios, and `.CSG` landscape files.
- **Locomotion Transport Bridge**: Architectural foundation for Chris Sawyer's *Locomotion* (`.SV5`, `.SC5`, `.DAT`), enabling multi-consist train networks, freight delivery cycles, and road vehicle transport.

### ⚡ DirectX 11 Hardware Presentation Engine
- **Uncapped High-Refresh Framerates**: Custom blit-discard DXGI swapchain eliminates flip-queue presentation bottlenecks, delivering 500+ FPS on 144Hz, 240Hz, and 360Hz displays.
- **Intelligent Hardware Selection**: Automatically detects and leverages DirectX Feature Level 11_1 / 11_0 hardware, with smooth automatic fallback to Direct3D 9 for older systems.
- **Zero OpenGL Overhead**: Windows builds run exclusively on direct native DirectX pipelines for minimal CPU overhead.

### 🎨 Post-Processing Shader FX
Selectable in real-time under *Options > Display > Scaling Quality*:
1. **Sharp (Point Crisp)**: Authentic 1:1 pixel-art presentation without blur.
2. **Smooth (Bilinear)**: Classic interpolated filtering.
3. **Smooth Nearest Neighbour**: Sub-pixel anti-aliased edge smoothing for non-integer window scales.
4. **Vibrant HDR**: Modern saturation, contrast, and color punch.
5. **Retro Arcade CRT**: Real-time curved scanlines, phosphor glow, and arcade monitor bloom.

### 🎢 Live Coaster-Cam (Picture-in-Picture)
- Dedicated broadcast-style viewport window tracking any roller coaster train or vehicle in real time.
- **Live Broadcast Telemetry HUD**: Displays real-time G-forces (Vertical & Lateral), velocity (mph / km/h), altitude, train index, and live POV status.
- Smear-free dragging: Secondary floating viewports drag across the screen without ghosting or memory tearing.

### 🔌 In-Game Plugin & Universal Module Manager
- Accessible directly via the top toolbar or Options menu.
- **Installed Scripts Dashboard**: View active JavaScript and TypeScript plugins with author, version, and type metadata.
- **Live Hot-Reload**: Reload all scripts instantly with zero game downtime.
- **One-Click Folder Access**: Open the local `plugins/` directory in File Explorer with one button.

---

## Quick Start (Pre-built Windows 64-Bit)

1. Download the latest release from the [Releases](https://github.com/KTMGv5/NeXTycoon/releases) page.
2. Extract the `.zip` archive to any directory.
3. Ensure you have your original *RollerCoaster Tycoon 2* game files available (Steam, GOG, or CD).
4. Run `run_64bit.bat` or launch `bin/openrct2.exe`.
5. Enjoy smooth, high-resolution simulation gameplay!

---

## Building from Source

### Prerequisites
- Windows 10 / 11 (or Windows 7/8.1 with DirectX 11)
- Visual Studio 2022 Community or newer (with "Desktop development with C++")
- MSBuild toolset

### Build Steps
```powershell
# Clone the repository
git clone https://github.com/KTMGv5/NeXTycoon.git
cd NeXTycoon

# Build x64 Release using MSBuild
msbuild openrct2.sln /p:Configuration=Release /p:Platform=x64 -m
```

The resulting binaries will be placed in `bin/`:
- `bin/openrct2.exe` (Main Game Executable)
- `bin/openrct2-cli.exe` (Command Line & Dedicated Server)

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
│   Entities / AI     │  Pathfinding           │  Multiplayer  │
└──────────────────────────────┬───────────────────────────────┘
                               │
┌──────────────────────────────▼───────────────────────────────┐
│               Hardware Presentation Pipeline                 │
│   Direct3D 11 (Primary)   ◄─── Auto-Fallback ───►  Direct3D 9│
│   Pixel Shaders (Point Crisp, Smooth, Vibrant HDR, CRT)     │
└──────────────────────────────────────────────────────────────┘
```

---

## Contributing

Contributions, issues, and feature proposals are welcome! Whether you are interested in:
- Adding Locomotion track and vehicle features
- Writing new JavaScript/TypeScript plugins
- Creating post-processing HLSL pixel shaders
- Improving simulation mechanics and performance

Feel free to open an issue or submit a Pull Request.

---

## License

NeXTycoon is licensed under the **GNU General Public License version 3 (GPLv3)**. See [contributors.md](contributors.md) for full historical acknowledgments of Chris Sawyer, OpenRCT2 developers, and the OpenLoco project.
