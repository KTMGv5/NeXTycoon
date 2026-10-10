# NeXTycoon Wall Street & Stock Exchange Native Plugin

A high-performance C-ABI native dynamic library (`.dll`) plugin for NeXTycoon / OpenRCT2.

## Overview
Classic theme park simulation games restrict park finances to static ticket fees, stall revenue, and fixed bank loans. `NeXTycoonWallStreet.dll` models your theme park as a publicly traded corporation listed on the NeXTycoon Exchange (Ticker: **$NXT**).

### Features
1. **Dynamic High-Frequency Stock Engine**:
   - Stock price fluctuates dynamically using Brownian motion and macroeconomic cycles (Bear, Neutral, Bull, Hyper Growth).
   - Price gravitates towards fundamental fair value based on Park Rating, Guest Count, Ride Count, and Total Park Valuation.
2. **Quarterly Shareholder Dividends**:
   - Every quarter (~1,200 ticks / ~30 seconds), institutional investors calculate dividend payouts.
   - High-performing parks receive direct capital injections (+$2,500 to +$35,000) straight into park treasury.
3. **Live Breaking News Ticker Broadcasts**:
   - All-Time High breakouts, economic cycle transitions, and dividend payouts broadcast live to the in-game bottom news ticker.
4. **Credit Agency Downgrade Warnings**:
   - If park rating drops below institutional threshold (400), credit agencies issue negative watch alerts on the ticker.

## Compilation
Compile with Microsoft Visual C++ (`cl.exe`) or any C99 compiler:
```cmd
cl /nologo /LD /O2 /W3 NeXTycoonWallStreet.c /Fe:bin/plugins/NeXTycoonWallStreet.dll
```

## Installation
Drop `NeXTycoonWallStreet.dll` into:
- The game's `plugins/` directory (inside the installation folder), OR
- `%USERPROFILE%\Documents\OpenRCT2\plugins\`

NeXTycoon scans both locations automatically and registers the plugin dynamically with zero restart required.
