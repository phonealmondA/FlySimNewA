#include <SFML/Graphics.hpp>
#include "Planet.h"
#include "Rocket.h"
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
#include "SavesMenu.h"
#include "SinglePlayerGame.h"
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

// Game class to encapsulate game logic
class Game {
private:
    sf::RenderWindow window;
    MainMenu mainMenu;
    std::unique_ptr<MultiplayerMenu> multiplayerMenu;
    std::unique_ptr<OnlineMultiplayerMenu> onlineMenu;
    GameState currentState;

    // UI Management
    std::unique_ptr<UIManager> uiManager;

    // Network manager for multiplayer
    std::unique_ptr<NetworkManager> networkManager;

    // NEW: Modular single player components
    std::unique_ptr<SavesMenu> savesMenu;
    std::unique_ptr<SinglePlayerGame> singlePlayerGame;

    // Multiplayer game objects (keep for multiplayer modes)
    std::unique_ptr<Planet> planet;
    std::vector<std::unique_ptr<Planet>> orbitingPlanets;
    std::vector<Planet*> planets;
    std::unique_ptr<VehicleManager> vehicleManager;
    std::unique_ptr<SplitScreenManager> splitScreenManager;
    std::unique_ptr<GravitySimulator> gravitySimulator;
    std::unique_ptr<SatelliteManager> satelliteManager;

    // Multiplayer camera and input
    sf::View gameView;
    sf::View uiView;
    float zoomLevel;
    float targetZoom;
    bool lKeyPressed;
    bool tKeyPressed;
    bool fuelIncreaseKeyPressed;
    bool fuelDecreaseKeyPressed;
    float gameTime;

    // Helper methods for state transitions
    void handleMainMenuResult() {
        GameMode mode = mainMenu.getSelectedMode();

        // Only process if there's an actual selection
        if (mode == GameMode::NONE) {
            return;
        }

        std::cout << "Processing main menu selection: " << static_cast<int>(mode) << std::endl;

        if (mode == GameMode::SINGLE_PLAYER) {
            if (!savesMenu) {
                std::cerr << "Error: SavesMenu not initialized! Creating new one..." << std::endl;
                savesMenu = std::make_unique<SavesMenu>(window.getSize());
            }

            currentState = GameState::SAVES_MENU;
            savesMenu->show();
            mainMenu.hide(); // Hide main menu
            std::cout << "Transitioning to SAVES_MENU" << std::endl;
        }
        else if (mode == GameMode::MULTIPLAYER_HOST) {
            if (!multiplayerMenu) {
                std::cerr << "Error: MultiplayerMenu not initialized! Creating new one..." << std::endl;
                multiplayerMenu = std::make_unique<MultiplayerMenu>(window.getSize());
            }

            currentState = GameState::MULTIPLAYER_MENU;
            multiplayerMenu->show();
            mainMenu.hide(); // Hide main menu
            std::cout << "Transitioning to MULTIPLAYER_MENU" << std::endl;
        }
        else if (mode == GameMode::QUIT) {
            std::cout << "Quitting game..." << std::endl;
            window.close();
        }

        // Reset the selection after processing
        mainMenu.reset();
    }

    void handleSavesMenuResult() {
        if (!savesMenu) return;

        LoadAction action = savesMenu->getSelectedAction();

        // Only process if there's an actual selection
        if (action == LoadAction::NONE) {
            return;
        }

        std::cout << "Processing saves menu selection: " << static_cast<int>(action) << std::endl;

        if (action == LoadAction::NEW_GAME) {
            // Create new single player game
            singlePlayerGame = std::make_unique<SinglePlayerGame>(window, window.getSize());
            if (singlePlayerGame->initializeNewGame()) {
                currentState = GameState::SINGLE_PLAYER;
                savesMenu->hide(); // Hide saves menu
                std::cout << "Starting new single player game" << std::endl;
            }
            else {
                std::cerr << "Failed to initialize new single player game!" << std::endl;
            }
        }
        else if (action == LoadAction::LOAD_GAME) {
            // Load existing game
            GameSaveData saveData;
            if (savesMenu->loadSelectedSaveData(saveData)) {
                singlePlayerGame = std::make_unique<SinglePlayerGame>(window, window.getSize());
                if (singlePlayerGame->initializeFromLoad(saveData, savesMenu->getSelectedSaveFile())) {
                    currentState = GameState::SINGLE_PLAYER;
                    savesMenu->hide(); // Hide saves menu
                    std::cout << "Loaded single player game: " << savesMenu->getSelectedSaveFile() << std::endl;
                }
                else {
                    std::cerr << "Failed to initialize loaded single player game!" << std::endl;
                }
            }
            else {
                std::cerr << "Failed to load save data!" << std::endl;
            }
        }
        else if (action == LoadAction::BACK_TO_MENU) {
            currentState = GameState::MAIN_MENU;
            mainMenu.show();
            savesMenu->hide(); // Hide saves menu
            std::cout << "Returning to main menu from saves" << std::endl;
        }
    }

    void handleSinglePlayerResult() {
        if (!singlePlayerGame) return;

        SinglePlayerResult result = singlePlayerGame->getResult();
        if (result == SinglePlayerResult::RETURN_TO_MENU) {
            // Auto-save before returning to menu
            singlePlayerGame->autoSave();
            singlePlayerGame.reset();
            currentState = GameState::MAIN_MENU;
            mainMenu.show();
            std::cout << "Returning to main menu from single player" << std::endl;
        }
        else if (result == SinglePlayerResult::QUIT_GAME) {
            singlePlayerGame->autoSave();
            window.close();
        }
    }

    // Multiplayer initialization methods (keep existing)
    void initializeMultiplayer() {
        // Initialize satellite manager first
        satelliteManager = std::make_unique<SatelliteManager>();

        // Create central planet - Use correct argument order: position, radius, mass, color
        planet = std::make_unique<Planet>(
            sf::Vector2f(640.f, 360.f),
            50.0f, 1000.0f, sf::Color::Blue);
        planets.push_back(planet.get());

        // Create some orbiting planets - Use correct argument order
        orbitingPlanets.push_back(std::make_unique<Planet>(
            sf::Vector2f(840.f, 360.f),
            15.0f, 50.0f, sf::Color::Red));
        orbitingPlanets.push_back(std::make_unique<Planet>(
            sf::Vector2f(440.f, 360.f),
            12.0f, 30.0f, sf::Color::Green));
        orbitingPlanets.push_back(std::make_unique<Planet>(
            sf::Vector2f(640.f, 160.f),
            18.0f, 80.0f, sf::Color::Yellow));

        for (auto& orbitPlanet : orbitingPlanets) {
            planets.push_back(orbitPlanet.get());
        }

        // Initialize gravity simulator - Use default constructor then add planets
        gravitySimulator = std::make_unique<GravitySimulator>();
        for (const auto& planetPtr : planets) {
            gravitySimulator->addPlanet(planetPtr);
        }
        gravitySimulator->addSatelliteManager(satelliteManager.get());
    }

    void initializeLocalMultiplayer() {
        initializeMultiplayer();

        sf::Vector2f player1Spawn(600.f, 320.f);
        sf::Vector2f player2Spawn(680.f, 400.f);

        splitScreenManager = std::make_unique<SplitScreenManager>(player1Spawn, player2Spawn, planets, satelliteManager.get());

        // Initialize camera
        gameView.setCenter(sf::Vector2f(640.f, 360.f));
        gameView.setSize(sf::Vector2f(1280.f, 720.f));
        uiView.setCenter(sf::Vector2f(640.f, 360.f));
        uiView.setSize(sf::Vector2f(1280.f, 720.f));
        zoomLevel = 1.0f;
        targetZoom = 1.0f;
    }

    // Multiplayer input/update/render methods (keep existing)
    void handleGameInput(float deltaTime) {
        // Handle game-specific input for multiplayer modes
        bool currentLPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L);
        bool currentTPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T);
        bool currentFuelIncreasePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Period);
        bool currentFuelDecreasePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Comma);

        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            splitScreenManager->handlePlayer1Input(deltaTime);
            splitScreenManager->handlePlayer2Input(deltaTime);

            if (currentLPressed && !lKeyPressed) {
                if (uiManager) {
                    uiManager->toggleUI();
                }
            }
        }

        lKeyPressed = currentLPressed;
        tKeyPressed = currentTPressed;
        fuelIncreaseKeyPressed = currentFuelIncreasePressed;
        fuelDecreaseKeyPressed = currentFuelDecreasePressed;
    }

    void updateGame(float deltaTime) {
        gameTime += deltaTime;

        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            splitScreenManager->update(deltaTime);

            if (gravitySimulator) {
                gravitySimulator->update(deltaTime);
            }

            if (satelliteManager) {
                satelliteManager->update(deltaTime);
            }

            updateCamera();

            if (uiManager) {
                uiManager->update(currentState, nullptr, splitScreenManager.get(), nullptr, planets, nullptr, satelliteManager.get());
            }
        }
    }

    void updateCamera() {
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
            sf::Vector2f centerPoint = splitScreenManager->getCenterPoint();
            float requiredZoom = splitScreenManager->getRequiredZoomToShowBothPlayers(window.getSize());
            targetZoom = std::max(0.3f, std::min(2.0f, requiredZoom));

            float zoomSpeed = 2.0f;
            float zoomDiff = targetZoom - zoomLevel;
            zoomLevel += zoomDiff * zoomSpeed * (1.0f / 60.0f);

            gameView.setCenter(centerPoint);
            gameView.setSize(sf::Vector2f(1280.f * zoomLevel, 720.f * zoomLevel));
        }
    }

    void renderMenu() {
        if (currentState == GameState::MAIN_MENU) {
            mainMenu.draw(window);
        }
        else if (currentState == GameState::MULTIPLAYER_MENU && multiplayerMenu) {
            multiplayerMenu->draw(window);
        }
        else if (currentState == GameState::ONLINE_MENU && onlineMenu) {
            onlineMenu->draw(window);
        }
    }

    void renderGame() {
        if (currentState == GameState::LOCAL_PC_MULTIPLAYER) {
            window.setView(gameView);

            // Draw planets
            if (planet) {
                planet->draw(window);
            }
            for (auto& orbitPlanet : orbitingPlanets) {
                orbitPlanet->draw(window);
            }

            // Draw split screen players
            if (splitScreenManager) {
                splitScreenManager->drawWithConstantSize(window, zoomLevel);
            }

            // Draw satellites
            if (satelliteManager) {
                satelliteManager->drawWithConstantSize(window, zoomLevel);
            }

            // Draw UI
            window.setView(uiView);
            if (uiManager) {
                uiManager->draw(window, currentState, nullptr, satelliteManager.get());
            }
        }
    }

public:
    Game() : window(sf::VideoMode({ 1280, 720 }), "MyGameFly"), currentState(GameState::MAIN_MENU), mainMenu(window.getSize()),
        zoomLevel(1.0f), targetZoom(1.0f), lKeyPressed(false), tKeyPressed(false),
        fuelIncreaseKeyPressed(false), fuelDecreaseKeyPressed(false), gameTime(0.0f)
    {
        std::cout << "Initializing game..." << std::endl;

        // Initialize UI Manager
        uiManager = std::make_unique<UIManager>(window.getSize());

        // Initialize menu systems
        try {
            savesMenu = std::make_unique<SavesMenu>(window.getSize());
            multiplayerMenu = std::make_unique<MultiplayerMenu>(window.getSize());
            onlineMenu = std::make_unique<OnlineMultiplayerMenu>(window.getSize());
            std::cout << "Menu systems initialized successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error initializing menu systems: " << e.what() << std::endl;
        }

        // SinglePlayerGame will be created when needed
        singlePlayerGame = nullptr;

        // Initialize camera views
        gameView.setCenter(sf::Vector2f(640.f, 360.f));
        gameView.setSize(sf::Vector2f(1280.f, 720.f));
        uiView.setCenter(sf::Vector2f(640.f, 360.f));
        uiView.setSize(sf::Vector2f(1280.f, 720.f));

        // CRITICAL FIX: Ensure main menu is active at startup
        mainMenu.show();
        std::cout << "Main menu activated. Menu active: " << mainMenu.getIsActive() << std::endl;
    }

    void run() {
        sf::Clock clock;
        std::cout << "Starting game loop..." << std::endl;

        while (window.isOpen()) {
            float deltaTime = clock.restart().asSeconds();
            deltaTime = std::min(deltaTime, 1.0f / 30.0f);

            while (auto optionalEvent = window.pollEvent()) {
                sf::Event event = *optionalEvent;

                if (event.is<sf::Event::Closed>()) {
                    window.close();
                }

                if (event.is<sf::Event::Resized>()) {
                    const auto* resized = event.getIf<sf::Event::Resized>();
                    if (resized) {
                        sf::FloatRect visibleArea({ 0.f, 0.f }, { static_cast<float>(resized->size.x), static_cast<float>(resized->size.y) });
                        window.setView(sf::View(visibleArea));
                        uiView = sf::View(visibleArea);

                        if (uiManager) {
                            uiManager->handleWindowResize(sf::Vector2u(resized->size.x, resized->size.y));
                        }
                        if (singlePlayerGame) {
                            singlePlayerGame->handleWindowResize(sf::Vector2u(resized->size.x, resized->size.y));
                        }
                    }
                }

                // Handle menu-specific events
                sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(window));

                if (currentState == GameState::MAIN_MENU) {
                    // DEBUG: Check main menu state
                    static int debugCounter = 0;
                    if (debugCounter % 60 == 0) { // Print every 60 frames (1 second at 60fps)
                        std::cout << "Main menu active: " << mainMenu.getIsActive()
                            << ", Selected mode: " << static_cast<int>(mainMenu.getSelectedMode()) << std::endl;
                    }
                    debugCounter++;

                    mainMenu.handleEvent(event, mousePos);
                    // FIXED: Only handle result if there's a selection
                    if (mainMenu.getSelectedMode() != GameMode::NONE) {
                        handleMainMenuResult();
                    }
                }
                else if (currentState == GameState::MULTIPLAYER_MENU && multiplayerMenu) {
                    multiplayerMenu->handleEvent(event, mousePos);
                    // Handle multiplayer menu results (existing logic)
                    auto mode = multiplayerMenu->getSelectedMode();
                    if (mode == MultiplayerMode::LOCAL_PC) {
                        initializeLocalMultiplayer();
                        currentState = GameState::LOCAL_PC_MULTIPLAYER;
                        multiplayerMenu->hide();
                    }
                    else if (mode == MultiplayerMode::BACK_TO_MAIN) {
                        currentState = GameState::MAIN_MENU;
                        mainMenu.show();
                        multiplayerMenu->hide();
                    }
                    // Reset multiplayer menu selection
                    if (mode != MultiplayerMode::NONE) {
                        multiplayerMenu->reset();
                    }
                }
                else if (currentState == GameState::SAVES_MENU && savesMenu) {
                    savesMenu->handleEvent(event, mousePos);
                    // FIXED: Only handle result if there's a selection
                    if (savesMenu->getSelectedAction() != LoadAction::NONE) {
                        handleSavesMenuResult();
                    }
                }
                else if (currentState == GameState::SINGLE_PLAYER && singlePlayerGame) {
                    SinglePlayerResult result = singlePlayerGame->handleEvents();
                    if (result != SinglePlayerResult::CONTINUE_PLAYING) {
                        handleSinglePlayerResult();
                    }
                }
                else if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
                    if (event.is<sf::Event::KeyPressed>()) {
                        const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
                        if (keyEvent && keyEvent->code == sf::Keyboard::Key::Escape) {
                            currentState = GameState::MAIN_MENU;
                            mainMenu.show();
                        }
                    }
                    splitScreenManager->handleTransformInputs(event);
                }
            }

            // Update game state
            switch (currentState) {
            case GameState::MAIN_MENU:
                // Update main menu (for hover effects, etc.)
            {
                sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(window));
                mainMenu.update(mousePos);
            }
            break;
            case GameState::SAVES_MENU:
                if (savesMenu) {
                    sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(window));
                    savesMenu->update(mousePos);
                }
                break;
            case GameState::SINGLE_PLAYER:
                if (singlePlayerGame) {
                    singlePlayerGame->update(deltaTime);
                    handleSinglePlayerResult();
                }
                break;
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
            case GameState::SAVES_MENU:
                if (savesMenu) {
                    savesMenu->draw(window);
                }
                break;
            case GameState::SINGLE_PLAYER:
                if (singlePlayerGame) {
                    singlePlayerGame->render();
                }
                break;
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