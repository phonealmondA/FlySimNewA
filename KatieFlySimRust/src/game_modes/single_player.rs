// Single Player Game - Main single player game mode
// Integrates all systems for playable game

use sfml::graphics::{Color, Font, RenderTarget, RenderWindow};
use sfml::system::Vector2f;
use sfml::window::{Event, Key};

use crate::entities::{GameObject, Planet, Rocket};
use crate::game_constants::GameConstants;
use crate::save_system::{GameSaveData, SavedCamera, SavedPlanet, SavedRocket, SavedSatellite};
use crate::systems::{EntityId, World};
use crate::ui::{Camera, Hud};

/// Single player game result
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SinglePlayerResult {
    Continue,
    ReturnToMenu,
    Quit,
}

/// Single player game mode
pub struct SinglePlayerGame<'a> {
    world: World,
    camera: Camera,
    hud: Hud<'a>,
    game_time: f32,
    is_paused: bool,

    // Input state
    thrust_input: f32,
    rotation_input: f32,

    // Save/load
    current_save_name: Option<String>,
    last_auto_save: f32,
    auto_save_interval: f32,
}

impl<'a> SinglePlayerGame<'a> {
    pub fn new(window_size: Vector2f, font: &'a Font) -> Self {
        SinglePlayerGame {
            world: World::new(),
            camera: Camera::new(window_size),
            hud: Hud::new(font, Vector2f::new(10.0, 10.0)),
            game_time: 0.0,
            is_paused: false,
            thrust_input: 0.0,
            rotation_input: 0.0,
            current_save_name: None,
            last_auto_save: 0.0,
            auto_save_interval: 60.0, // Auto-save every 60 seconds
        }
    }

    /// Initialize a new game with default setup
    pub fn initialize_new_game(&mut self) {
        self.world.clear_all();
        self.game_time = 0.0;

        // Create main planet (like Earth)
        let main_planet = Planet::new(
            Vector2f::new(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
            GameConstants::MAIN_PLANET_RADIUS,
            GameConstants::MAIN_PLANET_MASS,
            Color::BLUE,
        );
        self.world.add_planet(main_planet);

        // Create secondary planet (like Moon)
        let mut secondary_planet = Planet::new(
            Vector2f::new(*crate::game_constants::SECONDARY_PLANET_X, *crate::game_constants::SECONDARY_PLANET_Y),
            GameConstants::SECONDARY_PLANET_RADIUS,
            GameConstants::SECONDARY_PLANET_MASS,
            Color::rgb(150, 150, 150),
        );

        // Set orbital velocity for secondary planet
        secondary_planet.set_velocity(Vector2f::new(
            0.0,
            -*crate::game_constants::SECONDARY_PLANET_ORBITAL_VELOCITY,
        ));

        self.world.add_planet(secondary_planet);

        // Create starting rocket near main planet
        let rocket_spawn_distance = GameConstants::MAIN_PLANET_RADIUS + 200.0;
        let rocket = Rocket::new(
            Vector2f::new(
                GameConstants::MAIN_PLANET_X + rocket_spawn_distance,
                GameConstants::MAIN_PLANET_Y,
            ),
            Vector2f::new(0.0, 0.0),
            Color::WHITE,
            GameConstants::ROCKET_BASE_MASS,
        );

        let rocket_id = self.world.add_rocket(rocket);

        // Set camera to follow rocket
        if let Some(rocket) = self.world.get_rocket(rocket_id) {
            self.camera.set_center(rocket.position());
        }

        log::info!("New game initialized");
    }

    /// Load game from save data
    pub fn load_from_save(&mut self, save_data: GameSaveData, save_name: String) {
        self.world.clear_all();
        self.game_time = save_data.game_time;

        // Load planets
        for saved_planet in save_data.planets {
            let (_id, planet) = saved_planet.to_planet();
            self.world.add_planet(planet);
        }

        // Load rockets
        for saved_rocket in save_data.rockets {
            let (_id, rocket) = saved_rocket.to_rocket();
            self.world.add_rocket(rocket);
        }

        // Load satellites
        for saved_satellite in save_data.satellites {
            let (_id, satellite) = saved_satellite.to_satellite();
            self.world.add_satellite(satellite);
        }

        // Set active rocket
        self.world.set_active_rocket(save_data.active_rocket_id);

        // Restore camera
        self.camera.set_center(save_data.camera.center.into());
        self.camera.set_target_zoom(save_data.camera.zoom);

        self.current_save_name = Some(save_name.clone());

        log::info!("Game loaded from save: {}", save_name);
    }

    /// Save current game state
    pub fn save_game(&self, save_name: &str) -> Result<(), Box<dyn std::error::Error>> {
        let save_data = self.create_save_data();
        save_data.save_to_file(save_name)?;
        log::info!("Game saved: {}", save_name);
        Ok(())
    }

    /// Create save data from current game state
    fn create_save_data(&self) -> GameSaveData {
        let mut save_data = GameSaveData::new();
        save_data.game_time = self.game_time;

        // Save planets (we need to iterate with IDs - simplified for now)
        // In a real implementation, World would provide an iterator with IDs

        // Save camera
        save_data.camera = SavedCamera {
            center: self.camera.view().center().into(),
            zoom: self.camera.zoom_level(),
        };

        save_data.active_rocket_id = self.world.active_rocket_id();

        save_data
    }

    /// Handle input events
    pub fn handle_event(&mut self, event: &Event) -> SinglePlayerResult {
        match event {
            Event::Closed => SinglePlayerResult::Quit,

            Event::KeyPressed { code, .. } => match code {
                Key::Escape => SinglePlayerResult::ReturnToMenu,
                Key::P => {
                    self.is_paused = !self.is_paused;
                    SinglePlayerResult::Continue
                }
                Key::F5 => {
                    // Quick save
                    if let Err(e) = self.save_game("quicksave") {
                        log::error!("Failed to quick save: {}", e);
                    }
                    SinglePlayerResult::Continue
                }
                _ => SinglePlayerResult::Continue,
            },

            Event::MouseWheelScrolled { delta, .. } => {
                self.camera.adjust_zoom(-delta * 0.1);
                SinglePlayerResult::Continue
            }

            _ => SinglePlayerResult::Continue,
        }
    }

    /// Update game state
    pub fn update(&mut self, delta_time: f32) {
        if self.is_paused {
            return;
        }

        self.game_time += delta_time;

        // Handle input for active rocket
        self.update_rocket_input();

        // Update world (physics, entities)
        self.world.update(delta_time);

        // Update camera to follow active rocket
        if let Some(rocket) = self.world.get_active_rocket() {
            self.camera.follow(rocket.position());
        }

        self.camera.update(delta_time);

        // Auto-save
        if self.game_time - self.last_auto_save > self.auto_save_interval {
            if let Err(e) = self.save_game("autosave") {
                log::error!("Auto-save failed: {}", e);
            }
            self.last_auto_save = self.game_time;
        }
    }

    /// Update rocket based on keyboard input
    fn update_rocket_input(&mut self) {
        // Get input state
        let mut thrust_level = 0.0;
        let mut rotation_delta = 0.0;

        // Thrust controls
        if Key::Space.is_pressed() {
            thrust_level = 1.0;
        }

        // Rotation controls
        if Key::Left.is_pressed() || Key::A.is_pressed() {
            rotation_delta = -3.0; // degrees per frame
        }
        if Key::Right.is_pressed() || Key::D.is_pressed() {
            rotation_delta = 3.0;
        }

        // Convert degrees to radians
        let rotation_radians = rotation_delta * std::f32::consts::PI / 180.0;

        // Apply to active rocket
        if let Some(rocket) = self.world.get_active_rocket_mut() {
            rocket.set_thrust_level(thrust_level);
            if rotation_delta != 0.0 {
                rocket.rotate(rotation_radians);
            }
        }

        // Launch new rocket (L key)
        if Key::L.is_pressed() {
            self.launch_new_rocket();
        }

        // Convert to satellite (T key)
        if Key::T.is_pressed() {
            if let Some(rocket_id) = self.world.active_rocket_id() {
                if self.world.convert_rocket_to_satellite(rocket_id).is_some() {
                    log::info!("Rocket converted to satellite");
                }
            }
        }
    }

    /// Launch a new rocket from the active rocket's position
    fn launch_new_rocket(&mut self) {
        if let Some(current_rocket) = self.world.get_active_rocket() {
            let new_rocket = Rocket::new(
                current_rocket.position(),
                current_rocket.velocity(),
                Color::rgb(200, 200, 255),
                GameConstants::ROCKET_BASE_MASS,
            );

            let new_id = self.world.add_rocket(new_rocket);
            self.world.set_active_rocket(Some(new_id));

            log::info!("New rocket launched");
        }
    }

    /// Render the game
    pub fn render(&self, window: &mut RenderWindow) {
        // Set camera view
        window.set_view(self.camera.view());

        // Render world
        self.world.render(window);

        // Reset to UI view for HUD
        window.set_view(&window.default_view());

        // Render HUD
        if let Some(rocket) = self.world.get_active_rocket() {
            self.hud.draw_rocket_stats(window, rocket);
        } else {
            self.hud.draw_message(window, "No active rocket");
        }

        // Draw pause indicator if paused
        if self.is_paused {
            use sfml::graphics::{Text, Transformable};
            // Note: Would need font access here - simplified
        }
    }

    pub fn is_paused(&self) -> bool {
        self.is_paused
    }

    pub fn game_time(&self) -> f32 {
        self.game_time
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // Note: Tests requiring SFML resources (Font) are limited
    // Would need mock or test fixtures

    #[test]
    fn test_game_time_tracking() {
        // This test would require proper setup with Font
        // Simplified test structure shown
    }
}
