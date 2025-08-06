#include "GameConstants.h"
#include "Planet.h"

namespace GameConstants {
    std::vector<PlanetConfig> getPlanetConfigurations() {
        return {
            {0.06f, 1.0f, 0.0f, sf::Color::Green},                        // Current green planet
            {0.04f, 1.5f, GameConstants::PI / 3, sf::Color::Red},           // Smaller red planet, further out
            {0.025f, 2.2f, GameConstants::PI, sf::Color::Yellow},         // Even smaller yellow planet
            // Add more planets here as needed:
            // {0.015f, 3.0f, GameConstants::PI * 1.5f, sf::Color::Cyan},   // Tiny cyan planet, very far out
            // {0.035f, 2.8f, GameConstants::PI * 0.5f, sf::Color::Magenta} // Small magenta planet
        };
    }
}