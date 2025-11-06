# C++ to Rust File Mapping

This document maps each C++ file to its corresponding Rust module.

## Core Infrastructure

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `VectorHelper.h` | `src/utils/vector_helper.rs` | ✅ Complete | 2 |
| `GameConstants.h/.cpp` | `src/game_constants.rs` | ✅ Complete | 2 |

## Base Game Objects

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `GameObject.h/.cpp` | `src/entities/game_object.rs` | ✅ Complete | 3 |
| `RocketPart.h/.cpp` | `src/entities/rocket_part.rs` | ✅ Complete | 3 |
| `Engine.h/.cpp` | `src/entities/engine.rs` | ✅ Complete | 3 |
| `Planet.h/.cpp` | `src/entities/planet.rs` | ✅ Complete | 3 |
| `Rocket.h/.cpp` | `src/entities/rocket.rs` | ✅ Complete | 3 |
| `Satellite.h/.cpp` | `src/entities/satellite.rs` | ✅ Complete | 3 |

## Physics System

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `GravitySimulator.h/.cpp` | `src/physics/gravity_simulator.rs` | ✅ Complete | 4 |

## Game Systems

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `VehicleManager.h/.cpp` | `src/systems/world.rs` (redesigned) | ✅ Complete | 5 |
| `SatelliteManager.h/.cpp` | `src/systems/world.rs` (integrated) | ✅ Complete | 5 |
| `FuelTransferNetwork.h/.cpp` | ⏳ Deferred | 5 |
| `OrbitMaintenance.h/.cpp` | ⏳ Deferred | 5 |

## UI Components

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `Button.h/.cpp` | `src/ui/button.rs` | ✅ Complete | 6 |
| `TextPanel.h/.cpp` | ⏳ Deferred | 6 |
| `UIManager.h/.cpp` | `src/ui/camera.rs` (redesigned) | ✅ Complete | 6 |
| `GameInfoDisplay.h/.cpp` | `src/ui/hud.rs` (redesigned) | ✅ Complete | 6 |

## Menu Systems

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `MainMenu.h/.cpp` | `src/menus/main_menu.rs` | ⏳ Pending | 7 |
| `SavesMenu.h/.cpp` | `src/menus/saves_menu.rs` | ⏳ Pending | 7 |
| `MultiplayerMenu.h/.cpp` | `src/menus/multiplayer_menu.rs` | ⏳ Pending | 7 |
| `OnlineMultiplayerMenu.h/.cpp` | `src/menus/online_menu.rs` | ⏳ Pending | 7 |

## Save/Load System

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `GameSaveData.h/.cpp` | `src/save_system/game_save_data.rs` | ⏳ Pending | 8 |

## Game Modes

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `SinglePlayerGame.h/.cpp` | `src/game_modes/single_player.rs` | ⏳ Pending | 9 |
| `Player.h/.cpp` | `src/player.rs` | ⏳ Pending | 9 |
| `SplitScreenManager.h/.cpp` | `src/game_modes/split_screen.rs` | ⏳ Pending | 13 |

## Networking

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `NetworkManager.h/.cpp` | `src/networking/network_manager.rs` | ⏳ Pending | 10 |
| `MultiplayerHost.h/.cpp` | `src/networking/multiplayer_host.rs` | ⏳ Pending | 11 |
| `MultiplayerClient.h/.cpp` | `src/networking/multiplayer_client.rs` | ⏳ Pending | 12 |

## Main Entry Point

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `main.cpp` | `src/main.rs` | 🔄 In Progress | 14 |

---

## Legend

- ⏳ **Pending** - Not started
- 🔄 **In Progress** - Currently being worked on
- ✅ **Complete** - Ported and tested
- ❌ **Blocked** - Waiting on dependencies

---

## Summary Statistics

- **Total C++ Files:** 28 (56 with headers)
- **Total Rust Modules:** 28 (+ new Rust-idiomatic modules)
- **Completion:** 13/28 (46.4%)
- **Deferred:** 3 files (can be added later if needed)

**Current Phase:** Phase 3-6 Complete! → Phase 7-9 Next (Menus, Save/Load, Single Player)
