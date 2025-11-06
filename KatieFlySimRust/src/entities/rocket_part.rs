// RocketPart - Base for rocket components
// Ported from C++ RocketPart class

use sfml::graphics::{Color, RenderWindow};
use sfml::system::Vector2f;

/// Trait for rocket components
pub trait RocketPart {
    /// Draw the part relative to rocket position and rotation
    fn draw(&self, window: &mut RenderWindow, rocket_pos: Vector2f, rotation: f32, scale: f32);

    /// Get the relative position of this part
    fn relative_position(&self) -> Vector2f;

    /// Get the color of this part
    fn color(&self) -> Color;
}

/// Base rocket part data
#[derive(Debug, Clone)]
pub struct RocketPartData {
    pub relative_position: Vector2f,
    pub color: Color,
}

impl RocketPartData {
    pub fn new(relative_position: Vector2f, color: Color) -> Self {
        RocketPartData {
            relative_position,
            color,
        }
    }
}
