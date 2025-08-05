#pragma once
#include <SFML/Graphics.hpp>
#include "Button.h"
#include <vector>
#include <memory>
#include <string>

enum class OnlineMode {
    NONE,
    HOST_GAME,         // Host a game on specified port
    JOIN_GAME,         // Join a game at specified IP:port
    BACK_TO_MULTIPLAYER // Return to multiplayer menu
};

struct ConnectionInfo {
    std::string ipAddress;
    unsigned short port;
    bool isHost = false;
    bool isValid() const {
        return !ipAddress.empty() && port > 0;
    }
};

class OnlineMultiplayerMenu {
private:
    std::vector<std::unique_ptr<Button>> buttons;
    sf::RectangleShape background;
    sf::Font font;
    sf::Text titleText;
    sf::Text ipText;
    sf::Text portText;
    sf::Text statusText;
    bool isHost;  // Add this line

    // Input fields
    sf::RectangleShape ipInputBox;
    sf::RectangleShape portInputBox;
    sf::Text ipInputText;
    sf::Text portInputText;

    OnlineMode selectedMode;
    bool isActive;
    bool fontLoaded;
    sf::Vector2u windowSize;

    // Input handling
    std::string currentIP;
    std::string currentPort;
    bool ipInputActive;
    bool portInputActive;

    // Default values
    static const std::string DEFAULT_IP;
    static const unsigned short DEFAULT_PORT = 54000;

    // Button callbacks
    void onHostClicked();
    void onJoinClicked();
    void onBackClicked();

    // Helper methods
    void createButtons();
    void createInputFields();
    void setupBackground();
    void setupTitle();
    void loadFont();
    void updateInputBoxColors();
    void handleTextInput(char character);
    bool isValidIP(const std::string& ip) const;
    bool isValidPort(const std::string& port) const;

public:
    OnlineMultiplayerMenu(sf::Vector2u winSize);
    ~OnlineMultiplayerMenu() = default;

    void handleEvent(const sf::Event& event, const sf::Vector2f& mousePos);
    void update(const sf::Vector2f& mousePos);
    void draw(sf::RenderWindow& window);

    // Getters
    OnlineMode getSelectedMode() const { return selectedMode; }
    bool getIsActive() const { return isActive; }
    ConnectionInfo getConnectionInfo() const;

    // Control methods
    void show() {
        isActive = true;
        selectedMode = OnlineMode::NONE;
        // Reset to defaults when showing
        currentIP = DEFAULT_IP;
        currentPort = std::to_string(DEFAULT_PORT);
        updateInputDisplay();
    }
    void hide() { isActive = false; }
    void reset() { selectedMode = OnlineMode::NONE; }

    // Status display
    void setStatusMessage(const std::string& message);

private:
    void updateInputDisplay();
};