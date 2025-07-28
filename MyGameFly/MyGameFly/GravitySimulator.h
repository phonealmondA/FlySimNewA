// GravitySimulator.h - Updated
#pragma once
#include "Planet.h"
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <vector>

// Forward declaration
class VehicleManager;

class GravitySimulator {
private:
    std::vector<Planet*> planets;
    std::vector<Rocket*> rockets;
    VehicleManager* vehicleManager = nullptr;  // Single vehicle manager (legacy)
    std::vector<VehicleManager*> vehicleManagers;  // Multiple vehicle managers for split-screen
    const float G = GameConstants::G;
    bool simulatePlanetGravity = true;

public:
    void addPlanet(Planet* planet);
    void addRocket(Rocket* rocket);
    void addVehicleManager(VehicleManager* manager);  // Now adds to the vector
    void update(float deltaTime);
    void clearRockets();
    void addRocketGravityInteractions(float deltaTime);

    const std::vector<Planet*>& getPlanets() const { return planets; }
    void setSimulatePlanetGravity(bool enable) { simulatePlanetGravity = enable; }
};