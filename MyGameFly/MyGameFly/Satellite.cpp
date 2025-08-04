#include "Satellite.h"
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include "OrbitMaintenance.h" // Include to get OrbitalElements definition
#include <cmath>
#include <sstream>
#include <iostream>
#include <algorithm>

Satellite::Satellite(sf::Vector2f pos, sf::Vector2f vel, int id, sf::Color col, float baseM)
    : GameObject(pos, vel, col), satelliteID(id), baseMass(baseM), maxMass(GameConstants::SATELLITE_MAX_MASS),
    currentFuel(GameConstants::SATELLITE_STARTING_FUEL), maxFuel(GameConstants::SATELLITE_MAX_FUEL),
    maintenanceFuelReserve(0.0f), mass(baseM), status(SatelliteStatus::ACTIVE),
    orbitToleranceRadius(GameConstants::SATELLITE_ORBIT_TOLERANCE),
    orbitToleranceEccentricity(GameConstants::SATELLITE_ECCENTRICITY_TOLERANCE),
    lastMaintenanceTime(0.0f), maintenanceInterval(GameConstants::SATELLITE_MAINTENANCE_CHECK_INTERVAL),
    needsOrbitalCorrection(false), plannedCorrectionBurn(0.0f, 0.0f),
    transferRange(GameConstants::SATELLITE_TRANSFER_RANGE), isCollectingFuel(false),
    fuelSourcePlanet(nullptr), stationKeepingEfficiency(GameConstants::SATELLITE_FUEL_EFFICIENCY),
    maxCorrectionBurn(GameConstants::SATELLITE_MAX_SINGLE_BURN),
    fuelConsumptionRate(GameConstants::SATELLITE_MAINTENANCE_FUEL_RATE)
{
    // Initialize orbital elements
    targetOrbit = new OrbitalElements();
    currentOrbit = new OrbitalElements();

    // Generate default name
    std::stringstream ss;
    ss << "SAT-" << std::setfill('0') << std::setw(3) << satelliteID;
    name = ss.str();

    // Initialize mass and fuel reserve
    updateMassFromFuel();

    // Create satellite body (circular shape) - FIXED for SFML 3.0
    body.setRadius(GameConstants::SATELLITE_SIZE);
    body.setFillColor(GameConstants::SATELLITE_BODY_COLOR);
    body.setOrigin(sf::Vector2f(GameConstants::SATELLITE_SIZE, GameConstants::SATELLITE_SIZE));
    body.setPosition(position);

    // Create solar panels (rectangular shapes)
    for (int i = 0; i < 2; i++) {
        solarPanels[i].setPointCount(4);
        float panelWidth = GameConstants::SATELLITE_PANEL_SIZE * 3;
        float panelHeight = GameConstants::SATELLITE_PANEL_SIZE;

        // Left panel
        if (i == 0) {
            solarPanels[i].setPoint(0, sf::Vector2f(-panelWidth - GameConstants::SATELLITE_SIZE, -panelHeight / 2));
            solarPanels[i].setPoint(1, sf::Vector2f(-GameConstants::SATELLITE_SIZE, -panelHeight / 2));
            solarPanels[i].setPoint(2, sf::Vector2f(-GameConstants::SATELLITE_SIZE, panelHeight / 2));
            solarPanels[i].setPoint(3, sf::Vector2f(-panelWidth - GameConstants::SATELLITE_SIZE, panelHeight / 2));
        }
        // Right panel
        else {
            solarPanels[i].setPoint(0, sf::Vector2f(GameConstants::SATELLITE_SIZE, -panelHeight / 2));
            solarPanels[i].setPoint(1, sf::Vector2f(GameConstants::SATELLITE_SIZE + panelWidth, -panelHeight / 2));
            solarPanels[i].setPoint(2, sf::Vector2f(GameConstants::SATELLITE_SIZE + panelWidth, panelHeight / 2));
            solarPanels[i].setPoint(3, sf::Vector2f(GameConstants::SATELLITE_SIZE, panelHeight / 2));
        }

        solarPanels[i].setFillColor(GameConstants::SATELLITE_PANEL_COLOR);
        solarPanels[i].setPosition(position);
    }

    // Set target orbit to current state initially
    setTargetOrbitFromCurrent();

    std::cout << "Created satellite " << name << " at position (" << pos.x << ", " << pos.y << ")" << std::endl;
}

Satellite::~Satellite() {
    delete targetOrbit;
    delete currentOrbit;
}

std::unique_ptr<Satellite> Satellite::createFromRocket(const Rocket* rocket, int id) {
    if (!rocket) return nullptr;

    try {
        // Create satellite at rocket's position and velocity
        auto satellite = std::make_unique<Satellite>(
            rocket->getPosition(),
            rocket->getVelocity(),
            id,
            sf::Color::Cyan,
            GameConstants::SATELLITE_BASE_MASS
        );

        // Transfer fuel with conversion efficiency
        float transferredFuel = rocket->getCurrentFuel() * GameConstants::SATELLITE_CONVERSION_FUEL_RETENTION;
        transferredFuel = std::min(transferredFuel, GameConstants::SATELLITE_MAX_FUEL);
        satellite->setFuel(transferredFuel);

        // Set target orbit based on rocket's current trajectory
        satellite->setTargetOrbitFromCurrent();

        std::cout << "Converted rocket to satellite " << satellite->getName()
            << " with " << transferredFuel << " fuel units" << std::endl;

        return satellite;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during rocket to satellite conversion: " << e.what() << std::endl;
        return nullptr;
    }
}

void Satellite::updateOrbitalElements() {
    // Simplified orbital elements calculation
    // This is a placeholder - full implementation would be in OrbitMaintenance
    currentOrbit->semiMajorAxis = 300.0f; // Placeholder
    currentOrbit->eccentricity = 0.1f;    // Placeholder
    currentOrbit->inclination = 0.0f;     // 2D simulation
}

bool Satellite::checkOrbitAccuracy() {
    updateOrbitalElements();

    // Calculate drift from target orbit
    float radiusDrift = std::abs(currentOrbit->semiMajorAxis - targetOrbit->semiMajorAxis);
    float eccentricityDrift = std::abs(currentOrbit->eccentricity - targetOrbit->eccentricity);

    // Check if within tolerances
    bool radiusOK = radiusDrift <= orbitToleranceRadius;
    bool eccentricityOK = eccentricityDrift <= orbitToleranceEccentricity;

    return radiusOK && eccentricityOK;
}

void Satellite::calculateCorrectionBurn() {
    // Simplified correction calculation
    plannedCorrectionBurn = sf::Vector2f(0.0f, 0.0f);
    needsOrbitalCorrection = false;
}

void Satellite::performStationKeeping(float deltaTime) {
    // Simplified station-keeping
    if (needsOrbitalCorrection && canPerformMaintenance()) {
        // Apply minimal correction
        velocity += plannedCorrectionBurn * deltaTime;
        consumeMaintenanceFuel(0.1f * deltaTime);
        needsOrbitalCorrection = false;
    }
}

void Satellite::updateMassFromFuel() {
    float oldMass = mass;
    mass = baseMass + currentFuel;
    mass = std::min(mass, maxMass);

    // Update maintenance fuel reserve (percentage of total fuel)
    maintenanceFuelReserve = currentFuel * GameConstants::SATELLITE_MAINTENANCE_FUEL_PERCENT;
    maintenanceFuelReserve = std::min(maintenanceFuelReserve,
        GameConstants::SATELLITE_MINIMUM_MAINTENANCE_FUEL);
}

void Satellite::updateStatus() {
    float fuelPercent = getFuelPercentage() / 100.0f;

    if (currentFuel <= 0.0f) {
        status = SatelliteStatus::FUEL_DEPLETED;
    }
    else if (fuelPercent <= GameConstants::SATELLITE_CRITICAL_FUEL_THRESHOLD) {
        status = SatelliteStatus::CRITICAL_FUEL;
    }
    else if (fuelPercent <= GameConstants::SATELLITE_EMERGENCY_FUEL_THRESHOLD) {
        status = SatelliteStatus::LOW_FUEL;
    }
    else if (needsOrbitalCorrection) {
        status = SatelliteStatus::MAINTENANCE_MODE;
    }
    else if (isCollectingFuel) {
        status = SatelliteStatus::TRANSFER_MODE;
    }
    else {
        status = SatelliteStatus::ACTIVE;
    }
}

float Satellite::getAvailableFuelForTransfer() const {
    return std::max(0.0f, currentFuel - maintenanceFuelReserve);
}

bool Satellite::canPerformMaintenance() const {
    return maintenanceFuelReserve > 0.0f && status != SatelliteStatus::FUEL_DEPLETED;
}

void Satellite::consumeMaintenanceFuel(float amount) {
    float fuelToConsume = std::min(amount, maintenanceFuelReserve);
    maintenanceFuelReserve -= fuelToConsume;
    currentFuel -= fuelToConsume;
    currentFuel = std::max(0.0f, currentFuel);
    updateMassFromFuel();
}

void Satellite::setTargetOrbitFromCurrent() {
    updateOrbitalElements();
    *targetOrbit = *currentOrbit;
    std::cout << "Satellite " << name << " target orbit set - Semi-major axis: "
        << targetOrbit->semiMajorAxis << ", Eccentricity: " << targetOrbit->eccentricity << std::endl;
}

float Satellite::getOrbitAccuracy() const {
    if (targetOrbit->semiMajorAxis <= 0) return 100.0f; // No target set

    float radiusError = std::abs(currentOrbit->semiMajorAxis - targetOrbit->semiMajorAxis) / targetOrbit->semiMajorAxis;
    float eccentricityError = std::abs(currentOrbit->eccentricity - targetOrbit->eccentricity);

    float totalError = (radiusError + eccentricityError) / 2.0f;
    return std::max(0.0f, 100.0f - totalError * 100.0f);
}

float Satellite::getMaintenanceFuelPercentage() const {
    return maxFuel > 0 ? (maintenanceFuelReserve / maxFuel) * 100.0f : 0.0f;
}

void Satellite::setFuel(float fuel) {
    currentFuel = std::clamp(fuel, 0.0f, maxFuel);
    updateMassFromFuel();
}

void Satellite::addFuel(float fuel) {
    setFuel(currentFuel + fuel);
}

bool Satellite::transferFuelTo(Satellite* target, float amount) {
    if (!target || amount <= 0.0f) return false;

    float availableFuel = getAvailableFuelForTransfer();
    if (availableFuel < amount) return false;

    float targetCapacity = target->getMaxFuel() - target->getCurrentFuel();
    if (targetCapacity <= 0.0f) return false;

    float transferAmount = std::min(amount, std::min(availableFuel, targetCapacity));

    currentFuel -= transferAmount;
    target->addFuel(transferAmount);

    updateMassFromFuel();

    return true;
}

bool Satellite::transferFuelToRocket(Rocket* rocket, float amount) {
    if (!rocket || amount <= 0.0f) return false;

    float availableFuel = getAvailableFuelForTransfer();
    if (availableFuel < amount) return false;

    float rocketCapacity = rocket->getMaxFuel() - rocket->getCurrentFuel();
    if (rocketCapacity <= 0.0f) return false;

    float transferAmount = std::min(amount, std::min(availableFuel, rocketCapacity));

    currentFuel -= transferAmount;
    rocket->addFuel(transferAmount);

    updateMassFromFuel();

    return true;
}


void Satellite::transferFuelToNearbyRockets(float deltaTime) {
    if (!isOperational() || getAvailableFuel() < 1.0f) return;

    // Use the member variable nearbyRockets set by SatelliteManager
    if (nearbyRockets.empty()) {
        // No rockets nearby - finish any active transfers
        for (auto& entry : rocketTransferTracking) {
            if (entry.second.isActive) {
                std::cout << "Satellite " << name << " finished transfer - gave "
                    << entry.second.totalTransferred << " total fuel to rocket (no rockets nearby)" << std::endl;
            }
        }
        rocketTransferTracking.clear();
        return;
    }

    // Calculate total fuel demand from rockets
    float totalDemand = 0.0f;
    for (Rocket* rocket : nearbyRockets) {
        if (!rocket) continue; // Skip null rockets
        if (rocket->getCurrentFuel() < rocket->getMaxFuel()) {
            float capacity = rocket->getMaxFuel() - rocket->getCurrentFuel();
            totalDemand += capacity;
        }
    }

    if (totalDemand <= 0.0f) return;

    // Calculate transfer rate (units per second)
    float baseTransferRate = GameConstants::SATELLITE_BASE_TRANSFER_RATE;
    float availableFuel = getAvailableFuel();
    float maxTransferThisFrame = baseTransferRate * deltaTime;
    float actualTransfer = std::min(maxTransferThisFrame, availableFuel);

    // Distribute fuel proportionally based on demand
    for (Rocket* rocket : nearbyRockets) {
        if (!rocket || rocket->getCurrentFuel() >= rocket->getMaxFuel()) {
            // Rocket is full - check if we were transferring to it and output total
            auto it = std::find_if(rocketTransferTracking.begin(), rocketTransferTracking.end(),
                [rocket](const std::pair<Rocket*, RocketTransferInfo>& entry) {
                    return entry.first == rocket;
                });

            if (it != rocketTransferTracking.end() && it->second.isActive) {
                std::cout << "Satellite " << name << " finished transfer - gave "
                    << it->second.totalTransferred << " total fuel to rocket" << std::endl;
                rocketTransferTracking.erase(it);
            }
            continue;
        }

        float rocketCapacity = rocket->getMaxFuel() - rocket->getCurrentFuel();
        float proportion = rocketCapacity / totalDemand;
        float fuelForThisRocket = actualTransfer * proportion;

        if (fuelForThisRocket > 0.1f) { // Minimum transfer threshold
            if (transferFuelToRocket(rocket, fuelForThisRocket)) {
                status = SatelliteStatus::TRANSFER_MODE;

                // Find or create tracking entry for this rocket
                auto it = std::find_if(rocketTransferTracking.begin(), rocketTransferTracking.end(),
                    [rocket](const std::pair<Rocket*, RocketTransferInfo>& entry) {
                        return entry.first == rocket;
                    });

                if (it == rocketTransferTracking.end()) {
                    // First time transferring to this rocket - start tracking
                    RocketTransferInfo info;
                    info.totalTransferred = fuelForThisRocket;
                    info.isActive = true;
                    rocketTransferTracking.push_back({ rocket, info });
                    std::cout << "Satellite " << name << " started fuel transfer to rocket" << std::endl;
                }
                else {
                    // Add to existing transfer total
                    it->second.totalTransferred += fuelForThisRocket;
                }
            }
        }
    }

    // Clean up tracking for rockets that are no longer nearby
    auto it = rocketTransferTracking.begin();
    while (it != rocketTransferTracking.end()) {
        Rocket* trackedRocket = it->first;
        bool stillNearby = false;

        for (Rocket* rocket : nearbyRockets) {
            if (rocket == trackedRocket) {
                stillNearby = true;
                break;
            }
        }

        if (!stillNearby && it->second.isActive) {
            // Rocket moved away - output final total
            std::cout << "Satellite " << name << " finished transfer - gave "
                << it->second.totalTransferred << " total fuel to rocket (moved out of range)" << std::endl;
            it = rocketTransferTracking.erase(it);
        }
        else {
            ++it;
        }
    }
}

bool Satellite::isRocketInTransferRange(const Rocket* rocket) const {
    if (!rocket) return false;

    float dist = distance(position, rocket->getPosition());
    return dist <= GameConstants::SATELLITE_ROCKET_DOCKING_RANGE;
}

void Satellite::collectFuelFromPlanets(float deltaTime) {
    isCollectingFuel = false;
    fuelSourcePlanet = nullptr;

    if (currentFuel >= maxFuel) return;

    for (auto* planet : nearbyPlanets) {
        if (!planet) continue; // Add null check
        if (!planet->canCollectFuel()) continue;

        float distanceToSurface = distance(position, planet->getPosition()) - planet->getRadius();

        if (distanceToSurface <= GameConstants::FUEL_COLLECTION_RANGE) {
            float collectionRate = GameConstants::FUEL_COLLECTION_RATE *
                GameConstants::SATELLITE_PLANET_COLLECTION_EFFICIENCY * deltaTime;

            float fuelSpace = maxFuel - currentFuel;
            collectionRate = std::min(collectionRate, fuelSpace);

            float availableMass = planet->getMass() - GameConstants::MIN_PLANET_MASS_FOR_COLLECTION;
            float maxFromPlanet = availableMass / GameConstants::FUEL_COLLECTION_MASS_RATIO;
            collectionRate = std::min(collectionRate, maxFromPlanet);

            if (collectionRate > 0.0f) {
                addFuel(collectionRate);

                float massToRemove = collectionRate * GameConstants::FUEL_COLLECTION_MASS_RATIO;
                planet->setMass(planet->getMass() - massToRemove);

                isCollectingFuel = true;
                fuelSourcePlanet = planet;
                break;
            }
        }
    }
}

bool Satellite::isInFuelCollectionRange(const Planet* planet) const {
    if (!planet) return false;

    float distanceToSurface = distance(position, planet->getPosition()) - planet->getRadius();
    return distanceToSurface <= GameConstants::FUEL_COLLECTION_RANGE;
}

float Satellite::getMaintenanceFuelCost() const {
    return maxCorrectionBurn / stationKeepingEfficiency;
}

void Satellite::update(float deltaTime) {
    lastMaintenanceTime += deltaTime;

    // Apply gravity forces from nearby planets (same as rockets)
    for (auto* planet : nearbyPlanets) {
        if (!planet) continue; // Add null check

        sf::Vector2f direction = planet->getPosition() - position;
        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        // Avoid division by zero and very small distances
        if (distance > planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
            // Use satellite's DYNAMIC MASS for gravity calculation (same as rockets)
            float forceMagnitude = GameConstants::G * planet->getMass() * mass / (distance * distance);
            sf::Vector2f acceleration = normalize(direction) * forceMagnitude / mass;
            sf::Vector2f velocityChange = acceleration * deltaTime;
            velocity += velocityChange;
        }
    }

    // Update position after gravity effects
    position += velocity * deltaTime;

    body.setPosition(position);
    for (int i = 0; i < 2; i++) {
        solarPanels[i].setPosition(position);
    }

    collectFuelFromPlanets(deltaTime);

    if (lastMaintenanceTime >= maintenanceInterval) {
        if (!checkOrbitAccuracy()) {
            calculateCorrectionBurn();
        }
        lastMaintenanceTime = 0.0f;
    }

    performStationKeeping(deltaTime);
    transferFuelToNearbyRockets(deltaTime);
    updateStatus();
}

void Satellite::draw(sf::RenderWindow& window) {
    for (int i = 0; i < 2; i++) {
        window.draw(solarPanels[i]);
    }
    window.draw(body);
}

void Satellite::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel) {
    // FIXED for SFML 3.0
    sf::CircleShape scaledBody = body;
    scaledBody.setRadius(GameConstants::SATELLITE_SIZE * zoomLevel);
    scaledBody.setOrigin(sf::Vector2f(GameConstants::SATELLITE_SIZE * zoomLevel, GameConstants::SATELLITE_SIZE * zoomLevel));

    // Scale solar panels
    sf::ConvexShape scaledPanels[2];
    for (int i = 0; i < 2; i++) {
        scaledPanels[i] = solarPanels[i];
        for (size_t j = 0; j < scaledPanels[i].getPointCount(); j++) {
            sf::Vector2f point = solarPanels[i].getPoint(j);
            scaledPanels[i].setPoint(j, point * zoomLevel);
        }
    }

    // Draw scaled components
    for (int i = 0; i < 2; i++) {
        window.draw(scaledPanels[i]);
    }
    window.draw(scaledBody);
}

void Satellite::drawStatusIndicator(sf::RenderWindow& window, float zoomLevel) {
    sf::CircleShape statusRing;
    statusRing.setRadius((GameConstants::SATELLITE_SIZE + 5.0f) * zoomLevel);
    statusRing.setPosition(sf::Vector2f(position.x - statusRing.getRadius(), position.y - statusRing.getRadius()));
    statusRing.setFillColor(sf::Color::Transparent);
    statusRing.setOutlineThickness(2.0f * zoomLevel);

    // Set color based on status
    switch (status) {
    case SatelliteStatus::ACTIVE:
        statusRing.setOutlineColor(GameConstants::SATELLITE_STATUS_ACTIVE);
        break;
    case SatelliteStatus::LOW_FUEL:
        statusRing.setOutlineColor(GameConstants::SATELLITE_STATUS_LOW_FUEL);
        break;
    case SatelliteStatus::CRITICAL_FUEL:
        statusRing.setOutlineColor(GameConstants::SATELLITE_STATUS_CRITICAL);
        break;
    case SatelliteStatus::FUEL_DEPLETED:
        statusRing.setOutlineColor(GameConstants::SATELLITE_STATUS_DEPLETED);
        break;
    case SatelliteStatus::MAINTENANCE_MODE:
        statusRing.setOutlineColor(GameConstants::SATELLITE_MAINTENANCE_BURN_COLOR);
        break;
    case SatelliteStatus::TRANSFER_MODE:
        statusRing.setOutlineColor(GameConstants::SATELLITE_TRANSFER_FLOW_COLOR);
        break;
    }

    window.draw(statusRing);
}

void Satellite::drawFuelTransferLines(sf::RenderWindow& window) {
    if (!isCollectingFuel || !fuelSourcePlanet) return;

    sf::VertexArray line(sf::PrimitiveType::LineStrip);

    sf::Vertex start;
    start.position = position;
    start.color = GameConstants::SATELLITE_TRANSFER_FLOW_COLOR;
    line.append(start);

    sf::Vertex end;
    end.position = fuelSourcePlanet->getPosition();
    end.color = GameConstants::SATELLITE_TRANSFER_FLOW_COLOR;
    line.append(end);

    window.draw(line);
}

void Satellite::drawOrbitPath(sf::RenderWindow& window, const std::vector<Planet*>& planets) {
    // Placeholder implementation
}

void Satellite::drawTargetOrbitPath(sf::RenderWindow& window, const std::vector<Planet*>& planets) {
    // Placeholder implementation
}

void Satellite::drawMaintenanceBurn(sf::RenderWindow& window) {
    // Placeholder implementation
}

void Satellite::sendStatusUpdate() {
    // Placeholder for network synchronization
}

void Satellite::receiveStatusUpdate() {
    // Placeholder for network synchronization
}