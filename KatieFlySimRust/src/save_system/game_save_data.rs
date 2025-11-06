// Game Save Data - Serializable game state
// Ported from C++ GameSaveData with serde

use serde::{Deserialize, Serialize};
use macroquad::prelude::*;
use std::fs;
use std::path::Path;

use crate::entities::{Planet, Rocket, Satellite};
use crate::systems::EntityId;

/// Serializable Vec2 wrapper
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SavedVector2 {
    pub x: f32,
    pub y: f32,
}

impl From<Vec2> for SavedVector2 {
    fn from(v: Vec2) -> Self {
        SavedVector2 { x: v.x, y: v.y }
    }
}

impl From<SavedVector2> for Vec2 {
    fn from(v: SavedVector2) -> Self {
        Vec2::new(v.x, v.y)
    }
}

/// Saved planet data
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SavedPlanet {
    pub id: EntityId,
    pub position: SavedVector2,
    pub velocity: SavedVector2,
    pub mass: f32,
    pub radius: f32,
    pub color: (u8, u8, u8), // RGB
}

impl SavedPlanet {
    pub fn from_planet(id: EntityId, planet: &Planet) -> Self {
        use crate::entities::GameObject;

        SavedPlanet {
            id,
            position: planet.position().into(),
            velocity: planet.velocity().into(),
            mass: planet.mass(),
            radius: planet.radius(),
            color: (
                (planet.color().r * 255.0) as u8,
                (planet.color().g * 255.0) as u8,
                (planet.color().b * 255.0) as u8,
            ),
        }
    }

    pub fn to_planet(&self) -> (EntityId, Planet) {
        let planet = Planet::new(
            self.position.clone().into(),
            self.radius,
            self.mass,
            Color::from_rgba(self.color.0, self.color.1, self.color.2, 255),
        );

        (self.id, planet)
    }
}

/// Saved rocket data
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SavedRocket {
    pub id: EntityId,
    pub position: SavedVector2,
    pub velocity: SavedVector2,
    pub rotation: f32,
    pub fuel: f32,
    pub color: (u8, u8, u8),
}

impl SavedRocket {
    pub fn from_rocket(id: EntityId, rocket: &Rocket) -> Self {
        use crate::entities::GameObject;

        SavedRocket {
            id,
            position: rocket.position().into(),
            velocity: rocket.velocity().into(),
            rotation: rocket.rotation(),
            fuel: rocket.current_fuel(),
            color: (
                (rocket.color().r * 255.0) as u8,
                (rocket.color().g * 255.0) as u8,
                (rocket.color().b * 255.0) as u8,
            ),
        }
    }

    pub fn to_rocket(&self) -> (EntityId, Rocket) {
        use crate::game_constants::GameConstants;

        let mut rocket = Rocket::new(
            self.position.clone().into(),
            self.velocity.clone().into(),
            Color::from_rgba(self.color.0, self.color.1, self.color.2, 255),
            GameConstants::ROCKET_BASE_MASS,
        );

        rocket.set_fuel(self.fuel);
        // Note: rotation will need to be set via a method if we add one

        (self.id, rocket)
    }
}

/// Saved satellite data
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SavedSatellite {
    pub id: EntityId,
    pub position: SavedVector2,
    pub velocity: SavedVector2,
    pub fuel: f32,
}

impl SavedSatellite {
    pub fn from_satellite(id: EntityId, satellite: &Satellite) -> Self {
        use crate::entities::GameObject;

        SavedSatellite {
            id,
            position: satellite.position().into(),
            velocity: satellite.velocity().into(),
            fuel: satellite.current_fuel(),
        }
    }

    pub fn to_satellite(&self) -> (EntityId, Satellite) {
        let mut satellite = Satellite::new(
            self.position.clone().into(),
            self.velocity.clone().into(),
            Color::new(0.0, 1.0, 1.0, 1.0), // CYAN
        );

        satellite.add_fuel(self.fuel - satellite.current_fuel());

        (self.id, satellite)
    }
}

/// Camera save data
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SavedCamera {
    pub center: SavedVector2,
    pub zoom: f32,
}

/// Complete game save data
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GameSaveData {
    pub version: u32,
    pub game_time: f32,
    pub planets: Vec<SavedPlanet>,
    pub rockets: Vec<SavedRocket>,
    pub satellites: Vec<SavedSatellite>,
    pub active_rocket_id: Option<EntityId>,
    pub camera: SavedCamera,
    pub save_timestamp: String,
}

impl GameSaveData {
    pub fn new() -> Self {
        GameSaveData {
            version: 1,
            game_time: 0.0,
            planets: Vec::new(),
            rockets: Vec::new(),
            satellites: Vec::new(),
            active_rocket_id: None,
            camera: SavedCamera {
                center: SavedVector2 { x: 0.0, y: 0.0 },
                zoom: 1.0,
            },
            save_timestamp: format!("{}", std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap_or_default()
                .as_secs()),
        }
    }

    /// Save to JSON file
    pub fn save_to_file(&self, save_name: &str) -> Result<(), Box<dyn std::error::Error>> {
        // Create saves directory if it doesn't exist
        fs::create_dir_all("saves")?;

        let file_path = format!("saves/{}.json", save_name);
        let json = serde_json::to_string_pretty(self)?;
        fs::write(&file_path, json)?;

        log::info!("Game saved to: {}", file_path);
        Ok(())
    }

    /// Load from JSON file
    pub fn load_from_file(save_name: &str) -> Result<Self, Box<dyn std::error::Error>> {
        let file_path = format!("saves/{}.json", save_name);

        if !Path::new(&file_path).exists() {
            return Err(format!("Save file not found: {}", file_path).into());
        }

        let json = fs::read_to_string(&file_path)?;
        let save_data: GameSaveData = serde_json::from_str(&json)?;

        log::info!("Game loaded from: {}", file_path);
        Ok(save_data)
    }

    /// Delete a save file
    pub fn delete_save(save_name: &str) -> Result<(), Box<dyn std::error::Error>> {
        let file_path = format!("saves/{}.json", save_name);
        fs::remove_file(&file_path)?;
        log::info!("Deleted save: {}", file_path);
        Ok(())
    }

    /// Check if a save exists
    pub fn save_exists(save_name: &str) -> bool {
        let file_path = format!("saves/{}.json", save_name);
        Path::new(&file_path).exists()
    }
}

impl Default for GameSaveData {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_vector2_conversion() {
        let v = Vec2::new(100.0, 200.0);
        let saved: SavedVector2 = v.into();
        assert_eq!(saved.x, 100.0);
        assert_eq!(saved.y, 200.0);

        let restored: Vec2 = saved.into();
        assert_eq!(restored.x, 100.0);
        assert_eq!(restored.y, 200.0);
    }

    #[test]
    fn test_save_data_creation() {
        let save_data = GameSaveData::new();
        assert_eq!(save_data.version, 1);
        assert_eq!(save_data.game_time, 0.0);
        assert!(save_data.planets.is_empty());
    }

    #[test]
    fn test_serialization() {
        let save_data = GameSaveData::new();
        let json = serde_json::to_string(&save_data);
        assert!(json.is_ok());

        let deserialized: Result<GameSaveData, _> = serde_json::from_str(&json.unwrap());
        assert!(deserialized.is_ok());
    }
}
