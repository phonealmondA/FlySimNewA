#pragma once
#include "VehicleManager.h"
#include "Planet.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

// Forward declaration for NetworkManager data structures
struct PlayerInput;
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

    // Input state
    PlayerInput currentInput;
    bool inputChanged;

    // Visual identification (since no colors yet)
    std::string playerName;

    // Physics/game state
    std::vector<Planet*> planets;

public:
    // Constructor
    Player(int id, sf::Vector2f spawnPos, PlayerType playerType, const std::vector<Planet*>& planetList);
    ~Player() = default;

    // Core update and rendering
    void update(float deltaTime);
    void draw(sf::RenderWindow& window);
    void drawWithConstantSize(sf::RenderWindow& window, float zoomLevel);

    // Input handling
    void handleLocalInput(float deltaTime);  // For LOCAL players
    void applyNetworkInput(const PlayerInput& input);  // For REMOTE players
    PlayerInput getCurrentInput() const;
    bool hasInputChanged() const { return inputChanged; }
    void markInputProcessed() { inputChanged = false; }

    // State management for networking
    PlayerState getState() const;
    void applyState(const PlayerState& state);

    // Individual player controls (replaces shared controls)
    void setThrustLevel(float level);
    float getThrustLevel() const;
    void applyThrust(float amount);
    void rotate(float amount);
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
    void drawPlayerLabel(sf::RenderWindow& window, const sf::Font& font);  // Show "Player X"

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
    void updateInputFromKeyboard();
    PlayerInput createInputFromState() const;
};