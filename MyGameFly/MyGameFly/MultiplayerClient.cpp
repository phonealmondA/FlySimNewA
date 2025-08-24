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
        initializeFromSaveData(saveData);
        if (!connectToHost(true)) {
            std::cerr << "Failed to connect to LAN host for loaded game" << std::endl;
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
        initializeFromSaveData(saveData);
        if (!connectToHost(false)) {
            std::cerr << "Failed to connect to Online host for loaded game" << std::endl;
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

    // Setup gravity simulator
    gravitySimulator = std::make_unique<GravitySimulator>();
    for (const auto& planetPtr : planets) {
        gravitySimulator->addPlanet(planetPtr);
    }
    gravitySimulator->addSatelliteManager(satelliteManager.get());

    // Initialize views
    gameView = sf::View(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f));
    uiView = sf::View(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f));
}

void MultiplayerClient::initializeFromSaveData(const GameSaveData& saveData) {
    // Load planets from save data
    createPlanetsFromSaveData(saveData.planets);

    // Setup gravity simulator
    gravitySimulator = std::make_unique<GravitySimulator>();
    for (const auto& planetPtr : planets) {
        gravitySimulator->addPlanet(planetPtr);
    }
    gravitySimulator->addSatelliteManager(satelliteManager.get());

    gameTime = saveData.gameTime;
}

void MultiplayerClient::createDefaultPlanets() {
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

void MultiplayerClient::createPlanetsFromSaveData(const std::vector<SavedPlanetData>& planetData) {
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

void MultiplayerClient::setupLocalPlayer(sf::Vector2f spawnPos) {
    if (networkManager && networkManager->getLocalPlayerID() >= 0) {
        localPlayer = std::make_unique<Player>(
            networkManager->getLocalPlayerID(),
            spawnPos,
            PlayerType::LOCAL,
            planets,
            satelliteManager.get());

        waitingForPlayerID = false;
        std::cout << "Local player created with ID: " << networkManager->getLocalPlayerID() << std::endl;
    }
}

bool MultiplayerClient::connectToHost(bool lanMode, const std::string& hostAddress, int port) {
    if (!networkManager) {
        std::cerr << "NetworkManager not initialized" << std::endl;
        return false;
    }

    isLanMode = lanMode;

    if (networkManager->connectAsClient()) {
        isConnected = true;
        connectionTimeout = 0.0f;

        // Wait for player ID assignment from host
        waitingForPlayerID = true;

        std::cout << "Connected to " << (lanMode ? "LAN" : "Online")
            << " host" << std::endl;
        return true;
    }

    return false;
}

void MultiplayerClient::disconnect() {
    if (networkManager && isConnected) {
        networkManager->disconnect();
        isConnected = false;
        waitingForPlayerID = true;
        localPlayer.reset();
        remotePlayerStates.clear();
        std::cout << "Disconnected from host" << std::endl;
    }
}

bool MultiplayerClient::isConnectedToHost() const {
    return isConnected && networkManager && networkManager->isConnected();
}

int MultiplayerClient::getAssignedPlayerID() const {
    return networkManager ? networkManager->getLocalPlayerID() : -1;
}

MultiplayerClientResult MultiplayerClient::handleEvents() {
    while (std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            handleEscapeKey();
            currentResult = MultiplayerClientResult::QUIT_GAME;
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
                    currentResult = MultiplayerClientResult::RETURN_TO_MENU;
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
    if (!localPlayer || waitingForPlayerID) return;

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

void MultiplayerClient::updateGameObjects(float deltaTime) {
    // Update local player
    if (localPlayer) {
        localPlayer->update(deltaTime);
    }

    // Update satellites
    if (satelliteManager) {
        satelliteManager->update(deltaTime);
    }

    // Update gravity simulation for local player
    if (gravitySimulator && localPlayer) {
        gravitySimulator->update(deltaTime);
    }
}

void MultiplayerClient::updateNetworking(float deltaTime) {
    if (!networkManager || !isConnected) return;

    processNetworkMessages();

    // Send local player state periodically
    if (timeSinceLastSync >= SYNC_INTERVAL && localPlayer && !waitingForPlayerID) {
        syncWithHost();
        timeSinceLastSync = 0.0f;
    }

    // Reset connection timeout when we receive messages
    if (networkManager->isConnected()) {
        connectionTimeout = 0.0f;
    }
}

void MultiplayerClient::processNetworkMessages() {
    if (!networkManager) return;

    // Check for player ID assignment
    if (waitingForPlayerID) {
        networkManager->assignClientPlayerID();
        if (networkManager->getLocalPlayerID() >= 0) {
            // Set up local player once we get our ID
            sf::Vector2f spawnPos = sf::Vector2f(
                GameConstants::MAIN_PLANET_X + 200.f,
                GameConstants::MAIN_PLANET_Y - 150.f);
            setupLocalPlayer(spawnPos);
        }
    }

    // Receive remote player states
    for (auto& [playerID, state] : remotePlayerStates) {
        PlayerState newState;
        if (networkManager->receivePlayerState(playerID, newState)) {
            remotePlayerStates[playerID] = newState;
        }
    }
}

void MultiplayerClient::syncWithHost() {
    if (!localPlayer || !networkManager) return;

    PlayerState localState = localPlayer->getState();
    networkManager->sendPlayerState(localState);
}

void MultiplayerClient::updateCamera() {
    // Smooth zoom
    zoomLevel += (targetZoom - zoomLevel) * 3.0f * (1.0f / 60.0f);

    // Follow local player if available
    if (localPlayer) {
        sf::Vector2f playerPos = localPlayer->getPosition();
        gameView.setCenter(playerPos);
    }

    // Apply zoom
    sf::Vector2f baseSize(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
    gameView.setSize(baseSize / zoomLevel);
}

void MultiplayerClient::render() {
    if (!isInitialized) return;

    window.clear(sf::Color::Black);

    renderGameObjects();
    renderRemotePlayers();
    renderUI();

    window.display();
}

void MultiplayerClient::renderGameObjects() {
    window.setView(gameView);

    // Draw planets
    for (const auto& planetPtr : planets) {
        planetPtr->draw(window);
    }

    // Draw local player
    if (localPlayer) {
        localPlayer->draw(window);
    }

    // Draw satellites
    if (satelliteManager) {
        satelliteManager->draw(window);
    }
}

void MultiplayerClient::renderRemotePlayers() {
    // Draw remote players based on their last known states
    for (const auto& [playerID, state] : remotePlayerStates) {
        if (playerID != getAssignedPlayerID()) {
            // Draw a simple representation of remote players
            sf::CircleShape remotePlayerShape(10.0f);
            remotePlayerShape.setPosition(state.position);
            remotePlayerShape.setFillColor(sf::Color::Yellow);
            remotePlayerShape.setOrigin(sf::Vector2f(10.0f, 10.0f)); // FIXED: Use Vector2f for SFML 3.0
            window.draw(remotePlayerShape);
        }
    }
}

void MultiplayerClient::renderUI() {
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

void MultiplayerClient::handleEscapeKey() {
    // Auto-save before quitting (if possible for client)
    performAutoSave();

    // Disconnect from host
    disconnect();
}

void MultiplayerClient::handleConnectionLoss() {
    std::cerr << "Connection to host lost!" << std::endl;
    currentResult = MultiplayerClientResult::CONNECTION_ERROR;
    disconnect();
}

bool MultiplayerClient::performAutoSave() {
    // Limited auto-save capability for client - mainly for local state
    return true; // For now, always return true as client doesn't control saves
}

GameSaveData MultiplayerClient::getCurrentSaveData() const {
    GameSaveData saveData;
    saveData.gameTime = gameTime;

    // Save planet data
    for (const auto& planetPtr : planets) {
        SavedPlanetData planetData;
        planetData.position = planetPtr->getPosition();
        planetData.radius = planetPtr->getRadius();
        planetData.mass = planetPtr->getMass();
        planetData.color = sf::Color::Blue; // Default color since getColor() doesn't exist
        planetData.isMainPlanet = (planetPtr == planet.get());
        saveData.planets.push_back(planetData);
    }

    return saveData;
}

bool MultiplayerClient::quickSave() {
    // Clients typically can't perform saves in multiplayer
    return false;
}

bool MultiplayerClient::autoSave() {
    return performAutoSave();
}

void MultiplayerClient::handleWindowResize(sf::Vector2u newSize) {
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