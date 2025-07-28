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
#include "SplitScreenManager.h"
#include "NetworkManager.h"
#include "Player.h"
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

// Define a simple TextPanel class to display information
class TextPanel {
private:
    sf::Text text;
    sf::RectangleShape background;

public:
    TextPanel(const sf::Font& font, unsigned int characterSize, sf::Vector2f position,
        sf::Vector2f size, sf::Color bgColor = sf::Color(0, 0, 0, 180))
        : text(font) {
        background.setPosition(position);
        background.setSize(size);
        background.setFillColor(bgColor);
        background.setOutlineColor(sf::Color::White);
        background.setOutlineThickness(1.0f);

        text.setCharacterSize(characterSize);
        text.setFillColor(sf::Color::White);
        text.setPosition(sf::Vector2f(position.x + 5.f, position.y + 5.f));
    }

    void setText(const std::string& str) {
        text.setString(str);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(background);
        window.draw(text);
    }
};

// Forward declarations for helper functions
float calculateApoapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G);
float calculatePeriapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G);

// Game state management
enum class GameState {
    MAIN_MENU,
    MULTIPLAYER_MENU,
    SINGLE_PLAYER,
    LOCAL_PC_MULTIPLAYER,
    LAN_MULTIPLAYER,
    ONLINE_MULTIPLAYER,
    QUIT
};

// Game class to encapsulate game logic
class Game {
private:
    sf::RenderWindow window;
    MainMenu mainMenu;
    std::unique_ptr<MultiplayerMenu> multiplayerMenu;
    GameState currentState;
    sf::Font font;
    bool fontLoaded;

    // Network manager for LAN multiplayer
    std::unique_ptr<NetworkManager> networkManager;

    // Game objects (will be initialized when entering game modes)
    std::unique_ptr<Planet> planet;
    std::unique_ptr<Planet> planet2;
    std::vector<Planet*> planets;
    std::unique_ptr<VehicleManager> vehicleManager;
    std::unique_ptr<SplitScreenManager> splitScreenManager;
    std::unique_ptr<GravitySimulator> gravitySimulator;

    // Player management for network multiplayer
    std::vector<std::unique_ptr<Player>> players;
    std::unique_ptr<Player> localPlayer;
    std::vector<std::unique_ptr<Player>> remotePlayers;

    // Camera and UI
    sf::View gameView;
    sf::View uiView;
    float zoomLevel;
    float targetZoom;

    // UI panels
    std::unique_ptr<TextPanel> rocketInfoPanel;
    std::unique_ptr<TextPanel> planetInfoPanel;
    std::unique_ptr<TextPanel> orbitInfoPanel;
    std::unique_ptr<TextPanel> controlsPanel;
    std::unique_ptr<TextPanel> networkInfoPanel;

    // Input tracking
    bool lKeyPressed;

public:
    Game() : window(sf::VideoMode({ 1280, 720 }), "Katie's Space Program"),
        mainMenu(sf::Vector2u(1280, 720)),
        multiplayerMenu(std::make_unique<MultiplayerMenu>(sf::Vector2u(1280, 720))),
        currentState(GameState::MAIN_MENU),
        gameView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
        uiView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
        zoomLevel(1.0f),
        targetZoom(1.0f),
        lKeyPressed(false),
        fontLoaded(false) {

        loadFont();
        setupUI();
    }

    void loadFont() {
#ifdef _WIN32
        if (font.openFromFile("arial.ttf") ||
            font.openFromFile("C:/Windows/Fonts/arial.ttf") ||
            font.openFromFile("C:/Windows/Fonts/Arial.ttf")) {
            fontLoaded = true;
        }
#elif defined(__APPLE__)
        if (font.openFromFile("arial.ttf") ||
            font.openFromFile("/Library/Fonts/Arial.ttf") ||
            font.openFromFile("/System/Library/Fonts/Arial.ttf")) {
            fontLoaded = true;
        }
#elif defined(__linux__)
        if (font.openFromFile("arial.ttf") ||
            font.openFromFile("/usr/share/fonts/truetype/msttcorefonts/Arial.ttf") ||
            font.openFromFile("/usr/share/fonts/TTF/arial.ttf")) {
            fontLoaded = true;
        }
#else
        fontLoaded = font.openFromFile("arial.ttf");
#endif

        if (!fontLoaded) {
            std::cerr << "Warning: Could not load font file. Text won't display correctly." << std::endl;
        }
    }

    void setupUI() {
        if (fontLoaded) {
            rocketInfoPanel = std::make_unique<TextPanel>(font, 12, sf::Vector2f(10, 10), sf::Vector2f(250, 150));
            planetInfoPanel = std::make_unique<TextPanel>(font, 12, sf::Vector2f(10, 170), sf::Vector2f(250, 120));
            orbitInfoPanel = std::make_unique<TextPanel>(font, 12, sf::Vector2f(10, 300), sf::Vector2f(250, 100));
            controlsPanel = std::make_unique<TextPanel>(font, 12, sf::Vector2f(10, 410), sf::Vector2f(250, 120));
            networkInfoPanel = std::make_unique<TextPanel>(font, 12, sf::Vector2f(10, 540), sf::Vector2f(250, 80));

            controlsPanel->setText(
                "CONTROLS:\n"
                "Single: Arrows + 1-9 + L\n"
                "Split: P1(Arrows+L) P2(WASD+K)\n"
                "Network: P1(Arrows) P2(WASD)\n"
                "Shared: Mouse wheel zoom, ESC menu"
            );
        }
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

        // Create vehicle manager
        sf::Vector2f planetPos = planet->getPosition();
        float planetRadius = planet->getRadius();
        float rocketSize = GameConstants::ROCKET_SIZE;
        sf::Vector2f direction(0, -1);
        sf::Vector2f rocketPos = planetPos + direction * (planetRadius + rocketSize);

        vehicleManager = std::make_unique<VehicleManager>(rocketPos, planets);

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

        std::cout << "Single Player mode initialized!" << std::endl;
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

        // Setup planet vector
        planets.clear();
        planets.push_back(planet.get());
        planets.push_back(planet2.get());

        // Create spawn positions for both players
        sf::Vector2f planetPos = planet->getPosition();
        float planetRadius = planet->getRadius();
        float rocketSize = GameConstants::ROCKET_SIZE;

        // Player 1: North side of planet
        sf::Vector2f player1Pos = planetPos + sf::Vector2f(0, -1) * (planetRadius + rocketSize);

        // Player 2: South side of planet
        sf::Vector2f player2Pos = planetPos + sf::Vector2f(0, 1) * (planetRadius + rocketSize);

        // Create split screen manager
        splitScreenManager = std::make_unique<SplitScreenManager>(player1Pos, player2Pos, planets);

        // Setup gravity simulator for split screen
        gravitySimulator = std::make_unique<GravitySimulator>();
        gravitySimulator->addPlanet(planet.get());
        gravitySimulator->addPlanet(planet2.get());
        gravitySimulator->addVehicleManager(splitScreenManager->getPlayer1());
        gravitySimulator->addVehicleManager(splitScreenManager->getPlayer2());

        // Clear players for split-screen mode
        players.clear();
        localPlayer.reset();
        remotePlayers.clear();

        // Reset camera to center between players
        zoomLevel = 1.0f;
        targetZoom = 1.0f;
        gameView.setCenter(splitScreenManager->getCenterPoint());

        std::cout << "Local PC Split-Screen Multiplayer initialized!" << std::endl;
        std::cout << "Player 1: Arrow Keys + L to transform" << std::endl;
        std::cout << "Player 2: WASD + K to transform" << std::endl;
        std::cout << "Both: Numbers 1-9,0,= for shared thrust, Mouse wheel to zoom" << std::endl;
    }

    void initializeLANMultiplayer() {
        // Initialize network manager for LAN
        networkManager = std::make_unique<NetworkManager>();

        std::cout << "Attempting to start LAN multiplayer..." << std::endl;

        if (networkManager->attemptAutoConnect()) {
            std::cout << "Network connection successful!" << std::endl;

            // Create planets (same setup as other modes)
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

            // Generate spawn positions for network players
            sf::Vector2f planetPos = planet->getPosition();
            float planetRadius = planet->getRadius();
            std::vector<sf::Vector2f> spawnPositions = networkManager->generateSpawnPositions(
                planetPos, planetRadius, 4);  // Support up to 4 players

            // Proper player ID assignment
            int localPlayerID = -1;
            if (networkManager->getRole() == NetworkRole::HOST) {
                localPlayerID = 0;  // Host is always Player 0
            }
            else if (networkManager->getRole() == NetworkRole::CLIENT) {
                localPlayerID = 1;  // First client is Player 1
            }

            // Create local player with correct ID and spawn position
            if (localPlayerID >= 0 && localPlayerID < static_cast<int>(spawnPositions.size())) {
                localPlayer = std::make_unique<Player>(
                    localPlayerID,
                    spawnPositions[localPlayerID],
                    PlayerType::LOCAL,
                    planets
                );

                std::cout << "Created local player with ID: " << localPlayerID
                    << " at position (" << spawnPositions[localPlayerID].x
                    << ", " << spawnPositions[localPlayerID].y << ")" << std::endl;
            }

            // Create the remote player immediately for both host and client
            if (networkManager->getRole() == NetworkRole::HOST) {
                // Host creates a placeholder for Player 1 (client)
                auto remotePlayer = std::make_unique<Player>(
                    1,  // Client will be Player 1
                    spawnPositions[1],
                    PlayerType::REMOTE,
                    planets
                );
                remotePlayer->setName("Player 2");
                remotePlayers.push_back(std::move(remotePlayer));
                std::cout << "Host created placeholder for Player 2" << std::endl;
            }
            else if (networkManager->getRole() == NetworkRole::CLIENT) {
                // Client creates a placeholder for Player 0 (host)
                auto remotePlayer = std::make_unique<Player>(
                    0,  // Host is Player 0
                    spawnPositions[0],
                    PlayerType::REMOTE,
                    planets
                );
                remotePlayer->setName("Player 1");
                remotePlayers.push_back(std::move(remotePlayer));
                std::cout << "Client created placeholder for Player 1" << std::endl;
            }

            // Clear old game objects for network mode
            vehicleManager.reset();
            splitScreenManager.reset();
            players.clear();

            // Setup gravity simulator with Player system
            gravitySimulator = std::make_unique<GravitySimulator>();
            gravitySimulator->addPlanet(planet.get());
            gravitySimulator->addPlanet(planet2.get());
            if (localPlayer) {
                gravitySimulator->addPlayer(localPlayer.get());
            }

            // Add remote players to gravity simulator
            for (auto& remotePlayer : remotePlayers) {
                gravitySimulator->addPlayer(remotePlayer.get());
            }

            // Reset camera to local player's position
            zoomLevel = 1.0f;
            targetZoom = 1.0f;
            if (localPlayer) {
                gameView.setCenter(localPlayer->getPosition());
            }

            if (networkManager->getRole() == NetworkRole::HOST) {
                std::cout << "You are the HOST (Player 1). Use Arrow Keys." << std::endl;
            }
            else if (networkManager->getRole() == NetworkRole::CLIENT) {
                std::cout << "You are CLIENT (Player 2). Use WASD Keys." << std::endl;
            }
        }
        else {
            std::cout << "Failed to establish network connection. Falling back to single player." << std::endl;
            initializeSinglePlayer();
        }
    }

    void initializeOnlineMultiplayer() {
        // Placeholder for online multiplayer - same as LAN for now
        std::cout << "Online Multiplayer not yet implemented. Starting LAN mode instead." << std::endl;
        initializeLANMultiplayer();
    }

    void handleRemotePlayerJoin(int playerID, sf::Vector2f spawnPos) {
        std::cout << "Handling remote player join for ID: " << playerID << std::endl;

        // Check if we already have this remote player (from initialization)
        for (auto& remotePlayer : remotePlayers) {
            if (remotePlayer->getID() == playerID) {
                std::cout << "Remote player " << playerID << " already exists, updating position" << std::endl;
                remotePlayer->respawnAtPosition(spawnPos);
                return;
            }
        }

        // If we get here, this is a truly new player (shouldn't happen in 2-player setup)
        auto remotePlayer = std::make_unique<Player>(
            playerID,
            spawnPos,
            PlayerType::REMOTE,
            planets
        );

        std::cout << "Created new remote player with ID: " << playerID << std::endl;

        // Add to gravity simulator
        if (gravitySimulator) {
            gravitySimulator->addPlayer(remotePlayer.get());
        }

        remotePlayers.push_back(std::move(remotePlayer));
        std::cout << "Total remote players: " << remotePlayers.size() << std::endl;
    }

    void handleMenuEvents(const sf::Event& event) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), uiView);

        if (currentState == GameState::MAIN_MENU) {
            mainMenu.handleEvent(event, mousePos);

            // Check if menu selection was made
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
                    currentState = GameState::ONLINE_MULTIPLAYER;
                    initializeOnlineMultiplayer();
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
                    // Clean up network connection before returning to menu
                    if (networkManager) {
                        networkManager->disconnect();
                        networkManager.reset();
                    }

                    // Clean up players
                    remotePlayers.clear();
                    players.clear();
                    localPlayer.reset();

                    currentState = GameState::MAIN_MENU;
                    mainMenu.show();
                    return;
                }
                else if (keyEvent->code == sf::Keyboard::Key::P) {
                    static bool planetGravity = true;
                    planetGravity = !planetGravity;
                    gravitySimulator->setSimulatePlanetGravity(planetGravity);
                }
                else if (keyEvent->code == sf::Keyboard::Key::L && !lKeyPressed) {
                    lKeyPressed = true;
                    if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
                        // Handle in split screen manager
                    }
                    else if (currentState == GameState::LAN_MULTIPLAYER && localPlayer) {
                        // Handle player transform
                        localPlayer->requestTransform();
                    }
                    else if (vehicleManager) {
                        vehicleManager->switchVehicle();
                    }
                }
                // Network test controls
                else if (keyEvent->code == sf::Keyboard::Key::H && networkManager) {
                    networkManager->sendHello();
                    std::cout << "Sent hello message to network!" << std::endl;
                }
            }
        }

        if (event.is<sf::Event::KeyReleased>()) {
            const auto* keyEvent = event.getIf<sf::Event::KeyReleased>();
            if (keyEvent && keyEvent->code == sf::Keyboard::Key::L) {
                lKeyPressed = false;
            }
        }

        // Handle mouse wheel for zooming
        if (event.is<sf::Event::MouseWheelScrolled>()) {
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

                // Set camera center based on current mode
                if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
                    gameView.setCenter(splitScreenManager->getCenterPoint());
                }
                else if (currentState == GameState::LAN_MULTIPLAYER && localPlayer) {
                    gameView.setCenter(localPlayer->getPosition());
                }
                else if (vehicleManager) {
                    gameView.setCenter(vehicleManager->getActiveVehicle()->getPosition());
                }
            }
        }

        if (event.is<sf::Event::Resized>()) {
            const auto* resizeEvent = event.getIf<sf::Event::Resized>();
            if (resizeEvent) {
                float aspectRatio = static_cast<float>(resizeEvent->size.x) / static_cast<float>(resizeEvent->size.y);
                gameView.setSize(sf::Vector2f(
                    resizeEvent->size.y * aspectRatio * zoomLevel,
                    resizeEvent->size.y * zoomLevel
                ));

                uiView.setSize(sf::Vector2f(
                    static_cast<float>(resizeEvent->size.x),
                    static_cast<float>(resizeEvent->size.y)
                ));
                uiView.setCenter(sf::Vector2f(
                    static_cast<float>(resizeEvent->size.x) / 2.0f,
                    static_cast<float>(resizeEvent->size.y) / 2.0f
                ));

                window.setView(gameView);
            }
        }
    }

    void handleGameInput(float deltaTime) {
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            // Handle split-screen inputs
            splitScreenManager->handlePlayer1Input(deltaTime);
            splitScreenManager->handlePlayer2Input(deltaTime);

            // Synced thrust level controls
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0))
                splitScreenManager->setSyncedThrustLevel(0.0f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1))
                splitScreenManager->setSyncedThrustLevel(0.1f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2))
                splitScreenManager->setSyncedThrustLevel(0.2f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num3))
                splitScreenManager->setSyncedThrustLevel(0.3f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num4))
                splitScreenManager->setSyncedThrustLevel(0.4f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num5))
                splitScreenManager->setSyncedThrustLevel(0.5f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num6))
                splitScreenManager->setSyncedThrustLevel(0.6f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num7))
                splitScreenManager->setSyncedThrustLevel(0.7f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num8))
                splitScreenManager->setSyncedThrustLevel(0.8f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num9))
                splitScreenManager->setSyncedThrustLevel(0.9f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Equal))
                splitScreenManager->setSyncedThrustLevel(1.0f);
        }
        else if (currentState == GameState::LAN_MULTIPLAYER && localPlayer) {
            // STATE SYNC: Only handle LOCAL input - no networking here!
            localPlayer->handleLocalInput(deltaTime);
        }
        else if (vehicleManager) {
            // Single player input handling
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0))
                vehicleManager->getRocket()->setThrustLevel(0.0f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1))
                vehicleManager->getRocket()->setThrustLevel(0.1f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2))
                vehicleManager->getRocket()->setThrustLevel(0.2f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num3))
                vehicleManager->getRocket()->setThrustLevel(0.3f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num4))
                vehicleManager->getRocket()->setThrustLevel(0.4f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num5))
                vehicleManager->getRocket()->setThrustLevel(0.5f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num6))
                vehicleManager->getRocket()->setThrustLevel(0.6f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num7))
                vehicleManager->getRocket()->setThrustLevel(0.7f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num8))
                vehicleManager->getRocket()->setThrustLevel(0.8f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num9))
                vehicleManager->getRocket()->setThrustLevel(0.9f);
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Equal))
                vehicleManager->getRocket()->setThrustLevel(1.0f);

            // Movement controls
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
                vehicleManager->applyThrust(1.0f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
                vehicleManager->applyThrust(-0.5f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
                vehicleManager->rotate(-6.0f * deltaTime * 60.0f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
                vehicleManager->rotate(6.0f * deltaTime * 60.0f);

            handleCameraControls();
        }
    }

    void handleCameraControls() {
        // Single player camera controls only
        if (currentState == GameState::SINGLE_PLAYER && vehicleManager) {
            const float minZoom = 1.0f;
            const float maxZoom = 1000.0f;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z)) {
                targetZoom = std::min(maxZoom, targetZoom * 1.05f);
                gameView.setCenter(vehicleManager->getActiveVehicle()->getPosition());
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X)) {
                sf::Vector2f vehiclePos = vehicleManager->getActiveVehicle()->getPosition();
                float dist1 = std::sqrt(
                    std::pow(vehiclePos.x - planet->getPosition().x, 2) +
                    std::pow(vehiclePos.y - planet->getPosition().y, 2)
                );
                float dist2 = std::sqrt(
                    std::pow(vehiclePos.x - planet2->getPosition().x, 2) +
                    std::pow(vehiclePos.y - planet2->getPosition().y, 2)
                );
                targetZoom = minZoom + (std::min(dist1, dist2) - (planet->getRadius() + GameConstants::ROCKET_SIZE)) / 100.0f;
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

            // Handle new remote players joining
            if (networkManager->hasNewPlayer()) {
                PlayerSpawnInfo newPlayerInfo = networkManager->getNewPlayerInfo();

                std::cout << "Received spawn info for player " << newPlayerInfo.playerID
                    << " (local player is " << networkManager->getLocalPlayerID() << ")" << std::endl;

                // Only create remote player if it's not our local player
                if (newPlayerInfo.playerID != networkManager->getLocalPlayerID()) {
                    handleRemotePlayerJoin(newPlayerInfo.playerID, newPlayerInfo.spawnPosition);
                }
            }

            // STATE SYNC: Send local player state if needed
            if (localPlayer && localPlayer->shouldSendState()) {
                PlayerState currentState = localPlayer->getState();
                if (networkManager->sendPlayerState(currentState)) {
                    localPlayer->markStateSent();

                    static int sentCount = 0;
                    sentCount++;
                    if (sentCount % 30 == 0) {  // Debug every ~1 second at 30 FPS
                        std::cout << "Sent state for player " << localPlayer->getID() << std::endl;
                    }
                }
            }

            // STATE SYNC: Receive remote player states
            static int receivedCount = 0;
            for (auto& remotePlayer : remotePlayers) {
                PlayerState remoteState;
                if (networkManager->receivePlayerState(remotePlayer->getID(), remoteState)) {
                    remotePlayer->applyState(remoteState);
                    receivedCount++;
                    if (receivedCount % 30 == 0) {  // Debug every ~1 second at 30 FPS
                        std::cout << "Received state from player " << remotePlayer->getID() << std::endl;
                    }
                }
            }
        }

        // Update simulation
        gravitySimulator->update(deltaTime);
        planet->update(deltaTime);
        planet2->update(deltaTime);

        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            splitScreenManager->update(deltaTime);

            // Auto-center camera between both players
            if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
                gameView.setCenter(splitScreenManager->getCenterPoint());
            }
        }
        else if (currentState == GameState::LAN_MULTIPLAYER && localPlayer) {
            localPlayer->update(deltaTime);

            // Update all remote players
            for (auto& remotePlayer : remotePlayers) {
                remotePlayer->update(deltaTime);
            }

            // Auto-center camera on local player
            if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
                gameView.setCenter(localPlayer->getPosition());
            }
        }
        else if (vehicleManager) {
            vehicleManager->update(deltaTime);

            // Auto-zoom logic for single player
            if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z) &&
                !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X) &&
                !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {

                sf::Vector2f vehiclePos = vehicleManager->getActiveVehicle()->getPosition();
                sf::Vector2f vehicleToPlanet = planet->getPosition() - vehiclePos;
                sf::Vector2f vehicleToPlanet2 = planet2->getPosition() - vehiclePos;
                float distance1 = std::sqrt(vehicleToPlanet.x * vehicleToPlanet.x + vehicleToPlanet.y * vehicleToPlanet.y);
                float distance2 = std::sqrt(vehicleToPlanet2.x * vehicleToPlanet2.x + vehicleToPlanet2.y * vehicleToPlanet2.y);

                float closest = std::min(distance1, distance2);
                targetZoom = 1.0f + (closest - (planet->getRadius() + GameConstants::ROCKET_SIZE)) / 100.0f;
                targetZoom = std::max(1.0f, std::min(targetZoom, 1000.0f));

                gameView.setCenter(vehiclePos);
            }
        }

        // Smooth zoom
        zoomLevel += (targetZoom - zoomLevel) * deltaTime * 2.0f;
        gameView.setSize(sf::Vector2f(1280.f * zoomLevel, 720.f * zoomLevel));
    }

    void updateInfoPanels() {
        if (!fontLoaded) return;

        updateVehicleInfoPanel();
        updatePlanetInfoPanel();
        updateOrbitInfoPanel();
        updateNetworkInfoPanel();
    }

    void updateNetworkInfoPanel() {
        if (!networkInfoPanel) return;

        std::stringstream ss;
        if (networkManager) {
            std::string roleStr = "NONE";
            std::string statusStr = "DISCONNECTED";

            switch (networkManager->getRole()) {
            case NetworkRole::HOST: roleStr = "HOST"; break;
            case NetworkRole::CLIENT: roleStr = "CLIENT"; break;
            case NetworkRole::NONE: roleStr = "NONE"; break;
            }

            switch (networkManager->getStatus()) {
            case ConnectionStatus::CONNECTED: statusStr = "CONNECTED"; break;
            case ConnectionStatus::CONNECTING: statusStr = "CONNECTING"; break;
            case ConnectionStatus::FAILED: statusStr = "FAILED"; break;
            case ConnectionStatus::DISCONNECTED: statusStr = "DISCONNECTED"; break;
            }

            ss << "NETWORK STATUS (STATE SYNC)\n"
                << "Role: " << roleStr << "\n"
                << "Status: " << statusStr << "\n";

            if (localPlayer) {
                ss << "Player ID: " << localPlayer->getID() << "\n";
            }

            ss << "Each player controls own rocket";
        }
        else {
            ss << "NETWORK STATUS\n"
                << "Network not active\n"
                << "Use LAN multiplayer to connect";
        }

        networkInfoPanel->setText(ss.str());
    }

    void updateVehicleInfoPanel() {
        std::stringstream ss;

        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            VehicleManager* p1 = splitScreenManager->getPlayer1();
            VehicleManager* p2 = splitScreenManager->getPlayer2();

            ss << "SPLIT-SCREEN MODE\n";
            ss << "Player 1 (Arrows + L): " << (p1->getActiveVehicleType() == VehicleType::ROCKET ? "ROCKET" : "CAR") << "\n";
            ss << "Player 2 (WASD + K): " << (p2->getActiveVehicleType() == VehicleType::ROCKET ? "ROCKET" : "CAR") << "\n";

            if (p1->getActiveVehicleType() == VehicleType::ROCKET) {
                float thrustLevel = p1->getRocket()->getThrustLevel();
                ss << "Shared Thrust: " << std::fixed << std::setprecision(0) << thrustLevel * 100.0f << "%\n";
            }
            ss << "Mouse wheel: Zoom in/out";
        }
        else if (currentState == GameState::LAN_MULTIPLAYER && localPlayer) {
            if (localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* rocket = localPlayer->getVehicleManager()->getRocket();
                float speed = std::sqrt(rocket->getVelocity().x * rocket->getVelocity().x +
                    rocket->getVelocity().y * rocket->getVelocity().y);

                ss << "STATE SYNC MULTIPLAYER\n"
                    << "Player: " << localPlayer->getName() << "\n"
                    << "Role: " << (networkManager->getRole() == NetworkRole::HOST ? "HOST" : "CLIENT") << "\n"
                    << "Speed: " << std::fixed << std::setprecision(1) << speed << " units/s\n"
                    << "Thrust: " << std::setprecision(0) << rocket->getThrustLevel() * 100.0f << "%";
            }
            else {
                Car* car = localPlayer->getVehicleManager()->getCar();
                ss << "STATE SYNC MULTIPLAYER (CAR)\n"
                    << "Player: " << localPlayer->getName() << "\n"
                    << "On Ground: " << (car->isOnGround() ? "Yes" : "No") << "\n"
                    << "Position: (" << std::fixed << std::setprecision(1)
                    << car->getPosition().x << ", " << car->getPosition().y << ")";
            }
        }
        else if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* rocket = vehicleManager->getRocket();
            float speed = std::sqrt(rocket->getVelocity().x * rocket->getVelocity().x +
                rocket->getVelocity().y * rocket->getVelocity().y);

            ss << "ROCKET INFO\n"
                << "Mass: " << rocket->getMass() << " units\n"
                << "Speed: " << std::fixed << std::setprecision(1) << speed << " units/s\n"
                << "Velocity: (" << std::setprecision(1) << rocket->getVelocity().x << ", "
                << rocket->getVelocity().y << ")\n"
                << "Thrust Level: " << std::setprecision(2) << rocket->getThrustLevel() * 100.0f << "%";
        }
        else if (vehicleManager) {
            Car* car = vehicleManager->getCar();
            ss << "CAR INFO\n"
                << "On Ground: " << (car->isOnGround() ? "Yes" : "No") << "\n"
                << "Position: (" << std::fixed << std::setprecision(1)
                << car->getPosition().x << ", " << car->getPosition().y << ")\n"
                << "Orientation: " << std::setprecision(1) << car->getRotation() << " degrees\n"
                << "Press L to transform back to rocket when on ground";
        }
        rocketInfoPanel->setText(ss.str());
    }

    void updatePlanetInfoPanel() {
        std::stringstream ss;
        Planet* closestPlanet = nullptr;
        float closestDistance = std::numeric_limits<float>::max();

        GameObject* activeVehicle = nullptr;
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            activeVehicle = splitScreenManager->getPlayer1()->getActiveVehicle();
        }
        else if (currentState == GameState::LAN_MULTIPLAYER && localPlayer) {
            activeVehicle = localPlayer->getVehicleManager()->getActiveVehicle();
        }
        else if (vehicleManager) {
            activeVehicle = vehicleManager->getActiveVehicle();
        }

        if (activeVehicle) {
            for (const auto& planetPtr : planets) {
                sf::Vector2f direction = planetPtr->getPosition() - activeVehicle->getPosition();
                float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                if (dist < closestDistance) {
                    closestDistance = dist;
                    closestPlanet = planetPtr;
                }
            }

            if (closestPlanet) {
                std::string planetName = (closestPlanet == planet.get()) ? "Blue Planet" : "Green Planet";
                float speed = std::sqrt(closestPlanet->getVelocity().x * closestPlanet->getVelocity().x +
                    closestPlanet->getVelocity().y * closestPlanet->getVelocity().y);

                ss << "NEAREST PLANET: " << planetName << "\n"
                    << "Distance: " << std::fixed << std::setprecision(0) << closestDistance << " units\n"
                    << "Mass: " << closestPlanet->getMass() << " units\n"
                    << "Radius: " << closestPlanet->getRadius() << " units\n"
                    << "Speed: " << std::setprecision(1) << speed << " units/s";
            }
        }
        planetInfoPanel->setText(ss.str());
    }

    void updateOrbitInfoPanel() {
        std::stringstream ss;

        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            ss << "SPLIT-SCREEN MODE\n"
                << "Camera centered between players\n"
                << "Mouse wheel to zoom\n"
                << "Numbers 1-9,0,= control thrust\n"
                << "for both players";
        }
        else if (currentState == GameState::LAN_MULTIPLAYER && networkManager) {
            ss << "STATE SYNC MULTIPLAYER\n"
                << "Role: " << (networkManager->getRole() == NetworkRole::HOST ? "HOST" : "CLIENT") << "\n"
                << "Status: " << (networkManager->isConnected() ? "CONNECTED" : "DISCONNECTED") << "\n";
            if (localPlayer) {
                ss << "Playing as: " << localPlayer->getName() << "\n";
            }
            ss << "Immediate local response!";
        }
        else if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            ss << "ORBIT INFO\n"
                << "Trajectory visible on screen\n"
                << "Mouse wheel to zoom\n"
                << "Press C to focus on green planet";
        }
        else {
            ss << "ORBIT INFO\n"
                << "Not available in car mode\n"
                << "Transform to rocket for orbital data";
        }
        orbitInfoPanel->setText(ss.str());
    }

    void renderGame() {
        window.setView(gameView);

        // Find closest planet for orbit path
        Planet* closestPlanet = nullptr;
        float closestDistance = std::numeric_limits<float>::max();

        sf::Vector2f referencePos;
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            referencePos = splitScreenManager->getCenterPoint();
        }
        else if (currentState == GameState::LAN_MULTIPLAYER && localPlayer) {
            referencePos = localPlayer->getPosition();
        }
        else if (vehicleManager) {
            referencePos = vehicleManager->getActiveVehicle()->getPosition();
        }

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
        else if (currentState == GameState::LAN_MULTIPLAYER && localPlayer) {
            // Draw trajectory for local player if rocket
            if (localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                localPlayer->getVehicleManager()->getRocket()->drawTrajectory(window, planets,
                    GameConstants::TRAJECTORY_TIME_STEP, GameConstants::TRAJECTORY_STEPS, false);
            }

            // Draw trajectories for all remote players if they're rockets
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

        // Draw game objects
        planet->draw(window);
        planet2->draw(window);

        // Draw players/vehicles based on game mode
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            splitScreenManager->drawWithConstantSize(window, zoomLevel);
        }
        else if (currentState == GameState::LAN_MULTIPLAYER && localPlayer) {
            // Draw local player
            localPlayer->drawWithConstantSize(window, zoomLevel);

            // Draw all remote players
            for (auto& remotePlayer : remotePlayers) {
                remotePlayer->drawWithConstantSize(window, zoomLevel);

                if (fontLoaded) {
                    remotePlayer->drawPlayerLabel(window, font);
                }
            }

            // Draw local player label
            if (fontLoaded) {
                localPlayer->drawPlayerLabel(window, font);
            }
        }
        else if (vehicleManager) {
            vehicleManager->drawWithConstantSize(window, zoomLevel);
        }

        // Draw velocity vectors
        planet->drawVelocityVector(window, 5.0f);
        planet2->drawVelocityVector(window, 5.0f);

        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            splitScreenManager->drawVelocityVectors(window, 2.0f);
        }
        else if (currentState == GameState::LAN_MULTIPLAYER && localPlayer) {
            // Draw local player velocity vector
            localPlayer->drawVelocityVector(window, 2.0f);

            // Draw velocity vectors for all remote players
            for (auto& remotePlayer : remotePlayers) {
                remotePlayer->drawVelocityVector(window, 2.0f);

                // Draw gravity force vectors if in rocket mode
                if (remotePlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                    remotePlayer->getVehicleManager()->getRocket()->drawGravityForceVectors(window, planets, GameConstants::GRAVITY_VECTOR_SCALE);
                }
            }

            // Draw gravity force vectors for local player if in rocket mode
            if (localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                localPlayer->getVehicleManager()->getRocket()->drawGravityForceVectors(window, planets, GameConstants::GRAVITY_VECTOR_SCALE);
            }
        }
        else if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            vehicleManager->drawVelocityVector(window, 2.0f);
            vehicleManager->getRocket()->drawGravityForceVectors(window, planets, GameConstants::GRAVITY_VECTOR_SCALE);
        }

        // Switch to UI view for panels
        window.setView(uiView);

        if (fontLoaded) {
            rocketInfoPanel->draw(window);
            planetInfoPanel->draw(window);
            orbitInfoPanel->draw(window);
            controlsPanel->draw(window);

            // Draw network info panel when network is active
            if (networkManager || currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) {
                networkInfoPanel->draw(window);
            }
        }
    }

    void renderMenu() {
        window.setView(uiView);
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), uiView);

        if (currentState == GameState::MAIN_MENU) {
            mainMenu.update(mousePos);
            mainMenu.draw(window);
        }
        else if (currentState == GameState::MULTIPLAYER_MENU) {
            multiplayerMenu->update(mousePos);
            multiplayerMenu->draw(window);
        }
    }

    void run() {
        sf::Clock clock;

        while (window.isOpen()) {
            float deltaTime = std::min(clock.restart().asSeconds(), 0.1f);

            // Handle events
            if (std::optional<sf::Event> event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    // Clean up network before closing
                    if (networkManager) {
                        networkManager->disconnect();
                    }
                    window.close();
                    continue;
                }

                switch (currentState) {
                case GameState::MAIN_MENU:
                case GameState::MULTIPLAYER_MENU:
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
                updateInfoPanels();
                break;
            default:
                break;
            }

            // Render
            window.clear(sf::Color::Black);

            switch (currentState) {
            case GameState::MAIN_MENU:
            case GameState::MULTIPLAYER_MENU:
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