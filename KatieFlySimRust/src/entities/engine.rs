// Engine - Rocket engine component
// Ported from C++ Engine class

use sfml::graphics::{Color, ConvexShape, RenderTarget, RenderWindow, Shape, Transformable};
use sfml::system::Vector2f;

use super::rocket_part::{RocketPart, RocketPartData};
use crate::utils::vector_helper;

/// Rocket engine providing thrust
pub struct Engine {
    data: RocketPartData,
    thrust: f32,
}

impl Engine {
    pub fn new(relative_pos: Vector2f, thrust_power: f32, color: Color) -> Self {
        Engine {
            data: RocketPartData::new(relative_pos, color),
            thrust: thrust_power,
        }
    }

    pub fn thrust(&self) -> f32 {
        self.thrust
    }
}

impl RocketPart for Engine {
    fn draw(&self, window: &mut RenderWindow, rocket_pos: Vector2f, rotation: f32, scale: f32) {
        // Create engine shape (small triangle)
        let mut shape = ConvexShape::new(3);

        // Engine triangle points
        shape.set_point(0, Vector2f::new(0.0, -5.0 * scale));
        shape.set_point(1, Vector2f::new(-3.0 * scale, 5.0 * scale));
        shape.set_point(2, Vector2f::new(3.0 * scale, 5.0 * scale));

        // Apply rocket color
        shape.set_fill_color(self.data.color);

        // Calculate world position
        let rotated_offset = vector_helper::rotate(self.data.relative_position, rotation);
        let world_pos = rocket_pos + rotated_offset;

        // Set position and rotation
        shape.set_position(world_pos);
        shape.set_rotation(rotation * 180.0 / std::f32::consts::PI); // Convert to degrees

        // Draw
        window.draw(&shape);
    }

    fn relative_position(&self) -> Vector2f {
        self.data.relative_position
    }

    fn color(&self) -> Color {
        self.data.color
    }
}
