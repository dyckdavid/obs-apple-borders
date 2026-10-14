# Arch Linux / Omarchy packaging

Two PKGBUILDs ready for AUR submission:

| Directory | Package | Use when |
|-----------|---------|----------|
| `obs-apple-borders/` | Source build | Building against Arch’s `obs-studio` |
| `obs-apple-borders-bin/` | Prebuilt | Installing the GitHub Release `.pkg.tar.zst` |

## Local install from a release asset

```bash
sudo pacman -U obs-apple-borders-1.4.0-x86_64.pkg.tar.zst
```

Or double-click the file in Dolphin / Nautilus / Thunar (opens the graphical package installer).

Plugin files land at:

- `/usr/lib/obs-plugins/obs-apple-borders.so`
- `/usr/share/obs/obs-plugins/obs-apple-borders/`

Restart OBS Studio (Wayland / Hyprland included). Check **Help → Log Files** for `obs-apple-borders`.

## Build with makepkg (source)

```bash
cd packaging/arch/obs-apple-borders
makepkg -si
```
