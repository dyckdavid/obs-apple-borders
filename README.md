# OBS Apple Borders

macOS-inspired visuals for OBS Studio: window chrome, Picture-in-Picture, light backdrops, and streaming polish filters/sources.

### Filters

| Filter | What it does |
|--------|----------------|
| **Apple Border** | Rounds the source, draws a Continuity Camera–style hairline stroke, and adds a soft drop shadow. |
| **Apple Picture-in-Picture** | Composites another video source or scene into a corner, with the same Apple chrome on the inset (and optionally on the main source). |
| **Apple Vignette** | Soft circular / elliptical edge darkening. |
| **Apple Spotlight** | Soft radial brightening (center or offset). |
| **Apple Frost** | Frosted-glass multi-sample blur + light wash. |

### Sources

| Source | What it does |
|--------|----------------|
| **Apple Backdrop** | Bundled light Apple white-box / frost panel JPG presets. |
| **Apple Lower Third** | Frosted rounded bar with vivid left accent, style presets, and built-in title/subtitle text. |
| **Apple Soft Gradient** | Procedural multi-hue animated soft mesh gradient (shader-only). |

Designed and packaged for **macOS** (Apple Silicon and Intel). The CMake project also builds on Linux/Windows if `libobs` is available.

---

## Look & defaults

Out of the box (no tweaking required):

- Corner radius ≈ **24 px** at 1080p-class sources (14 px on PiP)
- **1 px** soft white hairline border (~22% opacity)
- Soft black shadow: blur **20**, opacity **0.40**, offset **(0, 10)**
- Content padding reserved so the shadow is not clipped

Avoids neon/gamer bezels — the goal is modern macOS window chrome.

---

## Requirements

| Tool | Notes |
|------|--------|
| **OBS Studio** 30+ (31+ recommended) | Installed app and/or a local source/build tree |
| **Xcode** 15+ (16 recommended) | macOS SDK + command-line tools |
| **CMake** 3.21+ | `brew install cmake` |
| **libobs** headers + library | From an OBS build, Homebrew, or OBS deps |

### Getting libobs on macOS

Pick one approach:

**A. OBS Studio source build (most reliable for plugin authors)**

```bash
git clone --recursive https://github.com/obsproject/obs-studio.git
# Build OBS per upstream docs, then point this plugin at the build tree:
export OBS_DIR="$HOME/obs-studio/build"
```

**B. CMake package prefix**

If you installed OBS deps / a SDK prefix that exports `libobsConfig.cmake`:

```bash
export CMAKE_PREFIX_PATH="/path/to/obs-deps:$CMAKE_PREFIX_PATH"
# or:
cmake -Dlibobs_DIR=/path/to/libobs/cmake ...
```

**C. Manual hints**

```bash
cmake -DOBS_DIR=/path/to/obs-studio \
      -DLIBOBS_INCLUDE_DIR=/path/to/obs-studio/libobs \
      -DLIBOBS_LIB=/path/to/libobs.dylib \
      ...
```

---

## Build (macOS)

### Quick script

```bash
chmod +x scripts/build-macos.sh

# Apple Silicon (default host arch)
OBS_DIR="$HOME/obs-studio/build" ./scripts/build-macos.sh

# Intel Mac
OBS_DIR="$HOME/obs-studio/build" ./scripts/build-macos.sh

# Universal binary (arm64 + x86_64)
OBS_DIR="$HOME/obs-studio/build" ARCHS="arm64;x86_64" ./scripts/build-macos.sh
```

### Manual CMake

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DOBS_DIR="$HOME/obs-studio/build"
cmake --build build --config Release
cmake --install build --config Release
```

On success you get:

```
build/rundir/obs-apple-borders.plugin
```

`cmake --install` copies it to the per-user OBS plugins folder (see below).

### Xcode generator (optional)

```bash
cmake -S . -B build -G Xcode -DOBS_DIR="$HOME/obs-studio/build"
cmake --build build --config Release
```

---

## Install on macOS

OBS loads bundles from:

```text
~/Library/Application Support/obs-studio/plugins/obs-apple-borders.plugin
```

Bundle layout:

```text
obs-apple-borders.plugin/
  Contents/
    Info.plist
    MacOS/obs-apple-borders
    Resources/
      effects/apple_chrome.effect
      locale/en-US.ini
```

After installing, **fully quit and reopen OBS Studio**. Confirm the module loaded under **Help → Log Files → View Current Log** (search for `obs-apple-borders`).

> Gatekeeper: locally built unsigned plugins may need a right-click → **Open** once, or `xattr -dr com.apple.quarantine` on the `.plugin` if macOS blocks it.

---

## Use in OBS

### Apple Border

1. Right-click a video source (Display Capture, Video Capture Device, Window Capture, Media, Browser, …)
2. **Filters → + → Apple Border**
3. Leave the defaults, or adjust radius / border / shadow / padding

Tip: if the shadow looks clipped, raise **Content Padding**.

### Apple Picture-in-Picture

1. Add your main full-frame source to the scene
2. Right-click it → **Filters → + → Apple Picture-in-Picture**
3. Set **PiP Source** to another source or scene (e.g. webcam)
4. Choose **PiP Corner** (top-left / top-right / bottom-left / bottom-right)
5. Tune **PiP Size (%)** and **Margin X/Y**
6. Keep **Apply Chrome to PiP** on for the Apple inset look
7. Optionally enable **Apply Chrome to Main Source**, or stack the separate **Apple Border** filter instead

Typical Continuity-style layout: main display capture + webcam PiP in the bottom-right.

---

## Properties reference

### Apple Border

| Property | Default | Description |
|----------|---------|-------------|
| Corner Radius | 24 px | macOS-like rounding |
| Content Padding | 32 px | Inset reserved for shadow |
| Enable Border | on | Hairline stroke |
| Border Thickness | 1 px | Stroke width |
| Border Color | soft white | Includes alpha |
| Enable Shadow | on | Soft drop shadow |
| Shadow Blur / Opacity / Offset | 20 / 0.40 / (0, 10) | Shadow shape |

### Apple PiP

| Property | Default | Description |
|----------|---------|-------------|
| PiP Source | (none) | Secondary source or scene |
| PiP Corner | Bottom Right | Placement |
| PiP Size | 28% | Width relative to parent |
| Margin X/Y | 24 px | Gap from edges |
| Apply Chrome to Main / PiP | off / on | Where to draw Apple chrome |
| Main / PiP chrome groups | — | Same controls as Apple Border |

---

## Repository layout

```text
obs-apple-borders/
├── CMakeLists.txt              # macOS .plugin bundle + libobs discovery
├── cmake/FindLibObs.cmake      # Finds libobs via package, pkg-config, or OBS_DIR
├── scripts/build-macos.sh      # One-shot configure/build/install
├── resources/macos/Info.plist.in
├── data/
│   ├── effects/apple_chrome.effect   # SDF rounded rect + border + shadow
│   └── locale/en-US.ini
├── src/
│   ├── plugin-main.c           # Module load; registers both filters
│   ├── apple-border-filter.c   # Apple Border filter
│   ├── apple-pip-filter.c      # Apple PiP filter
│   └── apple-chrome-util.*     # Shared defaults, properties, effect binding
├── LICENSE                     # MIT
└── README.md
```

---

## Technical notes

- Filters use **libobs GS effects** (`obs_source_process_filter_*`) so rounding and AA stay crisp on GPU.
- The chrome shader uses a **rounded-rectangle SDF** for the mask, hairline ring, and soft shadow falloff.
- PiP renders the selected source to a `gs_texrender`, then draws it with the same chrome effect.
- Weak source references avoid leaking destroyed PiP targets; show/activate refcounts keep media sources running while the filter is visible.

---

## License

MIT — see [LICENSE](LICENSE).

When you distribute a binary linked against OBS Studio (`libobs`, GPL-2.0), follow OBS’s licensing requirements for the combined work.
