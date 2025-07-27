// Update in GravitySimulator.h
#pragma once
#include "Planet.h"
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"  // Include the constants
#include <vector>

// Forward declaration
class VehicleManager;

class GravitySimulator {
private:
    std::vector<Planet*> planets;
    std::vector<Rocket*> rockets;
    VehicleManager* vehicleManager = nullptr;
    const float G = GameConstants::G;  // Use the constant from the header
    bool simulatePlanetGravity = true;

public:
    void addPlanet(Planet* planet);
    void addRocket(Rocket* rocket);
    void addVehicleManager(VehicleManager* manager) { vehicleManager = manager; }
    void update(float deltaTime);
    void clearRockets();
    void addRocketGravityInteractions(float deltaTime);

    const std::vector<Planet*>& getPlanets() const { return planets; }
    void setSimulatePlanetGravity(bool enable) { simulatePlanetGravity = enable; }
};