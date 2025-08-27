#pragma once
#ifndef FUEL_TRANSFER_NETWORK_H
#define FUEL_TRANSFER_NETWORK_H

#include <vector>
#include <map>
#include <memory>
#include <queue>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>

// Forward declarations
class Satellite;
class Planet;
class Rocket;

// Transfer request structure
struct FuelTransferRequest {
    int fromSatelliteID = -1;        // Source satellite (-1 = from planet)
    int toSatelliteID = -1;          // Target satellite (-1 = to rocket)
    float requestedAmount = 0.0f;    // Amount of fuel to transfer
    float maxTransferRate = 10.0f;   // Maximum transfer rate per second
    int priority = 5;                // Priority level (1=emergency, 10=low priority)
    float timeRemaining = -1.0f;     // Time limit (-1 = no limit)
    bool isEmergency = false;        // Emergency transfer flag

    // For transfers involving rockets or planets
    Rocket* targetRocket = nullptr;
    Planet* sourcePlanet = nullptr;

    // Status tracking
    float transferredAmount = 0.0f;  // Amount already transferred
    bool isActive = false;           // Currently being processed
    bool isComplete = false;         // Transfer completed
    bool isAborted = false;          // Transfer was cancelled/failed

    FuelTransferRequest() = default;
    FuelTransferRequest(int from, int to, float amount, int prio = 5)
        : fromSatelliteID(from), toSatelliteID(to), requestedAmount(amount), priority(prio) {
    }
};
// Network connection between two satellites
struct SatelliteConnection {
    int satellite1ID;
    int satellite2ID;
    float distance;
    float transferEfficiency;    // 0.0-1.0 based on distance and alignment
    bool isActive;              // Connection currently usable
    float maxTransferRate;      // Maximum fuel flow rate

    SatelliteConnection(int id1, int id2, float dist)
        : satellite1ID(id1), satellite2ID(id2), distance(dist),
        transferEfficiency(1.0f), isActive(true), maxTransferRate(5.0f) {
    }
};

// Network optimization algorithms
enum class NetworkOptimizationMode {
    BALANCED,           // Balance fuel across all satellites
    PRIORITY_INNER,     // Prioritize satellites closer to planets
    PRIORITY_OUTER,     // Prioritize satellites farther from planets
    EMERGENCY_ONLY,     // Only emergency transfers
    MAINTENANCE_FIRST,  // Ensure all satellites have maintenance fuel first
    CUSTOM             // User-defined priority system
};

// Network statistics for monitoring
struct NetworkFlowStats {
    float totalFuelInNetwork = 0.0f;
    float totalMaintenanceFuelReserved = 0.0f;
    float totalAvailableFuel = 0.0f;

    // Flow rates (per second)
    float planetToSatelliteFlow = 0.0f;
    float satelliteToSatelliteFlow = 0.0f;
    float satelliteToRocketFlow = 0.0f;
    float maintenanceConsumption = 0.0f;

    // Transfer efficiency
    float averageTransferEfficiency = 0.0f;
    int activeTransfers = 0;
    int queuedTransfers = 0;
    int completedTransfers = 0;
    int failedTransfers = 0;

    void reset() {
        totalFuelInNetwork = totalMaintenanceFuelReserved = totalAvailableFuel = 0.0f;
        planetToSatelliteFlow = satelliteToSatelliteFlow = 0.0f;
        satelliteToRocketFlow = maintenanceConsumption = 0.0f;
        averageTransferEfficiency = 0.0f;
        activeTransfers = queuedTransfers = completedTransfers = failedTransfers = 0;
    }
};

class FuelTransferNetwork {
private:
    // Network topology
    std::vector<Satellite*> satellites;
    std::vector<SatelliteConnection> connections;
    std::map<int, std::vector<int>> connectionMap; // satelliteID -> connected satellite IDs

    // Transfer management
    std::queue<FuelTransferRequest> transferQueue;
    std::vector<FuelTransferRequest> activeTransfers;
    std::vector<FuelTransferRequest> completedTransfers;

    // Network configuration
    float maxTransferRange = 500.0f;        // Maximum distance for fuel transfers
    float baseTransferRate = 5.0f;          // Base fuel transfer rate per second
    float transferEfficiencyFactor = 0.8f;  // Distance-based efficiency reduction
    int maxSimultaneousTransfers = 5;       // Maximum concurrent transfers
    NetworkOptimizationMode optimizationMode = NetworkOptimizationMode::BALANCED;

    // Statistics and monitoring
    NetworkFlowStats flowStats;
    float statsUpdateInterval = 1.0f;
    float timeSinceStatsUpdate = 0.0f;

    // Emergency handling
    float emergencyFuelThreshold = 0.1f;    // 10% fuel = emergency
    float criticalFuelThreshold = 0.05f;    // 5% fuel = critical
    std::vector<int> emergencySatellites;   // Satellites needing emergency fuel

    // Internal methods
    void updateConnections();
    void calculateTransferEfficiency(SatelliteConnection& connection);
    void processTransferQueue(float deltaTime);
    void executeTransfer(FuelTransferRequest& request, float deltaTime);
    void updateFlowStats();

    // Pathfinding and optimization
    std::vector<int> findFuelPath(int fromSatelliteID, int toSatelliteID);
    float calculatePathEfficiency(const std::vector<int>& path);
    void optimizeTransferRoutes();

    // Emergency handling
    void handleEmergencyRequests();
    void redistributeFuelForEmergencies();

public:
    FuelTransferNetwork();
    ~FuelTransferNetwork() = default;

    // Core network management
    void update(float deltaTime);
    void addSatellite(Satellite* satellite);
    void removeSatellite(int satelliteID);
    void clearNetwork();

    // Transfer operations
    bool requestTransfer(const FuelTransferRequest& request);
    bool requestEmergencyTransfer(int toSatelliteID, float amount);
    bool cancelTransfer(int fromSatelliteID, int toSatelliteID);
    void clearTransferQueue();

    // Direct transfer methods
    bool transferFuel(int fromSatelliteID, int toSatelliteID, float amount);
    bool transferFuelFromPlanet(Planet* planet, int toSatelliteID, float amount);
    bool transferFuelToRocket(int fromSatelliteID, Rocket* rocket, float amount);

    // Automatic rocket fuel transfers
    void processAutomaticRocketTransfers(float deltaTime);
    void addNearbyRocket(Rocket* rocket);
    void removeNearbyRocket(Rocket* rocket);
    std::vector<Rocket*> getRocketsInRange(int satelliteID) const;

    // Network queries
    bool areSatellitesConnected(int satellite1ID, int satellite2ID);
    float getConnectionDistance(int satellite1ID, int satellite2ID);
    float getConnectionEfficiency(int satellite1ID, int satellite2ID);
    std::vector<int> getConnectedSatellites(int satelliteID);

    // Network optimization
    void optimizeNetwork();
    void balanceFuelDistribution();
    void prioritizeMaintenanceFuel();
    void setOptimizationMode(NetworkOptimizationMode mode) { optimizationMode = mode; }
    NetworkOptimizationMode getOptimizationMode() const { return optimizationMode; }

    // Configuration
    void setMaxTransferRange(float range) { maxTransferRange = range; }
    float getMaxTransferRange() const { return maxTransferRange; }

    void setBaseTransferRate(float rate) { baseTransferRate = rate; }
    float getBaseTransferRate() const { return baseTransferRate; }

    void setMaxSimultaneousTransfers(int maxTransfers) { maxSimultaneousTransfers = maxTransfers; }
    int getMaxSimultaneousTransfers() const { return maxSimultaneousTransfers; }

    // Emergency thresholds
    void setEmergencyThreshold(float threshold) { emergencyFuelThreshold = threshold; }
    float getEmergencyThreshold() const { return emergencyFuelThreshold; }

    void setCriticalThreshold(float threshold) { criticalFuelThreshold = threshold; }
    float getCriticalThreshold() const { return criticalFuelThreshold; }

    // Statistics and monitoring
    const NetworkFlowStats& getFlowStats() const { return flowStats; }
    std::vector<FuelTransferRequest> getActiveTransfers() const { return activeTransfers; }
    std::vector<FuelTransferRequest> getQueuedTransfers() const;
    int getEmergencySatelliteCount() const { return static_cast<int>(emergencySatellites.size()); }

    // Network health
    bool isNetworkHealthy() const;
    float getNetworkEfficiency() const;
    float getAverageFuelLevel() const;
    std::vector<int> getUnderperformingSatellites() const;

    // Visual debugging
    void drawNetworkConnections(sf::RenderWindow& window);
    void drawTransferFlows(sf::RenderWindow& window);
    void drawEmergencyIndicators(sf::RenderWindow& window);

    // Advanced features
    void simulateNetworkFlow(float timeHorizon, std::vector<NetworkFlowStats>& results);
    void predictFuelDistribution(float timeAhead, std::map<int, float>& predictedLevels);

    // Custom priority system (for NetworkOptimizationMode::CUSTOM)
    void setCustomPriority(int satelliteID, float priority);
    float getCustomPriority(int satelliteID) const;

private:
    // Custom priority storage
    std::map<int, float> customPriorities;

    std::vector<Rocket*> nearbyRockets;        // Rockets available for fuel transfer

    // Advanced algorithms
    void dijkstraFuelPath(int startSatelliteID, std::map<int, float>& distances, std::map<int, int>& previous);
    void greedyFuelDistribution();
    void flowBasedOptimization();

    // Efficiency calculations
    float calculateDistanceEfficiency(float distance) const;
    float calculateOrbitAlignmentEfficiency(int satellite1ID, int satellite2ID) const;
};

#endif // FUEL_TRANSFER_NETWORK_H