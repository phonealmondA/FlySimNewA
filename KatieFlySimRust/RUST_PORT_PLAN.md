# KatieFlySimRust - Complete Port Plan

## Project Overview
Converting **FlySimNewA** (C++/SFML) → **KatieFlySimRust** (Rust)

- **Original Code:** ~15,000 lines across 56 files
- **Estimated Timeline:** 4-5 months (16-21 weeks)
- **Difficulty Level:** Medium-Hard (ownership model challenges)

---

## 📋 Phase Breakdown

### PHASE 1: PROJECT SETUP (Week 1)
**Goal:** Establish Rust project infrastructure

- [ ] Initialize Rust project with `cargo new --bin KatieFlySimRust`
- [ ] Configure Cargo.toml with dependencies:
  - `sfml = "0.21"` (Rust bindings to SFML)
  - `serde = { version = "1.0", features = ["derive"] }`
  - `serde_json = "1.0"` (for save files)
  - `bincode = "1.3"` (for network serialization)
  - `tokio = { version = "1", features = ["full"] }` (async networking)
- [ ] Set up module structure (lib.rs, main.rs)
- [ ] Create README.md documenting Rust port
- [ ] Set up build configuration and workspace

**Files Created:**
```
KatieFlySimRust/
├── Cargo.toml
├── Cargo.lock
├── README.md
└── src/
    ├── main.rs
    └── lib.rs
```

---

### PHASE 2: CORE INFRASTRUCTURE (Week 2)
**Goal:** Port foundational utilities and constants

**C++ Files → Rust Modules:**
- `VectorHelper.h` → `src/utils/vector_helper.rs`
- `GameConstants.h/.cpp` → `src/game_constants.rs`

**Tasks:**
- [ ] Port VectorHelper.h to vector_helper.rs module
- [ ] Port GameConstants.h/.cpp to game_constants.rs
- [ ] Create common types module (Vec2, Color wrappers)
- [ ] Create math utilities module for physics

**Key Challenges:**
- Replace C++ `constexpr` with Rust `const` or `lazy_static!`
- Handle SFML vector types vs Rust native types

---

### PHASE 3: BASE GAME OBJECTS (Weeks 3-5)
**Goal:** Core entity system with ownership model

**C++ Files → Rust Modules:**
- `GameObject.h/.cpp` → `src/entities/game_object.rs`
- `RocketPart.h/.cpp` → `src/entities/rocket_part.rs`
- `Engine.h/.cpp` → `src/entities/engine.rs`
- `Planet.h/.cpp` → `src/entities/planet.rs`
- `Rocket.h/.cpp` → `src/entities/rocket.rs`
- `Satellite.h/.cpp` → `src/entities/satellite.rs`

**Tasks:**
- [ ] Design GameObject trait or enum system
- [ ] Port GameObject base class
- [ ] Port RocketPart with component pattern
- [ ] Port Engine with thrust mechanics
- [ ] Port Planet with fuel storage and gravity
- [ ] Port Rocket with dynamic mass, fuel consumption, thrust control
- [ ] Port Satellite with auto-collection
- [ ] **Resolve ownership model for planet-rocket references**

**Key Challenges:**
```rust
// C++ uses raw pointers:
std::vector<Planet*> nearbyPlanets;

// Rust options:
// Option A: ID-based lookup
Vec<usize> nearby_planet_ids;

// Option B: Rc<RefCell<>>
Vec<Rc<RefCell<Planet>>>

// Option C: ECS pattern (Bevy)
Query<&Planet>
```

**Recommended Approach:**
Use **Entity IDs + HashMap** for simplicity:
```rust
struct EntityId(usize);
struct World {
    planets: HashMap<EntityId, Planet>,
    rockets: HashMap<EntityId, Rocket>,
}
```

---

### PHASE 4: PHYSICS SYSTEM (Weeks 5-7)
**Goal:** Complete gravity simulation and orbital mechanics

**C++ Files → Rust Modules:**
- `GravitySimulator.h/.cpp` → `src/physics/gravity_simulator.rs`

**Tasks:**
- [ ] Port GravitySimulator to gravity_simulator.rs
- [ ] Implement trajectory prediction system
- [ ] Implement orbital mechanics calculations (apoapsis/periapsis)
- [ ] Implement collision detection system
- [ ] Implement velocity and force vector calculations

**Key Physics Equations:**
```rust
// Gravitational force: F = G * m1 * m2 / r²
fn calculate_gravity_force(m1: f32, m2: f32, distance: f32) -> Vec2 {
    let force_magnitude = G * m1 * m2 / (distance * distance);
    // ... direction calculation
}

// Orbital velocity: v = sqrt(G * M / r)
fn calculate_orbital_velocity(planet_mass: f32, orbital_radius: f32) -> f32 {
    (G * planet_mass / orbital_radius).sqrt()
}
```

---

### PHASE 5: GAME SYSTEMS (Weeks 7-9)
**Goal:** Manager classes for game entity coordination

**C++ Files → Rust Modules:**
- `VehicleManager.h/.cpp` → `src/systems/vehicle_manager.rs`
- `SatelliteManager.h/.cpp` → `src/systems/satellite_manager.rs`
- `FuelTransferNetwork.h/.cpp` → `src/systems/fuel_transfer_network.rs`
- `OrbitMaintenance.h/.cpp` → `src/systems/orbit_maintenance.rs`

**Tasks:**
- [ ] Port VehicleManager (rocket spawning, player control)
- [ ] Port SatelliteManager (conversion, collection)
- [ ] Port FuelTransferNetwork (inter-planet fuel routing)
- [ ] Port OrbitMaintenance (station keeping)
- [ ] Implement entity ID system or consider ECS migration

**Architecture Decision:**
```rust
// Custom manager approach:
pub struct VehicleManager {
    rockets: HashMap<EntityId, Rocket>,
    active_rocket_id: Option<EntityId>,
}

// OR Bevy ECS approach:
fn vehicle_system(
    mut commands: Commands,
    rockets: Query<(Entity, &Rocket, &Transform)>,
) { ... }
```

---

### PHASE 6: UI COMPONENTS (Weeks 9-11)
**Goal:** User interface and HUD rendering

**C++ Files → Rust Modules:**
- `Button.h/.cpp` → `src/ui/button.rs`
- `TextPanel.h/.cpp` → `src/ui/text_panel.rs`
- `UIManager.h/.cpp` → `src/ui/ui_manager.rs`
- `GameInfoDisplay.h/.cpp` → `src/ui/game_info_display.rs`

**Tasks:**
- [ ] Port Button with click handling
- [ ] Port TextPanel for text rendering
- [ ] Port UIManager for HUD coordination
- [ ] Port GameInfoDisplay (speed, altitude, fuel, etc.)
- [ ] Implement camera system with zoom and pan
- [ ] Implement HUD rendering for rocket stats

**UI Layout:**
```rust
pub struct HUD {
    fuel_bar: ProgressBar,
    speed_text: Text,
    altitude_text: Text,
    apoapsis_text: Text,
    periapsis_text: Text,
}
```

---

### PHASE 7: MENU SYSTEMS (Weeks 11-12)
**Goal:** Navigation menus and game state transitions

**C++ Files → Rust Modules:**
- `MainMenu.h/.cpp` → `src/menus/main_menu.rs`
- `SavesMenu.h/.cpp` → `src/menus/saves_menu.rs`
- `MultiplayerMenu.h/.cpp` → `src/menus/multiplayer_menu.rs`
- `OnlineMultiplayerMenu.h/.cpp` → `src/menus/online_menu.rs`

**Tasks:**
- [ ] Port MainMenu (Single Player, Multiplayer, Exit)
- [ ] Port SavesMenu (New Game, Load Game, list saves)
- [ ] Port MultiplayerMenu (Local, Online, Split Screen)
- [ ] Port OnlineMultiplayerMenu (Host, Join, IP entry)
- [ ] Implement menu navigation and state transitions

**State Machine:**
```rust
enum GameState {
    MainMenu,
    SavesMenu,
    SinglePlayer,
    MultiplayerHost,
    MultiplayerClient,
    SplitScreen,
}
```

---

### PHASE 8: SAVE/LOAD SYSTEM (Weeks 12-13)
**Goal:** Persistent game state with serde

**C++ Files → Rust Modules:**
- `GameSaveData.h/.cpp` → `src/save_system/game_save_data.rs`

**Tasks:**
- [ ] Port GameSaveData structures
- [ ] Implement serde serialization for all game state
- [ ] Implement save file I/O with error handling
- [ ] Implement auto-save functionality
- [ ] Implement quick-save/quick-load features

**Serde Implementation:**
```rust
#[derive(Serialize, Deserialize)]
pub struct GameSaveData {
    pub game_time: f32,
    pub planets: Vec<SavedPlanetData>,
    pub rockets: Vec<SavedRocketData>,
    pub camera: SavedCameraData,
}

impl GameSaveData {
    pub fn save_to_file(&self, path: &str) -> Result<(), SaveError> {
        let json = serde_json::to_string_pretty(self)?;
        std::fs::write(path, json)?;
        Ok(())
    }
}
```

---

### PHASE 9: SINGLE PLAYER MODE (Weeks 13-15)
**Goal:** Complete single-player gameplay loop

**C++ Files → Rust Modules:**
- `SinglePlayerGame.h/.cpp` → `src/game_modes/single_player.rs`
- `Player.h/.cpp` → `src/player.rs`

**Tasks:**
- [ ] Port SinglePlayerGame main loop
- [ ] Port Player input handling
- [ ] Implement input handling system (keyboard/mouse)
- [ ] Implement game loop with delta time
- [ ] Integrate all systems (physics, UI, managers)
- [ ] Test and debug single player gameplay

**Game Loop Structure:**
```rust
pub fn run_single_player(window: &mut RenderWindow) {
    let mut game = SinglePlayerGame::new();
    let mut clock = Clock::start();

    loop {
        let delta_time = clock.restart().as_seconds();

        // Handle events
        while let Some(event) = window.poll_event() {
            game.handle_event(event);
        }

        // Update game state
        game.update(delta_time);

        // Render
        window.clear(Color::BLACK);
        game.render(window);
        window.display();
    }
}
```

---

### PHASE 10: NETWORKING FOUNDATION (Weeks 15-17) ⚠️
**Goal:** Async networking infrastructure

**C++ Files → Rust Modules:**
- `NetworkManager.h/.cpp` → `src/networking/network_manager.rs`

**Tasks:**
- [ ] Design async networking architecture with tokio
- [ ] Port NetworkManager to async Rust
- [ ] Implement message types with serde serialization
- [ ] Implement connection management (connect, disconnect, timeout)
- [ ] Implement player state synchronization protocol

**Tokio Networking:**
```rust
use tokio::net::{TcpListener, TcpStream};
use tokio::io::{AsyncReadExt, AsyncWriteExt};

pub struct NetworkManager {
    role: NetworkRole,
    connections: HashMap<PlayerId, TcpStream>,
}

#[derive(Serialize, Deserialize)]
pub enum NetworkMessage {
    PlayerState { id: u32, pos: Vec2, vel: Vec2, rotation: f32 },
    PlayerSpawn { id: u32 },
    PlayerDisconnect { id: u32 },
    SatelliteConversion { rocket_id: u32 },
}
```

**Key Challenge:** Converting synchronous SFML networking to async tokio.

---

### PHASE 11: MULTIPLAYER HOST (Weeks 17-18)
**Goal:** Authoritative server implementation

**C++ Files → Rust Modules:**
- `MultiplayerHost.h/.cpp` → `src/networking/multiplayer_host.rs`

**Tasks:**
- [ ] Port MultiplayerHost to async Rust
- [ ] Implement server socket listening and client acceptance
- [ ] Implement authoritative game state management
- [ ] Implement broadcast updates to all clients
- [ ] Handle client spawn and despawn events

**Host Architecture:**
```rust
pub struct MultiplayerHost {
    listener: TcpListener,
    game_state: GameState,
    clients: HashMap<u32, ClientConnection>,
}

impl MultiplayerHost {
    pub async fn run(&mut self) {
        loop {
            tokio::select! {
                // Accept new connections
                Ok((stream, addr)) = self.listener.accept() => {
                    self.handle_new_client(stream, addr).await;
                }
                // Update game state
                _ = tokio::time::sleep(Duration::from_millis(16)) => {
                    self.update_game_state();
                    self.broadcast_to_clients().await;
                }
            }
        }
    }
}
```

---

### PHASE 12: MULTIPLAYER CLIENT (Weeks 18-19)
**Goal:** Client-side prediction and interpolation

**C++ Files → Rust Modules:**
- `MultiplayerClient.h/.cpp` → `src/networking/multiplayer_client.rs`

**Tasks:**
- [ ] Port MultiplayerClient to async Rust
- [ ] Implement client connection to host
- [ ] Implement client-side prediction
- [ ] Implement state interpolation for smooth gameplay
- [ ] Handle network lag and packet loss

**Client Prediction:**
```rust
pub struct MultiplayerClient {
    connection: TcpStream,
    local_player_id: u32,
    predicted_state: PlayerState,
    server_state: PlayerState,
    interpolation_buffer: VecDeque<PlayerState>,
}

impl MultiplayerClient {
    // Predict local player movement immediately
    fn predict_local_movement(&mut self, input: &Input, dt: f32) {
        self.predicted_state.apply_input(input, dt);
    }

    // Reconcile with server updates
    fn reconcile_server_state(&mut self, server_state: PlayerState) {
        // Interpolate to server state
        self.predicted_state = lerp(self.predicted_state, server_state, 0.1);
    }
}
```

---

### PHASE 13: SPLIT SCREEN (Weeks 19-20)
**Goal:** Local multiplayer with viewports

**C++ Files → Rust Modules:**
- `SplitScreenManager.h/.cpp` → `src/game_modes/split_screen.rs`

**Tasks:**
- [ ] Port SplitScreenManager
- [ ] Implement multi-viewport rendering
- [ ] Implement per-player input handling
- [ ] Implement per-player camera management

**Split Screen Viewports:**
```rust
pub struct SplitScreenManager {
    player1_view: View,
    player2_view: View,
    viewport1: FloatRect,
    viewport2: FloatRect,
}

impl SplitScreenManager {
    pub fn render(&self, window: &mut RenderWindow, game_state: &GameState) {
        // Render player 1
        window.set_view(&self.player1_view);
        self.render_game_for_player(window, game_state, 0);

        // Render player 2
        window.set_view(&self.player2_view);
        self.render_game_for_player(window, game_state, 1);
    }
}
```

---

### PHASE 14: MAIN GAME LOOP (Week 20)
**Goal:** Complete game state machine

**C++ Files → Rust Modules:**
- `main.cpp` → `src/main.rs`

**Tasks:**
- [ ] Port main.cpp game loop
- [ ] Implement game state machine (menu → game modes)
- [ ] Implement window creation and event handling
- [ ] Implement frame rate limiting and timing

**Main Entry Point:**
```rust
fn main() {
    let mut window = RenderWindow::new(
        (1920, 1080),
        "KatieFlySimRust",
        Style::DEFAULT,
        &Default::default(),
    );
    window.set_framerate_limit(60);

    let mut game_state = GameState::MainMenu;
    let mut main_menu = MainMenu::new();
    let mut single_player: Option<SinglePlayerGame> = None;

    loop {
        match game_state {
            GameState::MainMenu => {
                if let Some(selection) = main_menu.update(&mut window) {
                    game_state = selection;
                }
            }
            GameState::SinglePlayer => {
                // ... run single player
            }
            // ... other states
        }
    }
}
```

---

### PHASE 15: TESTING & DEBUGGING (Weeks 20-21)
**Goal:** Comprehensive testing and optimization

**Tasks:**
- [ ] Test all game objects (rocket, planet, satellite)
- [ ] Test physics system (gravity, orbits, collisions)
- [ ] Test fuel transfer system
- [ ] Test save/load functionality
- [ ] Test single player mode end-to-end
- [ ] Test multiplayer host functionality
- [ ] Test multiplayer client functionality
- [ ] Test split screen mode
- [ ] Profile and optimize performance bottlenecks

**Testing Strategy:**
```rust
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_gravity_calculation() {
        let planet = Planet::new(10000.0, Vec2::new(0.0, 0.0));
        let rocket = Rocket::new(1.0, Vec2::new(100.0, 0.0));
        let force = calculate_gravity(&planet, &rocket);
        assert!(force.magnitude() > 0.0);
    }

    #[test]
    fn test_fuel_consumption() {
        let mut rocket = Rocket::new(1.0, Vec2::ZERO);
        rocket.set_fuel(100.0);
        rocket.apply_thrust(1.0);
        rocket.update(1.0);
        assert!(rocket.get_fuel() < 100.0);
    }
}
```

**Performance Profiling:**
```bash
cargo install cargo-flamegraph
cargo flamegraph --bin KatieFlySimRust
```

---

### PHASE 16: POLISH & RELEASE (Week 21+)
**Goal:** Production-ready release

**Tasks:**
- [ ] Fix any remaining bugs and edge cases
- [ ] Add error messages and user feedback
- [ ] Write comprehensive documentation
- [ ] Create build scripts for different platforms
- [ ] Test cross-platform compatibility (Windows, Linux, macOS)

**Build Configuration:**
```toml
# Cargo.toml
[profile.release]
opt-level = 3
lto = true
codegen-units = 1
```

**Cross-Platform Builds:**
```bash
# Windows
cargo build --release --target x86_64-pc-windows-gnu

# Linux
cargo build --release --target x86_64-unknown-linux-gnu

# macOS
cargo build --release --target x86_64-apple-darwin
```

---

## 🗺️ Module Structure (Final)

```
KatieFlySimRust/
├── Cargo.toml
├── Cargo.lock
├── README.md
├── RUST_PORT_PLAN.md (this file)
└── src/
    ├── main.rs                    # Entry point
    ├── lib.rs                     # Library root
    ├── game_constants.rs          # Global constants
    │
    ├── entities/
    │   ├── mod.rs
    │   ├── game_object.rs         # Trait or enum
    │   ├── planet.rs
    │   ├── rocket.rs
    │   ├── satellite.rs
    │   ├── rocket_part.rs
    │   └── engine.rs
    │
    ├── physics/
    │   ├── mod.rs
    │   ├── gravity_simulator.rs
    │   └── trajectory.rs
    │
    ├── systems/
    │   ├── mod.rs
    │   ├── vehicle_manager.rs
    │   ├── satellite_manager.rs
    │   ├── fuel_transfer_network.rs
    │   └── orbit_maintenance.rs
    │
    ├── ui/
    │   ├── mod.rs
    │   ├── button.rs
    │   ├── text_panel.rs
    │   ├── ui_manager.rs
    │   ├── game_info_display.rs
    │   └── camera.rs
    │
    ├── menus/
    │   ├── mod.rs
    │   ├── main_menu.rs
    │   ├── saves_menu.rs
    │   ├── multiplayer_menu.rs
    │   └── online_menu.rs
    │
    ├── game_modes/
    │   ├── mod.rs
    │   ├── single_player.rs
    │   └── split_screen.rs
    │
    ├── networking/
    │   ├── mod.rs
    │   ├── network_manager.rs
    │   ├── multiplayer_host.rs
    │   └── multiplayer_client.rs
    │
    ├── save_system/
    │   ├── mod.rs
    │   └── game_save_data.rs
    │
    ├── utils/
    │   ├── mod.rs
    │   ├── vector_helper.rs
    │   └── math.rs
    │
    └── player.rs
```

---

## 📊 Effort Estimation

| Phase | Duration | Difficulty | Risk Level |
|-------|----------|------------|------------|
| 1. Project Setup | 1 week | Easy | Low |
| 2. Core Infrastructure | 1 week | Easy | Low |
| 3. Base Game Objects | 3 weeks | **Hard** | **High** (ownership) |
| 4. Physics System | 2 weeks | Medium | Medium |
| 5. Game Systems | 2 weeks | Medium | Medium |
| 6. UI Components | 2 weeks | Medium | Low |
| 7. Menu Systems | 1 week | Easy | Low |
| 8. Save/Load System | 1 week | Medium | Low |
| 9. Single Player Mode | 2 weeks | Medium | Medium |
| 10. Networking Foundation | 2 weeks | **Hard** | **High** (async) |
| 11. Multiplayer Host | 1 week | **Hard** | **High** |
| 12. Multiplayer Client | 1 week | **Hard** | **High** |
| 13. Split Screen | 1 week | Medium | Low |
| 14. Main Game Loop | 1 week | Medium | Low |
| 15. Testing & Debugging | 1 week | Medium | Medium |
| 16. Polish & Release | 1+ weeks | Easy | Low |
| **TOTAL** | **21 weeks** | - | - |

---

## 🎯 Key Decision Points

### 1. Graphics Library Choice
| Option | Pros | Cons | Recommendation |
|--------|------|------|----------------|
| `sfml` crate | Familiar API, direct port | Unsafe bindings, older | ✅ **Best for learning** |
| `ggez` | Pure Rust, good docs | Different API | Consider for rewrite |
| `macroquad` | Very simple | Limited features | Too basic |
| `bevy` | Modern, performant | Complete rewrite needed | Best long-term |

**Decision:** Start with `sfml` crate for minimal friction.

---

### 2. Ownership Model
| Option | Pros | Cons | Recommendation |
|--------|------|------|----------------|
| Entity IDs + HashMap | Simple, flexible | Manual lookups | ✅ **Recommended** |
| `Rc<RefCell<>>` | Familiar to C++ | Runtime overhead, not idiomatic | Avoid |
| ECS (Bevy) | Most Rusty, performant | Architectural rewrite | Future consideration |

**Decision:** Use Entity IDs for simplicity.

---

### 3. Networking Library
| Option | Pros | Cons | Recommendation |
|--------|------|------|----------------|
| `tokio` + TCP | Industry standard, flexible | Learning curve | ✅ **Recommended** |
| `sfml` networking | Familiar | Blocking, limited | Avoid |
| `quinn` (QUIC) | Modern, fast | Overkill for this | Future |

**Decision:** Use `tokio` for async networking.

---

## 🔥 High-Risk Areas

### 1. **Ownership of Planet-Rocket References** 🚨
**Problem:** Rockets need to reference planets for fuel transfer and gravity.

**C++ Code:**
```cpp
class Rocket {
    std::vector<Planet*> nearbyPlanets; // Raw pointers
};
```

**Rust Solution:**
```rust
pub struct EntityId(usize);

pub struct World {
    planets: HashMap<EntityId, Planet>,
    rockets: HashMap<EntityId, Rocket>,
}

pub struct Rocket {
    nearby_planet_ids: Vec<EntityId>, // Store IDs, not references
}

impl Rocket {
    pub fn collect_fuel(&mut self, world: &mut World) {
        for planet_id in &self.nearby_planet_ids {
            if let Some(planet) = world.planets.get_mut(planet_id) {
                // Transfer fuel
            }
        }
    }
}
```

---

### 2. **Async Networking Migration** 🚨
**Problem:** SFML uses blocking I/O; Rust best practices use async.

**Solution:**
```rust
#[tokio::main]
async fn main() {
    // Run game loop in separate thread
    let game_thread = std::thread::spawn(|| {
        run_game_loop();
    });

    // Run networking on tokio runtime
    let networking_task = tokio::spawn(async {
        run_multiplayer_host().await;
    });

    // Coordinate via channels
    let (tx, rx) = tokio::sync::mpsc::channel(100);
}
```

---

### 3. **State Synchronization** 🚨
**Problem:** Keeping client and server state in sync.

**Solution:** Use explicit state snapshots with delta compression:
```rust
#[derive(Serialize, Deserialize)]
pub struct StateSnapshot {
    pub frame: u64,
    pub players: Vec<PlayerState>,
    pub projectiles: Vec<ProjectileState>,
}

// Client applies snapshots with interpolation
impl Client {
    pub fn apply_snapshot(&mut self, snapshot: StateSnapshot) {
        self.interpolation_buffer.push(snapshot);
        // Interpolate between last two snapshots
    }
}
```

---

## 📚 Learning Resources

### Rust Basics
- [The Rust Book](https://doc.rust-lang.org/book/)
- [Rust by Example](https://doc.rust-lang.org/rust-by-example/)

### Game Development
- [Rust SFML Tutorial](https://www.rust-sfml.org/tutorials.html)
- [Game Development in Rust](https://arewegameyet.rs/)

### Networking
- [Tokio Tutorial](https://tokio.rs/tokio/tutorial)
- [Fast-paced Multiplayer by Gabriel Gambetta](https://www.gabrielgambetta.com/client-server-game-architecture.html)

### ECS Pattern
- [Bevy Book](https://bevyengine.org/learn/book/introduction/)
- [Entity Component System FAQ](https://github.com/SanderMertens/ecs-faq)

---

## 🎮 Testing Milestones

### Milestone 1: Physics Demo (Week 7)
- Single rocket orbiting a planet
- Fuel consumption working
- Trajectory prediction visible

### Milestone 2: Single Player Alpha (Week 15)
- Full single player gameplay
- Save/load working
- All UI functional

### Milestone 3: Multiplayer Beta (Week 19)
- Host/client multiplayer working
- 2+ players can play together
- Basic synchronization

### Milestone 4: Release Candidate (Week 21)
- All features complete
- No critical bugs
- Cross-platform tested

---

## 💡 Pro Tips

1. **Start Small:** Get a single rocket rendering before building complex systems.
2. **Use Entity IDs:** Avoid `Rc<RefCell<>>` hell by using ID-based lookups.
3. **Test Early:** Write unit tests for physics calculations from day 1.
4. **Profile Often:** Use `cargo flamegraph` to find bottlenecks.
5. **Embrace Rust:** Don't fight the borrow checker—redesign to work with it.
6. **Commit Often:** Small, incremental commits make debugging easier.
7. **Ask for Help:** Rust community is friendly—use Discord, forums, etc.

---

## 📞 When to Pivot

Consider switching to **Bevy ECS** if:
- [ ] Ownership issues become unmanageable
- [ ] Performance is inadequate
- [ ] Want better Rust idioms
- [ ] Planning long-term maintenance

Bevy provides:
- Built-in entity management
- Parallel system execution
- Asset loading
- Plugin architecture

Trade-off: Requires architectural rewrite but better long-term.

---

## 🎬 Getting Started

**Next immediate steps:**
1. Run `cargo new --bin KatieFlySimRust` in this directory
2. Add SFML dependency to Cargo.toml
3. Port `GameConstants.rs` as first module
4. Create simple window with SFML
5. Render a single planet

**First code to write:**
```rust
// src/main.rs
use sfml::graphics::*;
use sfml::window::*;

fn main() {
    let mut window = RenderWindow::new(
        (1920, 1080),
        "KatieFlySimRust",
        Style::DEFAULT,
        &Default::default(),
    );
    window.set_framerate_limit(60);

    while window.is_open() {
        while let Some(event) = window.poll_event() {
            if event == Event::Closed {
                window.close();
            }
        }

        window.clear(Color::BLACK);
        // TODO: Render game
        window.display();
    }
}
```

---

**Good luck with the port! 🚀**
