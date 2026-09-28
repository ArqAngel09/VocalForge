# VocalForge

Original adaptive vocal-mixing VST3/Standalone prototype for Windows.

## Features
- Up to 15-second vocal analysis.
- Level, crest-factor and spectral-proxy analysis.
- Automatic body, presence, air, compression, saturation, de-essing and space settings.
- Manual controls after the automatic pass.
- VST3 and Standalone targets.

This is an original implementation inspired by the general workflow of automatic vocal processors. It does not copy Fresh Vocal's code, UI, branding, presets or proprietary algorithms.

## Build
JUCE 9.0.2 is pinned through CMake FetchContent.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

GitHub Actions builds the Windows VST3 and packages the installer.

Build pipeline targets the current GitHub Windows runner toolchain.