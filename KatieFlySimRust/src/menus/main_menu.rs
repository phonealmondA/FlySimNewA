// Main Menu - Entry point menu
// Ported from C++ MainMenu class

use macroquad::prelude::*;

use crate::game_state::GameMode;
use crate::ui::Button;

/// Main menu with game mode selection
pub struct MainMenu {
    title_text: String,
    title_position: Vec2,
    title_font_size: f32,
    single_player_button: Button,
    multiplayer_button: Button,
    quit_button: Button,
    selected_mode: GameMode,
}

impl MainMenu {
    pub fn new(window_size: Vec2) -> Self {
        // Title
        let title_text = "KatieFlySimRust".to_string();
        let title_font_size = 72.0;

        // Calculate title position (centered)
        let text_dims = measure_text(&title_text, None, title_font_size as u16, 1.0);
        let title_position = Vec2::new(
            window_size.x / 2.0 - text_dims.width / 2.0,
            100.0 + text_dims.height,
        );

        // Button positioning
        let button_width = 300.0;
        let button_height = 60.0;
        let button_spacing = 80.0;
        let start_y = window_size.y / 2.0 - 50.0;

        // Single Player button
        let single_player_button = Button::new(
            Vec2::new(
                window_size.x / 2.0 - button_width / 2.0,
                start_y,
            ),
            Vec2::new(button_width, button_height),
            "Single Player",
            Color::from_rgba(50, 100, 150, 255),
        );

        // Multiplayer button
        let multiplayer_button = Button::new(
            Vec2::new(
                window_size.x / 2.0 - button_width / 2.0,
                start_y + button_spacing,
            ),
            Vec2::new(button_width, button_height),
            "Multiplayer",
            Color::from_rgba(50, 120, 100, 255),
        );

        // Quit button
        let quit_button = Button::new(
            Vec2::new(
                window_size.x / 2.0 - button_width / 2.0,
                start_y + button_spacing * 2.0,
            ),
            Vec2::new(button_width, button_height),
            "Quit",
            Color::from_rgba(120, 50, 50, 255),
        );

        MainMenu {
            title_text,
            title_position,
            title_font_size,
            single_player_button,
            multiplayer_button,
            quit_button,
            selected_mode: GameMode::None,
        }
    }

    /// Update menu and handle input
    pub fn update(&mut self) -> GameMode {
        let mouse_pressed = is_mouse_button_down(MouseButton::Left);

        // Update buttons
        if self.single_player_button.update(mouse_pressed) {
            self.selected_mode = GameMode::SinglePlayer;
            return GameMode::SinglePlayer;
        }

        if self.multiplayer_button.update(mouse_pressed) {
            self.selected_mode = GameMode::Multiplayer;
            return GameMode::Multiplayer;
        }

        if self.quit_button.update(mouse_pressed) {
            self.selected_mode = GameMode::Quit;
            return GameMode::Quit;
        }

        GameMode::None
    }

    /// Draw the menu
    pub fn draw(&self) {
        // Draw title
        draw_text(
            &self.title_text,
            self.title_position.x,
            self.title_position.y,
            self.title_font_size,
            WHITE,
        );

        // Draw buttons
        self.single_player_button.draw();
        self.multiplayer_button.draw();
        self.quit_button.draw();
    }

    /// Get the selected mode
    pub fn selected_mode(&self) -> GameMode {
        self.selected_mode
    }

    /// Reset selection
    pub fn reset(&mut self) {
        self.selected_mode = GameMode::None;
    }
}
