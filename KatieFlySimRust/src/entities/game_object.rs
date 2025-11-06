// GameObject trait - Base trait for all game entities
// Ported from C++ GameObject base class
// Now using macroquad for pure Rust graphics (no external dependencies!)

use macroquad::prelude::*;

/// Core game object trait that all entities implement
pub trait GameObject {
    /// Update the game object's state
    fn update(&mut self, delta_time: f32);

    /// Draw the game object (macroquad uses global rendering context)
    fn draw(&self);

    /// Get the position of the object
    fn position(&self) -> Vec2;

    /// Get the velocity of the object
    fn velocity(&self) -> Vec2;

    /// Set the velocity of the object
    fn set_velocity(&mut self, velocity: Vec2);

    /// Get the color of the object
    fn color(&self) -> Color;
}

/// Common game object data that most entities share
#[derive(Debug, Clone)]
pub struct GameObjectData {
    pub position: Vec2,
    pub velocity: Vec2,
    pub color: Color,
}

impl GameObjectData {
    pub fn new(position: Vec2, velocity: Vec2, color: Color) -> Self {
        GameObjectData {
            position,
            velocity,
            color,
        }
    }
}

impl Default for GameObjectData {
    fn default() -> Self {
        GameObjectData {
            position: Vec2::new(0.0, 0.0),
            velocity: Vec2::new(0.0, 0.0),
            color: WHITE,
        }
    }
}
