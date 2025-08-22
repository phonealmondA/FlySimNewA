#pragma once
#ifndef GAME_CONSTANTS_H
#define GAME_CONSTANTS_H

#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
struct PlanetConfig;

namespace GameConstants {
    // Gravitational constants
    constexpr float G = 100.0f;  // Gravitational constant
    constexpr float PI = 3.14159265358979323846f;

    // Mass-radius relationship constants
    constexpr float BASE_RADIUS_FACTOR = 260.0f;  // Base size factor for planets
    constexpr float REFERENCE_MASS = 10000.0f;  // Reference mass for radius scaling

    // Primary inputs
    constexpr float MAIN_PLANET_MASS = 260000.0f;  // Primary parameter to adjust
    constexpr float ORBIT_PERIOD = 420.0f;  // Desired orbit period in seconds

    // Derived parameters
    constexpr float SECONDARY_PLANET_MASS = MAIN_PLANET_MASS * 0.06f;  // 8% of main planet mass

    // Fixed radius values (not using functions that would cause constexpr issues)
    constexpr float MAIN_PLANET_RADIUS = 10000.0f;  // Base radius at this mass
    constexpr float MASS_RATIO = 0.06f;  // Same as SECONDARY_PLANET_MASS / MAIN_PLANET_MASS
    constexpr float CUBE_ROOT_APPROX = 60.0f;  // Approximate cube root of 0.08
    constexpr float SECONDARY_PLANET_RADIUS = (MAIN_PLANET_RADIUS / CUBE_ROOT_APPROX) / 10000;  // ~430

    // Planet positions
    constexpr float MAIN_PLANET_X = 400.0f;
    constexpr float MAIN_PLANET_Y = 300.0f;

    // Non-constexpr calculations for orbital parameters
    const float PLANET_ORBIT_DISTANCE = std::pow((G * MAIN_PLANET_MASS * ORBIT_PERIOD * ORBIT_PERIOD) / (4.0f * PI * PI), 1.0f / 3.0f);

    // Secondary planet position based on orbital distance
    const float SECONDARY_PLANET_X = MAIN_PLANET_X + PLANET_ORBIT_DISTANCE;
    const float SECONDARY_PLANET_Y = MAIN_PLANET_Y;

    // Pre-calculate orbital velocity for a circular orbit
    const float SECONDARY_PLANET_ORBITAL_VELOCITY =
        std::sqrt(G * MAIN_PLANET_MASS / PLANET_ORBIT_DISTANCE);

    // Rocket parameters - UPDATED FOR DYNAMIC MASS SYSTEM
    constexpr float ROCKET_BASE_MASS = 1.0f;  // Mass to make a rocket (empty)
    constexpr float ROCKET_MAX_MASS = 101.0f;  // Maximum total rocket mass
    constexpr float ROCKET_SIZE = 15.0f;

    // FUEL SYSTEM CONSTANTS - UPDATED
    constexpr float ROCKET_MAX_FUEL = 100.0f;  // Maximum fuel capacity
    constexpr float ROCKET_STARTING_FUEL = 0.0f;  // Starting fuel amount (empty rocket)

    // Manual fuel transfer system
    constexpr float MANUAL_FUEL_TRANSFER_RATE = 10.0f;  // Fuel units per second during manual transfer
    constexpr float FUEL_TRANSFER_THRUST_MULTIPLIER = 0.1f;  // Multiplier for thrust level affecting transfer rate

    // Fuel consumption rates (units per second at different thrust levels)
    constexpr float FUEL_CONSUMPTION_BASE = 2.0f;  // Base consumption at 10% thrust
    constexpr float FUEL_CONSUMPTION_MULTIPLIER = 8.0f;  // Multiplier for higher thrust levels
    constexpr float FUEL_CONSUMPTION_MIN_THRESHOLD = 0.1f;  // Minimum thrust level that consumes fuel

    // AUTOMATIC fuel collection parameters (for future satellites)
    constexpr float FUEL_COLLECTION_RANGE = 250.0f;  // Distance from planet surface for fuel collection
    constexpr float FUEL_COLLECTION_RATE = 15.0f;  // Fuel units per second when collecting (auto mode)
    constexpr float FUEL_COLLECTION_MASS_RATIO = 150.0f;  // How much planet mass equals 1 fuel unit
    constexpr float MIN_PLANET_MASS_FOR_COLLECTION = 50.0f;  // Minimum planet mass before collection stops

    // Fuel collection ring visual settings
    constexpr float FUEL_RING_THICKNESS = 3.0f;  // Thickness of the fuel collection ring
    const sf::Color FUEL_RING_COLOR = sf::Color(0, 255, 255, 128);  // Cyan with transparency
    const sf::Color FUEL_RING_ACTIVE_COLOR = sf::Color(255, 255, 0, 200);  // Yellow when actively collecting

    // Visualization settings
    constexpr float GRAVITY_VECTOR_SCALE = 15.0f;
    constexpr float VELOCITY_VECTOR_SCALE = 10.0f;

    // Trajectory calculation settings
    constexpr float TRAJECTORY_TIME_STEP = 0.5f;
    constexpr int TRAJECTORY_STEPS = 100;
    constexpr float TRAJECTORY_COLLISION_RADIUS = 10.0f;


    // Engine parameters - scaled with gravitational constant
    constexpr float BASE_THRUST_MULTIPLIER = 10000000000.0f;
    constexpr float ENGINE_THRUST_POWER = G * BASE_THRUST_MULTIPLIER;

    // Vehicle transformation parameters
    constexpr float TRANSFORM_VELOCITY_FACTOR = 0.1f;  // Velocity reduction when transforming from rocket to car

    // SATELLITE SYSTEM CONSTANTS

    // Satellite basic parameters
    constexpr float SATELLITE_BASE_MASS = 0.8f;              // Empty satellite mass (lighter than rocket)
    constexpr float SATELLITE_MAX_MASS = 80.0f;              // Maximum total satellite mass
    constexpr float SATELLITE_MAX_FUEL = 80.0f;              // Maximum fuel capacity (slightly less than rocket)
    constexpr float SATELLITE_STARTING_FUEL = 60.0f;         // Starting fuel when converted from rocket
    constexpr float SATELLITE_SIZE = 12.0f;                  // Visual size for collision detection

    // Station-keeping and orbital maintenance
    constexpr float SATELLITE_MAINTENANCE_FUEL_PERCENT = 0.2f; // 20% of fuel reserved for maintenance
    constexpr float SATELLITE_ORBIT_TOLERANCE = 50.0f;        // Default acceptable orbit drift (units)
    constexpr float SATELLITE_ECCENTRICITY_TOLERANCE = 0.01f; // Acceptable eccentricity change
    constexpr float SATELLITE_INCLINATION_TOLERANCE = 0.017f; // ~1 degree in radians
    constexpr float SATELLITE_PERIOD_TOLERANCE = 10.0f;       // Acceptable orbital period change (seconds)

    // Maintenance timing
    constexpr float SATELLITE_MAINTENANCE_CHECK_INTERVAL = 30.0f; // Check orbit every 30 seconds
    constexpr float SATELLITE_CORRECTION_DELAY = 5.0f;           // Wait 5 seconds before executing corrections
    constexpr float SATELLITE_MAX_SINGLE_BURN = 3.0f;           // Maximum delta-V per correction burn

    // Fuel efficiency and consumption
    constexpr float SATELLITE_FUEL_EFFICIENCY = 1.2f;           // Better fuel efficiency than rockets
    constexpr float SATELLITE_MAINTENANCE_FUEL_RATE = 0.5f;     // Fuel consumption rate for maintenance (units/second)
    constexpr float SATELLITE_THRUST_TO_WEIGHT_RATIO = 0.08f;   // Lower thrust than rockets (more efficient)

    // Fuel transfer network
    constexpr float SATELLITE_TRANSFER_RANGE = 2500.0f;          // Maximum fuel transfer distance
    constexpr float SATELLITE_BASE_TRANSFER_RATE = 25.0f;        // Base fuel transfer rate between satellites
    constexpr float SATELLITE_TRANSFER_EFFICIENCY = 1.0f;       // Distance-based efficiency factor
    constexpr int SATELLITE_MAX_SIMULTANEOUS_TRANSFERS = 5;     // Maximum concurrent fuel transfers

    // Emergency thresholds
    constexpr float SATELLITE_EMERGENCY_FUEL_THRESHOLD = 0.1f;  // 10% fuel = emergency
    constexpr float SATELLITE_CRITICAL_FUEL_THRESHOLD = 0.05f;  // 5% fuel = critical
    constexpr float SATELLITE_MINIMUM_MAINTENANCE_FUEL = 5.0f;  // Minimum fuel for basic maintenance

    // Visual and UI constants
    constexpr float SATELLITE_PANEL_SIZE = 8.0f;               // Solar panel visual size
    const sf::Color SATELLITE_BODY_COLOR = sf::Color(100, 200, 255, 255);     // Light blue
    const sf::Color SATELLITE_PANEL_COLOR = sf::Color(50, 50, 200, 255);      // Dark blue
    const sf::Color SATELLITE_STATUS_ACTIVE = sf::Color(0, 255, 0, 200);      // Green for active
    const sf::Color SATELLITE_STATUS_LOW_FUEL = sf::Color(255, 255, 0, 200);  // Yellow for low fuel
    const sf::Color SATELLITE_STATUS_CRITICAL = sf::Color(255, 100, 0, 200);  // Orange for critical
    const sf::Color SATELLITE_STATUS_DEPLETED = sf::Color(255, 0, 0, 200);    // Red for depleted

    // Orbit visualization
    const sf::Color SATELLITE_ORBIT_PATH_COLOR = sf::Color(0, 255, 255, 128);     // Cyan orbit path
    const sf::Color SATELLITE_TARGET_ORBIT_COLOR = sf::Color(255, 255, 0, 128);   // Yellow target orbit
    const sf::Color SATELLITE_MAINTENANCE_BURN_COLOR = sf::Color(255, 0, 255, 255); // Magenta burns
    constexpr float SATELLITE_ORBIT_PATH_THICKNESS = 2.0f;

    // Network visualization
    const sf::Color SATELLITE_CONNECTION_COLOR = sf::Color(100, 255, 100, 100);   // Light green connections
    const sf::Color SATELLITE_TRANSFER_FLOW_COLOR = sf::Color(255, 255, 100, 200); // Yellow fuel flow
    const sf::Color SATELLITE_EMERGENCY_COLOR = sf::Color(255, 50, 50, 255);      // Bright red emergency
    constexpr float SATELLITE_CONNECTION_THICKNESS = 1.5f;
    constexpr float SATELLITE_TRANSFER_THICKNESS = 3.0f;

    // Conversion parameters
    constexpr float SATELLITE_CONVERSION_FUEL_RETENTION = 0.8f; // Retain 80% of rocket's fuel
    constexpr float SATELLITE_CONVERSION_MASS_EFFICIENCY = 0.9f; // 90% mass efficiency during conversion
    constexpr float SATELLITE_MIN_CONVERSION_ALTITUDE = 100.0f;  // Minimum altitude above planet for conversion

    // Orbital prediction and simulation
    constexpr float SATELLITE_PREDICTION_TIME_STEP = 1.0f;      // Prediction simulation time step
    constexpr int SATELLITE_PREDICTION_STEPS = 3600;            // Predict 1 hour ahead (3600 seconds)
    constexpr float SATELLITE_DRIFT_DETECTION_SENSITIVITY = 0.1f; // How sensitive drift detection is

    // Network optimization
    constexpr float SATELLITE_NETWORK_UPDATE_INTERVAL = 2.0f;   // Update network every 2 seconds
    constexpr float SATELLITE_FUEL_BALANCE_THRESHOLD = 0.15f;   // 15% difference triggers balancing
    constexpr float SATELLITE_PRIORITY_DISTANCE_FACTOR = 0.001f; // Distance factor for priority calculation

    // Performance and limits
    constexpr int SATELLITE_MAX_NETWORK_SIZE = 50;              // Maximum satellites in network
    constexpr int SATELLITE_MAX_MANEUVERS_QUEUED = 10;          // Maximum planned maneuvers per satellite
    constexpr float SATELLITE_MANEUVER_TIMEOUT = 300.0f;        // Cancel maneuvers after 5 minutes

    // Advanced features
    constexpr float SATELLITE_ADAPTIVE_LEARNING_RATE = 0.01f;   // Learning rate for adaptive maintenance
    constexpr float SATELLITE_COLLABORATIVE_RANGE = 1000.0f;    // Range for collaborative maintenance
    constexpr float SATELLITE_RESONANCE_DETECTION_THRESHOLD = 0.05f; // Threshold for orbital resonance detection

    // Integration with existing systems
    constexpr float SATELLITE_GRAVITY_INFLUENCE_FACTOR = 0.1f;  // How much satellites affect each other gravitationally
    constexpr float SATELLITE_ROCKET_DOCKING_RANGE = 210.0f;     // Range for fuel transfer to rockets
    constexpr float SATELLITE_PLANET_COLLECTION_EFFICIENCY = 1.2f; // More efficient collection than rockets

    std::vector<PlanetConfig> getPlanetConfigurations();
}



#endif // GAME_CONSTANTS_H