// MultiplayerClient.cpp - Multiplayer Client Game Logic Implementation
#include "MultiplayerClient.h"
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

MultiplayerClient::MultiplayerClient(sf::RenderWindow& win, sf::Vector2u winSize)
    : window(win), windowSize(winSize),
    gameView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
    uiView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
    zoomLevel(1.0f), targetZoom(1.0f),
    lKeyPressed(false), tKeyPressed(false),
    fuelIncreaseKeyPressed(false), fuelDecreaseKeyPressed(false), escKeyPressed(false),
    gameTime(0.0f), isInitialized(false), isConnected(false), waitingForPlayerID(true),
    currentResult(MultiplayerClientResult::CONTINUE_PLAYING), isLanMode(true),
    currentSaveName(""), timeSinceLastSync(0.0f), connectionTimeout(0.0f)
{
    // Initialize UI Manager
    uiManager = std::make_unique<UIManager>(windowSize);

    // Initialize Satellite Manager
    satelliteManager = std::make_unique<SatelliteManager>();

    // Initialize Network Manager
    networkManager = std::make_unique<NetworkManager>();
}

bool MultiplayerClient::initializeNewLanGame() {
    try {
        initializeFromDefaults(true);
        if (!connectToHost(true)) {
            std::cerr << "Failed to connect to LAN host" << std::endl;
            return false;
        }
        isInitialized = true;
        currentSaveName = "";
        std::cout << "LAN Multiplayer Client initialized successfully!" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize LAN client: " << e.what() << std::endl;
        return false;
    }
}

bool MultiplayerClient::initializeNewOnlineGame() {
    try {
        initializeFromDefaults(false);
        if (!connectToHost(false)) {
            std::cerr << "Failed to connect to Online host" << std::endl;
            return false;
        }
        isInitialized = true;
        currentSaveName = "";
        std::cout << "Online Multiplayer Client initialized successfully!" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize Online client: " << e.what() << std::endl;
        return false;
    }
}

bool MultiplayerClient::initializeFromLoadLan(const GameSaveData& saveData, const std::string& saveName) {
    try {
        initializeFromSaveData(saveData, true);
        if (!connectToHost(true)) {
            std::cerr << "Failed to connect to LAN host from save" << std::endl;
            return false;
        }
        isInitialized = true;
        currentSaveName = saveName;
        std::cout << "LAN Multiplayer Client loaded from save: " << saveName << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load LAN client from save: " << e.what() << std::endl;
        return false;
    }
}

bool MultiplayerClient::initializeFromLoadOnline(const GameSaveData& saveData, const std::string& saveName) {
    try {
        initializeFromSaveData(saveData, false);
        if (!connectToHost(false)) {
            std::cerr << "Failed to connect to Online host from save" << std::endl;
            return false;
        }
        isInitialized = true;
        currentSaveName = saveName;
        std::cout << "Online Multiplayer Client loaded from save: " << saveName << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load Online client from save: " << e.what() << std::endl;
        return false;
    }
}

void MultiplayerClient::initializeFromDefaults(bool lanMode) {
    isLanMode = lanMode;

    // Create default planets
    createDefaultPlanets();

    // Setup networking first
    setupNetworking(lanMode);

    // Local player will be created after receiving player ID from host
    // Just setup gravity simulator with planets for now
    gravitySimulator = std::make_unique<GravitySimulator>();
    for (const auto& planetPtr : planets) {
        gravitySimulator->addPlanet(planetPtr);
    }
    gravitySimulator->addSatelliteManager(satelliteManager.get());

    // Setup camera
    setupCamera();
    gameView.setCenter(sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y));

    gameTime = 0.0f;
    waitingForPlayerID = true;
}

void MultiplayerClient::initializeFromSaveData(const GameSaveData& saveData, bool lanMode) {
    isLanMode = lanMode;

    // Create planets from save data
    createPlanetsFromSaveData(saveData.planets);

    // Setup networking first
    setupNetworking(lanMode);

    // Local player will be created after receiving player ID from host
    // Setup gravity simulator
    gravitySimulator = std::make_unique<GravitySimulator>();
    for (const auto& planetPtr : planets) {
        gravitySimulator->addPlanet(planetPtr);
    }
    gravitySimulator->addSatelliteManager(satelliteManager.get());

    // Restore satellites (when method available)
    for (const auto& satData : saveData.satellites) {
        // TODO: Add satellite restoration when method is available
        // satelliteManager->createSatelliteFromSaveData(satData);
    }

    // Setup camera from save data
    setupCameraFromSaveData(saveData.camera);

    gameTime = saveData.gameTime;
    waitingForPlayerID = true;
}

void MultiplayerClient::createDefaultPlanets() {
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

void MultiplayerClient::createPlanetsFromSaveData(const std::vector<SavedPlanetData>& planetData) {
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

void MultiplayerClient::setupNetworking(bool lanMode) {
    isLanMode = lanMode;
    // Connection will be established in connectToHost()
}

void MultiplayerClient::setupLocalPlayer(sf::Vector2f spawnPosition, int playerID) {
    // Create local player with assigned ID and spawn position
    localPlayer = std::make_unique<Player>(
        playerID,
        spawnPosition,
        PlayerType::LOCAL,
        planets,
        satelliteManager.get()
    );

    // Add to gravity simulator
    if (gravitySimulator) {
        gravitySimulator->addPlayer(localPlayer.get());
    }

    // Center camera on local player
    gameView.setCenter(spawnPosition);

    waitingForPlayerID = false;
    std::cout << "Local player created with ID: " << playerID << " at position ("
        << spawnPosition.x << ", " << spawnPosition.y << ")" << std::endl;
}

void MultiplayerClient::setupCamera() {
    zoomLevel = 1.0f;
    targetZoom = 1.0f;
    gameView.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    uiView.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    uiView.setCenter(sf::Vector2f(static_cast<float>(windowSize.x) / 2.0f, static_cast<float>(windowSize.y) / 2.0f));
}

void MultiplayerClient::setupCameraFromSaveData(const SavedCameraData& cameraData) {
    zoomLevel = cameraData.zoom;
    targetZoom = cameraData.zoom;
    gameView.setCenter(cameraData.position);
    gameView.setSize(cameraData.size);
    uiView.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    uiView.setCenter(sf::Vector2f(static_cast<float>(windowSize.x) / 2.0f, static_cast<float>(windowSize.y) / 2.0f));
}

MultiplayerClientResult MultiplayerClient::handleEvents() {
    while (std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            handleEscapeKey(); // Auto-save before quit
            currentResult = MultiplayerClientResult::QUIT_GAME;  // FIX: Set member variable
            return MultiplayerClientResult::QUIT_GAME;
        }

        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            handleWindowResize({ resized->size.x, resized->size.y });
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                if (!escKeyPressed) {
                    escKeyPressed = true;
                    handleEscapeKey();
                    currentResult = MultiplayerClientResult::RETURN_TO_MENU;  // FIX: Set member variable
                    return MultiplayerClientResult::RETURN_TO_MENU;
                }
            }
        }

        if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
            if (keyReleased->scancode == sf::Keyboard::Scancode::Escape) {
                escKeyPressed = false;
            }
        }
    }

    return MultiplayerClientResult::CONTINUE_PLAYING;
}
void MultiplayerClient::update(float deltaTime) {
    if (!isInitialized) return;

    gameTime += deltaTime;
    timeSinceLastSync += deltaTime;
    connectionTimeout += deltaTime;

    updateNetworking(deltaTime);

    // Only update input and game objects if we have a local player
    if (!waitingForPlayerID && localPlayer) {
        updateInput(deltaTime);
        updateGameObjects(deltaTime);
        updateCamera();
    }

    // Check for connection timeout
    if (connectionTimeout > CONNECTION_TIMEOUT && !networkManager->isConnected()) {
        handleConnectionLoss();
    }
}

void MultiplayerClient::updateInput(float deltaTime) {
    // Only handle input for local player and only if we're not waiting for player ID
    if (!localPlayer || waitingForPlayerID) return;

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

void MultiplayerClient::updateNetworking(float deltaTime) {
    if (!networkManager) return;

    // Process incoming network messages
    networkManager->update(deltaTime);

    // Check if we're still connected
    if (networkManager->isConnected()) {
        isConnected = true;
        connectionTimeout = 0.0f; // Reset timeout

        // Handle player ID assignment if waiting
        if (waitingForPlayerID) {
            handlePlayerIDAssignment();
        }

        // Handle remote player states
        handleRemotePlayerStates();

        // Sync local player state at regular intervals
        if (timeSinceLastSync >= SYNC_INTERVAL && localPlayer) {
            syncLocalPlayerState();
            timeSinceLastSync = 0.0f;
        }
    }
    else {
        if (isConnected) {
            // Just lost connection
            handleConnectionLoss();
        }
    }
}

void MultiplayerClient::updateGameObjects(float deltaTime) {
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

    // Update satellite system
    if (satelliteManager) {
        satelliteManager->update(deltaTime);

        // Update satellite manager with current rocket
        if (localPlayer && localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
            satelliteManager->setNearbyRockets({ localPlayer->getVehicleManager()->getRocket() });
        }
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

void MultiplayerClient::updateCamera() {
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

void MultiplayerClient::handlePlayerIDAssignment() {
    if (!networkManager) return;

    // Check if we've received our player ID assignment
    if (networkManager->hasNewPlayer()) {
        PlayerSpawnInfo spawnInfo = networkManager->getNewPlayerInfo();

        // If this is our player ID assignment
        if (networkManager->getLocalPlayerID() == spawnInfo.playerID) {
            setupLocalPlayer(spawnInfo.spawnPosition, spawnInfo.playerID);
            std::cout << "Received player ID assignment: " << spawnInfo.playerID << std::endl;
        }
    }
}

void MultiplayerClient::handleRemotePlayerStates() {
    if (!networkManager) return;

    // Get all available remote player states
    // Since we don't have a method to get all player IDs, we'll check the common range
    for (int playerID = 0; playerID < 8; ++playerID) {
        if (localPlayer && playerID == localPlayer->getID()) {
            continue; // Skip our own player ID
        }

        if (networkManager->hasStateForPlayer(playerID)) {
            PlayerState remoteState;
            if (networkManager->receivePlayerState(playerID, remoteState)) {
                remotePlayerStates[playerID] = remoteState;
            }
        }
    }
}

void MultiplayerClient::syncLocalPlayerState() {
    if (!networkManager || !localPlayer) return;

    // Send local player state to host
    if (localPlayer->shouldSendState()) {
        PlayerState localState = localPlayer->getState();
        networkManager->sendPlayerState(localState);
        localPlayer->markStateSent();
    }
}

void MultiplayerClient::render() {
    if (!isInitialized) return;

    window.clear(sf::Color::Black);

    renderGameObjects();
    renderUI();

    window.display();
}

void MultiplayerClient::renderGameObjects() {
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

    // Draw remote player states
    renderRemotePlayerStates();

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

void MultiplayerClient::renderRemotePlayerStates() {
    // Draw simple representations of remote players based on their states
    for (const auto& [playerID, state] : remotePlayerStates) {
        if (state.isRocket) {
            // Draw a simple rocket representation
            sf::CircleShape remoteRocket(10.0f / zoomLevel);
            remoteRocket.setFillColor(sf::Color::Yellow);
            remoteRocket.setPosition(state.position);
            remoteRocket.setOrigin(sf::Vector2f(remoteRocket.getRadius(), remoteRocket.getRadius()));
            window.draw(remoteRocket);

            // Draw player ID label
            // TODO: Add text rendering for player ID when font is available
        }
        else {
            // Draw a simple car representation
            sf::RectangleShape remoteCar(sf::Vector2f(20.0f / zoomLevel, 10.0f / zoomLevel));
            remoteCar.setFillColor(sf::Color::Green);
            remoteCar.setPosition(state.position);
            remoteCar.setOrigin(sf::Vector2f(remoteCar.getSize().x / 2, remoteCar.getSize().y / 2));
            window.draw(remoteCar);
        }
    }
}

void MultiplayerClient::renderUI() {
    // Set UI view
    window.setView(uiView);

    if (uiManager) {
        GameState currentGameState = isLanMode ? GameState::LAN_MULTIPLAYER : GameState::ONLINE_MULTIPLAYER;
        uiManager->draw(window, currentGameState, networkManager.get(), satelliteManager.get());
    }

    // Draw connection status if waiting for player ID
    if (waitingForPlayerID) {
        // TODO: Add "Waiting for server..." text when font is available
    }
}

bool MultiplayerClient::connectToHost(bool lanMode, const std::string& hostAddress, int port) {
    if (!networkManager) {
        std::cerr << "NetworkManager not initialized!" << std::endl;
        return false;
    }

    isLanMode = lanMode;

    // NetworkManager uses attemptAutoConnect() as the public interface
    bool success = networkManager->attemptAutoConnect();

    if (success) {
        isConnected = true;
        connectionTimeout = 0.0f;
        std::cout << (lanMode ? "LAN" : "Online") << " client connected successfully" << std::endl;
    }
    else {
        std::cerr << "Failed to connect to " << (lanMode ? "LAN" : "Online") << " host" << std::endl;
    }

    return success;
}

void MultiplayerClient::disconnect() {
    if (networkManager && isConnected) {
        networkManager->disconnect();
        isConnected = false;
        waitingForPlayerID = true;
        remotePlayerStates.clear();
        std::cout << "Disconnected from host" << std::endl;
    }
}

bool MultiplayerClient::isConnectedToHost() const {
    return isConnected && networkManager && networkManager->isConnected();
}

int MultiplayerClient::getAssignedPlayerID() const {
    if (!networkManager || waitingForPlayerID) return -1;
    return networkManager->getLocalPlayerID();
}

void MultiplayerClient::handleEscapeKey() {
    std::cout << "ESC pressed - performing auto-save..." << std::endl;
    performAutoSave();
}

void MultiplayerClient::handleConnectionLoss() {
    std::cout << "Connection to host lost!" << std::endl;
    isConnected = false;
    currentResult = MultiplayerClientResult::DISCONNECTED;
}

bool MultiplayerClient::performAutoSave() {
    return autoSave();
}

GameSaveData MultiplayerClient::getCurrentSaveData() const {
    GameSaveData saveData;

    // Basic metadata
    saveData.saveName = currentSaveName.empty() ? "MultiplayerClient" : currentSaveName;
    saveData.playerName = "Client"; // Could be made configurable
    saveData.gameMode = isLanMode ? SavedGameMode::LAN_MULTIPLAYER : SavedGameMode::ONLINE_MULTIPLAYER;
    saveData.gameTime = gameTime;
    saveData.currentPlayerCount = static_cast<int>(remotePlayerStates.size()) + (localPlayer ? 1 : 0);

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

    // Save local player state
    if (localPlayer) {
        saveData.players.push_back(localPlayer->getState());
    }

    // Save remote player states
    for (const auto& [playerID, state] : remotePlayerStates) {
        saveData.players.push_back(state);
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

bool MultiplayerClient::quickSave() {
    GameSaveData saveData = getCurrentSaveData();
    return saveManager.quickSave(saveData);
}

bool MultiplayerClient::autoSave() {
    GameSaveData saveData = getCurrentSaveData();
    return saveManager.autoSave(saveData);
}

void MultiplayerClient::handleWindowResize(sf::Vector2u newSize) {
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