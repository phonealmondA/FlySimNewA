#pragma once
#include "VehicleManager.h"
#include "Planet.h"

#include <memory>
#include <vector>
class SatelliteManager;

class SplitScreenManager {
private:
    std::unique_ptr<VehicleManager> player1;
    std::unique_ptr<VehicleManager> player2;
    std::vector<Planet*> planets;
    SatelliteManager* satelliteManager;
    // Input tracking
    bool player1LKeyPressed;
    bool player2LKeyPressed;
    // Satellite system integration
    bool player1TKeyPressed;
    bool player2TKeyPressed;

public:

	// Constructor
    SplitScreenManager(sf::Vector2f player1Pos, sf::Vector2f player2Pos, const std::vector<Planet*>& planetList, SatelliteManager* satManager = nullptr);
    ~SplitScreenManager() = default;

    // Input handling
    void handlePlayer1Input(float deltaTime);
    void handlePlayer2Input(float deltaTime);
    void handleTransformInputs(const sf::Event& event);
    void setSyncedThrustLevel(float level);  // New method for synced thrust

    // Update and rendering
    void update(float deltaTime);
    void draw(sf::RenderWindow& window);
    void drawWithConstantSize(sf::RenderWindow& window, float zoomLevel);
    void drawVelocityVectors(sf::RenderWindow& window, float scale = 1.0f);

    // Getters
    VehicleManager* getPlayer1() { return player1.get(); }
    VehicleManager* getPlayer2() { return player2.get(); }
    sf::Vector2f getCenterPoint();

    // Camera helper
    float getRequiredZoomToShowBothPlayers(sf::Vector2u windowSize);
    // Satellite system methods
    void handleSatelliteConversionInputs(const sf::Event& event);
    bool canPlayerConvertToSatellite(int playerIndex) const;
    sf::Vector2f findNearestPlanetSurface(sf::Vector2f position, int playerIndex) const;
    void updateSatelliteManager();
};