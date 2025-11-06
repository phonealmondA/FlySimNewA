// Gravity Simulator - Applies gravitational forces between objects
// Ported from C++ GravitySimulator class

use macroquad::prelude::*;

use crate::entities::{GameObject, Planet, Rocket, Satellite};
use crate::game_constants::GameConstants;
use crate::utils::vector_helper;

/// Gravity simulator that applies gravitational forces between all objects
pub struct GravitySimulator {
    g: f32,
    simulate_planet_gravity: bool,
}

impl GravitySimulator {
    pub fn new() -> Self {
        GravitySimulator {
            g: GameConstants::G,
            simulate_planet_gravity: true,
        }
    }

    pub fn set_simulate_planet_gravity(&mut self, enable: bool) {
        self.simulate_planet_gravity = enable;
    }

    /// Apply gravity from all planets to a rocket
    pub fn apply_planet_gravity_to_rocket(
        &self,
        rocket: &mut Rocket,
        planets: &[&Planet],
        delta_time: f32,
    ) {
        if !self.simulate_planet_gravity {
            return;
        }

        for planet in planets {
            let force = self.calculate_gravitational_force(
                rocket.position(),
                rocket.mass(),
                planet.position(),
                planet.mass(),
            );

            let acceleration = force / rocket.mass();
            rocket.set_velocity(rocket.velocity() + acceleration * delta_time);
        }
    }

    /// Apply gravity from all planets to a satellite
    pub fn apply_planet_gravity_to_satellite(
        &self,
        satellite: &mut Satellite,
        planets: &[&Planet],
        delta_time: f32,
    ) {
        if !self.simulate_planet_gravity {
            return;
        }

        for planet in planets {
            let force = self.calculate_gravitational_force(
                satellite.position(),
                satellite.mass(),
                planet.position(),
                planet.mass(),
            );

            let acceleration = force / satellite.mass();
            satellite.set_velocity(satellite.velocity() + acceleration * delta_time);
        }
    }

    /// Apply mutual gravity between two planets
    pub fn apply_mutual_planet_gravity(
        &self,
        planet1: &mut Planet,
        planet2: &mut Planet,
        delta_time: f32,
    ) {
        if !self.simulate_planet_gravity {
            return;
        }

        let force1 = self.calculate_gravitational_force(
            planet1.position(),
            planet1.mass(),
            planet2.position(),
            planet2.mass(),
        );

        let acceleration1 = force1 / planet1.mass();
        let acceleration2 = -force1 / planet2.mass();

        planet1.set_velocity(planet1.velocity() + acceleration1 * delta_time);
        planet2.set_velocity(planet2.velocity() + acceleration2 * delta_time);
    }

    /// Apply gravity between two rockets
    pub fn apply_rocket_to_rocket_gravity(
        &self,
        rocket1: &mut Rocket,
        rocket2: &mut Rocket,
        delta_time: f32,
    ) {
        let force = self.calculate_gravitational_force(
            rocket1.position(),
            rocket1.mass(),
            rocket2.position(),
            rocket2.mass(),
        );

        let acceleration1 = force / rocket1.mass();
        let acceleration2 = -force / rocket2.mass();

        rocket1.set_velocity(rocket1.velocity() + acceleration1 * delta_time);
        rocket2.set_velocity(rocket2.velocity() + acceleration2 * delta_time);
    }

    /// Calculate gravitational force vector from object1 to object2
    /// Returns the force vector applied to object1
    fn calculate_gravitational_force(
        &self,
        pos1: Vec2,
        mass1: f32,
        pos2: Vec2,
        mass2: f32,
    ) -> Vec2 {
        let direction = pos2 - pos1;
        let mut distance = vector_helper::magnitude(direction);

        // Minimum distance to prevent extreme forces
        const MIN_DISTANCE: f32 = 0.5;
        if distance < MIN_DISTANCE {
            distance = MIN_DISTANCE;
        }

        // F = G * m1 * m2 / r^2
        let force_magnitude = self.g * mass1 * mass2 / (distance * distance);

        // Return force vector
        vector_helper::normalize(direction) * force_magnitude
    }

    /// Calculate orbital velocity for a circular orbit
    pub fn calculate_circular_orbit_velocity(
        &self,
        center_pos: Vec2,
        center_mass: f32,
        orbit_pos: Vec2,
    ) -> Vec2 {
        let direction_to_center = center_pos - orbit_pos;
        let distance = vector_helper::magnitude(direction_to_center);

        if distance < 0.01 {
            return Vec2::new(0.0, 0.0);
        }

        // v = sqrt(G * M / r)
        let velocity_magnitude = (self.g * center_mass / distance).sqrt();

        // Perpendicular to direction (rotate 90 degrees)
        let normalized_dir = vector_helper::normalize(direction_to_center);
        let perpendicular = Vec2::new(-normalized_dir.y, normalized_dir.x);

        perpendicular * velocity_magnitude
    }

    /// Calculate escape velocity from a celestial body
    pub fn calculate_escape_velocity(&self, mass: f32, distance: f32) -> f32 {
        // v_escape = sqrt(2 * G * M / r)
        (2.0 * self.g * mass / distance).sqrt()
    }
}

impl Default for GravitySimulator {
    fn default() -> Self {
        Self::new()
    }
}

/// Orbital mechanics calculations
pub mod orbital {
    use super::*;

    /// Calculate apoapsis (highest point in orbit)
    pub fn calculate_apoapsis(
        position: Vec2,
        velocity: Vec2,
        planet_pos: Vec2,
        planet_mass: f32,
        g: f32,
    ) -> f32 {
        let r = vector_helper::distance(position, planet_pos);
        let v = vector_helper::magnitude(velocity);

        // Specific orbital energy: E = v^2/2 - GM/r
        let energy = (v * v) / 2.0 - (g * planet_mass) / r;

        // For elliptical orbits (E < 0):
        if energy < 0.0 {
            // Semi-major axis: a = -GM / (2E)
            let semi_major_axis = -(g * planet_mass) / (2.0 * energy);

            // Specific angular momentum
            let relative_pos = position - planet_pos;
            let h = vector_helper::cross(relative_pos, velocity);

            // Eccentricity: e = sqrt(1 + 2Eh^2 / (GM)^2)
            let gm = g * planet_mass;
            let eccentricity = (1.0 + (2.0 * energy * h * h) / (gm * gm)).sqrt();

            // Apoapsis: r_a = a(1 + e)
            semi_major_axis * (1.0 + eccentricity)
        } else {
            // Hyperbolic or parabolic trajectory (no apoapsis)
            f32::INFINITY
        }
    }

    /// Calculate periapsis (lowest point in orbit)
    pub fn calculate_periapsis(
        position: Vec2,
        velocity: Vec2,
        planet_pos: Vec2,
        planet_mass: f32,
        g: f32,
    ) -> f32 {
        let r = vector_helper::distance(position, planet_pos);
        let v = vector_helper::magnitude(velocity);

        let energy = (v * v) / 2.0 - (g * planet_mass) / r;

        if energy < 0.0 {
            let semi_major_axis = -(g * planet_mass) / (2.0 * energy);

            let relative_pos = position - planet_pos;
            let h = vector_helper::cross(relative_pos, velocity);

            let gm = g * planet_mass;
            let eccentricity = (1.0 + (2.0 * energy * h * h) / (gm * gm)).sqrt();

            // Periapsis: r_p = a(1 - e)
            semi_major_axis * (1.0 - eccentricity)
        } else {
            // Current distance is the closest approach
            r
        }
    }

    /// Calculate orbital period
    pub fn calculate_orbital_period(
        position: Vec2,
        velocity: Vec2,
        planet_pos: Vec2,
        planet_mass: f32,
        g: f32,
    ) -> f32 {
        let r = vector_helper::distance(position, planet_pos);
        let v = vector_helper::magnitude(velocity);

        let energy = (v * v) / 2.0 - (g * planet_mass) / r;

        if energy < 0.0 {
            let semi_major_axis = -(g * planet_mass) / (2.0 * energy);

            // Kepler's third law: T = 2π * sqrt(a^3 / GM)
            2.0 * std::f32::consts::PI * (semi_major_axis.powi(3) / (g * planet_mass)).sqrt()
        } else {
            f32::INFINITY
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use approx::assert_relative_eq;

    #[test]
    fn test_gravitational_force_calculation() {
        let sim = GravitySimulator::new();
        let pos1 = Vec2::new(0.0, 0.0);
        let pos2 = Vec2::new(100.0, 0.0);
        let mass1 = 1000.0;
        let mass2 = 1000.0;

        let force = sim.calculate_gravitational_force(pos1, mass1, pos2, mass2);

        // Force should point in positive X direction
        assert!(force.x > 0.0);
        assert_relative_eq!(force.y, 0.0, epsilon = 0.01);
    }

    #[test]
    fn test_circular_orbit_velocity() {
        let sim = GravitySimulator::new();
        let center = Vec2::new(0.0, 0.0);
        let orbit_pos = Vec2::new(1000.0, 0.0);
        let mass = 198910000.0;

        let velocity = sim.calculate_circular_orbit_velocity(center, mass, orbit_pos);

        // Velocity should be perpendicular (in Y direction for this setup)
        assert_relative_eq!(velocity.x, 0.0, epsilon = 0.01);
        assert!(velocity.y.abs() > 0.0);

        // Check magnitude: v = sqrt(G*M/r)
        let expected_magnitude = (GameConstants::G * mass / 1000.0).sqrt();
        let actual_magnitude = vector_helper::magnitude(velocity);
        assert_relative_eq!(actual_magnitude, expected_magnitude, epsilon = 1.0);
    }

    #[test]
    fn test_escape_velocity() {
        let sim = GravitySimulator::new();
        let mass = 198910000.0;
        let distance = 10000.0;

        let v_escape = sim.calculate_escape_velocity(mass, distance);

        // v_escape = sqrt(2 * G * M / r)
        let expected = (2.0 * GameConstants::G * mass / distance).sqrt();
        assert_relative_eq!(v_escape, expected, epsilon = 0.01);
    }
}
