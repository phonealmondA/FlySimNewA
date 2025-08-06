#include "GameConstants.h"
#include "Planet.h"

namespace GameConstants {
    std::vector<PlanetConfig> getPlanetConfigurations() {
        return {
            {0.06f, 3.0f, 0.0f, sf::Color::Green},                        // Current green planet
            {0.004f, 1.2f, GameConstants::PI / 3, sf::Color::Magenta},           // Smaller Magenta planet, closer
            {0.0025f, 0.4f, GameConstants::PI, sf::Color(255, 192, 203)},         // Even smaller Pink planet closest
            // Add more planets here as needed:
            // {0.015f, 3.0f, GameConstants::PI * 1.5f, sf::Color::Cyan},   // Tiny cyan planet, very far out
            // {0.035f, 2.8f, GameConstants::PI * 0.5f, sf::Color::Magenta} // Small magenta planet
        };
    }
}
// PLANET CONFIGURATION EXPLAINED:
// {massRatio, orbitDistanceMultiplier, angleOffset, color}

//{ 0.06f,    1.0f,     0.0f,     sf::Color::Green},
//  ^        ^         ^         ^
//  |        |         |         |
//  |        |         |         ??? Planet color (visual appearance)
//  |        |         ??? Starting angle in orbit (radians)
//  |        |             0.0f = right side of central planet
//  |        |             PI/2 = bottom, PI = left, 3*PI/2 = top
//  |        ??? Distance from central planet multiplier
//  |            1.0f = same as base orbit distance
//  |            1.5f = 50% further out, 2.0f = twice as far
//  ??? Mass as percentage of central planet
//      0.06f = 6% of main planet mass
//      0.04f = 4% of main planet mass (smaller planet)
//      0.1f = 10% of main planet mass (larger planet)

// EXAMPLES:
// Small close planet:    {0.03f, 0.8f, 0.0f, sf::Color::Blue}
// Large distant planet:  {0.08f, 2.5f, PI, sf::Color::Red}
// Tiny far planet:       {0.015f, 3.0f, PI/4, sf::Color::Yellow}

// AVAILABLE COLORS:
// sf::Color::Black
// sf::Color::White
// sf::Color::Red
// sf::Color::Green
// sf::Color::Blue
// sf::Color::Yellow
// sf::Color::Magenta
// sf::Color::Cyan

// CUSTOM RGB COLORS (Red, Green, Blue values 0-255):
// sf::Color(255, 0, 0)      // Pure red
// sf::Color(255, 165, 0)    // Orange
// sf::Color(128, 0, 128)    // Purple
// sf::Color(255, 192, 203)  // Pink
// sf::Color(139, 69, 19)    // Brown
// sf::Color(255, 215, 0)    // Gold
// sf::Color(192, 192, 192)  // Silver
// sf::Color(0, 255, 127)    // Spring green
// sf::Color(70, 130, 180)   // Steel blue
// sf::Color(220, 20, 60)    // Crimson
// sf::Color(255, 20, 147)   // Deep pink
// sf::Color(0, 191, 255)    // Deep sky blue
// sf::Color(50, 205, 50)    // Lime green
// sf::Color(255, 69, 0)     // Red orange
// sf::Color(148, 0, 211)    // Dark violet