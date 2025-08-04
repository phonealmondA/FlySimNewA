#pragma once
#include <SFML/Graphics.hpp>

class GameObject {
protected:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;

public:
    GameObject(sf::Vector2f pos, sf::Vector2f vel, sf::Color col);
    virtual ~GameObject() = default;

    virtual void update(float deltaTime) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;

    sf::Vector2f getPosition() const;
    sf::Vector2f getVelocity() const;
    void setVelocity(sf::Vector2f vel);
    virtual void setPosition(sf::Vector2f pos);
};