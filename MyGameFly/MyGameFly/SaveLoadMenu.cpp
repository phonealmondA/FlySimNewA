#include "SaveLoadMenu.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

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
    pendingAction(SaveLoadAction::NONE), pendingIndex(-1) {

    // Try to load font (use default if not available)
    fontLoaded = font.loadFromFile("assets/fonts/arial.ttf");
    if (!fontLoaded) {
        // Try platform-specific font paths
#ifdef _WIN32
        fontLoaded = font.loadFromFile("C:/Windows/Fonts/arial.ttf");
#elif defined(__APPLE__)
        fontLoaded = font.loadFromFile("/System/Library/Fonts/Arial.ttf");
#elif defined(__linux__)
        fontLoaded = font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif
        if (!fontLoaded) {
            std::cout << "SaveLoadMenu: Could not load font, using default" << std::endl;
        }
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

void SaveLoadMenu::setupButtons() {
    float buttonWidth = 120.0f;
    float buttonHeight = 35.0f;
    float rightMargin = 50.0f;
    float topMargin = 150.0f;
    float spacing = 45.0f;

    float buttonX = static_cast<float>(windowSize.x) - rightMargin - buttonWidth;

    // New Save button (only for save mode)
    if (mode == SaveLoadMenuMode::SAVE) {
        newSaveButton = std::make_unique<Button>(
            sf::Vector2f(buttonX, topMargin),
            sf::Vector2f(buttonWidth, buttonHeight),
            "New Save",
            font,
            [this]() { startNewSaveInput(); }
        );
    }

    // Quick Save button (only for save mode)
    if (mode == SaveLoadMenuMode::SAVE) {
        quickSaveButton = std::make_unique<Button>(
            sf::Vector2f(buttonX, topMargin + spacing),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Quick Save",
            font,
            [this]() { executeQuickSave(); }
        );
    }

    // Quick Load button (only for load mode)
    if (mode == SaveLoadMenuMode::LOAD) {
        quickLoadButton = std::make_unique<Button>(
            sf::Vector2f(buttonX, topMargin),
            sf::Vector2f(buttonWidth, buttonHeight),
            "Quick Load",
            font,
            [this]() { executeQuickLoad(); }
        );
    }

    // Back button
    float backButtonY = (mode == SaveLoadMenuMode::SAVE) ? topMargin + spacing * 2 : topMargin + spacing;
    backButton = std::make_unique<Button>(
        sf::Vector2f(buttonX, backButtonY),
        sf::Vector2f(buttonWidth, buttonHeight),
        "Back",
        font,
        [this]() { close(); }
    );
}

void SaveLoadMenu::setupNewSaveInput() {
    if (!fontLoaded) return;

    float boxWidth = 300.0f;
    float boxHeight = 40.0f;
    float centerX = static_cast<float>(windowSize.x) / 2.0f;
    float centerY = static_cast<float>(windowSize.y) / 2.0f;

    newSaveInputBox.setSize(sf::Vector2f(boxWidth, boxHeight));
    newSaveInputBox.setFillColor(sf::Color(50, 50, 70, 220));
    newSaveInputBox.setOutlineColor(sf::Color(100, 100, 150));
    newSaveInputBox.setOutlineThickness(2.0f);
    newSaveInputBox.setPosition(centerX - boxWidth / 2.0f, centerY - boxHeight / 2.0f);

    newSaveInputText.setFont(font);
    newSaveInputText.setCharacterSize(18);
    newSaveInputText.setFillColor(TEXT_COLOR);
    newSaveInputText.setPosition(centerX - boxWidth / 2.0f + 10.0f, centerY - boxHeight / 2.0f + 8.0f);

    newSavePromptText.setFont(font);
    newSavePromptText.setCharacterSize(20);
    newSavePromptText.setFillColor(TEXT_COLOR);
    newSavePromptText.setString("Enter save name:");
    sf::FloatRect promptBounds = newSavePromptText.getLocalBounds();
    newSavePromptText.setPosition(centerX - promptBounds.size.x / 2.0f, centerY - boxHeight / 2.0f - 40.0f);
}

void SaveLoadMenu::setupConfirmDialog() {
    if (!fontLoaded) return;

    float buttonWidth = 80.0f;
    float buttonHeight = 35.0f;
    float centerX = static_cast<float>(windowSize.x) / 2.0f;
    float centerY = static_cast<float>(windowSize.y) / 2.0f;

    confirmYesButton = std::make_unique<Button>(
        sf::Vector2f(centerX - buttonWidth - 10.0f, centerY + 20.0f),
        sf::Vector2f(buttonWidth, buttonHeight),
        "Yes",
        font,
        [this]() { confirmAction(); }
    );

    confirmNoButton = std::make_unique<Button>(
        sf::Vector2f(centerX + 10.0f, centerY + 20.0f),
        sf::Vector2f(buttonWidth, buttonHeight),
        "No",
        font,
        [this]() { hideConfirmDialog(); }
    );
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

void SaveLoadMenu::handleMouseClick(sf::Vector2f mousePos, SaveGameManager* saveManager) {
    if (showingConfirmDialog) {
        if (confirmYesButton && confirmYesButton->contains(mousePos)) {
            confirmYesButton->handleClick();
            return;
        }
        if (confirmNoButton && confirmNoButton->contains(mousePos)) {
            confirmNoButton->handleClick();
            return;
        }
        return; // Block other clicks when dialog is shown
    }

    if (enteringNewSaveName) {
        // Click outside input box cancels new save
        if (!newSaveInputBox.getGlobalBounds().contains(mousePos)) {
            cancelNewSaveInput();
        }
        return;
    }

    // Handle save file button clicks
    for (size_t i = 0; i < saveFileButtons.size(); ++i) {
        if (saveFileButtons[i] && saveFileButtons[i]->contains(mousePos)) {
            if (mode == SaveLoadMenuMode::SAVE) {
                showConfirmDialog("Overwrite this save file?", SaveLoadAction::SAVE_GAME, static_cast<int>(i));
            }
            else {
                loadSelectedSave(static_cast<int>(i), saveManager);
            }
            return;
        }
    }

    // Handle delete button clicks
    for (size_t i = 0; i < deleteButtons.size(); ++i) {
        if (deleteButtons[i] && deleteButtons[i]->contains(mousePos)) {
            showConfirmDialog("Delete this save file?", SaveLoadAction::DELETE_SAVE, static_cast<int>(i));
            return;
        }
    }

    // Handle action button clicks
    if (newSaveButton && newSaveButton->contains(mousePos)) {
        newSaveButton->handleClick();
        return;
    }

    if (quickSaveButton && quickSaveButton->contains(mousePos)) {
        quickSaveButton->handleClick();
        return;
    }

    if (quickLoadButton && quickLoadButton->contains(mousePos)) {
        quickLoadButton->handleClick();
        return;
    }

    if (backButton && backButton->contains(mousePos)) {
        backButton->handleClick();
        return;
    }
}

void SaveLoadMenu::handleKeyInput(sf::Event::KeyPressed keyEvent) {
    if (enteringNewSaveName) {
        if (keyEvent.code == sf::Keyboard::Key::Enter) {
            finishNewSaveInput();
        }
        else if (keyEvent.code == sf::Keyboard::Key::Escape) {
            cancelNewSaveInput();
        }
        else if (keyEvent.code == sf::Keyboard::Key::Backspace && !newSaveName.empty()) {
            newSaveName.pop_back();
            newSaveInputText.setString(newSaveName);
        }
        return;
    }

    // Handle other key inputs when not entering save name
    if (keyEvent.code == sf::Keyboard::Key::Escape) {
        close();
    }
}

void SaveLoadMenu::handleTextInput(sf::Event::TextEntered textEvent) {
    if (!enteringNewSaveName) return;

    if (textEvent.unicode < 128 && textEvent.unicode != 8) { // Printable ASCII, not backspace
        char character = static_cast<char>(textEvent.unicode);

        // Allow alphanumeric, spaces, and common punctuation
        if ((character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9') ||
            character == ' ' || character == '-' || character == '_') {

            if (newSaveName.length() < 30) { // Limit save name length
                newSaveName += character;
                newSaveInputText.setString(newSaveName);
            }
        }
    }
}

void SaveLoadMenu::draw(sf::RenderWindow& window) {
    // Draw background
    window.draw(background);

    // Draw title and instructions
    if (fontLoaded) {
        window.draw(titleText);
        window.draw(instructionText);
    }

    // Draw save file list
    drawSaveFileList(window);

    // Draw action buttons
    if (newSaveButton) newSaveButton->draw(window);
    if (quickSaveButton) quickSaveButton->draw(window);
    if (quickLoadButton) quickLoadButton->draw(window);
    if (backButton) backButton->draw(window);

    // Draw new save input if active
    if (enteringNewSaveName) {
        drawNewSaveInput(window);
    }

    // Draw confirmation dialog if active
    if (showingConfirmDialog) {
        drawConfirmDialog(window);
    }
}

void SaveLoadMenu::drawSaveFileList(sf::RenderWindow& window) {
    float listX = 50.0f;
    float listY = 150.0f;
    float listWidth = static_cast<float>(windowSize.x) - 250.0f; // Leave space for buttons

    // Draw save file buttons
    for (size_t i = 0; i < saveFileButtons.size(); ++i) {
        if (saveFileButtons[i]) {
            saveFileButtons[i]->draw(window);
        }
        if (deleteButtons[i]) {
            deleteButtons[i]->draw(window);
        }
    }

    // Draw "No saves found" message if empty
    if (saveFiles.empty() && fontLoaded) {
        sf::Text noSavesText;
        noSavesText.setFont(font);
        noSavesText.setCharacterSize(18);
        noSavesText.setFillColor(sf::Color(150, 150, 150));
        noSavesText.setString("No save files found");

        sf::FloatRect textBounds = noSavesText.getLocalBounds();
        noSavesText.setPosition(
            listX + listWidth / 2.0f - textBounds.size.x / 2.0f,
            listY + 100.0f
        );

        window.draw(noSavesText);
    }
}

void SaveLoadMenu::drawNewSaveInput(sf::RenderWindow& window) {
    // Draw semi-transparent overlay
    sf::RectangleShape overlay;
    overlay.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    overlay.setFillColor(sf::Color(0, 0, 0, 100));
    window.draw(overlay);

    // Draw input elements
    if (fontLoaded) {
        window.draw(newSavePromptText);
    }
    window.draw(newSaveInputBox);
    if (fontLoaded) {
        window.draw(newSaveInputText);
    }
}

void SaveLoadMenu::drawConfirmDialog(sf::RenderWindow& window) {
    // Draw semi-transparent overlay
    sf::RectangleShape overlay;
    overlay.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(overlay);

    // Draw dialog box
    float boxWidth = 400.0f;
    float boxHeight = 150.0f;
    float centerX = static_cast<float>(windowSize.x) / 2.0f;
    float centerY = static_cast<float>(windowSize.y) / 2.0f;

    sf::RectangleShape dialogBox;
    dialogBox.setSize(sf::Vector2f(boxWidth, boxHeight));
    dialogBox.setFillColor(sf::Color(40, 40, 60, 240));
    dialogBox.setOutlineColor(sf::Color(100, 100, 150));
    dialogBox.setOutlineThickness(2.0f);
    dialogBox.setPosition(centerX - boxWidth / 2.0f, centerY - boxHeight / 2.0f);
    window.draw(dialogBox);

    // Draw confirmation message
    if (fontLoaded) {
        sf::Text confirmText;
        confirmText.setFont(font);
        confirmText.setCharacterSize(18);
        confirmText.setFillColor(TEXT_COLOR);
        confirmText.setString(confirmMessage);

        sf::FloatRect textBounds = confirmText.getLocalBounds();
        confirmText.setPosition(
            centerX - textBounds.size.x / 2.0f,
            centerY - boxHeight / 2.0f + 30.0f
        );

        window.draw(confirmText);
    }

    // Draw buttons
    if (confirmYesButton) confirmYesButton->draw(window);
    if (confirmNoButton) confirmNoButton->draw(window);
}

void SaveLoadMenu::refreshSaveList(SaveGameManager* saveManager) {
    if (!saveManager) return;

    saveFiles = saveManager->getAvailableSaves();

    // Clear existing buttons
    saveFileButtons.clear();
    deleteButtons.clear();

    // Create new buttons for each save file
    float listX = 50.0f;
    float listY = 150.0f;
    float buttonWidth = static_cast<float>(windowSize.x) - 300.0f;
    float buttonHeight = itemHeight - 5.0f;
    float deleteButtonWidth = 60.0f;

    for (size_t i = 0; i < saveFiles.size(); ++i) {
        float yPos = listY + static_cast<float>(i) * itemHeight - scrollOffset;

        // Skip if not visible
        if (yPos < listY - itemHeight || yPos > listY + visibleItems * itemHeight) {
            continue;
        }

        // Create save file button
        std::string buttonText = formatFileInfo(saveFiles[i]);
        auto saveButton = std::make_unique<Button>(
            sf::Vector2f(listX, yPos),
            sf::Vector2f(buttonWidth, buttonHeight),
            buttonText,
            font,
            []() {} // Handled by click detection
        );
        saveFileButtons.push_back(std::move(saveButton));

        // Create delete button
        auto deleteButton = std::make_unique<Button>(
            sf::Vector2f(listX + buttonWidth + 5.0f, yPos),
            sf::Vector2f(deleteButtonWidth, buttonHeight),
            "Delete",
            font,
            []() {} // Handled by click detection
        );
        deleteButtons.push_back(std::move(deleteButton));
    }

    // Update scroll limits
    maxScroll = std::max(0.0f, static_cast<float>(saveFiles.size()) * itemHeight - visibleItems * itemHeight);
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

SaveLoadAction SaveLoadMenu::getLastAction() const {
    return pendingAction;
}

std::string SaveLoadMenu::getSelectedSaveFile() const {
    if (selectedSaveIndex >= 0 && selectedSaveIndex < static_cast<int>(saveFiles.size())) {
        return saveFiles[selectedSaveIndex].filename;
    }
    return "";
}

std::string SaveLoadMenu::getNewSaveName() const {
    return newSaveName;
}

void SaveLoadMenu::show() {
    // Reset state when showing
    selectedSaveIndex = -1;
    enteringNewSaveName = false;
    showingConfirmDialog = false;
    pendingAction = SaveLoadAction::NONE;
    newSaveName.clear();
    scrollOffset = 0.0f;
}

void SaveLoadMenu::close() {
    pendingAction = SaveLoadAction::BACK_TO_GAME;
}

// Private helper methods
void SaveLoadMenu::startNewSaveInput() {
    enteringNewSaveName = true;
    newSaveName.clear();
    newSaveInputText.setString("");
}

void SaveLoadMenu::finishNewSaveInput() {
    if (!newSaveName.empty()) {
        std::string sanitized = sanitizeSaveName(newSaveName);
        if (!sanitized.empty()) {
            pendingAction = SaveLoadAction::NEW_SAVE;
            newSaveName = sanitized;
        }
    }
    enteringNewSaveName = false;
}

void SaveLoadMenu::cancelNewSaveInput() {
    enteringNewSaveName = false;
    newSaveName.clear();
}

void SaveLoadMenu::executeQuickSave() {
    pendingAction = SaveLoadAction::SAVE_GAME;
    newSaveName = "quicksave";
}

void SaveLoadMenu::executeQuickLoad() {
    pendingAction = SaveLoadAction::LOAD_GAME;
    newSaveName = "quicksave";
}

void SaveLoadMenu::loadSelectedSave(int index, SaveGameManager* saveManager) {
    if (index >= 0 && index < static_cast<int>(saveFiles.size())) {
        selectedSaveIndex = index;
        pendingAction = SaveLoadAction::LOAD_GAME;
    }
}

bool SaveLoadMenu::isValidSaveName(const std::string& name) const {
    if (name.empty() || name.length() > 50) return false;

    for (char c : name) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_')) {
            return false;
        }
    }
    return true;
}

std::string SaveLoadMenu::sanitizeSaveName(const std::string& name) const {
    std::string result;
    for (char c : name) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_') {
            result += c;
        }
    }

    // Trim spaces
    size_t start = result.find_first_not_of(' ');
    if (start == std::string::npos) return "";

    size_t end = result.find_last_not_of(' ');
    return result.substr(start, end - start + 1);
}

void SaveLoadMenu::showConfirmDialog(const std::string& message, SaveLoadAction action, int index) {
    confirmMessage = message;
    pendingAction = action;
    pendingIndex = index;
    showingConfirmDialog = true;
}

void SaveLoadMenu::hideConfirmDialog() {
    showingConfirmDialog = false;
    pendingAction = SaveLoadAction::NONE;
    pendingIndex = -1;
}

void SaveLoadMenu::confirmAction() {
    hideConfirmDialog();

    // The actual action execution should be handled by the calling code
    // based on pendingAction and pendingIndex
}