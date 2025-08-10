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
    std::vector<std::unique_ptr<Planet>> orbitingPlanets;
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
        // Create planets using dynamic system
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

        // After creating orbital planets, add their moons
        for (size_t i = 0; i < orbitingPlanets.size(); i++) {
            const auto& config = planetConfigs[i];

            // Check if this planet has moons configured
            if (!config.moons.empty()) {
                // Create moons for this planet
                for (const auto& moonConfig : config.moons) {
                    auto moon = Planet::createMoon(
                        orbitingPlanets[i].get(),
                        moonConfig.orbitDistanceMultiplier,
                        moonConfig.massRatio,
                        moonConfig.angleOffset,
                        moonConfig.color
                    );

                    if (moon) {
                        // Add moon to its parent planet
                        orbitingPlanets[i]->addMoon(std::move(moon));
                    }
                }

                std::cout << "Added " << config.moons.size() << " moons to planet " << i << std::endl;
            }
        }

        // After adding moons, update the planets vector to include all moons for gravity calculations
        for (auto& planet : orbitingPlanets) {
            auto moonPtrs = planet->getAllMoonPointers();
            for (auto* moon : moonPtrs) {
                planets.push_back(moon);
            }
        }

        // SATELLITE SYSTEM: Set planets for satellite manager FIRST
        satelliteManager->setPlanets(planets);


        // Create vehicle manager with satellite manager reference - spawn on green planet
        if (!orbitingPlanets.empty()) {
            sf::Vector2f planetPos = orbitingPlanets[0]->getPosition();
            float planetRadius = orbitingPlanets[0]->getRadius();
            float rocketSize = GameConstants::ROCKET_SIZE;
            sf::Vector2f direction(0, -1);
            sf::Vector2f rocketPos = planetPos + direction * (planetRadius + rocketSize);

            vehicleManager = std::make_unique<VehicleManager>(rocketPos, planets, satelliteManager.get());
            gameView.setCenter(rocketPos);
        }
        else {
            // Fallback to main planet if no orbiting planets
            sf::Vector2f planetPos = planet->getPosition();
            float planetRadius = planet->getRadius();
            float rocketSize = GameConstants::ROCKET_SIZE;
            sf::Vector2f direction(0, -1);
            sf::Vector2f rocketPos = planetPos + direction * (planetRadius + rocketSize);

            vehicleManager = std::make_unique<VehicleManager>(rocketPos, planets, satelliteManager.get());
            gameView.setCenter(rocketPos);
        }

        // FUEL SYSTEM: Set up fuel collection for the rocket
        if (vehicleManager->getRocket()) {
            vehicleManager->getRocket()->setNearbyPlanets(planets);
        }

        // Setup gravity simulator with satellite manager
        gravitySimulator = std::make_unique<GravitySimulator>();

        // Add all planets to gravity simulator
        for (const auto& planetPtr : planets) {
            gravitySimulator->addPlanet(planetPtr);
        }

        gravitySimulator->addVehicleManager(vehicleManager.get());
        gravitySimulator->addSatelliteManager(satelliteManager.get());

        // Clear players for single player mode
        players.clear();
        localPlayer.reset();
        remotePlayers.clear();

        // Reset camera
        zoomLevel = 1.0f;
        targetZoom = 1.0f;

        std::cout << "Single Player mode initialized with satellite system!" << std::endl;
        std::cout << "Use 'T' to convert rocket to satellite, '.' to collect fuel, ',' to give fuel" << std::endl;
    }

    void initializeLocalPCMultiplayer() {
        // Create planets using dynamic system
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

        // After creating orbital planets, add their moons
        for (size_t i = 0; i < orbitingPlanets.size(); i++) {
            const auto& config = planetConfigs[i];

            // Check if this planet has moons configured
            if (!config.moons.empty()) {
                // Create moons for this planet
                for (const auto& moonConfig : config.moons) {
                    auto moon = Planet::createMoon(
                        orbitingPlanets[i].get(),
                        moonConfig.orbitDistanceMultiplier,
                        moonConfig.massRatio,
                        moonConfig.angleOffset,
                        moonConfig.color
                    );

                    if (moon) {
                        // Add moon to its parent planet
                        orbitingPlanets[i]->addMoon(std::move(moon));
                    }
                }

                std::cout << "Added " << config.moons.size() << " moons to planet " << i << std::endl;
            }
        }

        // After adding moons, update the planets vector to include all moons for gravity calculations
        for (auto& planet : orbitingPlanets) {
            auto moonPtrs = planet->getAllMoonPointers();
            for (auto* moon : moonPtrs) {
                planets.push_back(moon);
            }
        }

        // SATELLITE SYSTEM: Set planets for satellite manager FIRST
        satelliteManager->setPlanets(planets);

        // Create spawn positions for both players - spawn on green planet
        sf::Vector2f player1Pos, player2Pos;
        if (!orbitingPlanets.empty()) {
            sf::Vector2f planetPos = orbitingPlanets[0]->getPosition();
            float planetRadius = orbitingPlanets[0]->getRadius();
            float rocketSize = GameConstants::ROCKET_SIZE;

            player1Pos = planetPos + sf::Vector2f(0, -1) * (planetRadius + rocketSize);
            player2Pos = planetPos + sf::Vector2f(0, 1) * (planetRadius + rocketSize);
        }
        else {
            // Fallback to main planet if no orbiting planets
            sf::Vector2f planetPos = planet->getPosition();
            float planetRadius = planet->getRadius();
            float rocketSize = GameConstants::ROCKET_SIZE;

            player1Pos = planetPos + sf::Vector2f(0, -1) * (planetRadius + rocketSize);
            player2Pos = planetPos + sf::Vector2f(0, 1) * (planetRadius + rocketSize);
        }

        // Create split screen manager with satellite manager reference
        splitScreenManager = std::make_unique<SplitScreenManager>(player1Pos, player2Pos, planets, satelliteManager.get());

        // Setup gravity simulator with satellite manager
        gravitySimulator = std::make_unique<GravitySimulator>();

        // Add all planets to gravity simulator
        for (const auto& planetPtr : planets) {
            gravitySimulator->addPlanet(planetPtr);
        }

        gravitySimulator->addSatelliteManager(satelliteManager.get());
        if (splitScreenManager) {
            gravitySimulator->addVehicleManager(splitScreenManager->getPlayer1());
            gravitySimulator->addVehicleManager(splitScreenManager->getPlayer2());
        }

        players.clear();
        localPlayer.reset();
        remotePlayers.clear();

        zoomLevel = 1.0f;
        targetZoom = 1.0f;
        if (splitScreenManager) {
            gameView.setCenter(splitScreenManager->getCenterPoint());
        }

        std::cout << "Local PC Split-Screen Multiplayer initialized with satellite system!" << std::endl;
        std::cout << "P1: T to convert to satellite, P2: Y to convert to satellite" << std::endl;
    }

    void initializeLANMultiplayer() {
        // Create planets using dynamic system
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

        // After creating orbital planets, add their moons
        for (size_t i = 0; i < orbitingPlanets.size(); i++) {
            const auto& config = planetConfigs[i];

            // Check if this planet has moons configured
            if (!config.moons.empty()) {
                // Create moons for this planet
                for (const auto& moonConfig : config.moons) {
                    auto moon = Planet::createMoon(
                        orbitingPlanets[i].get(),
                        moonConfig.orbitDistanceMultiplier,
                        moonConfig.massRatio,
                        moonConfig.angleOffset,
                        moonConfig.color
                    );

                    if (moon) {
                        // Add moon to its parent planet
                        orbitingPlanets[i]->addMoon(std::move(moon));
                    }
                }

                std::cout << "Added " << config.moons.size() << " moons to planet " << i << std::endl;
            }
        }

        // After adding moons, update the planets vector to include all moons for gravity calculations
        for (auto& planet : orbitingPlanets) {
            auto moonPtrs = planet->getAllMoonPointers();
            for (auto* moon : moonPtrs) {
                planets.push_back(moon);
            }
        }

        // SATELLITE SYSTEM: Set planets for satellite manager FIRST
        satelliteManager->setPlanets(planets);

        // Initialize network manager
        networkManager = std::make_unique<NetworkManager>();
        if (!networkManager->attemptAutoConnect()) {
            std::cout << "Failed to establish LAN connection" << std::endl;
            handleEscapeKey();
            return;
        }

        // Create local player with satellite manager reference - spawn on green planet
        sf::Vector2f spawnPos;
        if (!orbitingPlanets.empty()) {
            sf::Vector2f planetPos = orbitingPlanets[0]->getPosition();
            float planetRadius = orbitingPlanets[0]->getRadius();
            spawnPos = sf::Vector2f(planetPos.x, planetPos.y - planetRadius - 150.0f);
        }
        else {
            // Fallback to main planet
            spawnPos = sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y - 150.0f);
        }
        localPlayer = std::make_unique<Player>(networkManager->getLocalPlayerID(), spawnPos,
            PlayerType::LOCAL, planets, satelliteManager.get());

        // Setup gravity simulator with satellite manager
        gravitySimulator = std::make_unique<GravitySimulator>();

        // Add all planets to gravity simulator
        for (const auto& planetPtr : planets) {
            gravitySimulator->addPlanet(planetPtr);
        }

        gravitySimulator->addSatelliteManager(satelliteManager.get());
        gravitySimulator->addPlayer(localPlayer.get());

        vehicleManager.reset();
        splitScreenManager.reset();

        zoomLevel = 1.0f;
        targetZoom = 1.0f;
        gameView.setCenter(spawnPos);

        std::cout << "LAN Multiplayer initialized with satellite system!" << std::endl;
        std::cout << "Use 'T' to convert rocket to satellite" << std::endl;
    }

    void initializeOnlineMultiplayer(const ConnectionInfo& connectionInfo) {
        // Create planets using dynamic system
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

        // After creating orbital planets, add their moons
        for (size_t i = 0; i < orbitingPlanets.size(); i++) {
            const auto& config = planetConfigs[i];

            // Check if this planet has moons configured
            if (!config.moons.empty()) {
                // Create moons for this planet
                for (const auto& moonConfig : config.moons) {
                    auto moon = Planet::createMoon(
                        orbitingPlanets[i].get(),
                        moonConfig.orbitDistanceMultiplier,
                        moonConfig.massRatio,
                        moonConfig.angleOffset,
                        moonConfig.color
                    );

                    if (moon) {
                        // Add moon to its parent planet
                        orbitingPlanets[i]->addMoon(std::move(moon));
                    }
                }

                std::cout << "Added " << config.moons.size() << " moons to planet " << i << std::endl;
            }
        }

        // After adding moons, update the planets vector to include all moons for gravity calculations
        for (auto& planet : orbitingPlanets) {
            auto moonPtrs = planet->getAllMoonPointers();
            for (auto* moon : moonPtrs) {
                planets.push_back(moon);
            }
        }

        // SATELLITE SYSTEM: Set planets for satellite manager FIRST
        satelliteManager->setPlanets(planets);

        // Initialize network manager for online play
        networkManager = std::make_unique<NetworkManager>();
        // TODO: Implement online connection with connectionInfo
        if (!networkManager->attemptAutoConnect()) {
            std::cout << "Failed to establish online connection" << std::endl;
            handleEscapeKey();
            return;
        }

        // Create local player with satellite manager reference - spawn on green planet
        sf::Vector2f spawnPos;
        if (!orbitingPlanets.empty()) {
            sf::Vector2f planetPos = orbitingPlanets[0]->getPosition();
            float planetRadius = orbitingPlanets[0]->getRadius();
            spawnPos = sf::Vector2f(planetPos.x, planetPos.y - planetRadius - 150.0f);
        }
        else {
            // Fallback to main planet
            spawnPos = sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y - 150.0f);
        }
        localPlayer = std::make_unique<Player>(networkManager->getLocalPlayerID(), spawnPos,
            PlayerType::LOCAL, planets, satelliteManager.get());

        // Setup gravity simulator with satellite manager
        gravitySimulator = std::make_unique<GravitySimulator>();

        // Add all planets to gravity simulator
        for (const auto& planetPtr : planets) {
            gravitySimulator->addPlanet(planetPtr);
        }

        gravitySimulator->addSatelliteManager(satelliteManager.get());
        gravitySimulator->addPlayer(localPlayer.get());

        vehicleManager.reset();
        splitScreenManager.reset();

        zoomLevel = 1.0f;
        targetZoom = 1.0f;
        gameView.setCenter(spawnPos);

        std::cout << "Online Multiplayer initialized with satellite system!" << std::endl;
        std::cout << "Use 'T' to convert rocket to satellite" << std::endl;
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
        // Handle network multiplayer player events
        if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            localPlayer->handleSatelliteConversionInput(event);
        }


        if (auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
            switch (keyPressed->code) {
            case sf::Keyboard::Key::Escape:
                handleEscapeKey();
                return;
            case sf::Keyboard::Key::P:
                togglePlanetGravity();
                break;
            case sf::Keyboard::Key::L:
                if (!lKeyPressed) {
                    handleVehicleTransform();
                    lKeyPressed = true;
                }
                break;
            case sf::Keyboard::Key::T:
                if (!tKeyPressed) {
                    handleSatelliteConversion();
                    tKeyPressed = true;
                }
                break;
            case sf::Keyboard::Key::Period:
                if (!fuelIncreaseKeyPressed) {
                    handleFuelTransferStart(true);
                    fuelIncreaseKeyPressed = true;
                }
                break;
            case sf::Keyboard::Key::Comma:
                if (!fuelDecreaseKeyPressed) {
                    handleFuelTransferStart(false);
                    fuelDecreaseKeyPressed = true;
                }
                break;
            case sf::Keyboard::Key::H:
                if (networkManager) {
                    networkManager->sendHello();
                    std::cout << "Sent hello message to network!" << std::endl;
                }
                break;
            case sf::Keyboard::Key::F1:
                uiManager->toggleUI();
                break;
            case sf::Keyboard::Key::F2:
                uiManager->toggleDebugInfo();
                break;
            case sf::Keyboard::Key::F3:
                satelliteManager->printNetworkStatus();
                break;
            case sf::Keyboard::Key::F4:
                satelliteManager->optimizeNetworkFuelDistribution();
                std::cout << "Triggered satellite fuel optimization" << std::endl;
                break;
            default:
                break;
            }
        }

        if (auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
            switch (keyReleased->code) {
            case sf::Keyboard::Key::L:
                lKeyPressed = false;
                break;
            case sf::Keyboard::Key::T:
                tKeyPressed = false;
                break;
            case sf::Keyboard::Key::Period:
                handleFuelTransferStop();
                fuelIncreaseKeyPressed = false;
                break;
            case sf::Keyboard::Key::Comma:
                handleFuelTransferStop();
                fuelDecreaseKeyPressed = false;
                break;
            default:
                break;
            }
        }

        if (auto* mouseWheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
            handleMouseWheelZoom(event);
        }

        // Handle window resize - FIXED for SFML 3.0
        if (auto* resized = event.getIf<sf::Event::Resized>()) {
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

        // Draw orbit paths for all planets
        for (auto& planetPtr : planets) {
            // Skip drawing orbit path for the main stationary planet (it doesn't orbit)
            if (planetPtr != planet.get()) {
                planetPtr->drawOrbitPath(window, planets);
            }
        }

        // Draw trajectory for rockets
        drawTrajectories();

        // Draw game objects - planets automatically draw fuel collection rings
        planet->draw(window);

        // Draw all orbiting planets
        for (const auto& orbitingPlanet : orbitingPlanets) {
            orbitingPlanet->draw(window);
        }

        // SATELLITE SYSTEM: Draw satellites
        satelliteManager->drawWithConstantSize(window, zoomLevel);

        // Draw vehicles and fuel collection indicators
        drawVehiclesAndFuelLines();

        // Draw velocity vectors
        drawVelocityVectors();

        // Draw UI
        uiManager->draw(window, currentState, networkManager.get(), satelliteManager.get());
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

        // Draw velocity vectors for all orbiting planets
        for (const auto& orbitingPlanet : orbitingPlanets) {
            orbitingPlanet->drawVelocityVector(window, 5.0f);
        }

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

        std::cout << "DEBUG: Attempting satellite conversion..." << std::endl;

        if (currentState == GameState::SINGLE_PLAYER && vehicleManager &&
            vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {

            // Single player satellite conversion (unchanged logic)
            Rocket* rocket = vehicleManager->getRocket();
            if (!rocket) {
                std::cout << "ERROR: Rocket is null!" << std::endl;
                return;
            }

            if (satelliteManager && satelliteManager->canConvertRocketToSatellite(rocket)) {
                try {
                    SatelliteConversionConfig config = satelliteManager->getOptimalConversionConfig(rocket);
                    int satelliteID = satelliteManager->createSatelliteFromRocket(rocket, config);

                    if (satelliteID >= 0) {
                        sf::Vector2f newRocketPos = findNearestPlanetSurface(rocket->getPosition());
                        vehicleManager = std::make_unique<VehicleManager>(newRocketPos, planets, satelliteManager.get());

                        if (vehicleManager && vehicleManager->getRocket()) {
                            vehicleManager->getRocket()->setNearbyPlanets(planets);
                        }

                        if (gravitySimulator && vehicleManager) {
                            gravitySimulator->addVehicleManager(vehicleManager.get());
                        }

                        gameView.setCenter(newRocketPos);
                        std::cout << "Single player satellite conversion completed" << std::endl;
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Exception during satellite conversion: " << e.what() << std::endl;
                }
            }
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) &&
            localPlayer && networkManager) {

            // Network multiplayer satellite conversion
            if (localPlayer->canConvertToSatellite()) {
                try {
                    Rocket* rocket = localPlayer->getVehicleManager()->getRocket();
                    if (!rocket) return;

                    // Prepare conversion info for network
                    SatelliteConversionInfo conversionInfo;
                    conversionInfo.playerID = localPlayer->getID();
                    conversionInfo.satelliteID = satelliteManager->getSatelliteCount() + 1; // Estimated ID
                    conversionInfo.satellitePosition = rocket->getPosition();
                    conversionInfo.satelliteVelocity = rocket->getVelocity();
                    conversionInfo.satelliteFuel = rocket->getCurrentFuel() * GameConstants::SATELLITE_CONVERSION_FUEL_RETENTION;
                    conversionInfo.newRocketPosition = localPlayer->findNearestPlanetSurface();
                    conversionInfo.satelliteName = localPlayer->getName() + "-SAT-" + std::to_string(conversionInfo.satelliteID);

                    // Send conversion to network first
                    if (networkManager->sendSatelliteConversion(conversionInfo)) {
                        // Perform local conversion
                        localPlayer->convertRocketToSatellite();
                        std::cout << "Network satellite conversion sent and executed locally" << std::endl;
                    }
                    else {
                        std::cout << "Failed to send satellite conversion to network" << std::endl;
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Exception during network satellite conversion: " << e.what() << std::endl;
                }
            }
            else {
                std::cout << "Cannot convert rocket to satellite - check requirements" << std::endl;
            }
        }
        else {
            std::cout << "Satellite conversion not available in current mode/state" << std::endl;
        }
    }

    void handleNetworkSatelliteConversions() {
        if (!networkManager || !networkManager->hasPendingSatelliteConversion()) return;

        try {
            SatelliteConversionInfo conversionInfo = networkManager->getPendingSatelliteConversion();

            std::cout << "Processing network satellite conversion from Player " << conversionInfo.playerID << std::endl;

            // Create satellite from network data
            SatelliteConversionConfig config;
            config.customName = conversionInfo.satelliteName;

            int satelliteID = satelliteManager->createSatellite(
                conversionInfo.satellitePosition,
                conversionInfo.satelliteVelocity,
                config
            );

            if (satelliteID >= 0) {
                // Set the fuel level for the networked satellite
                Satellite* satellite = satelliteManager->getSatellite(satelliteID);
                if (satellite) {
                    satellite->setFuel(conversionInfo.satelliteFuel);
                }

                // If this conversion involved the local player, handle rocket respawn
                if (localPlayer && localPlayer->getID() == conversionInfo.playerID) {
                    // Create new rocket at the specified spawn position
                    localPlayer->respawnAtPosition(conversionInfo.newRocketPosition);
                }
                // If it's a remote player, create/update their rocket position
                else {
                    // Find or create remote player
                    Player* remotePlayer = nullptr;
                    for (auto& player : remotePlayers) {
                        if (player->getID() == conversionInfo.playerID) {
                            remotePlayer = player.get();
                            break;
                        }
                    }

                    if (remotePlayer) {
                        remotePlayer->respawnAtPosition(conversionInfo.newRocketPosition);
                    }
                }

                std::cout << "Network satellite conversion completed - ID: " << satelliteID << std::endl;
            }
            else {
                std::cout << "Failed to create satellite from network conversion" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception during network satellite conversion: " << e.what() << std::endl;
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
            else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) &&
                localPlayer && localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* rocket = localPlayer->getVehicleManager()->getRocket();
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
            else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) &&
                localPlayer && localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* rocket = localPlayer->getVehicleManager()->getRocket();
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
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) &&
            localPlayer && localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
            localPlayer->getVehicleManager()->getRocket()->stopFuelTransfer();
        }
    }

    void handleMouseWheelZoom(const sf::Event& event) {
        if (auto* scrollEvent = event.getIf<sf::Event::MouseWheelScrolled>()) {
            if (scrollEvent->wheel == sf::Mouse::Wheel::Vertical) {
                const float zoomFactor = 1.1f;
                if (scrollEvent->delta > 0) {
                    targetZoom /= zoomFactor;
                }
                else {
                    targetZoom *= zoomFactor;
                }
                targetZoom = std::max(0.0005f, std::min(targetZoom, 500.0f));

                updateCameraCenter();
            }
        }
    }


    void handleWindowResize(const sf::Event& event) {
        if (auto* resizeEvent = event.getIf<sf::Event::Resized>()) {
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

    void handleMultiplayerSatelliteInput() {
        // Handle satellite conversion input for network multiplayer
        if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            // The T key is already handled in handleGameEvents, but we could add additional
            // satellite-specific input handling here if needed
        }
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
                float dist1 = std::sqrt(direction1.x * direction1.x + direction1.y * direction1.y);

                float minDist = dist1;
                for (const auto& orbitingPlanet : orbitingPlanets) {
                    sf::Vector2f direction = vehiclePos - orbitingPlanet->getPosition();
                    float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (dist < minDist) {
                        minDist = dist;
                    }
                }

                targetZoom = minZoom + (minDist - (planet->getRadius() + GameConstants::ROCKET_SIZE)) / 100.0f;
                targetZoom = std::max(minZoom, std::min(targetZoom, maxZoom));
                gameView.setCenter(vehiclePos);
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
                targetZoom = 10.0f;
                // Focus on first orbiting planet if available, otherwise main planet
                if (!orbitingPlanets.empty()) {
                    gameView.setCenter(orbitingPlanets[0]->getPosition());
                }
                else {
                    gameView.setCenter(planet->getPosition());
                }
            }
        }
    }

    void updateGame(float deltaTime) {
        // Update network manager if active
        if (networkManager) {
            networkManager->update(deltaTime);
        }
        // Handle network satellite conversions
        if (networkManager) {
            handleNetworkSatelliteConversions();
        }

        // Update simulation
        gravitySimulator->update(deltaTime);
        planet->update(deltaTime);

        // Update all orbiting planets
        for (const auto& orbitingPlanet : orbitingPlanets) {
            orbitingPlanet->update(deltaTime);
        }

        // SATELLITE SYSTEM: Update satellites
        satelliteManager->update(deltaTime);

        // SATELLITE SYSTEM: Update rocket references for single player
        if (currentState == GameState::SINGLE_PLAYER && vehicleManager) {
            satelliteManager->setNearbyRockets({ vehicleManager->getRocket() });
        }
        // SATELLITE SYSTEM: Update rocket references for split screen
        else if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            std::vector<Rocket*> activeRockets;
            if (splitScreenManager->getPlayer1()->getActiveVehicleType() == VehicleType::ROCKET) {
                activeRockets.push_back(splitScreenManager->getPlayer1()->getRocket());
            }
            if (splitScreenManager->getPlayer2()->getActiveVehicleType() == VehicleType::ROCKET) {
                activeRockets.push_back(splitScreenManager->getPlayer2()->getRocket());
            }
            satelliteManager->setNearbyRockets(activeRockets);
        }
        // SATELLITE SYSTEM: Update rocket references for network multiplayer
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER)) {
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
            localPlayer.get(), planets, networkManager.get(), satelliteManager.get());
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

            // Handle events - CORRECTED for SFML 3.0
            for (auto event = window.pollEvent(); event.has_value(); event = window.pollEvent()) {
                // Handle window close - CORRECTED
                if (event->is<sf::Event::Closed>()) {
                    if (networkManager) {
                        networkManager->disconnect();
                    }
                    window.close();
                    continue;
                }

                // Route events based on game state
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