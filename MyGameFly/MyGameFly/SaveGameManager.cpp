#include "SaveGameManager.h"
#include "GameConstants.h"
#include <iostream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <algorithm>

// Static constants
const std::string SaveGameManager::SAVE_DIRECTORY = "saves";
const std::string SaveGameManager::SAVE_EXTENSION = ".mygf";  // MyGameFly save
const uint32_t SaveGameManager::SAVE_FILE_VERSION = 1;
const uint32_t SaveGameManager::SAVE_FILE_MAGIC = 0x4D594746;  // "MYGF" in hex
const std::string SaveGameManager::QUICKSAVE_NAME = "quicksave";
const std::string SaveGameManager::AUTOSAVE_NAME = "autosave";

SaveGameManager::SaveGameManager()
    : autoSaveInterval(5.0f), timeSinceAutoSave(0.0f) {
    createSaveDirectory();
}

bool SaveGameManager::saveGame(const GameSaveState& gameState, const std::string& saveName) {
    if (saveName.empty()) {
        std::cerr << "SaveGameManager: Save name cannot be empty" << std::endl;
        return false;
    }

    if (!createSaveDirectory()) {
        std::cerr << "SaveGameManager: Failed to create save directory" << std::endl;
        return false;
    }

    std::string filename = generateSaveFileName(saveName);
    std::string filepath = getSaveFilePath(filename);

    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "SaveGameManager: Failed to open save file for writing: " << filepath << std::endl;
        return false;
    }

    try {
        // Write file header
        file.write(reinterpret_cast<const char*>(&SAVE_FILE_MAGIC), sizeof(SAVE_FILE_MAGIC));
        file.write(reinterpret_cast<const char*>(&SAVE_FILE_VERSION), sizeof(SAVE_FILE_VERSION));

        // Write save metadata
        std::time_t currentTime = std::time(nullptr);
        file.write(reinterpret_cast<const char*>(&currentTime), sizeof(currentTime));

        // Write display name
        size_t nameLength = saveName.length();
        file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
        file.write(saveName.c_str(), nameLength);

        // Write game state
        file.write(reinterpret_cast<const char*>(&gameState.currentGameMode), sizeof(gameState.currentGameMode));
        file.write(reinterpret_cast<const char*>(&gameState.gameTimeElapsed), sizeof(gameState.gameTimeElapsed));

        // Write camera state
        serializeVector2f(file, gameState.cameraCenter);
        file.write(reinterpret_cast<const char*>(&gameState.zoomLevel), sizeof(gameState.zoomLevel));
        file.write(reinterpret_cast<const char*>(&gameState.targetZoom), sizeof(gameState.targetZoom));

        // Write planet states
        size_t planetCount = gameState.planets.size();
        file.write(reinterpret_cast<const char*>(&planetCount), sizeof(planetCount));
        for (const auto& planet : gameState.planets) {
            planet.serialize(file);
        }

        // Write player states
        size_t playerCount = gameState.players.size();
        file.write(reinterpret_cast<const char*>(&playerCount), sizeof(playerCount));
        for (size_t i = 0; i < playerCount; ++i) {
            serializePlayerState(file, gameState.players[i]);

            // Write player name
            size_t nameLen = gameState.playerNames[i].length();
            file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
            file.write(gameState.playerNames[i].c_str(), nameLen);
        }

        // Write satellite states
        size_t satelliteCount = gameState.satellites.size();
        file.write(reinterpret_cast<const char*>(&satelliteCount), sizeof(satelliteCount));
        for (const auto& satellite : gameState.satellites) {
            satellite.serialize(file);
        }

        // Write network stats
        file.write(reinterpret_cast<const char*>(&gameState.networkStats), sizeof(gameState.networkStats));

        // Write game settings
        file.write(reinterpret_cast<const char*>(&gameState.showTrajectories), sizeof(gameState.showTrajectories));
        file.write(reinterpret_cast<const char*>(&gameState.showVelocityVectors), sizeof(gameState.showVelocityVectors));
        file.write(reinterpret_cast<const char*>(&gameState.showGravityVectors), sizeof(gameState.showGravityVectors));
        file.write(reinterpret_cast<const char*>(&gameState.showSatelliteOrbits), sizeof(gameState.showSatelliteOrbits));

        // Write multiplayer data
        file.write(reinterpret_cast<const char*>(&gameState.localPlayerID), sizeof(gameState.localPlayerID));
        file.write(reinterpret_cast<const char*>(&gameState.isMultiplayerGame), sizeof(gameState.isMultiplayerGame));

        file.close();

        std::cout << "SaveGameManager: Successfully saved game as '" << saveName << "'" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "SaveGameManager: Exception during save: " << e.what() << std::endl;
        file.close();
        std::filesystem::remove(filepath); // Clean up partial file
        return false;
    }
}

bool SaveGameManager::loadGame(const std::string& filename, GameSaveState& outGameState) {
    std::string filepath = getSaveFilePath(filename);

    if (!isValidSaveFile(filepath)) {
        std::cerr << "SaveGameManager: Invalid save file: " << filepath << std::endl;
        return false;
    }

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "SaveGameManager: Failed to open save file for reading: " << filepath << std::endl;
        return false;
    }

    try {
        // Verify file header
        uint32_t magic, version;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        file.read(reinterpret_cast<char*>(&version), sizeof(version));

        if (magic != SAVE_FILE_MAGIC) {
            std::cerr << "SaveGameManager: Invalid file magic number" << std::endl;
            return false;
        }

        if (version > SAVE_FILE_VERSION) {
            std::cerr << "SaveGameManager: Save file version too new: " << version << std::endl;
            return false;
        }

        // Read save metadata
        std::time_t saveTime;
        file.read(reinterpret_cast<char*>(&saveTime), sizeof(saveTime));

        // Read display name
        size_t nameLength;
        file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
        std::string displayName(nameLength, '\0');
        file.read(&displayName[0], nameLength);

        // Read game state
        file.read(reinterpret_cast<char*>(&outGameState.currentGameMode), sizeof(outGameState.currentGameMode));
        file.read(reinterpret_cast<char*>(&outGameState.gameTimeElapsed), sizeof(outGameState.gameTimeElapsed));

        // Read camera state
        deserializeVector2f(file, outGameState.cameraCenter);
        file.read(reinterpret_cast<char*>(&outGameState.zoomLevel), sizeof(outGameState.zoomLevel));
        file.read(reinterpret_cast<char*>(&outGameState.targetZoom), sizeof(outGameState.targetZoom));

        // Read planet states
        size_t planetCount;
        file.read(reinterpret_cast<char*>(&planetCount), sizeof(planetCount));
        outGameState.planets.resize(planetCount);
        for (auto& planet : outGameState.planets) {
            planet.deserialize(file);
        }

        // Read player states
        size_t playerCount;
        file.read(reinterpret_cast<char*>(&playerCount), sizeof(playerCount));
        outGameState.players.resize(playerCount);
        outGameState.playerNames.resize(playerCount);

        for (size_t i = 0; i < playerCount; ++i) {
            deserializePlayerState(file, outGameState.players[i]);

            // Read player name
            size_t nameLen;
            file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
            outGameState.playerNames[i].resize(nameLen);
            file.read(&outGameState.playerNames[i][0], nameLen);
        }

        // Read satellite states
        size_t satelliteCount;
        file.read(reinterpret_cast<char*>(&satelliteCount), sizeof(satelliteCount));
        outGameState.satellites.resize(satelliteCount);
        for (auto& satellite : outGameState.satellites) {
            satellite.deserialize(file);
        }

        // Read network stats
        file.read(reinterpret_cast<char*>(&outGameState.networkStats), sizeof(outGameState.networkStats));

        // Read game settings
        file.read(reinterpret_cast<char*>(&outGameState.showTrajectories), sizeof(outGameState.showTrajectories));
        file.read(reinterpret_cast<char*>(&outGameState.showVelocityVectors), sizeof(outGameState.showVelocityVectors));
        file.read(reinterpret_cast<char*>(&outGameState.showGravityVectors), sizeof(outGameState.showGravityVectors));
        file.read(reinterpret_cast<char*>(&outGameState.showSatelliteOrbits), sizeof(outGameState.showSatelliteOrbits));

        // Read multiplayer data
        file.read(reinterpret_cast<char*>(&outGameState.localPlayerID), sizeof(outGameState.localPlayerID));
        file.read(reinterpret_cast<char*>(&outGameState.isMultiplayerGame), sizeof(outGameState.isMultiplayerGame));

        // Set metadata
        outGameState.metadata.filename = filename;
        outGameState.metadata.displayName = displayName;
        outGameState.metadata.saveTime = saveTime;
        outGameState.metadata.gameMode = outGameState.currentGameMode;
        outGameState.metadata.playerCount = static_cast<int>(playerCount);
        outGameState.metadata.satelliteCount = static_cast<int>(satelliteCount);
        outGameState.metadata.gameTimeElapsed = outGameState.gameTimeElapsed;
        outGameState.metadata.cameraPosition = outGameState.cameraCenter;
        outGameState.metadata.zoomLevel = outGameState.zoomLevel;

        file.close();

        std::cout << "SaveGameManager: Successfully loaded game '" << displayName << "'" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "SaveGameManager: Exception during load: " << e.what() << std::endl;
        file.close();
        return false;
    }
}

bool SaveGameManager::quickSave(const GameSaveState& gameState) {
    return saveGame(gameState, QUICKSAVE_NAME);
}

bool SaveGameManager::quickLoad(GameSaveState& outGameState) {
    return loadGame(QUICKSAVE_NAME + SAVE_EXTENSION, outGameState);
}

bool SaveGameManager::hasQuickSave() const {
    return saveExists(QUICKSAVE_NAME + SAVE_EXTENSION);
}

bool SaveGameManager::autoSave(const GameSaveState& gameState) {
    timeSinceAutoSave = 0.0f; // Reset timer
    return saveGame(gameState, AUTOSAVE_NAME);
}

bool SaveGameManager::shouldAutoSave(float deltaTime) {
    timeSinceAutoSave += deltaTime / 60.0f; // Convert to minutes
    return timeSinceAutoSave >= autoSaveInterval;
}

// Serialization helpers (similar to NetworkManager patterns)
void SaveGameManager::serializePlayerState(std::ofstream& file, const PlayerState& state) const {
    file.write(reinterpret_cast<const char*>(&state.playerID), sizeof(state.playerID));
    serializeVector2f(file, state.position);
    serializeVector2f(file, state.velocity);
    file.write(reinterpret_cast<const char*>(&state.rotation), sizeof(state.rotation));
    file.write(reinterpret_cast<const char*>(&state.isRocket), sizeof(state.isRocket));
    file.write(reinterpret_cast<const char*>(&state.isOnGround), sizeof(state.isOnGround));
    file.write(reinterpret_cast<const char*>(&state.mass), sizeof(state.mass));
    file.write(reinterpret_cast<const char*>(&state.thrustLevel), sizeof(state.thrustLevel));
    file.write(reinterpret_cast<const char*>(&state.currentFuel), sizeof(state.currentFuel));
    file.write(reinterpret_cast<const char*>(&state.maxFuel), sizeof(state.maxFuel));
    file.write(reinterpret_cast<const char*>(&state.isCollectingFuel), sizeof(state.isCollectingFuel));
    file.write(reinterpret_cast<const char*>(&state.isSatellite), sizeof(state.isSatellite));
    file.write(reinterpret_cast<const char*>(&state.satelliteID), sizeof(state.satelliteID));
    file.write(reinterpret_cast<const char*>(&state.orbitAccuracy), sizeof(state.orbitAccuracy));
    file.write(reinterpret_cast<const char*>(&state.needsMaintenance), sizeof(state.needsMaintenance));
}

void SaveGameManager::deserializePlayerState(std::ifstream& file, PlayerState& state) const {
    file.read(reinterpret_cast<char*>(&state.playerID), sizeof(state.playerID));
    deserializeVector2f(file, state.position);
    deserializeVector2f(file, state.velocity);
    file.read(reinterpret_cast<char*>(&state.rotation), sizeof(state.rotation));
    file.read(reinterpret_cast<char*>(&state.isRocket), sizeof(state.isRocket));
    file.read(reinterpret_cast<char*>(&state.isOnGround), sizeof(state.isOnGround));
    file.read(reinterpret_cast<char*>(&state.mass), sizeof(state.mass));
    file.read(reinterpret_cast<char*>(&state.thrustLevel), sizeof(state.thrustLevel));
    file.read(reinterpret_cast<char*>(&state.currentFuel), sizeof(state.currentFuel));
    file.read(reinterpret_cast<char*>(&state.maxFuel), sizeof(state.maxFuel));
    file.read(reinterpret_cast<char*>(&state.isCollectingFuel), sizeof(state.isCollectingFuel));
    file.read(reinterpret_cast<char*>(&state.isSatellite), sizeof(state.isSatellite));
    file.read(reinterpret_cast<char*>(&state.satelliteID), sizeof(state.satelliteID));
    file.read(reinterpret_cast<char*>(&state.orbitAccuracy), sizeof(state.orbitAccuracy));
    file.read(reinterpret_cast<char*>(&state.needsMaintenance), sizeof(state.needsMaintenance));
}

void SaveGameManager::serializeVector2f(std::ofstream& file, const sf::Vector2f& vec) const {
    file.write(reinterpret_cast<const char*>(&vec.x), sizeof(vec.x));
    file.write(reinterpret_cast<const char*>(&vec.y), sizeof(vec.y));
}

void SaveGameManager::deserializeVector2f(std::ifstream& file, sf::Vector2f& vec) const {
    file.read(reinterpret_cast<char*>(&vec.x), sizeof(vec.x));
    file.read(reinterpret_cast<char*>(&vec.y), sizeof(vec.y));
}

void SaveGameManager::serializeColor(std::ofstream& file, const sf::Color& color) const {
    file.write(reinterpret_cast<const char*>(&color.r), sizeof(color.r));
    file.write(reinterpret_cast<const char*>(&color.g), sizeof(color.g));
    file.write(reinterpret_cast<const char*>(&color.b), sizeof(color.b));
    file.write(reinterpret_cast<const char*>(&color.a), sizeof(color.a));
}

void SaveGameManager::deserializeColor(std::ifstream& file, sf::Color& color) const {
    file.read(reinterpret_cast<char*>(&color.r), sizeof(color.r));
    file.read(reinterpret_cast<char*>(&color.g), sizeof(color.g));
    file.read(reinterpret_cast<char*>(&color.b), sizeof(color.b));
    file.read(reinterpret_cast<char*>(&color.a), sizeof(color.a));
}

// Planet and Satellite state serialization
void PlanetSaveState::serialize(std::ofstream& file) const {
    file.write(reinterpret_cast<const char*>(&position.x), sizeof(position.x));
    file.write(reinterpret_cast<const char*>(&position.y), sizeof(position.y));
    file.write(reinterpret_cast<const char*>(&velocity.x), sizeof(velocity.x));
    file.write(reinterpret_cast<const char*>(&velocity.y), sizeof(velocity.y));
    file.write(reinterpret_cast<const char*>(&mass), sizeof(mass));
    file.write(reinterpret_cast<const char*>(&radius), sizeof(radius));
    file.write(reinterpret_cast<const char*>(&color.r), sizeof(color.r));
    file.write(reinterpret_cast<const char*>(&color.g), sizeof(color.g));
    file.write(reinterpret_cast<const char*>(&color.b), sizeof(color.b));
    file.write(reinterpret_cast<const char*>(&color.a), sizeof(color.a));
}

void PlanetSaveState::deserialize(std::ifstream& file) {
    file.read(reinterpret_cast<char*>(&position.x), sizeof(position.x));
    file.read(reinterpret_cast<char*>(&position.y), sizeof(position.y));
    file.read(reinterpret_cast<char*>(&velocity.x), sizeof(velocity.x));
    file.read(reinterpret_cast<char*>(&velocity.y), sizeof(velocity.y));
    file.read(reinterpret_cast<char*>(&mass), sizeof(mass));
    file.read(reinterpret_cast<char*>(&radius), sizeof(radius));
    file.read(reinterpret_cast<char*>(&color.r), sizeof(color.r));
    file.read(reinterpret_cast<char*>(&color.g), sizeof(color.g));
    file.read(reinterpret_cast<char*>(&color.b), sizeof(color.b));
    file.read(reinterpret_cast<char*>(&color.a), sizeof(color.a));
}

void SatelliteSaveState::serialize(std::ofstream& file) const {
    // Serialize base player state
    file.write(reinterpret_cast<const char*>(&baseState.playerID), sizeof(baseState.playerID));
    file.write(reinterpret_cast<const char*>(&baseState.position.x), sizeof(baseState.position.x));
    file.write(reinterpret_cast<const char*>(&baseState.position.y), sizeof(baseState.position.y));
    file.write(reinterpret_cast<const char*>(&baseState.velocity.x), sizeof(baseState.velocity.x));
    file.write(reinterpret_cast<const char*>(&baseState.velocity.y), sizeof(baseState.velocity.y));
    file.write(reinterpret_cast<const char*>(&baseState.currentFuel), sizeof(baseState.currentFuel));
    file.write(reinterpret_cast<const char*>(&baseState.maxFuel), sizeof(baseState.maxFuel));
    file.write(reinterpret_cast<const char*>(&baseState.mass), sizeof(baseState.mass));
    file.write(reinterpret_cast<const char*>(&baseState.satelliteID), sizeof(baseState.satelliteID));
    file.write(reinterpret_cast<const char*>(&baseState.orbitAccuracy), sizeof(baseState.orbitAccuracy));

    // Serialize satellite-specific data
    size_t nameLength = satelliteName.length();
    file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
    file.write(satelliteName.c_str(), nameLength);

    file.write(reinterpret_cast<const char*>(&stationKeepingEfficiency), sizeof(stationKeepingEfficiency));
    file.write(reinterpret_cast<const char*>(&transferRange), sizeof(transferRange));
    file.write(reinterpret_cast<const char*>(&enableAutomaticMaintenance), sizeof(enableAutomaticMaintenance));
    file.write(reinterpret_cast<const char*>(&enableAutomaticCollection), sizeof(enableAutomaticCollection));
    file.write(reinterpret_cast<const char*>(&targetSemiMajorAxis), sizeof(targetSemiMajorAxis));
    file.write(reinterpret_cast<const char*>(&targetEccentricity), sizeof(targetEccentricity));
    file.write(reinterpret_cast<const char*>(&targetInclination), sizeof(targetInclination));
}

void SatelliteSaveState::deserialize(std::ifstream& file) {
    // Deserialize base player state
    file.read(reinterpret_cast<char*>(&baseState.playerID), sizeof(baseState.playerID));
    file.read(reinterpret_cast<char*>(&baseState.position.x), sizeof(baseState.position.x));
    file.read(reinterpret_cast<char*>(&baseState.position.y), sizeof(baseState.position.y));
    file.read(reinterpret_cast<char*>(&baseState.velocity.x), sizeof(baseState.velocity.x));
    file.read(reinterpret_cast<char*>(&baseState.velocity.y), sizeof(baseState.velocity.y));
    file.read(reinterpret_cast<char*>(&baseState.currentFuel), sizeof(baseState.currentFuel));
    file.read(reinterpret_cast<char*>(&baseState.maxFuel), sizeof(baseState.maxFuel));
    file.read(reinterpret_cast<char*>(&baseState.mass), sizeof(baseState.mass));
    file.read(reinterpret_cast<char*>(&baseState.satelliteID), sizeof(baseState.satelliteID));
    file.read(reinterpret_cast<char*>(&baseState.orbitAccuracy), sizeof(baseState.orbitAccuracy));

    // Set satellite flags
    baseState.isSatellite = true;
    baseState.isRocket = false;

    // Deserialize satellite-specific data
    size_t nameLength;
    file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
    satelliteName.resize(nameLength);
    file.read(&satelliteName[0], nameLength);

    file.read(reinterpret_cast<char*>(&stationKeepingEfficiency), sizeof(stationKeepingEfficiency));
    file.read(reinterpret_cast<char*>(&transferRange), sizeof(transferRange));
    file.read(reinterpret_cast<char*>(&enableAutomaticMaintenance), sizeof(enableAutomaticMaintenance));
    file.read(reinterpret_cast<char*>(&enableAutomaticCollection), sizeof(enableAutomaticCollection));
    file.read(reinterpret_cast<char*>(&targetSemiMajorAxis), sizeof(targetSemiMajorAxis));
    file.read(reinterpret_cast<char*>(&targetEccentricity), sizeof(targetEccentricity));
    file.read(reinterpret_cast<char*>(&targetInclination), sizeof(targetInclination));
}

// Utility functions
std::string SaveGameManager::generateSaveFileName(const std::string& baseName) const {
    return baseName + SAVE_EXTENSION;
}

std::string SaveGameManager::getSaveFilePath(const std::string& filename) const {
    return SAVE_DIRECTORY + "/" + filename;
}

bool SaveGameManager::createSaveDirectory() const {
    try {
        if (!std::filesystem::exists(SAVE_DIRECTORY)) {
            return std::filesystem::create_directory(SAVE_DIRECTORY);
        }
        return true;
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "SaveGameManager: Failed to create save directory: " << e.what() << std::endl;
        return false;
    }
}

bool SaveGameManager::isValidSaveFile(const std::string& filepath) const {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.close();

    return magic == SAVE_FILE_MAGIC;
}

std::vector<SaveFileInfo> SaveGameManager::getAvailableSaves() const {
    std::vector<SaveFileInfo> saves;

    try {
        if (!std::filesystem::exists(SAVE_DIRECTORY)) {
            return saves;
        }

        for (const auto& entry : std::filesystem::directory_iterator(SAVE_DIRECTORY)) {
            if (entry.is_regular_file() && entry.path().extension() == SAVE_EXTENSION) {
                SaveFileInfo info = getSaveInfo(entry.path().filename().string());
                if (info.saveTime != 0) { // Valid save file
                    saves.push_back(info);
                }
            }
        }

        // Sort by save time (newest first)
        std::sort(saves.begin(), saves.end(),
            [](const SaveFileInfo& a, const SaveFileInfo& b) {
                return a.saveTime > b.saveTime;
            });
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "SaveGameManager: Error reading save directory: " << e.what() << std::endl;
    }

    return saves;
}

std::string SaveGameManager::formatSaveTime(std::time_t saveTime) const {
    std::stringstream ss;
    ss << std::put_time(std::localtime(&saveTime), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}