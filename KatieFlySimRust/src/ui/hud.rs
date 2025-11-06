// HUD - Heads-up display for game information
// Shows rocket stats, speed, altitude, fuel, etc.

use sfml::graphics::{
    Color, Font, RectangleShape, RenderTarget, RenderWindow, Shape, Text, Transformable,
};
use sfml::system::Vector2f;

use crate::entities::Rocket;

/// Heads-up display for showing game stats
pub struct Hud<'a> {
    font: &'a Font,
    position: Vector2f,
    background: RectangleShape<'static>,
}

impl<'a> Hud<'a> {
    pub fn new(font: &'a Font, position: Vector2f) -> Self {
        let mut background = RectangleShape::with_size(Vector2f::new(250.0, 150.0));
        background.set_position(position);
        background.set_fill_color(Color::rgba(0, 0, 0, 180));
        background.set_outline_color(Color::rgba(255, 255, 255, 100));
        background.set_outline_thickness(1.0);

        Hud {
            font,
            position,
            background,
        }
    }

    /// Draw HUD with rocket information
    pub fn draw_rocket_stats(&self, window: &mut RenderWindow, rocket: &Rocket) {
        // Draw background
        window.draw(&self.background);

        let mut y_offset = self.position.y + 10.0;
        let line_height = 25.0;

        // Velocity
        let velocity = rocket.velocity();
        let speed = (velocity.x * velocity.x + velocity.y * velocity.y).sqrt();
        self.draw_text(
            window,
            &format!("Speed: {:.1} m/s", speed),
            y_offset,
            Color::GREEN,
        );
        y_offset += line_height;

        // Fuel
        let fuel_percent = rocket.fuel_percentage();
        let fuel_color = if fuel_percent > 50.0 {
            Color::GREEN
        } else if fuel_percent > 20.0 {
            Color::YELLOW
        } else {
            Color::RED
        };
        self.draw_text(
            window,
            &format!("Fuel: {:.1}%", fuel_percent),
            y_offset,
            fuel_color,
        );
        y_offset += line_height;

        // Mass
        self.draw_text(
            window,
            &format!("Mass: {:.1} kg", rocket.mass()),
            y_offset,
            Color::CYAN,
        );
        y_offset += line_height;

        // Thrust
        let thrust_percent = rocket.thrust_level() * 100.0;
        self.draw_text(
            window,
            &format!("Thrust: {:.0}%", thrust_percent),
            y_offset,
            if thrust_percent > 0.0 {
                Color::rgb(255, 165, 0) // Orange
            } else {
                Color::WHITE
            },
        );
        y_offset += line_height;

        // Rotation
        let rotation_deg = rocket.rotation() * 180.0 / std::f32::consts::PI;
        self.draw_text(
            window,
            &format!("Heading: {:.0}°", rotation_deg),
            y_offset,
            Color::WHITE,
        );
    }

    /// Draw text at a specific y position
    fn draw_text(&self, window: &mut RenderWindow, text: &str, y: f32, color: Color) {
        let mut text_obj = Text::new(text, self.font, 16);
        text_obj.set_position(Vector2f::new(self.position.x + 10.0, y));
        text_obj.set_fill_color(color);
        window.draw(&text_obj);
    }

    /// Draw simple text overlay (for no active rocket)
    pub fn draw_message(&self, window: &mut RenderWindow, message: &str) {
        window.draw(&self.background);
        self.draw_text(
            window,
            message,
            self.position.y + 65.0, // Center vertically
            Color::WHITE,
        );
    }
}
