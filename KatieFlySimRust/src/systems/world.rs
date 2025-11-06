// World - Central entity manager using Entity ID pattern
// Rust-idiomatic approach to avoid ownership issues

use std::collections::HashMap;

use crate::entities::{GameObject, Planet, Rocket, Satellite};
use crate::physics::GravitySimulator;

/// Entity ID type for safe references
pub type EntityId = usize;

/// World manages all game entities using Entity IDs
pub struct World {
    // Entity storage
    planets: HashMap<EntityId, Planet>,
    rockets: HashMap<EntityId, Rocket>,
    satellites: HashMap<EntityId, Satellite>,

    // ID generation
    next_id: EntityId,

    // Physics
    gravity_simulator: GravitySimulator,

    // Active player rocket (for single player)
    active_rocket_id: Option<EntityId>,
}

impl World {
    pub fn new() -> Self {
        World {
            planets: HashMap::new(),
            rockets: HashMap::new(),
            satellites: HashMap::new(),
            next_id: 0,
            gravity_simulator: GravitySimulator::new(),
            active_rocket_id: None,
        }
    }

    // === Entity Management ===

    /// Add a planet and return its ID
    pub fn add_planet(&mut self, planet: Planet) -> EntityId {
        let id = self.next_id;
        self.planets.insert(id, planet);
        self.next_id += 1;
        id
    }

    /// Add a rocket and return its ID
    pub fn add_rocket(&mut self, rocket: Rocket) -> EntityId {
        let id = self.next_id;
        self.rockets.insert(id, rocket);
        self.next_id += 1;

        // If no active rocket, make this the active one
        if self.active_rocket_id.is_none() {
            self.active_rocket_id = Some(id);
        }

        id
    }

    /// Add a satellite and return its ID
    pub fn add_satellite(&mut self, satellite: Satellite) -> EntityId {
        let id = self.next_id;
        self.satellites.insert(id, satellite);
        self.next_id += 1;
        id
    }

    /// Convert rocket to satellite
    pub fn convert_rocket_to_satellite(&mut self, rocket_id: EntityId) -> Option<EntityId> {
        if let Some(rocket) = self.rockets.remove(&rocket_id) {
            let satellite = Satellite::from_rocket(
                rocket.position(),
                rocket.velocity(),
                rocket.current_fuel(),
            );

            let satellite_id = self.add_satellite(satellite);

            // If this was the active rocket, clear it
            if self.active_rocket_id == Some(rocket_id) {
                self.active_rocket_id = None;
            }

            Some(satellite_id)
        } else {
            None
        }
    }

    // === Getters ===

    pub fn get_planet(&self, id: EntityId) -> Option<&Planet> {
        self.planets.get(&id)
    }

    pub fn get_planet_mut(&mut self, id: EntityId) -> Option<&mut Planet> {
        self.planets.get_mut(&id)
    }

    pub fn get_rocket(&self, id: EntityId) -> Option<&Rocket> {
        self.rockets.get(&id)
    }

    pub fn get_rocket_mut(&mut self, id: EntityId) -> Option<&mut Rocket> {
        self.rockets.get_mut(&id)
    }

    pub fn get_satellite(&self, id: EntityId) -> Option<&Satellite> {
        self.satellites.get(&id)
    }

    pub fn get_satellite_mut(&mut self, id: EntityId) -> Option<&mut Satellite> {
        self.satellites.get_mut(&id)
    }

    pub fn get_active_rocket(&self) -> Option<&Rocket> {
        self.active_rocket_id
            .and_then(|id| self.rockets.get(&id))
    }

    pub fn get_active_rocket_mut(&mut self) -> Option<&mut Rocket> {
        self.active_rocket_id
            .and_then(move |id| self.rockets.get_mut(&id))
    }

    pub fn active_rocket_id(&self) -> Option<EntityId> {
        self.active_rocket_id
    }

    pub fn set_active_rocket(&mut self, id: Option<EntityId>) {
        self.active_rocket_id = id;
    }

    pub fn planet_count(&self) -> usize {
        self.planets.len()
    }

    pub fn rocket_count(&self) -> usize {
        self.rockets.len()
    }

    pub fn satellite_count(&self) -> usize {
        self.satellites.len()
    }

    // === Update ===

    pub fn update(&mut self, delta_time: f32) {
        // Update all planets
        for planet in self.planets.values_mut() {
            planet.update(delta_time);
        }

        // Apply gravity to rockets
        let planet_refs: Vec<&Planet> = self.planets.values().collect();
        for rocket in self.rockets.values_mut() {
            self.gravity_simulator
                .apply_planet_gravity_to_rocket(rocket, &planet_refs, delta_time);
            rocket.update(delta_time);
        }

        // Apply gravity to satellites
        for satellite in self.satellites.values_mut() {
            self.gravity_simulator
                .apply_planet_gravity_to_satellite(satellite, &planet_refs, delta_time);
            satellite.update(delta_time);
        }

        // TODO: Apply planet-to-planet gravity
        // TODO: Apply rocket-to-rocket gravity
    }

    // === Render ===

    pub fn render(&self) {
        // Draw planets
        for planet in self.planets.values() {
            planet.draw();
        }

        // Draw rockets
        for rocket in self.rockets.values() {
            rocket.draw();
        }

        // Draw satellites
        for satellite in self.satellites.values() {
            satellite.draw();
        }
    }

    // === Utility ===

    pub fn clear_all(&mut self) {
        self.planets.clear();
        self.rockets.clear();
        self.satellites.clear();
        self.active_rocket_id = None;
    }

    pub fn gravity_simulator(&self) -> &GravitySimulator {
        &self.gravity_simulator
    }

    pub fn gravity_simulator_mut(&mut self) -> &mut GravitySimulator {
        &mut self.gravity_simulator
    }
}

impl Default for World {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use macroquad::prelude::*;

    #[test]
    fn test_world_entity_management() {
        let mut world = World::new();

        let planet_id = world.add_planet(Planet::new(
            Vec2::new(0.0, 0.0),
            100.0,
            10000.0,
            BLUE,
        ));

        let rocket_id = world.add_rocket(Rocket::new(
            Vec2::new(200.0, 0.0),
            Vec2::new(0.0, 0.0),
            WHITE,
            1.0,
        ));

        assert_eq!(world.planet_count(), 1);
        assert_eq!(world.rocket_count(), 1);
        assert!(world.get_planet(planet_id).is_some());
        assert!(world.get_rocket(rocket_id).is_some());
    }

    #[test]
    fn test_active_rocket_management() {
        let mut world = World::new();

        let rocket1_id = world.add_rocket(Rocket::new(
            Vec2::new(0.0, 0.0),
            Vec2::new(0.0, 0.0),
            WHITE,
            1.0,
        ));

        // First rocket should be active
        assert_eq!(world.active_rocket_id(), Some(rocket1_id));

        let rocket2_id = world.add_rocket(Rocket::new(
            Vec2::new(100.0, 0.0),
            Vec2::new(0.0, 0.0),
            RED,
            1.0,
        ));

        // Still the first rocket
        assert_eq!(world.active_rocket_id(), Some(rocket1_id));

        // Change active rocket
        world.set_active_rocket(Some(rocket2_id));
        assert_eq!(world.active_rocket_id(), Some(rocket2_id));
    }

    #[test]
    fn test_rocket_to_satellite_conversion() {
        let mut world = World::new();

        let rocket_id = world.add_rocket(Rocket::new(
            Vec2::new(0.0, 0.0),
            Vec2::new(0.0, 0.0),
            WHITE,
            1.0,
        ));

        let satellite_id = world.convert_rocket_to_satellite(rocket_id);
        assert!(satellite_id.is_some());
        assert_eq!(world.rocket_count(), 0);
        assert_eq!(world.satellite_count(), 1);
    }
}
