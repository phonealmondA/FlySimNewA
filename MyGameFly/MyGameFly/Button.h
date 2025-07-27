#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

class Button {
private:
    sf::RectangleShape shape;
    // sf::Text text;  // Comment out for now
    std::function<void()> callback;
    bool isHovered;

public:
    Button(const sf::Vector2f& position, const sf::Vector2f& size,
        const std::string& buttonText, const sf::Font& font,
        std::function<void()> clickCallback);

    void update(const sf::Vector2f& mousePosition);
    void handleClick();
    void draw(sf::RenderWindow& window);
    bool contains(const sf::Vector2f& point) const;
};