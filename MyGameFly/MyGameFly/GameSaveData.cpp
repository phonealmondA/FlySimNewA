// GameSaveData.cpp - Game State Serialization Implementation
#include "GameSaveData.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>

// Suppress deprecation warnings for localtime on Windows
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

// Helper function to check if string ends with suffix (C++20 alternative)
bool endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

GameSaveManager::GameSaveManager()
    : saveDirectory("saves/"), defaultSaveExtension(".save") {
    // Create saves directory if it doesn't exist
    try {
        std::filesystem::create_directories(saveDirectory);
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Warning: Could not create save directory: " << e.what() << std::endl;
        //saveDirectory = "./"; // Fallback to current directory
    }
}

GameSaveManager::~GameSaveManager() {
    // No cleanup needed for this implementation
}

// === SERIALIZATION METHODS ===

void GameSaveManager::serializePlanetData(sf::Packet& packet, const SavedPlanetData& planetData) {
    packet << planetData.position.x << planetData.position.y;
    packet << planetData.mass;
    packet << planetData.color.r << planetData.color.g << planetData.color.b << planetData.color.a;
    packet << planetData.isMainPlanet;
    packet << planetData.radius;
}

void GameSaveManager::deserializePlanetData(sf::Packet& packet, SavedPlanetData& planetData) {
    packet >> planetData.position.x >> planetData.position.y;
    packet >> planetData.mass;
    packet >> planetData.color.r >> planetData.color.g >> planetData.color.b >> planetData.color.a;
    packet >> planetData.isMainPlanet;
    packet >> planetData.radius;
}

void GameSaveManager::serializeSatelliteData(sf::Packet& packet, const SavedSatelliteData& satelliteData) {
    // Serialize base PlayerState first
    serializePlayerState(packet, satelliteData.baseState);

    // Then satellite-specific data
    packet << satelliteData.satelliteName;
    packet << satelliteData.isOperational;
    packet << satelliteData.lastMaintenanceTime;
    packet << satelliteData.orbitPlanetID;
}

void GameSaveManager::deserializeSatelliteData(sf::Packet& packet, SavedSatelliteData& satelliteData) {
    // Deserialize base PlayerState first
    deserializePlayerState(packet, satelliteData.baseState);

    // Then satellite-specific data
    packet >> satelliteData.satelliteName;
    packet >> satelliteData.isOperational;
    packet >> satelliteData.lastMaintenanceTime;
    packet >> satelliteData.orbitPlanetID;
}

void GameSaveManager::serializeCameraData(sf::Packet& packet, const SavedCameraData& cameraData) {
    packet << cameraData.position.x << cameraData.position.y;
    packet << cameraData.size.x << cameraData.size.y;
    packet << cameraData.zoom;
    packet << cameraData.followingPlayerID;
}

void GameSaveManager::deserializeCameraData(sf::Packet& packet, SavedCameraData& cameraData) {
    packet >> cameraData.position.x >> cameraData.position.y;
    packet >> cameraData.size.x >> cameraData.size.y;
    packet >> cameraData.zoom;
    packet >> cameraData.followingPlayerID;
}

void GameSaveManager::serializeUIData(sf::Packet& packet, const SavedUIData& uiData) {
    packet << uiData.showUI << uiData.showDebugInfo;
    packet << uiData.showOrbitPaths << uiData.showFuelRings;
    packet << uiData.showPlayerLabels;
}

void GameSaveManager::deserializeUIData(sf::Packet& packet, SavedUIData& uiData) {
    packet >> uiData.showUI >> uiData.showDebugInfo;
    packet >> uiData.showOrbitPaths >> uiData.showFuelRings;
    packet >> uiData.showPlayerLabels;
}

void GameSaveManager::serializePlayerState(sf::Packet& packet, const PlayerState& state) {
    // Reuse the exact same serialization as NetworkManager
    packet << state.playerID << state.position.x << state.position.y;
    packet << state.velocity.x << state.velocity.y;
    packet << state.rotation << state.isRocket << state.isOnGround << state.mass;
    packet << state.thrustLevel;

    // Fuel system data
    packet << state.currentFuel << state.maxFuel << state.isCollectingFuel;

    // Satellite system data
    packet << state.isSatellite << state.satelliteID << state.orbitAccuracy;
    packet << state.needsMaintenance << state.requestingSatelliteConversion;
    packet << state.newSatelliteID;
    packet << state.newRocketSpawnPos.x << state.newRocketSpawnPos.y;
}

void GameSaveManager::deserializePlayerState(sf::Packet& packet, PlayerState& state) {
    // Reuse the exact same deserialization as NetworkManager
    packet >> state.playerID >> state.position.x >> state.position.y;
    packet >> state.velocity.x >> state.velocity.y;
    packet >> state.rotation >> state.isRocket >> state.isOnGround >> state.mass;
    packet >> state.thrustLevel;

    // Fuel system data
    packet >> state.currentFuel >> state.maxFuel >> state.isCollectingFuel;

    // Satellite system data
    packet >> state.isSatellite >> state.satelliteID >> state.orbitAccuracy;
    packet >> state.needsMaintenance >> state.requestingSatelliteConversion;
    packet >> state.newSatelliteID;
    packet >> state.newRocketSpawnPos.x >> state.newRocketSpawnPos.y;
}

// === FILE I/O OPERATIONS ===

bool GameSaveManager::writePacketToFile(const std::string& filename, const sf::Packet& packet) {
    try {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
            return false;
        }

        // Write packet size first
        size_t packetSize = packet.getDataSize();
        file.write(reinterpret_cast<const char*>(&packetSize), sizeof(packetSize));

        // Write packet data
        file.write(static_cast<const char*>(packet.getData()), packetSize);

        file.close();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error writing save file: " << e.what() << std::endl;
        return false;
    }
}

bool GameSaveManager::readPacketFromFile(const std::string& filename, sf::Packet& packet) const {
    try {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file for reading: " << filename << std::endl;
            return false;
        }

        // Read packet size
        size_t packetSize;
        file.read(reinterpret_cast<char*>(&packetSize), sizeof(packetSize));
        if (file.gcount() != sizeof(packetSize)) {
            std::cerr << "Error: Invalid save file format" << std::endl;
            return false;
        }

        // Read packet data
        std::vector<char> buffer(packetSize);
        file.read(buffer.data(), packetSize);
        if (static_cast<size_t>(file.gcount()) != packetSize) {
            std::cerr << "Error: Incomplete save file" << std::endl;
            return false;
        }

        // Clear and populate packet
        packet.clear();
        packet.append(buffer.data(), packetSize);

        file.close();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error reading save file: " << e.what() << std::endl;
        return false;
    }
}

// === MAIN SAVE/LOAD OPERATIONS ===

bool GameSaveManager::saveGame(const GameSaveData& saveData, const std::string& saveName) {
    if (!validateSaveData(saveData)) {
        std::cerr << "Error: Invalid save data" << std::endl;
        return false;
    }

    std::string filename = saveName.empty() ?
        generateSaveFilename(saveData.saveName) :
        generateSaveFilename(saveName);

    sf::Packet packet;

    try {
        // Serialize save metadata
        packet << saveData.saveVersion << saveData.saveTimestamp;
        packet << saveData.saveName << saveData.playerName;

        // Serialize game state
        packet << static_cast<uint8_t>(saveData.gameMode);
        packet << saveData.gameTime << saveData.currentPlayerCount;

        // Serialize planets
        packet << static_cast<uint32_t>(saveData.planets.size());
        for (const auto& planet : saveData.planets) {
            serializePlanetData(packet, planet);
        }

        // Serialize players
        packet << static_cast<uint32_t>(saveData.players.size());
        for (const auto& player : saveData.players) {
            serializePlayerState(packet, player);
        }

        // Serialize satellites
        packet << static_cast<uint32_t>(saveData.satellites.size());
        for (const auto& satellite : saveData.satellites) {
            serializeSatelliteData(packet, satellite);
        }

        // Serialize camera and UI
        serializeCameraData(packet, saveData.camera);
        serializeUIData(packet, saveData.uiSettings);

        // Serialize game settings
        packet << saveData.enableAutomaticMaintenance << saveData.enableAutomaticCollection;
        packet << saveData.globalOrbitTolerance << saveData.collectionEfficiency;

        // Write to file
        return writePacketToFile(filename, packet);
    }
    catch (const std::exception& e) {
        std::cerr << "Error serializing save data: " << e.what() << std::endl;
        return false;
    }
}

bool GameSaveManager::loadGame(GameSaveData& saveData, const std::string& saveFilename) {
    std::string fullPath = saveDirectory + saveFilename;
    if (!endsWith(saveFilename, defaultSaveExtension)) {
        fullPath += defaultSaveExtension;
    }

    sf::Packet packet;
    if (!readPacketFromFile(fullPath, packet)) {
        return false;
    }

    try {
        // Clear existing data
        saveData = GameSaveData();

        // Deserialize save metadata
        packet >> saveData.saveVersion >> saveData.saveTimestamp;
        packet >> saveData.saveName >> saveData.playerName;

        // Check version compatibility
        if (saveData.saveVersion > GAME_SAVE_VERSION) {
            std::cerr << "Error: Save file version " << saveData.saveVersion
                << " is newer than supported version " << GAME_SAVE_VERSION << std::endl;
            return false;
        }

        // Deserialize game state
        uint8_t gameModeValue;
        packet >> gameModeValue;
        saveData.gameMode = static_cast<SavedGameMode>(gameModeValue);
        packet >> saveData.gameTime >> saveData.currentPlayerCount;

        // Deserialize planets
        uint32_t planetCount;
        packet >> planetCount;
        saveData.planets.clear();
        saveData.planets.reserve(planetCount);
        for (uint32_t i = 0; i < planetCount; ++i) {
            SavedPlanetData planet;
            deserializePlanetData(packet, planet);
            saveData.planets.push_back(planet);
        }

        // Deserialize players
        uint32_t playerCount;
        packet >> playerCount;
        saveData.players.clear();
        saveData.players.reserve(playerCount);
        for (uint32_t i = 0; i < playerCount; ++i) {
            PlayerState player;
            deserializePlayerState(packet, player);
            saveData.players.push_back(player);
        }

        // Deserialize satellites
        uint32_t satelliteCount;
        packet >> satelliteCount;
        saveData.satellites.clear();
        saveData.satellites.reserve(satelliteCount);
        for (uint32_t i = 0; i < satelliteCount; ++i) {
            SavedSatelliteData satellite;
            deserializeSatelliteData(packet, satellite);
            saveData.satellites.push_back(satellite);
        }

        // Deserialize camera and UI
        deserializeCameraData(packet, saveData.camera);
        deserializeUIData(packet, saveData.uiSettings);

        // Deserialize game settings
        packet >> saveData.enableAutomaticMaintenance >> saveData.enableAutomaticCollection;
        packet >> saveData.globalOrbitTolerance >> saveData.collectionEfficiency;

        return validateSaveData(saveData);
    }
    catch (const std::exception& e) {
        std::cerr << "Error deserializing save data: " << e.what() << std::endl;
        return false;
    }
}

// === CONVENIENCE METHODS ===

bool GameSaveManager::quickSave(const GameSaveData& saveData) {
    return saveGame(saveData, "quicksave");
}

bool GameSaveManager::quickLoad(GameSaveData& saveData) {
    return loadGame(saveData, "quicksave");
}

bool GameSaveManager::autoSave(const GameSaveData& saveData) {
    return saveGame(saveData, "autosave");
}

bool GameSaveManager::hasAutoSave() const {
    return saveExists("autosave");
}

bool GameSaveManager::loadAutoSave(GameSaveData& saveData) {
    return loadGame(saveData, "autosave");
}

// === FILE MANAGEMENT ===

std::vector<std::string> GameSaveManager::getSaveFileList() const {
    std::vector<std::string> saveFiles;

    std::cout << "Looking for saves in: " << saveDirectory << std::endl;

    try {
        if (!std::filesystem::exists(saveDirectory)) {
            std::cout << "Save directory does not exist, creating it..." << std::endl;
            std::filesystem::create_directories(saveDirectory);
            return saveFiles;
        }

        for (const auto& entry : std::filesystem::directory_iterator(saveDirectory)) {
            std::cout << "Found file: " << entry.path().string() << std::endl;
            if (entry.is_regular_file() &&
                endsWith(entry.path().string(), defaultSaveExtension)) {
                saveFiles.push_back(entry.path().stem().string());
                std::cout << "Added to save list: " << entry.path().stem().string() << std::endl;
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error listing save files: " << e.what() << std::endl;
    }

    std::cout << "Total saves found: " << saveFiles.size() << std::endl;
    return saveFiles;
}
bool GameSaveManager::deleteSaveFile(const std::string& saveFilename) {
    try {
        std::string fullPath = saveDirectory + saveFilename;
        if (!endsWith(saveFilename, defaultSaveExtension)) {
            fullPath += defaultSaveExtension;
        }

        return std::filesystem::remove(fullPath);
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error deleting save file: " << e.what() << std::endl;
        return false;
    }
}

bool GameSaveManager::saveExists(const std::string& saveFilename) const {
    std::string fullPath = saveDirectory + saveFilename;
    if (!endsWith(saveFilename, defaultSaveExtension)) {
        fullPath += defaultSaveExtension;
    }

    return std::filesystem::exists(fullPath);
}

// === UTILITY METHODS ===

std::string GameSaveManager::generateSaveFilename(const std::string& saveName) const {
    std::string cleanName = saveName.empty() ? "save" : saveName;

    // Remove invalid filename characters
    for (char& c : cleanName) {
        if (c == '<' || c == '>' || c == ':' || c == '"' ||
            c == '/' || c == '\\' || c == '|' || c == '?' || c == '*') {
            c = '_';
        }
    }

    return saveDirectory + cleanName + defaultSaveExtension;
}

bool GameSaveManager::validateSaveData(const GameSaveData& saveData) const {
    // Basic validation checks
    if (saveData.saveVersion == 0) return false;
    if (saveData.currentPlayerCount < 0) return false;
    if (saveData.players.size() > 100) return false; // Reasonable limit
    if (saveData.planets.empty()) return false; // Need at least one planet

    return true;
}

void GameSaveManager::setSaveDirectory(const std::string& directory) {
    saveDirectory = directory;
    if (!saveDirectory.empty() && saveDirectory.back() != '/' && saveDirectory.back() != '\\') {
        saveDirectory += "/";
    }

    try {
        std::filesystem::create_directories(saveDirectory);
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Warning: Could not create save directory: " << e.what() << std::endl;
    }
}

bool GameSaveManager::isSaveFileValid(const std::string& saveFilename) const {
    if (!saveExists(saveFilename)) return false;

    // Try to read just the version to validate
    try {
        sf::Packet packet;
        std::string fullPath = saveDirectory + saveFilename;
        if (!endsWith(saveFilename, defaultSaveExtension)) {
            fullPath += defaultSaveExtension;
        }

        if (!readPacketFromFile(fullPath, packet)) return false;

        uint32_t version;
        packet >> version;
        return version > 0 && version <= GAME_SAVE_VERSION;
    }
    catch (...) {
        return false;
    }
}

uint32_t GameSaveManager::getSaveFileVersion(const std::string& saveFilename) const {
    try {
        sf::Packet packet;
        std::string fullPath = saveDirectory + saveFilename;
        if (!endsWith(saveFilename, defaultSaveExtension)) {
            fullPath += defaultSaveExtension;
        }

        if (!readPacketFromFile(fullPath, packet)) return 0;

        uint32_t version;
        packet >> version;
        return version;
    }
    catch (...) {
        return 0;
    }
}

bool GameSaveManager::isSaveCompatible(const std::string& saveFilename) const {
    uint32_t version = getSaveFileVersion(saveFilename);
    return version > 0 && version <= GAME_SAVE_VERSION;
}

void GameSaveManager::printSaveInfo(const GameSaveData& saveData) const {
    std::cout << "=== SAVE GAME INFO ===" << std::endl;
    std::cout << "Save Name: " << saveData.saveName << std::endl;
    std::cout << "Player Name: " << saveData.playerName << std::endl;
    std::cout << "Game Mode: " << static_cast<int>(saveData.gameMode) << std::endl;
    std::cout << "Game Time: " << saveData.gameTime << "s" << std::endl;
    std::cout << "Players: " << saveData.players.size() << std::endl;
    std::cout << "Planets: " << saveData.planets.size() << std::endl;
    std::cout << "Satellites: " << saveData.satellites.size() << std::endl;
    std::cout << "Save Version: " << saveData.saveVersion << std::endl;

    // Format timestamp
    std::time_t time = static_cast<std::time_t>(saveData.saveTimestamp);
    std::cout << "Save Date: " << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << std::endl;
    std::cout << "=====================" << std::endl;
}

size_t GameSaveManager::getSaveFileSize(const std::string& saveFilename) const {
    try {
        std::string fullPath = saveDirectory + saveFilename;
        if (!endsWith(saveFilename, defaultSaveExtension)) {
            fullPath += defaultSaveExtension;
        }

        return std::filesystem::file_size(fullPath);
    }
    catch (...) {
        return 0;
    }
}

std::string GameSaveManager::getSaveTimestamp(const std::string& saveFilename) const {
    try {
        sf::Packet packet;
        std::string fullPath = saveDirectory + saveFilename;
        if (!endsWith(saveFilename, defaultSaveExtension)) {
            fullPath += defaultSaveExtension;
        }

        if (!readPacketFromFile(fullPath, packet)) return "";

        uint32_t version;
        uint64_t timestamp;
        packet >> version >> timestamp;

        std::time_t time = static_cast<std::time_t>(timestamp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    catch (...) {
        return "";
    }
}

#ifdef _WIN32
#pragma warning(pop)
#endif