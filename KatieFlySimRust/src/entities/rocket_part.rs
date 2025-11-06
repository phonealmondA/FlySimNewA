// RocketPart - Base for rocket components
// Ported from C++ RocketPart class

use macroquad::prelude::*;

/// Trait for rocket components
pub trait RocketPart {
    /// Draw the part relative to rocket position and rotation
    fn draw(&self, rocket_pos: Vec2, rotation: f32, scale: f32);

    /// Get the relative position of this part
    fn relative_position(&self) -> Vec2;

    /// Get the color of this part
    fn color(&self) -> Color;
}

/// Base rocket part data
#[derive(Debug, Clone)]
pub struct RocketPartData {
    pub relative_position: Vec2,
    pub color: Color,
}

impl RocketPartData {
    pub fn new(relative_position: Vec2, color: Color) -> Self {
        RocketPartData {
            relative_position,
            color,
        }
    }
}
