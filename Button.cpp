#include "Button.h"

Button::Button(const sf::Vector2f& position, const sf::Vector2f& size,
    const std::string& buttonText, const sf::Font& font,
    std::function<void()> clickCallback)
    : callback(clickCallback), isHovered(false), text(font)
{
    // Set up shape
    shape.setPosition(position);
    shape.setSize(size);
    shape.setFillColor(sf::Color(100, 100, 100, 200));
    shape.setOutlineColor(sf::Color::White);
    shape.setOutlineThickness(1.0f);

    // Set up text
    text.setString(buttonText);
    text.setCharacterSize(18);
    text.setFillColor(sf::Color::White);

    // Center the text in the button
    sf::FloatRect textBounds = text.getLocalBounds();
    text.setPosition(sf::Vector2f(
        position.x + (size.x - textBounds.size.x) / 2.0f - textBounds.position.x,
        position.y + (size.y - textBounds.size.y) / 2.0f - textBounds.position.y
    ));
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
    window.draw(text);  // Now we draw the text
}

bool Button::contains(const sf::Vector2f& point) const
{
    return shape.getGlobalBounds().contains(point);
}