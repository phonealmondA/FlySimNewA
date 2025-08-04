#pragma once
#ifndef SATELLITE_H
#define SATELLITE_H

#include "GameObject.h"
#include "GameConstants.h"
#include "Planet.h"
#include <vector>
#include <memory>
#include <utility>  // For std::pair

// Forward declarations
class Rocket;

// Forward declare OrbitalElements to avoid duplicate definition
struct OrbitalElements;

// Satellite status enum
enum class SatelliteStatus {
    ACTIVE,           // Normal operation with fuel reserves
    LOW_FUEL,         // Below recommended fuel reserves
    CRITICAL_FUEL,    // Only maintenance fuel remaining
    FUEL_DEPLETED,    // No fuel - natural drift only
    MAINTENANCE_MODE, // Performing orbital correction
    TRANSFER_MODE     // Actively transferring fuel
};

class Satellite : public GameObject {
private:

    // Orbital maintenance system
    OrbitalElements* targetOrbit;      // Desired orbital parameters (pointer to avoid incomplete type)
    OrbitalElements* currentOrbit;     // Actual orbital parameters (pointer to avoid incomplete type)
    float orbitToleranceRadius;       // How much drift is acceptable (units)
    float orbitToleranceEccentricity; // How much eccentricity drift is acceptable

    // Fuel and mass system (similar to Rocket but with reserves)
    float currentFuel;                // Total fuel available
    float maxFuel;                    // Maximum fuel capacity
    float maintenanceFuelReserve;     // Fuel reserved for station-keeping
    float mass;                       // Current total mass
    float baseMass;                   // Empty satellite mass
    float maxMass;                    // Maximum total mass

    // Station-keeping system
    float lastMaintenanceTime;        // Time since last orbital correction
    float maintenanceInterval;        // How often to check orbit (seconds)
    bool needsOrbitalCorrection;      // Flag for pending correction
    sf::Vector2f plannedCorrectionBurn; // Next correction vector

    // Visual and identification
    sf::CircleShape body;             // Satellite visual representation
    sf::ConvexShape solarPanels[2];   // Solar panel visual elements
    int satelliteID;                  // Unique identifier
    std::string name;                 // Human-readable name
    SatelliteStatus status;           // Current operational status

    // Network and communication
    std::vector<Planet*> nearbyPlanets;      // Planets for fuel collection
    std::vector<Satellite*> nearbySatellites; // Other satellites in range
    std::vector<Rocket*> nearbyRockets;       // Rockets in range for fuel transfer
    float transferRange;                      // Maximum fuel transfer range
    bool isCollectingFuel;                   // Currently collecting from planet
    Planet* fuelSourcePlanet;                // Which planet we're collecting from

    // Network multiplayer support - NEW
    int ownerPlayerID;                       // Which player created this satellite
    bool isNetworkSatellite;                 // Created by remote player
    float lastNetworkUpdateTime;             // For network synchronization
    sf::Vector2f lastNetworkPosition;        // Last position from network
    sf::Vector2f lastNetworkVelocity;        // Last velocity from network
    bool needsNetworkSync;                   // Satellite state needs sync
    float networkSyncInterval;               // How often to sync with network

    // Station-keeping parameters
    float stationKeepingEfficiency;   // How efficiently satellite uses fuel for corrections
    float maxCorrectionBurn;          // Maximum single correction burn magnitude
    float fuelConsumptionRate;        // Base fuel consumption for maintenance

    // Rocket transfer tracking
    struct RocketTransferInfo {
        float totalTransferred = 0.0f;
        bool isActive = false;
    };
    std::vector<std::pair<Rocket*, RocketTransferInfo>> rocketTransferTracking;

    // Internal methods
    void updateOrbitalElements();
    bool checkOrbitAccuracy();
    void calculateCorrectionBurn();
    void performStationKeeping(float deltaTime);
    void updateMassFromFuel();
    void updateStatus();

    // Fuel management
    float getAvailableFuelForTransfer() const; // Total fuel minus maintenance reserve
    bool canPerformMaintenance() const;        // Check if we have maintenance fuel
    void consumeMaintenanceFuel(float amount);

public:
    // Construction and initialization
    Satellite(sf::Vector2f pos, sf::Vector2f vel, int id,
        sf::Color col = sf::Color::Cyan, float baseM = GameConstants::SATELLITE_BASE_MASS);

    // Destructor to clean up orbital elements
    ~Satellite();

    // Factory method to create satellite from existing rocket
    static std::unique_ptr<Satellite> createFromRocket(const Rocket* rocket, int id);

    // Core GameObject overrides
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;
    void drawWithConstantSize(sf::RenderWindow& window, float zoomLevel);

    // Orbital management
    void setTargetOrbitFromCurrent(); // Set current state as target
    float getOrbitAccuracy() const; // Returns percentage accuracy (0-100)

    // Fuel system
    float getCurrentFuel() const { return currentFuel; }
    float getMaxFuel() const { return maxFuel; }
    float getMaintenanceFuelReserve() const { return maintenanceFuelReserve; }
    float getAvailableFuel() const { return getAvailableFuelForTransfer(); }
    float getFuelPercentage() const { return maxFuel > 0 ? (currentFuel / maxFuel) * 100.0f : 0.0f; }
    float getMaintenanceFuelPercentage() const;

    void setFuel(float fuel);
    void addFuel(float fuel);
    bool transferFuelTo(Satellite* target, float amount);
    bool transferFuelToRocket(Rocket* rocket, float amount);

    void transferFuelToNearbyRockets(float deltaTime);
    bool isRocketInTransferRange(const Rocket* rocket) const;

    // Mass system
    float getMass() const { return mass; }
    float getBaseMass() const { return baseMass; }
    float getMaxMass() const { return maxMass; }

    // Status and identification
    int getID() const { return satelliteID; }
    std::string getName() const { return name; }
    void setName(const std::string& newName) { name = newName; }
    SatelliteStatus getStatus() const { return status; }
    bool isOperational() const { return status != SatelliteStatus::FUEL_DEPLETED; }

    // Network management
    void setNearbyPlanets(const std::vector<Planet*>& planets) { nearbyPlanets = planets; }
    void setNearbySatellites(const std::vector<Satellite*>& satellites) { nearbySatellites = satellites; }
    void setNearbyRockets(const std::vector<Rocket*>& rockets) { nearbyRockets = rockets; }
    float getTransferRange() const { return transferRange; }

    // Automated fuel collection
    void collectFuelFromPlanets(float deltaTime);
    bool isInFuelCollectionRange(const Planet* planet) const;

    // Maintenance and efficiency
    void setStationKeepingEfficiency(float efficiency) { stationKeepingEfficiency = efficiency; }
    float getStationKeepingEfficiency() const { return stationKeepingEfficiency; }
    float getMaintenanceFuelCost() const; // Cost per orbital correction

    // Visual and debugging
    void drawOrbitPath(sf::RenderWindow& window, const std::vector<Planet*>& planets);
    void drawTargetOrbitPath(sf::RenderWindow& window, const std::vector<Planet*>& planets);
    void drawFuelTransferLines(sf::RenderWindow& window);
    void drawMaintenanceBurn(sf::RenderWindow& window);
    void drawStatusIndicator(sf::RenderWindow& window, float zoomLevel);

    // Network communication (for multiplayer) - ENHANCED
    void sendStatusUpdate();
    void receiveStatusUpdate();

    // Network state management
    void setOwnerPlayerID(int playerID) { ownerPlayerID = playerID; }
    int getOwnerPlayerID() const { return ownerPlayerID; }
    void setNetworkSatellite(bool isNetwork) { isNetworkSatellite = isNetwork; }
    bool isFromNetwork() const { return isNetworkSatellite; }

    // Network synchronization
    void updateFromNetworkState(sf::Vector2f netPosition, sf::Vector2f netVelocity, float netFuel);
    bool needsNetworkUpdate() const { return needsNetworkSync; }
    void markNetworkSynced() { needsNetworkSync = false; }
    void requestNetworkSync() { needsNetworkSync = true; }

    // Enhanced drawing methods for multiplayer consistency
    void drawNetworkIndicator(sf::RenderWindow& window, float zoomLevel);  // Show owner info
    static float getConstantDisplaySize() { return GameConstants::SATELLITE_SIZE; }

    // Cross-player rocket targeting support
    void updateRocketTargeting();  // Refresh rocket targeting for all network rockets
    bool canTargetAllNetworkRockets() const;
    std::vector<Rocket*> getTargetableRockets() const { return nearbyRockets; }
};

#endif // SATELLITE_H