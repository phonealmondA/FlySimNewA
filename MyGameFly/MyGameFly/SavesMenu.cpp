// SavesMenu.cpp - Save Game Testing Menu Implementation
#include "SavesMenu.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>

// Suppress deprecation warnings for localtime on Windows
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

SavesMenu::SavesMenu(sf::Vector2u winSize)
    : selectedSaveType(SaveType::NONE), isActive(true), windowSize(winSize)
{
    if (!loadFont()) {
        std::cerr << "Warning: Could not load font file for saves menu. Text won't display correctly." << std::endl;
    }

    setupBackground();
    createButtons();
}

bool SavesMenu::loadFont() {
    // Try to load font - same paths as MainMenu
    bool fontLoaded = false;

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

    return fontLoaded;
}

void SavesMenu::setupBackground() {
    background.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    background.setFillColor(sf::Color(20, 20, 40, 200)); // Dark blue semi-transparent
    background.setPosition(sf::Vector2f(0.0f, 0.0f));
}

void SavesMenu::createButtons() {
    float centerX = static_cast<float>(windowSize.x) / 2.0f;
    float startY = static_cast<float>(windowSize.y) / 2.0f - 120.0f; // Start higher for 4 buttons
    float buttonWidth = 200.0f;
    float buttonHeight = 50.0f;
    float spacing = 70.0f;

    buttons.clear();

    try {
        // Manual Save Button
        auto manualSaveButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth / 2, startY),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Manual Save",
            font,
            [this]() { onManualSaveClicked(); }
        );
        buttons.push_back(std::move(manualSaveButton));

        // Auto Save Button
        auto autoSaveButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth / 2, startY + spacing),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Auto Save",
            font,
            [this]() { onAutoSaveClicked(); }
        );
        buttons.push_back(std::move(autoSaveButton));

        // Quick Save Button
        auto quickSaveButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth / 2, startY + spacing * 2),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Quick Save",
            font,
            [this]() { onQuickSaveClicked(); }
        );
        buttons.push_back(std::move(quickSaveButton));

        // Back Button
        auto backButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth / 2, startY + spacing * 3),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Back",
            font,
            [this]() { onBackClicked(); }
        );
        buttons.push_back(std::move(backButton));

    }
    catch (const std::exception& e) {
        std::cerr << "Error creating buttons: " << e.what() << std::endl;
    }
}

void SavesMenu::onManualSaveClicked() {
    std::cout << "=== TESTING MANUAL SAVE ===" << std::endl;

    // Generate timestamp-based save name
    auto now = std::time(nullptr);
    std::stringstream ss;
#ifdef _WIN32
    auto* timeinfo = std::localtime(&now);
    ss << "manual_" << std::put_time(timeinfo, "%Y%m%d_%H%M%S");
#else
    ss << "manual_" << std::put_time(std::localtime(&now), "%Y%m%d_%H%M%S");
#endif

    bool success = performManualSave(ss.str());
    printSaveResults(success, "Manual Save");

    selectedSaveType = SaveType::MANUAL_SAVE;
}

void SavesMenu::onAutoSaveClicked() {
    std::cout << "=== TESTING AUTO SAVE ===" << std::endl;

    bool success = performAutoSave();
    printSaveResults(success, "Auto Save");

    selectedSaveType = SaveType::AUTO_SAVE;
}

void SavesMenu::onQuickSaveClicked() {
    std::cout << "=== TESTING QUICK SAVE ===" << std::endl;

    bool success = performQuickSave();
    printSaveResults(success, "Quick Save");

    selectedSaveType = SaveType::QUICK_SAVE;
}

void SavesMenu::onBackClicked() {
    isActive = false;
    selectedSaveType = SaveType::NONE;
}

GameSaveData SavesMenu::generateTestSaveData() const {
    GameSaveData testData;

    // Set basic metadata
    testData.saveName = "Test Save";
    testData.playerName = "Test Player";
    testData.gameMode = SavedGameMode::SINGLE_PLAYER;
    testData.gameTime = 123.45f;
    testData.currentPlayerCount = 1;

    // Add test planets
    testData.planets.push_back(SavedPlanetData(
        sf::Vector2f(400.0f, 300.0f), 5000.0f, sf::Color::Blue, true, 30.0f
    ));
    testData.planets.push_back(SavedPlanetData(
        sf::Vector2f(600.0f, 400.0f), 2000.0f, sf::Color::Red, false, 20.0f
    ));

    // Add test player
    PlayerState testPlayer;
    testPlayer.playerID = 1;
    testPlayer.position = sf::Vector2f(450.0f, 350.0f);
    testPlayer.velocity = sf::Vector2f(0.0f, 0.0f);
    testPlayer.rotation = 0.0f;
    testPlayer.isRocket = true;
    testPlayer.isOnGround = false;
    testPlayer.mass = 100.0f;
    testPlayer.thrustLevel = 0.0f;
    testPlayer.currentFuel = 80.0f;
    testPlayer.maxFuel = 100.0f;
    testPlayer.isCollectingFuel = false;
    testPlayer.isSatellite = false;
    testPlayer.satelliteID = -1;
    testPlayer.orbitAccuracy = 0.0f;
    testPlayer.needsMaintenance = false;
    testPlayer.requestingSatelliteConversion = false;
    testPlayer.newSatelliteID = -1;
    testPlayer.newRocketSpawnPos = sf::Vector2f(0.0f, 0.0f);

    testData.players.push_back(testPlayer);

    // Set camera data
    testData.camera.position = sf::Vector2f(500.0f, 400.0f);
    testData.camera.size = sf::Vector2f(800.0f, 600.0f);
    testData.camera.zoom = 1.0f;
    testData.camera.followingPlayerID = 1;

    // Set UI settings to defaults
    testData.uiSettings.showUI = true;
    testData.uiSettings.showDebugInfo = false;
    testData.uiSettings.showOrbitPaths = true;
    testData.uiSettings.showFuelRings = true;
    testData.uiSettings.showPlayerLabels = true;

    // Set game settings
    testData.enableAutomaticMaintenance = true;
    testData.enableAutomaticCollection = true;
    testData.globalOrbitTolerance = 5.0f;
    testData.collectionEfficiency = 1.0f;

    return testData;
}

bool SavesMenu::performManualSave(const std::string& saveName) {
    GameSaveData testData = generateTestSaveData();
    testData.saveName = saveName;

    return saveManager.saveGame(testData, saveName);
}

bool SavesMenu::performAutoSave() {
    GameSaveData testData = generateTestSaveData();
    testData.saveName = "Auto Save";

    return saveManager.autoSave(testData);
}

bool SavesMenu::performQuickSave() {
    GameSaveData testData = generateTestSaveData();
    testData.saveName = "Quick Save";

    return saveManager.quickSave(testData);
}

void SavesMenu::printSaveResults(bool success, const std::string& saveType) const {
    if (success) {
        std::cout << saveType << " completed successfully!" << std::endl;

        // Print current save files
        auto saveFiles = getSaveFileList();
        std::cout << "Current save files (" << saveFiles.size() << "):" << std::endl;
        for (const auto& file : saveFiles) {
            std::cout << "  - " << file << std::endl;
        }
    }
    else {
        std::cout << saveType << " FAILED!" << std::endl;
    }
    std::cout << "========================" << std::endl;
}

std::vector<std::string> SavesMenu::getSaveFileList() const {
    return saveManager.getSaveFileList();
}

void SavesMenu::handleEvent(const sf::Event& event, const sf::Vector2f& mousePos) {
    if (!isActive) return;

    if (event.is<sf::Event::MouseButtonPressed>()) {
        const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>();
        if (mouseEvent && mouseEvent->button == sf::Mouse::Button::Left) {
            // Check which button was clicked
            for (auto& button : buttons) {
                if (button->contains(mousePos)) {
                    button->handleClick();
                    break;
                }
            }
        }
    }
}

void SavesMenu::update(const sf::Vector2f& mousePos) {
    if (!isActive) return;

    for (auto& button : buttons) {
        if (button) {
            button->update(mousePos);
        }
    }
}

void SavesMenu::draw(sf::RenderWindow& window) {
    if (!isActive) return;

    // Draw background
    window.draw(background);

    // Draw title - SFML 3.0 compatible
    sf::Text title(font);
    title.setString("Save System Testing");
    title.setCharacterSize(36);
    title.setFillColor(sf::Color::White);
    sf::FloatRect titleBounds = title.getLocalBounds();
    title.setPosition(sf::Vector2f(
        static_cast<float>(windowSize.x) / 2.0f - titleBounds.size.x / 2.0f,
        50.0f
    ));
    window.draw(title);

    // Draw buttons
    for (const auto& button : buttons) {
        if (button) {
            button->draw(window);
        }
    }

    // Draw instructions - SFML 3.0 compatible
    sf::Text instructions(font);
    instructions.setString("Click buttons to test different save types");
    instructions.setCharacterSize(16);
    instructions.setFillColor(sf::Color(200, 200, 200));
    sf::FloatRect instrBounds = instructions.getLocalBounds();
    instructions.setPosition(sf::Vector2f(
        static_cast<float>(windowSize.x) / 2.0f - instrBounds.size.x / 2.0f,
        static_cast<float>(windowSize.y) - 100.0f
    ));
    window.draw(instructions);
}

#ifdef _WIN32
#pragma warning(pop)
#endif