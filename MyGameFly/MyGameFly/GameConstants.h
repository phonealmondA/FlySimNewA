#pragma once
#ifndef GAME_CONSTANTS_H
#define GAME_CONSTANTS_H

#include <SFML/Graphics.hpp>
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
    constexpr float FUEL_COLLECTION_MASS_RATIO = 1.0f;  // How much planet mass equals 1 fuel unit
    constexpr float MIN_PLANET_MASS_FOR_COLLECTION = 50.0f;  // Minimum planet mass before collection stops

    // Fuel collection ring visual settings
    constexpr float FUEL_RING_THICKNESS = 3.0f;  // Thickness of the fuel collection ring
    const sf::Color FUEL_RING_COLOR = sf::Color(0, 255, 255, 128);  // Cyan with transparency
    const sf::Color FUEL_RING_ACTIVE_COLOR = sf::Color(255, 255, 0, 200);  // Yellow when actively collecting

    // Visualization settings
    constexpr float GRAVITY_VECTOR_SCALE = 15.0f;
    constexpr float VELOCITY_VECTOR_SCALE = 10.0f;

    // Trajectory calculation settings
    constexpr float TRAJECTORY_TIME_STEP = 0.05f;
    constexpr int TRAJECTORY_STEPS = 7500;
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

    // NETWORK MULTIPLAYER SATELLITE CONSTANTS - NEW

    // Network synchronization timing
    constexpr float NETWORK_SATELLITE_SYNC_INTERVAL = 1.0f / 30.0f;  // 30 FPS satellite sync
    constexpr float NETWORK_PLANET_SYNC_INTERVAL = 1.0f;            // 1 FPS planet sync
    constexpr float NETWORK_STATE_SEND_INTERVAL = 1.0f / 20.0f;     // 20 FPS player state sync
    constexpr float NETWORK_HEARTBEAT_INTERVAL = 5.0f;              // Heartbeat every 5 seconds

    // Network satellite identification
    constexpr int NETWORK_SATELLITE_ID_OFFSET = 1000;               // ID offset per player (Player 0: 0-999, Player 1: 1000-1999, etc.)
    constexpr int NETWORK_MAX_PLAYERS = 8;                          // Maximum players in network
    constexpr int NETWORK_MAX_SATELLITES_PER_PLAYER = 100;          // Max satellites per player

    // Cross-player satellite visibility and targeting
    constexpr bool NETWORK_ENABLE_CROSS_PLAYER_FUEL_TRANSFER = true; // Allow fuel transfer between players
    constexpr bool NETWORK_ENABLE_SATELLITE_SHARING = true;          // Allow satellites to connect across players
    constexpr float NETWORK_CROSS_PLAYER_TRANSFER_EFFICIENCY = 0.9f; // Slightly less efficient cross-player transfers
    constexpr float NETWORK_SATELLITE_OWNERSHIP_INDICATOR_SIZE = 3.0f; // Size of player ownership indicator

    // Planet state synchronization
    constexpr float NETWORK_PLANET_MASS_SYNC_THRESHOLD = 0.1f;       // Only sync if mass changes by this amount
    constexpr float NETWORK_PLANET_POSITION_SYNC_THRESHOLD = 1.0f;   // Only sync if position changes by this amount
    constexpr float NETWORK_PLANET_VELOCITY_SYNC_THRESHOLD = 0.1f;   // Only sync if velocity changes by this amount
    constexpr float NETWORK_PLANET_INTERPOLATION_FACTOR = 0.1f;      // Smooth interpolation for network updates

    // Satellite visual constants for constant size rendering
    constexpr float SATELLITE_CONSTANT_DISPLAY_SIZE = SATELLITE_SIZE; // Base size for constant rendering
    constexpr float SATELLITE_MIN_ZOOM_SIZE = 8.0f;                   // Minimum visual size when zoomed out
    constexpr float SATELLITE_MAX_ZOOM_SIZE = 24.0f;                  // Maximum visual size when zoomed in
    constexpr float SATELLITE_ZOOM_SCALE_FACTOR = 1.0f;               // Scale factor for zoom-independent rendering

    // Network timeout and retry constants
    constexpr float NETWORK_CONNECTION_TIMEOUT = 30.0f;              // Connection timeout in seconds
    constexpr float NETWORK_RETRY_INTERVAL = 5.0f;                   // Retry failed operations every 5 seconds
    constexpr int NETWORK_MAX_RETRY_ATTEMPTS = 3;                    // Maximum retry attempts
    constexpr float NETWORK_PACKET_LOSS_TOLERANCE = 0.1f;            // 10% packet loss tolerance

    // Network satellite state interpolation
    constexpr float NETWORK_SATELLITE_INTERPOLATION_FACTOR = 0.1f;   // Smooth satellite movement
    constexpr float NETWORK_SATELLITE_PREDICTION_TIME = 0.1f;        // Predict 100ms ahead for smooth movement
    constexpr float NETWORK_SATELLITE_EXTRAPOLATION_LIMIT = 0.5f;    // Maximum extrapolation time

    // Network fuel transfer synchronization
    constexpr float NETWORK_FUEL_TRANSFER_SYNC_RATE = 10.0f;         // Sync fuel transfers 10 times per second
    constexpr float NETWORK_FUEL_TRANSFER_BUFFER_SIZE = 5.0f;        // Buffer fuel transfers for smooth sync
    constexpr bool NETWORK_ENABLE_FUEL_PREDICTION = true;            // Predict fuel changes for smooth display

    // Network satellite network topology
    constexpr float NETWORK_SATELLITE_CONNECTION_RANGE = SATELLITE_TRANSFER_RANGE; // Use same range for network connections
    constexpr int NETWORK_MAX_SATELLITE_CONNECTIONS = 8;             // Maximum connections per satellite
    constexpr bool NETWORK_ENABLE_MESH_TOPOLOGY = true;              // Allow mesh network topology
    constexpr float NETWORK_TOPOLOGY_UPDATE_INTERVAL = 2.0f;         // Update network topology every 2 seconds

    // Network error recovery
    constexpr float NETWORK_STATE_MISMATCH_THRESHOLD = 10.0f;        // Threshold for detecting state mismatches
    constexpr bool NETWORK_ENABLE_STATE_CORRECTION = true;           // Enable automatic state correction
    constexpr float NETWORK_STATE_CORRECTION_INTERVAL = 1.0f;        // Correct state mismatches every second
    constexpr int NETWORK_MAX_STATE_CORRECTIONS = 5;                 // Maximum corrections per interval

    // Network performance optimization
    constexpr bool NETWORK_ENABLE_DELTA_COMPRESSION = true;          // Only send state changes
    constexpr float NETWORK_DELTA_THRESHOLD = 0.01f;                 // Minimum change to trigger update
    constexpr bool NETWORK_ENABLE_BATCH_UPDATES = true;              // Batch multiple updates together
    constexpr int NETWORK_MAX_BATCH_SIZE = 10;                       // Maximum updates per batch

    // Network satellite ownership visualization
    const sf::Color NETWORK_PLAYER_COLORS[NETWORK_MAX_PLAYERS] = {
        sf::Color(255, 100, 100, 180),  // Player 1: Light Red
        sf::Color(100, 100, 255, 180),  // Player 2: Light Blue  
        sf::Color(100, 255, 100, 180),  // Player 3: Light Green
        sf::Color(255, 255, 100, 180),  // Player 4: Light Yellow
        sf::Color(255, 100, 255, 180),  // Player 5: Light Magenta
        sf::Color(100, 255, 255, 180),  // Player 6: Light Cyan
        sf::Color(255, 255, 255, 180),  // Player 7: Light White
        sf::Color(255, 150, 100, 180)   // Player 8: Light Orange
    };

    // Network planet synchronization colors
    const sf::Color NETWORK_PLANET_INDICATOR_COLOR = sf::Color(255, 255, 0, 180); // Yellow for network-synced planets
    const sf::Color NETWORK_PLANET_HOST_COLOR = sf::Color(0, 255, 0, 180);        // Green for host-controlled planets
    const sf::Color NETWORK_PLANET_CLIENT_COLOR = sf::Color(0, 0, 255, 180);      // Blue for client-controlled planets

    // Network debugging and monitoring
    constexpr bool NETWORK_ENABLE_DEBUG_OVERLAY = false;             // Show network debug info
    constexpr bool NETWORK_ENABLE_LATENCY_COMPENSATION = true;       // Compensate for network latency
    constexpr float NETWORK_LATENCY_ESTIMATE = 0.05f;                // Estimated network latency (50ms)
    constexpr bool NETWORK_ENABLE_JITTER_BUFFER = true;              // Buffer to handle jitter
    constexpr float NETWORK_JITTER_BUFFER_SIZE = 0.1f;               // 100ms jitter buffer
}



#endif // GAME_CONSTANTS_H