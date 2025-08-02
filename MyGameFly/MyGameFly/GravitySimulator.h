// GravitySimulator.h - Updated with Player Support and Fuel System
#pragma once
#include "Planet.h"
#include "Rocket.h"
#include "Satellite.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <vector>

// Forward declarations
class VehicleManager;
class Player;
class Satellite;
class SatelliteManager;

class GravitySimulator {
private:
    std::vector<Planet*> planets;
    std::vector<Rocket*> rockets;
    VehicleManager* vehicleManager = nullptr;  // Single vehicle manager (legacy)
    std::vector<VehicleManager*> vehicleManagers;  // Multiple vehicle managers for split-screen
    std::vector<Player*> players;  // Multiple players for network multiplayer

    const float G = GameConstants::G;
    bool simulatePlanetGravity = true;
    SatelliteManager* satelliteManager = nullptr;
    std::vector<Satellite*> satellites;

public:
    void addPlanet(Planet* planet);
    void addRocket(Rocket* rocket);
    void addVehicleManager(VehicleManager* manager);

    // Player support methods
    void addPlayer(Player* player);
    void removePlayer(int playerID);
    const std::vector<Player*>& getPlayers() const { return players; }

    void update(float deltaTime);
    void clearRockets();
    void addRocketGravityInteractions(float deltaTime);

    // Player-specific gravity methods
    void applyGravityToPlayers(float deltaTime);
    void addPlayerGravityInteractions(float deltaTime);

    // Fuel system integration
    void processFuelCollectionForAllRockets(float deltaTime);
    void processFuelCollectionForVehicleManagers(float deltaTime);
    void processFuelCollectionForPlayers(float deltaTime);

    const std::vector<Planet*>& getPlanets() const { return planets; }
    void setSimulatePlanetGravity(bool enable) { simulatePlanetGravity = enable; }

    void addSatellite(Satellite* satellite);
    void addSatelliteManager(SatelliteManager* satManager);
};