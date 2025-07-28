#include "MainMenu.h"
#include <iostream>

MainMenu::MainMenu(sf::Vector2u winSize)
    : selectedMode(GameMode::NONE), isActive(true), windowSize(winSize)
{
    // For now, we'll skip font loading since your Button class doesn't use text yet
    // The buttons will be colored rectangles with different colors to distinguish them

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

    // Single Player Button (Green)
    auto singlePlayerButton = std::make_unique<Button>(
        sf::Vector2f(centerX - buttonWidth / 2, startY),
        sf::Vector2f(buttonWidth, buttonHeight),
        "Single Player", // Text (not displayed yet)
        font, // Font (not used yet)
        [this]() { onSinglePlayerClicked(); }
    );
    // We'll need to modify the button color - for now it uses default
    buttons.push_back(std::move(singlePlayerButton));

    // Multiplayer Button (Blue)
    auto multiplayerButton = std::make_unique<Button>(
        sf::Vector2f(centerX - buttonWidth / 2, startY + spacing),
        sf::Vector2f(buttonWidth, buttonHeight),
        "Multiplayer",
        font,
        [this]() { onMultiplayerClicked(); }
    );
    buttons.push_back(std::move(multiplayerButton));

    // Quit Button (Red)
    auto quitButton = std::make_unique<Button>(
        sf::Vector2f(centerX - buttonWidth / 2, startY + spacing * 2),
        sf::Vector2f(buttonWidth, buttonHeight),
        "Quit",
        font,
        [this]() { onQuitClicked(); }
    );
    buttons.push_back(std::move(quitButton));
}

void MainMenu::onSinglePlayerClicked() {
    selectedMode = GameMode::SINGLE_PLAYER;
    isActive = false;
    std::cout << "Single Player mode selected" << std::endl;
}

void MainMenu::onMultiplayerClicked() {
    selectedMode = GameMode::MULTIPLAYER_HOST; // For now, default to host
    isActive = false;
    std::cout << "Multiplayer mode selected" << std::endl;
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

    // TODO: Add title text when font system is working
    // For now, the colored buttons should be enough to identify:
    // Top button = Single Player
    // Middle button = Multiplayer  
    // Bottom button = Quit
}