#include "GameObject.h"

GameObject::GameObject(sf::Vector2f pos, sf::Vector2f vel, sf::Color col)
    : position(pos), velocity(vel), color(col)
{
}

sf::Vector2f GameObject::getPosition() const
{
    return position;
}

sf::Vector2f GameObject::getVelocity() const
{
    return velocity;
}

void GameObject::setVelocity(sf::Vector2f vel)
{
    velocity = vel;
}
void GameObject::setPosition(sf::Vector2f pos)
{
    position = pos;
}