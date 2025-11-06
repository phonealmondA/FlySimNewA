// Game State - Top-level game state machine
// Manages transitions between menus and game modes

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GameState {
    MainMenu,
    SavesMenu,
    Playing,
    Paused,
    Quit,
}

impl Default for GameState {
    fn default() -> Self {
        GameState::MainMenu
    }
}

/// Game mode selection from main menu
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GameMode {
    None,
    SinglePlayer,
    Multiplayer,
    Quit,
}
