// Rocket - Player-controlled spacecraft with fuel and thrust
// Ported from C++ Rocket class

use macroquad::prelude::*;

use super::game_object::{GameObject, GameObjectData};
use crate::game_constants::GameConstants;

/// Rocket with dynamic mass, fuel system, and thrust control
pub struct Rocket {
    data: GameObjectData,
    rotation: f32,
    angular_velocity: f32,
    thrust_level: f32, // 0.0 to 1.0

    // Dynamic mass system
    mass: f32,
    base_mass: f32,
    max_mass: f32,

    // Fuel system
    current_fuel: f32,
    max_fuel: f32,
    is_currently_thrusting: bool,

    // Manual fuel transfer
    is_transferring_fuel_in: bool,
    is_transferring_fuel_out: bool,
    fuel_transfer_rate: f32,

    // Rocket parts (engines, etc.) - simplified for now
    // parts: Vec<Box<dyn RocketPart>>, // Will add later
}

impl Rocket {
    pub fn new(position: Vec2, velocity: Vec2, color: Color, base_mass: f32) -> Self {
        let max_fuel = GameConstants::ROCKET_MAX_FUEL;
        let starting_fuel = GameConstants::ROCKET_STARTING_FUEL;
        let mass = base_mass + starting_fuel;

        Rocket {
            data: GameObjectData::new(position, velocity, color),
            rotation: 0.0,
            angular_velocity: 0.0,
            thrust_level: 0.0,
            mass,
            base_mass,
            max_mass: GameConstants::ROCKET_MAX_MASS,
            current_fuel: starting_fuel,
            max_fuel,
            is_currently_thrusting: false,
            is_transferring_fuel_in: false,
            is_transferring_fuel_out: false,
            fuel_transfer_rate: 0.0,
        }
    }

    // === Mass System ===
    pub fn mass(&self) -> f32 {
        self.mass
    }

    pub fn base_mass(&self) -> f32 {
        self.base_mass
    }

    pub fn max_mass(&self) -> f32 {
        self.max_mass
    }

    pub fn mass_capacity_remaining(&self) -> f32 {
        self.max_mass - self.mass
    }

    fn update_mass_from_fuel(&mut self) {
        let old_mass = self.mass;
        self.mass = self.base_mass + self.current_fuel;

        // Preserve momentum when mass changes
        if old_mass > 0.0 && self.mass > 0.0 {
            self.preserve_momentum_during_mass_change(old_mass, self.mass);
        }
    }

    fn preserve_momentum_during_mass_change(&mut self, old_mass: f32, new_mass: f32) {
        // p = m * v  =>  v_new = (m_old * v_old) / m_new
        if new_mass > 0.0 {
            self.data.velocity = self.data.velocity * (old_mass / new_mass);
        }
    }

    // === Fuel System ===
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

    pub fn can_thrust(&self) -> bool {
        self.current_fuel > 0.0
    }

    pub fn set_fuel(&mut self, fuel: f32) {
        self.current_fuel = fuel.clamp(0.0, self.max_fuel);
        self.update_mass_from_fuel();
    }

    pub fn add_fuel(&mut self, fuel: f32) {
        self.set_fuel(self.current_fuel + fuel);
    }

    fn consume_fuel(&mut self, delta_time: f32) {
        if self.thrust_level < GameConstants::FUEL_CONSUMPTION_MIN_THRESHOLD {
            return;
        }

        let consumption = self.calculate_fuel_consumption() * delta_time;
        self.current_fuel = (self.current_fuel - consumption).max(0.0);
        self.update_mass_from_fuel();
    }

    fn calculate_fuel_consumption(&self) -> f32 {
        if self.thrust_level < GameConstants::FUEL_CONSUMPTION_MIN_THRESHOLD {
            return 0.0;
        }

        GameConstants::FUEL_CONSUMPTION_BASE +
            (self.thrust_level * GameConstants::FUEL_CONSUMPTION_MULTIPLIER)
    }

    // === Thrust and Control ===
    pub fn thrust_level(&self) -> f32 {
        self.thrust_level
    }

    pub fn set_thrust_level(&mut self, level: f32) {
        self.thrust_level = level.clamp(0.0, 1.0);
    }

    pub fn apply_thrust(&mut self, amount: f32) {
        if !self.can_thrust() {
            self.is_currently_thrusting = false;
            return;
        }

        self.is_currently_thrusting = true;

        // Calculate thrust direction
        let thrust_direction = Vec2::new(
            self.rotation.cos(),
            self.rotation.sin(),
        );

        // Apply thrust force
        let thrust_force = thrust_direction * amount * GameConstants::ENGINE_THRUST_POWER;
        let acceleration = thrust_force / self.mass;

        self.data.velocity += acceleration;
    }

    pub fn rotate(&mut self, amount: f32) {
        self.rotation += amount;
        // Normalize angle to [0, 2*PI]
        self.rotation = self.rotation.rem_euclid(2.0 * std::f32::consts::PI);
    }

    pub fn rotation(&self) -> f32 {
        self.rotation
    }

    // === Fuel Transfer ===
    pub fn start_fuel_transfer_in(&mut self, transfer_rate: f32) {
        self.is_transferring_fuel_in = true;
        self.is_transferring_fuel_out = false;
        self.fuel_transfer_rate = transfer_rate;
    }

    pub fn start_fuel_transfer_out(&mut self, transfer_rate: f32) {
        self.is_transferring_fuel_out = true;
        self.is_transferring_fuel_in = false;
        self.fuel_transfer_rate = transfer_rate;
    }

    pub fn stop_fuel_transfer(&mut self) {
        self.is_transferring_fuel_in = false;
        self.is_transferring_fuel_out = false;
        self.fuel_transfer_rate = 0.0;
    }

    pub fn is_transferring_fuel(&self) -> bool {
        self.is_transferring_fuel_in || self.is_transferring_fuel_out
    }

    pub fn current_transfer_rate(&self) -> f32 {
        self.fuel_transfer_rate
    }
}

impl GameObject for Rocket {
    fn update(&mut self, delta_time: f32) {
        // Apply thrust if thrust level is set
        if self.thrust_level > 0.0 {
            self.apply_thrust(self.thrust_level * delta_time);
            self.consume_fuel(delta_time);
        } else {
            self.is_currently_thrusting = false;
        }

        // Update position
        self.data.position += self.data.velocity * delta_time;
    }

    fn draw(&self) {
        // Rocket body is a triangle
        // Local coordinates (relative to rocket center)
        let local_points = [
            Vec2::new(0.0, -GameConstants::ROCKET_SIZE),
            Vec2::new(-GameConstants::ROCKET_SIZE / 2.0, GameConstants::ROCKET_SIZE),
            Vec2::new(GameConstants::ROCKET_SIZE / 2.0, GameConstants::ROCKET_SIZE),
        ];

        // Rotate and translate points to world space
        let cos_r = self.rotation.cos();
        let sin_r = self.rotation.sin();

        let world_points: Vec<Vec2> = local_points.iter().map(|p| {
            let rotated_x = p.x * cos_r - p.y * sin_r;
            let rotated_y = p.x * sin_r + p.y * cos_r;
            self.data.position + Vec2::new(rotated_x, rotated_y)
        }).collect();

        // Draw the rocket body
        draw_triangle(
            world_points[0],
            world_points[1],
            world_points[2],
            self.data.color,
        );

        // TODO: Draw rocket parts (engines, etc.)
        // TODO: Draw velocity vector if enabled
        // TODO: Draw trajectory prediction if enabled
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
    use approx::assert_relative_eq;

    #[test]
    fn test_rocket_creation() {
        let rocket = Rocket::new(
            Vec2::new(0.0, 0.0),
            Vec2::new(0.0, 0.0),
            WHITE,
            GameConstants::ROCKET_BASE_MASS,
        );

        assert_eq!(rocket.base_mass(), GameConstants::ROCKET_BASE_MASS);
        assert_eq!(rocket.current_fuel(), GameConstants::ROCKET_STARTING_FUEL);
    }

    #[test]
    fn test_fuel_addition() {
        let mut rocket = Rocket::new(
            Vec2::new(0.0, 0.0),
            Vec2::new(0.0, 0.0),
            WHITE,
            GameConstants::ROCKET_BASE_MASS,
        );

        rocket.add_fuel(50.0);
        assert_relative_eq!(rocket.current_fuel(), GameConstants::ROCKET_STARTING_FUEL + 50.0, epsilon = 0.01);
    }

    #[test]
    fn test_fuel_consumption() {
        let mut rocket = Rocket::new(
            Vec2::new(0.0, 0.0),
            Vec2::new(0.0, 0.0),
            WHITE,
            GameConstants::ROCKET_BASE_MASS,
        );

        rocket.set_fuel(100.0);
        let initial_fuel = rocket.current_fuel();

        rocket.set_thrust_level(0.5);
        rocket.update(1.0);

        assert!(rocket.current_fuel() < initial_fuel);
    }

    #[test]
    fn test_mass_updates_with_fuel() {
        let mut rocket = Rocket::new(
            Vec2::new(0.0, 0.0),
            Vec2::new(0.0, 0.0),
            WHITE,
            GameConstants::ROCKET_BASE_MASS,
        );

        let initial_mass = rocket.mass();
        rocket.add_fuel(10.0);
        assert_relative_eq!(rocket.mass(), initial_mass + 10.0, epsilon = 0.01);
    }

    #[test]
    fn test_rotation() {
        let mut rocket = Rocket::new(
            Vec2::new(0.0, 0.0),
            Vec2::new(0.0, 0.0),
            WHITE,
            GameConstants::ROCKET_BASE_MASS,
        );

        rocket.rotate(std::f32::consts::PI / 2.0);
        assert_relative_eq!(rocket.rotation(), std::f32::consts::PI / 2.0, epsilon = 0.01);
    }
}
