#include "FuelTransferNetwork.h"
#include "Satellite.h"
#include "Planet.h"
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <algorithm>
#include <queue>
#include <limits>
#include <cmath>
#include <iostream>

FuelTransferNetwork::FuelTransferNetwork()
    : maxTransferRange(GameConstants::SATELLITE_TRANSFER_RANGE),
    baseTransferRate(GameConstants::SATELLITE_BASE_TRANSFER_RATE),
    transferEfficiencyFactor(GameConstants::SATELLITE_TRANSFER_EFFICIENCY),
    maxSimultaneousTransfers(GameConstants::SATELLITE_MAX_SIMULTANEOUS_TRANSFERS),
    optimizationMode(NetworkOptimizationMode::BALANCED),
    statsUpdateInterval(1.0f), timeSinceStatsUpdate(0.0f),
    emergencyFuelThreshold(GameConstants::SATELLITE_EMERGENCY_FUEL_THRESHOLD),
    criticalFuelThreshold(GameConstants::SATELLITE_CRITICAL_FUEL_THRESHOLD)
{
    flowStats.reset();
    std::cout << "FuelTransferNetwork initialized" << std::endl;
}

void FuelTransferNetwork::update(float deltaTime) {
    timeSinceStatsUpdate += deltaTime;

    // Update network connections
    updateConnections();

    // Process transfer queue
    processTransferQueue(deltaTime);

    // Handle emergency requests
    handleEmergencyRequests();

    // Update flow statistics
    if (timeSinceStatsUpdate >= statsUpdateInterval) {
        updateFlowStats();
        timeSinceStatsUpdate = 0.0f;
    }

    // Optimize network performance
    optimizeTransferRoutes();
}

void FuelTransferNetwork::addSatellite(Satellite* satellite) {
    if (!satellite) return;

    // Check if satellite is already in network
    auto it = std::find(satellites.begin(), satellites.end(), satellite);
    if (it != satellites.end()) return;

    satellites.push_back(satellite);

    // Update connections
    updateConnections();

    std::cout << "Added satellite " << satellite->getName() << " to fuel transfer network" << std::endl;
}

void FuelTransferNetwork::removeSatellite(int satelliteID) {
    // Find and remove satellite
    satellites.erase(
        std::remove_if(satellites.begin(), satellites.end(),
            [satelliteID](Satellite* sat) {
                return sat && sat->getID() == satelliteID;
            }),
        satellites.end()
    );

    // Remove from connection map
    connectionMap.erase(satelliteID);

    // Remove connections involving this satellite
    connections.erase(
        std::remove_if(connections.begin(), connections.end(),
            [satelliteID](const SatelliteConnection& conn) {
                return conn.satellite1ID == satelliteID || conn.satellite2ID == satelliteID;
            }),
        connections.end()
    );

    // Remove from emergency list
    emergencySatellites.erase(
        std::remove(emergencySatellites.begin(), emergencySatellites.end(), satelliteID),
        emergencySatellites.end()
    );

    // Cancel transfers involving this satellite
    transferQueue = std::queue<FuelTransferRequest>(); // Clear queue
    activeTransfers.erase(
        std::remove_if(activeTransfers.begin(), activeTransfers.end(),
            [satelliteID](const FuelTransferRequest& req) {
                return req.fromSatelliteID == satelliteID || req.toSatelliteID == satelliteID;
            }),
        activeTransfers.end()
    );

    std::cout << "Removed satellite ID " << satelliteID << " from fuel transfer network" << std::endl;
}

void FuelTransferNetwork::clearNetwork() {
    satellites.clear();
    connections.clear();
    connectionMap.clear();
    emergencySatellites.clear();
    transferQueue = std::queue<FuelTransferRequest>();
    activeTransfers.clear();
    completedTransfers.clear();
    flowStats.reset();
}

void FuelTransferNetwork::updateConnections() {
    connections.clear();
    connectionMap.clear();

    // Create connections between all satellite pairs within range
    for (size_t i = 0; i < satellites.size(); i++) {
        if (!satellites[i]) continue;

        connectionMap[satellites[i]->getID()] = std::vector<int>();

        for (size_t j = i + 1; j < satellites.size(); j++) {
            if (!satellites[j]) continue;

            float dist = distance(satellites[i]->getPosition(), satellites[j]->getPosition());

            if (dist <= maxTransferRange) {
                // Create connection
                SatelliteConnection connection(satellites[i]->getID(), satellites[j]->getID(), dist);
                calculateTransferEfficiency(connection);
                connections.push_back(connection);

                // Update connection map
                connectionMap[satellites[i]->getID()].push_back(satellites[j]->getID());
                connectionMap[satellites[j]->getID()].push_back(satellites[i]->getID());
            }
        }
    }
}

void FuelTransferNetwork::calculateTransferEfficiency(SatelliteConnection& connection) {
    // Distance-based efficiency
    float distanceEff = calculateDistanceEfficiency(connection.distance);

    // Orbital alignment efficiency (simplified - could be more sophisticated)
    float alignmentEff = calculateOrbitAlignmentEfficiency(connection.satellite1ID, connection.satellite2ID);

    // Combined efficiency
    connection.transferEfficiency = distanceEff * alignmentEff;
    connection.maxTransferRate = baseTransferRate * connection.transferEfficiency;
    connection.isActive = connection.transferEfficiency > 0.1f; // Minimum 10% efficiency
}

float FuelTransferNetwork::calculateDistanceEfficiency(float distance) const {
    if (distance <= 0.0f) return 1.0f;
    if (distance >= maxTransferRange) return 0.0f;

    // Linear efficiency decrease with distance
    return 1.0f - (distance / maxTransferRange) * (1.0f - transferEfficiencyFactor);
}

float FuelTransferNetwork::calculateOrbitAlignmentEfficiency(int satellite1ID, int satellite2ID) const {
    // Find satellites
    Satellite* sat1 = nullptr;
    Satellite* sat2 = nullptr;

    for (Satellite* sat : satellites) {
        if (sat && sat->getID() == satellite1ID) sat1 = sat;
        if (sat && sat->getID() == satellite2ID) sat2 = sat;
    }

    if (!sat1 || !sat2) return 0.5f; // Default efficiency

    // Calculate relative velocity (simplified orbital alignment check)
    sf::Vector2f relativeVel = sat1->getVelocity() - sat2->getVelocity();
    float relativeSpeed = std::sqrt(relativeVel.x * relativeVel.x + relativeVel.y * relativeVel.y);

    // Lower relative speed = better alignment = higher efficiency
    float alignmentFactor = 1.0f / (1.0f + relativeSpeed * 0.01f);
    return std::clamp(alignmentFactor, 0.3f, 1.0f);
}

void FuelTransferNetwork::processTransferQueue(float deltaTime) {
    // Process active transfers
    for (auto it = activeTransfers.begin(); it != activeTransfers.end();) {
        executeTransfer(*it, deltaTime);

        if (it->isComplete || it->isAborted) {
            completedTransfers.push_back(*it);
            it = activeTransfers.erase(it);
        }
        else {
            ++it;
        }
    }

    // Start new transfers from queue
    while (!transferQueue.empty() &&
        static_cast<int>(activeTransfers.size()) < maxSimultaneousTransfers) {

        FuelTransferRequest request = transferQueue.front();
        transferQueue.pop();

        // Validate request
        if (request.fromSatelliteID >= 0 && request.toSatelliteID >= 0) {
            if (areSatellitesConnected(request.fromSatelliteID, request.toSatelliteID)) {
                request.isActive = true;
                activeTransfers.push_back(request);
            }
            else {
                request.isAborted = true;
                completedTransfers.push_back(request);
            }
        }
    }
}

void FuelTransferNetwork::executeTransfer(FuelTransferRequest& request, float deltaTime) {
    // Find source and target satellites
    Satellite* fromSat = nullptr;
    Satellite* toSat = nullptr;

    for (Satellite* sat : satellites) {
        if (sat && sat->getID() == request.fromSatelliteID) fromSat = sat;
        if (sat && sat->getID() == request.toSatelliteID) toSat = sat;
    }

    if (!fromSat || !toSat) {
        request.isAborted = true;
        return;
    }

    // Check if satellites are still connected
    if (!areSatellitesConnected(request.fromSatelliteID, request.toSatelliteID)) {
        request.isAborted = true;
        return;
    }

    // Calculate transfer amount for this frame
    float connectionEfficiency = getConnectionEfficiency(request.fromSatelliteID, request.toSatelliteID);
    float actualTransferRate = request.maxTransferRate * connectionEfficiency;
    float transferAmount = actualTransferRate * deltaTime;

    // Limit by remaining amount needed
    float remainingAmount = request.requestedAmount - request.transferredAmount;
    transferAmount = std::min(transferAmount, remainingAmount);

    // Attempt the transfer
    if (fromSat->transferFuelTo(toSat, transferAmount)) {
        request.transferredAmount += transferAmount;
        flowStats.satelliteToSatelliteFlow += transferAmount / deltaTime;

        // Check if transfer is complete
        if (request.transferredAmount >= request.requestedAmount * 0.99f) {
            request.isComplete = true;
        }
    }
    else {
        // Transfer failed (insufficient fuel, capacity, etc.)
        request.isAborted = true;
    }

    // Check timeout
    if (request.timeRemaining > 0.0f) {
        request.timeRemaining -= deltaTime;
        if (request.timeRemaining <= 0.0f) {
            request.isAborted = true;
        }
    }
}

void FuelTransferNetwork::handleEmergencyRequests() {
    emergencySatellites.clear();

    // Identify satellites in emergency state
    for (Satellite* sat : satellites) {
        if (!sat) continue;

        float fuelPercent = sat->getFuelPercentage() / 100.0f;

        if (fuelPercent <= criticalFuelThreshold) {
            emergencySatellites.push_back(sat->getID());
        }
    }

    // Process emergency transfers
    for (int emergencyID : emergencySatellites) {
        Satellite* emergencySat = nullptr;
        for (Satellite* sat : satellites) {
            if (sat && sat->getID() == emergencyID) {
                emergencySat = sat;
                break;
            }
        }

        if (!emergencySat) continue;

        // Find the best donor satellite
        Satellite* bestDonor = nullptr;
        float bestScore = 0.0f;

        for (Satellite* donor : satellites) {
            if (!donor || donor->getID() == emergencyID) continue;
            if (donor->getAvailableFuel() < 10.0f) continue; // Must have fuel to spare

            if (areSatellitesConnected(donor->getID(), emergencyID)) {
                float efficiency = getConnectionEfficiency(donor->getID(), emergencyID);
                float availableFuel = donor->getAvailableFuel();
                float score = efficiency * availableFuel;

                if (score > bestScore) {
                    bestScore = score;
                    bestDonor = donor;
                }
            }
        }

        if (bestDonor) {
            // Create emergency transfer request
            float transferAmount = std::min(bestDonor->getAvailableFuel() * 0.5f,
                emergencySat->getMaxFuel() * 0.3f);

            FuelTransferRequest emergencyRequest(bestDonor->getID(), emergencyID, transferAmount, 1);
            emergencyRequest.isEmergency = true;
            emergencyRequest.maxTransferRate = baseTransferRate * 2.0f; // Faster emergency transfers

            requestTransfer(emergencyRequest);
        }
    }
}

void FuelTransferNetwork::updateFlowStats() {
    flowStats.reset();

    // Count satellites and calculate fuel totals
    for (Satellite* sat : satellites) {
        if (!sat) continue;

        flowStats.totalFuelInNetwork += sat->getCurrentFuel();
        flowStats.totalMaintenanceFuelReserved += sat->getMaintenanceFuelReserve();
        flowStats.totalAvailableFuel += sat->getAvailableFuel();
    }

    // Count transfers
    flowStats.activeTransfers = static_cast<int>(activeTransfers.size());
    flowStats.queuedTransfers = static_cast<int>(transferQueue.size());

    // Calculate transfer efficiency
    if (!connections.empty()) {
        float totalEfficiency = 0.0f;
        int activeConnections = 0;

        for (const auto& conn : connections) {
            if (conn.isActive) {
                totalEfficiency += conn.transferEfficiency;
                activeConnections++;
            }
        }

        if (activeConnections > 0) {
            flowStats.averageTransferEfficiency = totalEfficiency / activeConnections;
        }
    }

    // Count completed transfers
    flowStats.completedTransfers = static_cast<int>(completedTransfers.size());

    // Count failed transfers
    flowStats.failedTransfers = 0;
    for (const auto& transfer : completedTransfers) {
        if (transfer.isAborted && !transfer.isComplete) {
            flowStats.failedTransfers++;
        }
    }
}

bool FuelTransferNetwork::requestTransfer(const FuelTransferRequest& request) {
    if (request.requestedAmount <= 0.0f) return false;

    // Validate satellite IDs
    bool fromValid = (request.fromSatelliteID < 0) || // Planet source
        std::any_of(satellites.begin(), satellites.end(),
            [&request](Satellite* sat) {
                return sat && sat->getID() == request.fromSatelliteID;
            });

    bool toValid = (request.toSatelliteID < 0) || // Rocket target
        std::any_of(satellites.begin(), satellites.end(),
            [&request](Satellite* sat) {
                return sat && sat->getID() == request.toSatelliteID;
            });

    if (!fromValid || !toValid) return false;

    // Add to queue with priority handling
    if (request.isEmergency) {
        // Emergency transfers go to front of queue
        std::queue<FuelTransferRequest> tempQueue;
        tempQueue.push(request);

        while (!transferQueue.empty()) {
            tempQueue.push(transferQueue.front());
            transferQueue.pop();
        }

        transferQueue = tempQueue;
    }
    else {
        transferQueue.push(request);
    }

    std::cout << "Fuel transfer request queued: " << request.requestedAmount
        << " units from " << request.fromSatelliteID << " to " << request.toSatelliteID << std::endl;

    return true;
}

bool FuelTransferNetwork::requestEmergencyTransfer(int toSatelliteID, float amount) {
    FuelTransferRequest request;
    request.toSatelliteID = toSatelliteID;
    request.requestedAmount = amount;
    request.priority = 1; // Highest priority
    request.isEmergency = true;
    request.maxTransferRate = baseTransferRate * 3.0f; // Triple speed for emergencies

    return requestTransfer(request);
}

bool FuelTransferNetwork::cancelTransfer(int fromSatelliteID, int toSatelliteID) {
    // Remove from queue
    std::queue<FuelTransferRequest> tempQueue;
    bool found = false;

    while (!transferQueue.empty()) {
        auto request = transferQueue.front();
        transferQueue.pop();

        if (request.fromSatelliteID == fromSatelliteID && request.toSatelliteID == toSatelliteID) {
            found = true;
            // Don't add back to queue
        }
        else {
            tempQueue.push(request);
        }
    }

    transferQueue = tempQueue;

    // Abort active transfers
    for (auto& transfer : activeTransfers) {
        if (transfer.fromSatelliteID == fromSatelliteID && transfer.toSatelliteID == toSatelliteID) {
            transfer.isAborted = true;
            found = true;
        }
    }

    return found;
}

void FuelTransferNetwork::clearTransferQueue() {
    transferQueue = std::queue<FuelTransferRequest>();

    // Abort all active transfers
    for (auto& transfer : activeTransfers) {
        transfer.isAborted = true;
    }

    std::cout << "Fuel transfer queue cleared" << std::endl;
}

bool FuelTransferNetwork::transferFuel(int fromSatelliteID, int toSatelliteID, float amount) {
    FuelTransferRequest request(fromSatelliteID, toSatelliteID, amount);
    return requestTransfer(request);
}

bool FuelTransferNetwork::transferFuelFromPlanet(Planet* planet, int toSatelliteID, float amount) {
    if (!planet) return false;

    FuelTransferRequest request;
    request.fromSatelliteID = -1; // Planet source
    request.toSatelliteID = toSatelliteID;
    request.requestedAmount = amount;
    request.sourcePlanet = planet;

    return requestTransfer(request);
}

bool FuelTransferNetwork::transferFuelToRocket(int fromSatelliteID, Rocket* rocket, float amount) {
    if (!rocket) return false;

    FuelTransferRequest request;
    request.fromSatelliteID = fromSatelliteID;
    request.toSatelliteID = -1; // Rocket target
    request.requestedAmount = amount;
    request.targetRocket = rocket;

    return requestTransfer(request);
}

bool FuelTransferNetwork::areSatellitesConnected(int satellite1ID, int satellite2ID) {
    auto it = connectionMap.find(satellite1ID);
    if (it == connectionMap.end()) return false;

    const auto& connections = it->second;
    return std::find(connections.begin(), connections.end(), satellite2ID) != connections.end();
}

float FuelTransferNetwork::getConnectionDistance(int satellite1ID, int satellite2ID) {
    for (const auto& conn : connections) {
        if ((conn.satellite1ID == satellite1ID && conn.satellite2ID == satellite2ID) ||
            (conn.satellite1ID == satellite2ID && conn.satellite2ID == satellite1ID)) {
            return conn.distance;
        }
    }
    return std::numeric_limits<float>::max();
}

float FuelTransferNetwork::getConnectionEfficiency(int satellite1ID, int satellite2ID) {
    for (const auto& conn : connections) {
        if ((conn.satellite1ID == satellite1ID && conn.satellite2ID == satellite2ID) ||
            (conn.satellite1ID == satellite2ID && conn.satellite2ID == satellite1ID)) {
            return conn.transferEfficiency;
        }
    }
    return 0.0f;
}

std::vector<int> FuelTransferNetwork::getConnectedSatellites(int satelliteID) {
    auto it = connectionMap.find(satelliteID);
    if (it != connectionMap.end()) {
        return it->second;
    }
    return std::vector<int>();
}

void FuelTransferNetwork::optimizeNetwork() {
    switch (optimizationMode) {
    case NetworkOptimizationMode::BALANCED:
        balanceFuelDistribution();
        break;
    case NetworkOptimizationMode::PRIORITY_INNER:
        // Prioritize satellites closer to planets
        // Implementation would require planet references
        break;
    case NetworkOptimizationMode::PRIORITY_OUTER:
        // Prioritize satellites farther from planets
        break;
    case NetworkOptimizationMode::EMERGENCY_ONLY:
        // Only process emergency transfers
        break;
    case NetworkOptimizationMode::MAINTENANCE_FIRST:
        prioritizeMaintenanceFuel();
        break;
    case NetworkOptimizationMode::CUSTOM:
        // Use custom priority system
        break;
    }
}

void FuelTransferNetwork::balanceFuelDistribution() {
    if (satellites.size() < 2) return;

    // Calculate average available fuel
    float totalAvailableFuel = 0.0f;
    int operationalSatellites = 0;

    for (Satellite* sat : satellites) {
        if (sat && sat->isOperational()) {
            totalAvailableFuel += sat->getAvailableFuel();
            operationalSatellites++;
        }
    }

    if (operationalSatellites == 0) return;

    float targetFuelPerSatellite = totalAvailableFuel / operationalSatellites;

    // Find satellites with excess and deficit fuel
    std::vector<std::pair<int, float>> excessSatellites;
    std::vector<std::pair<int, float>> deficitSatellites;

    for (Satellite* sat : satellites) {
        if (!sat || !sat->isOperational()) continue;

        float availableFuel = sat->getAvailableFuel();
        float difference = availableFuel - targetFuelPerSatellite;

        if (difference > 5.0f) { // Has excess (threshold to avoid micro-transfers)
            excessSatellites.push_back({ sat->getID(), difference });
        }
        else if (difference < -5.0f) { // Has deficit
            deficitSatellites.push_back({ sat->getID(), -difference });
        }
    }

    // Create transfer requests to balance fuel
    for (const auto& excess : excessSatellites) {
        for (const auto& deficit : deficitSatellites) {
            if (areSatellitesConnected(excess.first, deficit.first)) {
                float transferAmount = std::min(excess.second * 0.5f, deficit.second * 0.5f);

                if (transferAmount > 1.0f) { // Minimum transfer threshold
                    FuelTransferRequest request(excess.first, deficit.first, transferAmount, 5);
                    requestTransfer(request);
                }
            }
        }
    }
}

void FuelTransferNetwork::prioritizeMaintenanceFuel() {
    // Ensure all satellites have sufficient maintenance fuel reserves
    for (Satellite* sat : satellites) {
        if (!sat || !sat->isOperational()) continue;

        float currentMaintenance = sat->getMaintenanceFuelReserve();
        float requiredMaintenance = sat->getMaxFuel() * GameConstants::SATELLITE_MAINTENANCE_FUEL_PERCENT;

        if (currentMaintenance < requiredMaintenance * 0.8f) {
            // Satellite needs more maintenance fuel
            float needed = requiredMaintenance - currentMaintenance;

            // Find a donor satellite with excess fuel
            for (Satellite* donor : satellites) {
                if (!donor || donor->getID() == sat->getID()) continue;
                if (donor->getAvailableFuel() < needed * 1.5f) continue;

                if (areSatellitesConnected(donor->getID(), sat->getID())) {
                    FuelTransferRequest request(donor->getID(), sat->getID(), needed, 2);
                    request.isEmergency = true; // High priority for maintenance
                    requestTransfer(request);
                    break;
                }
            }
        }
    }
}

bool FuelTransferNetwork::isNetworkHealthy() const {
    if (satellites.empty()) return true;

    int healthySatellites = 0;
    for (Satellite* sat : satellites) {
        if (sat && sat->isOperational() && sat->getFuelPercentage() > 20.0f) {
            healthySatellites++;
        }
    }

    float healthyRatio = static_cast<float>(healthySatellites) / satellites.size();
    return healthyRatio >= 0.8f; // 80% of satellites should be healthy
}

float FuelTransferNetwork::getNetworkEfficiency() const {
    return flowStats.averageTransferEfficiency;
}

float FuelTransferNetwork::getAverageFuelLevel() const {
    if (satellites.empty()) return 0.0f;

    float totalFuelPercent = 0.0f;
    for (Satellite* sat : satellites) {
        if (sat) {
            totalFuelPercent += sat->getFuelPercentage();
        }
    }

    return totalFuelPercent / satellites.size();
}

std::vector<int> FuelTransferNetwork::getUnderperformingSatellites() const {
    std::vector<int> underperforming;

    for (Satellite* sat : satellites) {
        if (!sat) continue;

        if (sat->getFuelPercentage() < 30.0f ||
            sat->getOrbitAccuracy() < 70.0f ||
            sat->getStatus() == SatelliteStatus::CRITICAL_FUEL) {
            underperforming.push_back(sat->getID());
        }
    }

    return underperforming;
}

std::vector<FuelTransferRequest> FuelTransferNetwork::getQueuedTransfers() const {
    std::vector<FuelTransferRequest> queuedTransfers;
    std::queue<FuelTransferRequest> tempQueue = transferQueue;

    while (!tempQueue.empty()) {
        queuedTransfers.push_back(tempQueue.front());
        tempQueue.pop();
    }

    return queuedTransfers;
}

void FuelTransferNetwork::optimizeTransferRoutes() {
    // Simple optimization: prioritize shorter, more efficient connections
    for (auto& transfer : activeTransfers) {
        if (transfer.fromSatelliteID >= 0 && transfer.toSatelliteID >= 0) {
            float efficiency = getConnectionEfficiency(transfer.fromSatelliteID, transfer.toSatelliteID);

            // Adjust transfer rate based on efficiency
            transfer.maxTransferRate = baseTransferRate * efficiency;
        }
    }
}

void FuelTransferNetwork::drawNetworkConnections(sf::RenderWindow& window) {
    for (const auto& conn : connections) {
        if (!conn.isActive) continue;

        // Find satellite positions
        sf::Vector2f pos1, pos2;
        bool found1 = false, found2 = false;

        for (Satellite* sat : satellites) {
            if (sat && sat->getID() == conn.satellite1ID) {
                pos1 = sat->getPosition();
                found1 = true;
            }
            if (sat && sat->getID() == conn.satellite2ID) {
                pos2 = sat->getPosition();
                found2 = true;
            }
        }

        if (!found1 || !found2) continue;

        // Draw connection line
        sf::VertexArray line(sf::PrimitiveType::LineStrip);

        sf::Vertex start;
        start.position = pos1;
        start.color = GameConstants::SATELLITE_CONNECTION_COLOR;
        start.color.a = static_cast<uint8_t>(conn.transferEfficiency * 255);
        line.append(start);

        sf::Vertex end;
        end.position = pos2;
        end.color = start.color;
        line.append(end);

        window.draw(line);
    }
}

void FuelTransferNetwork::drawTransferFlows(sf::RenderWindow& window) {
    for (const auto& transfer : activeTransfers) {
        if (transfer.fromSatelliteID < 0 || transfer.toSatelliteID < 0) continue;

        // Find satellite positions
        sf::Vector2f fromPos, toPos;
        bool foundFrom = false, foundTo = false;

        for (Satellite* sat : satellites) {
            if (sat && sat->getID() == transfer.fromSatelliteID) {
                fromPos = sat->getPosition();
                foundFrom = true;
            }
            if (sat && sat->getID() == transfer.toSatelliteID) {
                toPos = sat->getPosition();
                foundTo = true;
            }
        }

        if (!foundFrom || !foundTo) continue;

        // Draw thicker line for active fuel flow
        sf::VertexArray flow(sf::PrimitiveType::LineStrip);

        sf::Color flowColor = transfer.isEmergency ?
            GameConstants::SATELLITE_EMERGENCY_COLOR :
            GameConstants::SATELLITE_TRANSFER_FLOW_COLOR;

        sf::Vertex start;
        start.position = fromPos;
        start.color = flowColor;
        flow.append(start);

        sf::Vertex end;
        end.position = toPos;
        end.color = flowColor;
        flow.append(end);

        window.draw(flow);
    }
}

void FuelTransferNetwork::drawEmergencyIndicators(sf::RenderWindow& window) {
    for (int emergencyID : emergencySatellites) {
        // Find satellite position
        sf::Vector2f pos;
        bool found = false;

        for (Satellite* sat : satellites) {
            if (sat && sat->getID() == emergencyID) {
                pos = sat->getPosition();
                found = true;
                break;
            }
        }

        if (!found) continue;

        // Draw pulsing emergency indicator
        sf::CircleShape indicator;
        indicator.setRadius(25.0f);
        indicator.setPosition(pos.x - 25.0f, pos.y - 25.0f);
        indicator.setFillColor(sf::Color::Transparent);
        indicator.setOutlineColor(GameConstants::SATELLITE_EMERGENCY_COLOR);
        indicator.setOutlineThickness(3.0f);

        window.draw(indicator);
    }
}