# SensShift

**Real-time mouse sensitivity and input transformation for Windows.**

SensShift is a lightweight Windows utility for dynamically modifying mouse movement through configurable hotkeys, sensitivity profiles, axis multipliers, and acceleration curves.

Originally intended for situations in FPS games where different sensitivity behavior is useful during aiming or recoil control, SensShift is designed as a general-purpose mouse input transformation tool.

> Windows  •  Native  •  Low-latency  •  Configurable

## Features

### Dynamic Sensitivity

Change mouse sensitivity instantly using configurable hotkeys.

* X-axis sensitivity
* Y-axis sensitivity
* Independent X/Y multipliers
* Overall sensitivity multiplier
* Hold or toggle activation

For example:

```text
Normal:
X × 1.0
Y × 1.0

While hotkey is held:
X × 0.5
Y × 1.0
```

This allows you to temporarily reduce horizontal sensitivity without affecting vertical movement.

### Mouse Acceleration

Apply customizable acceleration to mouse movement.

SensShift is designed around raw mouse movement, allowing acceleration behavior to be defined independently from the standard Windows mouse acceleration setting.

Planned acceleration curves include:

* Linear
* Polynomial
* Exponential
* Custom curves

Example:

```text
Input speed      Multiplier
──────────────   ──────────
Low              1.00×
Medium           1.25×
High             1.60×
Very high        2.00×
```

### Configurable Hotkeys

Bind transformations to keyboard or mouse inputs.

Examples:

```text
Shift
Ctrl + Shift
Mouse Button 4
Mouse Button 5
Shift + Mouse Button 4
```

Supported activation modes:

* **Hold** — active only while the hotkey is held
* **Toggle** — press once to activate, press again to deactivate

### Profiles

Create multiple sensitivity profiles and switch between them instantly.

Example:

```text
Default
├── X: 1.00
├── Y: 1.00
└── Acceleration: Off

Precision
├── X: 0.50
├── Y: 0.50
└── Acceleration: Off

Fast Turn
├── X: 2.00
├── Y: 2.00
└── Acceleration: On
```

Profiles can optionally be associated with different applications.

### Per-Application Profiles

SensShift can automatically select a profile based on the currently focused application.

For example:

```text
Global
└── Default

PUBG
└── FPS Profile

Photoshop
└── Precision Profile

Desktop
└── Default
```

### Input Transformations

The processing pipeline is designed to support more than simple sensitivity scaling.

A transformation can modify:

* X movement
* Y movement
* Overall sensitivity
* Movement-dependent acceleration
* Future custom transformations

Transformations can be combined into a configurable pipeline.

---

## How It Works

SensShift processes mouse movement through a configurable transformation pipeline.

Conceptually:

```text
Physical Mouse
      │
      ▼
Raw Mouse Movement
      │
      ▼
┌──────────────────────┐
│ Sensitivity Modifier │
└──────────┬───────────┘
           ▼
┌──────────────────────┐
│ Acceleration Curve   │
└──────────┬───────────┘
           ▼
┌──────────────────────┐
│ Axis Transformation  │
└──────────┬───────────┘
           ▼
     Modified Input
```

The processing path is designed to remain lightweight so mouse movement can be transformed with minimal additional latency.

## Sensitivity

SensShift supports independent X/Y scaling.

Given:

```text
Raw X = 10
Raw Y = 5
```

and:

```text
X multiplier = 0.5
Y multiplier = 1.0
```

the resulting movement becomes approximately:

```text
X = 5
Y = 5
```

Fractional sensitivity values are supported:

```text
X = 0.35
Y = 1.00
```

Where necessary, fractional movement is accumulated so that small values are not simply discarded due to integer rounding.

## Acceleration

Acceleration is based on mouse movement rather than cursor position.

Conceptually:

```text
speed = √(dx² + dy²)

multiplier = accelerationCurve(speed)

outputX = dx × multiplier
outputY = dy × multiplier
```

This makes it possible to create behaviors such as:

```text
Slow movement  → precise
Fast movement  → increasingly sensitive
```

The acceleration system is designed to be extensible so additional curves can be added without changing the core input system.

## Input Architecture

SensShift uses native Windows input APIs where possible.

The exact interception and injection architecture is being developed with compatibility and latency in mind. Windows applications can consume mouse input through different mechanisms, including:

* Standard Windows mouse input
* Raw Input
* DirectInput
* Application-specific input systems

Because of this, a user-mode input transformation cannot necessarily guarantee identical behavior across every application or game.

SensShift will document compatibility and limitations as the project develops.

## Performance

SensShift is intended to work with high-frequency mouse input and minimize additional latency.

The mouse processing path avoids unnecessary:

* Cursor-position polling
* Memory allocations
* File I/O
* Logging
* GUI operations

The goal is to keep the input path lightweight even with high-polling-rate mice.

## Safety

SensShift is designed to restore normal mouse behavior when the application is disabled or closed.

An emergency hotkey can also disable all active transformations.

Example:

```text
Ctrl + Shift + F12
        ↓
Disable all modifiers
```

## Configuration

SensShift uses a human-readable configuration format such as JSON.

Example:

```json
{
    "profiles": [
        {
            "name": "Precision",
            "hotkey": "SHIFT",
            "activation": "hold",
            "xMultiplier": 0.5,
            "yMultiplier": 0.5,
            "acceleration": {
                "enabled": false
            }
        }
    ]
}
```

The configuration format may change during early development.

## Status

> **Early development**

The project is currently focused on establishing the core input-processing architecture.

### Planned

* [ ] X/Y sensitivity modifiers
* [ ] Overall sensitivity modifier
* [ ] Hold hotkeys
* [ ] Toggle hotkeys
* [ ] Mouse-button hotkeys
* [ ] Profile system
* [ ] JSON configuration
* [ ] Raw mouse input processing
* [ ] Acceleration curves
* [ ] Custom acceleration curves
* [ ] Per-application profiles
* [ ] GUI
* [ ] System tray integration
* [ ] Real-time acceleration curve editor
* [ ] High-polling-rate mouse testing
* [ ] Advanced input transformation pipeline

## Building

### Requirements

* Windows 10 or Windows 11
* C++ compiler with modern C++ support
* CMake
* Windows SDK

### Build

```bash
git clone https://github.com/YOUR_USERNAME/SensShift.git
cd SensShift

cmake -S . -B build
cmake --build build --config Release
```

The resulting executable will be located in the build output directory.

## Roadmap

### Phase 1 — Core

* Raw mouse input
* X/Y sensitivity
* Hotkeys
* Input transformation pipeline

### Phase 2 — Profiles

* Multiple profiles
* Hold/toggle activation
* Application-specific profiles
* Persistent configuration

### Phase 3 — Acceleration

* Built-in acceleration curves
* Custom curves
* Curve editor
* Transformation ordering

### Phase 4 — UI

* Settings interface
* Real-time input visualization
* Curve editor
* System tray integration

### Phase 5 — Advanced

* Additional input transformations
* Advanced hotkey combinations
* High-polling-rate optimization
* Expanded application compatibility

## Contributing

Contributions, ideas, bug reports, and performance testing are welcome.

When reporting an input compatibility issue, please include:

* Windows version
* Mouse model
* Mouse polling rate
* Application/game
* Input mode, if known
* SensShift configuration
* Description of the behavior

## License

License: **TBD**

## Why SensShift?

Mouse sensitivity does not always need to be static.

SensShift is built around a simple idea:

> **When I do X, transform my mouse movement like Y.**

Whether that means temporarily lowering sensitivity for precision, increasing it for fast turns, applying custom acceleration, or experimenting with more advanced input transformations, SensShift aims to make mouse behavior programmable.
