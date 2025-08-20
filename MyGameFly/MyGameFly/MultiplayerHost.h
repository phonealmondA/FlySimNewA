// MultiplayerHost.h - Multiplayer Host Game Logic
#pragma once
#include <SFML/Graphics.hpp>
#include "Planet.h"
#include "Player.h"
#include "GravitySimulator.h"
#include "UIManager.h"
#include "SatelliteManager.h"
#include "NetworkManager.h"
#include "GameSaveData.h"
#include "GameConstants.h"
#include <memory>
#include <vector>

// Game result enumeration for return to main menu
enum class MultiplayerHostResult {
    CONTINUE_PLAYING,
    RETURN_TO_MENU,
    QUIT_GAME,
    CONNECTION_ERROR
};

class MultiplayerHost {
private:
    // Core game objects
    std::unique_ptr<Planet> planet;
    std::vector<std::unique_ptr<Planet>> orbitingPlanets;
    std::vector<Planet*> planets;
    std::unique_ptr<GravitySimulator> gravitySimulator;
    std::unique_ptr<SatelliteManager> satelliteManager;
    std::unique_ptr<UIManager> uiManager;

    // Network management
    std::unique_ptr<NetworkManager> networkManager;

    // Player management
    std::unique_ptr<Player> localPlayer;
    std::vector<std::unique_ptr<Player>> remotePlayers;
    std::vector<std::unique_ptr<Player>> players; // All players for convenience
    int maxPlayers;

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

    // Game state
    float gameTime;
    bool isInitialized;
    bool isHosting;
    MultiplayerHostResult currentResult;
    bool isLanMode; // true for LAN, false for Online

    // Save/Load functionality
    GameSaveManager saveManager;
    std::string currentSaveName;

    // Network state tracking
    float timeSinceLastSync;
    static constexpr float SYNC_INTERVAL = 1.0f / 30.0f; // 30 FPS sync rate

    // Helper methods
    void initializeFromDefaults(bool lanMode);
    void initializeFromSaveData(const GameSaveData& saveData, bool lanMode);
    void createDefaultPlanets();
    void createPlanetsFromSaveData(const std::vector<SavedPlanetData>& planetData);
    void setupNetworking(bool lanMode);
    void setupLocalPlayer();
    void setupPlayersFromSaveData(const std::vector<PlayerState>& playerStates);
    void setupCamera();
    void setupCameraFromSaveData(const SavedCameraData& cameraData);
    void updateInput(float deltaTime);
    void updateGameObjects(float deltaTime);
    void updateNetworking(float deltaTime);
    void updateCamera();
    void handlePlayerConnections();
    void handlePlayerDisconnections();
    void syncGameState();
    void renderGameObjects();
    void renderUI();
    void handleEscapeKey();
    bool performAutoSave();
    sf::Vector2f generatePlayerSpawnPosition(int playerID);

public:
    // Constructors
    MultiplayerHost(sf::RenderWindow& win, sf::Vector2u winSize, int maxPlayerCount = 4);
    ~MultiplayerHost() = default;

    // Initialization methods
    bool initializeNewLanGame();
    bool initializeNewOnlineGame();
    bool initializeFromLoadLan(const GameSaveData& saveData, const std::string& saveName = "");
    bool initializeFromLoadOnline(const GameSaveData& saveData, const std::string& saveName = "");

    // Main game loop methods
    MultiplayerHostResult handleEvents();
    void update(float deltaTime);
    void render();

    // Network management
    bool startHosting(bool lanMode, int port = 55001);
    void stopHosting();
    int getConnectedPlayerCount() const;
    bool isPlayerConnected(int playerID) const;
    void kickPlayer(int playerID);

    // Save system interface
    GameSaveData getCurrentSaveData() const;
    bool saveGame(const std::string& saveName);
    bool quickSave();
    bool autoSave();

    // State queries
    bool isGameInitialized() const { return isInitialized; }
    bool isCurrentlyHosting() const { return isHosting; }
    MultiplayerHostResult getResult() const { return currentResult; }
    const std::string& getCurrentSaveName() const { return currentSaveName; }
    bool isLanGame() const { return isLanMode; }

    // Window management
    void handleWindowResize(sf::Vector2u newSize);

    // Player management
    void addRemotePlayer(int playerID);
    void removeRemotePlayer(int playerID);
    Player* getLocalPlayer() const { return localPlayer.get(); }
    const std::vector<std::unique_ptr<Player>>& getRemotePlayers() const { return remotePlayers; }
    Player* getPlayer(int playerID) const;
};