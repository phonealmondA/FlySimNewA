// Vector Helper Functions - Ported from C++ VectorHelper.h
// Utility functions for Vec2 operations (now using macroquad)

use macroquad::prelude::Vec2;

/// Calculate the magnitude (length) of a vector
#[inline]
pub fn magnitude(v: Vec2) -> f32 {
    (v.x * v.x + v.y * v.y).sqrt()
}

/// Normalize a vector (return unit vector in same direction)
/// If the vector has zero length, returns the original vector
#[inline]
pub fn normalize(source: Vec2) -> Vec2 {
    let length = magnitude(source);
    if length != 0.0 {
        Vec2::new(source.x / length, source.y / length)
    } else {
        source
    }
}

/// Calculate the distance between two points
#[inline]
pub fn distance(a: Vec2, b: Vec2) -> f32 {
    let diff = b - a;
    magnitude(diff)
}

/// Calculate the squared distance (faster, avoids sqrt)
/// Useful for distance comparisons where exact distance isn't needed
#[inline]
pub fn distance_squared(a: Vec2, b: Vec2) -> f32 {
    let diff = b - a;
    diff.x * diff.x + diff.y * diff.y
}

/// Calculate the dot product of two vectors
#[inline]
pub fn dot(a: Vec2, b: Vec2) -> f32 {
    a.x * b.x + a.y * b.y
}

/// Calculate the cross product magnitude (2D cross product returns scalar)
#[inline]
pub fn cross(a: Vec2, b: Vec2) -> f32 {
    a.x * b.y - a.y * b.x
}

/// Rotate a vector by an angle (in radians)
#[inline]
pub fn rotate(v: Vec2, angle_radians: f32) -> Vec2 {
    let cos_a = angle_radians.cos();
    let sin_a = angle_radians.sin();
    Vec2::new(
        v.x * cos_a - v.y * sin_a,
        v.x * sin_a + v.y * cos_a,
    )
}

/// Linear interpolation between two vectors
#[inline]
pub fn lerp(a: Vec2, b: Vec2, t: f32) -> Vec2 {
    Vec2::new(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
    )
}

/// Clamp vector magnitude to a maximum value
#[inline]
pub fn clamp_magnitude(v: Vec2, max_magnitude: f32) -> Vec2 {
    let mag = magnitude(v);
    if mag > max_magnitude && mag > 0.0 {
        Vec2::new(
            v.x * max_magnitude / mag,
            v.y * max_magnitude / mag,
        )
    } else {
        v
    }
}

/// Get the angle of a vector in radians
#[inline]
pub fn angle(v: Vec2) -> f32 {
    v.y.atan2(v.x)
}

/// Get the angle between two vectors in radians
#[inline]
pub fn angle_between(a: Vec2, b: Vec2) -> f32 {
    let dot_product = dot(a, b);
    let mag_product = magnitude(a) * magnitude(b);
    if mag_product > 0.0 {
        (dot_product / mag_product).acos()
    } else {
        0.0
    }
}

/// Project vector a onto vector b
#[inline]
pub fn project(a: Vec2, b: Vec2) -> Vec2 {
    let b_mag_sq = b.x * b.x + b.y * b.y;
    if b_mag_sq > 0.0 {
        let scalar = dot(a, b) / b_mag_sq;
        Vec2::new(b.x * scalar, b.y * scalar)
    } else {
        Vec2::new(0.0, 0.0)
    }
}

/// Reflect vector v across normal n
#[inline]
pub fn reflect(v: Vec2, n: Vec2) -> Vec2 {
    let normal = normalize(n);
    let dot_product = dot(v, normal);
    Vec2::new(
        v.x - 2.0 * dot_product * normal.x,
        v.y - 2.0 * dot_product * normal.y,
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    use approx::assert_relative_eq;

    #[test]
    fn test_magnitude() {
        let v = Vec2::new(3.0, 4.0);
        assert_relative_eq!(magnitude(v), 5.0, epsilon = 0.0001);
    }

    #[test]
    fn test_normalize() {
        let v = Vec2::new(3.0, 4.0);
        let normalized = normalize(v);
        assert_relative_eq!(magnitude(normalized), 1.0, epsilon = 0.0001);
    }

    #[test]
    fn test_normalize_zero_vector() {
        let v = Vec2::new(0.0, 0.0);
        let normalized = normalize(v);
        assert_eq!(normalized.x, 0.0);
        assert_eq!(normalized.y, 0.0);
    }

    #[test]
    fn test_distance() {
        let a = Vec2::new(0.0, 0.0);
        let b = Vec2::new(3.0, 4.0);
        assert_relative_eq!(distance(a, b), 5.0, epsilon = 0.0001);
    }

    #[test]
    fn test_distance_squared() {
        let a = Vec2::new(0.0, 0.0);
        let b = Vec2::new(3.0, 4.0);
        assert_relative_eq!(distance_squared(a, b), 25.0, epsilon = 0.0001);
    }

    #[test]
    fn test_dot_product() {
        let a = Vec2::new(1.0, 0.0);
        let b = Vec2::new(0.0, 1.0);
        assert_relative_eq!(dot(a, b), 0.0, epsilon = 0.0001);

        let c = Vec2::new(1.0, 0.0);
        let d = Vec2::new(1.0, 0.0);
        assert_relative_eq!(dot(c, d), 1.0, epsilon = 0.0001);
    }

    #[test]
    fn test_rotate() {
        use std::f32::consts::PI;
        let v = Vec2::new(1.0, 0.0);
        let rotated = rotate(v, PI / 2.0); // 90 degrees
        assert_relative_eq!(rotated.x, 0.0, epsilon = 0.0001);
        assert_relative_eq!(rotated.y, 1.0, epsilon = 0.0001);
    }

    #[test]
    fn test_lerp() {
        let a = Vec2::new(0.0, 0.0);
        let b = Vec2::new(10.0, 10.0);
        let mid = lerp(a, b, 0.5);
        assert_relative_eq!(mid.x, 5.0, epsilon = 0.0001);
        assert_relative_eq!(mid.y, 5.0, epsilon = 0.0001);
    }

    #[test]
    fn test_clamp_magnitude() {
        let v = Vec2::new(3.0, 4.0); // magnitude 5.0
        let clamped = clamp_magnitude(v, 2.0);
        assert_relative_eq!(magnitude(clamped), 2.0, epsilon = 0.0001);
    }

    #[test]
    fn test_angle() {
        use std::f32::consts::PI;
        let v = Vec2::new(1.0, 0.0);
        assert_relative_eq!(angle(v), 0.0, epsilon = 0.0001);

        let v2 = Vec2::new(0.0, 1.0);
        assert_relative_eq!(angle(v2), PI / 2.0, epsilon = 0.0001);
    }
}
