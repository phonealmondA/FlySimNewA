#pragma once
#include "RocketPart.h"

class Engine : public RocketPart {
private:
    sf::ConvexShape shape;
    float thrust;

public:
    Engine(sf::Vector2f relPos, float thrustPower, sf::Color col = sf::Color(255, 100, 0));

    void draw(sf::RenderWindow& window, sf::Vector2f rocketPos, float rotation, float scale = 1.0f) override;
    float getThrust() const;
};