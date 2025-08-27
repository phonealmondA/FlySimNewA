#pragma once
#include <SFML/Graphics.hpp>
#include "Button.h"
#include <vector>
#include <memory>

enum class MultiplayerMode {
    NONE,
    LOCAL_PC,          // Split-screen on same computer
    LOCAL_AREA,        // LAN multiplayer (auto-discovery)
    ONLINE,            // Online multiplayer (manual IP/port) - NEW: Opens OnlineMultiplayerMenu
    BACK_TO_MAIN       // Return to main menu
};

class MultiplayerMenu {
private:
    std::vector<std::unique_ptr<Button>> buttons;
    sf::RectangleShape background;
    sf::Font font;
    sf::Text titleText;
    MultiplayerMode selectedMode;
    bool isActive;
    bool fontLoaded;
    sf::Vector2u windowSize;

    // Button callbacks
    void onLocalPCClicked();
    void onLocalAreaClicked();
    void onOnlineClicked();  // Now opens OnlineMultiplayerMenu
    void onBackClicked();

    // Helper methods
    void createButtons();
    void setupBackground();
    void setupTitle();
    void loadFont();

public:
    MultiplayerMenu(sf::Vector2u winSize);
    ~MultiplayerMenu() = default;

    void handleEvent(const sf::Event& event, const sf::Vector2f& mousePos);
    void update(const sf::Vector2f& mousePos);
    void draw(sf::RenderWindow& window);

    // Getters
    MultiplayerMode getSelectedMode() const { return selectedMode; }
    bool getIsActive() const { return isActive; }

    // Control methods
    void show() { isActive = true; selectedMode = MultiplayerMode::NONE; }
    void hide() { isActive = false; }
    void reset() { selectedMode = MultiplayerMode::NONE; }
};