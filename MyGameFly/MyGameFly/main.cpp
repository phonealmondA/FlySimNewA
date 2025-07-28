#include <SFML/Graphics.hpp>
#include "Planet.h"
#include "Rocket.h"
#include "Car.h"
#include "VehicleManager.h"
#include "GravitySimulator.h"
#include "GameConstants.h"
#include "Button.h"
#include "MainMenu.h"
#include <memory>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <limits>
#include <iostream>

#ifdef _DEBUG
#pragma comment(lib, "sfml-graphics-d.lib")
#pragma comment(lib, "sfml-window-d.lib")
#pragma comment(lib, "sfml-system-d.lib")
#else
#pragma comment(lib, "sfml-graphics.lib")
#pragma comment(lib, "sfml-window.lib")
#pragma comment(lib, "sfml-system.lib")
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
    SINGLE_PLAYER,
    MULTIPLAYER_HOST,
    MULTIPLAYER_CLIENT,
    QUIT
};

// Game class to encapsulate game logic
class Game {
private:
    sf::RenderWindow window;
    MainMenu mainMenu;
    GameState currentState;
    sf::Font font;
    bool fontLoaded;

    // Game objects (will be initialized when entering game modes)
    std::unique_ptr<Planet> planet;
    std::unique_ptr<Planet> planet2;
    std::vector<Planet*> planets;
    std::unique_ptr<VehicleManager> vehicleManager;
    std::unique_ptr<GravitySimulator> gravitySimulator;

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

    // Input tracking
    bool lKeyPressed;

public:
    Game() : window(sf::VideoMode({ 1280, 720 }), "Katie's Space Program"),
        mainMenu(sf::Vector2u(1280, 720)),
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

            controlsPanel->setText(
                "CONTROLS:\n"
                "Arrows: Move/Steer\n"
                "1-9: Set thrust level\n"
                "L: Transform vehicle\n"
                "Z: Zoom out\n"
                "X: Auto-zoom\n"
                "C: Focus planet 2\n"
                "ESC: Return to menu"
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

        // Reset camera
        zoomLevel = 1.0f;
        targetZoom = 1.0f;
        gameView.setCenter(rocketPos);
    }

    void initializeMultiplayerHost() {
        // For now, same as single player - we'll add networking later
        initializeSinglePlayer();
        std::cout << "Multiplayer Host mode - networking to be implemented" << std::endl;
    }

    void initializeMultiplayerClient() {
        // For now, same as single player - we'll add networking later
        initializeSinglePlayer();
        std::cout << "Multiplayer Client mode - networking to be implemented" << std::endl;
    }

    void handleMenuEvents(const sf::Event& event) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), uiView);

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
                currentState = GameState::MULTIPLAYER_HOST;
                initializeMultiplayerHost();
                break;
            case GameMode::MULTIPLAYER_JOIN:
                currentState = GameState::MULTIPLAYER_CLIENT;
                initializeMultiplayerClient();
                break;
            case GameMode::QUIT:
                window.close();
                break;
            default:
                break;
            }
        }
    }

    void handleGameEvents(const sf::Event& event) {
        if (event.is<sf::Event::KeyPressed>()) {
            const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
            if (keyEvent) {
                if (keyEvent->code == sf::Keyboard::Key::Escape) {
                    // Return to main menu
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
                    vehicleManager->switchVehicle();
                }
            }
        }

        if (event.is<sf::Event::KeyReleased>()) {
            const auto* keyEvent = event.getIf<sf::Event::KeyReleased>();
            if (keyEvent && keyEvent->code == sf::Keyboard::Key::L) {
                lKeyPressed = false;
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
        // Thrust level controls
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

        // Camera controls
        handleCameraControls();
    }

    void handleCameraControls() {
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

    void updateGame(float deltaTime) {
        // Update simulation
        gravitySimulator->update(deltaTime);
        planet->update(deltaTime);
        planet2->update(deltaTime);
        vehicleManager->update(deltaTime);

        // Auto-zoom logic
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

        // Smooth zoom
        zoomLevel += (targetZoom - zoomLevel) * deltaTime * 1.0f;
        gameView.setSize(sf::Vector2f(1280.f * zoomLevel, 720.f * zoomLevel));
    }

    void updateInfoPanels() {
        if (!fontLoaded) return;

        // Update all info panels with current game data
        updateVehicleInfoPanel();
        updatePlanetInfoPanel();
        updateOrbitInfoPanel();
    }

    void updateVehicleInfoPanel() {
        std::stringstream ss;
        if (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
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
        else {
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
        GameObject* activeVehicle = vehicleManager->getActiveVehicle();

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
        planetInfoPanel->setText(ss.str());
    }

    void updateOrbitInfoPanel() {
        std::stringstream ss;
        if (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            ss << "ORBIT INFO\n"
                << "Trajectory visible on screen\n"
                << "Use Z/X for zoom control\n"
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
        sf::Vector2f rocketPos = vehicleManager->getActiveVehicle()->getPosition();

        for (auto& planetPtr : planets) {
            sf::Vector2f direction = planetPtr->getPosition() - rocketPos;
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

        // Draw trajectory only if in rocket mode
        if (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            vehicleManager->getRocket()->drawTrajectory(window, planets,
                GameConstants::TRAJECTORY_TIME_STEP, GameConstants::TRAJECTORY_STEPS, false);
        }

        // Draw game objects
        planet->draw(window);
        planet2->draw(window);
        vehicleManager->drawWithConstantSize(window, zoomLevel);

        // Draw velocity vectors
        planet->drawVelocityVector(window, 5.0f);
        planet2->drawVelocityVector(window, 5.0f);

        if (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
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
        }
    }

    void renderMenu() {
        window.setView(uiView);
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), uiView);
        mainMenu.update(mousePos);
        mainMenu.draw(window);
    }

    void run() {
        sf::Clock clock;

        while (window.isOpen()) {
            float deltaTime = std::min(clock.restart().asSeconds(), 0.1f);

            // Handle events
            if (std::optional<sf::Event> event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                    continue;
                }

                switch (currentState) {
                case GameState::MAIN_MENU:
                    handleMenuEvents(*event);
                    break;
                case GameState::SINGLE_PLAYER:
                case GameState::MULTIPLAYER_HOST:
                case GameState::MULTIPLAYER_CLIENT:
                    handleGameEvents(*event);
                    break;
                default:
                    break;
                }
            }

            // Update game state
            switch (currentState) {
            case GameState::SINGLE_PLAYER:
            case GameState::MULTIPLAYER_HOST:
            case GameState::MULTIPLAYER_CLIENT:
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
                renderMenu();
                break;
            case GameState::SINGLE_PLAYER:
            case GameState::MULTIPLAYER_HOST:
            case GameState::MULTIPLAYER_CLIENT:
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