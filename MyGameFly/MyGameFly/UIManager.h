#pragma once
#ifndef UIMANAGER_H
#define UIMANAGER_H

#include "GameInfoDisplay.h"
#include <SFML/Graphics.hpp>
#include <memory>
class SatelliteManager;
// Forward declarations
class VehicleManager;
class SplitScreenManager;
class Player;
class Planet;
class Rocket;
class NetworkManager;

// Game state enum - moved here to be the single source
enum class GameState {
    MAIN_MENU,
    MULTIPLAYER_MENU,
    ONLINE_MENU,
    SINGLE_PLAYER,
    LOCAL_PC_MULTIPLAYER,
    LAN_MULTIPLAYER,
    ONLINE_MULTIPLAYER,
    QUIT
};

class UIManager {
private:
    // Core UI components
    std::unique_ptr<GameInfoDisplay> gameInfoDisplay;

    // Font management
    sf::Font font;
    bool fontLoaded;

    // Window management
    sf::Vector2u windowSize;
    sf::View uiView;

    // Visual settings
    bool showUI;
    bool showDebugInfo;

    // Helper methods
    void loadFont();
    void setupViews();

public:
    UIManager(sf::Vector2u winSize);
    ~UIManager() = default;

    // Core UI operations
    void update(GameState currentState, VehicleManager* vehicleManager,
        SplitScreenManager* splitScreenManager, Player* localPlayer,
        const std::vector<Planet*>& planets, NetworkManager* networkManager,
        SatelliteManager* satelliteManager = nullptr);



    void draw(sf::RenderWindow& window, GameState currentState, NetworkManager* networkManager,
        SatelliteManager* satelliteManager = nullptr);


    // Font and setup
    bool initializeFont();
    bool isFontLoaded() const { return fontLoaded; }
    const sf::Font& getFont() const { return font; }

    // Window management
    void handleWindowResize(sf::Vector2u newSize);
    void setUIView(sf::RenderWindow& window);

    // UI control
    void toggleUI() { showUI = !showUI; }
    void setShowUI(bool show) { showUI = show; }
    bool isUIVisible() const { return showUI; }

    void toggleDebugInfo() { showDebugInfo = !showDebugInfo; }
    void setShowDebugInfo(bool show) { showDebugInfo = show; }
    bool isDebugInfoVisible() const { return showDebugInfo; }

    // Drawing utilities (moved from main.cpp)
    void drawFuelCollectionLines(sf::RenderWindow& window, Rocket* rocket) const;
    void drawMultipleFuelLines(sf::RenderWindow& window, VehicleManager* vm1, VehicleManager* vm2 = nullptr) const;

    // Satellite system drawing utilities
    void drawSatelliteNetworkLines(sf::RenderWindow& window, SatelliteManager* satelliteManager) const;
    void drawSatelliteFuelTransfers(sf::RenderWindow& window, SatelliteManager* satelliteManager) const;
    void drawSatelliteToRocketLines(sf::RenderWindow& window, SatelliteManager* satelliteManager,
        VehicleManager* vm1, VehicleManager* vm2 = nullptr) const;

    // Utility methods for main.cpp
    sf::Vector2f getMousePosition(sf::RenderWindow& window) const;

    // Panel access (if needed for specific customization)
    GameInfoDisplay* getGameInfoDisplay() const { return gameInfoDisplay.get(); }
};

#endif // UIMANAGER_H