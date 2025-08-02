#include <SFML/Graphics.hpp>
#include "Planet.h"
#include "Rocket.h"
#include "Car.h"
#include "VehicleManager.h"
#include "GravitySimulator.h"
#include "GameConstants.h"
#include "Button.h"
#include "MainMenu.h"
#include "MultiplayerMenu.h"
#include "OnlineMultiplayerMenu.h"
#include "SplitScreenManager.h"
#include "NetworkManager.h"
#include "Player.h"
#include "UIManager.h"
#include "SatelliteManager.h"
#include "Satellite.h"
#include <memory>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <limits>
#include <iostream>
#include <cmath>
#include <algorithm>

#ifdef _DEBUG
#pragma comment(lib, "sfml-graphics-d.lib")
#pragma comment(lib, "sfml-window-d.lib")
#pragma comment(lib, "sfml-system-d.lib")
#pragma comment(lib, "sfml-network-d.lib")
#else
#pragma comment(lib, "sfml-graphics.lib")
#pragma comment(lib, "sfml-window.lib")
#pragma comment(lib, "sfml-system.lib")
#pragma comment(lib, "sfml-network.lib")
#endif

// Helper function declarations
float calculateApoapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G);
float calculatePeriapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G);
//
//// Game state management
//enum class GameState {
//    MAIN_MENU,
//    MULTIPLAYER_MENU,
//    ONLINE_MENU,
//    SINGLE_PLAYER,
//    LOCAL_PC_MULTIPLAYER,
//    LAN_MULTIPLAYER,
//    ONLINE_MULTIPLAYER,
//    QUIT
//};

// Game class to encapsulate game logic
class Game {
private:
    sf::RenderWindow window;
    MainMenu mainMenu;
    std::unique_ptr<MultiplayerMenu> multiplayerMenu;
    std::unique_ptr<OnlineMultiplayerMenu> onlineMenu;
    GameState currentState;

    // UI Management - UPDATED
    std::unique_ptr<UIManager> uiManager;

    // Network manager for multiplayer
    std::unique_ptr<NetworkManager> networkManager;

    // Game objects
    std::unique_ptr<Planet> planet;
    std::unique_ptr<Planet> planet2;
    std::vector<Planet*> planets;
    std::unique_ptr<VehicleManager> vehicleManager;
    std::unique_ptr<SplitScreenManager> splitScreenManager;
    std::unique_ptr<GravitySimulator> gravitySimulator;

    // SATELLITE SYSTEM - NEW
    std::unique_ptr<SatelliteManager> satelliteManager;

    // Player management for network multiplayer
    std::vector<std::unique_ptr<Player>> players;
    std::unique_ptr<Player> localPlayer;
    std::vector<std::unique_ptr<Player>> remotePlayers;

    // Camera and visual settings
    sf::View gameView;
    sf::View uiView;
    float zoomLevel;
    float targetZoom;

    // Input tracking - UPDATED FOR SATELLITE SYSTEM
    bool lKeyPressed;
    bool tKeyPressed;  // NEW: Transform to satellite key
    bool fuelIncreaseKeyPressed;
    bool fuelDecreaseKeyPressed;

public:
    Game() : window(sf::VideoMode({ 1280, 720 }), "Katie's Space Program"),
        mainMenu(sf::Vector2u(1280, 720)),
        multiplayerMenu(std::make_unique<MultiplayerMenu>(sf::Vector2u(1280, 720))),
        onlineMenu(std::make_unique<OnlineMultiplayerMenu>(sf::Vector2u(1280, 720))),
        currentState(GameState::MAIN_MENU),
        gameView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
        uiView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
        zoomLevel(1.0f), targetZoom(1.0f),
        lKeyPressed(false), tKeyPressed(false),
        fuelIncreaseKeyPressed(false), fuelDecreaseKeyPressed(false)
    {
        // Initialize UI Manager
        uiManager = std::make_unique<UIManager>(sf::Vector2u(1280, 720));

        // Initialize Satellite Manager
        satelliteManager = std::make_unique<SatelliteManager>();
    }

    void initializeSinglePlayer() {
        // Create planets
        planet = std::make_unique<Planet>(
            sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
            0.0f, GameConstants::MAIN_PLANET_MASS, sf::Color::Blue);
        planet->setVelocity(sf::Vector2f(0.f, 0.f));

        planet2 = std::make_unique<Planet>(
            sf::Vector2f(GameConstants::SECONDARY_PLANET_X, GameConstants::SECONDARY_PLANET_Y),
            0.0f, GameConstants::SECONDARY_PLANET_MASS, sf::Color::Green);

        float orbitSpeed = std::sqrt(GameConstants::G * planet->getMass() / GameConstants::PLANET_ORBIT_DISTANCE);
        planet2->setVelocity(sf::Vector2f(0.f, orbitSpeed));

        // Setup planet vector
        planets.clear();
        planets.push_back(planet.get());
        planets.push_back(planet2.get());

        // SATELLITE SYSTEM: Set planets for satellite manager
        satelliteManager->setPlanets(planets);

        // Create vehicle manager
        sf::Vector2f planetPos = planet->getPosition();
        float planetRadius = planet->getRadius();
        float rocketSize = GameConstants::ROCKET_SIZE;
        sf::Vector2f direction(0, -1);
        sf::Vector2f rocketPos = planetPos + direction * (planetRadius + rocketSize);

        vehicleManager = std::make_unique<VehicleManager>(rocketPos, planets);

        // FUEL SYSTEM: Set up fuel collection for the rocket
        if (vehicleManager->getRocket()) {
            vehicleManager->getRocket()->setNearbyPlanets(planets);
        }

        // Setup gravity simulator
        gravitySimulator = std::make_unique<GravitySimulator>();
        gravitySimulator->addPlanet(planet.get());
        gravitySimulator->addPlanet(planet2.get());
        gravitySimulator->addVehicleManager(vehicleManager.get());

        // Clear players for single player mode
        players.clear();
        localPlayer.reset();
        remotePlayers.clear();

        // Reset camera
        zoomLevel = 1.0f;
        targetZoom = 1.0f;
        gameView.setCenter(rocketPos);

        std::cout << "Single Player mode initialized with satellite system!" << std::endl;
        std::cout << "Use 'T' to convert rocket to satellite, '.' to collect fuel, ',' to give fuel" << std::endl;
    }

    void initializeLocalPCMultiplayer() {
        // Create planets (same as single player)
        planet = std::make_unique<Planet>(
            sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
            0.0f, GameConstants::MAIN_PLANET_MASS, sf::Color::Blue);
        planet->setVelocity(sf::Vector2f(0.f, 0.f));

        planet2 = std::make_unique<Planet>(
            sf::Vector2f(GameConstants::SECONDARY_PLANET_X, GameConstants::SECONDARY_PLANET_Y),
            0.0f, GameConstants::SECONDARY_PLANET_MASS, sf::Color::Green);

        float orbitSpeed = std::sqrt(GameConstants::G * planet->getMass() / GameConstants::PLANET_ORBIT_DISTANCE);
        planet2->setVelocity(sf::Vector2f(0.f, orbitSpeed));

        planets.clear();
        planets.push_back(planet.get());
        planets.push_back(planet2.get());

        // SATELLITE SYSTEM: Set planets for satellite manager
        satelliteManager->setPlanets(planets);

        // Create spawn positions for both players
        sf::Vector2f planetPos = planet->getPosition();
        float planetRadius = planet->getRadius();
        float rocketSize = GameConstants::ROCKET_SIZE;

        sf::Vector2f player1Pos = planetPos + sf::Vector2f(0, -1) * (planetRadius + rocketSize);
        sf::Vector2f player2Pos = planetPos + sf::Vector2f(0, 1) * (planetRadius + rocketSize);

        splitScreenManager = std::make_unique<SplitScreenManager>(player1Pos, player2Pos, planets);

        gravitySimulator = std::make_unique<GravitySimulator>();
        gravitySimulator->addPlanet(planet.get());
        gravitySimulator->addPlanet(planet2.get());
        gravitySimulator->addVehicleManager(splitScreenManager->getPlayer1());
        gravitySimulator->addVehicleManager(splitScreenManager->getPlayer2());

        players.clear();
        localPlayer.reset();
        remotePlayers.clear();

        zoomLevel = 1.0f;
        targetZoom = 1.0f;
        gameView.setCenter(splitScreenManager->getCenterPoint());

        std::cout << "Local PC Split-Screen Multiplayer initialized with satellite system!" << std::endl;
    }

    void initializeLANMultiplayer() {
        std::cout << "LAN Multiplayer satellite system not yet fully implemented" << std::endl;
        initializeSinglePlayer(); // Fallback for now
    }

    void initializeOnlineMultiplayer(const ConnectionInfo& connectionInfo) {
        std::cout << "Online Multiplayer satellite system not yet fully implemented" << std::endl;
        initializeSinglePlayer(); // Fallback for now
    }

    void handleMenuEvents(const sf::Event& event) {
        sf::Vector2f mousePos = uiManager->getMousePosition(window);

        if (currentState == GameState::MAIN_MENU) {
            mainMenu.handleEvent(event, mousePos);

            if (!mainMenu.getIsActive()) {
                GameMode selectedMode = mainMenu.getSelectedMode();

                switch (selectedMode) {
                case GameMode::SINGLE_PLAYER:
                    currentState = GameState::SINGLE_PLAYER;
                    initializeSinglePlayer();
                    break;
                case GameMode::MULTIPLAYER_HOST:
                case GameMode::MULTIPLAYER_JOIN:
                    currentState = GameState::MULTIPLAYER_MENU;
                    multiplayerMenu->show();
                    break;
                case GameMode::QUIT:
                    window.close();
                    break;
                default:
                    break;
                }
            }
        }
        else if (currentState == GameState::MULTIPLAYER_MENU) {
            multiplayerMenu->handleEvent(event, mousePos);

            if (!multiplayerMenu->getIsActive()) {
                MultiplayerMode selectedMode = multiplayerMenu->getSelectedMode();

                switch (selectedMode) {
                case MultiplayerMode::LOCAL_PC:
                    currentState = GameState::LOCAL_PC_MULTIPLAYER;
                    initializeLocalPCMultiplayer();
                    break;
                case MultiplayerMode::LOCAL_AREA:
                    currentState = GameState::LAN_MULTIPLAYER;
                    initializeLANMultiplayer();
                    break;
                case MultiplayerMode::ONLINE:
                    currentState = GameState::ONLINE_MENU;
                    onlineMenu->show();
                    break;
                case MultiplayerMode::BACK_TO_MAIN:
                    currentState = GameState::MAIN_MENU;
                    mainMenu.show();
                    break;
                default:
                    break;
                }
            }
        }
        else if (currentState == GameState::ONLINE_MENU) {
            onlineMenu->handleEvent(event, mousePos);

            if (!onlineMenu->getIsActive()) {
                OnlineMode selectedMode = onlineMenu->getSelectedMode();

                switch (selectedMode) {
                case OnlineMode::HOST_GAME:
                case OnlineMode::JOIN_GAME: {
                    ConnectionInfo connectionInfo = onlineMenu->getConnectionInfo();
                    if (connectionInfo.isValid()) {
                        currentState = GameState::ONLINE_MULTIPLAYER;
                        initializeOnlineMultiplayer(connectionInfo);
                    }
                    else {
                        std::cout << "Invalid connection info!" << std::endl;
                        onlineMenu->show();
                    }
                    break;
                }
                case OnlineMode::BACK_TO_MULTIPLAYER:
                    currentState = GameState::MULTIPLAYER_MENU;
                    multiplayerMenu->show();
                    break;
                default:
                    break;
                }
            }
        }
    }
    void handleGameEvents(const sf::Event& event) {
        // Handle split-screen transform inputs first
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            splitScreenManager->handleTransformInputs(event);
        }

        if (event.is<sf::Event::KeyPressed>()) {
            const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
            if (keyEvent) {
                if (keyEvent->code == sf::Keyboard::Key::Escape) {
                    handleEscapeKey();
                    return;
                }
                else if (keyEvent->code == sf::Keyboard::Key::P) {
                    togglePlanetGravity();
                }
                else if (keyEvent->code == sf::Keyboard::Key::L && !lKeyPressed) {
                    handleVehicleTransform();
                }
                // SATELLITE CONVERSION - NEW
                else if (keyEvent->code == sf::Keyboard::Key::T && !tKeyPressed) {
                    handleSatelliteConversion();
                }
                // FUEL TRANSFER INPUT HANDLING
                else if (keyEvent->code == sf::Keyboard::Key::Period && !fuelIncreaseKeyPressed) {
                    handleFuelTransferStart(true);
                }
                else if (keyEvent->code == sf::Keyboard::Key::Comma && !fuelDecreaseKeyPressed) {
                    handleFuelTransferStart(false);
                }
                // DEBUG KEYS
                else if (keyEvent->code == sf::Keyboard::Key::H && networkManager) {
                    networkManager->sendHello();
                    std::cout << "Sent hello message to network!" << std::endl;
                }
                // UI CONTROLS
                else if (keyEvent->code == sf::Keyboard::Key::F1) {
                    uiManager->toggleUI();
                }
                else if (keyEvent->code == sf::Keyboard::Key::F2) {
                    uiManager->toggleDebugInfo();
                }
                // SATELLITE DEBUG CONTROLS
                else if (keyEvent->code == sf::Keyboard::Key::F3) {
                    satelliteManager->printNetworkStatus();
                }
                else if (keyEvent->code == sf::Keyboard::Key::F4) {
                    satelliteManager->optimizeNetworkFuelDistribution();
                    std::cout << "Triggered satellite fuel optimization" << std::endl;
                }
            }
        }

        if (event.is<sf::Event::KeyReleased>()) {
            const auto* keyEvent = event.getIf<sf::Event::KeyReleased>();
            if (keyEvent) {
                if (keyEvent->code == sf::Keyboard::Key::L) {
                    lKeyPressed = false;
                }
                else if (keyEvent->code == sf::Keyboard::Key::T) {
                    tKeyPressed = false;
                }
                else if (keyEvent->code == sf::Keyboard::Key::Period) {
                    handleFuelTransferStop();
                }
                else if (keyEvent->code == sf::Keyboard::Key::Comma) {
                    handleFuelTransferStop();
                }
            }
        }

        // Handle mouse wheel for zooming
        if (event.is<sf::Event::MouseWheelScrolled>()) {
            handleMouseWheelZoom(event);
        }

        if (event.is<sf::Event::Resized>()) {
            handleWindowResize(event);
        }
    }
    void handleEscapeKey() {
        if (networkManager) {
            networkManager->disconnect();
            networkManager.reset();
        }

        remotePlayers.clear();
        players.clear();
        localPlayer.reset();

        // Clear satellite system
        satelliteManager->removeAllSatellites();

        currentState = GameState::MAIN_MENU;
        mainMenu.show();
    }

    void togglePlanetGravity() {
        static bool planetGravity = true;
        planetGravity = !planetGravity;
        gravitySimulator->setSimulatePlanetGravity(planetGravity);
        std::cout << "Planet gravity " << (planetGravity ? "enabled" : "disabled") << std::endl;
    }

    void renderGame() {
        window.setView(gameView);

        // Find closest planet for orbit path
        Planet* closestPlanet = nullptr;
        float closestDistance = std::numeric_limits<float>::max();

        sf::Vector2f referencePos = getCameraReferencePosition();

        for (auto& planetPtr : planets) {
            sf::Vector2f direction = planetPtr->getPosition() - referencePos;
            float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            if (dist < closestDistance) {
                closestDistance = dist;
                closestPlanet = planetPtr;
            }
        }

        // Draw orbit path for closest planet
        if (closestPlanet) {
            closestPlanet->drawOrbitPath(window, planets);
        }

        // Draw trajectory for rockets
        drawTrajectories();

        // Draw game objects - planets automatically draw fuel collection rings
        planet->draw(window);
        planet2->draw(window);

        // SATELLITE SYSTEM: Draw satellites
        satelliteManager->drawWithConstantSize(window, zoomLevel);

        // Draw vehicles and fuel collection indicators
        drawVehiclesAndFuelLines();

        // Draw velocity vectors
        drawVelocityVectors();

        // Draw UI
        uiManager->draw(window, currentState, networkManager.get());
    }

    sf::Vector2f getCameraReferencePosition() {
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            return splitScreenManager->getCenterPoint();
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            return localPlayer->getPosition();
        }
        else if (vehicleManager) {
            return vehicleManager->getActiveVehicle()->getPosition();
        }
        return sf::Vector2f(0, 0);
    }

    void drawTrajectories() {
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            if (splitScreenManager->getPlayer1()->getActiveVehicleType() == VehicleType::ROCKET) {
                splitScreenManager->getPlayer1()->getRocket()->drawTrajectory(window, planets,
                    GameConstants::TRAJECTORY_TIME_STEP, GameConstants::TRAJECTORY_STEPS, false);
            }
            if (splitScreenManager->getPlayer2()->getActiveVehicleType() == VehicleType::ROCKET) {
                splitScreenManager->getPlayer2()->getRocket()->drawTrajectory(window, planets,
                    GameConstants::TRAJECTORY_TIME_STEP, GameConstants::TRAJECTORY_STEPS, false);
            }
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            if (localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                localPlayer->getVehicleManager()->getRocket()->drawTrajectory(window, planets,
                    GameConstants::TRAJECTORY_TIME_STEP, GameConstants::TRAJECTORY_STEPS, false);
            }

            for (auto& remotePlayer : remotePlayers) {
                if (remotePlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                    remotePlayer->getVehicleManager()->getRocket()->drawTrajectory(window, planets,
                        GameConstants::TRAJECTORY_TIME_STEP, GameConstants::TRAJECTORY_STEPS, false);
                }
            }
        }
        else if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            vehicleManager->getRocket()->drawTrajectory(window, planets,
                GameConstants::TRAJECTORY_TIME_STEP, GameConstants::TRAJECTORY_STEPS, false);
        }
    }

    void drawVehiclesAndFuelLines() {
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            splitScreenManager->drawWithConstantSize(window, zoomLevel);
            uiManager->drawMultipleFuelLines(window,
                splitScreenManager->getPlayer1(), splitScreenManager->getPlayer2());
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            localPlayer->drawWithConstantSize(window, zoomLevel);

            if (localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                uiManager->drawFuelCollectionLines(window, localPlayer->getVehicleManager()->getRocket());
            }

            for (auto& remotePlayer : remotePlayers) {
                remotePlayer->drawWithConstantSize(window, zoomLevel);

                if (remotePlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                    uiManager->drawFuelCollectionLines(window, remotePlayer->getVehicleManager()->getRocket());
                }

                if (uiManager->isFontLoaded()) {
                    remotePlayer->drawPlayerLabel(window, uiManager->getFont());
                }
            }

            if (uiManager->isFontLoaded()) {
                localPlayer->drawPlayerLabel(window, uiManager->getFont());
            }
        }
        else if (vehicleManager) {
            vehicleManager->drawWithConstantSize(window, zoomLevel);

            if (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
                uiManager->drawFuelCollectionLines(window, vehicleManager->getRocket());
            }
        }
    }

    void drawVelocityVectors() {
        // Draw planet velocity vectors
        planet->drawVelocityVector(window, 5.0f);
        planet2->drawVelocityVector(window, 5.0f);

        // Draw vehicle velocity vectors
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            splitScreenManager->drawVelocityVectors(window, 2.0f);
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            localPlayer->drawVelocityVector(window, 2.0f);

            for (auto& remotePlayer : remotePlayers) {
                remotePlayer->drawVelocityVector(window, 2.0f);
                if (remotePlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                    remotePlayer->getVehicleManager()->getRocket()->drawGravityForceVectors(window, planets, GameConstants::GRAVITY_VECTOR_SCALE);
                }
            }

            if (localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                localPlayer->getVehicleManager()->getRocket()->drawGravityForceVectors(window, planets, GameConstants::GRAVITY_VECTOR_SCALE);
            }
        }
        else if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            vehicleManager->drawVelocityVector(window, 2.0f);
            vehicleManager->getRocket()->drawGravityForceVectors(window, planets, GameConstants::GRAVITY_VECTOR_SCALE);
        }
    }

    void renderMenu() {
        uiManager->setUIView(window);
        sf::Vector2f mousePos = uiManager->getMousePosition(window);

        if (currentState == GameState::MAIN_MENU) {
            mainMenu.update(mousePos);
            mainMenu.draw(window);
        }
        else if (currentState == GameState::MULTIPLAYER_MENU) {
            multiplayerMenu->update(mousePos);
            multiplayerMenu->draw(window);
        }
        else if (currentState == GameState::ONLINE_MENU) {
            onlineMenu->update(mousePos);
            onlineMenu->draw(window);
        }
    }


    void handleSatelliteConversion() {
        tKeyPressed = true;

        if (currentState == GameState::SINGLE_PLAYER && vehicleManager &&
            vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {

            Rocket* rocket = vehicleManager->getRocket();

            if (satelliteManager->canConvertRocketToSatellite(rocket)) {
                // Get optimal conversion configuration
                SatelliteConversionConfig config = satelliteManager->getOptimalConversionConfig(rocket);

                // Create satellite from rocket
                int satelliteID = satelliteManager->createSatelliteFromRocket(rocket, config);

                if (satelliteID >= 0) {
                    std::cout << "Successfully converted rocket to satellite (ID: " << satelliteID << ")" << std::endl;

                    // Create new rocket at nearest planet surface
                    sf::Vector2f newRocketPos = findNearestPlanetSurface(rocket->getPosition());
                    vehicleManager = std::make_unique<VehicleManager>(newRocketPos, planets);

                    if (vehicleManager->getRocket()) {
                        vehicleManager->getRocket()->setNearbyPlanets(planets);
                    }

                    // Update gravity simulator
                    gravitySimulator->addVehicleManager(vehicleManager.get());

                    // Update camera to follow new rocket
                    gameView.setCenter(newRocketPos);

                    std::cout << "New rocket spawned at (" << newRocketPos.x << ", " << newRocketPos.y << ")" << std::endl;
                }
                else {
                    std::cout << "Failed to convert rocket to satellite" << std::endl;
                }
            }
            else {
                std::cout << "Cannot convert rocket to satellite - check fuel and altitude requirements" << std::endl;
            }
        }
        else {
            std::cout << "Satellite conversion only available for rockets in single player mode" << std::endl;
        }
    }

    sf::Vector2f findNearestPlanetSurface(sf::Vector2f position) {
        Planet* nearestPlanet = nullptr;
        float minDistance = std::numeric_limits<float>::max();

        for (auto* planet : planets) {
            sf::Vector2f direction = planet->getPosition() - position;
            float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            if (dist < minDistance) {
                minDistance = dist;
                nearestPlanet = planet;
            }
        }

        if (nearestPlanet) {
            sf::Vector2f direction = position - nearestPlanet->getPosition();
            float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            if (length > 0) {
                direction /= length; // normalize
            }
            return nearestPlanet->getPosition() + direction * (nearestPlanet->getRadius() + GameConstants::ROCKET_SIZE + 5.0f);
        }

        // Fallback to original planet
        return sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y - 150.0f);
    }

    void handleFuelTransferStart(bool isIncrease) {
        if (isIncrease) {
            fuelIncreaseKeyPressed = true;
            if (currentState == GameState::SINGLE_PLAYER && vehicleManager &&
                vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* rocket = vehicleManager->getRocket();
                float transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * rocket->getThrustLevel() * GameConstants::FUEL_TRANSFER_THRUST_MULTIPLIER;
                if (transferRate < GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f) {
                    transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f;
                }
                rocket->startFuelTransferIn(transferRate);
            }
        }
        else {
            fuelDecreaseKeyPressed = true;
            if (currentState == GameState::SINGLE_PLAYER && vehicleManager &&
                vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* rocket = vehicleManager->getRocket();
                float transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * rocket->getThrustLevel() * GameConstants::FUEL_TRANSFER_THRUST_MULTIPLIER;
                if (transferRate < GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f) {
                    transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f;
                }
                rocket->startFuelTransferOut(transferRate);
            }
        }
    }

    void handleFuelTransferStop() {
        fuelIncreaseKeyPressed = false;
        fuelDecreaseKeyPressed = false;

        if (currentState == GameState::SINGLE_PLAYER && vehicleManager &&
            vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            vehicleManager->getRocket()->stopFuelTransfer();
        }
    }

    void handleMouseWheelZoom(const sf::Event& event) {
        const auto* scrollEvent = event.getIf<sf::Event::MouseWheelScrolled>();
        if (scrollEvent && scrollEvent->wheel == sf::Mouse::Wheel::Vertical) {
            const float zoomFactor = 1.1f;
            if (scrollEvent->delta > 0) {
                targetZoom /= zoomFactor;
            }
            else {
                targetZoom *= zoomFactor;
            }
            targetZoom = std::max(0.5f, std::min(targetZoom, 50.0f));

            updateCameraCenter();
        }
    }

    void handleWindowResize(const sf::Event& event) {
        const auto* resizeEvent = event.getIf<sf::Event::Resized>();
        if (resizeEvent) {
            sf::Vector2u newSize(resizeEvent->size.x, resizeEvent->size.y);

            float aspectRatio = static_cast<float>(newSize.x) / static_cast<float>(newSize.y);
            gameView.setSize(sf::Vector2f(
                newSize.y * aspectRatio * zoomLevel,
                newSize.y * zoomLevel
            ));

            uiView.setSize(sf::Vector2f(
                static_cast<float>(newSize.x),
                static_cast<float>(newSize.y)
            ));
            uiView.setCenter(sf::Vector2f(
                static_cast<float>(newSize.x) / 2.0f,
                static_cast<float>(newSize.y) / 2.0f
            ));

            // Update UI Manager
            uiManager->handleWindowResize(newSize);

            window.setView(gameView);
        }
    }

    void updateCameraCenter() {
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            gameView.setCenter(splitScreenManager->getCenterPoint());
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            gameView.setCenter(localPlayer->getPosition());
        }
        else if (vehicleManager) {
            gameView.setCenter(vehicleManager->getActiveVehicle()->getPosition());
        }
    }

    void handleGameInput(float deltaTime) {
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            handleSplitScreenInput(deltaTime);
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            localPlayer->handleLocalInput(deltaTime);
        }
        else if (vehicleManager) {
            handleSinglePlayerInput(deltaTime);
        }
    }

    void handleSplitScreenInput(float deltaTime) {
        splitScreenManager->handlePlayer1Input(deltaTime);
        splitScreenManager->handlePlayer2Input(deltaTime);

        // Synced thrust level controls
        handleThrustLevelControls(deltaTime, true);
    }

    void handleSinglePlayerInput(float deltaTime) {
        // Thrust level controls
        handleThrustLevelControls(deltaTime, false);

        // Movement controls
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            vehicleManager->applyThrust(vehicleManager->getRocket()->getThrustLevel());
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            vehicleManager->applyThrust(-vehicleManager->getRocket()->getThrustLevel());
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            vehicleManager->rotate(-6.0f * deltaTime * 60.0f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            vehicleManager->rotate(6.0f * deltaTime * 60.0f);

        handleCameraControls();
    }

    void handleThrustLevelControls(float deltaTime, bool isSplitScreen) {
        float newThrustLevel = -1.0f;

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
            if (isSplitScreen && splitScreenManager) {
                splitScreenManager->setSyncedThrustLevel(newThrustLevel);
            }
            else if (vehicleManager && vehicleManager->getRocket()) {
                vehicleManager->getRocket()->setThrustLevel(newThrustLevel);
            }
        }
    }

    void handleCameraControls() {
        if (currentState == GameState::SINGLE_PLAYER && vehicleManager) {
            const float minZoom = 1.0f;
            const float maxZoom = 1000.0f;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z)) {
                targetZoom = std::min(maxZoom, targetZoom * 1.05f);
                gameView.setCenter(vehicleManager->getActiveVehicle()->getPosition());
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X)) {
                sf::Vector2f vehiclePos = vehicleManager->getActiveVehicle()->getPosition();
                sf::Vector2f direction1 = vehiclePos - planet->getPosition();
                sf::Vector2f direction2 = vehiclePos - planet2->getPosition();
                float dist1 = std::sqrt(direction1.x * direction1.x + direction1.y * direction1.y);
                float dist2 = std::sqrt(direction2.x * direction2.x + direction2.y * direction2.y);

                targetZoom = minZoom + (std::min(dist1, dist2) - (planet->getRadius() + GameConstants::ROCKET_SIZE)) / 100.0f;
                targetZoom = std::max(minZoom, std::min(targetZoom, maxZoom));
                gameView.setCenter(vehiclePos);
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
                targetZoom = 10.0f;
                gameView.setCenter(planet2->getPosition());
            }
        }
    }

    void updateGame(float deltaTime) {
        // Update network manager if active
        if (networkManager) {
            networkManager->update(deltaTime);
        }

        // Update simulation
        gravitySimulator->update(deltaTime);
        planet->update(deltaTime);
        planet2->update(deltaTime);

        // SATELLITE SYSTEM: Update satellites
        satelliteManager->update(deltaTime);

        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            splitScreenManager->update(deltaTime);

            if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
                gameView.setCenter(splitScreenManager->getCenterPoint());
            }
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            localPlayer->update(deltaTime);

            for (auto& remotePlayer : remotePlayers) {
                remotePlayer->update(deltaTime);
            }

            if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
                gameView.setCenter(localPlayer->getPosition());
            }
        }
        else if (vehicleManager) {
            vehicleManager->update(deltaTime);

            // Auto-center camera on vehicle
            sf::Vector2f vehiclePos = vehicleManager->getActiveVehicle()->getPosition();
            gameView.setCenter(vehiclePos);
        }

        // Smooth zoom
        zoomLevel += (targetZoom - zoomLevel) * deltaTime * 2.0f;
        gameView.setSize(sf::Vector2f(1280.f * zoomLevel, 720.f * zoomLevel));

        // Update UI Manager
        uiManager->update(currentState, vehicleManager.get(), splitScreenManager.get(),
            localPlayer.get(), planets, networkManager.get());
    }

    // Fix the handleVehicleTransform method (it had incorrect logic)
    void handleVehicleTransform() {
        lKeyPressed = true;

        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            // Split screen transform is handled in splitScreenManager->handleTransformInputs()
            // No additional logic needed here
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            localPlayer->requestTransform();
        }
        else if (vehicleManager) {
            vehicleManager->switchVehicle();
        }
    }


    void run() {
        sf::Clock clock;

        while (window.isOpen()) {
            float deltaTime = std::min(clock.restart().asSeconds(), 0.1f);

            // Handle events
            if (std::optional<sf::Event> event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    if (networkManager) {
                        networkManager->disconnect();
                    }
                    window.close();
                    continue;
                }

                switch (currentState) {
                case GameState::MAIN_MENU:
                case GameState::MULTIPLAYER_MENU:
                case GameState::ONLINE_MENU:
                    handleMenuEvents(*event);
                    break;
                case GameState::SINGLE_PLAYER:
                case GameState::LOCAL_PC_MULTIPLAYER:
                case GameState::LAN_MULTIPLAYER:
                case GameState::ONLINE_MULTIPLAYER:
                    handleGameEvents(*event);
                    break;
                default:
                    break;
                }
            }

            // Update game state
            switch (currentState) {
            case GameState::SINGLE_PLAYER:
            case GameState::LOCAL_PC_MULTIPLAYER:
            case GameState::LAN_MULTIPLAYER:
            case GameState::ONLINE_MULTIPLAYER:
                handleGameInput(deltaTime);
                updateGame(deltaTime);
                break;
            default:
                break;
            }

            // Render
            window.clear(sf::Color::Black);

            switch (currentState) {
            case GameState::MAIN_MENU:
            case GameState::MULTIPLAYER_MENU:
            case GameState::ONLINE_MENU:
                renderMenu();
                break;
            case GameState::SINGLE_PLAYER:
            case GameState::LOCAL_PC_MULTIPLAYER:
            case GameState::LAN_MULTIPLAYER:
            case GameState::ONLINE_MULTIPLAYER:
                renderGame();
                break;
            default:
                break;
            }

            window.display();
        }
    }
};

// Helper function implementations
float calculateApoapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G) {
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
    float energy = 0.5f * speed * speed - G * planetMass / distance;

    float semiMajor = -G * planetMass / (2 * energy);

    if (energy >= 0) return -1.0f;

    sf::Vector2f eVec;
    float vSquared = speed * speed;
    eVec.x = (vSquared * pos.x - (pos.x * vel.x + pos.y * vel.y) * vel.x) / (G * planetMass) - pos.x / distance;
    eVec.y = (vSquared * pos.y - (pos.x * vel.x + pos.y * vel.y) * vel.y) / (G * planetMass) - pos.y / distance;
    float ecc = std::sqrt(eVec.x * eVec.x + eVec.y * eVec.y);

    return semiMajor * (1 + ecc);
}

float calculatePeriapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G) {
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
    float energy = 0.5f * speed * speed - G * planetMass / distance;

    float semiMajor = -G * planetMass / (2 * energy);

    if (energy >= 0) return -1.0f;

    sf::Vector2f eVec;
    float vSquared = speed * speed;
    eVec.x = (vSquared * pos.x - (pos.x * vel.x + pos.y * vel.y) * vel.x) / (G * planetMass) - pos.x / distance;
    eVec.y = (vSquared * pos.y - (pos.x * vel.x + pos.y * vel.y) * vel.y) / (G * planetMass) - pos.y / distance;
    float ecc = std::sqrt(eVec.x * eVec.x + eVec.y * eVec.y);

    return semiMajor * (1 - ecc);
}

int main() {
    Game game;
    game.run();
    return 0;
}


