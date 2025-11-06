// Planet - Celestial body with gravity and fuel storage
// Ported from C++ Planet class (simplified without moons initially)

use sfml::graphics::{
    CircleShape, Color, RenderTarget, RenderWindow, Shape, Transformable,
};
use sfml::system::Vector2f;

use super::game_object::{GameObject, GameObjectData};
use crate::game_constants::GameConstants;

/// Planet entity with mass, gravity, and fuel storage
pub struct Planet {
    data: GameObjectData,
    mass: f32,
    radius: f32,
    shape: CircleShape<'static>,
}

impl Planet {
    pub fn new(position: Vector2f, radius: f32, mass: f32, color: Color) -> Self {
        let mut shape = CircleShape::new(radius, 50);
        shape.set_fill_color(color);
        shape.set_origin(Vector2f::new(radius, radius));
        shape.set_position(position);

        Planet {
            data: GameObjectData::new(position, Vector2f::new(0.0, 0.0), color),
            mass,
            radius,
            shape,
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

        // Update shape
        self.shape.set_radius(self.radius);
        self.shape.set_origin(Vector2f::new(self.radius, self.radius));
        self.shape.set_point_count(50);
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
    pub fn draw_fuel_collection_ring(&self, window: &mut RenderWindow, is_actively_collecting: bool) {
        if !self.can_collect_fuel() {
            return;
        }

        let collection_radius = self.fuel_collection_range();
        let mut ring = CircleShape::new(collection_radius, 50);
        ring.set_fill_color(Color::TRANSPARENT);

        let color = if is_actively_collecting {
            crate::game_constants::colors::FUEL_RING_ACTIVE_COLOR
        } else {
            crate::game_constants::colors::FUEL_RING_COLOR
        };

        ring.set_outline_color(color);
        ring.set_outline_thickness(GameConstants::FUEL_RING_THICKNESS);
        ring.set_origin(Vector2f::new(collection_radius, collection_radius));
        ring.set_position(self.data.position);

        window.draw(&ring);
    }
}

impl GameObject for Planet {
    fn update(&mut self, delta_time: f32) {
        // Update position based on velocity
        self.data.position += self.data.velocity * delta_time;
        self.shape.set_position(self.data.position);
    }

    fn draw(&self, window: &mut RenderWindow) {
        window.draw(&self.shape);
    }

    fn position(&self) -> Vector2f {
        self.data.position
    }

    fn velocity(&self) -> Vector2f {
        self.data.velocity
    }

    fn set_velocity(&mut self, velocity: Vector2f) {
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
            Vector2f::new(0.0, 0.0),
            100.0,
            10000.0,
            Color::BLUE,
        );
        assert_eq!(planet.mass(), 10000.0);
        assert_eq!(planet.radius(), 100.0);
    }

    #[test]
    fn test_planet_mass_update() {
        let mut planet = Planet::new(
            Vector2f::new(0.0, 0.0),
            100.0,
            10000.0,
            Color::BLUE,
        );
        planet.set_mass(20000.0);
        assert_eq!(planet.mass(), 20000.0);
        // Radius should be updated (cube root scaling)
        assert!(planet.radius() > 100.0);
    }

    #[test]
    fn test_fuel_collection_check() {
        let planet = Planet::new(
            Vector2f::new(0.0, 0.0),
            100.0,
            GameConstants::MIN_PLANET_MASS_FOR_COLLECTION + 10.0,
            Color::BLUE,
        );
        assert!(planet.can_collect_fuel());

        let small_planet = Planet::new(
            Vector2f::new(0.0, 0.0),
            10.0,
            GameConstants::MIN_PLANET_MASS_FOR_COLLECTION - 10.0,
            Color::BLUE,
        );
        assert!(!small_planet.can_collect_fuel());
    }
}
