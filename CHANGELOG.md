# Changelog

## 1.4.0

### Arch Linux & Omarchy

- Native Arch package (`.pkg.tar.zst`) built in CI against Arch’s `obs-studio`, attached to GitHub Releases alongside macOS / Windows / Ubuntu assets.
- Install with a double-click in a graphical package installer, or `sudo pacman -U file.pkg.tar.zst`.
- AUR-ready PKGBUILDs under `packaging/arch/` (source + `-bin` that uses the release asset).
- Install paths match Arch OBS: `/usr/lib/obs-plugins` and `/usr/share/obs/obs-plugins`.
- Documented for Arch and Omarchy (Hyprland / Wayland) in the README.

### Borders & chrome

- Border style presets: Hairline Light/Dark, Thick, Glass, Soft Glow, Double, Inner Glow, Inset, Solid Light/Dark, Accent Blue, None, Custom.
- Per-corner radius (optional) with sensible uniform default.
- Gradient border color (start/end + angle).
- Shadow presets: None, Soft, Medium, Deep, Floating, Custom.
- Properties UI grouped into Shape / Border / Shadow with sensible defaults.

### Backdrops

- Eight new bundled Apple-style backgrounds (light + dark), same quiet panel aesthetic — no neon/gamer look:
  Porcelain Shelf, Linen Panels, Cloud Layers, Paper Tiles, Charcoal Panels, Midnight Stack, Slate Ribbons, Dusk Veil.

### Other

- Linux lower-third text prefers **Noto Sans** (common on Arch / Omarchy).
- Version bumped to 1.4.0 everywhere. macOS signing/notarization CI steps unchanged.

## 1.3.0

- Migrated to obs-plugintemplate CMake + CI layout.
- Cross-platform installers: macOS signed/notarized pkg+dmg, Windows Inno exe+zip, Linux deb on `v*` tags.

## 1.2.2

- Initial public import.
