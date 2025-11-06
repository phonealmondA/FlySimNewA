// HUD - Heads-up display for game information
// Shows rocket stats, speed, altitude, fuel, etc.

use macroquad::prelude::*;

use crate::entities::{GameObject, Rocket};

/// Heads-up display for showing game stats
pub struct Hud {
    position: Vec2,
    bg_size: Vec2,
    font_size: f32,
}

impl Hud {
    pub fn new(position: Vec2) -> Self {
        Hud {
            position,
            bg_size: Vec2::new(250.0, 150.0),
            font_size: 16.0,
        }
    }

    /// Draw HUD with rocket information
    pub fn draw_rocket_stats(&self, rocket: &Rocket) {
        // Draw background
        draw_rectangle(
            self.position.x,
            self.position.y,
            self.bg_size.x,
            self.bg_size.y,
            Color::new(0.0, 0.0, 0.0, 0.7),
        );

        // Draw border
        draw_rectangle_lines(
            self.position.x,
            self.position.y,
            self.bg_size.x,
            self.bg_size.y,
            1.0,
            Color::new(1.0, 1.0, 1.0, 0.4),
        );

        let mut y_offset = self.position.y + 10.0;
        let line_height = 25.0;

        // Velocity
        let velocity = rocket.velocity();
        let speed = (velocity.x * velocity.x + velocity.y * velocity.y).sqrt();
        self.draw_text(
            &format!("Speed: {:.1} m/s", speed),
            y_offset,
            GREEN,
        );
        y_offset += line_height;

        // Fuel
        let fuel_percent = rocket.fuel_percentage();
        let fuel_color = if fuel_percent > 50.0 {
            GREEN
        } else if fuel_percent > 20.0 {
            YELLOW
        } else {
            RED
        };
        self.draw_text(
            &format!("Fuel: {:.1}%", fuel_percent),
            y_offset,
            fuel_color,
        );
        y_offset += line_height;

        // Mass
        self.draw_text(
            &format!("Mass: {:.1} kg", rocket.mass()),
            y_offset,
            Color::new(0.0, 1.0, 1.0, 1.0), // CYAN
        );
        y_offset += line_height;

        // Thrust
        let thrust_percent = rocket.thrust_level() * 100.0;
        self.draw_text(
            &format!("Thrust: {:.0}%", thrust_percent),
            y_offset,
            if thrust_percent > 0.0 {
                Color::new(1.0, 0.65, 0.0, 1.0) // Orange
            } else {
                WHITE
            },
        );
        y_offset += line_height;

        // Rotation
        let rotation_deg = rocket.rotation() * 180.0 / std::f32::consts::PI;
        self.draw_text(
            &format!("Heading: {:.0}°", rotation_deg),
            y_offset,
            WHITE,
        );
    }

    /// Draw text at a specific y position
    fn draw_text(&self, text: &str, y: f32, color: Color) {
        draw_text(
            text,
            self.position.x + 10.0,
            y + self.font_size,
            self.font_size,
            color,
        );
    }

    /// Draw simple text overlay (for no active rocket)
    pub fn draw_message(&self, message: &str) {
        // Draw background
        draw_rectangle(
            self.position.x,
            self.position.y,
            self.bg_size.x,
            self.bg_size.y,
            Color::new(0.0, 0.0, 0.0, 0.7),
        );

        // Draw border
        draw_rectangle_lines(
            self.position.x,
            self.position.y,
            self.bg_size.x,
            self.bg_size.y,
            1.0,
            Color::new(1.0, 1.0, 1.0, 0.4),
        );

        self.draw_text(
            message,
            self.position.y + 65.0, // Center vertically
            WHITE,
        );
    }
}
