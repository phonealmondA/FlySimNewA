// MultiplayerHost.cpp - Multiplayer Host Game Logic Implementation
#include "MultiplayerHost.h"
#include "VectorHelper.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

// Suppress deprecation warnings for localtime on Windows
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

MultiplayerHost::MultiplayerHost(sf::RenderWindow& win, sf::Vector2u winSize, int maxPlayerCount)
    : window(win), windowSize(winSize), maxPlayers(maxPlayerCount),
    gameView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
    uiView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
    zoomLevel(1.0f), targetZoom(1.0f),
    lKeyPressed(false), tKeyPressed(false),
    fuelIncreaseKeyPressed(false), fuelDecreaseKeyPressed(false), escKeyPressed(false),
    gameTime(0.0f), isInitialized(false), isHosting(false),
    currentResult(MultiplayerHostResult::CONTINUE_PLAYING), isLanMode(true),
    currentSaveName(""), timeSinceLastSync(0.0f)
{
    // Initialize UI Manager
    uiManager = std::make_unique<UIManager>(windowSize);

    // Initialize Satellite Manager
    satelliteManager = std::make_unique<SatelliteManager>();

    // Initialize Network Manager
    networkManager = std::make_unique<NetworkManager>();
}

bool MultiplayerHost::initializeNewLanGame() {
    try {
        initializeFromDefaults(true);
        if (!startHosting(true)) {
            std::cerr << "Failed to start LAN hosting" << std::endl;
            return false;
        }
        isInitialized = true;
        currentSaveName = "";
        std::cout << "LAN Multiplayer Host initialized successfully!" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize LAN game: " << e.what() << std::endl;
        return false;
    }
}

bool MultiplayerHost::initializeNewOnlineGame() {
    try {
        initializeFromDefaults(false);
        if (!startHosting(false)) {
            std::cerr << "Failed to start Online hosting" << std::endl;
            return false;
        }
        isInitialized = true;
        currentSaveName = "";
        std::cout << "Online Multiplayer Host initialized successfully!" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize Online game: " << e.what() << std::endl;
        return false;
    }
}

bool MultiplayerHost::initializeFromLoadLan(const GameSaveData& saveData, const std::string& saveName) {
    try {
        initializeFromSaveData(saveData);
        if (!startHosting(true)) {
            std::cerr << "Failed to start LAN hosting for loaded game" << std::endl;
            return false;
        }
        isInitialized = true;
        currentSaveName = saveName;
        std::cout << "LAN Multiplayer Host loaded from save: " << saveName << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load LAN game from save: " << e.what() << std::endl;
        return false;
    }
}

bool MultiplayerHost::initializeFromLoadOnline(const GameSaveData& saveData, const std::string& saveName) {
    try {
        initializeFromSaveData(saveData);
        if (!startHosting(false)) {
            std::cerr << "Failed to start Online hosting for loaded game" << std::endl;
            return false;
        }
        isInitialized = true;
        currentSaveName = saveName;
        std::cout << "Online Multiplayer Host loaded from save: " << saveName << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load Online game from save: " << e.what() << std::endl;
        return false;
    }
}

void MultiplayerHost::initializeFromDefaults(bool lanMode) {
    isLanMode = lanMode;

    // Create default planets
    createDefaultPlanets();

    // Setup gravity simulator
    gravitySimulator = std::make_unique<GravitySimulator>();
    for (const auto& planetPtr : planets) {
        gravitySimulator->addPlanet(planetPtr);
    }
    gravitySimulator->addSatelliteManager(satelliteManager.get());

    // Set up local player (host is always player 0)
    sf::Vector2f spawnPos = sf::Vector2f(
        GameConstants::MAIN_PLANET_X + 150.f,
        GameConstants::MAIN_PLANET_Y - 150.f);
    setupLocalPlayer(spawnPos);

    // Initialize views
    gameView = sf::View(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f));
    uiView = sf::View(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f));
}

void MultiplayerHost::initializeFromSaveData(const GameSaveData& saveData) {
    // Load planets from save data
    createPlanetsFromSaveData(saveData.planets);

    // Setup gravity simulator
    gravitySimulator = std::make_unique<GravitySimulator>();
    for (const auto& planetPtr : planets) {
        gravitySimulator->addPlanet(planetPtr);
    }
    gravitySimulator->addSatelliteManager(satelliteManager.get());

    // Set up local player
    sf::Vector2f spawnPos = sf::Vector2f(
        GameConstants::MAIN_PLANET_X + 150.f,
        GameConstants::MAIN_PLANET_Y - 150.f);
    setupLocalPlayer(spawnPos);

    gameTime = saveData.gameTime;
}

void MultiplayerHost::createDefaultPlanets() {
    // Create central planet
    planet = std::make_unique<Planet>(
        sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
        GameConstants::MAIN_PLANET_RADIUS,
        GameConstants::MAIN_PLANET_MASS,
        sf::Color::Blue);
    planets.push_back(planet.get());

    // Create some orbiting planets
    orbitingPlanets.push_back(std::make_unique<Planet>(
        sf::Vector2f(GameConstants::MAIN_PLANET_X + 200.f, GameConstants::MAIN_PLANET_Y),
        15.0f, 50.0f, sf::Color::Red));
    orbitingPlanets.push_back(std::make_unique<Planet>(
        sf::Vector2f(GameConstants::MAIN_PLANET_X - 300.f, GameConstants::MAIN_PLANET_Y + 100.f),
        20.0f, 75.0f, sf::Color::Green));

    for (auto& orbitingPlanet : orbitingPlanets) {
        planets.push_back(orbitingPlanet.get());
    }
}

void MultiplayerHost::createPlanetsFromSaveData(const std::vector<SavedPlanetData>& planetData) {
    planets.clear();
    orbitingPlanets.clear();

    for (const auto& savedPlanet : planetData) {
        if (savedPlanet.isMainPlanet) {
            // Create main planet
            planet = std::make_unique<Planet>(
                savedPlanet.position,
                savedPlanet.radius,
                savedPlanet.mass,
                savedPlanet.color);
            planets.push_back(planet.get());
        }
        else {
            // Create orbiting planet
            orbitingPlanets.push_back(std::make_unique<Planet>(
                savedPlanet.position,
                savedPlanet.radius,
                savedPlanet.mass,
                savedPlanet.color));
            planets.push_back(orbitingPlanets.back().get());
        }
    }
}

void MultiplayerHost::setupLocalPlayer(sf::Vector2f spawnPos) {
    localPlayer = std::make_unique<Player>(
        0, // Host is always player 0
        spawnPos,
        PlayerType::LOCAL,
        planets,
        satelliteManager.get());

    // Add local player to gravity simulator
    if (gravitySimulator) {
        gravitySimulator->addPlayer(localPlayer.get());
    }

    std::cout << "Host player created at position (" << spawnPos.x << ", " << spawnPos.y << ")" << std::endl;
}

bool MultiplayerHost::startHosting(bool lanMode, int port) {
    if (!networkManager) {
        std::cerr << "NetworkManager not initialized" << std::endl;
        return false;
    }

    isLanMode = lanMode;

    // Use the correct NetworkManager method - no parameters
    if (networkManager->attemptAutoConnect()) {
        isHosting = true;
        std::cout << "Started hosting " << (lanMode ? "LAN" : "Online")
            << " game" << std::endl;
        return true;
    }

    return false;
}

void MultiplayerHost::stopHosting() {
    if (networkManager && isHosting) {
        networkManager->disconnect();
        isHosting = false;
        remotePlayers.clear();
        std::cout << "Stopped hosting game" << std::endl;
    }
}

int MultiplayerHost::getConnectedPlayerCount() const {
    return static_cast<int>(remotePlayers.size()); // Don't count host in connected players
}

bool MultiplayerHost::isPlayerConnected(int playerID) const {
    if (playerID == 0) return true; // Host is always connected

    auto it = std::find_if(remotePlayers.begin(), remotePlayers.end(),
        [playerID](const std::unique_ptr<Player>& player) {
            return player && player->getID() == playerID;
        });
    return it != remotePlayers.end();
}

void MultiplayerHost::kickPlayer(int playerID) {
    if (playerID == 0) return; // Can't kick the host

    removeRemotePlayer(playerID);
    if (networkManager) {
        networkManager->sendPlayerDisconnect(playerID);
    }
}

void MultiplayerHost::addRemotePlayer(int playerID) {
    // Generate spawn position
    std::vector<sf::Vector2f> spawnPositions = networkManager->generateSpawnPositions(
        sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
        GameConstants::MAIN_PLANET_RADIUS + 50.0f,
        maxPlayers);

    sf::Vector2f spawnPos = spawnPositions[playerID % spawnPositions.size()];

    auto newPlayer = std::make_unique<Player>(
        playerID,
        spawnPos,
        PlayerType::REMOTE,
        planets,
        satelliteManager.get());

    // Add to gravity simulator
    if (gravitySimulator) {
        gravitySimulator->addPlayer(newPlayer.get());
    }

    remotePlayers.push_back(std::move(newPlayer));

    // Send spawn info to all clients
    if (networkManager) {
        PlayerSpawnInfo spawnInfo;
        spawnInfo.playerID = playerID;
        spawnInfo.spawnPosition = spawnPos;
        spawnInfo.isHost = false;
        networkManager->sendPlayerSpawn(spawnInfo);
    }

    std::cout << "Added remote player " << playerID << " at position ("
        << spawnPos.x << ", " << spawnPos.y << ")" << std::endl;
}

void MultiplayerHost::removeRemotePlayer(int playerID) {
    auto it = std::remove_if(remotePlayers.begin(), remotePlayers.end(),
        [playerID](const std::unique_ptr<Player>& player) {
            return player && player->getID() == playerID;
        });

    if (it != remotePlayers.end()) {
        // Remove from gravity simulator before deleting
        if (gravitySimulator) {
            gravitySimulator->removePlayer(playerID);
        }
        remotePlayers.erase(it, remotePlayers.end());
        std::cout << "Removed remote player " << playerID << std::endl;
    }
}

Player* MultiplayerHost::getPlayer(int playerID) const {
    if (playerID == 0) return localPlayer.get();

    auto it = std::find_if(remotePlayers.begin(), remotePlayers.end(),
        [playerID](const std::unique_ptr<Player>& player) {
            return player && player->getID() == playerID;
        });

    return (it != remotePlayers.end()) ? it->get() : nullptr;
}

MultiplayerHostResult MultiplayerHost::handleEvents() {
    while (std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            handleEscapeKey();
            currentResult = MultiplayerHostResult::QUIT_GAME;
            return MultiplayerHostResult::QUIT_GAME;
        }

        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            handleWindowResize({ resized->size.x, resized->size.y });
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                if (!escKeyPressed) {
                    escKeyPressed = true;
                    handleEscapeKey();
                    currentResult = MultiplayerHostResult::RETURN_TO_MENU;
                    return MultiplayerHostResult::RETURN_TO_MENU;
                }
            }
        }

        if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
            if (keyReleased->scancode == sf::Keyboard::Scancode::Escape) {
                escKeyPressed = false;
            }
        }
    }

    return MultiplayerHostResult::CONTINUE_PLAYING;
}

void MultiplayerHost::update(float deltaTime) {
    if (!isInitialized) return;

    gameTime += deltaTime;
    timeSinceLastSync += deltaTime;

    updateNetworking(deltaTime);
    updateInput(deltaTime);
    updateGameObjects(deltaTime);
    updateCamera();
}

void MultiplayerHost::updateInput(float deltaTime) {
    if (!localPlayer) return;

    // Local player input handling
    localPlayer->handleLocalInput(deltaTime);

    // Camera controls
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Equal)) {
        targetZoom = std::min(targetZoom * 1.1f, 5.0f);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Hyphen)) {
        targetZoom = std::max(targetZoom * 0.9f, 0.2f);
    }

    // Toggle info display
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L)) {
        if (!lKeyPressed) {
            lKeyPressed = true;
            // Toggle UI display
        }
    }
    else {
        lKeyPressed = false;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T)) {
        if (!tKeyPressed) {
            tKeyPressed = true;
            // Toggle trajectory display
        }
    }
    else {
        tKeyPressed = false;
    }
}

void MultiplayerHost::updateGameObjects(float deltaTime) {
    // Update local player
    if (localPlayer) {
        localPlayer->update(deltaTime);
    }

    // Update remote players
    for (auto& remotePlayer : remotePlayers) {
        if (remotePlayer) {
            remotePlayer->update(deltaTime);
        }
    }

    // Update satellites
    if (satelliteManager) {
        satelliteManager->update(deltaTime);
    }

    // Update gravity simulation for all players - FIXED: Use update() method
    if (gravitySimulator) {
        gravitySimulator->update(deltaTime);
    }
}

void MultiplayerHost::updateNetworking(float deltaTime) {
    if (!networkManager || !isHosting) return;

    handlePlayerConnections();
    handlePlayerDisconnections();

    // Sync game state periodically
    if (timeSinceLastSync >= SYNC_INTERVAL) {
        syncGameState();
        timeSinceLastSync = 0.0f;
    }
}

void MultiplayerHost::handlePlayerConnections() {
    if (!networkManager) return;

    // Check for new player connections
    if (networkManager->hasNewPlayer()) {
        PlayerSpawnInfo newPlayerInfo = networkManager->getNewPlayerInfo();
        addRemotePlayer(newPlayerInfo.playerID);
        std::cout << "Player " << newPlayerInfo.playerID << " connected to host" << std::endl;
    }
}

void MultiplayerHost::handlePlayerDisconnections() {
    if (!networkManager) return;

    // For now, handle disconnections through network status checking
    // TODO: Implement proper disconnection detection when available in NetworkManager
    if (!networkManager->isConnected()) {
        // Clear all remote players if connection lost
        for (auto& remotePlayer : remotePlayers) {
            if (remotePlayer) {
                std::cout << "Player " << remotePlayer->getID() << " disconnected from host" << std::endl;
            }
        }
        remotePlayers.clear();
    }
}

void MultiplayerHost::syncGameState() {
    if (!networkManager || !localPlayer) return;

    // Send local player state to all clients
    if (localPlayer->shouldSendState()) {
        PlayerState localState = localPlayer->getState();
        networkManager->sendPlayerState(localState);
        localPlayer->markStateSent();
    }

    // Broadcast any satellite state changes
    // TODO: Add satellite state broadcasting when methods are available
}

void MultiplayerHost::updateCamera() {
    // Smooth zoom
    zoomLevel += (targetZoom - zoomLevel) * 3.0f * (1.0f / 60.0f);

    // Follow local player if not manually controlling camera
    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C) && localPlayer) {
        sf::Vector2f playerPos = localPlayer->getPosition();
        gameView.setCenter(playerPos);
    }

    // Apply zoom
    sf::Vector2f baseSize(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
    gameView.setSize(baseSize / zoomLevel);
}

void MultiplayerHost::render() {
    if (!isInitialized) return;

    window.clear(sf::Color::Black);

    renderGameObjects();
    renderUI();

    window.display();
}

void MultiplayerHost::renderGameObjects() {
    // Set game view
    window.setView(gameView);

    // Draw planets
    for (const auto& planetPtr : planets) {
        planetPtr->draw(window);
    }

    // Draw local player
    if (localPlayer) {
        localPlayer->draw(window);
    }

    // Draw remote players
    for (const auto& remotePlayer : remotePlayers) {
        if (remotePlayer) {
            remotePlayer->draw(window);
        }
    }

    // Draw satellites
    if (satelliteManager) {
        satelliteManager->draw(window);
    }
}

void MultiplayerHost::renderUI() {
    window.setView(uiView);

    if (uiManager) {
        GameState currentGameState = isLanMode ? GameState::LAN_MULTIPLAYER : GameState::ONLINE_MULTIPLAYER;
        uiManager->update(currentGameState,
            localPlayer ? localPlayer->getVehicleManager() : nullptr,
            nullptr, // splitScreenManager not used in network multiplayer
            localPlayer.get(),
            planets,
            networkManager.get(),
            satelliteManager.get());

        uiManager->draw(window, currentGameState, networkManager.get(), satelliteManager.get());
    }
}

void MultiplayerHost::handleEscapeKey() {
    // Auto-save before quitting
    performAutoSave();

    // Stop hosting
    stopHosting();
}

bool MultiplayerHost::performAutoSave() {
    if (currentSaveName.empty()) {
        // Generate auto-save name with timestamp
        auto now = std::time(nullptr);
        auto* localTime = std::localtime(&now);
        std::ostringstream oss;
        oss << "autosave_multiplayer_" << std::put_time(localTime, "%Y%m%d_%H%M%S");
        currentSaveName = oss.str();
    }

    return autoSave();
}

GameSaveData MultiplayerHost::getCurrentSaveData() const {
    GameSaveData saveData;
    saveData.gameTime = gameTime;

    // Save planet data - FIXED: Use planets vector
    for (const auto& planetPtr : planets) {
        SavedPlanetData planetData;
        planetData.position = planetPtr->getPosition();
        planetData.radius = planetPtr->getRadius();
        planetData.mass = planetPtr->getMass();
        planetData.color = sf::Color::Blue; // Default color since getColor() doesn't exist
        planetData.isMainPlanet = (planetPtr == planet.get());
        saveData.planets.push_back(planetData);
    }

    // Save player data (host and remote players) - FIXED: Use PlayerState directly
    if (localPlayer) {
        saveData.players.push_back(localPlayer->getState());
    }

    for (const auto& remotePlayer : remotePlayers) {
        if (remotePlayer) {
            saveData.players.push_back(remotePlayer->getState());
        }
    }

    return saveData;
}

bool MultiplayerHost::saveGame(const std::string& saveName) {
    try {
        GameSaveData saveData = getCurrentSaveData();
        GameSaveManager saveManager;

        if (saveManager.saveGame(saveData, saveName)) {
            currentSaveName = saveName;
            std::cout << "Game saved successfully as: " << saveName << std::endl;
            return true;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to save game: " << e.what() << std::endl;
    }

    return false;
}

bool MultiplayerHost::quickSave() {
    std::string quickSaveName = "quicksave_multiplayer";
    if (!currentSaveName.empty()) {
        quickSaveName = currentSaveName + "_quick";
    }

    return saveGame(quickSaveName);
}

bool MultiplayerHost::autoSave() {
    if (currentSaveName.empty()) {
        return performAutoSave();
    }

    return saveGame(currentSaveName + "_auto");
}

void MultiplayerHost::handleWindowResize(sf::Vector2u newSize) {
    windowSize = newSize;

    // Update views
    gameView.setSize(sf::Vector2f(static_cast<float>(newSize.x), static_cast<float>(newSize.y)));
    uiView.setSize(sf::Vector2f(static_cast<float>(newSize.x), static_cast<float>(newSize.y)));

    // Update UI manager
    if (uiManager) {
        uiManager->handleWindowResize(newSize);
    }

    // Center the views
    gameView.setCenter(sf::Vector2f(static_cast<float>(newSize.x) / 2.0f, static_cast<float>(newSize.y) / 2.0f));
    uiView.setCenter(sf::Vector2f(static_cast<float>(newSize.x) / 2.0f, static_cast<float>(newSize.y) / 2.0f));
}

#ifdef _WIN32
#pragma warning(pop)
#endif