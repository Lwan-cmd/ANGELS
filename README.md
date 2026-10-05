# ANGEL ENGINE — v0.1

Experimental stereo VST3 effect built with JUCE.

## Macros

- **AGE** — sample-hold degradation + progressive low-pass wear
- **AIR** — high-frequency emphasis / airy edge
- **GHOST** — feedback memory delay
- **WIDTH** — mid/side stereo expansion
- **MELT** — unstable modulated micro-delay + saturation
- **CHAOS** — slow irregular modulation injected into MELT and GHOST

## Browser-only build

This repository includes a GitHub Actions workflow.

1. Push the project to GitHub.
2. Open the repository's **Actions** tab.
3. Run **Build ANGEL ENGINE VST3** if it has not started automatically.
4. When the job succeeds, download the artifact named:
   `ANGEL-ENGINE-Windows-VST3`
5. On your personal Windows PC, extract the artifact and place the `.vst3`
   bundle in your VST3 folder (normally `C:\Program Files\Common Files\VST3`).
6. Rescan plugins in FL Studio.

## Local build later

Requires CMake and a Windows C++ toolchain.

```powershell
cmake -S . -B build
cmake --build build --config Release --target AngelEngine_VST3
```

JUCE is downloaded automatically at version **9.0.3** via CMake FetchContent.

## Current status

v0.1 is intentionally simple. The point of this build is to verify:

- VST3 compilation
- FL Studio loading
- parameter automation
- state recall
- stereo processing
- first-pass ANGEL ENGINE DSP

The sound engine will be refined after listening tests.
