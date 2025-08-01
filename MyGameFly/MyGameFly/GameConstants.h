#pragma once
#include <cmath>
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

    // Rocket parameters
    constexpr float ROCKET_MASS = 1000.0f;
    constexpr float ROCKET_SIZE = 15.0f;

    // FUEL SYSTEM CONSTANTS
    constexpr float ROCKET_MAX_FUEL = 100.0f;  // Maximum fuel capacity
    constexpr float ROCKET_STARTING_FUEL = 100.0f;  // Starting fuel amount

    // Fuel consumption rates (units per second at different thrust levels)
    constexpr float FUEL_CONSUMPTION_BASE = 2.0f;  // Base consumption at 10% thrust
    constexpr float FUEL_CONSUMPTION_MULTIPLIER = 8.0f;  // Multiplier for higher thrust levels
    constexpr float FUEL_CONSUMPTION_MIN_THRESHOLD = 0.1f;  // Minimum thrust level that consumes fuel

    // Fuel collection parameters
    constexpr float FUEL_COLLECTION_RANGE = 250.0f;  // Distance from planet surface for fuel collection
    constexpr float FUEL_COLLECTION_RATE = 15.0f;  // Fuel units per second when collecting
    constexpr float FUEL_COLLECTION_MASS_RATIO = 1.0f;  // How much planet mass equals 1 fuel unit
    constexpr float MIN_PLANET_MASS_FOR_COLLECTION = 50.0f;  // Minimum planet mass before collection stops

    // Fuel collection ring visual settings
    constexpr float FUEL_RING_THICKNESS = 3.0f;  // Thickness of the fuel collection ring
    const sf::Color FUEL_RING_COLOR = sf::Color(0, 255, 255, 128);  // Cyan with transparency
    const sf::Color FUEL_RING_ACTIVE_COLOR = sf::Color(255, 255, 0, 200);  // Yellow when actively collecting

    // Visualization settings
    constexpr float GRAVITY_VECTOR_SCALE = 5.0f;
    constexpr float VELOCITY_VECTOR_SCALE = 0.01f;

    // Trajectory calculation settings
    constexpr float TRAJECTORY_TIME_STEP = 0.05f;
    constexpr int TRAJECTORY_STEPS = 5000;
    constexpr float TRAJECTORY_COLLISION_RADIUS = 10.0f;

    // Vehicle physics
    constexpr float FRICTION = 0.0098f;  // Friction coefficient for surface movement
    constexpr float TRANSFORM_DISTANCE = 30.0f;  // Distance for vehicle transformation
    constexpr float ADAPTIVE_TIMESTEP_THRESHOLD = 10.0f;  // Threshold for adaptive timestep
    constexpr float CAR_WHEEL_RADIUS = 5.0f;  // Radius of car wheels
    constexpr float CAR_BODY_WIDTH = 30.0f;  // Width of car body
    constexpr float CAR_BODY_HEIGHT = 15.0f;  // Height of car body

    // Engine parameters - scaled with gravitational constant
    constexpr float BASE_THRUST_MULTIPLIER = 10000000000.0f;
    constexpr float ENGINE_THRUST_POWER = G * BASE_THRUST_MULTIPLIER;

    // Vehicle transformation parameters
    constexpr float TRANSFORM_VELOCITY_FACTOR = 0.1f;  // Velocity reduction when transforming from rocket to car
}