#pragma once
#ifndef TEXTPANEL_H
#define TEXTPANEL_H

#include <SFML/Graphics.hpp>
#include <string>

class TextPanel {
private:
    sf::Text text;
    sf::RectangleShape background;

public:
    TextPanel(const sf::Font& font, unsigned int characterSize, sf::Vector2f position,
        sf::Vector2f size, sf::Color bgColor = sf::Color(0, 0, 0, 180));

    void setText(const std::string& str);
    void draw(sf::RenderWindow& window);

    // Getters and setters for customization
    void setPosition(sf::Vector2f position);
    void setSize(sf::Vector2f size);
    void setBackgroundColor(sf::Color color);
    void setTextColor(sf::Color color);
    void setCharacterSize(unsigned int size);

    sf::Vector2f getPosition() const;
    sf::Vector2f getSize() const;
    sf::Color getBackgroundColor() const;
    sf::Color getTextColor() const;
    unsigned int getCharacterSize() const;
};

#endif // TEXTPANEL_H