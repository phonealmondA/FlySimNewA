#pragma once
#include <SFML/System/Vector2.hpp>
#include <cmath>

// Rest of the code remains the same

// Vector2 helper functions
inline sf::Vector2f normalize(const sf::Vector2f& source) {
    float length = std::sqrt(source.x * source.x + source.y * source.y);
    if (length != 0)
        return source / length;
    return source;
}

inline float distance(const sf::Vector2f& a, const sf::Vector2f& b) {
    sf::Vector2f diff = b - a;
    return std::sqrt(diff.x * diff.x + diff.y * diff.y);
}