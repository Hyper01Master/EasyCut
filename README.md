# EasyCut

Minimal, fast low-cut (high-pass) filter plugin (VST3) built with [JUCE](https://juce.com).

![status](https://img.shields.io/badge/format-VST3-a78bfa) ![platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey) ![license](https://img.shields.io/badge/license-AGPL--3.0-blue)

## Features

- **Freq** (left knob): 30-500 Hz, logarithmic, default 120 Hz, smoothed.
- **Slope** (right knob): 12 / 24 / 48 / 72 dB/oct (Butterworth biquad cascade, zero latency) and **Brick Wall** (linear-phase FIR, adds latency).
- Live spectrum display: grey = before the filter, purple = after, white line = filter curve.
- Dark theme with lavender accents.

## Install (no build needed)

1. Download `EasyCut-v0.1-VST3.zip` from the [Releases](../../releases) page and unzip it.
2. Copy the **`EasyCut.vst3` folder** to `C:\Program Files\Common Files\VST3\`.
3. Rescan plugins in your DAW and add EasyCut as an effect.

Requires 64-bit Windows and a VST3-capable DAW. If the plugin does not load, install the
[VC++ Redistributable x64](https://aka.ms/vs/17/release/vc_redist.x64.exe).

## Build from source

Requires Visual Studio 2022 (C++ workload) and CMake 3.22+. JUCE 8.0.6 is fetched automatically.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `build/EasyCut_artefacts/Release/VST3/EasyCut.vst3`

## Notes

- Brick Wall reports 2048 samples of latency (4096 at 88.2 kHz and above); hosts normally compensate automatically.
- At very low cutoffs the Brick Wall transition band is roughly 50-60 Hz wide.
- Early version (v0.1): not yet tested across many DAWs.

## Project layout

```
Source/Dsp.h              biquad, slope design, analyser FIFO
Source/PluginProcessor.*  audio processing, parameters, FIR builder
Source/PluginEditor.*     UI layout
Source/SpectrumDisplay.h  spectrum + filter curve
Source/LookAndFeel.h      theme and knob drawing
```

## License

AGPL-3.0 (see `LICENSE`), as required by JUCE's open-source licence.
