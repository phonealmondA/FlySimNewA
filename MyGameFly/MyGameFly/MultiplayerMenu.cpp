#include "MultiplayerMenu.h"
#include <iostream>

MultiplayerMenu::MultiplayerMenu(sf::Vector2u winSize)
    : selectedMode(MultiplayerMode::NONE), isActive(false), fontLoaded(false), windowSize(winSize), titleText(font)
{
    loadFont();
    setupBackground();
    setupTitle();
    createButtons();
}

void MultiplayerMenu::loadFont() {
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
        std::cerr << "Warning: Could not load font file for multiplayer menu." << std::endl;
    }
}

void MultiplayerMenu::setupBackground() {
    background.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    background.setFillColor(sf::Color(30, 30, 60, 220)); // Slightly different blue than main menu
    background.setPosition(sf::Vector2f(0.0f, 0.0f));
}

void MultiplayerMenu::setupTitle() {
    if (fontLoaded) {
        titleText.setString("MULTIPLAYER OPTIONS");
        titleText.setCharacterSize(36);
        titleText.setFillColor(sf::Color::White);

        // Center the title
        sf::FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setPosition(sf::Vector2f(
            static_cast<float>(windowSize.x) / 2.0f - titleBounds.size.x / 2.0f - titleBounds.position.x,
            100.0f
        ));
    }
}

void MultiplayerMenu::createButtons() {
    if (!fontLoaded) {
        std::cerr << "Cannot create buttons without font" << std::endl;
        return;
    }

    float centerX = static_cast<float>(windowSize.x) / 2.0f;
    float startY = static_cast<float>(windowSize.y) / 2.0f - 80.0f;
    float buttonWidth = 250.0f;
    float buttonHeight = 50.0f;
    float spacing = 70.0f;

    try {
        // Local PC Button (Split-screen)
        auto localPCButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth / 2, startY),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Local PC (Split-Screen)",
            font,
            [this]() { onLocalPCClicked(); }
        );
        buttons.push_back(std::move(localPCButton));

        // Local Area Network Button (LAN)
        auto localAreaButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth / 2, startY + spacing),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Local Area Network",
            font,
            [this]() { onLocalAreaClicked(); }
        );
        buttons.push_back(std::move(localAreaButton));

        // Online Multiplayer Button
        auto onlineButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth / 2, startY + spacing * 2),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Online Multiplayer",
            font,
            [this]() { onOnlineClicked(); }
        );
        buttons.push_back(std::move(onlineButton));

        // Back Button
        auto backButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth / 2, startY + spacing * 3.5f),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Back to Main Menu",
            font,
            [this]() { onBackClicked(); }
        );
        buttons.push_back(std::move(backButton));
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating multiplayer menu buttons: " << e.what() << std::endl;
    }
}

void MultiplayerMenu::onLocalPCClicked() {
    selectedMode = MultiplayerMode::LOCAL_PC;
    isActive = false;
    std::cout << "Local PC (Split-Screen) mode selected" << std::endl;
}

void MultiplayerMenu::onLocalAreaClicked() {
    selectedMode = MultiplayerMode::LOCAL_AREA;
    isActive = false;
    std::cout << "Local Area Network mode selected (placeholder)" << std::endl;
}

void MultiplayerMenu::onOnlineClicked() {
    selectedMode = MultiplayerMode::ONLINE;
    isActive = false;
    std::cout << "Online Multiplayer mode selected (placeholder)" << std::endl;
}

void MultiplayerMenu::onBackClicked() {
    selectedMode = MultiplayerMode::BACK_TO_MAIN;
    isActive = false;
    std::cout << "Returning to main menu" << std::endl;
}

void MultiplayerMenu::handleEvent(const sf::Event& event, const sf::Vector2f& mousePos) {
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

void MultiplayerMenu::update(const sf::Vector2f& mousePos) {
    if (!isActive) return;

    // Update all buttons (for hover effects)
    for (auto& button : buttons) {
        button->update(mousePos);
    }
}

void MultiplayerMenu::draw(sf::RenderWindow& window) {
    if (!isActive) return;

    // Draw background
    window.draw(background);

    // Draw title
    if (fontLoaded) {
        window.draw(titleText);
    }

    // Draw all buttons
    for (auto& button : buttons) {
        button->draw(window);
    }
}