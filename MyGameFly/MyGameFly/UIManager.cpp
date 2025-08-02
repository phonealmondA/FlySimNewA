#include "UIManager.h"
#include "VehicleManager.h"
#include "SplitScreenManager.h"
#include "Player.h"
#include "Planet.h"
#include "Rocket.h"
#include "NetworkManager.h"
#include "GameConstants.h"
#include <iostream>

UIManager::UIManager(sf::Vector2u winSize)
    : windowSize(winSize), fontLoaded(false), showUI(true), showDebugInfo(true),
    uiView(sf::Vector2f(winSize.x / 2.0f, winSize.y / 2.0f), sf::Vector2f(static_cast<float>(winSize.x), static_cast<float>(winSize.y)))
{
    loadFont();
    setupViews();

    if (fontLoaded) {
        gameInfoDisplay = std::make_unique<GameInfoDisplay>(font, windowSize);
    }
}

void UIManager::loadFont() {
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

void UIManager::setupViews() {
    uiView.setCenter(sf::Vector2f(windowSize.x / 2.0f, windowSize.y / 2.0f));
    uiView.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
}

bool UIManager::initializeFont() {
    if (!fontLoaded) {
        loadFont();
        if (fontLoaded && !gameInfoDisplay) {
            gameInfoDisplay = std::make_unique<GameInfoDisplay>(font, windowSize);
        }
    }
    return fontLoaded;
}

void UIManager::update(GameState currentState, VehicleManager* vehicleManager,
    SplitScreenManager* splitScreenManager, Player* localPlayer,
    const std::vector<Planet*>& planets, NetworkManager* networkManager) {

    if (!showUI || !fontLoaded || !gameInfoDisplay) return;

    gameInfoDisplay->updateAllPanels(currentState, vehicleManager, splitScreenManager,
        localPlayer, planets, networkManager);
}

void UIManager::draw(sf::RenderWindow& window, GameState currentState, NetworkManager* networkManager) {
    if (!showUI || !fontLoaded || !gameInfoDisplay) return;

    // Set UI view for drawing
    setUIView(window);

    // Draw all panels
    gameInfoDisplay->drawAllPanels(window, currentState, networkManager);
}

void UIManager::handleWindowResize(sf::Vector2u newSize) {
    windowSize = newSize;
    setupViews();

    if (gameInfoDisplay) {
        gameInfoDisplay->repositionPanels(newSize);
    }
}

void UIManager::setUIView(sf::RenderWindow& window) {
    window.setView(uiView);
}

sf::Vector2f UIManager::getMousePosition(sf::RenderWindow& window) const {
    return window.mapPixelToCoords(sf::Mouse::getPosition(window), uiView);
}

void UIManager::drawFuelCollectionLines(sf::RenderWindow& window, Rocket* rocket) const {
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

void UIManager::drawMultipleFuelLines(sf::RenderWindow& window, VehicleManager* vm1, VehicleManager* vm2) const {
    // Draw fuel collection lines for multiple vehicle managers
    if (vm1 && vm1->getActiveVehicleType() == VehicleType::ROCKET) {
        drawFuelCollectionLines(window, vm1->getRocket());
    }

    if (vm2 && vm2->getActiveVehicleType() == VehicleType::ROCKET) {
        drawFuelCollectionLines(window, vm2->getRocket());
    }
}