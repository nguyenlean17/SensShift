# SensShift

**Real-time mouse sensitivity and input transformation for Windows.**

SensShift is a lightweight utility for dynamically modifying mouse input with hotkeys, sensitivity profiles, axis multipliers, and acceleration curves.

Built primarily for FPS gaming, but usable anywhere you want more control over mouse movement.

> Windows • Native • Low-latency • Configurable

## Features

### Dynamic Sensitivity

Change sensitivity instantly with a hotkey.

```text
Normal
X × 1.0
Y × 1.0

While held
X × 0.5
Y × 1.0
```

Supports:

* X/Y sensitivity
* Overall sensitivity
* Hold or toggle activation
* Fractional multipliers

### Mouse Acceleration

Apply custom acceleration based on mouse movement speed.

```text
Slow   → 1.00×
Medium → 1.25×
Fast   → 1.60×
Very fast → 2.00×
```

Planned curves include:

* Linear
* Polynomial
* Exponential
* Custom curves

### Hotkeys

Bind transformations to keyboard or mouse buttons.

```text
Shift
Ctrl + Shift
Mouse Button 4
Shift + Mouse Button 4
```

Supports **hold** and **toggle** modes.

### Profiles

Create and switch between different mouse configurations.

```text
Default
X: 1.00  Y: 1.00

Precision
X: 0.50  Y: 0.50

Fast Turn
X: 2.00  Y: 2.00
Acceleration: On
```

Profiles can optionally be assigned to specific applications or games.

### Input Pipeline

Mouse input can pass through multiple transformations:

```text
Raw Mouse Input
      ↓
Sensitivity
      ↓
Acceleration
      ↓
Axis Transform
      ↓
Output
```

This allows different transformations to be combined rather than limiting SensShift to simple sensitivity changes.

## Gaming Use Cases

SensShift is designed for situations where you want different mouse behavior without changing your game's sensitivity.

Examples:

* Hold a key to temporarily lower sensitivity for precise aiming
* Use a separate sensitivity for recoil control
* Increase sensitivity for fast turns
* Apply custom acceleration
* Automatically switch settings between games

## Performance

SensShift is designed for high-polling-rate mice and low input latency.

The input path avoids unnecessary polling, allocations, logging, and GUI processing.

The goal is simple:

> **Transform the input without getting in the way.**

## Compatibility

SensShift uses native Windows input APIs.

Compatibility can vary between applications because games may process mouse input through different systems, including Raw Input, DirectInput, or their own input pipelines.

Compatibility will be tested and documented as development progresses.

## Safety

An emergency hotkey can immediately disable all transformations.

```text
Ctrl + Shift + F12
        ↓
Disable all modifiers
```

Closing SensShift restores normal mouse behavior.

## Configuration

SensShift uses a human-readable JSON configuration.

```json
{
    "profiles": [
        {
            "name": "Precision",
            "hotkey": "SHIFT",
            "activation": "hold",
            "xMultiplier": 0.5,
            "yMultiplier": 0.5
        }
    ]
}
```

The configuration format may change during development.

## Building

### Requirements

* Windows 10/11
* Modern C++ compiler
* CMake
* Windows SDK

```bash
git clone https://github.com/YOUR_USERNAME/SensShift.git
cd SensShift

cmake -S . -B build
cmake --build build --config Release
```

## Contributing

Bug reports, ideas, and performance testing are welcome.

For input issues, include:

* Windows version
* Mouse model
* Polling rate
* Game/application
* SensShift configuration
* Description of the issue

## License

**TBD**

## Why SensShift?

I made it hoping to get better at spraying in PUBG. This made me worse. May we suck together.

