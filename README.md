# VocalCleanupPlugin
Repository for Vocal Cleanup Plugin Investigation + Implementation

## Overview

**VocalCleanupPlugin** is a JUCE-based VST3/AU audio plugin designed to clean up vocal recordings. It provides five processing stages in a single, easy-to-use interface:

| Stage | Controls | Purpose |
|---|---|---|
| **High-Pass Filter** | Frequency (20–500 Hz) | Removes low-end rumble and microphone handling noise |
| **Noise Gate** | Threshold, Attack, Release | Silences quiet passages between vocal phrases |
| **Compressor** | Threshold, Ratio, Attack, Release | Tames peak dynamics and evens out vocal levels |
| **De-Esser** | Frequency (3–16 kHz), Threshold | Reduces harsh sibilance ("s", "sh" sounds) |
| **Output Gain** | Gain (−24 – +12 dB) | Final level trim |

## Requirements

- CMake ≥ 3.22
- C++17 compiler (Clang, GCC, or MSVC)
- Internet access on first build (JUCE is fetched automatically via CMake FetchContent)

## Building

```bash
# 1. Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. Build
cmake --build build --config Release

# 3. The compiled plugin will be located in:
#    build/VocalCleanupPlugin_artefacts/Release/VST3/
#    build/VocalCleanupPlugin_artefacts/Release/AU/      (macOS only)
#    build/VocalCleanupPlugin_artefacts/Release/Standalone/
```

## Project Structure

```
VocalCleanupPlugin/
├── CMakeLists.txt          # Build configuration (JUCE via FetchContent)
└── Source/
    ├── PluginProcessor.h   # AudioProcessor declaration + parameter layout
    ├── PluginProcessor.cpp # DSP implementation
    ├── PluginEditor.h      # GUI declaration
    └── PluginEditor.cpp    # GUI implementation
```

## Plugin Parameters

All parameters are automatable and saved with the session.

| Parameter ID | Name | Range | Default |
|---|---|---|---|
| `highPassFreq` | HP Freq | 20–500 Hz | 80 Hz |
| `gateThreshold` | Gate Thresh | −80–0 dB | −40 dB |
| `gateAttack` | Gate Attack | 0.1–100 ms | 5 ms |
| `gateRelease` | Gate Release | 10–2000 ms | 150 ms |
| `compThreshold` | Comp Thresh | −60–0 dB | −18 dB |
| `compRatio` | Comp Ratio | 1:1–20:1 | 3:1 |
| `compAttack` | Comp Attack | 0.1–200 ms | 10 ms |
| `compRelease` | Comp Release | 10–2000 ms | 100 ms |
| `deEsserFreq` | DeEss Freq | 3000–16000 Hz | 8000 Hz |
| `deEsserThresh` | DeEss Thresh | −60–0 dB | −20 dB |
| `outputGain` | Output Gain | −24–+12 dB | 0 dB |
