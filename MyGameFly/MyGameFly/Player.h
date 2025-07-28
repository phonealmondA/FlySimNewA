#pragma once
#include "VehicleManager.h"
#include "Planet.h"
#include "NetworkManager.h"  // Include this instead of forward declaration
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

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

    // Setters
    void setType(PlayerType newType) { type = newType; }
    void setName(const std::string& name) { playerName = name; }

    // Visual helpers
    void drawVelocityVector(sf::RenderWindow& window, float scale = 1.0f);
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