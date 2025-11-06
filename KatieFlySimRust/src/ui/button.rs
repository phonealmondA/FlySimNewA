// Button - UI button component
// Ported from C++ Button class

use macroquad::prelude::*;

/// Simple UI button
pub struct Button {
    text: String,
    position: Vec2,
    size: Vec2,
    is_hovered: bool,
    is_pressed: bool,
    normal_color: Color,
    hover_color: Color,
    press_color: Color,
    font_size: f32,
}

impl Button {
    pub fn new(
        position: Vec2,
        size: Vec2,
        text: &str,
        normal_color: Color,
    ) -> Self {
        let hover_color = Color::new(
            (normal_color.r + 0.12).min(1.0),
            (normal_color.g + 0.12).min(1.0),
            (normal_color.b + 0.12).min(1.0),
            normal_color.a,
        );

        let press_color = Color::new(
            (normal_color.r - 0.12).max(0.0),
            (normal_color.g - 0.12).max(0.0),
            (normal_color.b - 0.12).max(0.0),
            normal_color.a,
        );

        Button {
            text: text.to_string(),
            position,
            size,
            is_hovered: false,
            is_pressed: false,
            normal_color,
            hover_color,
            press_color,
            font_size: 20.0,
        }
    }

    /// Update button state based on mouse position and clicks
    pub fn update(&mut self, mouse_pressed: bool) -> bool {
        let mouse_pos = mouse_position();
        let mouse_pos_vec = Vec2::new(mouse_pos.0, mouse_pos.1);

        // Check if mouse is over button
        self.is_hovered = self.contains_point(mouse_pos_vec);

        // Update visual state and detect clicks
        if self.is_hovered {
            if mouse_pressed {
                self.is_pressed = true;
            } else {
                // Button was clicked (released over button after being pressed)
                if self.is_pressed {
                    self.is_pressed = false;
                    return true; // Button clicked!
                }
            }
        } else {
            self.is_pressed = false;
        }

        false
    }

    /// Check if a point is inside the button
    fn contains_point(&self, point: Vec2) -> bool {
        point.x >= self.position.x
            && point.x <= self.position.x + self.size.x
            && point.y >= self.position.y
            && point.y <= self.position.y + self.size.y
    }

    /// Draw the button
    pub fn draw(&self) {
        // Determine current color based on state
        let current_color = if self.is_pressed {
            self.press_color
        } else if self.is_hovered {
            self.hover_color
        } else {
            self.normal_color
        };

        // Draw button background
        draw_rectangle(
            self.position.x,
            self.position.y,
            self.size.x,
            self.size.y,
            current_color,
        );

        // Draw button outline
        draw_rectangle_lines(
            self.position.x,
            self.position.y,
            self.size.x,
            self.size.y,
            2.0,
            WHITE,
        );

        // Draw text centered in button
        let text_dims = measure_text(&self.text, None, self.font_size as u16, 1.0);
        let text_x = self.position.x + (self.size.x - text_dims.width) / 2.0;
        let text_y = self.position.y + (self.size.y - text_dims.height) / 2.0 + text_dims.height;

        draw_text(
            &self.text,
            text_x,
            text_y,
            self.font_size,
            WHITE,
        );
    }

    /// Set button text
    pub fn set_text(&mut self, text: &str) {
        self.text = text.to_string();
    }

    pub fn is_hovered(&self) -> bool {
        self.is_hovered
    }
}
