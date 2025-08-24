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
        initializeFromSaveData(saveData, true);
        if (!startHosting(true)) {
            std::cerr << "Failed to start LAN hosting from save" << std::endl;
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
        initializeFromSaveData(saveData, false);
        if (!startHosting(false)) {
            std::cerr << "Failed to start Online hosting from save" << std::endl;
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

    // Setup networking first
    setupNetworking(lanMode);

    // Setup local player
    setupLocalPlayer();

    // Setup gravity simulator
    gravitySimulator = std::make_unique<GravitySimulator>();
    for (const auto& planetPtr : planets) {
        gravitySimulator->addPlanet(planetPtr);
    }
    gravitySimulator->addSatelliteManager(satelliteManager.get());
    gravitySimulator->addPlayer(localPlayer.get());

    // Setup camera
    setupCamera();
    if (localPlayer) {
        gameView.setCenter(localPlayer->getPosition());
    }

    gameTime = 0.0f;
}

void MultiplayerHost::initializeFromSaveData(const GameSaveData& saveData, bool lanMode) {
    isLanMode = lanMode;

    // Create planets from save data
    createPlanetsFromSaveData(saveData.planets);

    // Setup networking first
    setupNetworking(lanMode);

    // Setup players from save data
    setupPlayersFromSaveData(saveData.players);

    // Setup gravity simulator
    gravitySimulator = std::make_unique<GravitySimulator>();
    for (const auto& planetPtr : planets) {
        gravitySimulator->addPlanet(planetPtr);
    }
    gravitySimulator->addSatelliteManager(satelliteManager.get());

    // Add all players to gravity simulator
    if (localPlayer) {
        gravitySimulator->addPlayer(localPlayer.get());
    }
    for (const auto& remotePlayer : remotePlayers) {
        gravitySimulator->addPlayer(remotePlayer.get());
    }

    // Restore satellites
    for (const auto& satData : saveData.satellites) {
        // TODO: Add satellite restoration when method is available
        // satelliteManager->createSatelliteFromSaveData(satData);
    }

    // Setup camera from save data
    setupCameraFromSaveData(saveData.camera);

    gameTime = saveData.gameTime;
}

void MultiplayerHost::createDefaultPlanets() {
    // Create main planet
    planet = std::make_unique<Planet>(
        sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
        0.0f, GameConstants::MAIN_PLANET_MASS, sf::Color(139, 0, 0));
    planet->setVelocity(sf::Vector2f(0.f, 0.f));

    planets.clear();
    planets.push_back(planet.get());

    // Get planet configurations from GameConstants
    std::vector<PlanetConfig> planetConfigs = GameConstants::getPlanetConfigurations();

    // Create orbital planets dynamically
    orbitingPlanets.clear();
    for (const auto& config : planetConfigs) {
        auto orbitalPlanet = Planet::createOrbitalPlanet(
            planet.get(),
            GameConstants::PLANET_ORBIT_DISTANCE * config.orbitDistanceMultiplier,
            config.massRatio,
            config.angleOffset,
            config.color
        );

        planets.push_back(orbitalPlanet.get());
        orbitingPlanets.push_back(std::move(orbitalPlanet));
    }
}

void MultiplayerHost::createPlanetsFromSaveData(const std::vector<SavedPlanetData>& planetData) {
    planets.clear();
    orbitingPlanets.clear();

    for (const auto& savedPlanet : planetData) {
        if (savedPlanet.isMainPlanet) {
            // Create main planet
            planet = std::make_unique<Planet>(
                savedPlanet.position, 0.0f, savedPlanet.mass, savedPlanet.color);
            planet->setVelocity(sf::Vector2f(0.f, 0.f));
            planets.push_back(planet.get());
        }
        else {
            // Create orbiting planet
            auto orbitalPlanet = std::make_unique<Planet>(
                savedPlanet.position, 0.0f, savedPlanet.mass, savedPlanet.color);
            planets.push_back(orbitalPlanet.get());
            orbitingPlanets.push_back(std::move(orbitalPlanet));
        }
    }
}

void MultiplayerHost::setupNetworking(bool lanMode) {
    isLanMode = lanMode;
    // Network setup will be done in startHosting()
}

void MultiplayerHost::setupLocalPlayer() {
    if (!networkManager) {
        std::cerr << "NetworkManager not initialized!" << std::endl;
        return;
    }

    // Generate spawn position for local player
    sf::Vector2f spawnPos = generatePlayerSpawnPosition(networkManager->getLocalPlayerID());

    // Create local player with satellite manager reference
    localPlayer = std::make_unique<Player>(
        networkManager->getLocalPlayerID(),
        spawnPos,
        PlayerType::LOCAL,
        planets,
        satelliteManager.get()
    );

    std::cout << "Local player created with ID: " << networkManager->getLocalPlayerID() << std::endl;
}

void MultiplayerHost::setupPlayersFromSaveData(const std::vector<PlayerState>& playerStates) {
    if (!networkManager) {
        std::cerr << "NetworkManager not initialized!" << std::endl;
        return;
    }

    // Clear existing remote players
    remotePlayers.clear();

    // Find and setup local player
    int localPlayerID = networkManager->getLocalPlayerID();
    bool localPlayerFound = false;

    for (const auto& playerState : playerStates) {
        if (playerState.playerID == localPlayerID) {
            // Create local player from saved state
            localPlayer = std::make_unique<Player>(
                playerState.playerID,
                playerState.position,
                PlayerType::LOCAL,
                planets,
                satelliteManager.get()
            );

            // Apply saved state
            localPlayer->applyState(playerState);
            localPlayerFound = true;
            std::cout << "Local player restored from save with ID: " << localPlayerID << std::endl;
        }
        else {
            // Create remote player placeholder (will be connected later)
            auto remotePlayer = std::make_unique<Player>(
                playerState.playerID,
                playerState.position,
                PlayerType::REMOTE,
                planets,
                satelliteManager.get()
            );

            // Apply saved state
            remotePlayer->applyState(playerState);
            remotePlayers.push_back(std::move(remotePlayer));
            std::cout << "Remote player placeholder created with ID: " << playerState.playerID << std::endl;
        }
    }

    // If no local player found in save, create default
    if (!localPlayerFound) {
        setupLocalPlayer();
    }
}

void MultiplayerHost::setupCamera() {
    zoomLevel = 1.0f;
    targetZoom = 1.0f;
    gameView.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    uiView.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    uiView.setCenter(sf::Vector2f(static_cast<float>(windowSize.x) / 2.0f, static_cast<float>(windowSize.y) / 2.0f));
}

void MultiplayerHost::setupCameraFromSaveData(const SavedCameraData& cameraData) {
    zoomLevel = cameraData.zoom;
    targetZoom = cameraData.zoom;
    gameView.setCenter(cameraData.position);
    gameView.setSize(cameraData.size);
    uiView.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    uiView.setCenter(sf::Vector2f(static_cast<float>(windowSize.x) / 2.0f, static_cast<float>(windowSize.y) / 2.0f));
}

MultiplayerHostResult MultiplayerHost::handleEvents() {
    while (std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            handleEscapeKey(); // Auto-save before quit
            currentResult = MultiplayerHostResult::QUIT_GAME;  // FIX: Set member variable
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
                    currentResult = MultiplayerHostResult::RETURN_TO_MENU;  // FIX: Set member variable
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

    updateInput(deltaTime);
    updateNetworking(deltaTime);
    updateGameObjects(deltaTime);
    updateCamera();
}

void MultiplayerHost::updateInput(float deltaTime) {
    // Only handle input for local player
    if (!localPlayer) return;

    // Local player input handling
    localPlayer->handleLocalInput(deltaTime);

    // Manual fuel transfer input
    localPlayer->handleFuelTransferInput(deltaTime);

    // Zoom controls
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)) {
        targetZoom = std::max(0.1f, targetZoom - 2.0f * deltaTime);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
        targetZoom = std::min(10.0f, targetZoom + 2.0f * deltaTime);
    }
}

void MultiplayerHost::updateNetworking(float deltaTime) {
    if (!networkManager || !isHosting) return;

    // Handle player connections and disconnections
    handlePlayerConnections();
    handlePlayerDisconnections();

    // Process incoming network messages
    networkManager->update(deltaTime);

    // Sync game state at regular intervals
    if (timeSinceLastSync >= SYNC_INTERVAL) {
        syncGameState();
        timeSinceLastSync = 0.0f;
    }

    // Apply received states to remote players
    if (localPlayer) {
        for (auto& remotePlayer : remotePlayers) {
            if (remotePlayer && networkManager->hasStateForPlayer(remotePlayer->getID())) {
                PlayerState receivedState;
                if (networkManager->receivePlayerState(remotePlayer->getID(), receivedState)) {
                    remotePlayer->applyState(receivedState);
                }
            }
        }
    }
}

void MultiplayerHost::updateGameObjects(float deltaTime) {
    // Update main planet
    if (planet) {
        planet->update(deltaTime);
    }

    // Update orbiting planets
    for (const auto& orbitingPlanet : orbitingPlanets) {
        orbitingPlanet->update(deltaTime);
    }

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

    // Update satellite system
    if (satelliteManager) {
        satelliteManager->update(deltaTime);

        // Update satellite manager with current rockets
        std::vector<Rocket*> networkRockets;
        if (localPlayer && localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
            networkRockets.push_back(localPlayer->getVehicleManager()->getRocket());
        }

        for (auto& remotePlayer : remotePlayers) {
            if (remotePlayer && remotePlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                networkRockets.push_back(remotePlayer->getVehicleManager()->getRocket());
            }
        }

        satelliteManager->setNearbyRockets(networkRockets);
    }

    // Update gravity simulator
    if (gravitySimulator) {
        gravitySimulator->update(deltaTime);
    }

    // Update UI
    if (uiManager) {
        GameState currentGameState = isLanMode ? GameState::LAN_MULTIPLAYER : GameState::ONLINE_MULTIPLAYER;
        uiManager->update(currentGameState,
            localPlayer ? localPlayer->getVehicleManager() : nullptr,
            nullptr, // splitScreenManager not used in network multiplayer
            localPlayer.get(),
            planets,
            networkManager.get(),
            satelliteManager.get());
    }
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

    // Draw main planet
    if (planet) {
        planet->draw(window);
    }

    // Draw orbiting planets
    for (const auto& orbitingPlanet : orbitingPlanets) {
        orbitingPlanet->draw(window);
    }

    // Draw local player
    if (localPlayer) {
        localPlayer->drawWithConstantSize(window, zoomLevel);
    }

    // Draw remote players
    for (const auto& remotePlayer : remotePlayers) {
        if (remotePlayer) {
            remotePlayer->drawWithConstantSize(window, zoomLevel);
        }
    }

    // Draw satellites
    if (satelliteManager) {
        satelliteManager->draw(window);
    }

    // Draw UI elements that need game view
    if (uiManager && localPlayer) {
        uiManager->drawFuelCollectionLines(window, localPlayer->getVehicleManager()->getRocket());
        uiManager->drawSatelliteNetworkLines(window, satelliteManager.get());
        uiManager->drawSatelliteFuelTransfers(window, satelliteManager.get());
        uiManager->drawSatelliteToRocketLines(window, satelliteManager.get(),
            localPlayer->getVehicleManager());
    }
}

void MultiplayerHost::renderUI() {
    // Set UI view
    window.setView(uiView);

    if (uiManager) {
        GameState currentGameState = isLanMode ? GameState::LAN_MULTIPLAYER : GameState::ONLINE_MULTIPLAYER;
        uiManager->draw(window, currentGameState, networkManager.get(), satelliteManager.get());
    }
}

bool MultiplayerHost::startHosting(bool lanMode, int port) {
    if (!networkManager) {
        std::cerr << "NetworkManager not initialized!" << std::endl;
        return false;
    }

    isLanMode = lanMode;

    // NetworkManager's startAsHost() doesn't take parameters, so we use the default
    bool success = networkManager->attemptAutoConnect();

    if (success) {
        isHosting = true;
        std::cout << (lanMode ? "LAN" : "Online") << " hosting started" << std::endl;
    }
    else {
        std::cerr << "Failed to start " << (lanMode ? "LAN" : "Online") << " hosting" << std::endl;
    }

    return success;
}

void MultiplayerHost::stopHosting() {
    if (networkManager && isHosting) {
        networkManager->disconnect();
        isHosting = false;
        std::cout << "Stopped hosting" << std::endl;
    }
}

int MultiplayerHost::getConnectedPlayerCount() const {
    if (!networkManager) return 0;
    // For now, return based on whether we have connection + remote players
    return (networkManager->isConnected() ? 1 : 0) + static_cast<int>(remotePlayers.size());
}

bool MultiplayerHost::isPlayerConnected(int playerID) const {
    if (!networkManager) return false;
    if (localPlayer && localPlayer->getID() == playerID) return true;

    for (const auto& remotePlayer : remotePlayers) {
        if (remotePlayer && remotePlayer->getID() == playerID) {
            return true;
        }
    }
    return false;
}

void MultiplayerHost::kickPlayer(int playerID) {
    if (networkManager && isHosting) {
        // TODO: Add disconnect specific player when method available
        removeRemotePlayer(playerID);
        std::cout << "Kicked player " << playerID << std::endl;
    }
}

void MultiplayerHost::addRemotePlayer(int playerID) {
    // Check if player already exists
    for (const auto& player : remotePlayers) {
        if (player && player->getID() == playerID) {
            return; // Player already exists
        }
    }

    // Generate spawn position
    sf::Vector2f spawnPos = generatePlayerSpawnPosition(playerID);

    // Create remote player
    auto remotePlayer = std::make_unique<Player>(
        playerID,
        spawnPos,
        PlayerType::REMOTE,
        planets,
        satelliteManager.get()
    );

    // Add to gravity simulator
    if (gravitySimulator) {
        gravitySimulator->addPlayer(remotePlayer.get());
    }

    remotePlayers.push_back(std::move(remotePlayer));
}

void MultiplayerHost::removeRemotePlayer(int playerID) {
    // Remove from gravity simulator first
    if (gravitySimulator) {
        gravitySimulator->removePlayer(playerID);
    }

    // Remove from remote players vector
    remotePlayers.erase(
        std::remove_if(remotePlayers.begin(), remotePlayers.end(),
            [playerID](const std::unique_ptr<Player>& player) {
                return player && player->getID() == playerID;
            }),
        remotePlayers.end()
    );
}

Player* MultiplayerHost::getPlayer(int playerID) const {
    if (localPlayer && localPlayer->getID() == playerID) {
        return localPlayer.get();
    }

    for (const auto& remotePlayer : remotePlayers) {
        if (remotePlayer && remotePlayer->getID() == playerID) {
            return remotePlayer.get();
        }
    }

    return nullptr;
}

sf::Vector2f MultiplayerHost::generatePlayerSpawnPosition(int playerID) {
    // Generate spawn positions around different planets based on player ID
    if (orbitingPlanets.empty()) {
        // Fallback to main planet if no orbiting planets
        return sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y - 150.0f);
    }

    // Use player ID to determine which planet to spawn near
    int planetIndex = (playerID - 1) % orbitingPlanets.size();
    sf::Vector2f planetPos = orbitingPlanets[planetIndex]->getPosition();
    float planetRadius = orbitingPlanets[planetIndex]->getRadius();

    // Generate position slightly above the planet
    return sf::Vector2f(planetPos.x, planetPos.y - planetRadius - 150.0f);
}

void MultiplayerHost::handleEscapeKey() {
    std::cout << "ESC pressed - performing auto-save..." << std::endl;
    performAutoSave();
}

bool MultiplayerHost::performAutoSave() {
    return autoSave();
}

GameSaveData MultiplayerHost::getCurrentSaveData() const {
    GameSaveData saveData;

    // Basic metadata
    saveData.saveName = currentSaveName.empty() ? "MultiplayerHost" : currentSaveName;
    saveData.playerName = "Host"; // Could be made configurable
    saveData.gameMode = isLanMode ? SavedGameMode::LAN_MULTIPLAYER : SavedGameMode::ONLINE_MULTIPLAYER;
    saveData.gameTime = gameTime;
    saveData.currentPlayerCount = getConnectedPlayerCount() + 1; // +1 for host

    // Save planets
    saveData.planets.clear();
    if (planet) {
        SavedPlanetData mainPlanetData;
        mainPlanetData.position = planet->getPosition();
        mainPlanetData.mass = planet->getMass();
        mainPlanetData.color = sf::Color(139, 0, 0);
        mainPlanetData.isMainPlanet = true;
        mainPlanetData.radius = planet->getRadius();
        saveData.planets.push_back(mainPlanetData);
    }

    for (const auto& orbitingPlanet : orbitingPlanets) {
        SavedPlanetData planetData;
        planetData.position = orbitingPlanet->getPosition();
        planetData.mass = orbitingPlanet->getMass();
        planetData.color = sf::Color::Green;
        planetData.isMainPlanet = false;
        planetData.radius = orbitingPlanet->getRadius();
        saveData.planets.push_back(planetData);
    }

    // Save player states
    if (localPlayer) {
        saveData.players.push_back(localPlayer->getState());
    }

    for (const auto& remotePlayer : remotePlayers) {
        if (remotePlayer) {
            saveData.players.push_back(remotePlayer->getState());
        }
    }

    // Save satellites
    if (satelliteManager) {
        // TODO: Add proper getSaveData method to SatelliteManager
        saveData.satellites.clear();
    }

    // Save camera data
    saveData.camera.position = gameView.getCenter();
    saveData.camera.size = gameView.getSize();
    saveData.camera.zoom = zoomLevel;
    saveData.camera.followingPlayerID = localPlayer ? localPlayer->getID() : -1;

    // Save UI settings (using defaults for now)
    saveData.uiSettings.showUI = true;
    saveData.uiSettings.showDebugInfo = false;
    saveData.uiSettings.showOrbitPaths = true;
    saveData.uiSettings.showFuelRings = true;
    saveData.uiSettings.showPlayerLabels = true;

    return saveData;
}

bool MultiplayerHost::saveGame(const std::string& saveName) {
    GameSaveData saveData = getCurrentSaveData();
    saveData.saveName = saveName;

    bool success = saveManager.saveGame(saveData, saveName);
    if (success) {
        currentSaveName = saveName;
        std::cout << "Multiplayer game saved successfully as: " << saveName << std::endl;
    }
    else {
        std::cerr << "Failed to save multiplayer game: " << saveName << std::endl;
    }

    return success;
}

bool MultiplayerHost::quickSave() {
    GameSaveData saveData = getCurrentSaveData();
    return saveManager.quickSave(saveData);
}

bool MultiplayerHost::autoSave() {
    GameSaveData saveData = getCurrentSaveData();
    return saveManager.autoSave(saveData);
}

void MultiplayerHost::handleWindowResize(sf::Vector2u newSize) {
    windowSize = newSize;

    // Update views
    gameView.setSize(sf::Vector2f(static_cast<float>(newSize.x), static_cast<float>(newSize.y)) / zoomLevel);
    uiView.setSize(sf::Vector2f(static_cast<float>(newSize.x), static_cast<float>(newSize.y)));
    uiView.setCenter(sf::Vector2f(static_cast<float>(newSize.x) / 2.0f, static_cast<float>(newSize.y) / 2.0f));

    // Update UI manager
    if (uiManager) {
        uiManager->handleWindowResize(newSize);
    }
}

#ifdef _WIN32
#pragma warning(pop)
#endif