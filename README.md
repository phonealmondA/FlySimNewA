# KatieFlySimRust

A Rust port of the FlySimNewA space flight simulator game.

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

1. **Rust** - Install from [rustup.rs](https://rustup.rs/)
   ```bash
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   ```

2. **SFML Libraries** - Required for graphics

   ### Easy Installation (Automated)

   **Linux/macOS:**
   ```bash
   ./install_sfml.sh
   ```

   **Windows (PowerShell):**
   ```powershell
   powershell -ExecutionPolicy Bypass -File install_sfml.ps1
   ```

   ### Manual Installation

   - **Ubuntu/Debian**: `sudo apt-get install libsfml-dev libcsfml-dev`
   - **Fedora/RHEL**: `sudo dnf install SFML-devel CSFML-devel`
   - **Arch Linux**: `sudo pacman -S sfml csfml`
   - **macOS**: `brew install sfml csfml`
   - **Windows**:
     - Option 1: `vcpkg install sfml:x64-windows csfml:x64-windows`
     - Option 2: Download from [SFML website](https://www.sfml-dev.org/download.php)

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

## Documentation

- `KatieFlySimRust/RUST_PORT_PLAN.md` - Complete 16-phase conversion plan
- `KatieFlySimRust/FILE_MAPPING.md` - C++ to Rust file mapping
- `KatieFlySimRust/CPP_TO_RUST_PATTERNS.md` - Translation patterns guide
- `KatieFlySimRust/PROGRESS.md` - Current implementation status

## Current Status

- ✅ 9/16 phases complete (56.25%)
- ✅ 18/28 files ported (64.3%)
- ✅ ~4,150 lines of Rust code
- ✅ 36 unit tests passing
- ✅ **Playable single-player game**

## License

Same as original FlySimNewA project.
