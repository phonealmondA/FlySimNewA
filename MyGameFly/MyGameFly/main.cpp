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
    ONLINE_MENU,
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
    std::unique_ptr<OnlineMultiplayerMenu> onlineMenu;
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

    // Input tracking - UPDATED FOR FUEL SYSTEM
    bool lKeyPressed;
    bool fuelIncreaseKeyPressed;  // '.' key
    bool fuelDecreaseKeyPressed;  // ',' key

public:
    Game() : window(sf::VideoMode({ 1280, 720 }), "Katie's Space Program"),
        mainMenu(sf::Vector2u(1280, 720)),
        multiplayerMenu(std::make_unique<MultiplayerMenu>(sf::Vector2u(1280, 720))),
        onlineMenu(std::make_unique<OnlineMultiplayerMenu>(sf::Vector2u(1280, 720))),
        currentState(GameState::MAIN_MENU),
        gameView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
        uiView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
        zoomLevel(1.0f),
        targetZoom(1.0f),
        lKeyPressed(false),
        fuelIncreaseKeyPressed(false),
        fuelDecreaseKeyPressed(false),
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
            rocketInfoPanel = std::make_unique<TextPanel>(font, 12, sf::Vector2f(10, 10), sf::Vector2f(250, 180));
            planetInfoPanel = std::make_unique<TextPanel>(font, 12, sf::Vector2f(10, 200), sf::Vector2f(250, 120));
            orbitInfoPanel = std::make_unique<TextPanel>(font, 12, sf::Vector2f(10, 330), sf::Vector2f(250, 100));
            controlsPanel = std::make_unique<TextPanel>(font, 12, sf::Vector2f(10, 440), sf::Vector2f(250, 150));
            networkInfoPanel = std::make_unique<TextPanel>(font, 12, sf::Vector2f(10, 600), sf::Vector2f(250, 80));

            // UPDATED CONTROLS for fuel system
            controlsPanel->setText(
                "CONTROLS:\n"
                "Arrows: Move/Rotate\n"
                "1-9,0,=: Thrust level & fuel rate\n"
                ".: Collect fuel from planet\n"
                ",: Give fuel to planet\n"
                "L: Transform rocket/car\n"
                "Mouse wheel: Zoom\n"
                "ESC: Menu"
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

        std::cout << "Single Player mode initialized with fuel system!" << std::endl;
        std::cout << "Use '.' to collect fuel, ',' to give fuel, 1-9,0,= for thrust/transfer rate" << std::endl;
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

        // Create spawn positions for both players
        sf::Vector2f planetPos = planet->getPosition();
        float planetRadius = planet->getRadius();
        float rocketSize = GameConstants::ROCKET_SIZE;

        sf::Vector2f player1Pos = planetPos + sf::Vector2f(0, -1) * (planetRadius + rocketSize);
        sf::Vector2f player2Pos = planetPos + sf::Vector2f(0, 1) * (planetRadius + rocketSize);

        splitScreenManager = std::make_unique<SplitScreenManager>(player1Pos, player2Pos, planets);

        // FUEL SYSTEM: Set up fuel collection for both rockets
        // TODO: Update SplitScreenManager to handle individual fuel transfer controls
        // Player 1 will use '.' and ',' keys, Player 2 will need different keys (maybe '[' and ']')

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

        std::cout << "Local PC Split-Screen Multiplayer initialized!" << std::endl;
        std::cout << "TODO: Implement individual fuel transfer controls for each player" << std::endl;
    }

    void initializeLANMultiplayer() {
        // TODO: Implement fuel transfer synchronization for network multiplayer
        // Each player will need to send fuel transfer state to other players
        // Planet mass changes need to be synchronized across all clients
        std::cout << "LAN Multiplayer fuel system not yet implemented" << std::endl;
        initializeSinglePlayer(); // Fallback for now
    }

    void initializeOnlineMultiplayer(const ConnectionInfo& connectionInfo) {
        // TODO: Implement fuel transfer synchronization for online multiplayer
        // Same requirements as LAN but with custom IP/port support
        std::cout << "Online Multiplayer fuel system not yet implemented" << std::endl;
        initializeSinglePlayer(); // Fallback for now
    }

    void handleMenuEvents(const sf::Event& event) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), uiView);

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
            // TODO: Add fuel transfer input handling for split-screen mode
        }

        if (event.is<sf::Event::KeyPressed>()) {
            const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
            if (keyEvent) {
                if (keyEvent->code == sf::Keyboard::Key::Escape) {
                    if (networkManager) {
                        networkManager->disconnect();
                        networkManager.reset();
                    }

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
                    else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
                        localPlayer->requestTransform();
                    }
                    else if (vehicleManager) {
                        vehicleManager->switchVehicle();
                    }
                }
                // FUEL TRANSFER INPUT HANDLING - SINGLE PLAYER ONLY FOR NOW
                else if (keyEvent->code == sf::Keyboard::Key::Period && !fuelIncreaseKeyPressed) {
                    fuelIncreaseKeyPressed = true;
                    if (currentState == GameState::SINGLE_PLAYER && vehicleManager &&
                        vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
                        Rocket* rocket = vehicleManager->getRocket();
                        float transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * rocket->getThrustLevel();
                        if (transferRate < GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f) {
                            transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f;
                        }
                        rocket->startFuelTransferIn(transferRate);
                        std::cout << "Started fuel collection at rate: " << transferRate << std::endl;
                    }
                }
                else if (keyEvent->code == sf::Keyboard::Key::Comma && !fuelDecreaseKeyPressed) {
                    fuelDecreaseKeyPressed = true;
                    if (currentState == GameState::SINGLE_PLAYER && vehicleManager &&
                        vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
                        Rocket* rocket = vehicleManager->getRocket();
                        float transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * rocket->getThrustLevel();
                        if (transferRate < GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f) {
                            transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f;
                        }
                        rocket->startFuelTransferOut(transferRate);
                        std::cout << "Started fuel transfer out at rate: " << transferRate << std::endl;
                    }
                }
                else if (keyEvent->code == sf::Keyboard::Key::H && networkManager) {
                    networkManager->sendHello();
                    std::cout << "Sent hello message to network!" << std::endl;
                }
            }
        }

        if (event.is<sf::Event::KeyReleased>()) {
            const auto* keyEvent = event.getIf<sf::Event::KeyReleased>();
            if (keyEvent) {
                if (keyEvent->code == sf::Keyboard::Key::L) {
                    lKeyPressed = false;
                }
                // FUEL TRANSFER KEY RELEASES
                else if (keyEvent->code == sf::Keyboard::Key::Period) {
                    fuelIncreaseKeyPressed = false;
                    if (currentState == GameState::SINGLE_PLAYER && vehicleManager &&
                        vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
                        vehicleManager->getRocket()->stopFuelTransfer();
                        std::cout << "Stopped fuel transfer" << std::endl;
                    }
                }
                else if (keyEvent->code == sf::Keyboard::Key::Comma) {
                    fuelDecreaseKeyPressed = false;
                    if (currentState == GameState::SINGLE_PLAYER && vehicleManager &&
                        vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
                        vehicleManager->getRocket()->stopFuelTransfer();
                        std::cout << "Stopped fuel transfer" << std::endl;
                    }
                }
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
            splitScreenManager->handlePlayer1Input(deltaTime);
            splitScreenManager->handlePlayer2Input(deltaTime);

            // Synced thrust level controls (also affects fuel transfer rate)
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

            // TODO: Handle fuel transfer inputs for split-screen players
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            // TODO: Network player input will include fuel transfer controls
            localPlayer->handleLocalInput(deltaTime);
        }
        else if (vehicleManager) {
            // Single player input handling - UPDATED FOR FUEL SYSTEM

            // Thrust level controls (also affects fuel transfer rate)
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
                vehicleManager->applyThrust(vehicleManager->getRocket()->getThrustLevel());
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
                vehicleManager->applyThrust(-vehicleManager->getRocket()->getThrustLevel());
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
                vehicleManager->rotate(-6.0f * deltaTime * 60.0f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
                vehicleManager->rotate(6.0f * deltaTime * 60.0f);

            handleCameraControls();
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
                float dist1 = std::sqrt(
                    std::pow(vehiclePos.x - planet->getPosition().x, 2) +
                    std::pow(vehiclePos.y - planet->getPosition().y, 2)
                );
                float dist2 = std::sqrt(
                    std::pow(vehiclePos.x - planet2->getPosition().x, 2) +
                    std::pow(vehiclePos.y - planet2->getPosition().y, 2)
                );
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
            // TODO: Update network manager for fuel system synchronization
            // - Send/receive fuel transfer states
            // - Synchronize planet mass changes
            // - Handle fuel transfer conflicts between players
            networkManager->update(deltaTime);
        }

        // Update simulation
        gravitySimulator->update(deltaTime);
        planet->update(deltaTime);
        planet2->update(deltaTime);

        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            splitScreenManager->update(deltaTime);

            // TODO: Process fuel transfers for both split-screen players
            // Need to handle separate fuel transfer controls for each player

            if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
                gameView.setCenter(splitScreenManager->getCenterPoint());
            }
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            // TODO: Network multiplayer fuel system updates
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
            // TODO: Add fuel system network status
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

            ss << "NETWORK STATUS\n"
                << "Role: " << roleStr << "\n"
                << "Status: " << statusStr << "\n";

            if (localPlayer) {
                ss << "Player ID: " << localPlayer->getID() << "\n";
            }

            ss << "Fuel sync: TODO";
        }
        else {
            ss << "NETWORK STATUS\n"
                << "Network not active\n"
                << "Single player fuel system active";
        }

        networkInfoPanel->setText(ss.str());
    }

    void updateVehicleInfoPanel() {
        std::stringstream ss;

        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            // TODO: Update for fuel system in split-screen
            VehicleManager* p1 = splitScreenManager->getPlayer1();
            VehicleManager* p2 = splitScreenManager->getPlayer2();

            ss << "SPLIT-SCREEN MODE\n";
            ss << "Player 1 (Arrows + L): " << (p1->getActiveVehicleType() == VehicleType::ROCKET ? "ROCKET" : "CAR") << "\n";
            ss << "Player 2 (WASD + K): " << (p2->getActiveVehicleType() == VehicleType::ROCKET ? "ROCKET" : "CAR") << "\n";

            if (p1->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* rocket = p1->getRocket();
                float thrustLevel = rocket->getThrustLevel();
                ss << "Shared Thrust: " << std::fixed << std::setprecision(0) << thrustLevel * 100.0f << "%\n";
                ss << "P1 Fuel: " << std::setprecision(1) << rocket->getCurrentFuel() << "/"
                    << rocket->getMaxFuel() << " (" << std::setprecision(0) << rocket->getFuelPercentage() << "%)\n";
                ss << "P1 Mass: " << std::setprecision(1) << rocket->getMass() << "/" << rocket->getMaxMass() << "\n";
            }

            if (p2->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* rocket2 = p2->getRocket();
                ss << "P2 Fuel: " << std::setprecision(1) << rocket2->getCurrentFuel() << "/"
                    << rocket2->getMaxFuel() << " (" << std::setprecision(0) << rocket2->getFuelPercentage() << "%)\n";
                ss << "P2 Mass: " << std::setprecision(1) << rocket2->getMass() << "/" << rocket2->getMaxMass() << "\n";
            }

            ss << "TODO: Individual fuel controls";
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            // TODO: Network multiplayer fuel info
            std::string modeStr = (currentState == GameState::LAN_MULTIPLAYER) ? "LAN" : "ONLINE";

            if (localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* rocket = localPlayer->getVehicleManager()->getRocket();
                float speed = std::sqrt(rocket->getVelocity().x * rocket->getVelocity().x +
                    rocket->getVelocity().y * rocket->getVelocity().y);

                ss << modeStr << " MULTIPLAYER\n"
                    << "Player: " << localPlayer->getName() << "\n"
                    << "Speed: " << std::fixed << std::setprecision(1) << speed << " units/s\n"
                    << "Thrust: " << std::setprecision(0) << rocket->getThrustLevel() * 100.0f << "%\n"
                    << "Fuel: " << std::setprecision(1) << rocket->getCurrentFuel() << "/"
                    << rocket->getMaxFuel() << " (" << std::setprecision(0) << rocket->getFuelPercentage() << "%)\n"
                    << "Mass: " << std::setprecision(1) << rocket->getMass() << "/" << rocket->getMaxMass() << "\n"
                    << "Collecting: " << (rocket->getIsCollectingFuel() ? "YES" : "NO");
            }
            else {
                Car* car = localPlayer->getVehicleManager()->getCar();
                ss << modeStr << " MULTIPLAYER (CAR)\n"
                    << "Player: " << localPlayer->getName() << "\n"
                    << "On Ground: " << (car->isOnGround() ? "Yes" : "No");
            }
        }
        else if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            // UPDATED: Single player rocket info with fuel system
            Rocket* rocket = vehicleManager->getRocket();
            float speed = std::sqrt(rocket->getVelocity().x * rocket->getVelocity().x +
                rocket->getVelocity().y * rocket->getVelocity().y);

            ss << "ROCKET INFO (FUEL SYSTEM)\n"
                << "Speed: " << std::fixed << std::setprecision(1) << speed << " units/s\n"
                << "Thrust Level: " << std::setprecision(0) << rocket->getThrustLevel() * 100.0f << "%\n"
                << "Fuel: " << std::setprecision(1) << rocket->getCurrentFuel() << "/"
                << rocket->getMaxFuel() << " (" << std::setprecision(0) << rocket->getFuelPercentage() << "%)\n"
                << "Mass: " << std::setprecision(1) << rocket->getMass() << "/" << rocket->getMaxMass() << "\n"
                << "Transferring: " << (rocket->isTransferringFuel() ? "YES" : "NO") << "\n"
                << "Rate: " << std::setprecision(1) << rocket->getCurrentTransferRate() << " units/s\n"
                << "Can Thrust: " << (rocket->canThrust() ? "YES" : "NO");
        }
        else if (vehicleManager) {
            Car* car = vehicleManager->getCar();
            ss << "CAR INFO\n"
                << "On Ground: " << (car->isOnGround() ? "Yes" : "No") << "\n"
                << "Position: (" << std::fixed << std::setprecision(1)
                << car->getPosition().x << ", " << car->getPosition().y << ")\n"
                << "Orientation: " << std::setprecision(1) << car->getRotation() << " degrees\n"
                << "Press L to transform to rocket\n"
                << "Cars don't use fuel system";
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
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            activeVehicle = localPlayer->getVehicleManager()->getActiveVehicle();
        }
        else if (vehicleManager) {
            activeVehicle = vehicleManager->getActiveVehicle();
        }

        if (activeVehicle) {
            for (const auto& planetPtr : planets) {
                sf::Vector2f direction = planetPtr->getPosition() - activeVehicle->getPosition();
                float centerDistance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                if (centerDistance < closestDistance) {
                    closestDistance = centerDistance;
                    closestPlanet = planetPtr;
                }
            }

            if (closestPlanet) {
                std::string planetName = (closestPlanet == planet.get()) ? "Blue Planet" : "Green Planet";
                float speed = std::sqrt(closestPlanet->getVelocity().x * closestPlanet->getVelocity().x +
                    closestPlanet->getVelocity().y * closestPlanet->getVelocity().y);
                float surfaceDistance = closestDistance - closestPlanet->getRadius() - GameConstants::ROCKET_SIZE;
                float fuelRange = closestPlanet->getFuelCollectionRange();
                bool inFuelRange = closestDistance <= fuelRange;

                ss << "NEAREST PLANET: " << planetName << "\n"
                    << "Distance: " << std::fixed << std::setprecision(0) << surfaceDistance << " units\n"
                    << "Mass: " << std::setprecision(0) << closestPlanet->getMass() << " units\n"
                    << "Radius: " << std::setprecision(0) << closestPlanet->getRadius() << " units\n"
                    << "Speed: " << std::setprecision(1) << speed << " units/s\n"
                    << "Can collect fuel: " << (closestPlanet->canCollectFuel() ? "YES" : "NO") << "\n"
                    << "In fuel range: " << (inFuelRange ? "YES" : "NO") << "\n"
                    << "Fuel range: " << std::setprecision(0) << (fuelRange - closestPlanet->getRadius()) << " units";
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
                << "TODO: Individual fuel controls\n"
                << "Fuel mining from planets!";
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && networkManager) {
            std::string modeStr = (currentState == GameState::LAN_MULTIPLAYER) ? "LAN" : "ONLINE";
            ss << modeStr << " MULTIPLAYER\n"
                << "Role: " << (networkManager->getRole() == NetworkRole::HOST ? "HOST" : "CLIENT") << "\n"
                << "Status: " << (networkManager->isConnected() ? "CONNECTED" : "DISCONNECTED") << "\n";
            if (localPlayer) {
                ss << "Playing as: " << localPlayer->getName() << "\n";
            }
            ss << "TODO: Fuel sync implementation\n"
                << "Local fuel system active";
        }
        else if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            ss << "FUEL SYSTEM ACTIVE\n"
                << ".: Collect fuel from planet\n"
                << ",: Give fuel to planet\n"
                << "1-9,0,=: Set thrust & transfer rate\n"
                << "Mouse wheel to zoom\n"
                << "Dynamic mass affects physics!\n"
                << "Mine planets for fuel!";
        }
        else {
            ss << "CAR MODE\n"
                << "No fuel system in car mode\n"
                << "Transform to rocket (L) for fuel\n"
                << "Rockets start empty - need fuel!";
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
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
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

        // Draw game objects - planets automatically draw fuel collection rings
        planet->draw(window);
        planet2->draw(window);

        // Draw fuel collection indicators and players/vehicles
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            splitScreenManager->drawWithConstantSize(window, zoomLevel);
            // FUEL COLLECTION LINES for split-screen
            drawFuelCollectionLines(splitScreenManager->getPlayer1()->getRocket());
            drawFuelCollectionLines(splitScreenManager->getPlayer2()->getRocket());
        }
        else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
            localPlayer->drawWithConstantSize(window, zoomLevel);
            drawFuelCollectionLines(localPlayer->getVehicleManager()->getRocket());

            for (auto& remotePlayer : remotePlayers) {
                remotePlayer->drawWithConstantSize(window, zoomLevel);
                drawFuelCollectionLines(remotePlayer->getVehicleManager()->getRocket());
                if (fontLoaded) {
                    remotePlayer->drawPlayerLabel(window, font);
                }
            }

            if (fontLoaded) {
                localPlayer->drawPlayerLabel(window, font);
            }
        }
        else if (vehicleManager) {
            vehicleManager->drawWithConstantSize(window, zoomLevel);
            // FUEL COLLECTION LINES for single player
            if (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
                drawFuelCollectionLines(vehicleManager->getRocket());
            }
        }

        // Draw velocity vectors
        planet->drawVelocityVector(window, 5.0f);
        planet2->drawVelocityVector(window, 5.0f);

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

        // Switch to UI view for panels
        window.setView(uiView);

        if (fontLoaded) {
            rocketInfoPanel->draw(window);
            planetInfoPanel->draw(window);
            orbitInfoPanel->draw(window);
            controlsPanel->draw(window);

            if (networkManager || currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) {
                networkInfoPanel->draw(window);
            }
        }
    }

    // FUEL SYSTEM: Helper method to draw fuel collection lines
    void drawFuelCollectionLines(Rocket* rocket) {
        if (!rocket || !rocket->isTransferringFuel()) return;

        Planet* sourcePlanet = rocket->getFuelSourcePlanet();
        if (!sourcePlanet) return;

        sf::VertexArray fuelLine(sf::PrimitiveType::LineStrip);
        sf::Vertex startVertex;
        startVertex.position = rocket->getPosition();
        startVertex.color = GameConstants::FUEL_RING_ACTIVE_COLOR;
        fuelLine.append(startVertex);

        sf::Vertex endVertex;
        endVertex.position = sourcePlanet->getPosition();
        endVertex.color = GameConstants::FUEL_RING_ACTIVE_COLOR;
        fuelLine.append(endVertex);

        window.draw(fuelLine);
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
        else if (currentState == GameState::ONLINE_MENU) {
            onlineMenu->update(mousePos);
            onlineMenu->draw(window);
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