# KatieFlySimRust

A Rust port of the FlySimNewA space flight simulator game.

**Now using macroquad - Pure Rust graphics with ZERO external dependencies!**

## Quick Start

### Windows
Double-click `run.cmd` or run from command prompt:
```cmd
run.cmd
```

### Linux/macOS
Run from terminal:
```bash
./run.sh
```

Or manually:
```bash
cd KatieFlySimRust
cargo run --release
```

## Requirements

**Only Rust is required!** - Install from [rustup.rs](https://rustup.rs/)
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

That's it! No external graphics libraries needed.

## Why Macroquad?

We've ported from SFML to **macroquad**, a pure Rust game library that:
- ✅ **Zero external dependencies** - No SFML, no SDL, nothing to install!
- ✅ **Cross-platform** - Works on Windows, Linux, macOS out of the box
- ✅ **Simple and fast** - Lightweight immediate-mode rendering
- ✅ **Just `cargo run`** - Clone and play in seconds

## Game Controls

- **Space**: Thrust
- **A/D** or **Left/Right Arrow**: Rotate
- **E**: Launch from planet / Detach from rocket
- **C**: Convert rocket to satellite
- **F**: Toggle camera follow mode
- **F5**: Quick-save
- **Escape**: Return to menu
- **Mouse Wheel**: Zoom in/out

## Features

- Physics-based orbital mechanics
- Rocket control with fuel management
- Dynamic mass system
- Camera zoom and follow
- Real-time HUD display
- Save/load system with auto-save
- Single-player mode
- Pure Rust implementation (no C++ dependencies!)

## Project Structure

See `KatieFlySimRust/` for the complete Rust source code and detailed documentation.

## Development

Build only:
```bash
cd KatieFlySimRust
cargo build --release
```

Run tests:
```bash
cd KatieFlySimRust
cargo test
```

Run with logging:
```bash
cd KatieFlySimRust
RUST_LOG=info cargo run --release
```

## Documentation

- `KatieFlySimRust/RUST_PORT_PLAN.md` - Complete 16-phase conversion plan
- `KatieFlySimRust/FILE_MAPPING.md` - C++ to Rust file mapping
- `KatieFlySimRust/CPP_TO_RUST_PATTERNS.md` - Translation patterns guide
- `KatieFlySimRust/PROGRESS.md` - Current implementation status

## Current Status

- ✅ Complete SFML → macroquad port
- ✅ Zero external dependencies
- ✅ ~4,150 lines of Rust code
- ✅ **39 unit tests passing**
- ✅ **Playable single-player game**
- ✅ Full feature parity with SFML version

## Technical Highlights

**Graphics**: Pure Rust using macroquad (no C++ bindings)
**Physics**: Custom n-body gravity simulation
**Architecture**: Entity ID pattern for clean ownership
**Serialization**: JSON-based saves with serde
**Testing**: Comprehensive unit tests for all systems

## License

Same as original FlySimNewA project.
