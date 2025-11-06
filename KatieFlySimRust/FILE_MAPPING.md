# C++ to Rust File Mapping

This document maps each C++ file to its corresponding Rust module.

## Core Infrastructure

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `VectorHelper.h` | `src/utils/vector_helper.rs` | ⏳ Pending | 2 |
| `GameConstants.h/.cpp` | `src/game_constants.rs` | ⏳ Pending | 2 |

## Base Game Objects

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `GameObject.h/.cpp` | `src/entities/game_object.rs` | ⏳ Pending | 3 |
| `RocketPart.h/.cpp` | `src/entities/rocket_part.rs` | ⏳ Pending | 3 |
| `Engine.h/.cpp` | `src/entities/engine.rs` | ⏳ Pending | 3 |
| `Planet.h/.cpp` | `src/entities/planet.rs` | ⏳ Pending | 3 |
| `Rocket.h/.cpp` | `src/entities/rocket.rs` | ⏳ Pending | 3 |
| `Satellite.h/.cpp` | `src/entities/satellite.rs` | ⏳ Pending | 3 |

## Physics System

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `GravitySimulator.h/.cpp` | `src/physics/gravity_simulator.rs` | ⏳ Pending | 4 |

## Game Systems

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `VehicleManager.h/.cpp` | `src/systems/vehicle_manager.rs` | ⏳ Pending | 5 |
| `SatelliteManager.h/.cpp` | `src/systems/satellite_manager.rs` | ⏳ Pending | 5 |
| `FuelTransferNetwork.h/.cpp` | `src/systems/fuel_transfer_network.rs` | ⏳ Pending | 5 |
| `OrbitMaintenance.h/.cpp` | `src/systems/orbit_maintenance.rs` | ⏳ Pending | 5 |

## UI Components

| C++ File | Rust Module | Status | Phase |
|----------|-------------|--------|-------|
| `Button.h/.cpp` | `src/ui/button.rs` | ⏳ Pending | 6 |
| `TextPanel.h/.cpp` | `src/ui/text_panel.rs` | ⏳ Pending | 6 |
| `UIManager.h/.cpp` | `src/ui/ui_manager.rs` | ⏳ Pending | 6 |
| `GameInfoDisplay.h/.cpp` | `src/ui/game_info_display.rs` | ⏳ Pending | 6 |

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
| `main.cpp` | `src/main.rs` | ⏳ Pending | 14 |

---

## Legend

- ⏳ **Pending** - Not started
- 🔄 **In Progress** - Currently being worked on
- ✅ **Complete** - Ported and tested
- ❌ **Blocked** - Waiting on dependencies

---

## Summary Statistics

- **Total C++ Files:** 28 (56 with headers)
- **Total Rust Modules:** 28
- **Completion:** 0/28 (0%)

**Current Phase:** 1 - Project Setup
