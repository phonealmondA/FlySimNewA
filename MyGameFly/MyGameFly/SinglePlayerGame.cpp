// SinglePlayerGame.cpp - Single Player Game Logic Implementation
#include "SinglePlayerGame.h"
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

SinglePlayerGame::SinglePlayerGame(sf::RenderWindow& win, sf::Vector2u winSize)
    : window(win), windowSize(winSize),
    gameView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
    uiView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
    zoomLevel(1.0f), targetZoom(1.0f),
    lKeyPressed(false), tKeyPressed(false),
    fuelIncreaseKeyPressed(false), fuelDecreaseKeyPressed(false), escKeyPressed(false),
    
    gameTime(0.0f), isInitialized(false), currentResult(SinglePlayerResult::CONTINUE_PLAYING),
    currentSaveName("")
{
    // Initialize UI Manager
    uiManager = std::make_unique<UIManager>(windowSize);

    // Initialize Satellite Manager
    satelliteManager = std::make_unique<SatelliteManager>();
}

bool SinglePlayerGame::initializeNewGame() {
    try {
        initializeFromDefaults();
        isInitialized = true;
        currentSaveName = "";
        std::cout << "Single Player Game initialized successfully!" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize new game: " << e.what() << std::endl;
        return false;
    }
}

bool SinglePlayerGame::initializeFromLoad(const GameSaveData& saveData, const std::string& saveName) {
    try {
        initializeFromSaveData(saveData);
        isInitialized = true;
        currentSaveName = saveName;
        std::cout << "Single Player Game loaded from save: " << saveName << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load game from save: " << e.what() << std::endl;
        return false;
    }
}


void SinglePlayerGame::initializeFromDefaults() {
    // Create default planets
    createDefaultPlanets();

    // Setup vehicle manager at default spawn position
    sf::Vector2f spawnPos;
    if (!orbitingPlanets.empty()) {
        sf::Vector2f planetPos = orbitingPlanets[0]->getPosition();
        float planetRadius = orbitingPlanets[0]->getRadius();
        spawnPos = sf::Vector2f(planetPos.x, planetPos.y - planetRadius - 150.0f);
    }
    else {
        spawnPos = sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y - 150.0f);
    }

    setupVehicleManager(spawnPos);

    // Setup gravity simulator
    gravitySimulator = std::make_unique<GravitySimulator>();
    for (const auto& planetPtr : planets) {
        gravitySimulator->addPlanet(planetPtr);
    }

    // ADD THIS MISSING LINE - This is why gravity wasn't working!
    gravitySimulator->addVehicleManager(vehicleManager.get());

    gravitySimulator->addSatelliteManager(satelliteManager.get());

    // Setup camera
    setupCamera();
    gameView.setCenter(spawnPos);

    gameTime = 0.0f;
}

void SinglePlayerGame::initializeFromSaveData(const GameSaveData& saveData) {
    // Create planets from save data
    createPlanetsFromSaveData(saveData.planets);

    // Setup vehicle manager from save data
    setupVehicleFromSaveData(saveData.players);

    // Setup gravity simulator
    gravitySimulator = std::make_unique<GravitySimulator>();
    for (const auto& planetPtr : planets) {
        gravitySimulator->addPlanet(planetPtr);
    }

    // ADD THIS MISSING LINE - Also needed when loading saves!
    gravitySimulator->addVehicleManager(vehicleManager.get());

    gravitySimulator->addSatelliteManager(satelliteManager.get());

    // Restore satellites
    for (const auto& satData : saveData.satellites) {
        // Convert SavedSatelliteData back to satellite
        // For now, skip satellite restoration until we add the proper method
        // TODO: Add satelliteManager->createSatelliteFromSaveData(satData);
    }

    // Setup camera from save data
    setupCameraFromSaveData(saveData.camera);

    gameTime = saveData.gameTime;
}
void SinglePlayerGame::createDefaultPlanets() {
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

void SinglePlayerGame::createPlanetsFromSaveData(const std::vector<SavedPlanetData>& planetData) {
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
            // Note: Planet doesn't have setRadius method, radius is set during construction
            planets.push_back(orbitalPlanet.get());
            orbitingPlanets.push_back(std::move(orbitalPlanet));
        }
    }
}

void SinglePlayerGame::setupVehicleManager(sf::Vector2f spawnPosition) {
    vehicleManager = std::make_unique<VehicleManager>(spawnPosition, planets, satelliteManager.get());
}

void SinglePlayerGame::setupVehicleFromSaveData(const std::vector<PlayerState>& players) {
    if (!players.empty()) {
        const auto& playerState = players[0]; // Single player uses first player
        vehicleManager = std::make_unique<VehicleManager>(playerState.position, planets, satelliteManager.get());

        // Restore vehicle state
        if (playerState.isRocket && vehicleManager->getRocket()) {
            vehicleManager->getRocket()->setPosition(playerState.position);
            vehicleManager->getRocket()->setVelocity(playerState.velocity);
            vehicleManager->getRocket()->setFuel(playerState.currentFuel);
            // Note: Rocket doesn't have setRotation method, rotation is handled internally
        }
        // Add car state restoration if needed
    }
    else {
        // Fallback to default spawn
        setupVehicleManager(sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y - 150.0f));
    }
}

void SinglePlayerGame::setupCamera() {
    zoomLevel = 1.0f;
    targetZoom = 1.0f;
    gameView.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    uiView.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    uiView.setCenter(sf::Vector2f(static_cast<float>(windowSize.x) / 2.0f, static_cast<float>(windowSize.y) / 2.0f));
}

void SinglePlayerGame::setupCameraFromSaveData(const SavedCameraData& cameraData) {
    zoomLevel = cameraData.zoom;
    targetZoom = cameraData.zoom;
    gameView.setCenter(cameraData.position);
    gameView.setSize(cameraData.size);
    uiView.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    uiView.setCenter(sf::Vector2f(static_cast<float>(windowSize.x) / 2.0f, static_cast<float>(windowSize.y) / 2.0f));
}

SinglePlayerResult SinglePlayerGame::handleEvents() {
    while (std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            handleEscapeKey(); // Auto-save before quit
            return SinglePlayerResult::QUIT_GAME;
        }

        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            handleWindowResize({ resized->size.x, resized->size.y });
        }

        // *** ADD SCROLL WHEEL ZOOM HANDLING ***
        if (event->is<sf::Event::MouseWheelScrolled>()) {
            const auto* wheelEvent = event->getIf<sf::Event::MouseWheelScrolled>();
            if (wheelEvent) {
                // Zoom in/out based on scroll direction
                // Positive delta = scroll up = zoom in
                // Negative delta = scroll down = zoom out
                float zoomFactor = 1.0f + wheelEvent->delta * 0.1f;
                targetZoom *= zoomFactor;

                // Clamp zoom level to reasonable limits
                targetZoom = std::max(0.1f, std::min(targetZoom, 10.0f));
            }
        }

        // In SinglePlayerGame::handleEvents()
// Replace the complex ESC key logic with this simple version:

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                if (!escKeyPressed) {
                    escKeyPressed = true;
                    handleEscapeKey(); // Auto-save
                    std::cout << "ESC pressed - returning to saves menu..." << std::endl;
                    return SinglePlayerResult::RETURN_TO_MENU;
                }
            }
        }
        if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
            if (keyReleased->scancode == sf::Keyboard::Scancode::Escape) {
                escKeyPressed = false;
            }
        }
    }

    return SinglePlayerResult::CONTINUE_PLAYING;
}


void SinglePlayerGame::update(float deltaTime) {
    if (!isInitialized) return;

    gameTime += deltaTime;

    //// ADDED: Reset firstEscPress if timeout exceeded
    //if (firstEscPress && gameTime - escPressTime > DOUBLE_ESC_TIMEOUT) {
    //    firstEscPress = false;
    //}

    updateInput(deltaTime);
    updateGameObjects(deltaTime);
    updateCamera();
}




void SinglePlayerGame::updateInput(float deltaTime) {
    // Thrust controls
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
        vehicleManager->applyThrust(1.0f);
    }

    // Rotation controls
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
        vehicleManager->rotate(-180.0f * deltaTime);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
        vehicleManager->rotate(180.0f * deltaTime);
    }

    // *** FIX 1: ADD THRUST LEVEL HANDLING (1-9 keys) ***
    if (vehicleManager->getRocket()) {
        float newThrustLevel = -1.0f; // -1 means no change
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0)) newThrustLevel = 0.0f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1)) newThrustLevel = 0.1f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2)) newThrustLevel = 0.2f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num3)) newThrustLevel = 0.3f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num4)) newThrustLevel = 0.4f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num5)) newThrustLevel = 0.5f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num6)) newThrustLevel = 0.6f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num7)) newThrustLevel = 0.7f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num8)) newThrustLevel = 0.8f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num9)) newThrustLevel = 0.9f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Equal)) newThrustLevel = 1.0f;

        if (newThrustLevel >= 0.0f) {
            vehicleManager->getRocket()->setThrustLevel(newThrustLevel);
        }
    }

    // Vehicle switching
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L) && !lKeyPressed) {
        lKeyPressed = true;
        vehicleManager->switchVehicle();
    }
    else if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L)) {
        lKeyPressed = false;
    }

    // Satellite conversion
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T) && !tKeyPressed) {
        tKeyPressed = true;
        if (vehicleManager->canConvertToSatellite()) {
            int satelliteID = vehicleManager->convertRocketToSatellite();
            if (satelliteID >= 0) {
                std::cout << "Converted rocket to satellite (ID: " << satelliteID << ")" << std::endl;
            }
        }
    }
    else if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T)) {
        tKeyPressed = false;
    }

    // *** FIX 2: ADD COMPLETE FUEL TRANSFER LOGIC ***
    if (vehicleManager->getRocket()) {
        Rocket* rocket = vehicleManager->getRocket();

        // Manual fuel transfer controls - Period key (.) for fuel increase
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Period) && !fuelIncreaseKeyPressed) {
            fuelIncreaseKeyPressed = true;
            // Start fuel transfer from planet to rocket
            float transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * rocket->getThrustLevel() * GameConstants::FUEL_TRANSFER_THRUST_MULTIPLIER;
            if (transferRate < GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f) {
                transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f; // Minimum 10% rate
            }
            rocket->startFuelTransferIn(transferRate);
        }
        else if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Period)) {
            if (fuelIncreaseKeyPressed) {
                rocket->stopFuelTransfer();
            }
            fuelIncreaseKeyPressed = false;
        }

        // Manual fuel transfer controls - Comma key (,) for fuel decrease  
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Comma) && !fuelDecreaseKeyPressed) {
            fuelDecreaseKeyPressed = true;
            // Start fuel transfer from rocket to planet
            float transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * rocket->getThrustLevel() * GameConstants::FUEL_TRANSFER_THRUST_MULTIPLIER;
            if (transferRate < GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f) {
                transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f; // Minimum 10% rate
            }
            rocket->startFuelTransferOut(transferRate);
        }
        else if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Comma)) {
            if (fuelDecreaseKeyPressed) {
                rocket->stopFuelTransfer();
            }
            fuelDecreaseKeyPressed = false;
        }
    }

    // Zoom controls
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)) {
        targetZoom *= 1.02f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
        targetZoom /= 1.02f;
    }

    // Clamp zoom level
    targetZoom = std::max(0.1f, std::min(targetZoom, 10.0f));
}





void SinglePlayerGame::updateGameObjects(float deltaTime) {
    // Update main planet
    if (planet) {
        planet->update(deltaTime);
    }

    // Update orbiting planets
    for (const auto& orbitingPlanet : orbitingPlanets) {
        orbitingPlanet->update(deltaTime);
    }

    // Update vehicle manager
    if (vehicleManager) {
        vehicleManager->update(deltaTime);
    }

    // Update satellite system
    if (satelliteManager) {
        satelliteManager->update(deltaTime);

        // Update satellite manager with current rocket
        if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            satelliteManager->setNearbyRockets({ vehicleManager->getRocket() });
        }
    }

    // Update gravity simulator
    if (gravitySimulator) {
        gravitySimulator->update(deltaTime);
    }

    // Update UI
    if (uiManager) {
        uiManager->update(GameState::SINGLE_PLAYER, vehicleManager.get(), nullptr, nullptr,
            planets, nullptr, satelliteManager.get());
    }
}

void SinglePlayerGame::updateCamera() {
    // Smooth zoom
    zoomLevel += (targetZoom - zoomLevel) * 3.0f * (1.0f / 60.0f); // Assuming 60 FPS for smooth transition

    // Follow vehicle if not manually controlling camera
    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C) && vehicleManager) {
        sf::Vector2f vehiclePos = vehicleManager->getActiveVehicle()->getPosition();
        gameView.setCenter(vehiclePos);
    }

    // Apply zoom
    sf::Vector2f baseSize(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
    gameView.setSize(baseSize / zoomLevel);
}

void SinglePlayerGame::render() {
    if (!isInitialized) return;

    //window.clear(sf::Color::Black);

    renderGameObjects();
    renderUI();

    //window.display();
}

void SinglePlayerGame::renderGameObjects() {
    // Set game view
    window.setView(gameView);

    // Draw main planet
    if (planet) {
        planet->draw(window);
    }

    // Draw orbiting planets and their moons
    for (const auto& orbitingPlanet : orbitingPlanets) {
        orbitingPlanet->draw(window);

        // Draw moon trajectories if they exist
        const auto& moons = orbitingPlanet->getMoons();
        for (const auto& moon : moons) {
            if (moon) {
                moon->draw(window);
            }
        }
    }

    // Draw vehicles
    if (vehicleManager) {
        vehicleManager->drawWithConstantSize(window, zoomLevel);

        // ADD THESE MISSING VISUAL LINES:
        vehicleManager->drawVelocityVector(window, GameConstants::VELOCITY_VECTOR_SCALE);
        vehicleManager->drawGravityForceVectors(window, GameConstants::GRAVITY_VECTOR_SCALE);
        vehicleManager->drawTrajectory(window, GameConstants::TRAJECTORY_TIME_STEP, GameConstants::TRAJECTORY_STEPS);
    }

    // Draw satellites
    if (satelliteManager) {
        satelliteManager->draw(window);
    }

    // Draw UI elements that need game view
    if (uiManager) {
        uiManager->drawFuelCollectionLines(window, vehicleManager->getRocket());
        uiManager->drawSatelliteNetworkLines(window, satelliteManager.get());
        uiManager->drawSatelliteFuelTransfers(window, satelliteManager.get());
        uiManager->drawSatelliteToRocketLines(window, satelliteManager.get(), vehicleManager.get());
    }
}


void SinglePlayerGame::renderUI() {
    // Set UI view
    window.setView(uiView);

    if (uiManager) {
        uiManager->draw(window, GameState::SINGLE_PLAYER, nullptr, satelliteManager.get());
    }
}

void SinglePlayerGame::handleEscapeKey() {
    std::cout << "ESC pressed - performing auto-save..." << std::endl;
    performAutoSave();
}

bool SinglePlayerGame::performAutoSave() {
    // Generate auto-save name with timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

#ifdef _WIN32
    struct tm tm;
    localtime_s(&tm, &time_t);
#else
    auto tm = *std::localtime(&time_t);
#endif

    // Create auto-save name without using stringstream for now
    return autoSave();
}

GameSaveData SinglePlayerGame::getCurrentSaveData() const {
    GameSaveData saveData;

    // Basic metadata
    saveData.saveName = currentSaveName.empty() ? "SinglePlayer" : currentSaveName;
    saveData.playerName = "Player"; // Could be made configurable
    saveData.gameMode = SavedGameMode::SINGLE_PLAYER;
    saveData.gameTime = gameTime;
    saveData.currentPlayerCount = 1;

    // Save planets
    saveData.planets.clear();
    if (planet) {
        SavedPlanetData mainPlanetData;
        mainPlanetData.position = planet->getPosition();
        mainPlanetData.mass = planet->getMass();
        mainPlanetData.color = sf::Color(139, 0, 0); // Main planet color
        mainPlanetData.isMainPlanet = true;
        mainPlanetData.radius = planet->getRadius();
        saveData.planets.push_back(mainPlanetData);
    }

    for (const auto& orbitingPlanet : orbitingPlanets) {
        SavedPlanetData planetData;
        planetData.position = orbitingPlanet->getPosition();
        planetData.mass = orbitingPlanet->getMass();
        planetData.color = sf::Color::Green; // Could get actual color from planet
        planetData.isMainPlanet = false;
        planetData.radius = orbitingPlanet->getRadius();
        saveData.planets.push_back(planetData);
    }

    // Save player state
    if (vehicleManager) {
        PlayerState playerState;
        playerState.playerID = 1;
        playerState.position = vehicleManager->getActiveVehicle()->getPosition();
        playerState.velocity = vehicleManager->getActiveVehicle()->getVelocity();
        playerState.isRocket = (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET);

        if (playerState.isRocket && vehicleManager->getRocket()) {
            playerState.rotation = vehicleManager->getRocket()->getRotation();
            playerState.currentFuel = vehicleManager->getRocket()->getCurrentFuel();
            playerState.maxFuel = vehicleManager->getRocket()->getMaxFuel();
            playerState.mass = vehicleManager->getRocket()->getMass();
        }

        saveData.players.push_back(playerState);
    }

    // Save satellites
    if (satelliteManager) {
        // TODO: Add proper getSaveData method to SatelliteManager
        // saveData.satellites = satelliteManager->getSaveData();
        saveData.satellites.clear(); // For now, empty satellites
    }

    // Save camera data
    saveData.camera.position = gameView.getCenter();
    saveData.camera.size = gameView.getSize();
    saveData.camera.zoom = zoomLevel;
    saveData.camera.followingPlayerID = 1; // Single player always follows player 1

    // Save UI settings (using defaults for now)
    saveData.uiSettings.showUI = true;
    saveData.uiSettings.showDebugInfo = false;
    saveData.uiSettings.showOrbitPaths = true;
    saveData.uiSettings.showFuelRings = true;
    saveData.uiSettings.showPlayerLabels = true;

    return saveData;
}

bool SinglePlayerGame::saveGame(const std::string& saveName) {
    GameSaveData saveData = getCurrentSaveData();
    saveData.saveName = saveName;

    bool success = saveManager.saveGame(saveData, saveName);
    if (success) {
        currentSaveName = saveName;
        std::cout << "Game saved successfully as: " << saveName << std::endl;
    }
    else {
        std::cerr << "Failed to save game: " << saveName << std::endl;
    }

    return success;
}

bool SinglePlayerGame::quickSave() {
    GameSaveData saveData = getCurrentSaveData();
    return saveManager.quickSave(saveData);
}

bool SinglePlayerGame::autoSave() {
    GameSaveData saveData = getCurrentSaveData();
    return saveManager.autoSave(saveData);
}

void SinglePlayerGame::handleWindowResize(sf::Vector2u newSize) {
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