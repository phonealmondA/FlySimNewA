#pragma once
#ifndef GAMEINFODISPLAY_H
#define GAMEINFODISPLAY_H

#include "TextPanel.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
#include "VectorHelper.h"

// Forward declarations
class VehicleManager;
class SplitScreenManager;
class Player;
class Planet;
class Rocket;
class Car;
class NetworkManager;

// Forward declare GameState instead of including UIManager.h to avoid circular dependency
enum class GameState;

class GameInfoDisplay {
private:
    // UI panels
    std::unique_ptr<TextPanel> rocketInfoPanel;
    std::unique_ptr<TextPanel> planetInfoPanel;
    std::unique_ptr<TextPanel> orbitInfoPanel;
    std::unique_ptr<TextPanel> controlsPanel;
    std::unique_ptr<TextPanel> networkInfoPanel;

    // Font reference
    const sf::Font* font;
    bool fontLoaded;

    // Helper methods for panel content generation
    std::string generateVehicleInfo(GameState currentState, VehicleManager* vehicleManager,
        SplitScreenManager* splitScreenManager, Player* localPlayer) const;
    std::string generatePlanetInfo(GameState currentState, VehicleManager* vehicleManager,
        SplitScreenManager* splitScreenManager, Player* localPlayer,
        const std::vector<Planet*>& planets) const;
    std::string generateOrbitInfo(GameState currentState, VehicleManager* vehicleManager,
        SplitScreenManager* splitScreenManager, NetworkManager* networkManager,
        Player* localPlayer) const;
    std::string generateNetworkInfo(NetworkManager* networkManager, Player* localPlayer) const;

    // Utility methods for calculations
    float calculateApoapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G) const;
    float calculatePeriapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G) const;
    Planet* findClosestPlanet(sf::Vector2f position, const std::vector<Planet*>& planets) const;

public:
    GameInfoDisplay(const sf::Font& gameFont, sf::Vector2u windowSize);
    ~GameInfoDisplay() = default;

    // Core update and render methods
    void updateAllPanels(GameState currentState, VehicleManager* vehicleManager,
        SplitScreenManager* splitScreenManager, Player* localPlayer,
        const std::vector<Planet*>& planets, NetworkManager* networkManager);

    void drawAllPanels(sf::RenderWindow& window, GameState currentState,
        NetworkManager* networkManager) const;

    // Individual panel updates (if needed for specific updates)
    void updateVehicleInfoPanel(GameState currentState, VehicleManager* vehicleManager,
        SplitScreenManager* splitScreenManager, Player* localPlayer);
    void updatePlanetInfoPanel(GameState currentState, VehicleManager* vehicleManager,
        SplitScreenManager* splitScreenManager, Player* localPlayer,
        const std::vector<Planet*>& planets);
    void updateOrbitInfoPanel(GameState currentState, VehicleManager* vehicleManager,
        SplitScreenManager* splitScreenManager, NetworkManager* networkManager,
        Player* localPlayer);
    void updateNetworkInfoPanel(NetworkManager* networkManager, Player* localPlayer);

    // Panel management
    void repositionPanels(sf::Vector2u newWindowSize);
    void setFontLoaded(bool loaded) { fontLoaded = loaded; }
    bool isFontLoaded() const { return fontLoaded; }

    // Visibility control
    void showAllPanels();
    void hideAllPanels();
    void showPanel(const std::string& panelName);
    void hidePanel(const std::string& panelName);
};

#endif // GAMEINFODISPLAY_H