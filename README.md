# OBS Glass Pill Widget

Native OBS Studio plugin that renders a configurable glass pill overlay with procedural caustics, SVG-quality text (time, day, placeholder weather), and timed slide/fade animations.

## Features

- **Glass pill** with animated procedural caustics (HLSL shader)
- **SVG-quality text** via [LunaSVG](https://github.com/sammycage/lunasvg) rasterized at 1–4× scale
- **Timed cycle**: hidden by default, appears every 5 minutes (configurable), shows ~15s, animates in/out
- **Placeholder weather** text and optional SVG icon (no API required for v1)
- **Debug mode** to keep the pill visible while tuning layout in OBS

> **Note:** The glass effect is simulated (translucent fill + caustic highlights). OBS custom sources cannot sample the scene behind them for true backdrop refraction.

## Requirements

- Windows 10/11
- OBS Studio 31.x (recommended for the Windows plugin build from CI)

Local compilation is optional — see **Cloud CI** below if you do not want Visual Studio on your machine.

## Cloud CI (recommended — no Visual Studio)

GitHub Actions builds the Windows `.dll` for you on every push to `main` / `master`, or when you trigger the workflow manually.

### One-time setup

1. Create a new GitHub repository (e.g. `obs-widget`).
2. Point git at your repo (the clone still references the OBS template upstream):

```powershell
cd c:\Users\albin\Documents\projects\obs_widget
git remote remove origin
git remote add origin https://github.com/YOUR_USER/obs-widget.git
git add .
git commit -m "Add OBS Glass Pill Widget plugin"
git push -u origin master
```

3. Open **GitHub → Actions** and wait for **Windows Plugin** to finish (about 10–20 minutes on first run while OBS deps download).

### Download and install

1. Open the completed workflow run.
2. Download the artifact named `obs-widget-windows-<commit>`.
3. Extract the zip.
4. Copy everything into:

```
%APPDATA%\obs-studio\plugins\obs-widget\
```

5. Restart OBS → add source **Glass Pill Widget**.

You can also run the build on demand: **Actions → Windows Plugin → Run workflow**.

## Local build (optional)

### Windows

Requires Visual Studio 2022 and CMake 3.30+:

```powershell
cd obs_widget
cmake --preset windows-x64
cmake --build --preset windows-x64 --config RelWithDebInfo
```

Install from `build_x64\rundir\RelWithDebInfo\obs-widget\` to `%APPDATA%\obs-studio\plugins\obs-widget\`.

### Linux / WSL (compile check only)

```bash
cmake --preset ubuntu-x86_64
cmake --build build_x86_64
```

Output: `build_x86_64/rundir/RelWithDebInfo/obs-widget.so` plus `data/`.

> WSL builds a Linux `.so` — it will **not** load in Windows OBS.

## Usage

1. In OBS, add a source: **Glass Pill Widget**
2. Position and scale the source in your scene
3. Enable **Always visible (debug)** while adjusting size, fonts, and timing
4. Disable debug when finished — the pill will appear on the configured interval

### Key settings

| Setting | Default | Description |
|---------|---------|-------------|
| Width / Height | 480 × 72 | Source dimensions |
| Interval | 300 s | Time hidden between appearances |
| Show duration | 15 s | How long the pill stays visible |
| Render scale | 3 | SVG raster multiplier (higher = sharper text) |
| Locale | fr-FR | Date formatting (`fr-FR`, `en-US`, …) |
| Weather text | 22°C | Static placeholder |
| Caustic intensity | 0.6 | Strength of animated highlights |

## Project structure

```
src/
  plugin-main.cpp      Plugin entry + source registration
  pill-source.cpp      OBS source (properties, tick, render)
  pill-animation.cpp   Show/hide state machine + easing
  svg-text.cpp         LunaSVG text → gs_texture
  pill-layout.cpp      Horizontal section layout
  time-format.cpp      Clock and date formatting
data/
  effects/pill_glass.effect
  locale/en-US.ini
```

## License

GPL-2.0 (same as OBS plugin template)
