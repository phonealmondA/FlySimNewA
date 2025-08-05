#include "OnlineMultiplayerMenu.h"
#include <iostream>
#include <sstream>
#include <regex>

const std::string OnlineMultiplayerMenu::DEFAULT_IP = "127.0.0.1";

OnlineMultiplayerMenu::OnlineMultiplayerMenu(sf::Vector2u winSize)
    : selectedMode(OnlineMode::NONE), isActive(false), fontLoaded(false), windowSize(winSize),
    titleText(font), ipText(font), portText(font), statusText(font),
    ipInputText(font), portInputText(font),
    currentIP(DEFAULT_IP), currentPort(std::to_string(DEFAULT_PORT)),
    ipInputActive(false), portInputActive(false), isHost(false)
{
    loadFont();
    setupBackground();
    setupTitle();
    createInputFields();
    createButtons();
    updateInputDisplay();
}

void OnlineMultiplayerMenu::loadFont() {
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
        std::cerr << "Warning: Could not load font file for online multiplayer menu." << std::endl;
    }
}

void OnlineMultiplayerMenu::setupBackground() {
    background.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    background.setFillColor(sf::Color(40, 20, 60, 220)); // Purple tint to distinguish from other menus
    background.setPosition(sf::Vector2f(0.0f, 0.0f));
}

void OnlineMultiplayerMenu::setupTitle() {
    if (fontLoaded) {
        titleText.setString("ONLINE MULTIPLAYER");
        titleText.setCharacterSize(36);
        titleText.setFillColor(sf::Color::White);

        // Center the title
        sf::FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setPosition(sf::Vector2f(
            static_cast<float>(windowSize.x) / 2.0f - titleBounds.size.x / 2.0f - titleBounds.position.x,
            80.0f
        ));
    }
}

void OnlineMultiplayerMenu::createInputFields() {
    if (!fontLoaded) return;

    float centerX = static_cast<float>(windowSize.x) / 2.0f;
    float startY = 180.0f;
    float inputWidth = 200.0f;
    float inputHeight = 30.0f;
    float spacing = 60.0f;

    // IP Address label and input
    ipText.setString("Server IP Address:");
    ipText.setCharacterSize(16);
    ipText.setFillColor(sf::Color::White);
    sf::FloatRect ipTextBounds = ipText.getLocalBounds();
    ipText.setPosition(sf::Vector2f(centerX - inputWidth / 2.0f, startY - 25.0f));

    ipInputBox.setSize(sf::Vector2f(inputWidth, inputHeight));
    ipInputBox.setPosition(sf::Vector2f(centerX - inputWidth / 2.0f, startY));
    ipInputBox.setFillColor(sf::Color(60, 60, 60, 200));
    ipInputBox.setOutlineColor(sf::Color::White);
    ipInputBox.setOutlineThickness(1.0f);

    ipInputText.setCharacterSize(14);
    ipInputText.setFillColor(sf::Color::White);
    ipInputText.setPosition(sf::Vector2f(centerX - inputWidth / 2.0f + 5.0f, startY + 5.0f));

    // Port label and input
    portText.setString("Port:");
    portText.setCharacterSize(16);
    portText.setFillColor(sf::Color::White);
    portText.setPosition(sf::Vector2f(centerX - inputWidth / 2.0f, startY + spacing - 25.0f));

    portInputBox.setSize(sf::Vector2f(inputWidth, inputHeight));
    portInputBox.setPosition(sf::Vector2f(centerX - inputWidth / 2.0f, startY + spacing));
    portInputBox.setFillColor(sf::Color(60, 60, 60, 200));
    portInputBox.setOutlineColor(sf::Color::White);
    portInputBox.setOutlineThickness(1.0f);

    portInputText.setCharacterSize(14);
    portInputText.setFillColor(sf::Color::White);
    portInputText.setPosition(sf::Vector2f(centerX - inputWidth / 2.0f + 5.0f, startY + spacing + 5.0f));

    // Status text
    statusText.setCharacterSize(14);
    statusText.setFillColor(sf::Color::Yellow);
    statusText.setPosition(sf::Vector2f(centerX - 150.0f, startY + spacing * 2.0f));
    statusText.setString("Enter IP for Hamachi/VPN or LAN address");
}

void OnlineMultiplayerMenu::createButtons() {
    if (!fontLoaded) {
        std::cerr << "Cannot create buttons without font" << std::endl;
        return;
    }

    float centerX = static_cast<float>(windowSize.x) / 2.0f;
    float startY = 350.0f;
    float buttonWidth = 150.0f;
    float buttonHeight = 50.0f;
    float spacing = 70.0f;

    try {
        // Host Game Button
        auto hostButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth - 10.0f, startY),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Host Game",
            font,
            [this]() { onHostClicked(); }
        );
        buttons.push_back(std::move(hostButton));

        // Join Game Button
        auto joinButton = std::make_unique<Button>(
            sf::Vector2f(centerX + 10.0f, startY),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Join Game",
            font,
            [this]() { onJoinClicked(); }
        );
        buttons.push_back(std::move(joinButton));

        // Back Button
        auto backButton = std::make_unique<Button>(
            sf::Vector2f(centerX - buttonWidth / 2.0f, startY + spacing),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Back",
            font,
            [this]() { onBackClicked(); }
        );
        buttons.push_back(std::move(backButton));
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating online multiplayer menu buttons: " << e.what() << std::endl;
    }
}

void OnlineMultiplayerMenu::onHostClicked() {
    if (isValidPort(currentPort)) {
        selectedMode = OnlineMode::HOST_GAME;
        isHost = true;  // Set host flag
        isActive = false;
        std::cout << "Host Game selected on port " << currentPort << std::endl;
    }
    else {
        setStatusMessage("Invalid port number!");
    }
}

void OnlineMultiplayerMenu::onJoinClicked() {
    if (isValidIP(currentIP) && isValidPort(currentPort)) {
        selectedMode = OnlineMode::JOIN_GAME;
        isHost = false;  // Set client flag
        isActive = false;
        std::cout << "Join Game selected: " << currentIP << ":" << currentPort << std::endl;
    }
    else {
        if (!isValidIP(currentIP)) {
            setStatusMessage("Invalid IP address!");
        }
        else {
            setStatusMessage("Invalid port number!");
        }
    }
}

void OnlineMultiplayerMenu::onBackClicked() {
    selectedMode = OnlineMode::BACK_TO_MULTIPLAYER;
    isActive = false;
    std::cout << "Returning to multiplayer menu" << std::endl;
}

void OnlineMultiplayerMenu::handleEvent(const sf::Event& event, const sf::Vector2f& mousePos) {
    if (!isActive) return;

    if (event.is<sf::Event::MouseButtonPressed>()) {
        const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>();
        if (mouseEvent && mouseEvent->button == sf::Mouse::Button::Left) {
            // Check if input boxes were clicked
            if (ipInputBox.getGlobalBounds().contains(mousePos)) {
                ipInputActive = true;
                portInputActive = false;
                updateInputBoxColors();
                return;
            }
            else if (portInputBox.getGlobalBounds().contains(mousePos)) {
                portInputActive = true;
                ipInputActive = false;
                updateInputBoxColors();
                return;
            }
            else {
                // Clicked elsewhere, deactivate input boxes
                ipInputActive = false;
                portInputActive = false;
                updateInputBoxColors();
            }

            // Check which button was clicked
            for (auto& button : buttons) {
                if (button->contains(mousePos)) {
                    button->handleClick();
                    break;
                }
            }
        }
    }

    if (event.is<sf::Event::TextEntered>()) {
        const auto* textEvent = event.getIf<sf::Event::TextEntered>();
        if (textEvent) {
            handleTextInput(static_cast<char>(textEvent->unicode));
        }
    }

    if (event.is<sf::Event::KeyPressed>()) {
        const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
        if (keyEvent) {
            if (keyEvent->code == sf::Keyboard::Key::Tab) {
                // Tab to switch between input fields
                if (ipInputActive) {
                    ipInputActive = false;
                    portInputActive = true;
                }
                else if (portInputActive) {
                    portInputActive = false;
                    ipInputActive = true;
                }
                else {
                    ipInputActive = true;
                }
                updateInputBoxColors();
            }
            else if (keyEvent->code == sf::Keyboard::Key::Enter) {
                // Enter to join game (if IP input is valid)
                if (isValidIP(currentIP) && isValidPort(currentPort)) {
                    onJoinClicked();
                }
            }
            else if (keyEvent->code == sf::Keyboard::Key::Backspace) {
                // Handle backspace
                if (ipInputActive && !currentIP.empty()) {
                    currentIP.pop_back();
                    updateInputDisplay();
                }
                else if (portInputActive && !currentPort.empty()) {
                    currentPort.pop_back();
                    updateInputDisplay();
                }
            }
        }
    }
}

void OnlineMultiplayerMenu::handleTextInput(char character) {
    // Only handle printable characters
    if (character < 32 || character > 126) return;

    if (ipInputActive) {
        // Allow numbers, dots, and basic characters for IP
        if ((character >= '0' && character <= '9') || character == '.') {
            if (currentIP.length() < 15) { // Max IP length
                currentIP += character;
                updateInputDisplay();
                statusText.setString(""); // Clear error message
            }
        }
    }
    else if (portInputActive) {
        // Allow only numbers for port
        if (character >= '0' && character <= '9') {
            if (currentPort.length() < 5) { // Max port length (65535)
                currentPort += character;
                updateInputDisplay();
                statusText.setString(""); // Clear error message
            }
        }
    }
}

void OnlineMultiplayerMenu::updateInputBoxColors() {
    if (ipInputActive) {
        ipInputBox.setOutlineColor(sf::Color::Cyan);
        ipInputBox.setOutlineThickness(2.0f);
    }
    else {
        ipInputBox.setOutlineColor(sf::Color::White);
        ipInputBox.setOutlineThickness(1.0f);
    }

    if (portInputActive) {
        portInputBox.setOutlineColor(sf::Color::Cyan);
        portInputBox.setOutlineThickness(2.0f);
    }
    else {
        portInputBox.setOutlineColor(sf::Color::White);
        portInputBox.setOutlineThickness(1.0f);
    }
}

void OnlineMultiplayerMenu::updateInputDisplay() {
    ipInputText.setString(currentIP);
    portInputText.setString(currentPort);
}

bool OnlineMultiplayerMenu::isValidIP(const std::string& ip) const {
    if (ip.empty()) return false;

    // Simple regex for IP validation (including Hamachi ranges)
    std::regex ipRegex(R"(^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    return std::regex_match(ip, ipRegex);
}

bool OnlineMultiplayerMenu::isValidPort(const std::string& port) const {
    if (port.empty()) return false;

    try {
        int portNum = std::stoi(port);
        return portNum > 0 && portNum <= 65535;
    }
    catch (...) {
        return false;
    }
}


ConnectionInfo OnlineMultiplayerMenu::getConnectionInfo() const {
    ConnectionInfo info;
    info.ipAddress = currentIP;
    info.isHost = (selectedMode == OnlineMode::HOST_GAME);

    try {
        info.port = static_cast<unsigned short>(std::stoi(currentPort));
    }
    catch (...) {
        info.port = DEFAULT_PORT;
    }

    return info;
}

void OnlineMultiplayerMenu::setStatusMessage(const std::string& message) {
    statusText.setString(message);
    statusText.setFillColor(sf::Color::Red);
}

void OnlineMultiplayerMenu::update(const sf::Vector2f& mousePos) {
    if (!isActive) return;

    // Update all buttons (for hover effects)
    for (auto& button : buttons) {
        button->update(mousePos);
    }
}

void OnlineMultiplayerMenu::draw(sf::RenderWindow& window) {
    if (!isActive) return;

    // Draw background
    window.draw(background);

    if (fontLoaded) {
        // Draw title
        window.draw(titleText);

        // Draw labels
        window.draw(ipText);
        window.draw(portText);

        // Draw input boxes
        window.draw(ipInputBox);
        window.draw(portInputBox);

        // Draw input text
        window.draw(ipInputText);
        window.draw(portInputText);

        // Draw status text
        window.draw(statusText);
    }

    // Draw all buttons
    for (auto& button : buttons) {
        button->draw(window);
    }
}