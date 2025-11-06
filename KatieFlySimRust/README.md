# KatieFlySimRust 🚀

A complete Rust port of **FlySimNewA** - A physics-based space flight simulator.

## Project Status

🚧 **Under Development** - Currently in Phase 1: Project Setup

See [RUST_PORT_PLAN.md](RUST_PORT_PLAN.md) for detailed conversion plan and progress.

---

## About

KatieFlySimRust is a ground-up rewrite of the C++/SFML space flight simulator in Rust. The game features:

- ✨ **Realistic orbital mechanics** with gravitational simulation
- 🚀 **Dynamic rocket system** with fuel management and multi-stage capabilities
- 🛰️ **Satellite deployment** and autonomous fuel collection
- 🌍 **Multi-planet system** with orbital transfers
- 🎮 **Multiple game modes:**
  - Single player with save/load
  - Online multiplayer (host/client)
  - Local split-screen
- 📊 **Advanced UI** with trajectory prediction, orbital parameters, and HUD

---

## Why Rust?

The port from C++ to Rust brings several advantages:

- **Memory Safety**: No segfaults, buffer overflows, or undefined behavior
- **Fearless Concurrency**: Better performance for physics and networking
- **Modern Tooling**: `cargo` provides excellent package management and build system
- **Explicit Error Handling**: `Result<T, E>` types make error cases clear
- **Cross-Platform**: Easier deployment to Windows, Linux, and macOS
- **Long-term Maintainability**: Rust's strong type system prevents entire classes of bugs

---

## Getting Started

### Prerequisites

- **Rust** (1.70+): Install from [rustup.rs](https://rustup.rs/)
- **SFML** (2.5+): Install SFML development libraries for your platform
  - **Ubuntu/Debian**: `sudo apt-get install libsfml-dev`
  - **macOS**: `brew install sfml`
  - **Windows**: Download from [SFML website](https://www.sfml-dev.org/download.php)

### Building

```bash
# Clone the repository
cd KatieFlySimRust

# Build in debug mode
cargo build

# Build in release mode (optimized)
cargo build --release

# Run the game
cargo run --release
```

### Testing

```bash
# Run all tests
cargo test

# Run specific test
cargo test test_gravity_calculation

# Run with output
cargo test -- --nocapture
```

---

## Project Structure

```
KatieFlySimRust/
├── Cargo.toml              # Rust package configuration
├── README.md               # This file
├── RUST_PORT_PLAN.md       # Detailed conversion plan
├── FILE_MAPPING.md         # C++ to Rust file mapping
│
└── src/
    ├── main.rs             # Entry point
    ├── lib.rs              # Library root
    ├── game_constants.rs   # Global game constants
    │
    ├── entities/           # Game objects (Planet, Rocket, Satellite)
    ├── physics/            # Gravity simulation and orbital mechanics
    ├── systems/            # Manager classes (Vehicle, Satellite, Fuel)
    ├── ui/                 # User interface and HUD
    ├── menus/              # Menu screens
    ├── game_modes/         # Single player, split screen
    ├── networking/         # Multiplayer host/client
    ├── save_system/        # Save/load functionality
    └── utils/              # Helper functions
```

See [FILE_MAPPING.md](FILE_MAPPING.md) for detailed C++ to Rust module mapping.

---

## Development Roadmap

### Phase 1: Project Setup ⏳ (Week 1)
- [x] Initialize Rust project
- [ ] Configure dependencies
- [ ] Set up module structure

### Phase 2: Core Infrastructure (Week 2)
- [ ] Port game constants
- [ ] Port vector helpers
- [ ] Create common types

### Phase 3: Base Game Objects (Weeks 3-5)
- [ ] Design entity system
- [ ] Port Planet, Rocket, Satellite
- [ ] Resolve ownership model

### Phase 4-16: See [RUST_PORT_PLAN.md](RUST_PORT_PLAN.md)

**Estimated Completion:** 21 weeks (5 months)

---

## Key Technical Decisions

### Graphics Library: SFML (`sfml` crate)
- **Pros:** Familiar API, minimal friction for porting
- **Cons:** Unsafe Rust bindings, older design
- **Alternative:** Consider migrating to Bevy ECS in the future

### Ownership Model: Entity IDs + HashMap
- **Approach:** Entities reference each other by ID, not direct references
- **Reason:** Avoids Rust borrow checker complexity
- **Trade-off:** Manual lookups vs. direct pointers

### Networking: Tokio (async)
- **Approach:** Async TCP networking with `tokio`
- **Reason:** Industry standard, better than blocking SFML network
- **Challenge:** Coordination with synchronous game loop

See [RUST_PORT_PLAN.md](RUST_PORT_PLAN.md) for detailed technical decisions.

---

## Controls

### Single Player
- **Arrow Keys / WASD**: Rotate rocket
- **Space**: Thrust
- **[/]**: Decrease/Increase throttle
- **T**: Convert to satellite
- **L**: Launch new rocket
- **F5**: Quick save
- **F9**: Quick load
- **ESC**: Menu

### Split Screen
- **Player 1**: WASD + Space
- **Player 2**: Arrow Keys + Enter

---

## Contributing

This is a port-in-progress. Contributions are welcome!

### How to Help
1. **Port a module**: Pick a file from [FILE_MAPPING.md](FILE_MAPPING.md)
2. **Write tests**: Add unit tests for physics calculations
3. **Optimize**: Profile and improve performance
4. **Document**: Improve comments and documentation

### Coding Standards
- Follow Rust naming conventions (snake_case for functions, CamelCase for types)
- Write doc comments for public APIs
- Add unit tests for physics and game logic
- Run `cargo fmt` and `cargo clippy` before committing

---

## Testing

### Physics Tests
```bash
cargo test physics
```

### Integration Tests
```bash
cargo test --test integration
```

### Performance Profiling
```bash
cargo install cargo-flamegraph
cargo flamegraph --bin katie_fly_sim_rust
```

---

## Known Issues

- [ ] Entity ownership model still under design (Phase 3)
- [ ] Networking not yet implemented (Phase 10-12)
- [ ] UI rendering incomplete (Phase 6)

See GitHub Issues for full list.

---

## License

This project is licensed under MIT OR Apache-2.0.

Original C++ version: FlySimNewA

---

## Resources

### Rust Learning
- [The Rust Book](https://doc.rust-lang.org/book/)
- [Rust by Example](https://doc.rust-lang.org/rust-by-example/)

### Game Development
- [Rust SFML Tutorial](https://www.rust-sfml.org/tutorials.html)
- [Are We Game Yet?](https://arewegameyet.rs/)

### Networking
- [Tokio Tutorial](https://tokio.rs/tokio/tutorial)
- [Fast-paced Multiplayer](https://www.gabrielgambetta.com/client-server-game-architecture.html)

---

## Contact

For questions about the Rust port, open an issue on GitHub.

**Happy coding! 🦀🚀**
