#include "MainMenu.h"
#include <iostream>

MainMenu::MainMenu(sf::Vector2u winSize)
    : selectedMode(GameMode::NONE), isActive(true), windowSize(winSize)
{
    // Load font - try the same paths as in main.cpp
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

    if (!fontLoaded) {
        std::cerr << "Warning: Could not load font file for menu. Text won't display correctly." << std::endl;
        // Create a dummy font or handle the error appropriately
        // For now, we'll continue but buttons won't have text
    }

    setupBackground();
    createButtons();
}

void MainMenu::setupBackground() {
    background.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    background.setFillColor(sf::Color(20, 20, 40, 200)); // Dark blue semi-transparent
    background.setPosition(sf::Vector2f(0.0f, 0.0f));
}

void MainMenu::createButtons() {
    float centerX = static_cast<float>(windowSize.x) / 2.0f;
    float startY = static_cast<float>(windowSize.y) / 2.0f - 100.0f; // Start a bit above center
    float buttonWidth = 200.0f;
    float buttonHeight = 50.0f;
    float spacing = 70.0f;

    // Only create buttons if we have a valid font
    try {
        // Single Player Button
        auto singlePlayerButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth / 2, startY),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Single Player",
            font,
            [this]() { onSinglePlayerClicked(); }
        );
        buttons.push_back(std::move(singlePlayerButton));

        // Multiplayer Button
        auto multiplayerButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth / 2, startY + spacing),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Multiplayer",
            font,
            [this]() { onMultiplayerClicked(); }
        );
        buttons.push_back(std::move(multiplayerButton));

        // Quit Button
        auto quitButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth / 2, startY + spacing * 2),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Quit",
            font,
            [this]() { onQuitClicked(); }
        );
        buttons.push_back(std::move(quitButton));
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating buttons: " << e.what() << std::endl;
        // If button creation fails, we could create simple colored rectangles instead
    }
}

void MainMenu::onSinglePlayerClicked() {
    selectedMode = GameMode::SINGLE_PLAYER;
    isActive = false;
    std::cout << "Single Player mode selected" << std::endl;
}

void MainMenu::onMultiplayerClicked() {
    selectedMode = GameMode::MULTIPLAYER_HOST; // This will trigger the multiplayer submenu
    isActive = false;
    std::cout << "Opening multiplayer menu..." << std::endl;
}

void MainMenu::onQuitClicked() {
    selectedMode = GameMode::QUIT;
    isActive = false;
    std::cout << "Quit selected" << std::endl;
}

void MainMenu::handleEvent(const sf::Event& event, const sf::Vector2f& mousePos) {
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

void MainMenu::update(const sf::Vector2f& mousePos) {
    if (!isActive) return;

    // Update all buttons (for hover effects)
    for (auto& button : buttons) {
        button->update(mousePos);
    }
}

void MainMenu::draw(sf::RenderWindow& window) {
    if (!isActive) return;

    // Draw background
    window.draw(background);

    // Draw all buttons
    for (auto& button : buttons) {
        button->draw(window);
    }
}