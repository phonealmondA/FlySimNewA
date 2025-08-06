#pragma once
#include <SFML/Graphics.hpp>
#include "Button.h"
#include <vector>
#include <memory>
#include <functional>

enum class GameMode {
    NONE,
    SINGLE_PLAYER,
    LOAD_GAME,
    MULTIPLAYER_HOST,
    MULTIPLAYER_JOIN,
    QUIT
};

class MainMenu {
private:
    std::vector<std::unique_ptr<Button>> buttons;
    sf::RectangleShape background;
    sf::Font font; // We'll handle font loading
    GameMode selectedMode;
    bool isActive;
    sf::Vector2u windowSize;

    // Button callbacks
    void onSinglePlayerClicked();
    void onMultiplayerClicked();
    void onQuitClicked();
    void onLoadGameClicked();

    // Helper methods
    void createButtons();
    void setupBackground();

public:
    MainMenu(sf::Vector2u winSize);
    ~MainMenu() = default;

    void handleEvent(const sf::Event& event, const sf::Vector2f& mousePos);
    void update(const sf::Vector2f& mousePos);
    void draw(sf::RenderWindow& window);

    // Getters
    GameMode getSelectedMode() const { return selectedMode; }
    bool getIsActive() const { return isActive; }

    // Control methods
    void show() { isActive = true; selectedMode = GameMode::NONE; }
    void hide() { isActive = false; }
    void reset() { selectedMode = GameMode::NONE; }
};