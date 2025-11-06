// Saves Menu - New game and load game selection
// Ported from C++ SavesMenu class

use sfml::graphics::{Color, Font, RenderTarget, RenderWindow, Text, Transformable};
use sfml::system::Vector2f;
use sfml::window::mouse;

use crate::ui::Button;

/// Result from saves menu interaction
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SavesMenuResult {
    None,
    NewGame,
    LoadGame(String),
    Back,
}

/// Saves menu for creating new games or loading existing ones
pub struct SavesMenu<'a> {
    title: Text<'a>,
    new_game_button: Button<'a>,
    back_button: Button<'a>,
    save_buttons: Vec<Button<'a>>,
    save_names: Vec<String>,
}

impl<'a> SavesMenu<'a> {
    pub fn new(window_size: Vector2f, font: &'a Font) -> Self {
        // Title
        let mut title = Text::new("Select Save", font, 48);
        title.set_fill_color(Color::WHITE);
        let title_bounds = title.local_bounds();
        title.set_origin(Vector2f::new(title_bounds.width / 2.0, 0.0));
        title.set_position(Vector2f::new(window_size.x / 2.0, 80.0));

        // Button dimensions
        let button_width = 350.0;
        let button_height = 50.0;
        let button_spacing = 60.0;
        let start_y = 180.0;

        // New Game button
        let new_game_button = Button::new(
            Vector2f::new(
                window_size.x / 2.0 - button_width / 2.0,
                start_y,
            ),
            Vector2f::new(button_width, button_height),
            "New Game",
            font,
            Color::rgb(50, 150, 50),
        );

        // Back button
        let back_button = Button::new(
            Vector2f::new(50.0, window_size.y - 80.0),
            Vector2f::new(150.0, 50.0),
            "Back",
            font,
            Color::rgb(100, 100, 100),
        );

        SavesMenu {
            title,
            new_game_button,
            back_button,
            save_buttons: Vec::new(),
            save_names: Vec::new(),
        }
    }

    /// Load available save files and create buttons
    pub fn refresh_saves(&mut self, font: &'a Font, window_size: Vector2f) {
        // Clear existing save buttons
        self.save_buttons.clear();
        self.save_names.clear();

        // Get save files from disk
        if let Ok(saves) = self.load_save_list() {
            let button_width = 350.0;
            let button_height = 50.0;
            let button_spacing = 60.0;
            let start_y = 260.0; // Below "New Game" button

            for (i, save_name) in saves.iter().enumerate() {
                let button = Button::new(
                    Vector2f::new(
                        window_size.x / 2.0 - button_width / 2.0,
                        start_y + (i as f32 * button_spacing),
                    ),
                    Vector2f::new(button_width, button_height),
                    save_name,
                    font,
                    Color::rgb(70, 90, 120),
                );

                self.save_buttons.push(button);
                self.save_names.push(save_name.clone());
            }
        }
    }

    /// Load list of save files from disk
    fn load_save_list(&self) -> Result<Vec<String>, std::io::Error> {
        use std::fs;

        let saves_dir = "saves";

        // Create saves directory if it doesn't exist
        fs::create_dir_all(saves_dir)?;

        let mut saves = Vec::new();

        // Read all .json files in saves directory
        for entry in fs::read_dir(saves_dir)? {
            let entry = entry?;
            let path = entry.path();

            if path.extension().and_then(|s| s.to_str()) == Some("json") {
                if let Some(file_name) = path.file_stem().and_then(|s| s.to_str()) {
                    saves.push(file_name.to_string());
                }
            }
        }

        saves.sort();
        Ok(saves)
    }

    /// Update menu and handle input
    pub fn update(&mut self, window: &RenderWindow) -> SavesMenuResult {
        let mouse_pressed = mouse::Button::Left.is_pressed();

        // Check new game button
        if self.new_game_button.update(window, mouse_pressed) {
            return SavesMenuResult::NewGame;
        }

        // Check back button
        if self.back_button.update(window, mouse_pressed) {
            return SavesMenuResult::Back;
        }

        // Check save file buttons
        for (i, button) in self.save_buttons.iter_mut().enumerate() {
            if button.update(window, mouse_pressed) {
                if let Some(save_name) = self.save_names.get(i) {
                    return SavesMenuResult::LoadGame(save_name.clone());
                }
            }
        }

        SavesMenuResult::None
    }

    /// Draw the menu
    pub fn draw(&self, window: &mut RenderWindow) {
        window.draw(&self.title);
        self.new_game_button.draw(window);

        for button in &self.save_buttons {
            button.draw(window);
        }

        self.back_button.draw(window);
    }
}
