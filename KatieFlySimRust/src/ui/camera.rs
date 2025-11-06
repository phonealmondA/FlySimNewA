// Camera - View management with zoom and pan
// Camera system for following entities and zooming

use sfml::graphics::View;
use sfml::system::Vector2f;

/// Camera for managing the game view
pub struct Camera {
    view: View,
    zoom_level: f32,
    target_zoom: f32,
    target_center: Vector2f,
    zoom_speed: f32,
    follow_smoothing: f32,
}

impl Camera {
    pub fn new(window_size: Vector2f) -> Self {
        let mut view = View::new(
            Vector2f::new(window_size.x / 2.0, window_size.y / 2.0),
            window_size,
        );

        Camera {
            view,
            zoom_level: 1.0,
            target_zoom: 1.0,
            target_center: Vector2f::new(window_size.x / 2.0, window_size.y / 2.0),
            zoom_speed: 5.0,
            follow_smoothing: 0.1,
        }
    }

    /// Update camera (smooth zoom and follow)
    pub fn update(&mut self, delta_time: f32) {
        // Smooth zoom
        if (self.zoom_level - self.target_zoom).abs() > 0.01 {
            let zoom_delta = (self.target_zoom - self.zoom_level) * self.zoom_speed * delta_time;
            self.zoom_level += zoom_delta;
            self.view.zoom(1.0 + zoom_delta / self.zoom_level);
        }

        // Smooth follow
        let current_center = self.view.center();
        let center_delta = self.target_center - current_center;

        if center_delta.x.abs() > 0.1 || center_delta.y.abs() > 0.1 {
            let smooth_delta = center_delta * self.follow_smoothing;
            self.view
                .set_center(current_center + smooth_delta);
        }
    }

    /// Set target zoom level
    pub fn set_target_zoom(&mut self, zoom: f32) {
        self.target_zoom = zoom.max(0.1).min(10.0); // Clamp zoom
    }

    /// Adjust zoom by a delta
    pub fn adjust_zoom(&mut self, delta: f32) {
        self.set_target_zoom(self.target_zoom + delta);
    }

    /// Set center position (instant)
    pub fn set_center(&mut self, center: Vector2f) {
        self.target_center = center;
        self.view.set_center(center);
    }

    /// Set target center (smooth follow)
    pub fn set_target_center(&mut self, center: Vector2f) {
        self.target_center = center;
    }

    /// Follow an entity position
    pub fn follow(&mut self, position: Vector2f) {
        self.set_target_center(position);
    }

    /// Get the current view
    pub fn view(&self) -> &View {
        &self.view
    }

    /// Get zoom level
    pub fn zoom_level(&self) -> f32 {
        self.zoom_level
    }

    /// Reset camera to default
    pub fn reset(&mut self, window_size: Vector2f) {
        self.zoom_level = 1.0;
        self.target_zoom = 1.0;
        let center = Vector2f::new(window_size.x / 2.0, window_size.y / 2.0);
        self.target_center = center;
        self.view.set_center(center);
        self.view.set_size(window_size);
    }

    /// Handle window resize
    pub fn handle_resize(&mut self, new_size: Vector2f) {
        self.view.set_size(new_size * self.zoom_level);
    }

    /// Convert screen coordinates to world coordinates
    pub fn screen_to_world(&self, screen_pos: Vector2f, window_size: Vector2f) -> Vector2f {
        let center = self.view.center();
        let view_size = self.view.size();

        // Convert screen coordinates to normalized device coordinates [-1, 1]
        let ndc_x = (screen_pos.x / window_size.x) * 2.0 - 1.0;
        let ndc_y = (screen_pos.y / window_size.y) * 2.0 - 1.0;

        // Convert to world coordinates
        Vector2f::new(
            center.x + ndc_x * view_size.x / 2.0,
            center.y + ndc_y * view_size.y / 2.0,
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_camera_creation() {
        let camera = Camera::new(Vector2f::new(1920.0, 1080.0));
        assert_eq!(camera.zoom_level(), 1.0);
    }

    #[test]
    fn test_zoom_clamping() {
        let mut camera = Camera::new(Vector2f::new(1920.0, 1080.0));

        camera.set_target_zoom(20.0); // Too high
        assert_eq!(camera.target_zoom, 10.0);

        camera.set_target_zoom(0.01); // Too low
        assert_eq!(camera.target_zoom, 0.1);
    }

    #[test]
    fn test_follow() {
        let mut camera = Camera::new(Vector2f::new(1920.0, 1080.0));
        let target_pos = Vector2f::new(500.0, 300.0);

        camera.follow(target_pos);
        assert_eq!(camera.target_center, target_pos);
    }
}
