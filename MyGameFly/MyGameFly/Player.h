#pragma once
#ifndef PLAYER_H
#define PLAYER_H

#include "VehicleManager.h"
#include "Planet.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

// Forward declaration to avoid circular dependency
class NetworkManager;
struct PlayerState;

enum class PlayerType {
    LOCAL,    // This instance controls this player
    REMOTE    // This player is controlled by network/other source
};

class Player {
private:
    int playerID;
    std::unique_ptr<VehicleManager> vehicleManager;
    PlayerType type;
    sf::Vector2f spawnPosition;

    // Visual identification
    std::string playerName;

    // Physics/game state
    std::vector<Planet*> planets;

    // State tracking for networking
    bool stateChanged;
    float timeSinceLastStateSent;
    static constexpr float STATE_SEND_INTERVAL = 1.0f / 30.0f; // 30 FPS state sync

    // MANUAL FUEL TRANSFER INPUT TRACKING
    bool fuelIncreaseKeyPressed;    // '.' key state
    bool fuelDecreaseKeyPressed;    // ',' key state

public:
    // Constructor
    Player(int id, sf::Vector2f spawnPos, PlayerType playerType, const std::vector<Planet*>& planetList);
    ~Player() = default;

    // Core update and rendering
    void update(float deltaTime);
    void draw(sf::RenderWindow& window);
    void drawWithConstantSize(sf::RenderWindow& window, float zoomLevel);

    // Input handling - ONLY for LOCAL players
    void handleLocalInput(float deltaTime);

    // MANUAL FUEL TRANSFER INPUT HANDLING
    void handleFuelTransferInput(float deltaTime);  // Process fuel transfer controls

    // State management for networking
    PlayerState getState() const;
    void applyState(const PlayerState& state);
    bool shouldSendState() const;
    void markStateSent() { timeSinceLastStateSent = 0.0f; }

    // Transform handling
    void requestTransform();

    // Getters
    int getID() const { return playerID; }
    PlayerType getType() const { return type; }
    sf::Vector2f getPosition() const;
    sf::Vector2f getVelocity() const;
    VehicleManager* getVehicleManager() const { return vehicleManager.get(); }
    std::string getName() const { return playerName; }

    // FUEL SYSTEM GETTERS (updated for dynamic mass)
    float getCurrentFuel() const;
    float getMaxFuel() const;
    float getFuelPercentage() const;
    bool canThrust() const;
    bool isCollectingFuel() const;
    float getCurrentMass() const;         // NEW: Get current rocket mass
    float getMaxMass() const;            // NEW: Get max rocket mass
    float getMassPercentage() const;     // NEW: Get mass as percentage

    // Setters
    void setType(PlayerType newType) { type = newType; }
    void setName(const std::string& name) { playerName = name; }

    // Visual helpers
    void drawVelocityVector(sf::RenderWindow& window, float scale = 1.0f);
    void drawFuelCollectionIndicator(sf::RenderWindow& window);
    void drawPlayerLabel(sf::RenderWindow& window, const sf::Font& font);

    // Physics integration
    void setNearbyPlanets(const std::vector<Planet*>& planetList) {
        planets = planetList;
        if (vehicleManager) {
            vehicleManager->getRocket()->setNearbyPlanets(planetList);
        }
    }

    // Spawn point management
    sf::Vector2f getSpawnPosition() const { return spawnPosition; }
    void respawnAtPosition(sf::Vector2f newSpawnPos);

private:
    // Helper methods
    void initializeVehicleManager();
    void updateInputFromKeyboard(float deltaTime);
};

#endif // PLAYER_H