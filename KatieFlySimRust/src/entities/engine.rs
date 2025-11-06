// Engine - Rocket engine component
// Ported from C++ Engine class

use macroquad::prelude::*;

use super::rocket_part::{RocketPart, RocketPartData};
use crate::utils::vector_helper;

/// Rocket engine providing thrust
pub struct Engine {
    data: RocketPartData,
    thrust: f32,
}

impl Engine {
    pub fn new(relative_pos: Vec2, thrust_power: f32, color: Color) -> Self {
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
    fn draw(&self, rocket_pos: Vec2, rotation: f32, scale: f32) {
        // Calculate world position
        let rotated_offset = vector_helper::rotate(self.data.relative_position, rotation);
        let world_pos = rocket_pos + rotated_offset;

        // Engine triangle points (local coordinates)
        let local_points = [
            Vec2::new(0.0, -5.0 * scale),
            Vec2::new(-3.0 * scale, 5.0 * scale),
            Vec2::new(3.0 * scale, 5.0 * scale),
        ];

        // Rotate and translate points to world space
        let cos_r = rotation.cos();
        let sin_r = rotation.sin();

        let world_points: Vec<Vec2> = local_points.iter().map(|p| {
            let rotated_x = p.x * cos_r - p.y * sin_r;
            let rotated_y = p.x * sin_r + p.y * cos_r;
            world_pos + Vec2::new(rotated_x, rotated_y)
        }).collect();

        // Draw triangle
        draw_triangle(
            world_points[0],
            world_points[1],
            world_points[2],
            self.data.color,
        );
    }

    fn relative_position(&self) -> Vec2 {
        self.data.relative_position
    }

    fn color(&self) -> Color {
        self.data.color
    }
}
