# OBS Apple Borders

macOS-inspired visuals for OBS Studio: window chrome, Picture-in-Picture, light backdrops, and streaming polish filters/sources.

**Version 1.3.0** — built on the official [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate) CMake + CI layout. Requires **OBS Studio 30+**.

### Filters

| Filter | What it does |
|--------|----------------|
| **Apple Border** | Rounds the source, draws a Continuity Camera–style hairline stroke, and adds a soft drop shadow. |
| **Apple Picture-in-Picture** | Composites another video source or scene into a corner, with Apple chrome on the inset. |
| **Apple Vignette** | Soft circular / elliptical edge darkening. |
| **Apple Spotlight** | Soft radial brightening (center or offset). |
| **Apple Frost** | Frosted-glass multi-sample blur + light wash. |

### Sources

| Source | What it does |
|--------|----------------|
| **Apple Backdrop** | Bundled light Apple white-box / frost panel JPG presets. |
| **Apple Lower Third** | Frosted rounded bar with accent, style presets, and title/subtitle text. |
| **Apple Soft Gradient** | Procedural multi-hue animated soft mesh gradient. |

Stable plugin IDs (do not rename — existing scenes depend on them):

- Filters: `apple_border_filter`, `apple_pip_filter`, `apple_frost_filter`, `apple_vignette_filter`, `apple_spotlight_filter`
- Sources: `apple_backdrop_source`, `apple_lower_third_source`, `apple_soft_gradient_source`

---

## Install (no terminal)

Download the installer for your system from [Releases](https://github.com/dyckdavid/obs-apple-borders/releases) (or from the Actions artifacts on a build). You do **not** need to use a terminal.

### macOS

1. Download the **`.dmg`** (preferred) or the **`.pkg`**.
2. If you downloaded the DMG: open it, then double-click the **`.pkg`** inside.
3. Click through the installer. It installs for your user account into  
   `~/Library/Application Support/obs-studio/plugins`.
4. Fully quit OBS Studio, then open it again.
5. Use a filter or source as usual (right-click a source → **Filters**, or **+** → Sources).

If macOS warns that the installer is from an unidentified developer (unsigned CI builds): **right-click** the `.pkg` → **Open** → **Open**.

### Windows

1. Download the **`.exe`** installer.
2. Double-click it and follow the prompts.
3. The installer looks up your OBS Studio folder from the Windows registry. Confirm or browse to the folder that contains `bin\64bit\obs64.exe` (usually `C:\Program Files\obs-studio`).
4. Finish the installer, then restart OBS Studio.

A **`.zip`** is also published if you prefer to copy files yourself.

### Linux

1. Download the **`.deb`** package.
2. Double-click it in your file manager (Ubuntu Software / Discover / GDebi), or open it with your package installer.
3. Restart OBS Studio.

---

## After installing

1. Restart OBS Studio completely.
2. Check **Help → Log Files → View Current Log** and search for `obs-apple-borders`.
3. Add **Apple Border** (or another filter/source) from the Filters / Sources lists.

---

## GitHub Release tags

Push a version tag to publish a draft GitHub Release with all installers attached:

```text
v1.3.0
```

Also accepted: `1.3.0`, and prerelease forms like `v1.3.0-beta1` / `v1.3.0-rc1`.

Attached artifacts typically include:

| Platform | Files |
|----------|--------|
| macOS | Universal `.pkg`, `.dmg` (pkg + short README), optional `.tar.xz` |
| Windows | `.exe` installer, `.zip` |
| Linux | `.deb` (and debug symbols when produced) |

---

## macOS codesigning & notarization (for David)

CI **always** builds unsigned installers. Signing and notarization run **only when the secrets below are present**. Without them, builds still succeed.

Add these under **GitHub → Settings → Secrets and variables → Actions**:

| Secret | What it is |
|--------|------------|
| `APPLE_APPLICATION_IDENTITY` | Developer ID Application identity string, e.g. `Developer ID Application: Your Name (TEAMID)` |
| `APPLE_INSTALLER_IDENTITY` | Developer ID Installer identity string, e.g. `Developer ID Installer: Your Name (TEAMID)` |
| `APPLE_CERTIFICATE` | Your `.p12` signing certificate, **base64-encoded** |
| `APPLE_CERTIFICATE_PASSWORD` | Password for that `.p12` |
| `APPLE_KEYCHAIN_PASSWORD` | Optional password for the temporary CI keychain |
| `APPLE_ID` | Apple ID email used for notarization |
| `APPLE_ID_PASSWORD` | App-specific password from [appleid.apple.com](https://appleid.apple.com) (not your normal Apple ID password) |
| `APPLE_PROVISIONING_PROFILE` | Optional; base64 provisioning profile if you use one |

Legacy `MACOS_*` secret names from the OBS plugin template are still accepted as fallbacks.

Notarization is attempted on version tags (`v*.*.*`) when Apple ID secrets are present.

---

## Build from source

| Tool | Notes |
|------|--------|
| **OBS Studio** 30+ | Runtime |
| **CMake** 3.28+ | Plugin template |
| **Ninja** / **Xcode 16** / **VS 2022** | Platform toolchain |
| **libobs** | OBS deps, Homebrew, or `libobs-dev` / OBS PPA |

### Linux

```bash
sudo apt install build-essential cmake ninja-build pkg-config \
  libobs-dev libsimde-dev zsh jq ccache

cmake --preset ubuntu-x86_64
cmake --build --preset ubuntu-x86_64
cmake --install build_x86_64 --prefix release/RelWithDebInfo
```

### macOS

```bash
cmake --preset macos
cmake --build --preset macos --config RelWithDebInfo
```

### Windows

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --config RelWithDebInfo
```

---

## Repository layout

```text
obs-apple-borders/
├── CMakeLists.txt
├── buildspec.json              # name, version, UUIDs, OBS/deps pins
├── cmake/                      # obs-plugintemplate helpers
├── .github/                    # build + package + release workflows
├── resources/macos/            # DMG README for end users
├── data/                       # effects, locale, backgrounds
└── src/
    ├── common/                 # shared chrome / effect / filter helpers
    ├── filters/
    └── sources/
```

---

## Compatibility

Scene collections from v1.2.x keep working: filter/source **IDs** and **setting keys** are unchanged in 1.3.0.

---

## License

MIT — see [LICENSE](LICENSE).

When you distribute a binary linked against OBS Studio (`libobs`, GPL-2.0), follow OBS’s licensing requirements for the combined work.
