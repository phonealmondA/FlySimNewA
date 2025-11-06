// Game Constants - Ported from C++ GameConstants.h
// Global constants for physics, gameplay, and visualization

use lazy_static::lazy_static;
use std::f32::consts::PI as STD_PI;

/// Game constants structure
pub struct GameConstants;

impl GameConstants {
    // ==================== Gravitational Constants ====================
    pub const G: f32 = 100.0;  // Gravitational constant
    pub const PI: f32 = 3.14159265358979323846;

    // ==================== Mass-Radius Relationship ====================
    pub const BASE_RADIUS_FACTOR: f32 = 260.0;
    pub const REFERENCE_MASS: f32 = 10000.0;

    // ==================== Primary Planet Parameters ====================
    pub const MAIN_PLANET_MASS: f32 = 198910000.0;
    pub const ORBIT_PERIOD: f32 = 420.0;  // seconds

    // ==================== Derived Planet Parameters ====================
    pub const SECONDARY_PLANET_MASS: f32 = Self::MAIN_PLANET_MASS * 0.06;

    // Fixed radius values
    pub const MAIN_PLANET_RADIUS: f32 = 10000.0;
    pub const MASS_RATIO: f32 = 0.06;
    pub const CUBE_ROOT_APPROX: f32 = 60.0;
    pub const SECONDARY_PLANET_RADIUS: f32 = (Self::MAIN_PLANET_RADIUS / Self::CUBE_ROOT_APPROX) / 10000.0;

    // Planet positions
    pub const MAIN_PLANET_X: f32 = 400.0;
    pub const MAIN_PLANET_Y: f32 = 300.0;

    // ==================== Rocket Parameters ====================
    pub const ROCKET_BASE_MASS: f32 = 1.0;
    pub const ROCKET_MAX_MASS: f32 = 101.0;
    pub const ROCKET_SIZE: f32 = 15.0;

    // ==================== Fuel System Constants ====================
    pub const ROCKET_MAX_FUEL: f32 = 100.0;
    pub const ROCKET_STARTING_FUEL: f32 = 0.0;

    // Manual fuel transfer
    pub const MANUAL_FUEL_TRANSFER_RATE: f32 = 10.0;
    pub const FUEL_TRANSFER_THRUST_MULTIPLIER: f32 = 0.1;

    // Fuel consumption
    pub const FUEL_CONSUMPTION_BASE: f32 = 2.0;
    pub const FUEL_CONSUMPTION_MULTIPLIER: f32 = 8.0;
    pub const FUEL_CONSUMPTION_MIN_THRESHOLD: f32 = 0.1;

    // Automatic fuel collection (for satellites)
    pub const FUEL_COLLECTION_RANGE: f32 = 250.0;
    pub const FUEL_COLLECTION_RATE: f32 = 15.0;
    pub const FUEL_COLLECTION_MASS_RATIO: f32 = 150.0;
    pub const MIN_PLANET_MASS_FOR_COLLECTION: f32 = 50.0;

    // Fuel collection ring visual
    pub const FUEL_RING_THICKNESS: f32 = 3.0;

    // ==================== Visualization Settings ====================
    pub const GRAVITY_VECTOR_SCALE: f32 = 15.0;
    pub const VELOCITY_VECTOR_SCALE: f32 = 10.0;

    // ==================== Trajectory Calculation ====================
    pub const TRAJECTORY_TIME_STEP: f32 = 0.5;
    pub const TRAJECTORY_STEPS: i32 = 1000;
    pub const TRAJECTORY_COLLISION_RADIUS: f32 = 0.5;

    // ==================== Engine Parameters ====================
    pub const BASE_THRUST_MULTIPLIER: f32 = 10000000000.0;
    pub const ENGINE_THRUST_POWER: f32 = Self::G * Self::BASE_THRUST_MULTIPLIER;

    // ==================== Vehicle Transformation ====================
    pub const TRANSFORM_VELOCITY_FACTOR: f32 = 0.1;

    // ==================== Satellite System Constants ====================

    // Basic parameters
    pub const SATELLITE_BASE_MASS: f32 = 0.8;
    pub const SATELLITE_MAX_MASS: f32 = 80.0;
    pub const SATELLITE_MAX_FUEL: f32 = 80.0;
    pub const SATELLITE_STARTING_FUEL: f32 = 60.0;
    pub const SATELLITE_SIZE: f32 = 12.0;

    // Station-keeping and orbital maintenance
    pub const SATELLITE_MAINTENANCE_FUEL_PERCENT: f32 = 0.2;
    pub const SATELLITE_ORBIT_TOLERANCE: f32 = 50.0;
    pub const SATELLITE_ECCENTRICITY_TOLERANCE: f32 = 0.01;
    pub const SATELLITE_INCLINATION_TOLERANCE: f32 = 0.017;
    pub const SATELLITE_PERIOD_TOLERANCE: f32 = 10.0;

    // Maintenance timing
    pub const SATELLITE_MAINTENANCE_CHECK_INTERVAL: f32 = 30.0;
    pub const SATELLITE_CORRECTION_DELAY: f32 = 5.0;
    pub const SATELLITE_MAX_SINGLE_BURN: f32 = 3.0;

    // Fuel efficiency
    pub const SATELLITE_FUEL_EFFICIENCY: f32 = 1.2;
    pub const SATELLITE_MAINTENANCE_FUEL_RATE: f32 = 0.5;
    pub const SATELLITE_THRUST_TO_WEIGHT_RATIO: f32 = 0.08;

    // Fuel transfer network
    pub const SATELLITE_TRANSFER_RANGE: f32 = 2500.0;
    pub const SATELLITE_BASE_TRANSFER_RATE: f32 = 25.0;
    pub const SATELLITE_TRANSFER_EFFICIENCY: f32 = 1.0;
    pub const SATELLITE_MAX_SIMULTANEOUS_TRANSFERS: i32 = 5;

    // Emergency thresholds
    pub const SATELLITE_EMERGENCY_FUEL_THRESHOLD: f32 = 0.1;
    pub const SATELLITE_CRITICAL_FUEL_THRESHOLD: f32 = 0.05;
    pub const SATELLITE_MINIMUM_MAINTENANCE_FUEL: f32 = 5.0;

    // Visual constants
    pub const SATELLITE_PANEL_SIZE: f32 = 8.0;
    pub const SATELLITE_ORBIT_PATH_THICKNESS: f32 = 2.0;
    pub const SATELLITE_CONNECTION_THICKNESS: f32 = 1.5;
    pub const SATELLITE_TRANSFER_THICKNESS: f32 = 3.0;

    // Conversion parameters
    pub const SATELLITE_CONVERSION_FUEL_RETENTION: f32 = 0.8;
    pub const SATELLITE_CONVERSION_MASS_EFFICIENCY: f32 = 0.9;
    pub const SATELLITE_MIN_CONVERSION_ALTITUDE: f32 = 100.0;

    // Orbital prediction
    pub const SATELLITE_PREDICTION_TIME_STEP: f32 = 1.0;
    pub const SATELLITE_PREDICTION_STEPS: i32 = 3600;
    pub const SATELLITE_DRIFT_DETECTION_SENSITIVITY: f32 = 0.1;

    // Network optimization
    pub const SATELLITE_NETWORK_UPDATE_INTERVAL: f32 = 2.0;
    pub const SATELLITE_FUEL_BALANCE_THRESHOLD: f32 = 0.15;
    pub const SATELLITE_PRIORITY_DISTANCE_FACTOR: f32 = 0.001;

    // Performance limits
    pub const SATELLITE_MAX_NETWORK_SIZE: i32 = 50;
    pub const SATELLITE_MAX_MANEUVERS_QUEUED: i32 = 10;
    pub const SATELLITE_MANEUVER_TIMEOUT: f32 = 300.0;

    // Advanced features
    pub const SATELLITE_ADAPTIVE_LEARNING_RATE: f32 = 0.01;
    pub const SATELLITE_COLLABORATIVE_RANGE: f32 = 1000.0;
    pub const SATELLITE_RESONANCE_DETECTION_THRESHOLD: f32 = 0.05;

    // Integration with existing systems
    pub const SATELLITE_GRAVITY_INFLUENCE_FACTOR: f32 = 0.1;
    pub const SATELLITE_ROCKET_DOCKING_RANGE: f32 = 210.0;
    pub const SATELLITE_PLANET_COLLECTION_EFFICIENCY: f32 = 1.2;
}

// Runtime-calculated constants using lazy_static
lazy_static! {
    /// Orbital distance calculated from gravitational constant and orbital period
    pub static ref PLANET_ORBIT_DISTANCE: f32 = {
        let g = GameConstants::G;
        let m = GameConstants::MAIN_PLANET_MASS;
        let t = GameConstants::ORBIT_PERIOD;
        let pi = GameConstants::PI;
        ((g * m * t * t) / (4.0 * pi * pi)).powf(1.0 / 3.0)
    };

    /// Secondary planet X position
    pub static ref SECONDARY_PLANET_X: f32 = {
        GameConstants::MAIN_PLANET_X + *PLANET_ORBIT_DISTANCE
    };

    /// Secondary planet Y position
    pub static ref SECONDARY_PLANET_Y: f32 = {
        GameConstants::MAIN_PLANET_Y
    };

    /// Orbital velocity for circular orbit
    pub static ref SECONDARY_PLANET_ORBITAL_VELOCITY: f32 = {
        (GameConstants::G * GameConstants::MAIN_PLANET_MASS / *PLANET_ORBIT_DISTANCE).sqrt()
    };
}

// Color constants (using macroquad color representation)
pub mod colors {
    use macroquad::prelude::Color;

    // Helper function to create colors from RGBA values (0-255)
    const fn rgba(r: u8, g: u8, b: u8, a: u8) -> Color {
        Color::new(
            r as f32 / 255.0,
            g as f32 / 255.0,
            b as f32 / 255.0,
            a as f32 / 255.0,
        )
    }

    // Fuel ring colors
    pub const FUEL_RING_COLOR: Color = rgba(0, 255, 255, 128);
    pub const FUEL_RING_ACTIVE_COLOR: Color = rgba(255, 255, 0, 200);

    // Satellite body colors
    pub const SATELLITE_BODY_COLOR: Color = rgba(100, 200, 255, 255);
    pub const SATELLITE_PANEL_COLOR: Color = rgba(50, 50, 200, 255);

    // Satellite status colors
    pub const SATELLITE_STATUS_ACTIVE: Color = rgba(0, 255, 0, 200);
    pub const SATELLITE_STATUS_LOW_FUEL: Color = rgba(255, 255, 0, 200);
    pub const SATELLITE_STATUS_CRITICAL: Color = rgba(255, 100, 0, 200);
    pub const SATELLITE_STATUS_DEPLETED: Color = rgba(255, 0, 0, 200);

    // Orbit visualization colors
    pub const SATELLITE_ORBIT_PATH_COLOR: Color = rgba(0, 255, 255, 128);
    pub const SATELLITE_TARGET_ORBIT_COLOR: Color = rgba(255, 255, 0, 128);
    pub const SATELLITE_MAINTENANCE_BURN_COLOR: Color = rgba(255, 0, 255, 255);

    // Network visualization colors
    pub const SATELLITE_CONNECTION_COLOR: Color = rgba(100, 255, 100, 100);
    pub const SATELLITE_TRANSFER_FLOW_COLOR: Color = rgba(255, 255, 100, 200);
    pub const SATELLITE_EMERGENCY_COLOR: Color = rgba(255, 50, 50, 255);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_gravitational_constant() {
        assert_eq!(GameConstants::G, 100.0);
    }

    #[test]
    fn test_planet_masses() {
        assert_eq!(GameConstants::MAIN_PLANET_MASS, 198910000.0);
        assert_eq!(
            GameConstants::SECONDARY_PLANET_MASS,
            GameConstants::MAIN_PLANET_MASS * 0.06
        );
    }

    #[test]
    fn test_orbit_distance_calculation() {
        // Should be positive and finite
        assert!(*PLANET_ORBIT_DISTANCE > 0.0);
        assert!(PLANET_ORBIT_DISTANCE.is_finite());
    }

    #[test]
    fn test_orbital_velocity() {
        // Should be positive and finite
        assert!(*SECONDARY_PLANET_ORBITAL_VELOCITY > 0.0);
        assert!(SECONDARY_PLANET_ORBITAL_VELOCITY.is_finite());
    }

    #[test]
    fn test_fuel_constants() {
        assert_eq!(GameConstants::ROCKET_MAX_FUEL, 100.0);
        assert_eq!(GameConstants::ROCKET_STARTING_FUEL, 0.0);
        assert!(GameConstants::ROCKET_STARTING_FUEL <= GameConstants::ROCKET_MAX_FUEL);
    }
}
