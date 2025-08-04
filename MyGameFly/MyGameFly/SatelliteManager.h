#pragma once
#ifndef SATELLITE_MANAGER_H
#define SATELLITE_MANAGER_H

#include "Satellite.h"
#include "Planet.h"
#include "Rocket.h"
#include "FuelTransferNetwork.h"
#include <vector>
#include <memory>
#include <map>
#include <string>

// Forward declarations
class Player;
class VehicleManager;
struct SatelliteCreationInfo;  // Add this forward declaration
struct PlayerState;  // Add this forward declaration
// Satellite management statistics
struct SatelliteNetworkStats {
    int totalSatellites = 0;
    int activeSatellites = 0;
    int lowFuelSatellites = 0;
    int criticalFuelSatellites = 0;
    int depletedSatellites = 0;

    float totalFuel = 0.0f;
    float totalMaintenanceFuel = 0.0f;
    float totalAvailableFuel = 0.0f;
    float averageOrbitAccuracy = 0.0f;

    // Fuel flow statistics
    float fuelCollectionRate = 0.0f;    // Fuel collected from planets per second
    float fuelConsumptionRate = 0.0f;   // Fuel consumed for maintenance per second
    float fuelTransferRate = 0.0f;      // Fuel transferred between satellites per second

    void reset() {
        totalSatellites = activeSatellites = lowFuelSatellites = 0;
        criticalFuelSatellites = depletedSatellites = 0;
        totalFuel = totalMaintenanceFuel = totalAvailableFuel = 0.0f;
        averageOrbitAccuracy = 0.0f;
        fuelCollectionRate = fuelConsumptionRate = fuelTransferRate = 0.0f;
    }
};

// Satellite conversion options
struct SatelliteConversionConfig {
    bool maintainCurrentOrbit = true;     // Use current rocket orbit as target
    float maintenanceFuelPercent = 0.2f;  // 20% of fuel reserved for maintenance
    float stationKeepingEfficiency = 1.0f; // How efficiently satellite uses fuel
    float transferRange = 500.0f;         // Maximum fuel transfer range
    std::string customName = "";          // Custom satellite name (empty = auto-generate)
};

class SatelliteManager {
private:
    // Satellite storage and tracking
    std::vector<std::unique_ptr<Satellite>> satellites;
    std::map<int, Satellite*> satelliteMap;     // Fast ID-based lookup
    std::vector<Planet*> planets;               // Reference to game planets
    std::vector<Rocket*> nearbyRockets;         // Rockets available for fuel transfer
    int nextSatelliteID;                        // Auto-incrementing ID counter

    // Fuel transfer network
    std::unique_ptr<FuelTransferNetwork> transferNetwork;

    // Statistics and monitoring
    SatelliteNetworkStats networkStats;
    float statsUpdateInterval = 1.0f;      // Update stats every second
    float timeSinceStatsUpdate = 0.0f;

    // Orbital maintenance parameters
    float globalMaintenanceInterval = 5.0f;     // Check orbits every 5 seconds
    float globalOrbitTolerance = 50.0f;         // Default orbit tolerance
    bool enableAutomaticMaintenance = true;    // Global maintenance toggle

    // Fuel collection parameters
    bool enableAutomaticCollection = true;     // Global fuel collection toggle
    float collectionEfficiency = 1.0f;         // Global collection efficiency multiplier

    // Visual and UI
    bool showOrbitPaths = true;
    bool showTargetOrbitPaths = false;
    bool showFuelTransferLines = true;
    bool showMaintenanceBurns = true;
    bool showStatusIndicators = true;

    // Internal methods
    void updateNetworkStats();
    void performNetworkFuelTransfers(float deltaTime);
    void optimizeFuelDistribution();
    std::string generateSatelliteName(int id) const;

    // Network synchronization helpers
    void updateRocketProximity();

    // Network multiplayer support - NEW
    std::map<int, int> playerSatelliteOwnership;  // Maps satelliteID -> playerID
    bool networkMultiplayerMode;
    class NetworkManager* networkManager;  // Forward declaration

    // Planet state tracking for network sync
    std::vector<Planet*> originalPlanets;  // Original planet references
    std::map<int, PlanetState> receivedPlanetStates;  // Network planet updates

    // Cross-network satellite management
    void mergeNetworkSatellites(const std::vector<SatelliteCreationInfo>& networkSatellites);
    void updateSatelliteFromNetworkState(int satelliteID, const PlayerState& networkState);
    void syncNetworkSatellites();
    void handleNetworkPlanetUpdates();

public:
    SatelliteManager();
    ~SatelliteManager() = default;

    // Core management
    void update(float deltaTime);
    void draw(sf::RenderWindow& window);
    void drawWithConstantSize(sf::RenderWindow& window, float zoomLevel);

    // Satellite creation and removal
    int createSatelliteFromRocket(const Rocket* rocket, const SatelliteConversionConfig& config = SatelliteConversionConfig());
    int createSatellite(sf::Vector2f position, sf::Vector2f velocity, const SatelliteConversionConfig& config = SatelliteConversionConfig());
    bool removeSatellite(int satelliteID);
    void removeAllSatellites();

    // Satellite access and queries
    Satellite* getSatellite(int satelliteID);
    const Satellite* getSatellite(int satelliteID) const;
    std::vector<Satellite*> getAllSatellites();
    std::vector<const Satellite*> getAllSatellites() const;
    std::vector<Satellite*> getSatellitesInRange(sf::Vector2f position, float range);
    std::vector<Satellite*> getOperationalSatellites();

    // Network statistics
    const SatelliteNetworkStats& getNetworkStats() const { return networkStats; }
    int getSatelliteCount() const { return static_cast<int>(satellites.size()); }
    int getOperationalSatelliteCount() const;

    // Planet management
    void setPlanets(const std::vector<Planet*>& planetList) { planets = planetList; }
    const std::vector<Planet*>& getPlanets() const { return planets; }

    // Global settings
    void setAutomaticMaintenance(bool enabled) { enableAutomaticMaintenance = enabled; }
    bool getAutomaticMaintenance() const { return enableAutomaticMaintenance; }

    void setAutomaticCollection(bool enabled) { enableAutomaticCollection = enabled; }
    bool getAutomaticCollection() const { return enableAutomaticCollection; }

    void setGlobalOrbitTolerance(float tolerance) { globalOrbitTolerance = tolerance; }
    float getGlobalOrbitTolerance() const { return globalOrbitTolerance; }

    void setCollectionEfficiency(float efficiency) { collectionEfficiency = efficiency; }
    float getCollectionEfficiency() const { return collectionEfficiency; }

    // Visual settings
    void setShowOrbitPaths(bool show) { showOrbitPaths = show; }
    void setShowTargetOrbitPaths(bool show) { showTargetOrbitPaths = show; }
    void setShowFuelTransferLines(bool show) { showFuelTransferLines = show; }
    void setShowMaintenanceBurns(bool show) { showMaintenanceBurns = show; }
    void setShowStatusIndicators(bool show) { showStatusIndicators = show; }

    bool getShowOrbitPaths() const { return showOrbitPaths; }
    bool getShowTargetOrbitPaths() const { return showTargetOrbitPaths; }
    bool getShowFuelTransferLines() const { return showFuelTransferLines; }
    bool getShowMaintenanceBurns() const { return showMaintenanceBurns; }
    bool getShowStatusIndicators() const { return showStatusIndicators; }

    // Fuel transfer operations
    bool transferFuelBetweenSatellites(int fromID, int toID, float amount);
    bool transferFuelToRocket(int satelliteID, Rocket* rocket, float amount);
    bool requestFuelFromNetwork(int satelliteID, float amount); // Emergency fuel request

    // Network optimization
    void optimizeNetworkFuelDistribution();
    void balanceFuelAcrossNetwork();
    void prioritizeMaintenanceFuel();

    // Emergency operations
    void handleLowFuelEmergency(int satelliteID);
    void shutdownNonEssentialSatellites(); // Free up fuel for critical satellites
    void restartSatelliteOperations();     // Re-enable satellites when fuel available

    // Conversion utilities
    bool canConvertRocketToSatellite(const Rocket* rocket) const;
    SatelliteConversionConfig getOptimalConversionConfig(const Rocket* rocket) const;

    // Debugging and monitoring
    void printNetworkStatus() const;
    void printSatelliteDetails(int satelliteID) const;
    std::vector<std::string> getNetworkStatusReport() const;

    // Network multiplayer support - ENHANCED
    void setNetworkManager(NetworkManager* netManager);
    void enableNetworkMode(bool enabled) { networkMultiplayerMode = enabled; }
    bool isNetworkMode() const { return networkMultiplayerMode; }

    // Network satellite creation from remote players
    int createNetworkSatellite(const SatelliteCreationInfo& creationInfo);
    void receiveNetworkSatelliteStates(const std::vector<PlayerState>& satelliteStates);
    void sendNetworkSatelliteStates();

    // Planet state synchronization
    void updateFromNetworkPlanetStates(const std::vector<PlanetState>& planetStates);
    void sendPlanetStatesToNetwork();
    std::vector<PlanetState> getCurrentPlanetStates() const;

    // Cross-player satellite ownership tracking
    void assignSatelliteOwner(int satelliteID, int playerID);
    int getSatelliteOwner(int satelliteID) const;
    std::vector<Satellite*> getSatellitesOwnedByPlayer(int playerID);
    std::vector<Satellite*> getAllNetworkSatellites();  // All satellites regardless of owner

    // Enhanced rocket targeting for multiplayer
    void setAllNetworkRockets(const std::vector<Rocket*>& allRockets);
    void addNetworkRocket(Rocket* rocket, int ownerPlayerID);
    void removeNetworkRocket(Rocket* rocket);

    // Integration with existing systems
    void integrateWithGravitySimulator(class GravitySimulator* gravSim);
    void integrateWithPlayer(Player* player);
    void integrateWithVehicleManager(VehicleManager* vehicleManager);
    // Rocket management for fuel transfers
    void addNearbyRocket(Rocket* rocket);
    void removeNearbyRocket(Rocket* rocket);
    void setNearbyRockets(const std::vector<Rocket*>& rocketList);
};

#endif // SATELLITE_MANAGER_H