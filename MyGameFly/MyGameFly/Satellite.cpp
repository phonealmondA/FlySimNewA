#include "Satellite.h"
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <cmath>
#include <sstream>
#include <iostream>
#include <algorithm>

// OrbitalElements methods implementation
float OrbitalElements::getOrbitalPeriod(float centralMass) const {
    const float G = GameConstants::G;
    return 2.0f * GameConstants::PI * std::sqrt(std::pow(semiMajorAxis, 3) / (G * centralMass));
}

float OrbitalElements::getOrbitalVelocity(float centralMass, float radius) const {
    const float G = GameConstants::G;
    return std::sqrt(G * centralMass / radius);
}

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
    // Generate default name
    std::stringstream ss;
    ss << "SAT-" << std::setfill('0') << std::setw(3) << satelliteID;
    name = ss.str();

    // Initialize mass and fuel reserve
    updateMassFromFuel();

    // Create satellite body (circular shape)
    body.setRadius(GameConstants::SATELLITE_SIZE);
    body.setFillColor(GameConstants::SATELLITE_BODY_COLOR);
    body.setOrigin(GameConstants::SATELLITE_SIZE, GameConstants::SATELLITE_SIZE);
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

std::unique_ptr<Satellite> Satellite::createFromRocket(const Rocket* rocket, int id) {
    if (!rocket) return nullptr;

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

void Satellite::updateOrbitalElements() {
    if (nearbyPlanets.empty()) return;

    // Find the primary gravitational body (usually the largest/closest planet)
    Planet* primaryPlanet = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (auto* planet : nearbyPlanets) {
        float dist = distance(position, planet->getPosition());
        if (dist < minDistance) {
            minDistance = dist;
            primaryPlanet = planet;
        }
    }

    if (!primaryPlanet) return;

    // Calculate orbital elements relative to primary planet
    sf::Vector2f relativePos = position - primaryPlanet->getPosition();
    sf::Vector2f relativeVel = velocity - primaryPlanet->getVelocity();

    float r = std::sqrt(relativePos.x * relativePos.x + relativePos.y * relativePos.y);
    float v = std::sqrt(relativeVel.x * relativeVel.x + relativeVel.y * relativeVel.y);

    // Calculate orbital energy
    float mu = GameConstants::G * primaryPlanet->getMass();
    float energy = 0.5f * v * v - mu / r;

    // Calculate semi-major axis
    if (energy < 0) {
        currentOrbit.semiMajorAxis = -mu / (2.0f * energy);
    }
    else {
        currentOrbit.semiMajorAxis = std::numeric_limits<float>::infinity(); // Hyperbolic orbit
    }

    // Calculate eccentricity vector
    sf::Vector2f eVector;
    float vSquared = v * v;
    float rdotv = relativePos.x * relativeVel.x + relativePos.y * relativeVel.y;

    eVector.x = (vSquared * relativePos.x - rdotv * relativeVel.x) / mu - relativePos.x / r;
    eVector.y = (vSquared * relativePos.y - rdotv * relativeVel.y) / mu - relativePos.y / r;

    currentOrbit.eccentricity = std::sqrt(eVector.x * eVector.x + eVector.y * eVector.y);

    // For 2D simulation, inclination is always 0
    currentOrbit.inclination = 0.0f;

    // Calculate true anomaly (angle from periapsis)
    if (currentOrbit.eccentricity > 0.001f) {
        float cosTA = (eVector.x * relativePos.x + eVector.y * relativePos.y) /
            (currentOrbit.eccentricity * r);
        currentOrbit.trueAnomaly = std::acos(std::clamp(cosTA, -1.0f, 1.0f));

        // Determine if we're moving toward or away from periapsis
        if (rdotv < 0) {
            currentOrbit.trueAnomaly = 2.0f * GameConstants::PI - currentOrbit.trueAnomaly;
        }
    }
    else {
        currentOrbit.trueAnomaly = 0.0f; // Circular orbit
    }
}

bool Satellite::checkOrbitAccuracy() {
    updateOrbitalElements();

    // Calculate drift from target orbit
    float radiusDrift = std::abs(currentOrbit.semiMajorAxis - targetOrbit.semiMajorAxis);
    float eccentricityDrift = std::abs(currentOrbit.eccentricity - targetOrbit.eccentricity);

    // Check if within tolerances
    bool radiusOK = radiusDrift <= orbitToleranceRadius;
    bool eccentricityOK = eccentricityDrift <= orbitToleranceEccentricity;

    return radiusOK && eccentricityOK;
}

void Satellite::calculateCorrectionBurn() {
    if (nearbyPlanets.empty()) {
        plannedCorrectionBurn = sf::Vector2f(0.0f, 0.0f);
        return;
    }

    // Simple correction algorithm - more sophisticated algorithms could be implemented
    float radiusDrift = currentOrbit.semiMajorAxis - targetOrbit.semiMajorAxis;
    float eccentricityDrift = currentOrbit.eccentricity - targetOrbit.eccentricity;

    sf::Vector2f correction(0.0f, 0.0f);

    // Radial correction (adjust orbital energy)
    if (std::abs(radiusDrift) > orbitToleranceRadius * 0.1f) {
        // Calculate velocity direction
        sf::Vector2f velDir = normalize(velocity);

        // Prograde burn to increase orbit, retrograde to decrease
        float burnMagnitude = radiusDrift * 0.01f; // Simple proportional control
        burnMagnitude = std::clamp(burnMagnitude, -maxCorrectionBurn, maxCorrectionBurn);

        correction += velDir * burnMagnitude;
    }

    // Eccentricity correction (circularization)
    if (std::abs(eccentricityDrift) > orbitToleranceEccentricity * 0.1f) {
        // Find the position vector relative to primary planet
        Planet* primaryPlanet = nearbyPlanets[0]; // Simplified - use first planet
        sf::Vector2f radialDir = normalize(position - primaryPlanet->getPosition());
        sf::Vector2f tangentialDir(-radialDir.y, radialDir.x);

        // Burn perpendicular to velocity to change eccentricity
        float burnMagnitude = eccentricityDrift * 10.0f; // Scaling factor
        burnMagnitude = std::clamp(burnMagnitude, -maxCorrectionBurn * 0.5f, maxCorrectionBurn * 0.5f);

        correction += tangentialDir * burnMagnitude;
    }

    plannedCorrectionBurn = correction;
    needsOrbitalCorrection = (correction.x != 0.0f || correction.y != 0.0f);
}

void Satellite::performStationKeeping(float deltaTime) {
    if (!needsOrbitalCorrection || !canPerformMaintenance()) {
        return;
    }

    // Calculate fuel cost for the planned burn
    float burnMagnitude = std::sqrt(plannedCorrectionBurn.x * plannedCorrectionBurn.x +
        plannedCorrectionBurn.y * plannedCorrectionBurn.y);
    float fuelCost = burnMagnitude / stationKeepingEfficiency;

    // Check if we have enough maintenance fuel
    if (fuelCost > maintenanceFuelReserve) {
        // Reduce burn magnitude to match available fuel
        float scaleFactor = maintenanceFuelReserve / fuelCost;
        plannedCorrectionBurn *= scaleFactor;
        fuelCost = maintenanceFuelReserve;
    }

    // Apply the correction burn
    velocity += plannedCorrectionBurn * deltaTime;

    // Consume maintenance fuel
    consumeMaintenanceFuel(fuelCost * deltaTime);

    // Reset correction
    plannedCorrectionBurn = sf::Vector2f(0.0f, 0.0f);
    needsOrbitalCorrection = false;
    lastMaintenanceTime = 0.0f;

    std::cout << "Satellite " << name << " performed orbital correction, fuel cost: "
        << fuelCost * deltaTime << std::endl;
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
    float maintenancePercent = getMaintenanceFuelPercentage() / 100.0f;

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
    targetOrbit = currentOrbit;
    std::cout << "Satellite " << name << " target orbit set - Semi-major axis: "
        << targetOrbit.semiMajorAxis << ", Eccentricity: " << targetOrbit.eccentricity << std::endl;
}

float Satellite::getOrbitAccuracy() const {
    if (targetOrbit.semiMajorAxis <= 0) return 100.0f; // No target set

    float radiusError = std::abs(currentOrbit.semiMajorAxis - targetOrbit.semiMajorAxis) / targetOrbit.semiMajorAxis;
    float eccentricityError = std::abs(currentOrbit.eccentricity - targetOrbit.eccentricity);

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

    // Check if we have available fuel (excluding maintenance reserve)
    float availableFuel = getAvailableFuelForTransfer();
    if (availableFuel < amount) return false;

    // Check if target can receive fuel
    float targetCapacity = target->getMaxFuel() - target->getCurrentFuel();
    if (targetCapacity <= 0.0f) return false;

    // Calculate actual transfer amount
    float transferAmount = std::min(amount, std::min(availableFuel, targetCapacity));

    // Perform transfer
    currentFuel -= transferAmount;
    target->addFuel(transferAmount);

    updateMassFromFuel();

    std::cout << "Transferred " << transferAmount << " fuel from " << name
        << " to " << target->getName() << std::endl;

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

    std::cout << "Transferred " << transferAmount << " fuel from satellite " << name
        << " to rocket" << std::endl;

    return true;
}

void Satellite::collectFuelFromPlanets(float deltaTime) {
    isCollectingFuel = false;
    fuelSourcePlanet = nullptr;

    if (currentFuel >= maxFuel) return; // Already full

    for (auto* planet : nearbyPlanets) {
        if (!planet->canCollectFuel()) continue;

        float distanceToSurface = distance(position, planet->getPosition()) - planet->getRadius();

        if (distanceToSurface <= GameConstants::FUEL_COLLECTION_RANGE) {
            // Calculate collection rate with satellite efficiency bonus
            float collectionRate = GameConstants::FUEL_COLLECTION_RATE *
                GameConstants::SATELLITE_PLANET_COLLECTION_EFFICIENCY * deltaTime;

            // Limit by available fuel capacity
            float fuelSpace = maxFuel - currentFuel;
            collectionRate = std::min(collectionRate, fuelSpace);

            // Limit by planet mass availability
            float availableMass = planet->getMass() - GameConstants::MIN_PLANET_MASS_FOR_COLLECTION;
            float maxFromPlanet = availableMass / GameConstants::FUEL_COLLECTION_MASS_RATIO;
            collectionRate = std::min(collectionRate, maxFromPlanet);

            if (collectionRate > 0.0f) {
                // Collect fuel
                addFuel(collectionRate);

                // Remove mass from planet
                float massToRemove = collectionRate * GameConstants::FUEL_COLLECTION_MASS_RATIO;
                planet->setMass(planet->getMass() - massToRemove);

                // Set collection status
                isCollectingFuel = true;
                fuelSourcePlanet = planet;

                break; // Only collect from one planet at a time
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
    // Estimate fuel cost for typical orbital correction
    return maxCorrectionBurn / stationKeepingEfficiency;
}

void Satellite::update(float deltaTime) {
    // Update timing
    lastMaintenanceTime += deltaTime;

    // Update position and velocity
    position += velocity * deltaTime;

    // Update visual components
    body.setPosition(position);
    for (int i = 0; i < 2; i++) {
        solarPanels[i].setPosition(position);
    }

    // Collect fuel from nearby planets
    collectFuelFromPlanets(deltaTime);

    // Perform orbital maintenance check
    if (lastMaintenanceTime >= maintenanceInterval) {
        if (!checkOrbitAccuracy()) {
            calculateCorrectionBurn();
        }
    }

    // Perform station-keeping
    performStationKeeping(deltaTime);

    // Update status
    updateStatus();
}

void Satellite::draw(sf::RenderWindow& window) {
    // Draw solar panels first (behind main body)
    for (int i = 0; i < 2; i++) {
        window.draw(solarPanels[i]);
    }

    // Draw main satellite body
    window.draw(body);
}

void Satellite::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel) {
    // Scale satellite components based on zoom level
    sf::CircleShape scaledBody = body;
    scaledBody.setRadius(GameConstants::SATELLITE_SIZE * zoomLevel);
    scaledBody.setOrigin(GameConstants::SATELLITE_SIZE * zoomLevel, GameConstants::SATELLITE_SIZE * zoomLevel);

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
    statusRing.setPosition(position.x - statusRing.getRadius(), position.y - statusRing.getRadius());
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