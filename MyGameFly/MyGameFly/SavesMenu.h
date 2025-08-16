// SavesMenu.h - Save Game Testing Menu
#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <functional>
#include "Button.h"
#include "GameSaveData.h"

enum class SaveType {
    NONE = 0,
    MANUAL_SAVE = 1,
    AUTO_SAVE = 2,
    QUICK_SAVE = 3
};

class SavesMenu {
private:
    // UI Elements
    sf::Font font;
    sf::RectangleShape background;
    std::vector<std::unique_ptr<Button>> buttons;

    // Window properties
    sf::Vector2u windowSize;
    bool isActive;
    SaveType selectedSaveType;

    // Save system
    GameSaveManager saveManager;

    // Setup methods
    void setupBackground();
    void createButtons();
    bool loadFont();

    // Button callback methods
    void onManualSaveClicked();
    void onAutoSaveClicked();
    void onQuickSaveClicked();
    void onBackClicked();

    // Test data generation
    GameSaveData generateTestSaveData() const;

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
    SaveType getSelectedSaveType() const { return selectedSaveType; }
    void resetSelection() { selectedSaveType = SaveType::NONE; }

    // Save testing interface
    bool performManualSave(const std::string& saveName = "test_manual");
    bool performAutoSave();
    bool performQuickSave();

    // Utility methods
    void printSaveResults(bool success, const std::string& saveType) const;
    std::vector<std::string> getSaveFileList() const;
};