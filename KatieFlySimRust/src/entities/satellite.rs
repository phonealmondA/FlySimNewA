// Satellite - Autonomous spacecraft for fuel collection
// Ported from C++ Satellite class (simplified)

use sfml::graphics::{
    CircleShape, Color, RectangleShape, RenderTarget, RenderWindow, Shape, Transformable,
};
use sfml::system::Vector2f;

use super::game_object::{GameObject, GameObjectData};
use crate::game_constants::GameConstants;

/// Satellite for automated fuel collection and orbital maintenance
pub struct Satellite {
    data: GameObjectData,
    rotation: f32,

    // Mass and fuel
    mass: f32,
    base_mass: f32,
    max_mass: f32,
    current_fuel: f32,
    max_fuel: f32,

    // Orbital maintenance
    target_orbit_radius: f32,
    is_maintaining_orbit: bool,

    // Visual components
    body: CircleShape<'static>,
}

impl Satellite {
    pub fn new(position: Vector2f, velocity: Vector2f, color: Color) -> Self {
        let mut body = CircleShape::new(GameConstants::SATELLITE_SIZE, 30);
        body.set_fill_color(color);
        body.set_origin(Vector2f::new(
            GameConstants::SATELLITE_SIZE,
            GameConstants::SATELLITE_SIZE,
        ));
        body.set_position(position);

        Satellite {
            data: GameObjectData::new(position, velocity, color),
            rotation: 0.0,
            mass: GameConstants::SATELLITE_BASE_MASS + GameConstants::SATELLITE_STARTING_FUEL,
            base_mass: GameConstants::SATELLITE_BASE_MASS,
            max_mass: GameConstants::SATELLITE_MAX_MASS,
            current_fuel: GameConstants::SATELLITE_STARTING_FUEL,
            max_fuel: GameConstants::SATELLITE_MAX_FUEL,
            target_orbit_radius: 0.0,
            is_maintaining_orbit: false,
            body,
        }
    }

    /// Create satellite from a rocket (conversion)
    pub fn from_rocket(position: Vector2f, velocity: Vector2f, rocket_fuel: f32) -> Self {
        let retained_fuel =
            rocket_fuel * GameConstants::SATELLITE_CONVERSION_FUEL_RETENTION;
        let fuel = retained_fuel.min(GameConstants::SATELLITE_MAX_FUEL);

        let mut satellite = Self::new(
            position,
            velocity,
            crate::game_constants::colors::SATELLITE_BODY_COLOR,
        );
        satellite.current_fuel = fuel;
        satellite.mass = satellite.base_mass + fuel;

        satellite
    }

    // === Getters ===
    pub fn mass(&self) -> f32 {
        self.mass
    }

    pub fn current_fuel(&self) -> f32 {
        self.current_fuel
    }

    pub fn max_fuel(&self) -> f32 {
        self.max_fuel
    }

    pub fn fuel_percentage(&self) -> f32 {
        if self.max_fuel > 0.0 {
            (self.current_fuel / self.max_fuel) * 100.0
        } else {
            0.0
        }
    }

    pub fn is_maintaining_orbit(&self) -> bool {
        self.is_maintaining_orbit
    }

    pub fn set_maintaining_orbit(&mut self, maintaining: bool) {
        self.is_maintaining_orbit = maintaining;
    }

    pub fn set_target_orbit_radius(&mut self, radius: f32) {
        self.target_orbit_radius = radius;
    }

    /// Add fuel to satellite
    pub fn add_fuel(&mut self, amount: f32) {
        self.current_fuel = (self.current_fuel + amount).min(self.max_fuel);
        self.mass = self.base_mass + self.current_fuel;
    }

    /// Consume fuel for operations
    pub fn consume_fuel(&mut self, amount: f32) -> bool {
        if self.current_fuel >= amount {
            self.current_fuel -= amount;
            self.mass = self.base_mass + self.current_fuel;
            true
        } else {
            false
        }
    }

    /// Get satellite status color based on fuel level
    pub fn status_color(&self) -> Color {
        let fuel_percent = self.fuel_percentage();
        if fuel_percent > GameConstants::SATELLITE_EMERGENCY_FUEL_THRESHOLD * 100.0 {
            crate::game_constants::colors::SATELLITE_STATUS_ACTIVE
        } else if fuel_percent > GameConstants::SATELLITE_CRITICAL_FUEL_THRESHOLD * 100.0 {
            crate::game_constants::colors::SATELLITE_STATUS_LOW_FUEL
        } else if fuel_percent > 0.0 {
            crate::game_constants::colors::SATELLITE_STATUS_CRITICAL
        } else {
            crate::game_constants::colors::SATELLITE_STATUS_DEPLETED
        }
    }
}

impl GameObject for Satellite {
    fn update(&mut self, delta_time: f32) {
        // Update position
        self.data.position += self.data.velocity * delta_time;

        // Update rotation for visual effect
        self.rotation += delta_time * 0.5; // Slow rotation

        // Update visual
        self.body.set_position(self.data.position);
        self.body.set_rotation(self.rotation * 180.0 / std::f32::consts::PI);

        // TODO: Implement orbital maintenance logic
        // TODO: Implement automatic fuel collection
    }

    fn draw(&self, window: &mut RenderWindow) {
        // Draw body
        window.draw(&self.body);

        // Draw solar panels (simple rectangles)
        let panel_offset = GameConstants::SATELLITE_SIZE + 2.0;
        let panel_size = Vector2f::new(GameConstants::SATELLITE_PANEL_SIZE, 2.0);

        // Left panel
        let mut left_panel = RectangleShape::with_size(panel_size);
        left_panel.set_fill_color(crate::game_constants::colors::SATELLITE_PANEL_COLOR);
        left_panel.set_origin(Vector2f::new(panel_size.x, panel_size.y / 2.0));
        left_panel.set_position(Vector2f::new(
            self.data.position.x - panel_offset,
            self.data.position.y,
        ));
        left_panel.set_rotation(self.rotation * 180.0 / std::f32::consts::PI);
        window.draw(&left_panel);

        // Right panel
        let mut right_panel = RectangleShape::with_size(panel_size);
        right_panel.set_fill_color(crate::game_constants::colors::SATELLITE_PANEL_COLOR);
        right_panel.set_origin(Vector2f::new(0.0, panel_size.y / 2.0));
        right_panel.set_position(Vector2f::new(
            self.data.position.x + panel_offset,
            self.data.position.y,
        ));
        right_panel.set_rotation(self.rotation * 180.0 / std::f32::consts::PI);
        window.draw(&right_panel);

        // Draw status indicator
        let status_radius = 3.0;
        let mut status = CircleShape::new(status_radius, 10);
        status.set_fill_color(self.status_color());
        status.set_origin(Vector2f::new(status_radius, status_radius));
        status.set_position(self.data.position);
        window.draw(&status);
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
    fn test_satellite_creation() {
        let satellite = Satellite::new(
            Vector2f::new(0.0, 0.0),
            Vector2f::new(0.0, 0.0),
            Color::CYAN,
        );
        assert_eq!(satellite.current_fuel(), GameConstants::SATELLITE_STARTING_FUEL);
    }

    #[test]
    fn test_satellite_from_rocket_conversion() {
        let rocket_fuel = 100.0;
        let satellite = Satellite::from_rocket(
            Vector2f::new(0.0, 0.0),
            Vector2f::new(0.0, 0.0),
            rocket_fuel,
        );

        let expected_fuel = rocket_fuel * GameConstants::SATELLITE_CONVERSION_FUEL_RETENTION;
        assert_eq!(satellite.current_fuel(), expected_fuel.min(GameConstants::SATELLITE_MAX_FUEL));
    }

    #[test]
    fn test_fuel_operations() {
        let mut satellite = Satellite::new(
            Vector2f::new(0.0, 0.0),
            Vector2f::new(0.0, 0.0),
            Color::CYAN,
        );

        let initial_fuel = satellite.current_fuel();
        satellite.add_fuel(10.0);
        assert_eq!(satellite.current_fuel(), initial_fuel + 10.0);

        assert!(satellite.consume_fuel(5.0));
        assert_eq!(satellite.current_fuel(), initial_fuel + 5.0);

        // Try to consume more than available
        assert!(!satellite.consume_fuel(1000.0));
    }
}
