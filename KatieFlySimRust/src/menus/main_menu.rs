// Main Menu - Entry point menu
// Ported from C++ MainMenu class

use sfml::graphics::{Color, Font, RenderTarget, RenderWindow, Text, Transformable};
use sfml::system::Vector2f;
use sfml::window::mouse;

use crate::game_state::GameMode;
use crate::ui::Button;

/// Main menu with game mode selection
pub struct MainMenu<'a> {
    title: Text<'a>,
    single_player_button: Button<'a>,
    multiplayer_button: Button<'a>,
    quit_button: Button<'a>,
    selected_mode: GameMode,
}

impl<'a> MainMenu<'a> {
    pub fn new(window_size: Vector2f, font: &'a Font) -> Self {
        // Title
        let mut title = Text::new("KatieFlySimRust", font, 72);
        title.set_fill_color(Color::WHITE);
        let title_bounds = title.local_bounds();
        title.set_origin(Vector2f::new(title_bounds.width / 2.0, 0.0));
        title.set_position(Vector2f::new(window_size.x / 2.0, 100.0));

        // Button positioning
        let button_width = 300.0;
        let button_height = 60.0;
        let button_spacing = 80.0;
        let start_y = window_size.y / 2.0 - 50.0;

        // Single Player button
        let single_player_button = Button::new(
            Vector2f::new(
                window_size.x / 2.0 - button_width / 2.0,
                start_y,
            ),
            Vector2f::new(button_width, button_height),
            "Single Player",
            font,
            Color::rgb(50, 100, 150),
        );

        // Multiplayer button
        let multiplayer_button = Button::new(
            Vector2f::new(
                window_size.x / 2.0 - button_width / 2.0,
                start_y + button_spacing,
            ),
            Vector2f::new(button_width, button_height),
            "Multiplayer",
            font,
            Color::rgb(50, 120, 100),
        );

        // Quit button
        let quit_button = Button::new(
            Vector2f::new(
                window_size.x / 2.0 - button_width / 2.0,
                start_y + button_spacing * 2.0,
            ),
            Vector2f::new(button_width, button_height),
            "Quit",
            font,
            Color::rgb(120, 50, 50),
        );

        MainMenu {
            title,
            single_player_button,
            multiplayer_button,
            quit_button,
            selected_mode: GameMode::None,
        }
    }

    /// Update menu and handle input
    pub fn update(&mut self, window: &RenderWindow) -> GameMode {
        let mouse_pressed = mouse::Button::Left.is_pressed();

        // Update buttons
        if self.single_player_button.update(window, mouse_pressed) {
            self.selected_mode = GameMode::SinglePlayer;
            return GameMode::SinglePlayer;
        }

        if self.multiplayer_button.update(window, mouse_pressed) {
            self.selected_mode = GameMode::Multiplayer;
            return GameMode::Multiplayer;
        }

        if self.quit_button.update(window, mouse_pressed) {
            self.selected_mode = GameMode::Quit;
            return GameMode::Quit;
        }

        GameMode::None
    }

    /// Draw the menu
    pub fn draw(&self, window: &mut RenderWindow) {
        window.draw(&self.title);
        self.single_player_button.draw(window);
        self.multiplayer_button.draw(window);
        self.quit_button.draw(window);
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
