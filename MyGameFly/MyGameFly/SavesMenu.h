// SavesMenu.h - Load Game / New Game Selection Menu
#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include "Button.h"
#include "GameSaveData.h"

// Action selected by user in the saves menu
enum class LoadAction {
    NONE = 0,
    NEW_GAME = 1,
    LOAD_GAME = 2,
    BACK_TO_MENU = 3
};

// Represents a save file with metadata for display
struct SaveFileInfo {
    std::string filename;
    std::string displayName;
    std::string dateTime;
    float gameTime;
    bool isValid;

    SaveFileInfo() : gameTime(0.0f), isValid(false) {}
    SaveFileInfo(const std::string& file, const std::string& display,
        const std::string& date, float time, bool valid)
        : filename(file), displayName(display), dateTime(date), gameTime(time), isValid(valid) {
    }
};

class SavesMenu {
private:
    // UI Elements
    sf::Font font;
    sf::RectangleShape background;
    std::vector<std::unique_ptr<Button>> buttons;

    // Save file list UI
    std::vector<std::unique_ptr<Button>> saveFileButtons;
    sf::RectangleShape saveListBackground;
    std::optional<sf::Text> noSavesText;  // Use optional to delay initialization

    // Scroll handling for save list
    float scrollOffset;
    float maxScrollOffset;
    static constexpr float SCROLL_SPEED = 30.0f;

    // Window properties
    sf::Vector2u windowSize;
    bool isActive;
    LoadAction selectedAction;

    // Save file selection
    std::vector<SaveFileInfo> saveFiles;
    int selectedSaveIndex;
    std::string selectedSaveFile;

    // Save system
    GameSaveManager saveManager;

    // Setup methods
    void setupBackground();
    void createButtons();
    void refreshSaveFileList();
    void createSaveFileButtons();
    bool loadFont();
    void updateScrollLimits();

    // Button callback methods
    void onNewGameClicked();
    void onBackClicked();
    void onSaveFileClicked(int saveIndex);

    // Save file management
    SaveFileInfo createSaveFileInfo(const std::string& filename);
    std::string formatGameTime(float gameTime) const;
    std::string formatFileDateTime(const std::string& filename) const;

    // UI layout constants
    static constexpr float BUTTON_WIDTH = 200.0f;
    static constexpr float BUTTON_HEIGHT = 50.0f;
    static constexpr float SAVE_BUTTON_HEIGHT = 40.0f;
    static constexpr float SPACING = 70.0f;
    static constexpr float SAVE_LIST_MARGIN = 20.0f;

public:
    SavesMenu(sf::Vector2u winSize);
    ~SavesMenu() = default;

    // Main interface - matching MainMenu pattern
    void handleEvent(const sf::Event& event, const sf::Vector2f& mousePos);
    void update(const sf::Vector2f& mousePos);
    void draw(sf::RenderWindow& window);

    // State management
    bool getIsActive() const { return isActive; }
    void setActive(bool active) { isActive = active; }
    LoadAction getSelectedAction() const { return selectedAction; }
    void resetSelection() {
        selectedAction = LoadAction::NONE;
        selectedSaveIndex = -1;
        selectedSaveFile = "";
    }

    // Save file access for game initialization
    std::string getSelectedSaveFile() const { return selectedSaveFile; }
    bool loadSelectedSaveData(GameSaveData& saveData);

    // Utility methods
    void refreshSaves() { refreshSaveFileList(); createSaveFileButtons(); }
    int getSaveFileCount() const { return static_cast<int>(saveFiles.size()); }

    // Show/hide control
    void show() {
        isActive = true;
        resetSelection();
        refreshSaves();
    }
    void hide() { isActive = false; }
};