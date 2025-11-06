// Planet - Celestial body with gravity and fuel storage
// Ported from C++ Planet class - now using macroquad for pure Rust graphics

use macroquad::prelude::*;

use super::game_object::{GameObject, GameObjectData};
use crate::game_constants::GameConstants;

/// Planet entity with mass, gravity, and fuel storage
pub struct Planet {
    data: GameObjectData,
    mass: f32,
    radius: f32,
}

impl Planet {
    pub fn new(position: Vec2, radius: f32, mass: f32, color: Color) -> Self {
        Planet {
            data: GameObjectData::new(position, Vec2::new(0.0, 0.0), color),
            mass,
            radius,
        }
    }

    pub fn mass(&self) -> f32 {
        self.mass
    }

    pub fn radius(&self) -> f32 {
        self.radius
    }

    pub fn set_mass(&mut self, new_mass: f32) {
        self.mass = new_mass;
        self.update_radius_from_mass();
    }

    /// Update planet radius based on mass using game constants
    pub fn update_radius_from_mass(&mut self) {
        // Use cube root for volume-based scaling
        let mass_ratio = self.mass / GameConstants::REFERENCE_MASS;
        self.radius = GameConstants::BASE_RADIUS_FACTOR * mass_ratio.powf(1.0 / 3.0);
    }

    /// Check if planet has enough mass for fuel collection
    pub fn can_collect_fuel(&self) -> bool {
        self.mass >= GameConstants::MIN_PLANET_MASS_FOR_COLLECTION
    }

    /// Get fuel collection range for this planet
    pub fn fuel_collection_range(&self) -> f32 {
        self.radius + GameConstants::FUEL_COLLECTION_RANGE
    }

    /// Draw fuel collection ring around planet
    pub fn draw_fuel_collection_ring(&self, is_actively_collecting: bool) {
        if !self.can_collect_fuel() {
            return;
        }

        let collection_radius = self.fuel_collection_range();

        let color = if is_actively_collecting {
            crate::game_constants::colors::FUEL_RING_ACTIVE_COLOR
        } else {
            crate::game_constants::colors::FUEL_RING_COLOR
        };

        // Draw collection ring using macroquad
        draw_circle_lines(
            self.data.position.x,
            self.data.position.y,
            collection_radius,
            GameConstants::FUEL_RING_THICKNESS,
            color,
        );
    }
}

impl GameObject for Planet {
    fn update(&mut self, delta_time: f32) {
        // Update position based on velocity
        self.data.position += self.data.velocity * delta_time;
    }

    fn draw(&self) {
        // Draw the planet as a filled circle
        draw_circle(
            self.data.position.x,
            self.data.position.y,
            self.radius,
            self.data.color,
        );
    }

    fn position(&self) -> Vec2 {
        self.data.position
    }

    fn velocity(&self) -> Vec2 {
        self.data.velocity
    }

    fn set_velocity(&mut self, velocity: Vec2) {
        self.data.velocity = velocity;
    }

    fn color(&self) -> Color {
        self.data.color
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_planet_creation() {
        let planet = Planet::new(
            Vec2::new(100.0, 100.0),
            20.0,
            1000.0,
            BLUE,
        );
        assert_eq!(planet.mass(), 1000.0);
        assert_eq!(planet.radius(), 20.0);
        assert_eq!(planet.position(), Vec2::new(100.0, 100.0));
    }

    #[test]
    fn test_planet_mass_update() {
        let mut planet = Planet::new(
            Vec2::new(0.0, 0.0),
            20.0,
            1000.0,
            BLUE,
        );

        let old_radius = planet.radius();
        planet.set_mass(2000.0);

        assert_eq!(planet.mass(), 2000.0);
        // Radius should increase with mass (cube root relationship)
        assert!(planet.radius() > old_radius);
    }

    #[test]
    fn test_fuel_collection_capability() {
        let planet = Planet::new(
            Vec2::new(0.0, 0.0),
            20.0,
            GameConstants::MIN_PLANET_MASS_FOR_COLLECTION + 100.0,
            BLUE,
        );
        assert!(planet.can_collect_fuel());

        let small_planet = Planet::new(
            Vec2::new(0.0, 0.0),
            5.0,
            GameConstants::MIN_PLANET_MASS_FOR_COLLECTION - 100.0,
            BLUE,
        );
        assert!(!small_planet.can_collect_fuel());
    }
}
