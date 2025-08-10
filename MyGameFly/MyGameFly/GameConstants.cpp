#include "GameConstants.h"
#include "Planet.h"

namespace GameConstants {

    // Helper function to create a ring of identical planets
    std::vector<PlanetConfig> createPlanetRing(
        int numPlanets,           // How many planets in the ring
        float massRatio,          // Mass of each planet (as ratio of main)
        float orbitDistance,      // Distance from center (multiplier)
        sf::Color color,          // Color of all planets
        float startAngle = 0.0f)  // Starting angle offset (optional)
    {
        std::vector<PlanetConfig> ringPlanets;

        // Calculate angle between each planet for even spacing
        float angleIncrement = (2.0f * GameConstants::PI) / numPlanets;

        for (int i = 0; i < numPlanets; i++) {
            float angle = startAngle + (angleIncrement * i);
            PlanetConfig planet(massRatio, orbitDistance, angle, color);
            ringPlanets.push_back(planet);
        }

        return ringPlanets;
    }

    std::vector<PlanetConfig> getPlanetConfigurations() {
        std::vector<PlanetConfig> configs;

        // Green planet
        PlanetConfig green(0.06f, 2.6f, 0.0f, sf::Color::Green);
        green.moons.push_back(MoonConfig(0.01f, 2.5f, 0.0f, sf::Color(200, 200, 255)));        // Large blue-white moon
        //green.moons.push_back(MoonConfig(0.015f, 4.0f, GameConstants::PI * 0.5f, sf::Color(255, 200, 200))); // Medium pink moon
        //green.moons.push_back(MoonConfig(0.008f, 5.5f, GameConstants::PI, sf::Color(200, 255, 200)));        // Small green moon

        configs.push_back(green);

        // Magenta planet
        PlanetConfig magenta(0.014f, 1.9f, GameConstants::PI / 3, sf::Color::Magenta);

        //magenta.moons.push_back(MoonConfig(0.01f, 1.5f, 0.0f, sf::Color(200, 200, 255)));
        configs.push_back(magenta);

        // Pink planet
        PlanetConfig pink(0.0065f, 0.6f, 0.0f, sf::Color(255, 192, 203));

        //pink.moons.push_back(MoonConfig(0.01f, 1.5f, 0.0f, sf::Color(200, 200, 255)));
        configs.push_back(pink);

        // Cyan planet - MASSIVE with MOONS!
        //PlanetConfig cyan(0.6f, 5.0f, 0.0f, sf::Color::Cyan);

        //// Add 3 moons to the cyan planet (it's massive enough to hold them)
        //cyan.moons.push_back(MoonConfig(0.03f, 2.5f, 0.0f, sf::Color(200, 200, 255)));        // Large blue-white moon
        //cyan.moons.push_back(MoonConfig(0.015f, 4.0f, GameConstants::PI * 0.5f, sf::Color(255, 200, 200))); // Medium pink moon
        //cyan.moons.push_back(MoonConfig(0.008f, 5.5f, GameConstants::PI, sf::Color(200, 255, 200)));        // Small green moon

        //configs.push_back(cyan);

        //PlanetConfig pink2(0.0065f, 10.0f, 0.0f, sf::Color(255, 192, 203));
        //configs.push_back(pink2);
        //PlanetConfig pink3(0.0065f, 10.0f, 0.5f, sf::Color(255, 192, 203));
        //configs.push_back(pink3);
        //PlanetConfig pink4(0.0065f, 10.0f, 1.0f, sf::Color(255, 192, 203));
        //configs.push_back(pink4);

        //auto outerRing = createPlanetRing(
        //    8,                              // 8 planets in the ring
        //    0.065f,                        // Small mass
        //    10.0f,                          // Far out
        //    sf::Color(255, 192, 203),       // Pink color
        //    0.0f                            // Start at angle 0
        //);

        //// Add all ring planets to configs
        //for (const auto& planet : outerRing) {
        //    configs.push_back(planet);
        //}

        return configs;
    }
}
// PLANET CONFIGURATION EXPLAINED:
// {massRatio, orbitDistanceMultiplier, angleOffset, color}

//{ 0.06f,    1.0f,     0.0f,     sf::Color::Green},
//                            
//  |        |         |         |
//  |        |         |         ??? Planet color (visual appearance)
//  |        |         ??? Starting angle in orbit (radians)
//  |        |             0.0f = right side of central planet
//  |        |             PI/2 = bottom, PI = left, 3*PI/2 = top
//  |        ??? Distance from central planet multiplier
//  |            1.0f = same as base orbit distance
//  |            1.5f = 50 further out, 2.0f = twice as far
//  ??? Mass as percentage of central planet
//      0.06f = 6 of main planet mass
//      0.04f = 4 of main planet mass (smaller planet)
//      0.1f = 10 of main planet mass (larger planet)

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