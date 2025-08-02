#include "OrbitMaintenance.h"
#include "Satellite.h"
#include "Planet.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>

OrbitMaintenance::OrbitMaintenance(Satellite* sat)
    : satellite(sat), maneuverExecutionTimer(0.0f), timeSinceLastCheck(0.0f),
    timeSinceLastCorrection(0.0f), nextScheduledCheck(0.0f),
    totalFuelConsumed(0.0f), correctionsPerformed(0),
    averageCorrectionSize(0.0f), totalDeltaVExpended(0.0f),
    predictionTimeHorizon(3600.0f), predictionSteps(GameConstants::SATELLITE_PREDICTION_STEPS)
{
    if (!satellite) {
        std::cerr << "OrbitMaintenance: Cannot initialize with null satellite" << std::endl;
        return;
    }

    // Initialize with current orbital state
    updateCurrentOrbit();
    targetOrbit = lastKnownOrbit;

    // Set default configuration
    config = StationKeepingConfig(); // Use default values

    std::cout << "OrbitMaintenance initialized for satellite " << satellite->getName() << std::endl;
}


float OrbitalElements::getOrbitalPeriod(float centralMass) const {
    const float G = GameConstants::G;
    return 2.0f * GameConstants::PI * std::sqrt(std::pow(semiMajorAxis, 3) / (G * centralMass));
}
void OrbitMaintenance::update(float deltaTime) {
    if (!satellite) return;

    timeSinceLastCheck += deltaTime;
    timeSinceLastCorrection += deltaTime;
    maneuverExecutionTimer += deltaTime;

    // Perform scheduled maintenance checks
    if (timeSinceLastCheck >= config.checkInterval) {
        performMaintenanceCheck();
        timeSinceLastCheck = 0.0f;
    }

    // Execute scheduled maneuvers
    executeScheduledManeuvers(deltaTime);

    // Update orbit prediction periodically
    if (static_cast<int>(timeSinceLastCheck) % 30 == 0) { // Every 30 seconds
        updateOrbitPrediction();
    }
}

void OrbitMaintenance::updateCurrentOrbit() {
    if (!satellite || celestialBodies.empty()) return;

    // Find primary gravitational body (largest mass within reasonable distance)
    Planet* primaryBody = nullptr;
    float minInfluenceRatio = std::numeric_limits<float>::max();

    for (auto* planet : celestialBodies) {
        float dist = distance(satellite->getPosition(), planet->getPosition());
        float influenceRatio = dist / planet->getMass(); // Lower = more influential

        if (influenceRatio < minInfluenceRatio) {
            minInfluenceRatio = influenceRatio;
            primaryBody = planet;
        }
    }

    if (!primaryBody) return;

    // Calculate orbital elements relative to primary body
    sf::Vector2f relativePos = satellite->getPosition() - primaryBody->getPosition();
    sf::Vector2f relativeVel = satellite->getVelocity() - primaryBody->getVelocity();

    float mu = GameConstants::G * primaryBody->getMass();
    lastKnownOrbit = calculateOrbitalElements(relativePos, relativeVel, mu);
}

void OrbitMaintenance::analyzeDrift() {
    updateCurrentOrbit();

    // Calculate drift from target orbit
    currentDrift.radiusDrift = lastKnownOrbit.semiMajorAxis - targetOrbit.semiMajorAxis;
    currentDrift.eccentricityDrift = lastKnownOrbit.eccentricity - targetOrbit.eccentricity;
    currentDrift.inclinationDrift = lastKnownOrbit.inclination - targetOrbit.inclination;

    // Calculate drift rates (simplified - would need historical data for accuracy)
    float timeDelta = std::max(1.0f, timeSinceLastCorrection);
    currentDrift.radiusDriftRate = currentDrift.radiusDrift / timeDelta;
    currentDrift.eccentricityDriftRate = currentDrift.eccentricityDrift / timeDelta;
    currentDrift.inclinationDriftRate = currentDrift.inclinationDrift / timeDelta;

    // Calculate apoapsis and periapsis drift
    currentDrift.apoapsisDrift = lastKnownOrbit.getApoapsis() - targetOrbit.getApoapsis();
    currentDrift.periapsisDrift = lastKnownOrbit.getPeriapsis() - targetOrbit.getPeriapsis();

    // Calculate orbital period drift
    if (!celestialBodies.empty()) {
        float primaryMass = celestialBodies[0]->getMass(); // Simplified
        currentDrift.periodDrift = lastKnownOrbit.getOrbitalPeriod(primaryMass) -
            targetOrbit.getOrbitalPeriod(primaryMass);
    }

    // Assess drift severity
    assessDriftSeverity();

    // Detect drift causes
    detectGravitationalPerturbations();
    detectResonanceEffects();
}

void OrbitMaintenance::assessDriftSeverity() {
    // Calculate normalized drift values
    float radiusError = std::abs(currentDrift.radiusDrift) / config.orbitToleranceRadius;
    float eccentricityError = std::abs(currentDrift.eccentricityDrift) / config.eccentricityTolerance;
    float inclinationError = std::abs(currentDrift.inclinationDrift) / config.inclinationTolerance;

    // Combined severity score (0-1, where 1 is maximum tolerance)
    currentDrift.overallSeverity = std::max({ radiusError, eccentricityError, inclinationError });

    // Determine action requirements
    currentDrift.requiresAction = currentDrift.overallSeverity > 1.0f;
    currentDrift.requiresImmediateAction = currentDrift.overallSeverity > config.emergencyThreshold;

    // Predict time to orbital decay (simplified)
    currentDrift.timeToDecay = predictTimeToDecay();
}

float OrbitMaintenance::predictTimeToDecay() const {
    if (currentDrift.radiusDriftRate >= 0) return -1.0f; // Orbit is stable or expanding

    // Calculate time until orbit drops below minimum safe altitude
    float currentAltitude = lastKnownOrbit.semiMajorAxis;
    float safeAltitude = 100.0f; // Minimum safe altitude above planet surface

    if (currentAltitude <= safeAltitude) return 0.0f; // Already critical

    float altitudeToLose = currentAltitude - safeAltitude;
    float decayRate = std::abs(currentDrift.radiusDriftRate);

    if (decayRate <= 0.0f) return -1.0f; // No decay detected

    return altitudeToLose / decayRate;
}

void OrbitMaintenance::detectGravitationalPerturbations() {
    currentDrift.gravitationalPerturbation = false;

    if (celestialBodies.size() > 1) {
        // Check if other bodies are significantly affecting the orbit
        sf::Vector2f totalPerturbation(0.0f, 0.0f);

        for (auto* planet : celestialBodies) {
            sf::Vector2f direction = planet->getPosition() - satellite->getPosition();
            float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            if (dist > 0.0f) {
                float perturbationMagnitude = GameConstants::G * planet->getMass() / (dist * dist);
                totalPerturbation += normalize(direction) * perturbationMagnitude;
            }
        }

        float perturbationStrength = std::sqrt(totalPerturbation.x * totalPerturbation.x +
            totalPerturbation.y * totalPerturbation.y);

        // If perturbation is significant relative to primary body's gravity
        if (perturbationStrength > 0.1f) { // Threshold for significant perturbation
            currentDrift.gravitationalPerturbation = true;
        }
    }
}

void OrbitMaintenance::detectResonanceEffects() {
    currentDrift.resonanceEffects = false;

    if (celestialBodies.size() > 1 && !celestialBodies.empty()) {
        float primaryMass = celestialBodies[0]->getMass();
        float satellitePeriod = lastKnownOrbit.getOrbitalPeriod(primaryMass);

        // Check for orbital resonances with other bodies
        for (size_t i = 1; i < celestialBodies.size(); i++) {
            auto* otherBody = celestialBodies[i];
            float otherDistance = distance(otherBody->getPosition(), celestialBodies[0]->getPosition());
            float otherPeriod = 2.0f * GameConstants::PI * std::sqrt(std::pow(otherDistance, 3) /
                (GameConstants::G * primaryMass));

            // Check for simple resonances (1:2, 2:3, 3:4, etc.)
            float periodRatio = satellitePeriod / otherPeriod;

            // Check if close to common resonance ratios
            std::vector<float> resonanceRatios = { 0.5f, 2.0f / 3.0f, 0.75f, 1.0f, 4.0f / 3.0f, 1.5f, 2.0f };

            for (float ratio : resonanceRatios) {
                if (std::abs(periodRatio - ratio) < 0.05f) { // 5% tolerance
                    currentDrift.resonanceEffects = true;
                    break;
                }
            }

            if (currentDrift.resonanceEffects) break;
        }
    }
}

void OrbitMaintenance::performMaintenanceCheck() {
    if (!satellite) return;

    // Analyze current drift
    analyzeDrift();

    // Clear old planned maneuvers if orbit has changed significantly
    if (currentDrift.overallSeverity > 0.5f) {
        clearPlannedManeuvers();
    }

    // Calculate required maneuvers
    if (currentDrift.requiresAction) {
        calculateRequiredManeuvers();

        if (currentDrift.requiresImmediateAction) {
            std::cout << "EMERGENCY: Satellite " << satellite->getName()
                << " requires immediate orbital correction!" << std::endl;
        }
    }

    std::cout << "Maintenance check for " << satellite->getName()
        << " - Drift severity: " << (currentDrift.overallSeverity * 100.0f) << "%" << std::endl;
}

void OrbitMaintenance::calculateRequiredManeuvers() {
    plannedManeuvers.clear();

    // Prioritize corrections based on severity and satellite configuration
    if (config.prioritizeCircularOrbit && std::abs(currentDrift.eccentricityDrift) > config.eccentricityTolerance) {
        auto circularizeManeuver = planCircularizationBurn();
        if (circularizeManeuver.deltaV > 0.1f) {
            plannedManeuvers.push_back(circularizeManeuver);
        }
    }

    if (std::abs(currentDrift.radiusDrift) > config.orbitToleranceRadius) {
        auto radiusManeuver = planProgradeCorrection(-currentDrift.radiusDrift);
        if (radiusManeuver.deltaV > 0.1f) {
            plannedManeuvers.push_back(radiusManeuver);
        }
    }

    if (config.prioritizeInclination && std::abs(currentDrift.inclinationDrift) > config.inclinationTolerance) {
        auto inclinationManeuver = planInclinationCorrection(targetOrbit.inclination);
        if (inclinationManeuver.deltaV > 0.1f) {
            plannedManeuvers.push_back(inclinationManeuver);
        }
    }

    // Optimize maneuver sequence
    if (plannedManeuvers.size() > 1) {
        optimizeManeuverSequence();
    }

    // Set urgency levels
    for (auto& maneuver : plannedManeuvers) {
        maneuver.urgency = currentDrift.overallSeverity;
        maneuver.isEmergency = currentDrift.requiresImmediateAction;

        // Calculate fuel cost
        maneuver.fuelCost = calculateFuelCost(maneuver.deltaV);
    }
}

OrbitalManeuver OrbitMaintenance::planProgradeCorrection(float targetRadiusChange) {
    OrbitalManeuver maneuver;
    maneuver.type = (targetRadiusChange > 0) ? ManeuverType::PROGRADE : ManeuverType::RETROGRADE;

    // Calculate required delta-V for radius change (simplified circular orbit assumption)
    float currentRadius = lastKnownOrbit.semiMajorAxis;
    float targetRadius = currentRadius + targetRadiusChange;

    if (!celestialBodies.empty()) {
        float mu = GameConstants::G * celestialBodies[0]->getMass();

        // Vis-viva equation: v = sqrt(mu * (2/r - 1/a))
        float currentVelocity = std::sqrt(mu / currentRadius);
        float targetVelocity = std::sqrt(mu / targetRadius);

        maneuver.deltaV = std::abs(targetVelocity - currentVelocity);
        maneuver.deltaV = std::min(maneuver.deltaV, config.maxSingleBurn);

        // Calculate burn direction (prograde/retrograde)
        sf::Vector2f velocityDirection = normalize(satellite->getVelocity());
        maneuver.burnVector = velocityDirection * maneuver.deltaV;

        if (targetRadiusChange < 0) {
            maneuver.burnVector = -maneuver.burnVector; // Retrograde burn
        }

        maneuver.duration = maneuver.deltaV / (config.thrustToWeightRatio * 9.81f); // Simplified
    }

    return maneuver;
}

OrbitalManeuver OrbitMaintenance::planCircularizationBurn() {
    OrbitalManeuver maneuver;
    maneuver.type = ManeuverType::CIRCULARIZE;

    // Calculate delta-V needed to circularize orbit
    float eccentricity = lastKnownOrbit.eccentricity;
    float semiMajorAxis = lastKnownOrbit.semiMajorAxis;

    if (!celestialBodies.empty() && eccentricity > 0.001f) {
        float mu = GameConstants::G * celestialBodies[0]->getMass();

        // Perform circularization burn at apoapsis (most efficient)
        float apoapsis = semiMajorAxis * (1 + eccentricity);
        float vApoapsis = std::sqrt(mu * (2.0f / apoapsis - 1.0f / semiMajorAxis));
        float vCircular = std::sqrt(mu / apoapsis);

        maneuver.deltaV = std::abs(vCircular - vApoapsis);
        maneuver.deltaV = std::min(maneuver.deltaV, config.maxSingleBurn);

        // Burn direction is prograde at apoapsis
        sf::Vector2f velocityDirection = normalize(satellite->getVelocity());
        maneuver.burnVector = velocityDirection * maneuver.deltaV;

        maneuver.duration = maneuver.deltaV / (config.thrustToWeightRatio * 9.81f);
    }

    return maneuver;
}

OrbitalManeuver OrbitMaintenance::planInclinationCorrection(float targetInclination) {
    OrbitalManeuver maneuver;
    maneuver.type = (targetInclination > lastKnownOrbit.inclination) ?
        ManeuverType::NORMAL : ManeuverType::ANTI_NORMAL;

    float inclinationChange = std::abs(targetInclination - lastKnownOrbit.inclination);

    if (!celestialBodies.empty() && inclinationChange > 0.001f) {
        float mu = GameConstants::G * celestialBodies[0]->getMass();
        float currentRadius = lastKnownOrbit.semiMajorAxis;
        float velocity = std::sqrt(mu / currentRadius);

        // Delta-V for inclination change: deltaV = 2 * v * sin(deltaI / 2)
        maneuver.deltaV = 2.0f * velocity * std::sin(inclinationChange / 2.0f);
        maneuver.deltaV = std::min(maneuver.deltaV, config.maxSingleBurn);

        // Calculate normal direction (perpendicular to orbital plane)
        sf::Vector2f velocityDir = normalize(satellite->getVelocity());
        sf::Vector2f normalDir(-velocityDir.y, velocityDir.x); // 90 degree rotation

        if (targetInclination < lastKnownOrbit.inclination) {
            normalDir = -normalDir; // Anti-normal
        }

        maneuver.burnVector = normalDir * maneuver.deltaV;
        maneuver.duration = maneuver.deltaV / (config.thrustToWeightRatio * 9.81f);
    }

    return maneuver;
}

std::vector<OrbitalManeuver> OrbitMaintenance::planMultiStepCorrection() {
    std::vector<OrbitalManeuver> multiStepPlan;

    // Break large corrections into smaller, fuel-efficient steps
    if (currentDrift.overallSeverity > 2.0f) {
        // Very large correction needed - split into multiple burns

        // Step 1: Partial radius correction
        if (std::abs(currentDrift.radiusDrift) > config.orbitToleranceRadius * 2.0f) {
            float partialCorrection = currentDrift.radiusDrift * 0.5f;
            auto step1 = planProgradeCorrection(-partialCorrection);
            multiStepPlan.push_back(step1);
        }

        // Step 2: Circularization
        if (currentDrift.eccentricityDrift > config.eccentricityTolerance * 2.0f) {
            auto step2 = planCircularizationBurn();
            multiStepPlan.push_back(step2);
        }

        // Step 3: Final radius adjustment
        if (std::abs(currentDrift.radiusDrift) > config.orbitToleranceRadius) {
            float finalCorrection = currentDrift.radiusDrift * 0.5f;
            auto step3 = planProgradeCorrection(-finalCorrection);
            multiStepPlan.push_back(step3);
        }
    }

    return multiStepPlan;
}

void OrbitMaintenance::optimizeManeuverSequence() {
    if (plannedManeuvers.size() < 2) return;

    // Sort maneuvers by efficiency and urgency
    std::sort(plannedManeuvers.begin(), plannedManeuvers.end(),
        [](const OrbitalManeuver& a, const OrbitalManeuver& b) {
            // Prioritize by urgency, then by fuel efficiency
            if (a.isEmergency != b.isEmergency) return a.isEmergency;
            return (a.fuelCost / a.deltaV) < (b.fuelCost / b.deltaV);
        });

    // Combine compatible maneuvers if possible
    for (size_t i = 0; i < plannedManeuvers.size() - 1; i++) {
        if (plannedManeuvers[i].type == plannedManeuvers[i + 1].type) {
            // Same type of maneuver - can potentially combine
            float combinedDeltaV = plannedManeuvers[i].deltaV + plannedManeuvers[i + 1].deltaV;

            if (combinedDeltaV <= config.maxSingleBurn) {
                // Combine maneuvers
                plannedManeuvers[i].deltaV = combinedDeltaV;
                plannedManeuvers[i].burnVector =
                    plannedManeuvers[i].burnVector + plannedManeuvers[i + 1].burnVector;
                plannedManeuvers[i].fuelCost = calculateFuelCost(combinedDeltaV);

                // Remove the second maneuver
                plannedManeuvers.erase(plannedManeuvers.begin() + i + 1);
            }
        }
    }
}

void OrbitMaintenance::executeScheduledManeuvers(float deltaTime) {
    if (plannedManeuvers.empty() || !satellite) return;

    // Check if we should start a new maneuver
    if (!activeManeuver && timeSinceLastCorrection >= config.correctionDelay) {
        // Start the next planned maneuver
        activeManeuver = std::make_unique<OrbitalManeuver>(plannedManeuvers.front());
        plannedManeuvers.erase(plannedManeuvers.begin());

        activeManeuver->remainingDeltaV = activeManeuver->deltaV;
        maneuverExecutionTimer = 0.0f;

        std::cout << "Starting orbital maneuver for " << satellite->getName()
            << " - Delta-V: " << activeManeuver->deltaV << std::endl;
    }

    // Execute active maneuver
    if (activeManeuver) {
        executeManeuver(*activeManeuver, deltaTime);
    }
}

void OrbitMaintenance::executeManeuver(OrbitalManeuver& maneuver, float deltaTime) {
    if (!satellite || maneuver.isComplete || maneuver.isAborted) return;

    // Check if satellite has enough fuel
    if (!requestMaintenanceFuel(maneuver.fuelCost * deltaTime / maneuver.duration)) {
        maneuver.isAborted = true;
        std::cout << "Maneuver aborted - insufficient fuel for " << satellite->getName() << std::endl;
        return;
    }

    // Calculate burn for this time step
    float thrustPower = config.thrustToWeightRatio * satellite->getMass() * 9.81f;
    float maxDeltaVThisStep = thrustPower * deltaTime / satellite->getMass();

    float deltaVThisStep = std::min(maxDeltaVThisStep, maneuver.remainingDeltaV);

    if (deltaVThisStep > 0.0f) {
        // Apply velocity change
        sf::Vector2f velocityChange = normalize(maneuver.burnVector) * deltaVThisStep;
        satellite->setVelocity(satellite->getVelocity() + velocityChange);

        // Update maneuver progress
        maneuver.executedBurn += velocityChange;
        maneuver.remainingDeltaV -= deltaVThisStep;

        // Consume fuel
        float fuelConsumed = deltaVThisStep / config.fuelEfficiency;
        totalFuelConsumed += fuelConsumed;
        totalDeltaVExpended += deltaVThisStep;

        // Check if maneuver is complete
        if (maneuver.remainingDeltaV <= 0.01f) {
            maneuver.isComplete = true;
            correctionsPerformed++;
            averageCorrectionSize = totalDeltaVExpended / correctionsPerformed;
            timeSinceLastCorrection = 0.0f;

            std::cout << "Completed orbital maneuver for " << satellite->getName()
                << " - Total Delta-V: " << maneuver.deltaV << std::endl;
        }
    }

    // Check for timeout
    if (maneuverExecutionTimer > maneuver.duration * 2.0f) {
        maneuver.isAborted = true;
        std::cout << "Maneuver timed out for " << satellite->getName() << std::endl;
    }

    // Clear active maneuver if finished
    if (maneuver.isComplete || maneuver.isAborted) {
        activeManeuver.reset();
    }
}

bool OrbitMaintenance::validateManeuverExecution(const OrbitalManeuver& maneuver) const {
    if (!satellite) return false;

    // Check fuel availability
    float requiredFuel = maneuver.fuelCost;
    if (satellite->getAvailableFuel() < requiredFuel) return false;

    // Check satellite operational status
    if (!satellite->isOperational()) return false;

    // Check if maneuver makes sense given current orbit
    if (maneuver.deltaV > config.maxSingleBurn * 2.0f) return false;

    return true;
}

sf::Vector2f OrbitMaintenance::calculateGravitationalForces(sf::Vector2f position) const {
    sf::Vector2f totalForce(0.0f, 0.0f);

    for (auto* planet : celestialBodies) {
        sf::Vector2f direction = planet->getPosition() - position;
        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        if (distance > planet->getRadius() + 10.0f) { // Avoid singularities
            float forceMagnitude = GameConstants::G * planet->getMass() * satellite->getMass() /
                (distance * distance);
            totalForce += normalize(direction) * forceMagnitude;
        }
    }

    return totalForce;
}

OrbitalElements OrbitMaintenance::calculateOrbitalElements(sf::Vector2f position, sf::Vector2f velocity, float centralMass) const {
    OrbitalElements elements;

    float r = std::sqrt(position.x * position.x + position.y * position.y);
    float v = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

    // Specific orbital energy
    float energy = 0.5f * v * v - centralMass / r;

    // Semi-major axis
    if (energy < 0) {
        elements.semiMajorAxis = -centralMass / (2.0f * energy);
    }
    else {
        elements.semiMajorAxis = std::numeric_limits<float>::infinity();
    }

    // Eccentricity vector calculation
    sf::Vector2f eVector;
    float vSquared = v * v;
    float rdotv = position.x * velocity.x + position.y * velocity.y;

    eVector.x = (vSquared * position.x - rdotv * velocity.x) / centralMass - position.x / r;
    eVector.y = (vSquared * position.y - rdotv * velocity.y) / centralMass - position.y / r;

    elements.eccentricity = std::sqrt(eVector.x * eVector.x + eVector.y * eVector.y);

    // For 2D simulation, inclination is always 0
    elements.inclination = 0.0f;

    // True anomaly (angle from periapsis)
    if (elements.eccentricity > 0.001f) {
        float cosTA = (eVector.x * position.x + eVector.y * position.y) /
            (elements.eccentricity * r);
        elements.trueAnomaly = std::acos(std::clamp(cosTA, -1.0f, 1.0f));

        if (rdotv < 0) {
            elements.trueAnomaly = 2.0f * GameConstants::PI - elements.trueAnomaly;
        }
    }

    return elements;
}

float OrbitMaintenance::calculateFuelCost(float deltaV) const {
    return deltaV / config.fuelEfficiency;
}

bool OrbitMaintenance::requestMaintenanceFuel(float amount) {
    if (!satellite) return false;

    // Check if satellite has enough maintenance fuel
    if (satellite->getMaintenanceFuelReserve() >= amount) {
        // This would need to be implemented in the Satellite class
        // For now, just check availability
        return true;
    }

    return false;
}

void OrbitMaintenance::updateOrbitPrediction() {
    predictedOrbitPath.clear();

    if (!satellite || celestialBodies.empty()) return;

    // Simulate future orbital path
    sf::Vector2f simPosition = satellite->getPosition();
    sf::Vector2f simVelocity = satellite->getVelocity();

    for (int step = 0; step < predictionSteps; step++) {
        predictedOrbitPath.push_back(simPosition);

        // Calculate gravitational forces
        sf::Vector2f totalForce = calculateGravitationalForces(simPosition);
        sf::Vector2f acceleration = totalForce / satellite->getMass();

        // Simple Euler integration (could be improved with RK4)
        simVelocity += acceleration * GameConstants::SATELLITE_PREDICTION_TIME_STEP;
        simPosition += simVelocity * GameConstants::SATELLITE_PREDICTION_TIME_STEP;
    }
}

// Basic implementations for required interface methods
float OrbitMaintenance::getOrbitAccuracy() const {
    if (currentDrift.overallSeverity <= 0.0f) return 100.0f;
    return std::max(0.0f, 100.0f - currentDrift.overallSeverity * 100.0f);
}

bool OrbitMaintenance::planManualCorrection(ManeuverType type, float magnitude) {
    // Basic implementation - create a manual maneuver and add to queue
    OrbitalManeuver manualManeuver;
    manualManeuver.type = type;
    manualManeuver.deltaV = std::min(magnitude, config.maxSingleBurn);
    manualManeuver.fuelCost = calculateFuelCost(manualManeuver.deltaV);
    manualManeuver.urgency = 1.0f;

    // Add to front of queue (high priority)
    plannedManeuvers.insert(plannedManeuvers.begin(), manualManeuver);
    return true;
}

bool OrbitMaintenance::executeImmediateCorrection(float deltaV, sf::Vector2f direction) {
    if (!satellite || deltaV <= 0.0f) return false;

    // Apply velocity change immediately
    satellite->setVelocity(satellite->getVelocity() + normalize(direction) * deltaV);

    // Apply velocity change immediately
    satellite->setVelocity(satellite->getVelocity() + normalize(direction) * deltaV);

    // Update statistics
    totalDeltaVExpended += deltaV;
    totalFuelConsumed += calculateFuelCost(deltaV);
    correctionsPerformed++;
    averageCorrectionSize = totalDeltaVExpended / correctionsPerformed;
    timeSinceLastCorrection = 0.0f;

    return true;
}

void OrbitMaintenance::abortActiveManeuver() {
    if (activeManeuver) {
        activeManeuver->isAborted = true;
        std::cout << "Aborted active maneuver for " << satellite->getName() << std::endl;
    }
}

void OrbitMaintenance::clearPlannedManeuvers() {
    plannedManeuvers.clear();
    std::cout << "Cleared planned maneuvers for " << satellite->getName() << std::endl;
}

float OrbitMaintenance::getFuelEfficiency() const {
    if (totalFuelConsumed <= 0.0f) return 0.0f;

    // Fuel efficiency = orbit accuracy maintained per unit fuel consumed
    float avgAccuracy = getOrbitAccuracy();
    return avgAccuracy / totalFuelConsumed;
}

float OrbitMaintenance::predictFuelRequirement(float timeAhead) const {
    // Predict fuel needed for maintenance over the given time period
    float predictedDrift = currentDrift.overallSeverity * (timeAhead / 3600.0f); // Assume linear drift
    float correctionsNeeded = std::ceil(predictedDrift / 1.0f); // One correction per severity unit

    float fuelPerCorrection = config.maxSingleBurn / config.fuelEfficiency;
    return correctionsNeeded * fuelPerCorrection;
}

bool OrbitMaintenance::predictOrbitDecay(float timeAhead) const {
    if (currentDrift.timeToDecay < 0) return false; // No decay predicted
    return currentDrift.timeToDecay <= timeAhead;
}

void OrbitMaintenance::optimizeStationKeepingStrategy() {
    // Analyze historical performance and adjust strategy
    if (correctionsPerformed > 5) {
        // Calculate efficiency metrics
        float fuelEfficiency = getFuelEfficiency();
        float avgAccuracy = getOrbitAccuracy();

        // Adjust configuration based on performance
        if (fuelEfficiency < 10.0f) { // Low efficiency
            // Reduce correction frequency, allow more drift
            config.orbitToleranceRadius *= 1.1f;
            config.eccentricityTolerance *= 1.1f;
            config.checkInterval *= 1.2f;
        }
        else if (avgAccuracy > 95.0f && fuelEfficiency > 20.0f) {
            // High efficiency and accuracy - can be more aggressive
            config.orbitToleranceRadius *= 0.9f;
            config.eccentricityTolerance *= 0.9f;
            config.checkInterval *= 0.8f;
        }

        // Adjust maneuver preferences based on drift patterns
        if (currentDrift.eccentricityDrift > currentDrift.radiusDrift) {
            config.prioritizeCircularOrbit = true;
        }
        else {
            config.prioritizeCircularOrbit = false;
        }
    }

    std::cout << "Optimized station-keeping strategy for " << satellite->getName() << std::endl;
}

void OrbitMaintenance::adaptToFuelAvailability(float availableFuel) {
    if (!satellite) return;

    // Adjust maintenance strategy based on available fuel
    float fuelRatio = availableFuel / satellite->getMaxFuel();

    if (fuelRatio < 0.2f) {
        // Low fuel - emergency mode
        config.orbitToleranceRadius *= 2.0f;
        config.eccentricityTolerance *= 2.0f;
        config.checkInterval *= 2.0f;
        config.maxSingleBurn *= 0.5f;
        config.preferSmallFrequentBurns = true;

        std::cout << "Satellite " << satellite->getName() << " entering fuel conservation mode" << std::endl;
    }
    else if (fuelRatio > 0.8f) {
        // Plenty of fuel - can be more precise
        config.orbitToleranceRadius *= 0.8f;
        config.eccentricityTolerance *= 0.8f;
        config.checkInterval *= 0.8f;
        config.preferSmallFrequentBurns = false;
    }
}

void OrbitMaintenance::setMaintenancePriority(float priority) {
    // Adjust all tolerances and intervals based on priority (0-1)
    priority = std::clamp(priority, 0.1f, 1.0f);

    float baseToleranceRadius = GameConstants::SATELLITE_ORBIT_TOLERANCE;
    float baseEccentricityTolerance = GameConstants::SATELLITE_ECCENTRICITY_TOLERANCE;
    float baseCheckInterval = GameConstants::SATELLITE_MAINTENANCE_CHECK_INTERVAL;

    // Higher priority = tighter tolerances and more frequent checks
    config.orbitToleranceRadius = baseToleranceRadius / priority;
    config.eccentricityTolerance = baseEccentricityTolerance / priority;
    config.checkInterval = baseCheckInterval / priority;

    std::cout << "Set maintenance priority for " << satellite->getName()
        << " to " << (priority * 100.0f) << "%" << std::endl;
}

void OrbitMaintenance::performEmergencyCorrection() {
    if (!satellite || !currentDrift.requiresImmediateAction) return;

    // Clear all planned maneuvers and execute emergency correction
    clearPlannedManeuvers();

    // Calculate emergency burn to stabilize orbit
    float emergencyDeltaV = std::min(config.maxSingleBurn,
        currentDrift.overallSeverity * 2.0f);

    // Direction based on most critical drift component
    sf::Vector2f burnDirection(0.0f, 0.0f);

    if (std::abs(currentDrift.radiusDrift) > config.orbitToleranceRadius) {
        // Radial correction needed
        burnDirection = normalize(satellite->getVelocity());
        if (currentDrift.radiusDrift > 0) burnDirection = -burnDirection; // Retrograde to lower orbit
    }
    else if (currentDrift.eccentricityDrift > config.eccentricityTolerance) {
        // Circularization needed
        burnDirection = normalize(satellite->getVelocity());
    }

    executeImmediateCorrection(emergencyDeltaV, burnDirection);

    std::cout << "EMERGENCY CORRECTION executed for " << satellite->getName() << std::endl;
}

void OrbitMaintenance::minimizeOrbitDecay() {
    // Emergency procedure when fuel is critically low
    if (!satellite) return;

    // Cancel all non-essential maneuvers
    clearPlannedManeuvers();
    abortActiveManeuver();

    // Only correct the most critical drift component
    float maxDrift = std::max({
        std::abs(currentDrift.radiusDrift) / config.orbitToleranceRadius,
        std::abs(currentDrift.eccentricityDrift) / config.eccentricityTolerance,
        std::abs(currentDrift.inclinationDrift) / config.inclinationTolerance
        });

    if (maxDrift > 2.0f) { // Only if drift is severe
        // Minimal correction to prevent immediate decay
        float minimalDeltaV = std::min(1.0f, config.maxSingleBurn * 0.1f);

        if (std::abs(currentDrift.radiusDrift) / config.orbitToleranceRadius == maxDrift) {
            // Radial drift is most critical
            sf::Vector2f direction = (currentDrift.radiusDrift > 0) ?
                -normalize(satellite->getVelocity()) : normalize(satellite->getVelocity());
            executeImmediateCorrection(minimalDeltaV, direction);
        }
    }

    // Extend all tolerances to minimize future corrections
    config.orbitToleranceRadius *= 3.0f;
    config.eccentricityTolerance *= 3.0f;
    config.inclinationTolerance *= 3.0f;
    config.checkInterval *= 5.0f;

    std::cout << "Orbit decay minimization activated for " << satellite->getName() << std::endl;
}

void OrbitMaintenance::prepareForControlledDeorbit() {
    // Prepare satellite for graceful end-of-life
    if (!satellite) return;

    // Cancel all maintenance operations
    clearPlannedManeuvers();
    abortActiveManeuver();

    // Calculate deorbit burn
    float deorbitDeltaV = 5.0f; // Simple fixed deorbit burn

    // Execute retrograde burn to lower orbit
    sf::Vector2f retrogradeDirection = -normalize(satellite->getVelocity());

    if (satellite->getAvailableFuel() >= calculateFuelCost(deorbitDeltaV)) {
        executeImmediateCorrection(deorbitDeltaV, retrogradeDirection);
        std::cout << "Controlled deorbit initiated for " << satellite->getName() << std::endl;
    }
    else {
        std::cout << "Insufficient fuel for controlled deorbit of " << satellite->getName() << std::endl;
    }
}

void OrbitMaintenance::enableAdaptiveMaintenance(bool enable) {
    config.enablePredictiveCorrections = enable;
    std::cout << "Adaptive maintenance " << (enable ? "enabled" : "disabled")
        << " for " << satellite->getName() << std::endl;
}

void OrbitMaintenance::setMaintenanceAggressiveness(float level) {
    level = std::clamp(level, 0.1f, 2.0f);

    // Adjust tolerances based on aggressiveness
    config.orbitToleranceRadius = GameConstants::SATELLITE_ORBIT_TOLERANCE / level;
    config.eccentricityTolerance = GameConstants::SATELLITE_ECCENTRICITY_TOLERANCE / level;
    config.checkInterval = GameConstants::SATELLITE_MAINTENANCE_CHECK_INTERVAL / level;

    std::cout << "Set maintenance aggressiveness to " << (level * 100.0f)
        << "% for " << satellite->getName() << std::endl;
}

void OrbitMaintenance::enableCollaborativeMaintenance(bool enable) {
    // Placeholder for future collaborative maintenance between satellites
    std::cout << "Collaborative maintenance " << (enable ? "enabled" : "disabled")
        << " for " << satellite->getName() << std::endl;
}

float OrbitMaintenance::calculateMaintenanceFuelReserve() const {
    if (!satellite) return 0.0f;

    // Calculate recommended fuel reserve based on expected corrections
    float baseFuel = config.maxSingleBurn / config.fuelEfficiency;
    float driftFactor = 1.0f + currentDrift.overallSeverity;

    return baseFuel * driftFactor * 3.0f; // Reserve for 3 corrections
}

void OrbitMaintenance::updateFuelReserveRequirements() {
    // Update satellite's fuel reserve based on maintenance needs
    float recommendedReserve = calculateMaintenanceFuelReserve();
    // This would need integration with the satellite's fuel management

    std::cout << "Recommended fuel reserve for " << satellite->getName()
        << ": " << recommendedReserve << " units" << std::endl;
}

std::vector<std::string> OrbitMaintenance::generateMaintenanceReport() const {
    std::vector<std::string> report;

    if (!satellite) {
        report.push_back("ERROR: No satellite associated with maintenance system");
        return report;
    }

    // Header
    report.push_back("=== ORBITAL MAINTENANCE REPORT ===");
    report.push_back("Satellite: " + satellite->getName());

    // Current orbital state
    std::stringstream ss;
    ss << "Semi-major axis: " << std::fixed << std::setprecision(1) << lastKnownOrbit.semiMajorAxis;
    report.push_back(ss.str());

    ss.str("");
    ss << "Eccentricity: " << std::setprecision(4) << lastKnownOrbit.eccentricity;
    report.push_back(ss.str());

    ss.str("");
    ss << "Orbit accuracy: " << std::setprecision(1) << getOrbitAccuracy() << "%";
    report.push_back(ss.str());

    // Drift analysis
    report.push_back("");
    report.push_back("--- DRIFT ANALYSIS ---");

    ss.str("");
    ss << "Overall severity: " << std::setprecision(1) << (currentDrift.overallSeverity * 100.0f) << "%";
    report.push_back(ss.str());

    ss.str("");
    ss << "Radius drift: " << std::setprecision(2) << currentDrift.radiusDrift;
    report.push_back(ss.str());

    ss.str("");
    ss << "Eccentricity drift: " << std::setprecision(4) << currentDrift.eccentricityDrift;
    report.push_back(ss.str());

    if (currentDrift.timeToDecay > 0) {
        ss.str("");
        ss << "Time to decay: " << std::setprecision(0) << (currentDrift.timeToDecay / 3600.0f) << " hours";
        report.push_back(ss.str());
    }

    // Performance metrics
    report.push_back("");
    report.push_back("--- PERFORMANCE METRICS ---");

    ss.str("");
    ss << "Corrections performed: " << correctionsPerformed;
    report.push_back(ss.str());

    ss.str("");
    ss << "Total fuel consumed: " << std::setprecision(2) << totalFuelConsumed;
    report.push_back(ss.str());

    ss.str("");
    ss << "Fuel efficiency: " << std::setprecision(1) << getFuelEfficiency();
    report.push_back(ss.str());

    ss.str("");
    ss << "Average correction size: " << std::setprecision(3) << averageCorrectionSize;
    report.push_back(ss.str());

    // Status and recommendations
    report.push_back("");
    report.push_back("--- STATUS & RECOMMENDATIONS ---");

    if (currentDrift.requiresImmediateAction) {
        report.push_back("CRITICAL: Immediate correction required!");
    }
    else if (currentDrift.requiresAction) {
        report.push_back("WARNING: Orbital correction needed");
    }
    else {
        report.push_back("STATUS: Orbit stable");
    }

    if (currentDrift.gravitationalPerturbation) {
        report.push_back("Note: Gravitational perturbations detected");
    }

    if (currentDrift.resonanceEffects) {
        report.push_back("Note: Orbital resonance effects detected");
    }

    report.push_back("=====================================");

    return report;
}

void OrbitMaintenance::logMaintenanceEvent(const std::string& event) {
    std::cout << "[" << satellite->getName() << " Maintenance] " << event << std::endl;
}

bool OrbitMaintenance::validateOrbitIntegrity() const {
    if (!satellite) return false;

    // Check if orbit parameters are reasonable
    if (lastKnownOrbit.semiMajorAxis <= 0) return false;
    if (lastKnownOrbit.eccentricity < 0 || lastKnownOrbit.eccentricity >= 1.0f) return false;

    // Check if satellite is not inside planet
    for (auto* planet : celestialBodies) {
        float dist = distance(satellite->getPosition(), planet->getPosition());
        if (dist <= planet->getRadius()) return false;
    }

    return true;
}

// Static utility methods
float OrbitMaintenance::calculateOrbitPeriod(float semiMajorAxis, float centralMass) {
    return 2.0f * GameConstants::PI * std::sqrt(std::pow(semiMajorAxis, 3) /
        (GameConstants::G * centralMass));
}

float OrbitMaintenance::calculateOrbitalVelocity(float radius, float centralMass) {
    return std::sqrt(GameConstants::G * centralMass / radius);
}

float OrbitMaintenance::calculateEscapeVelocity(float radius, float centralMass) {
    return std::sqrt(2.0f * GameConstants::G * centralMass / radius);
}

OrbitalElements OrbitMaintenance::convertStateToElements(sf::Vector2f position, sf::Vector2f velocity, float centralMass) {
    // Static version of calculateOrbitalElements
    OrbitalElements elements;

    float r = std::sqrt(position.x * position.x + position.y * position.y);
    float v = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

    float energy = 0.5f * v * v - centralMass / r;

    if (energy < 0) {
        elements.semiMajorAxis = -centralMass / (2.0f * energy);
    }
    else {
        elements.semiMajorAxis = std::numeric_limits<float>::infinity();
    }

    // Calculate eccentricity vector
    sf::Vector2f eVector;
    float vSquared = v * v;
    float rdotv = position.x * velocity.x + position.y * velocity.y;

    eVector.x = (vSquared * position.x - rdotv * velocity.x) / centralMass - position.x / r;
    eVector.y = (vSquared * position.y - rdotv * velocity.y) / centralMass - position.y / r;

    elements.eccentricity = std::sqrt(eVector.x * eVector.x + eVector.y * eVector.y);
    elements.inclination = 0.0f; // 2D simulation

    return elements;
}

std::pair<sf::Vector2f, sf::Vector2f> OrbitMaintenance::convertElementsToState(const OrbitalElements& elements, float centralMass) {
    // Convert orbital elements back to position and velocity vectors
    float semiMajorAxis = elements.semiMajorAxis;
    float eccentricity = elements.eccentricity;
    float trueAnomaly = elements.trueAnomaly;

    // Calculate radius at current true anomaly
    float radius = semiMajorAxis * (1 - eccentricity * eccentricity) /
        (1 + eccentricity * std::cos(trueAnomaly));

    // Position in orbital plane
    sf::Vector2f position;
    position.x = radius * std::cos(trueAnomaly);
    position.y = radius * std::sin(trueAnomaly);

    // Velocity calculation (simplified)
    float mu = centralMass;
    float h = std::sqrt(mu * semiMajorAxis * (1 - eccentricity * eccentricity));

    sf::Vector2f velocity;
    velocity.x = -mu / h * std::sin(trueAnomaly);
    velocity.y = mu / h * (eccentricity + std::cos(trueAnomaly));

    return std::make_pair(position, velocity);
}

// Visual debugging methods (basic implementations)
void OrbitMaintenance::drawPlannedManeuvers(sf::RenderWindow& window) const {
    if (!satellite || plannedManeuvers.empty()) return;

    sf::Vector2f satellitePos = satellite->getPosition();

    for (size_t i = 0; i < plannedManeuvers.size(); i++) {
        const auto& maneuver = plannedManeuvers[i];

        // Draw maneuver vector
        sf::VertexArray maneuverLine(sf::PrimitiveType::LineStrip);

        sf::Color maneuverColor = maneuver.isEmergency ?
            sf::Color::Red : GameConstants::SATELLITE_MAINTENANCE_BURN_COLOR;

        sf::Vertex start;
        start.position = satellitePos;
        start.color = maneuverColor;
        maneuverLine.append(start);

        sf::Vertex end;
        end.position = satellitePos + maneuver.burnVector * 50.0f; // Scale for visibility
        end.color = maneuverColor;
        maneuverLine.append(end);

        window.draw(maneuverLine);
    }
}

void OrbitMaintenance::drawOrbitDrift(sf::RenderWindow& window) const {
    if (!satellite) return;

    // Draw drift severity indicator
    sf::Vector2f satellitePos = satellite->getPosition();
    sf::CircleShape driftIndicator;
    float radius = 15.0f + currentDrift.overallSeverity * 20.0f;
    driftIndicator.setRadius(radius);
    driftIndicator.setPosition({ satellitePos.x - radius, satellitePos.y - radius });
    driftIndicator.setFillColor(sf::Color::Transparent);

    // Color based on drift severity
    if (currentDrift.requiresImmediateAction) {
        driftIndicator.setOutlineColor(sf::Color::Red);
    }
    else if (currentDrift.requiresAction) {
        driftIndicator.setOutlineColor(sf::Color::Yellow);
    }
    else {
        driftIndicator.setOutlineColor(sf::Color::Green);
    }

    driftIndicator.setOutlineThickness(2.0f);
    window.draw(driftIndicator);
}

void OrbitMaintenance::drawPredictedPath(sf::RenderWindow& window) const {
    if (predictedOrbitPath.size() < 2) return;

    sf::VertexArray pathLine(sf::PrimitiveType::LineStrip);

    for (size_t i = 0; i < predictedOrbitPath.size(); i++) {
        sf::Vertex point;
        point.position = predictedOrbitPath[i];

        // Fade from current color to transparent
        float alpha = 255.0f * (1.0f - static_cast<float>(i) / predictedOrbitPath.size());
        point.color = sf::Color(100, 150, 255, static_cast<uint8_t>(alpha));

        pathLine.append(point);
    }

    window.draw(pathLine);
}

void OrbitMaintenance::drawMaintenanceBurns(sf::RenderWindow& window) const {
    if (!activeManeuver || !satellite) return;

    // Draw active burn visualization
    sf::Vector2f satellitePos = satellite->getPosition();
    sf::Vector2f burnDirection = normalize(activeManeuver->burnVector);

    // Simple burn effect
    for (int i = 0; i < 3; i++) {
        sf::CircleShape burnParticle;
        burnParticle.setRadius(3.0f);

        sf::Vector2f particlePos = satellitePos - burnDirection * (15.0f + i * 8.0f);
        burnParticle.setPosition({ particlePos.x - 3.0f, particlePos.y - 3.0f });

        sf::Color burnColor = GameConstants::SATELLITE_MAINTENANCE_BURN_COLOR;
        burnColor.a = static_cast<uint8_t>(200 - i * 60);
        burnParticle.setFillColor(burnColor);

        window.draw(burnParticle);
    }
}