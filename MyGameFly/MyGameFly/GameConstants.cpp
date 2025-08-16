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

        // SOLAR SYSTEM TO SCALE
        // All masses and distances are scaled relative to the Sun (main planet)
        // Using real astronomical data scaled to work with your game's physics

        // MERCURY - Closest to the Sun
        PlanetConfig mercury(
            0.000165f,              // Mass ratio: Mercury is tiny compared to Sun
            0.4f,                   // 0.39 AU from Sun (closest planet)
            0.0f,                   // Starting at 0 degrees
            sf::Color(169, 169, 169) // Dark gray color
        );
        configs.push_back(mercury);

        // VENUS - The "morning star"
        PlanetConfig venus(
            0.00244f,               // Venus mass ratio to Sun
            0.7f,                   // 0.72 AU from Sun
            GameConstants::PI / 4,  // 45 degrees offset
            sf::Color(255, 165, 0)  // Orange-yellow (hot surface)
        );
        configs.push_back(venus);

        // EARTH - Our home planet with THE MOON
        PlanetConfig earth(
            0.003f,                 // Earth mass ratio to Sun
            1.0f,                   // 1.0 AU by definition
            GameConstants::PI / 2,  // 90 degrees offset
            sf::Color(0, 100, 200)  // Blue planet
        );
        // Add the Moon to Earth
        earth.moons.push_back(MoonConfig(
            0.012f,                 // Moon is about 1/81th Earth's mass (scaled up for visibility)
            3.0f,                   // Distance from Earth (scaled for gameplay)
            0.0f,                   // Starting angle
            sf::Color(220, 220, 220) // Light gray
        ));
        configs.push_back(earth);

        // MARS - The red planet with two tiny moons
        PlanetConfig mars(
            0.000323f,              // Mars mass ratio to Sun
            1.5f,                   // 1.52 AU from Sun
            GameConstants::PI,      // 180 degrees offset
            sf::Color(205, 92, 92)  // Red planet
        );
        // Phobos (larger moon)
        mars.moons.push_back(MoonConfig(
            0.002f,                 // Tiny moon (scaled up for visibility)
            2.0f,                   // Close to Mars
            0.0f,                   // Starting angle
            sf::Color(139, 69, 19)  // Brown color
        ));
        // Deimos (smaller, more distant moon)
        mars.moons.push_back(MoonConfig(
            0.001f,                 // Even tinier moon
            3.5f,                   // Farther from Mars
            GameConstants::PI,      // Opposite side
            sf::Color(160, 82, 45)  // Saddle brown
        ));
        configs.push_back(mars);

        // ASTEROID BELT - Between Mars and Jupiter (2.2 - 3.2 AU)
        // Create 8 asteroids scattered in the belt region
        auto asteroidBelt = createPlanetRing(
            8,                              // 8 asteroids
            0.00001f,                       // Very small mass (asteroid-sized)
            2.7f,                           // 2.7 AU from Sun (middle of asteroid belt)
            sf::Color(139, 69, 19),         // Brown/rocky color
            0.0f                            // Starting angle
        );

        // Add slight variations to make asteroids look more natural
        for (int i = 0; i < asteroidBelt.size(); i++) {
            // Vary the distances slightly (2.3 to 3.1 AU range)
            float distanceVariation = -0.4f + (0.8f * i / 7.0f); // -0.4 to +0.4
            asteroidBelt[i].orbitDistanceMultiplier = 2.7f + distanceVariation;

            // Vary the masses slightly
            asteroidBelt[i].massRatio = 0.00001f + (0.00002f * (i % 3) / 2.0f);

            // Vary colors between brown, dark gray, and reddish-brown
            if (i % 3 == 0) {
                asteroidBelt[i].color = sf::Color(139, 69, 19);   // Saddle brown
            }
            else if (i % 3 == 1) {
                asteroidBelt[i].color = sf::Color(105, 105, 105); // Dark gray
            }
            else {
                asteroidBelt[i].color = sf::Color(160, 82, 45);   // Saddle brown (reddish)
            }
        }

        // Add all asteroids to the configuration
        for (const auto& asteroid : asteroidBelt) {
            configs.push_back(asteroid);
        }

        // JUPITER - The gas giant with major moons
        PlanetConfig jupiter(
            0.954f,                 // Jupiter is massive! (95% of everything else combined)
            5.2f,                   // 5.2 AU from Sun
            3 * GameConstants::PI / 2, // 270 degrees offset
            sf::Color(255, 140, 0)  // Orange (Great Red Spot color)
        );
        // Io - volcanic moon
        jupiter.moons.push_back(MoonConfig(
            0.015f,                 // Slightly larger than Earth's moon
            2.5f,                   // Close orbit
            0.0f,
            sf::Color(255, 255, 0)  // Yellow (sulfur)
        ));
        // Europa - ice moon
        jupiter.moons.push_back(MoonConfig(
            0.008f,                 // Smaller than Io
            3.5f,                   // Next orbit out
            GameConstants::PI / 2,
            sf::Color(173, 216, 230) // Light blue (ice)
        ));
        // Ganymede - largest moon in solar system
        jupiter.moons.push_back(MoonConfig(
            0.025f,                 // Largest moon
            4.5f,                   // Farther out
            GameConstants::PI,
            sf::Color(139, 69, 19)  // Brown-gray
        ));
        // Callisto - heavily cratered
        jupiter.moons.push_back(MoonConfig(
            0.018f,                 // Second largest
            6.0f,                   // Outermost major moon
            3 * GameConstants::PI / 2,
            sf::Color(105, 105, 105) // Dark gray
        ));
        configs.push_back(jupiter);

        // SATURN - The ringed planet with Titan
        PlanetConfig saturn(
            0.286f,                 // Saturn mass ratio to Sun
            9.5f,                   // 9.5 AU from Sun
            GameConstants::PI / 6,  // 30 degrees offset
            sf::Color(250, 227, 139) // Pale yellow
        );
        // Titan - largest moon of Saturn
        saturn.moons.push_back(MoonConfig(
            0.023f,                 // Larger than Mercury!
            4.0f,                   // Distant orbit
            0.0f,
            sf::Color(205, 133, 63) // Orange (thick atmosphere)
        ));
        // Enceladus - ice geysers
        saturn.moons.push_back(MoonConfig(
            0.002f,                 // Small icy moon
            2.5f,                   // Closer orbit
            GameConstants::PI,
            sf::Color(255, 255, 255) // Pure white (fresh ice)
        ));
        configs.push_back(saturn);

        // URANUS - The ice giant (tilted on its side)
        PlanetConfig uranus(
            0.0436f,                // Uranus mass ratio to Sun
            19.2f,                  // 19.2 AU from Sun
            GameConstants::PI / 3,  // 60 degrees offset
            sf::Color(64, 224, 208) // Turquoise (methane atmosphere)
        );
        configs.push_back(uranus);

        // NEPTUNE - The windiest planet
        PlanetConfig neptune(
            0.0515f,                // Neptune mass ratio to Sun
            30.0f,                  // 30 AU from Sun
            5 * GameConstants::PI / 6, // 150 degrees offset
            sf::Color(0, 0, 139)    // Dark blue
        );
        // Triton - large retrograde moon
        neptune.moons.push_back(MoonConfig(
            0.02f,                  // Large moon
            3.5f,                   // Close orbit
            0.0f,
            sf::Color(176, 196, 222) // Light steel blue
        ));
        configs.push_back(neptune);

        // OPTIONAL: Add Pluto as a dwarf planet (uncomment if desired)
        /*
        PlanetConfig pluto(
            0.0000659f,             // Tiny mass ratio to Sun
            39.5f,                  // 39.5 AU from Sun (very far!)
            GameConstants::PI / 12, // 15 degrees offset
            sf::Color(139, 69, 19)  // Brown dwarf planet
        );
        // Charon - Pluto's large moon
        pluto.moons.push_back(MoonConfig(
            0.02f,                  // Large relative to Pluto
            2.0f,                   // Close orbit (tidally locked system)
            0.0f,
            sf::Color(105, 105, 105) // Dark gray
        ));
        configs.push_back(pluto);
        */

        return configs;
    }
}

/*
SOLAR SYSTEM SCALING NOTES:
=============================

This configuration creates a scientifically accurate solar system scaled to work with your game.

MASS SCALING:
- All planet masses are scaled relative to the Sun (your main planet)
- Jupiter is the most massive at 95.4% (it's almost a star!)
- Earth is only 0.3% of the Sun's mass
- Moons are scaled relative to their parent planets

DISTANCE SCALING:
- Uses Astronomical Units (AU) where Earth = 1.0 AU
- Mercury at 0.4 AU, Neptune at 30 AU
- Distances compressed slightly for gameplay (real Neptune would be tiny)

ORBITAL MECHANICS:
- Planets closer to the Sun orbit faster (Kepler's laws)
- Your existing orbital calculation will automatically handle this
- Each planet starts at a different angle for visual variety

ASTEROID BELT:
- 8 small rocky bodies between Mars (1.5 AU) and Jupiter (5.2 AU)
- Scattered between 2.3 and 3.1 AU (realistic asteroid belt range)
- Varied sizes, colors, and orbital distances for natural appearance
- Very low mass - won't significantly affect planetary orbits

MOON SYSTEMS:
- Earth: The Moon (our familiar companion)
- Mars: Phobos and Deimos (tiny potato-shaped moons)
- Jupiter: The four Galilean moons (Io, Europa, Ganymede, Callisto)
- Saturn: Titan and Enceladus (most interesting moons)
- Neptune: Triton (captured Kuiper Belt object)

COLORS:
- Based on real astronomical observations
- Mercury: Gray (no atmosphere)
- Venus: Orange (hot, thick atmosphere)
- Earth: Blue (oceans and atmosphere)
- Mars: Red (iron oxide surface)
- Jupiter: Orange (Great Red Spot)
- Saturn: Pale yellow (ammonia clouds)
- Uranus: Turquoise (methane atmosphere)
- Neptune: Dark blue (deep methane atmosphere)

GAMEPLAY NOTES:
- Inner planets are close together for interesting gravity interactions
- Asteroid belt provides navigation challenges and mining opportunities
- Outer planets are spaced for long-range exploration
- Moon systems provide orbital mechanics challenges
- Jupiter's gravity will dominate the outer solar system
- Earth provides a familiar reference point

To adjust the scale, modify the orbitDistanceMultiplier values.
To make planets more visible, increase the massRatio values.
*/









// ---------------old version---------------

//#include "GameConstants.h"
//#include "Planet.h"
//
//namespace GameConstants {
//
//    // Helper function to create a ring of identical planets
//    std::vector<PlanetConfig> createPlanetRing(
//        int numPlanets,           // How many planets in the ring
//        float massRatio,          // Mass of each planet (as ratio of main)
//        float orbitDistance,      // Distance from center (multiplier)
//        sf::Color color,          // Color of all planets
//        float startAngle = 0.0f)  // Starting angle offset (optional)
//    {
//        std::vector<PlanetConfig> ringPlanets;
//
//        // Calculate angle between each planet for even spacing
//        float angleIncrement = (2.0f * GameConstants::PI) / numPlanets;
//
//        for (int i = 0; i < numPlanets; i++) {
//            float angle = startAngle + (angleIncrement * i);
//            PlanetConfig planet(massRatio, orbitDistance, angle, color);
//            ringPlanets.push_back(planet);
//        }
//
//        return ringPlanets;
//    }
//
//    std::vector<PlanetConfig> getPlanetConfigurations() {
//        std::vector<PlanetConfig> configs;
//
//        // Green planet
//        PlanetConfig green(0.06f, 2.6f, 0.0f, sf::Color::Green);
//        green.moons.push_back(MoonConfig(0.01f, 2.5f, 0.0f, sf::Color(200, 200, 255)));        // Large blue-white moon
//        //green.moons.push_back(MoonConfig(0.015f, 4.0f, GameConstants::PI * 0.5f, sf::Color(255, 200, 200))); // Medium pink moon
//        //green.moons.push_back(MoonConfig(0.008f, 5.5f, GameConstants::PI, sf::Color(200, 255, 200)));        // Small green moon
//
//        configs.push_back(green);
//
//        // Magenta planet
//        PlanetConfig magenta(0.014f, 1.9f, GameConstants::PI / 3, sf::Color::Magenta);
//
//        //magenta.moons.push_back(MoonConfig(0.01f, 1.5f, 0.0f, sf::Color(200, 200, 255)));
//        configs.push_back(magenta);
//
//        // Pink planet
//        PlanetConfig pink(0.0065f, 0.6f, 0.0f, sf::Color(255, 192, 203));
//
//        //pink.moons.push_back(MoonConfig(0.01f, 1.5f, 0.0f, sf::Color(200, 200, 255)));
//        configs.push_back(pink);
//
//        // Cyan planet - MASSIVE with MOONS!
//        //PlanetConfig cyan(0.6f, 5.0f, 0.0f, sf::Color::Cyan);
//
//        //// Add 3 moons to the cyan planet (it's massive enough to hold them)
//        //cyan.moons.push_back(MoonConfig(0.03f, 2.5f, 0.0f, sf::Color(200, 200, 255)));        // Large blue-white moon
//        //cyan.moons.push_back(MoonConfig(0.015f, 4.0f, GameConstants::PI * 0.5f, sf::Color(255, 200, 200))); // Medium pink moon
//        //cyan.moons.push_back(MoonConfig(0.008f, 5.5f, GameConstants::PI, sf::Color(200, 255, 200)));        // Small green moon
//
//        //configs.push_back(cyan);
//
//        //PlanetConfig pink2(0.0065f, 10.0f, 0.0f, sf::Color(255, 192, 203));
//        //configs.push_back(pink2);
//        //PlanetConfig pink3(0.0065f, 10.0f, 0.5f, sf::Color(255, 192, 203));
//        //configs.push_back(pink3);
//        //PlanetConfig pink4(0.0065f, 10.0f, 1.0f, sf::Color(255, 192, 203));
//        //configs.push_back(pink4);
//
//        //auto outerRing = createPlanetRing(
//        //    8,                              // 8 planets in the ring
//        //    0.065f,                        // Small mass
//        //    10.0f,                          // Far out
//        //    sf::Color(255, 192, 203),       // Pink color
//        //    0.0f                            // Start at angle 0
//        //);
//
//        //// Add all ring planets to configs
//        //for (const auto& planet : outerRing) {
//        //    configs.push_back(planet);
//        //}
//
//        return configs;
//    }
//}
//// PLANET CONFIGURATION EXPLAINED:
//// {massRatio, orbitDistanceMultiplier, angleOffset, color}
//
////{ 0.06f,    1.0f,     0.0f,     sf::Color::Green},
////                            
////  |        |         |         |
////  |        |         |         ??? Planet color (visual appearance)
////  |        |         ??? Starting angle in orbit (radians)
////  |        |             0.0f = right side of central planet
////  |        |             PI/2 = bottom, PI = left, 3*PI/2 = top
////  |        ??? Distance from central planet multiplier
////  |            1.0f = same as base orbit distance
////  |            1.5f = 50 further out, 2.0f = twice as far
////  ??? Mass as percentage of central planet
////      0.06f = 6 of main planet mass
////      0.04f = 4 of main planet mass (smaller planet)
////      0.1f = 10 of main planet mass (larger planet)
//
//// EXAMPLES:
//// Small close planet:    {0.03f, 0.8f, 0.0f, sf::Color::Blue}
//// Large distant planet:  {0.08f, 2.5f, PI, sf::Color::Red}
//// Tiny far planet:       {0.015f, 3.0f, PI/4, sf::Color::Yellow}
//
//// AVAILABLE COLORS:
//// sf::Color::Black
//// sf::Color::White
//// sf::Color::Red
//// sf::Color::Green
//// sf::Color::Blue
//// sf::Color::Yellow
//// sf::Color::Magenta
//// sf::Color::Cyan
//
//// CUSTOM RGB COLORS (Red, Green, Blue values 0-255):
//// sf::Color(255, 0, 0)      // Pure red
//// sf::Color(255, 165, 0)    // Orange
//// sf::Color(128, 0, 128)    // Purple
//// sf::Color(255, 192, 203)  // Pink
//// sf::Color(139, 69, 19)    // Brown
//// sf::Color(255, 215, 0)    // Gold
//// sf::Color(192, 192, 192)  // Silver
//// sf::Color(0, 255, 127)    // Spring green
//// sf::Color(70, 130, 180)   // Steel blue
//// sf::Color(220, 20, 60)    // Crimson
//// sf::Color(255, 20, 147)   // Deep pink
//// sf::Color(0, 191, 255)    // Deep sky blue
//// sf::Color(50, 205, 50)    // Lime green
//// sf::Color(255, 69, 0)     // Red orange
//// sf::Color(148, 0, 211)    // Dark violet