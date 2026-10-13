# OBS Apple Borders

macOS-inspired visuals for OBS Studio: window chrome, Picture-in-Picture, light backdrops, and streaming polish filters/sources.

**Version 1.3.0** — built on the official [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate) CMake + CI layout. Requires **OBS Studio 30+**.

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

Stable plugin IDs (do not rename — existing scenes depend on them):

- Filters: `apple_border_filter`, `apple_pip_filter`, `apple_frost_filter`, `apple_vignette_filter`, `apple_spotlight_filter`
- Sources: `apple_backdrop_source`, `apple_lower_third_source`, `apple_soft_gradient_source`

---

## Requirements

| Tool | Notes |
|------|--------|
| **OBS Studio** 30+ (31+ recommended) | Runtime for the plugin |
| **CMake** 3.28+ | From the OBS plugin template |
| **Ninja** (Linux) / **Xcode 16** (macOS) / **VS 2022** (Windows) | Platform toolchain |
| **libobs** | Via OBS deps (CI), Homebrew/OBS build (macOS), or `libobs-dev` / OBS PPA (Linux) |

---

## Install (release packages)

### macOS

1. Download the universal `.pkg` from [Releases](https://github.com/dyckdavid/obs-apple-borders/releases).
2. Open the installer and follow the prompts.
3. Fully quit and reopen OBS Studio.
4. Confirm load under **Help → Log Files → View Current Log** (search for `obs-apple-borders`).

Manual / unsigned builds install to:

```text
~/Library/Application Support/obs-studio/plugins/obs-apple-borders.plugin
```

If Gatekeeper blocks a local build: right-click → **Open**, or clear quarantine with `xattr -dr com.apple.quarantine` on the `.plugin`.

### Windows

1. Download the Windows installer (or zip) from Releases.
2. Run the installer, **or** extract the zip so that:
   - `obs-apple-borders.dll` lands in OBS `obs-plugins/64bit`
   - `data/obs-plugins/obs-apple-borders/` contains `effects/` and `locale/`
3. Restart OBS Studio.

Typical portable / zip layout:

```text
obs-plugins/64bit/obs-apple-borders.dll
data/obs-plugins/obs-apple-borders/...
```

### Linux

1. Download the `.deb` (Ubuntu/Debian) or tarball from Releases.
2. Install with your package manager, for example:

```bash
sudo apt install ./obs-apple-borders_*.deb
```

Or unpack the tarball into the OBS plugin + data paths used by your distro (often `/usr/lib/x86_64-linux-gnu/obs-plugins/` and `/usr/share/obs/obs-plugins/obs-apple-borders/`).

3. Restart OBS Studio.

---

## Build from source

This project follows the OBS plugin template presets (`macos`, `windows-x64`, `ubuntu-x86_64`).

### Linux (Ubuntu 24.04+)

```bash
# Dependencies (system libobs 30+ or OBS PPA)
sudo apt install build-essential cmake ninja-build pkg-config \
  libobs-dev libsimde-dev zsh jq ccache

cmake --preset ubuntu-x86_64
cmake --build --preset ubuntu-x86_64
cmake --install build_x86_64 --prefix release/RelWithDebInfo
```

Or use the template script (adds the OBS PPA and builds a `.deb` when packaging):

```bash
.github/scripts/build-ubuntu -t ubuntu-x86_64 -c RelWithDebInfo
```

### macOS

```bash
# Xcode 16 + CMake 3.30 recommended; OBS deps are fetched by the template buildspec
cmake --preset macos
cmake --build --preset macos --config RelWithDebInfo
```

Universal CI-style build / package:

```bash
.github/scripts/build-macos -t macos-universal -c RelWithDebInfo
.github/scripts/package-macos -t macos-universal -c RelWithDebInfo --package
```

### Windows

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --config RelWithDebInfo
# Packaging:
.github/scripts/Build-Windows.ps1 -Target x64 -Configuration RelWithDebInfo
.github/scripts/Package-Windows.ps1 -Target x64 -Configuration RelWithDebInfo -Package
```

---

## Use in OBS

### Apple Border

1. Right-click a video source → **Filters → + → Apple Border**
2. Leave the defaults, or adjust radius / border / shadow / padding

Tip: if the shadow looks clipped, raise **Content Padding**.

### Apple Picture-in-Picture

1. Add your main full-frame source to the scene
2. Right-click it → **Filters → + → Apple Picture-in-Picture**
3. Set **PiP Source** to another source or scene
4. Choose corner, size, and margins
5. Keep **Apply Chrome to PiP** on for the Apple inset look

---

## Repository layout

```text
obs-apple-borders/
├── CMakeLists.txt                 # obs-plugintemplate project
├── buildspec.json                 # name, version, OBS/deps pins, macOS bundle id
├── CMakePresets.json              # macos / windows-x64 / ubuntu-x86_64
├── cmake/                         # Official template helpers (linux/macos/windows)
├── .github/                       # Actions: build + package all platforms
├── data/
│   ├── effects/                   # GS effect shaders
│   ├── backgrounds/               # Backdrop JPG presets
│   └── locale/en-US.ini
├── src/
│   ├── plugin-main.c              # Module load; registers all filters/sources
│   ├── plugin-support.*           # Logging helpers
│   ├── common/                    # Shared chrome, effect, filter helpers
│   ├── filters/                   # Border, PiP, Frost, Vignette, Spotlight
│   └── sources/                   # Backdrop, Lower Third, Soft Gradient
├── LICENSE                        # MIT
└── README.md
```

---

## Continuous integration

GitHub Actions (from the OBS plugin template) build release artifacts for:

| Platform | Artifact |
|----------|----------|
| **macOS** | Universal (`arm64` + `x86_64`) `.pkg` |
| **Windows** | Installer / zip (`x64`) |
| **Linux** | `.deb` and/or tarball (`x86_64`) |

Workflows: `push.yaml`, `pr-pull.yaml`, `dispatch.yaml` → `build-project.yaml`.

---

## Compatibility

Scene collections from v1.2.x keep working: filter/source **IDs** and **setting keys** are unchanged in 1.3.0.

---

## License

MIT — see [LICENSE](LICENSE).

When you distribute a binary linked against OBS Studio (`libobs`, GPL-2.0), follow OBS’s licensing requirements for the combined work.
