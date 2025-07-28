// GravitySimulator.h - Updated with Player Support
#pragma once
#include "Planet.h"
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <vector>

// Forward declarations
class VehicleManager;
// class Player;  // TODO: Uncomment when Player.h is created

class GravitySimulator {
private:
    std::vector<Planet*> planets;
    std::vector<Rocket*> rockets;
    VehicleManager* vehicleManager = nullptr;  // Single vehicle manager (legacy)
    std::vector<VehicleManager*> vehicleManagers;  // Multiple vehicle managers for split-screen

    // TODO: Player support - uncomment when Player class is ready
    // std::vector<Player*> players;  // Multiple players for network multiplayer

    const float G = GameConstants::G;
    bool simulatePlanetGravity = true;

public:
    void addPlanet(Planet* planet);
    void addRocket(Rocket* rocket);
    void addVehicleManager(VehicleManager* manager);  // Now adds to the vector

    // TODO: Player support methods - uncomment when Player class is ready
    // void addPlayer(Player* player);
    // void removePlayer(int playerID);
    // const std::vector<Player*>& getPlayers() const { return players; }

    void update(float deltaTime);
    void clearRockets();
    void addRocketGravityInteractions(float deltaTime);

    // TODO: Player-specific gravity methods - uncomment when Player class is ready
    // void applyGravityToPlayers(float deltaTime);
    // void addPlayerGravityInteractions(float deltaTime);

    const std::vector<Planet*>& getPlanets() const { return planets; }
    void setSimulatePlanetGravity(bool enable) { simulatePlanetGravity = enable; }
};