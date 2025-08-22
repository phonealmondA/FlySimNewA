// SavesMenu.cpp - Load Game / New Game Selection Menu Implementation
#include "SavesMenu.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>

// Suppress deprecation warnings for localtime on Windows
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

SavesMenu::SavesMenu(sf::Vector2u winSize)
    : selectedAction(LoadAction::NONE), isActive(true), windowSize(winSize),
    selectedSaveIndex(-1), selectedSaveFile(""), scrollOffset(0.0f), maxScrollOffset(0.0f)
{
    if (!loadFont()) {
        std::cerr << "Warning: Could not load font file for saves menu. Text won't display correctly." << std::endl;
    }

    setupBackground();
    createButtons();
    refreshSaveFileList();
    createSaveFileButtons();
}

bool SavesMenu::loadFont() {
    // Try to load font - same paths as MainMenu
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

    return fontLoaded;
}

void SavesMenu::setupBackground() {
    background.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    background.setFillColor(sf::Color(20, 20, 40, 200)); // Dark blue semi-transparent
    background.setPosition(sf::Vector2f(0.0f, 0.0f));

    // Save list background - right side of screen
    float saveListWidth = static_cast<float>(windowSize.x) * 0.6f;
    float saveListHeight = static_cast<float>(windowSize.y) * 0.7f;
    saveListBackground.setSize(sf::Vector2f(saveListWidth, saveListHeight));
    saveListBackground.setFillColor(sf::Color(10, 10, 30, 180));
    saveListBackground.setPosition(sf::Vector2f(
        static_cast<float>(windowSize.x) - saveListWidth - SAVE_LIST_MARGIN,
        100.0f
    ));

    // Initialize noSavesText with font
    noSavesText = sf::Text(font);
    noSavesText->setString("No save files found");
    noSavesText->setCharacterSize(20);
    noSavesText->setFillColor(sf::Color(150, 150, 150));
}

void SavesMenu::createButtons() {
    float leftX = static_cast<float>(windowSize.x) * 0.15f;
    float startY = static_cast<float>(windowSize.y) / 2.0f - 60.0f;

    buttons.clear();

    try {
        // New Game Button
        auto newGameButton = std::make_unique<Button>(
            sf::Vector2f(leftX, startY),
            sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
            "New Game",
            font,
            [this]() { onNewGameClicked(); }
        );
        buttons.push_back(std::move(newGameButton));

        // Back Button
        auto backButton = std::make_unique<Button>(
            sf::Vector2f(leftX, startY + SPACING),
            sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
            "Back to Main Menu",
            font,
            [this]() { onBackClicked(); }
        );
        buttons.push_back(std::move(backButton));

    }
    catch (const std::exception& e) {
        std::cerr << "Error creating buttons: " << e.what() << std::endl;
    }
}

void SavesMenu::refreshSaveFileList() {
    saveFiles.clear();
    auto saveFileNames = saveManager.getSaveFileList();

    std::cout << "Found " << saveFileNames.size() << " save files" << std::endl;

    for (const auto& filename : saveFileNames) {
        SaveFileInfo info = createSaveFileInfo(filename);
        if (info.isValid) {
            saveFiles.push_back(info);
            std::cout << "Added save: " << info.displayName << " (" << info.dateTime << ")" << std::endl;
        }
    }

    // Sort by filename (most recent first for auto-saves)
    std::sort(saveFiles.begin(), saveFiles.end(),
        [](const SaveFileInfo& a, const SaveFileInfo& b) {
            return a.filename > b.filename; // Reverse sort for newest first
        });
}

void SavesMenu::createSaveFileButtons() {
    saveFileButtons.clear();

    if (saveFiles.empty()) {
        updateScrollLimits();
        return;
    }

    // We don't actually create Button objects anymore since we draw manually
    // Just calculate scroll limits based on save file count
    updateScrollLimits();
}

void SavesMenu::updateScrollLimits() {
    if (saveFiles.empty()) {
        maxScrollOffset = 0.0f;
        return;
    }

    float totalContentHeight = saveFiles.size() * (SAVE_BUTTON_HEIGHT + 5.0f);
    float visibleHeight = saveListBackground.getSize().y - 20.0f; // Padding

    maxScrollOffset = std::max(0.0f, totalContentHeight - visibleHeight);
    scrollOffset = std::clamp(scrollOffset, 0.0f, maxScrollOffset);
}

SaveFileInfo SavesMenu::createSaveFileInfo(const std::string& filename) {
    SaveFileInfo info;
    info.filename = filename;
    info.isValid = false;

    try {
        // Try to load basic metadata from the save file
        GameSaveData saveData;
        if (saveManager.loadGame(saveData, filename)) {
            info.displayName = saveData.saveName.empty() ? filename : saveData.saveName;
            info.gameTime = saveData.gameTime;
            info.dateTime = this->formatFileDateTime(filename);
            info.isValid = true;
        }
        else {
            // Fallback - just use filename
            info.displayName = filename;
            info.dateTime = this->formatFileDateTime(filename);
            info.gameTime = 0.0f;
            info.isValid = true; // Still show it, but might not load properly
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error reading save file " << filename << ": " << e.what() << std::endl;
        // Still create entry but mark as invalid
        info.displayName = filename + " (corrupted)";
        info.dateTime = "";
        info.gameTime = 0.0f;
        info.isValid = false;
    }

    return info;
}

std::string SavesMenu::formatGameTime(float gameTime) const {
    int minutes = static_cast<int>(gameTime) / 60;
    int seconds = static_cast<int>(gameTime) % 60;

    std::stringstream ss;
    ss << minutes << "m " << seconds << "s";
    return ss.str();
}

std::string SavesMenu::formatFileDateTime(const std::string& filename) const {
    // Try to extract timestamp from filename if it follows auto-save pattern
    if (filename.find("auto_") == 0 || filename.find("manual_") == 0) {
        // Look for pattern like "auto_20241225_143022"
        size_t dateStart = filename.find_last_of('_');
        if (dateStart != std::string::npos && dateStart > 0) {
            size_t prevUnderscore = filename.find_last_of('_', dateStart - 1);
            if (prevUnderscore != std::string::npos) {
                std::string datePart = filename.substr(prevUnderscore + 1, dateStart - prevUnderscore - 1);
                std::string timePart = filename.substr(dateStart + 1);

                // Format as readable date
                if (datePart.length() == 8 && timePart.length() >= 6) {
                    return datePart.substr(4, 2) + "/" + datePart.substr(6, 2) + "/" + datePart.substr(0, 4) +
                        " " + timePart.substr(0, 2) + ":" + timePart.substr(2, 2);
                }
            }
        }
    }

    // Fallback: just return filename without extension
    size_t dotPos = filename.find_last_of('.');
    return dotPos != std::string::npos ? filename.substr(0, dotPos) : filename;
}

void SavesMenu::onNewGameClicked() {
    selectedAction = LoadAction::NEW_GAME;
    selectedSaveIndex = -1;
    selectedSaveFile = "";
    //isActive = false;
    std::cout << "New Game selected" << std::endl;
}

void SavesMenu::onBackClicked() {
    selectedAction = LoadAction::BACK_TO_MENU;
    selectedSaveIndex = -1;
    selectedSaveFile = "";
    //isActive = false;
    std::cout << "Back to main menu selected" << std::endl;
}

void SavesMenu::onSaveFileClicked(int saveIndex) {
    if (saveIndex >= 0 && saveIndex < static_cast<int>(saveFiles.size())) {
        selectedAction = LoadAction::LOAD_GAME;
        selectedSaveIndex = saveIndex;
        selectedSaveFile = saveFiles[saveIndex].filename;
        //isActive = false;
        std::cout << "Load game selected: " << saveFiles[saveIndex].displayName << std::endl;
    }
}

bool SavesMenu::loadSelectedSaveData(GameSaveData& saveData) {
    if (!(selectedAction == LoadAction::LOAD_GAME) || selectedSaveFile.empty()) {
        return false;
    }

    return saveManager.loadGame(saveData, selectedSaveFile);
}

void SavesMenu::handleEvent(const sf::Event& event, const sf::Vector2f& mousePos) {
    if (!isActive) return;

    // Handle scrolling in save list area
    if (event.is<sf::Event::MouseWheelScrolled>()) {
        const auto* wheelEvent = event.getIf<sf::Event::MouseWheelScrolled>();
        if (wheelEvent && saveListBackground.getGlobalBounds().contains(mousePos)) {
            scrollOffset -= wheelEvent->delta * SCROLL_SPEED;
            scrollOffset = std::clamp(scrollOffset, 0.0f, maxScrollOffset);
        }
    }

    if (event.is<sf::Event::MouseButtonPressed>()) {
        const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>();
        if (mouseEvent && mouseEvent->button == sf::Mouse::Button::Left) {
            // Check main buttons first
            for (auto& button : buttons) {
                if (button->contains(mousePos)) {
                    button->handleClick();
                    return;
                }
            }

            // Check save file buttons (manual hit testing)
            if (saveListBackground.getGlobalBounds().contains(mousePos)) {
                float saveListX = static_cast<float>(windowSize.x) - (static_cast<float>(windowSize.x) * 0.6f) - SAVE_LIST_MARGIN + 10.0f;
                float saveListY = 120.0f;
                float saveButtonWidth = static_cast<float>(windowSize.x) * 0.55f;

                for (size_t i = 0; i < saveFiles.size(); ++i) {
                    float buttonY = saveListY + (SAVE_BUTTON_HEIGHT + 5.0f) * static_cast<float>(i) - scrollOffset;

                    // Check if click is within this button's bounds
                    if (mousePos.x >= saveListX && mousePos.x <= saveListX + saveButtonWidth &&
                        mousePos.y >= buttonY && mousePos.y <= buttonY + SAVE_BUTTON_HEIGHT) {
                        onSaveFileClicked(static_cast<int>(i));
                        return;
                    }
                }
            }
        }
    }
}

void SavesMenu::update(const sf::Vector2f& mousePos) {
    if (!isActive) return;

    // Update main buttons
    for (auto& button : buttons) {
        if (button) {
            button->update(mousePos);
        }
    }

    // Note: Save file buttons are now drawn manually, so no update needed
}

void SavesMenu::draw(sf::RenderWindow& window) {
    if (!isActive) return;

    // Draw background
    window.draw(background);

    // Draw title
    sf::Text title(font);
    title.setString("Select Game");
    title.setCharacterSize(36);
    title.setFillColor(sf::Color::White);
    sf::FloatRect titleBounds = title.getLocalBounds();
    title.setPosition(sf::Vector2f(
        static_cast<float>(windowSize.x) / 2.0f - titleBounds.size.x / 2.0f,
        30.0f
    ));
    window.draw(title);

    // Draw main buttons
    for (const auto& button : buttons) {
        if (button) {
            button->draw(window);
        }
    }

    // Draw save list background
    window.draw(saveListBackground);

    // Draw save list title
    sf::Text saveListTitle(font);
    saveListTitle.setString("Saved Games:");
    saveListTitle.setCharacterSize(24);
    saveListTitle.setFillColor(sf::Color::White);
    saveListTitle.setPosition(sf::Vector2f(
        saveListBackground.getPosition().x + 10.0f,
        saveListBackground.getPosition().y + 10.0f
    ));
    window.draw(saveListTitle);

    // Set up clipping for save list (pseudo-clipping by checking bounds)
    sf::FloatRect saveListBounds = saveListBackground.getGlobalBounds();

    if (saveFileButtons.empty()) {
        // Draw "no saves" message
        if (noSavesText.has_value()) {
            sf::FloatRect noSavesBounds = noSavesText->getLocalBounds();
            noSavesText->setPosition(sf::Vector2f(
                saveListBounds.position.x + saveListBounds.size.x / 2.0f - noSavesBounds.size.x / 2.0f,
                saveListBounds.position.y + saveListBounds.size.y / 2.0f - noSavesBounds.size.y / 2.0f
            ));
            window.draw(*noSavesText);
        }
    }
    else {
        // Draw save file buttons with scroll offset - simplified approach
        float saveListX = static_cast<float>(windowSize.x) - (static_cast<float>(windowSize.x) * 0.6f) - SAVE_LIST_MARGIN + 10.0f;
        float saveListY = 120.0f;

        for (size_t i = 0; i < saveFileButtons.size(); ++i) {
            if (saveFileButtons[i]) {
                float buttonY = saveListY + (SAVE_BUTTON_HEIGHT + 5.0f) * static_cast<float>(i) - scrollOffset;

                // Only draw if visible in the save list area
                if (buttonY + SAVE_BUTTON_HEIGHT >= saveListBounds.position.y + 40.0f &&
                    buttonY <= saveListBounds.position.y + saveListBounds.size.y - 10.0f) {

                    // Create a temporary button for drawing at the correct position
                    sf::RectangleShape tempShape;
                    tempShape.setPosition(sf::Vector2f(saveListX, buttonY));
                    tempShape.setSize(sf::Vector2f(static_cast<float>(windowSize.x) * 0.55f, SAVE_BUTTON_HEIGHT));
                    tempShape.setFillColor(sf::Color(100, 100, 100, 200));
                    tempShape.setOutlineColor(sf::Color::White);
                    tempShape.setOutlineThickness(1.0f);

                    window.draw(tempShape);

                    // Draw button text
                    sf::Text buttonText(font);
                    const auto& saveInfo = saveFiles[i];
                    std::string displayText = saveInfo.displayName;
                    if (!saveInfo.dateTime.empty()) {
                        displayText += " - " + saveInfo.dateTime;
                    }
                    if (saveInfo.gameTime > 0.0f) {
                        displayText += " - " + formatGameTime(saveInfo.gameTime);
                    }

                    buttonText.setString(displayText);
                    buttonText.setCharacterSize(16);
                    buttonText.setFillColor(sf::Color::White);

                    sf::FloatRect textBounds = buttonText.getLocalBounds();
                    buttonText.setPosition(sf::Vector2f(
                        saveListX + 10.0f,
                        buttonY + (SAVE_BUTTON_HEIGHT - textBounds.size.y) / 2.0f - textBounds.position.y
                    ));

                    window.draw(buttonText);
                }
            }
        }

        // Draw scroll indicator if needed
        if (maxScrollOffset > 0.0f) {
            sf::Text scrollHint(font);
            scrollHint.setString("Scroll for more saves");
            scrollHint.setCharacterSize(14);
            scrollHint.setFillColor(sf::Color(150, 150, 150));
            scrollHint.setPosition(sf::Vector2f(
                saveListBounds.position.x + 10.0f,
                saveListBounds.position.y + saveListBounds.size.y - 25.0f
            ));
            window.draw(scrollHint);
        }
    }

    // Draw instructions
    sf::Text instructions(font);
    instructions.setString("Choose 'New Game' or select a saved game to load");
    instructions.setCharacterSize(16);
    instructions.setFillColor(sf::Color(200, 200, 200));
    sf::FloatRect instrBounds = instructions.getLocalBounds();
    instructions.setPosition(sf::Vector2f(
        static_cast<float>(windowSize.x) / 2.0f - instrBounds.size.x / 2.0f,
        static_cast<float>(windowSize.y) - 50.0f
    ));
    window.draw(instructions);
}

#ifdef _WIN32
#pragma warning(pop)
#endif