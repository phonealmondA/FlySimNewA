// KatieFlySimRust - Library Root
// Rust port of FlySimNewA space flight simulator

// Game constants and configuration
pub mod game_constants;

// Game state management
pub mod game_state;

// Utility modules
pub mod utils;

// Core game entities
pub mod entities;

// Physics simulation
pub mod physics;

// Game systems (managers)
pub mod systems;

// User interface
pub mod ui;

// Menu screens
pub mod menus;

// Game modes
pub mod game_modes;

// Networking (multiplayer)
pub mod networking;

// Save/load system
pub mod save_system;

// Player control
pub mod player;

// Re-export commonly used types
pub use game_constants::GameConstants;
