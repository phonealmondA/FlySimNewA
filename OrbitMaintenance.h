#pragma once
#ifndef ORBIT_MAINTENANCE_H
#define ORBIT_MAINTENANCE_H

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

// Forward declarations
class Satellite;
class Planet;
struct OrbitalElements;

// ADD THIS STRUCT DEFINITION:
struct OrbitalElements {
    float semiMajorAxis = 0.0f;     // a - size of orbit
    float eccentricity = 0.0f;       // e - shape of orbit (0 = circular)
    float inclination = 0.0f;        // i - orbital plane angle
    float longitudeOfAscendingNode = 0.0f;  // ? - orientation in space
    float argumentOfPeriapsis = 0.0f;       // ? - orientation in orbital plane
    float trueAnomaly = 0.0f;               // ? - position in orbit

    // Convenience methods
    float getApoapsis() const { return semiMajorAxis * (1 + eccentricity); }
    float getPeriapsis() const { return semiMajorAxis * (1 - eccentricity); }
    float getOrbitalPeriod(float centralMass) const;
    float getOrbitalVelocity(float centralMass, float radius) const;
};

// Types of orbital maneuvers
enum class ManeuverType {
    PROGRADE,           // Speed up (raise apoapsis)
    RETROGRADE,         // Slow down (lower apoapsis)
    NORMAL,             // Raise inclination
    ANTI_NORMAL,        // Lower inclination
    RADIAL_IN,          // Move toward central body
    RADIAL_OUT,         // Move away from central body
    CIRCULARIZE,        // Reduce eccentricity
    PLANE_CHANGE        // Change orbital plane
};

// Orbital correction maneuver
struct OrbitalManeuver {
    ManeuverType type;
    sf::Vector2f burnVector;        // Direction and magnitude of burn
    float deltaV;                   // Total velocity change required
    float fuelCost;                 // Estimated fuel consumption
    float duration;                 // Time to execute maneuver
    float urgency;                  // Priority level (0-1, 1=urgent)
    bool isEmergency;               // Critical orbital decay

    // Execution tracking
    sf::Vector2f executedBurn;      // Already executed portion
    float remainingDeltaV;          // Remaining velocity change
    bool isComplete;                // Maneuver finished
    bool isAborted;                 // Maneuver cancelled

    OrbitalManeuver() : type(ManeuverType::PROGRADE), deltaV(0.0f), fuelCost(0.0f),
        duration(0.0f), urgency(0.0f), isEmergency(false),
        remainingDeltaV(0.0f), isComplete(false), isAborted(false) {
    }
};

// Drift detection and analysis
struct OrbitDriftAnalysis {
    float radiusDrift;              // Difference in orbital radius (units)
    float eccentricityDrift;        // Change in orbital eccentricity
    float inclinationDrift;         // Change in orbital inclination (radians)
    float periodDrift;              // Change in orbital period (seconds)
    float apoapsisDrift;            // Change in apoapsis distance
    float periapsisDrift;           // Change in periapsis distance

    // Drift rates (per second)
    float radiusDriftRate;
    float eccentricityDriftRate;
    float inclinationDriftRate;

    // Severity assessment
    float overallSeverity;          // Combined drift severity (0-1)
    bool requiresImmediateAction;   // Critical drift detected
    bool requiresAction;            // Non-critical but needs correction
    float timeToDecay;              // Estimated time until orbital decay (seconds)

    // Drift causes
    bool gravitationalPerturbation; // Other bodies affecting orbit
    bool atmosphericDrag;           // Atmospheric drag (if applicable)
    bool resonanceEffects;          // Orbital resonance with other bodies
    bool thrustingEffects;          // Previous thrust maneuvers

    OrbitDriftAnalysis() : radiusDrift(0.0f), eccentricityDrift(0.0f), inclinationDrift(0.0f),
        periodDrift(0.0f), apoapsisDrift(0.0f), periapsisDrift(0.0f),
        radiusDriftRate(0.0f), eccentricityDriftRate(0.0f), inclinationDriftRate(0.0f),
        overallSeverity(0.0f), requiresImmediateAction(false), requiresAction(false),
        timeToDecay(-1.0f), gravitationalPerturbation(false), atmosphericDrag(false),
        resonanceEffects(false), thrustingEffects(false) {
    }
};

// Station-keeping strategy configuration
struct StationKeepingConfig {
    float orbitToleranceRadius;     // Acceptable drift in orbital radius
    float eccentricityTolerance;    // Acceptable change in eccentricity
    float inclinationTolerance;     // Acceptable change in inclination (radians)
    float periodTolerance;          // Acceptable change in orbital period

    // Correction parameters
    float maxSingleBurn;            // Maximum delta-V per correction
    float fuelEfficiency;           // Fuel efficiency for this satellite
    float thrustToWeightRatio;      // Satellite's thrust capabilities

    // Timing parameters
    float checkInterval;            // How often to check orbit (seconds)
    float correctionDelay;          // Delay before executing corrections
    float emergencyThreshold;       // Severity that triggers emergency correction

    // Strategy preferences
    bool preferSmallFrequentBurns;  // Many small corrections vs fewer large ones
    bool prioritizeCircularOrbit;   // Focus on maintaining circular orbit
    bool prioritizeInclination;     // Focus on maintaining orbital plane
    bool enablePredictiveCorrections; // Anticipate drift and correct early

    StationKeepingConfig() : orbitToleranceRadius(50.0f), eccentricityTolerance(0.01f),
        inclinationTolerance(0.017f), periodTolerance(10.0f),
        maxSingleBurn(5.0f), fuelEfficiency(1.0f), thrustToWeightRatio(0.1f),
        checkInterval(30.0f), correctionDelay(5.0f), emergencyThreshold(0.8f),
        preferSmallFrequentBurns(true), prioritizeCircularOrbit(true),
        prioritizeInclination(false), enablePredictiveCorrections(true) {
    }
};

class OrbitMaintenance {
private:
    // Associated satellite
    Satellite* satellite;
    std::vector<Planet*> celestialBodies;

    // Current state tracking
    OrbitalElements lastKnownOrbit;
    OrbitalElements targetOrbit;
    OrbitDriftAnalysis currentDrift;

    // Maintenance configuration
    StationKeepingConfig config;

    // Maneuver planning and execution
    std::vector<OrbitalManeuver> plannedManeuvers;
    std::unique_ptr<OrbitalManeuver> activeManeuver;
    float maneuverExecutionTimer;

    // Timing and scheduling
    float timeSinceLastCheck;
    float timeSinceLastCorrection;
    float nextScheduledCheck;

    // Performance tracking
    float totalFuelConsumed;
    int correctionsPerformed;
    float averageCorrectionSize;
    float totalDeltaVExpended;

    // Predictive analysis
    std::vector<sf::Vector2f> predictedOrbitPath;
    float predictionTimeHorizon;
    int predictionSteps;

    // Internal calculation methods
    void updateCurrentOrbit();
    void analyzeDrift();
    void calculateRequiredManeuvers();
    void optimizeManeuverSequence();

    // Physics calculations
    sf::Vector2f calculateGravitationalForces(sf::Vector2f position) const;
    OrbitalElements calculateOrbitalElements(sf::Vector2f position, sf::Vector2f velocity, float centralMass) const;
    sf::Vector2f calculateBurnVector(ManeuverType maneuverType, float magnitude) const;
    float calculateFuelCost(float deltaV) const;

    // Drift analysis
    void detectGravitationalPerturbations();
    void detectResonanceEffects();
    void assessDriftSeverity();
    float predictTimeToDecay() const;

    // Maneuver planning
    OrbitalManeuver planProgradeCorrection(float targetRadiusChange);
    OrbitalManeuver planCircularizationBurn();
    OrbitalManeuver planInclinationCorrection(float targetInclination);
    std::vector<OrbitalManeuver> planMultiStepCorrection();

    // Execution
    void executeManeuver(OrbitalManeuver& maneuver, float deltaTime);
    bool validateManeuverExecution(const OrbitalManeuver& maneuver) const;

public:
    OrbitMaintenance(Satellite* sat);
    ~OrbitMaintenance() = default;

    // Core maintenance cycle
    void update(float deltaTime);
    void performMaintenanceCheck();
    void executeScheduledManeuvers(float deltaTime);

    // Configuration
    void setStationKeepingConfig(const StationKeepingConfig& newConfig) { config = newConfig; }
    StationKeepingConfig getStationKeepingConfig() const { return config; }

    void setTargetOrbit(const OrbitalElements& orbit) { targetOrbit = orbit; }
    OrbitalElements getTargetOrbit() const { return targetOrbit; }

    void setCelestialBodies(const std::vector<Planet*>& bodies) { celestialBodies = bodies; }

    // Manual control
    bool planManualCorrection(ManeuverType type, float magnitude);
    bool executeImmediateCorrection(float deltaV, sf::Vector2f direction);
    void abortActiveManeuver();
    void clearPlannedManeuvers();

    // Status and analysis
    OrbitDriftAnalysis getCurrentDrift() const { return currentDrift; }
    bool requiresCorrection() const { return currentDrift.requiresAction; }
    bool requiresEmergencyCorrection() const { return currentDrift.requiresImmediateAction; }
    float getOrbitAccuracy() const; // Returns 0-100% accuracy

    // Performance metrics
    float getTotalFuelConsumed() const { return totalFuelConsumed; }
    int getCorrectionsPerformed() const { return correctionsPerformed; }
    float getAverageCorrectionSize() const { return averageCorrectionSize; }
    float getFuelEfficiency() const; // Fuel per unit accuracy maintained

    // Predictive capabilities
    void updateOrbitPrediction();
    std::vector<sf::Vector2f> getPredictedOrbitPath() const { return predictedOrbitPath; }
    float predictFuelRequirement(float timeAhead) const;
    bool predictOrbitDecay(float timeAhead) const;

    // Optimization
    void optimizeStationKeepingStrategy();
    void adaptToFuelAvailability(float availableFuel);
    void setMaintenancePriority(float priority); // 0-1, affects correction timing

    // Emergency procedures
    void performEmergencyCorrection();
    void minimizeOrbitDecay(); // Last resort when fuel is critically low
    void prepareForControlledDeorbit(); // Graceful deorbiting

    // Visual debugging and monitoring
    void drawPlannedManeuvers(sf::RenderWindow& window) const;
    void drawOrbitDrift(sf::RenderWindow& window) const;
    void drawPredictedPath(sf::RenderWindow& window) const;
    void drawMaintenanceBurns(sf::RenderWindow& window) const;

    // Integration with fuel system
    bool requestMaintenanceFuel(float amount);
    float calculateMaintenanceFuelReserve() const;
    void updateFuelReserveRequirements();

    // Advanced features
    void enableAdaptiveMaintenance(bool enable); // AI-driven maintenance optimization
    void setMaintenanceAggressiveness(float level); // How proactive to be (0-1)
    void enableCollaborativeMaintenance(bool enable); // Coordinate with other satellites

    // Diagnostic and reporting
    std::vector<std::string> generateMaintenanceReport() const;
    void logMaintenanceEvent(const std::string& event);
    bool validateOrbitIntegrity() const;

    // Static utility methods
    static float calculateOrbitPeriod(float semiMajorAxis, float centralMass);
    static float calculateOrbitalVelocity(float radius, float centralMass);
    static float calculateEscapeVelocity(float radius, float centralMass);
    static OrbitalElements convertStateToElements(sf::Vector2f position, sf::Vector2f velocity, float centralMass);
    static std::pair<sf::Vector2f, sf::Vector2f> convertElementsToState(const OrbitalElements& elements, float centralMass);
};

#endif // ORBIT_MAINTENANCE_H