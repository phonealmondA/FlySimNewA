// Button - UI button component
// Ported from C++ Button class

use sfml::graphics::{
    Color, Font, RectangleShape, RenderTarget, RenderWindow, Shape, Text, Transformable,
};
use sfml::system::Vector2f;
use sfml::window::mouse;

/// Simple UI button
pub struct Button<'a> {
    shape: RectangleShape<'static>,
    text: Text<'a>,
    position: Vector2f,
    size: Vector2f,
    is_hovered: bool,
    is_pressed: bool,
    normal_color: Color,
    hover_color: Color,
    press_color: Color,
}

impl<'a> Button<'a> {
    pub fn new(
        position: Vector2f,
        size: Vector2f,
        text: &str,
        font: &'a Font,
        normal_color: Color,
    ) -> Self {
        let mut shape = RectangleShape::with_size(size);
        shape.set_position(position);
        shape.set_fill_color(normal_color);
        shape.set_outline_color(Color::WHITE);
        shape.set_outline_thickness(2.0);

        let mut text_obj = Text::new(text, font, 20);
        text_obj.set_fill_color(Color::WHITE);

        // Center text in button
        let text_bounds = text_obj.local_bounds();
        text_obj.set_origin(Vector2f::new(text_bounds.width / 2.0, text_bounds.height / 2.0));
        text_obj.set_position(Vector2f::new(
            position.x + size.x / 2.0,
            position.y + size.y / 2.0 - 5.0, // Slight offset for better visual centering
        ));

        let hover_color = Color::rgb(
            normal_color.r.saturating_add(30),
            normal_color.g.saturating_add(30),
            normal_color.b.saturating_add(30),
        );

        let press_color = Color::rgb(
            normal_color.r.saturating_sub(30),
            normal_color.g.saturating_sub(30),
            normal_color.b.saturating_sub(30),
        );

        Button {
            shape,
            text: text_obj,
            position,
            size,
            is_hovered: false,
            is_pressed: false,
            normal_color,
            hover_color,
            press_color,
        }
    }

    /// Update button state based on mouse position and clicks
    pub fn update(&mut self, window: &RenderWindow, mouse_pressed: bool) -> bool {
        let mouse_pos = mouse::position_f(window);
        let mouse_pos_vec = Vector2f::new(mouse_pos.x, mouse_pos.y);

        // Check if mouse is over button
        let was_hovered = self.is_hovered;
        self.is_hovered = self.contains_point(mouse_pos_vec);

        // Update visual state
        if self.is_hovered {
            if mouse_pressed {
                self.shape.set_fill_color(self.press_color);
                self.is_pressed = true;
            } else {
                self.shape.set_fill_color(self.hover_color);

                // Button was clicked (released over button after being pressed)
                if self.is_pressed {
                    self.is_pressed = false;
                    return true; // Button clicked!
                }
            }
        } else {
            self.shape.set_fill_color(self.normal_color);
            self.is_pressed = false;
        }

        false
    }

    /// Check if a point is inside the button
    fn contains_point(&self, point: Vector2f) -> bool {
        point.x >= self.position.x
            && point.x <= self.position.x + self.size.x
            && point.y >= self.position.y
            && point.y <= self.position.y + self.size.y
    }

    /// Draw the button
    pub fn draw(&self, window: &mut RenderWindow) {
        window.draw(&self.shape);
        window.draw(&self.text);
    }

    /// Set button text
    pub fn set_text(&mut self, text: &str) {
        self.text.set_string(text);

        // Re-center text
        let text_bounds = self.text.local_bounds();
        self.text.set_origin(Vector2f::new(text_bounds.width / 2.0, text_bounds.height / 2.0));
        self.text.set_position(Vector2f::new(
            self.position.x + self.size.x / 2.0,
            self.position.y + self.size.y / 2.0 - 5.0,
        ));
    }

    pub fn is_hovered(&self) -> bool {
        self.is_hovered
    }
}
