// GameObject trait - Base trait for all game entities
// Ported from C++ GameObject base class

use sfml::graphics::{Color, RenderWindow};
use sfml::system::Vector2f;

/// Core game object trait that all entities implement
pub trait GameObject {
    /// Update the game object's state
    fn update(&mut self, delta_time: f32);

    /// Draw the game object to the window
    fn draw(&self, window: &mut RenderWindow);

    /// Get the position of the object
    fn position(&self) -> Vector2f;

    /// Get the velocity of the object
    fn velocity(&self) -> Vector2f;

    /// Set the velocity of the object
    fn set_velocity(&mut self, velocity: Vector2f);

    /// Get the color of the object
    fn color(&self) -> Color;
}

/// Common game object data that most entities share
#[derive(Debug, Clone)]
pub struct GameObjectData {
    pub position: Vector2f,
    pub velocity: Vector2f,
    pub color: Color,
}

impl GameObjectData {
    pub fn new(position: Vector2f, velocity: Vector2f, color: Color) -> Self {
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
            position: Vector2f::new(0.0, 0.0),
            velocity: Vector2f::new(0.0, 0.0),
            color: Color::WHITE,
        }
    }
}
