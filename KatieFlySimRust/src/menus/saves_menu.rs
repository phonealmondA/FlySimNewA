// Saves Menu - New game and load game selection
// Ported from C++ SavesMenu class

use macroquad::prelude::*;

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
pub struct SavesMenu {
    title_text: String,
    title_position: Vec2,
    title_font_size: f32,
    new_game_button: Button,
    back_button: Button,
    save_buttons: Vec<Button>,
    save_names: Vec<String>,
    window_size: Vec2,
}

impl SavesMenu {
    pub fn new(window_size: Vec2) -> Self {
        // Title
        let title_text = "Select Save".to_string();
        let title_font_size = 48.0;

        // Calculate title position (centered)
        let text_dims = measure_text(&title_text, None, title_font_size as u16, 1.0);
        let title_position = Vec2::new(
            window_size.x / 2.0 - text_dims.width / 2.0,
            80.0 + text_dims.height,
        );

        // Button dimensions
        let button_width = 350.0;
        let button_height = 50.0;
        let start_y = 180.0;

        // New Game button
        let new_game_button = Button::new(
            Vec2::new(
                window_size.x / 2.0 - button_width / 2.0,
                start_y,
            ),
            Vec2::new(button_width, button_height),
            "New Game",
            Color::from_rgba(50, 150, 50, 255),
        );

        // Back button
        let back_button = Button::new(
            Vec2::new(50.0, window_size.y - 80.0),
            Vec2::new(150.0, 50.0),
            "Back",
            Color::from_rgba(100, 100, 100, 255),
        );

        SavesMenu {
            title_text,
            title_position,
            title_font_size,
            new_game_button,
            back_button,
            save_buttons: Vec::new(),
            save_names: Vec::new(),
            window_size,
        }
    }

    /// Load available save files and create buttons
    pub fn refresh_saves(&mut self) {
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
                    Vec2::new(
                        self.window_size.x / 2.0 - button_width / 2.0,
                        start_y + (i as f32 * button_spacing),
                    ),
                    Vec2::new(button_width, button_height),
                    save_name,
                    Color::from_rgba(70, 90, 120, 255),
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
    pub fn update(&mut self) -> SavesMenuResult {
        let mouse_pressed = is_mouse_button_down(MouseButton::Left);

        // Check new game button
        if self.new_game_button.update(mouse_pressed) {
            return SavesMenuResult::NewGame;
        }

        // Check back button
        if self.back_button.update(mouse_pressed) {
            return SavesMenuResult::Back;
        }

        // Check save file buttons
        for (i, button) in self.save_buttons.iter_mut().enumerate() {
            if button.update(mouse_pressed) {
                if let Some(save_name) = self.save_names.get(i) {
                    return SavesMenuResult::LoadGame(save_name.clone());
                }
            }
        }

        SavesMenuResult::None
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
        self.new_game_button.draw();

        for button in &self.save_buttons {
            button.draw();
        }

        self.back_button.draw();
    }
}
