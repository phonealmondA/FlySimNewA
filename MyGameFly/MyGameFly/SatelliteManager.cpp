#include "SatelliteManager.h"
#include "FuelTransferNetwork.h"
#include "Player.h"
#include "VehicleManager.h"
#include "GravitySimulator.h"
#include <iostream>
#include <sstream>
#include <algorithm>

SatelliteManager::SatelliteManager()
    : nextSatelliteID(1), statsUpdateInterval(1.0f), timeSinceStatsUpdate(0.0f),
    globalMaintenanceInterval(5.0f), globalOrbitTolerance(50.0f),
    enableAutomaticMaintenance(true), enableAutomaticCollection(true),
    collectionEfficiency(1.0f), showOrbitPaths(true), showTargetOrbitPaths(false),
    showFuelTransferLines(true), showMaintenanceBurns(true), showStatusIndicators(true)
{
    transferNetwork = std::make_unique<FuelTransferNetwork>();
    networkStats.reset();

    std::cout << "SatelliteManager initialized" << std::endl;
}

void SatelliteManager::update(float deltaTime) {
    timeSinceStatsUpdate += deltaTime;

    // Update all satellites
    for (auto& satellite : satellites) {
        if (satellite) {
            // Set nearby planets for each satellite
            satellite->setNearbyPlanets(planets);

            // Update satellite
            satellite->update(deltaTime);
        }
    }

    // Update fuel transfer network
    if (transferNetwork) {
        // Update network with current satellites
        transferNetwork->clearNetwork();
        for (auto& satellite : satellites) {
            if (satellite && satellite->isOperational()) {
                transferNetwork->addSatellite(satellite.get());
            }
        }

        transferNetwork->update(deltaTime);
    }

    // Perform network fuel optimization
    if (enableAutomaticCollection) {
        performNetworkFuelTransfers(deltaTime);
    }

    // Update statistics
    if (timeSinceStatsUpdate >= statsUpdateInterval) {
        updateNetworkStats();
        timeSinceStatsUpdate = 0.0f;
    }

    // Handle emergency situations
    for (auto& satellite : satellites) {
        if (satellite && satellite->getStatus() == SatelliteStatus::CRITICAL_FUEL) {
            handleLowFuelEmergency(satellite->getID());
        }
    }
}

void SatelliteManager::draw(sf::RenderWindow& window) {
    // Draw orbit paths
    if (showOrbitPaths || showTargetOrbitPaths) {
        for (const auto& satellite : satellites) {
            if (satellite) {
                if (showOrbitPaths) {
                    satellite->drawOrbitPath(window, planets);
                }
                if (showTargetOrbitPaths) {
                    satellite->drawTargetOrbitPath(window, planets);
                }
            }
        }
    }

    // Draw satellites
    for (const auto& satellite : satellites) {
        if (satellite) {
            satellite->draw(window);
        }
    }

    // Draw fuel transfer lines
    if (showFuelTransferLines) {
        for (const auto& satellite : satellites) {
            if (satellite) {
                satellite->drawFuelTransferLines(window);
            }
        }

        // Draw network connections
        if (transferNetwork) {
            transferNetwork->drawNetworkConnections(window);
            transferNetwork->drawTransferFlows(window);
        }
    }

    // Draw maintenance burns
    if (showMaintenanceBurns) {
        for (const auto& satellite : satellites) {
            if (satellite && satellite->getStatus() == SatelliteStatus::MAINTENANCE_MODE) {
                satellite->drawMaintenanceBurn(window);
            }
        }
    }
}

void SatelliteManager::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel) {
    // Draw orbit paths (not affected by zoom)
    if (showOrbitPaths || showTargetOrbitPaths) {
        for (const auto& satellite : satellites) {
            if (satellite) {
                if (showOrbitPaths) {
                    satellite->drawOrbitPath(window, planets);
                }
                if (showTargetOrbitPaths) {
                    satellite->drawTargetOrbitPath(window, planets);
                }
            }
        }
    }

    // Draw satellites with constant size
    for (const auto& satellite : satellites) {
        if (satellite) {
            satellite->drawWithConstantSize(window, zoomLevel);

            // Draw status indicators
            if (showStatusIndicators) {
                satellite->drawStatusIndicator(window, zoomLevel);
            }
        }
    }

    // Draw network connections with zoom scaling
    if (showFuelTransferLines && transferNetwork) {
        transferNetwork->drawNetworkConnections(window);
        transferNetwork->drawTransferFlows(window);
        transferNetwork->drawEmergencyIndicators(window);
    }
}

int SatelliteManager::createSatelliteFromRocket(const Rocket* rocket, const SatelliteConversionConfig& config) {
    if (!canConvertRocketToSatellite(rocket)) {
        std::cout << "Cannot convert rocket to satellite - insufficient conditions" << std::endl;
        return -1;
    }

    // Create satellite from rocket
    auto satellite = Satellite::createFromRocket(rocket, nextSatelliteID);
    if (!satellite) {
        std::cout << "Failed to create satellite from rocket" << std::endl;
        return -1;
    }

    // Apply configuration
    if (!config.customName.empty()) {
        satellite->setName(config.customName);
    }

    satellite->setStationKeepingEfficiency(config.stationKeepingEfficiency);

    // Set target orbit based on configuration
    if (config.maintainCurrentOrbit) {
        satellite->setTargetOrbitFromCurrent();
    }

    // Store satellite
    int satelliteID = satellite->getID();
    satelliteMap[satelliteID] = satellite.get();
    satellites.push_back(std::move(satellite));

    // Update next ID
    nextSatelliteID++;

    std::cout << "Created satellite " << satelliteMap[satelliteID]->getName()
        << " (ID: " << satelliteID << ") from rocket conversion" << std::endl;

    return satelliteID;
}

int SatelliteManager::createSatellite(sf::Vector2f position, sf::Vector2f velocity, const SatelliteConversionConfig& config) {
    auto satellite = std::make_unique<Satellite>(
        position, velocity, nextSatelliteID, sf::Color::Cyan, GameConstants::SATELLITE_BASE_MASS
    );

    // Apply configuration
    if (!config.customName.empty()) {
        satellite->setName(config.customName);
    }

    satellite->setStationKeepingEfficiency(config.stationKeepingEfficiency);
    satellite->setTargetOrbitFromCurrent();

    // Store satellite
    int satelliteID = satellite->getID();
    satelliteMap[satelliteID] = satellite.get();
    satellites.push_back(std::move(satellite));

    // Update next ID
    nextSatelliteID++;

    std::cout << "Created satellite " << satelliteMap[satelliteID]->getName()
        << " (ID: " << satelliteID << ") at position (" << position.x << ", " << position.y << ")" << std::endl;

    return satelliteID;
}

bool SatelliteManager::removeSatellite(int satelliteID) {
    auto it = satelliteMap.find(satelliteID);
    if (it == satelliteMap.end()) {
        return false;
    }

    // Remove from map
    std::string satelliteName = it->second->getName();
    satelliteMap.erase(it);

    // Remove from vector
    satellites.erase(
        std::remove_if(satellites.begin(), satellites.end(),
            [satelliteID](const std::unique_ptr<Satellite>& sat) {
                return sat && sat->getID() == satelliteID;
            }),
        satellites.end()
    );

    std::cout << "Removed satellite " << satelliteName << " (ID: " << satelliteID << ")" << std::endl;
    return true;
}

void SatelliteManager::removeAllSatellites() {
    std::cout << "Removing all " << satellites.size() << " satellites" << std::endl;
    satellites.clear();
    satelliteMap.clear();
    networkStats.reset();
}

Satellite* SatelliteManager::getSatellite(int satelliteID) {
    auto it = satelliteMap.find(satelliteID);
    return (it != satelliteMap.end()) ? it->second : nullptr;
}

const Satellite* SatelliteManager::getSatellite(int satelliteID) const {
    auto it = satelliteMap.find(satelliteID);
    return (it != satelliteMap.end()) ? it->second : nullptr;
}

std::vector<Satellite*> SatelliteManager::getAllSatellites() {
    std::vector<Satellite*> result;
    for (auto& satellite : satellites) {
        if (satellite) {
            result.push_back(satellite.get());
        }
    }
    return result;
}

std::vector<const Satellite*> SatelliteManager::getAllSatellites() const {
    std::vector<const Satellite*> result;
    for (const auto& satellite : satellites) {
        if (satellite) {
            result.push_back(satellite.get());
        }
    }
    return result;
}

std::vector<Satellite*> SatelliteManager::getSatellitesInRange(sf::Vector2f position, float range) {
    std::vector<Satellite*> result;

    for (auto& satellite : satellites) {
        if (satellite) {
            float dist = distance(position, satellite->getPosition());
            if (dist <= range) {
                result.push_back(satellite.get());
            }
        }
    }

    return result;
}

std::vector<Satellite*> SatelliteManager::getOperationalSatellites() {
    std::vector<Satellite*> result;

    for (auto& satellite : satellites) {
        if (satellite && satellite->isOperational()) {
            result.push_back(satellite.get());
        }
    }

    return result;
}

int SatelliteManager::getOperationalSatelliteCount() const {
    int count = 0;
    for (const auto& satellite : satellites) {
        if (satellite && satellite->isOperational()) {
            count++;
        }
    }
    return count;
}

void SatelliteManager::updateNetworkStats() {
    networkStats.reset();

    networkStats.totalSatellites = static_cast<int>(satellites.size());

    for (const auto& satellite : satellites) {
        if (!satellite) continue;

        networkStats.totalFuel += satellite->getCurrentFuel();
        networkStats.totalMaintenanceFuel += satellite->getMaintenanceFuelReserve();
        networkStats.totalAvailableFuel += satellite->getAvailableFuel();

        // Count satellites by status
        switch (satellite->getStatus()) {
        case SatelliteStatus::ACTIVE:
        case SatelliteStatus::MAINTENANCE_MODE:
        case SatelliteStatus::TRANSFER_MODE:
            networkStats.activeSatellites++;
            break;
        case SatelliteStatus::LOW_FUEL:
            networkStats.lowFuelSatellites++;
            break;
        case SatelliteStatus::CRITICAL_FUEL:
            networkStats.criticalFuelSatellites++;
            break;
        case SatelliteStatus::FUEL_DEPLETED:
            networkStats.depletedSatellites++;
            break;
        }

        // Average orbit accuracy
        networkStats.averageOrbitAccuracy += satellite->getOrbitAccuracy();
    }

    // Calculate averages
    if (networkStats.totalSatellites > 0) {
        networkStats.averageOrbitAccuracy /= networkStats.totalSatellites;
    }

    // Get flow stats from network
    if (transferNetwork) {
        const auto& flowStats = transferNetwork->getFlowStats();
        networkStats.fuelCollectionRate = flowStats.planetToSatelliteFlow;
        networkStats.fuelConsumptionRate = flowStats.maintenanceConsumption;
        networkStats.fuelTransferRate = flowStats.satelliteToSatelliteFlow;
    }
}

void SatelliteManager::performNetworkFuelTransfers(float deltaTime) {
    if (!transferNetwork || !enableAutomaticCollection) return;

    // Let the transfer network handle optimization
    transferNetwork->optimizeNetwork();
    transferNetwork->balanceFuelDistribution();
}

void SatelliteManager::optimizeFuelDistribution() {
    if (!transferNetwork) return;

    transferNetwork->optimizeNetwork();
    transferNetwork->balanceFuelDistribution();
    transferNetwork->prioritizeMaintenanceFuel();
}

std::string SatelliteManager::generateSatelliteName(int id) const {
    std::stringstream ss;
    ss << "SAT-" << std::setfill('0') << std::setw(3) << id;
    return ss.str();
}

bool SatelliteManager::transferFuelBetweenSatellites(int fromID, int toID, float amount) {
    Satellite* fromSat = getSatellite(fromID);
    Satellite* toSat = getSatellite(toID);

    if (!fromSat || !toSat) return false;

    return fromSat->transferFuelTo(toSat, amount);
}

bool SatelliteManager::transferFuelToRocket(int satelliteID, Rocket* rocket, float amount) {
    Satellite* satellite = getSatellite(satelliteID);
    if (!satellite || !rocket) return false;

    return satellite->transferFuelToRocket(rocket, amount);
}

bool SatelliteManager::requestFuelFromNetwork(int satelliteID, float amount) {
    if (!transferNetwork) return false;

    return transferNetwork->requestEmergencyTransfer(satelliteID, amount);
}

void SatelliteManager::optimizeNetworkFuelDistribution() {
    optimizeFuelDistribution();
}

void SatelliteManager::balanceFuelAcrossNetwork() {
    if (!transferNetwork) return;

    transferNetwork->balanceFuelDistribution();
}

void SatelliteManager::prioritizeMaintenanceFuel() {
    if (!transferNetwork) return;

    transferNetwork->prioritizeMaintenanceFuel();
}

void SatelliteManager::handleLowFuelEmergency(int satelliteID) {
    Satellite* satellite = getSatellite(satelliteID);
    if (!satellite) return;

    std::cout << "Handling fuel emergency for satellite " << satellite->getName() << std::endl;

    // Request emergency fuel from network
    float fuelNeeded = satellite->getMaxFuel() * 0.3f; // Request 30% capacity
    requestFuelFromNetwork(satelliteID, fuelNeeded);
}

void SatelliteManager::shutdownNonEssentialSatellites() {
    // Find satellites with lowest priority and shutdown to conserve fuel
    std::vector<Satellite*> lowPrioritySatellites;

    for (auto& satellite : satellites) {
        if (satellite && satellite->getStatus() == SatelliteStatus::ACTIVE) {
            // Simple priority: satellites farther from planets are lower priority
            float minDistanceToPlanet = std::numeric_limits<float>::max();
            for (const auto* planet : planets) {
                float dist = distance(satellite->getPosition(), planet->getPosition());
                minDistanceToPlanet = std::min(minDistanceToPlanet, dist);
            }

            if (minDistanceToPlanet > 1000.0f) { // Arbitrary threshold
                lowPrioritySatellites.push_back(satellite.get());
            }
        }
    }

    // Transfer fuel from low priority satellites to critical ones
    for (auto* satellite : lowPrioritySatellites) {
        float availableFuel = satellite->getAvailableFuel();
        if (availableFuel > 5.0f) { // Keep minimal fuel
            // Find a critical satellite to transfer to
            for (auto& criticalSat : satellites) {
                if (criticalSat && criticalSat->getStatus() == SatelliteStatus::CRITICAL_FUEL) {
                    satellite->transferFuelTo(criticalSat.get(), availableFuel * 0.8f);
                    break;
                }
            }
        }
    }

    std::cout << "Emergency fuel redistribution completed for "
        << lowPrioritySatellites.size() << " satellites" << std::endl;
}

void SatelliteManager::restartSatelliteOperations() {
    std::cout << "Restarting satellite operations" << std::endl;

    // Re-enable automatic systems
    enableAutomaticMaintenance = true;
    enableAutomaticCollection = true;

    // Reset any emergency states
    for (auto& satellite : satellites) {
        if (satellite && satellite->getCurrentFuel() > GameConstants::SATELLITE_MINIMUM_MAINTENANCE_FUEL) {
            // Satellite has enough fuel to resume operations
            // Status will be updated automatically in next update cycle
        }
    }
}

bool SatelliteManager::canConvertRocketToSatellite(const Rocket* rocket) const {
    if (!rocket) return false;

    // Check if rocket has minimum fuel for conversion
    if (rocket->getCurrentFuel() < 10.0f) return false;

    // Check if rocket is at sufficient altitude
    float minAltitude = GameConstants::SATELLITE_MIN_CONVERSION_ALTITUDE;
    for (const auto* planet : planets) {
        float altitude = distance(rocket->getPosition(), planet->getPosition()) - planet->getRadius();
        if (altitude < minAltitude) return false;
    }

    return true;
}

SatelliteConversionConfig SatelliteManager::getOptimalConversionConfig(const Rocket* rocket) const {
    SatelliteConversionConfig config;

    if (!rocket) return config;

    // Calculate optimal maintenance fuel percentage based on current orbit
    float totalFuel = rocket->getCurrentFuel() * GameConstants::SATELLITE_CONVERSION_FUEL_RETENTION;

    // Higher altitude orbits need more maintenance fuel
    float avgAltitude = 0.0f;
    for (const auto* planet : planets) {
        avgAltitude += distance(rocket->getPosition(), planet->getPosition()) - planet->getRadius();
    }
    avgAltitude /= planets.size();

    // Adjust maintenance fuel percentage based on altitude
    if (avgAltitude > 500.0f) {
        config.maintenanceFuelPercent = 0.25f; // 25% for high orbits
    }
    else if (avgAltitude > 200.0f) {
        config.maintenanceFuelPercent = 0.2f;  // 20% for medium orbits
    }
    else {
        config.maintenanceFuelPercent = 0.15f; // 15% for low orbits
    }

    // Set efficiency based on rocket's current thrust level
    config.stationKeepingEfficiency = std::max(0.8f, rocket->getThrustLevel() + 0.5f);

    // Generate optimal name
    config.customName = generateSatelliteName(nextSatelliteID);

    return config;
}

void SatelliteManager::printNetworkStatus() const {
    std::cout << "\n=== SATELLITE NETWORK STATUS ===" << std::endl;
    std::cout << "Total Satellites: " << networkStats.totalSatellites << std::endl;
    std::cout << "Active: " << networkStats.activeSatellites << ", Low Fuel: " << networkStats.lowFuelSatellites
        << ", Critical: " << networkStats.criticalFuelSatellites << ", Depleted: " << networkStats.depletedSatellites << std::endl;
    std::cout << "Total Fuel: " << networkStats.totalFuel << ", Available: " << networkStats.totalAvailableFuel << std::endl;
    std::cout << "Maintenance Reserve: " << networkStats.totalMaintenanceFuel << std::endl;
    std::cout << "Average Orbit Accuracy: " << networkStats.averageOrbitAccuracy << "%" << std::endl;
    std::cout << "Fuel Collection Rate: " << networkStats.fuelCollectionRate << " units/sec" << std::endl;
    std::cout << "Fuel Consumption Rate: " << networkStats.fuelConsumptionRate << " units/sec" << std::endl;
    std::cout << "================================\n" << std::endl;
}

void SatelliteManager::printSatelliteDetails(int satelliteID) const {
    const Satellite* satellite = getSatellite(satelliteID);
    if (!satellite) {
        std::cout << "Satellite ID " << satelliteID << " not found" << std::endl;
        return;
    }

    std::cout << "\n=== SATELLITE DETAILS ===" << std::endl;
    std::cout << "Name: " << satellite->getName() << " (ID: " << satelliteID << ")" << std::endl;
    std::cout << "Status: ";

    switch (satellite->getStatus()) {
    case SatelliteStatus::ACTIVE: std::cout << "Active"; break;
    case SatelliteStatus::LOW_FUEL: std::cout << "Low Fuel"; break;
    case SatelliteStatus::CRITICAL_FUEL: std::cout << "Critical Fuel"; break;
    case SatelliteStatus::FUEL_DEPLETED: std::cout << "Fuel Depleted"; break;
    case SatelliteStatus::MAINTENANCE_MODE: std::cout << "Maintenance Mode"; break;
    case SatelliteStatus::TRANSFER_MODE: std::cout << "Transfer Mode"; break;
    }
    std::cout << std::endl;

    std::cout << "Position: (" << satellite->getPosition().x << ", " << satellite->getPosition().y << ")" << std::endl;
    std::cout << "Fuel: " << satellite->getCurrentFuel() << "/" << satellite->getMaxFuel()
        << " (" << satellite->getFuelPercentage() << "%)" << std::endl;
    std::cout << "Maintenance Reserve: " << satellite->getMaintenanceFuelReserve()
        << " (" << satellite->getMaintenanceFuelPercentage() << "%)" << std::endl;
    std::cout << "Available for Transfer: " << satellite->getAvailableFuel() << std::endl;
    std::cout << "Mass: " << satellite->getMass() << "/" << satellite->getMaxMass() << std::endl;
    std::cout << "Orbit Accuracy: " << satellite->getOrbitAccuracy() << "%" << std::endl;
    std::cout << "========================\n" << std::endl;
}

std::vector<std::string> SatelliteManager::getNetworkStatusReport() const {
    std::vector<std::string> report;

    // Network summary
    std::stringstream ss;
    ss << "Network Status: " << networkStats.activeSatellites << "/" << networkStats.totalSatellites << " operational";
    report.push_back(ss.str());

    ss.str("");
    ss << "Fuel Status: " << std::fixed << std::setprecision(1) << networkStats.totalAvailableFuel
        << " available, " << networkStats.totalMaintenanceFuel << " reserved";
    report.push_back(ss.str());

    ss.str("");
    ss << "Network Health: " << std::setprecision(0) << networkStats.averageOrbitAccuracy << "% avg accuracy";
    report.push_back(ss.str());

    // Critical alerts
    if (networkStats.criticalFuelSatellites > 0) {
        ss.str("");
        ss << "ALERT: " << networkStats.criticalFuelSatellites << " satellites critically low on fuel";
        report.push_back(ss.str());
    }

    if (networkStats.depletedSatellites > 0) {
        ss.str("");
        ss << "WARNING: " << networkStats.depletedSatellites << " satellites fuel depleted";
        report.push_back(ss.str());
    }

    return report;
}

void SatelliteManager::integrateWithGravitySimulator(GravitySimulator* gravSim) {
    if (!gravSim) return;

    // Add all operational satellites to gravity simulation
    for (auto& satellite : satellites) {
        if (satellite && satellite->isOperational()) {
            // Note: This would require extending GravitySimulator to handle satellites
            // For now, satellites handle their own orbital mechanics
        }
    }
}

void SatelliteManager::integrateWithPlayer(Player* player) {
    if (!player) return;

    // Set up planets for satellite fuel collection
    auto vehicleManager = player->getVehicleManager();
    if (vehicleManager && vehicleManager->getRocket()) {
        // Enable fuel transfers between satellites and player's rocket
        // This integration would allow the player to request fuel from satellites
    }
}

void SatelliteManager::integrateWithVehicleManager(VehicleManager* vehicleManager) {
    if (!vehicleManager) return;

    // Enable satellite-to-rocket fuel transfers
    if (vehicleManager->getRocket()) {
        // Find nearby satellites and enable fuel transfer options
        auto nearbySatellites = getSatellitesInRange(
            vehicleManager->getRocket()->getPosition(),
            GameConstants::SATELLITE_ROCKET_DOCKING_RANGE
        );

        // This integration would allow automatic fuel transfer from satellites to rockets
        for (auto* satellite : nearbySatellites) {
            if (satellite->getAvailableFuel() > 10.0f) {
                // Potential for automatic fuel transfer
            }
        }
    }
}