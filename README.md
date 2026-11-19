# EasyCut

Minimal low-cut (high-pass) VST3 plugin built with JUCE.

- **Freq:** 30-500 Hz (default 120 Hz)
- **Slope:** 12 / 24 / 48 / 72 dB/oct (Butterworth biquads) and Brick Wall (linear-phase FIR, adds latency)
- Live pre/post spectrum with filter curve

## Build (Windows)

Requires Visual Studio 2022 (C++ workload) and CMake 3.22+. JUCE is fetched automatically.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `build/EasyCut_artefacts/Release/VST3/EasyCut.vst3`. Copy the whole folder to `C:\Program Files\Common Files\VST3\`.
