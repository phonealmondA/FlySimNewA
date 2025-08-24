// MultiplayerClient.h - Multiplayer Client Game Logic
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
#include <map>

// Game result enumeration for return to main menu
enum class MultiplayerClientResult {
    CONTINUE_PLAYING,
    RETURN_TO_MENU,
    QUIT_GAME,
    CONNECTION_ERROR,
    DISCONNECTED
};

class MultiplayerClient {
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
    std::map<int, PlayerState> remotePlayerStates; // Store remote player states for rendering
    bool waitingForPlayerID;

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

    // Game state tracking
    float gameTime;
    bool isInitialized;
    bool isConnected;
    MultiplayerClientResult currentResult;
    bool isLanMode;
    std::string currentSaveName;

    // Network synchronization
    float timeSinceLastSync;
    float connectionTimeout;
    static constexpr float SYNC_INTERVAL = 1.0f / 30.0f; // 30 FPS sync
    static constexpr float CONNECTION_TIMEOUT = 10.0f; // 10 seconds timeout

    // Private helper methods
    void initializeFromDefaults(bool lanMode);
    void initializeFromSaveData(const GameSaveData& saveData);
    void createDefaultPlanets();
    void createPlanetsFromSaveData(const std::vector<SavedPlanetData>& planetData);
    void setupLocalPlayer(sf::Vector2f spawnPos);
    void updateInput(float deltaTime);
    void updateGameObjects(float deltaTime);
    void updateNetworking(float deltaTime);
    void updateCamera();
    void renderGameObjects();
    void renderUI();
    void renderRemotePlayers();
    void handleEscapeKey();
    void handleConnectionLoss();
    void processNetworkMessages();
    void syncWithHost();
    bool performAutoSave();

public:
    // Constructors
    MultiplayerClient(sf::RenderWindow& win, sf::Vector2u winSize);
    ~MultiplayerClient() = default;

    // Initialization methods
    bool initializeNewLanGame();
    bool initializeNewOnlineGame();
    bool initializeFromLoadLan(const GameSaveData& saveData, const std::string& saveName = "");
    bool initializeFromLoadOnline(const GameSaveData& saveData, const std::string& saveName = "");

    // Main game loop methods
    MultiplayerClientResult handleEvents();
    void update(float deltaTime);
    void render();

    // Network management
    bool connectToHost(bool lanMode, const std::string& hostAddress = "localhost", int port = 55001);
    void disconnect();
    bool isConnectedToHost() const;
    int getAssignedPlayerID() const;

    // Save system interface (limited for client)
    GameSaveData getCurrentSaveData() const;
    bool quickSave();
    bool autoSave();

    // State queries
    bool isGameInitialized() const { return isInitialized; }
    bool isCurrentlyConnected() const { return isConnected; }
    MultiplayerClientResult getResult() const { return currentResult; }
    const std::string& getCurrentSaveName() const { return currentSaveName; }
    bool isLanGame() const { return isLanMode; }
    bool isWaitingForPlayerID() const { return waitingForPlayerID; }

    // Window management
    void handleWindowResize(sf::Vector2u newSize);

    // Player management
    Player* getLocalPlayer() const { return localPlayer.get(); }
    const std::map<int, PlayerState>& getRemotePlayerStates() const { return remotePlayerStates; }
};