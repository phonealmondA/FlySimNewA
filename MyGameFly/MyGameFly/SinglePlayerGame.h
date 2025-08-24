// SinglePlayerGame.h - Single Player Game Logic
#pragma once
#include <SFML/Graphics.hpp>
#include "Planet.h"
#include "VehicleManager.h"
#include "GravitySimulator.h"
#include "UIManager.h"
#include "SatelliteManager.h"
#include "GameSaveData.h"
#include "GameConstants.h"
#include <memory>
#include <vector>

// Game result enumeration for return to main menu
enum class SinglePlayerResult {
    CONTINUE_PLAYING,
    RETURN_TO_MENU,
    QUIT_GAME
};

class SinglePlayerGame {
private:
    // Core game objects
    std::unique_ptr<Planet> planet;
    std::vector<std::unique_ptr<Planet>> orbitingPlanets;
    std::vector<Planet*> planets;
    std::unique_ptr<VehicleManager> vehicleManager;
    std::unique_ptr<GravitySimulator> gravitySimulator;
    std::unique_ptr<SatelliteManager> satelliteManager;
    std::unique_ptr<UIManager> uiManager;

    // Camera and visual settings
    sf::View gameView;
    sf::View uiView;
    float zoomLevel;
    float targetZoom;

    // Window reference and size
    sf::RenderWindow& window;
    sf::Vector2u windowSize;

    // Input tracking
    bool lKeyPressed;
    bool tKeyPressed;
    bool fuelIncreaseKeyPressed;
    bool fuelDecreaseKeyPressed;
    bool escKeyPressed;


    void handleMouseWheelZoom(const sf::Event& event);
    void updateCameraCenter();

    // Game state
    float gameTime;
    bool isInitialized;
    SinglePlayerResult currentResult;

    // Save/Load functionality
    GameSaveManager saveManager;
    std::string currentSaveName;

    // Helper methods
    void initializeFromDefaults();
    void initializeFromSaveData(const GameSaveData& saveData);
    void createDefaultPlanets();
    void createPlanetsFromSaveData(const std::vector<SavedPlanetData>& planetData);
    void setupVehicleManager(sf::Vector2f spawnPosition);
    void setupVehicleFromSaveData(const std::vector<PlayerState>& players);
    void setupCamera();
    void setupCameraFromSaveData(const SavedCameraData& cameraData);
    void updateInput(float deltaTime);
    void updateGameObjects(float deltaTime);
    void updateCamera();
    void renderGameObjects();
    void renderUI();
    void handleEscapeKey();
    bool performAutoSave();

public:
    // Constructors
    SinglePlayerGame(sf::RenderWindow& win, sf::Vector2u winSize);
    ~SinglePlayerGame() = default;

    // Initialization methods
    bool initializeNewGame();
    bool initializeFromLoad(const GameSaveData& saveData, const std::string& saveName = "");

    // Main game loop methods
    SinglePlayerResult handleEvents();
    void update(float deltaTime);
    void render();

    // Save system interface
    GameSaveData getCurrentSaveData() const;
    bool saveGame(const std::string& saveName);
    bool quickSave();
    bool autoSave();

    // State queries
    bool isGameInitialized() const { return isInitialized; }
    SinglePlayerResult getResult() const { return currentResult; }
    const std::string& getCurrentSaveName() const { return currentSaveName; }

    // Window management
    void handleWindowResize(sf::Vector2u newSize);
};