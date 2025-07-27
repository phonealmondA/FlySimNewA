#include "Button.h"

Button::Button(const sf::Vector2f& position, const sf::Vector2f& size,
    const std::string& buttonText, const sf::Font& font,
    std::function<void()> clickCallback)
    : callback(clickCallback), isHovered(false)
{
    // Set up shape
    shape.setPosition(position);
    shape.setSize(size);
    shape.setFillColor(sf::Color(100, 100, 100, 200));
    shape.setOutlineColor(sf::Color::White);
    shape.setOutlineThickness(1.0f);

    // In SFML 3.0, we'll need to handle text differently
    // For now, skip text rendering - just use colored buttons
    // We can add text back once the basic UI is working
}

void Button::update(const sf::Vector2f& mousePosition)
{
    isHovered = contains(mousePosition);

    if (isHovered) {
        shape.setFillColor(sf::Color(150, 150, 150, 200));
    }
    else {
        shape.setFillColor(sf::Color(100, 100, 100, 200));
    }
}

void Button::handleClick()
{
    if (isHovered && callback) {
        callback();
    }
}

void Button::draw(sf::RenderWindow& window)
{
    window.draw(shape);
    // Skip text drawing for now
    // window.draw(text);
}

bool Button::contains(const sf::Vector2f& point) const
{
    return shape.getGlobalBounds().contains(point);
}