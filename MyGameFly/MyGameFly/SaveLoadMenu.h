#pragma once
#ifndef SAVE_LOAD_MENU_H
#define SAVE_LOAD_MENU_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include "SaveGameManager.h"
#include "Button.h"

enum class SaveLoadMenuMode {
    SAVE,
    LOAD
};

enum class SaveLoadAction {
    NONE,
    SAVE_GAME,
    LOAD_GAME,
    DELETE_SAVE,
    NEW_SAVE,
    BACK_TO_GAME,
    BACK_TO_MAIN_MENU
};

class SaveLoadMenu {
private:
    SaveLoadMenuMode mode;
    sf::Vector2u windowSize;
    sf::Font font;
    bool fontLoaded;

    // UI Elements
    sf::RectangleShape background;
    sf::Text titleText;
    sf::Text instructionText;

    // Save file list
    std::vector<SaveFileInfo> saveFiles;
    std::vector<std::unique_ptr<Button>> saveFileButtons;
    std::vector<std::unique_ptr<Button>> deleteButtons;

    // Action buttons
    std::unique_ptr<Button> newSaveButton;
    std::unique_ptr<Button> quickSaveButton;
    std::unique_ptr<Button> quickLoadButton;
    std::unique_ptr<Button> backButton;

    // Scroll and selection
    int selectedSaveIndex;
    float scrollOffset;
    float maxScroll;
    const float itemHeight = 50.0f;
    const float visibleItems = 8.0f;

    // New save input
    bool enteringNewSaveName;
    std::string newSaveName;
    sf::RectangleShape newSaveInputBox;
    sf::Text newSaveInputText;
    sf::Text newSavePromptText;

    // Confirmation dialog
    bool showingConfirmDialog;
    std::string confirmMessage;
    std::unique_ptr<Button> confirmYesButton;
    std::unique_ptr<Button> confirmNoButton;
    SaveLoadAction pendingAction;
    int pendingIndex;

    // Colors and styling
    static const sf::Color BACKGROUND_COLOR;
    static const sf::Color SELECTED_COLOR;
    static const sf::Color HOVER_COLOR;
    static const sf::Color TEXT_COLOR;
    static const sf::Color BUTTON_COLOR;

public:
    SaveLoadMenu(sf::Vector2u windowSize, SaveLoadMenuMode mode = SaveLoadMenuMode::LOAD);
    ~SaveLoadMenu() = default;

    // Core functionality
    void update(sf::Vector2f mousePos, SaveGameManager* saveManager);
    void draw(sf::RenderWindow& window);
    void handleEvent(const sf::Event& event, SaveGameManager* saveManager);

    // Menu management
    void setMode(SaveLoadMenuMode newMode);
    SaveLoadMenuMode getMode() const { return mode; }
    void refreshSaveList(SaveGameManager* saveManager);

    // Input handling
    void handleMouseClick(sf::Vector2f mousePos, SaveGameManager* saveManager);
    void handleKeyPress(sf::Keyboard::Key key, SaveGameManager* saveManager);
    void handleTextInput(sf::Uint32 unicode);
    void handleMouseWheel(float delta);

    // State queries
    SaveLoadAction getLastAction() const { return lastAction; }
    std::string getSelectedSaveFile() const;
    std::string getNewSaveName() const { return newSaveName; }
    void clearAction() { lastAction = SaveLoadAction::NONE; }

    // UI state
    bool isEnteringText() const { return enteringNewSaveName; }
    bool isShowingDialog() const { return showingConfirmDialog; }

private:
    SaveLoadAction lastAction;

    // Setup methods
    void setupBackground();
    void setupTitle();
    void setupButtons();
    void setupNewSaveInput();
    void setupConfirmDialog();

    // Layout methods
    void layoutSaveFileButtons();
    void layoutActionButtons();
    void updateScrollLimits();

    // Rendering helpers
    void drawSaveFileList(sf::RenderWindow& window);
    void drawActionButtons(sf::RenderWindow& window);
    void drawNewSaveInput(sf::RenderWindow& window);
    void drawConfirmDialog(sf::RenderWindow& window);
    void drawScrollbar(sf::RenderWindow& window);

    // Utility methods
    std::string formatFileInfo(const SaveFileInfo& info) const;
    std::string formatGameMode(GameState gameMode) const;
    std::string truncateString(const std::string& str, size_t maxLength) const;
    sf::Color getItemColor(int index, bool isHovered, bool isSelected) const;

    // Input validation
    bool isValidSaveName(const std::string& name) const;
    std::string sanitizeSaveName(const std::string& name) const;

    // Confirmation dialog management
    void showConfirmDialog(const std::string& message, SaveLoadAction action, int index = -1);
    void hideConfirmDialog();
    void executeConfirmedAction(SaveGameManager* saveManager);
};

// Implementation would go in SaveLoadMenu.cpp
// Here's a simplified version of key methods:

// Static color definitions
const sf::Color SaveLoadMenu::BACKGROUND_COLOR = sf::Color(30, 30, 40, 240);
const sf::Color SaveLoadMenu::SELECTED_COLOR = sf::Color(100, 150, 255, 180);
const sf::Color SaveLoadMenu::HOVER_COLOR = sf::Color(70, 70, 90, 160);
const sf::Color SaveLoadMenu::TEXT_COLOR = sf::Color::White;
const sf::Color SaveLoadMenu::BUTTON_COLOR = sf::Color(60, 60, 80, 200);

SaveLoadMenu::SaveLoadMenu(sf::Vector2u windowSize, SaveLoadMenuMode mode)
    : mode(mode), windowSize(windowSize), fontLoaded(false),
    selectedSaveIndex(-1), scrollOffset(0.0f), maxScroll(0.0f),
    enteringNewSaveName(false), showingConfirmDialog(false),
    lastAction(SaveLoadAction::NONE), pendingAction(SaveLoadAction::NONE),
    pendingIndex(-1) {

    // Try to load font (use default if not available)
    fontLoaded = font.loadFromFile("assets/fonts/arial.ttf");
    if (!fontLoaded) {
        std::cout << "SaveLoadMenu: Could not load font, using default" << std::endl;
    }

    setupBackground();
    setupTitle();
    setupButtons();
    setupNewSaveInput();
    setupConfirmDialog();
}

void SaveLoadMenu::setupBackground() {
    background.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    background.setFillColor(BACKGROUND_COLOR);
    background.setPosition(0.0f, 0.0f);
}

void SaveLoadMenu::setupTitle() {
    if (fontLoaded) {
        titleText.setFont(font);
        titleText.setCharacterSize(36);
        titleText.setFillColor(TEXT_COLOR);

        std::string title = (mode == SaveLoadMenuMode::SAVE) ? "SAVE GAME" : "LOAD GAME";
        titleText.setString(title);

        // Center the title
        sf::FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setPosition(
            static_cast<float>(windowSize.x) / 2.0f - titleBounds.size.x / 2.0f - titleBounds.position.x,
            50.0f
        );

        // Setup instruction text
        instructionText.setFont(font);
        instructionText.setCharacterSize(16);
        instructionText.setFillColor(sf::Color(200, 200, 200));

        std::string instruction = (mode == SaveLoadMenuMode::SAVE) ?
            "Click on a save slot or create new save" :
            "Click on a save file to load";
        instructionText.setString(instruction);

        sf::FloatRect instrBounds = instructionText.getLocalBounds();
        instructionText.setPosition(
            static_cast<float>(windowSize.x) / 2.0f - instrBounds.size.x / 2.0f - instrBounds.position.x,
            100.0f
        );
    }
}

void SaveLoadMenu::update(sf::Vector2f mousePos, SaveGameManager* saveManager) {
    if (saveManager) {
        refreshSaveList(saveManager);
    }

    // Update button hover states
    for (auto& button : saveFileButtons) {
        if (button) {
            button->update(mousePos);
        }
    }

    for (auto& button : deleteButtons) {
        if (button) {
            button->update(mousePos);
        }
    }

    if (newSaveButton) newSaveButton->update(mousePos);
    if (quickSaveButton) quickSaveButton->update(mousePos);
    if (quickLoadButton) quickLoadButton->update(mousePos);
    if (backButton) backButton->update(mousePos);

    if (showingConfirmDialog) {
        if (confirmYesButton) confirmYesButton->update(mousePos);
        if (confirmNoButton) confirmNoButton->update(mousePos);
    }
}

std::string SaveLoadMenu::formatFileInfo(const SaveFileInfo& info) const {
    std::stringstream ss;
    ss << info.displayName << " - ";
    ss << formatGameMode(info.gameMode) << " - ";
    ss << info.playerCount << "P - ";
    ss << info.satelliteCount << " sats - ";

    // Format time
    std::time_t saveTime = info.saveTime;
    ss << std::put_time(std::localtime(&saveTime), "%m/%d %H:%M");

    return ss.str();
}

std::string SaveLoadMenu::formatGameMode(GameState gameMode) const {
    switch (gameMode) {
    case GameState::SINGLE_PLAYER: return "Single";
    case GameState::LOCAL_PC_MULTIPLAYER: return "Split";
    case GameState::LAN_MULTIPLAYER: return "LAN";
    case GameState::ONLINE_MULTIPLAYER: return "Online";
    default: return "Unknown";
    }
}

void SaveLoadMenu::handleMouseClick(sf::Vector2f mousePos, SaveGameManager* saveManager) {
    if (showingConfirmDialog) {
        if (confirmYesButton && confirmYesButton->isClicked(mousePos)) {
            executeConfirmedAction(saveManager);
            hideConfirmDialog();
        }
        else if (confirmNoButton && confirmNoButton->isClicked(mousePos)) {
            hideConfirmDialog();
        }
        return;
    }

    if (enteringNewSaveName) {
        // Click outside input box cancels new save
        sf::FloatRect inputBounds = newSaveInputBox.getGlobalBounds();
        if (!inputBounds.contains(mousePos)) {
            enteringNewSaveName = false;
            newSaveName.clear();
        }
        return;
    }

    // Handle save file button clicks
    for (size_t i = 0; i < saveFileButtons.size(); ++i) {
        if (saveFileButtons[i] && saveFileButtons[i]->isClicked(mousePos)) {
            selectedSaveIndex = static_cast<int>(i);

            if (mode == SaveLoadMenuMode::LOAD) {
                lastAction = SaveLoadAction::LOAD_GAME;
            }
            else {
                // For save mode, show overwrite confirmation
                showConfirmDialog("Overwrite this save file?", SaveLoadAction::SAVE_GAME, static_cast<int>(i));
            }
            return;
        }
    }

    // Handle delete button clicks
    for (size_t i = 0; i < deleteButtons.size(); ++i) {
        if (deleteButtons[i] && deleteButtons[i]->isClicked(mousePos)) {
            showConfirmDialog("Delete this save file permanently?", SaveLoadAction::DELETE_SAVE, static_cast<int>(i));
            return;
        }
    }

    // Handle action button clicks
    if (newSaveButton && newSaveButton->isClicked(mousePos)) {
        if (mode == SaveLoadMenuMode::SAVE) {
            enteringNewSaveName = true;
            newSaveName.clear();
        }
    }

    if (quickSaveButton && quickSaveButton->isClicked(mousePos)) {
        lastAction = (mode == SaveLoadMenuMode::SAVE) ? SaveLoadAction::SAVE_GAME : SaveLoadAction::LOAD_GAME;
        selectedSaveIndex = -2; // Special index for quick save
    }

    if (backButton && backButton->isClicked(mousePos)) {
        lastAction = SaveLoadAction::BACK_TO_GAME;
    }
}

#endif // SAVE_LOAD_MENU_H