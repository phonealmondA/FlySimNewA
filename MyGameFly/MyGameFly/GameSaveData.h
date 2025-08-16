// GameSaveData.h - Game State Serialization System
#pragma once
#include <SFML/Network.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "NetworkManager.h" // For PlayerState struct

// Game save version for compatibility checking
constexpr uint32_t GAME_SAVE_VERSION = 1;

// Game mode enumeration for save files
enum class SavedGameMode : uint8_t {
    SINGLE_PLAYER = 0,
    LOCAL_PC_MULTIPLAYER = 1,
    LAN_MULTIPLAYER = 2,
    ONLINE_MULTIPLAYER = 3
};

// Planet configuration data for saving
struct SavedPlanetData {
    sf::Vector2f position;
    float mass = 1000.0f;
    sf::Color color = sf::Color::Blue;
    bool isMainPlanet = false;
    float radius = 20.0f;

    SavedPlanetData() = default;
    SavedPlanetData(sf::Vector2f pos, float m, sf::Color c, bool isMain = false, float r = 20.0f)
        : position(pos), mass(m), color(c), isMainPlanet(isMain), radius(r) {
    }
};

// Satellite save data (extends PlayerState with additional satellite info)
struct SavedSatelliteData {
    PlayerState baseState;           // Reuse existing PlayerState
    std::string satelliteName;
    bool isOperational = true;
    float lastMaintenanceTime = 0.0f;
    int orbitPlanetID = -1;         // Which planet it's orbiting (-1 for none)

    SavedSatelliteData() = default;
    SavedSatelliteData(const PlayerState& state, const std::string& name)
        : baseState(state), satelliteName(name) {
    }
};

// Camera/view save data
struct SavedCameraData {
    sf::Vector2f position;
    sf::Vector2f size;
    float zoom = 1.0f;
    int followingPlayerID = -1;     // -1 if not following anyone

    SavedCameraData() = default;
    SavedCameraData(sf::Vector2f pos, sf::Vector2f sz, float z = 1.0f)
        : position(pos), size(sz), zoom(z) {
    }
};

// UI preferences save data
struct SavedUIData {
    bool showUI = true;
    bool showDebugInfo = false;
    bool showOrbitPaths = true;
    bool showFuelRings = true;
    bool showPlayerLabels = true;

    SavedUIData() = default;
};

// Main game save data structure
struct GameSaveData {
    // Save metadata
    uint32_t saveVersion = GAME_SAVE_VERSION;
    uint64_t saveTimestamp = 0;      // Unix timestamp
    std::string saveName;
    std::string playerName;

    // Game state
    SavedGameMode gameMode = SavedGameMode::SINGLE_PLAYER;
    float gameTime = 0.0f;           // Total game time elapsed
    int currentPlayerCount = 1;

    // World data
    std::vector<SavedPlanetData> planets;
    std::vector<PlayerState> players;
    std::vector<SavedSatelliteData> satellites;

    // View and UI state
    SavedCameraData camera;
    SavedUIData uiSettings;

    // Game settings
    bool enableAutomaticMaintenance = true;
    bool enableAutomaticCollection = true;
    float globalOrbitTolerance = 5.0f;
    float collectionEfficiency = 1.0f;

    GameSaveData() {
        // Set timestamp to current time
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        saveTimestamp = static_cast<uint64_t>(time_t);
    }
};

// Save game management class
class GameSaveManager {
private:
    std::string saveDirectory;
    std::string defaultSaveExtension;

    // Serialization helpers
    void serializePlanetData(sf::Packet& packet, const SavedPlanetData& planetData);
    void deserializePlanetData(sf::Packet& packet, SavedPlanetData& planetData);

    void serializeSatelliteData(sf::Packet& packet, const SavedSatelliteData& satelliteData);
    void deserializeSatelliteData(sf::Packet& packet, SavedSatelliteData& satelliteData);

    void serializeCameraData(sf::Packet& packet, const SavedCameraData& cameraData);
    void deserializeCameraData(sf::Packet& packet, SavedCameraData& cameraData);

    void serializeUIData(sf::Packet& packet, const SavedUIData& uiData);
    void deserializeUIData(sf::Packet& packet, SavedUIData& uiData);

    void serializePlayerState(sf::Packet& packet, const PlayerState& state);
    void deserializePlayerState(sf::Packet& packet, PlayerState& state);

    // File operations
    bool writePacketToFile(const std::string& filename, const sf::Packet& packet);
    bool readPacketFromFile(const std::string& filename, sf::Packet& packet) const;

    // Utility functions
    std::string generateSaveFilename(const std::string& saveName) const;
    bool validateSaveData(const GameSaveData& saveData) const;

public:
    GameSaveManager();
    ~GameSaveManager();

    // Main save/load operations
    bool saveGame(const GameSaveData& saveData, const std::string& saveName = "");
    bool loadGame(GameSaveData& saveData, const std::string& saveFilename);

    // Quick save/load (using default names)
    bool quickSave(const GameSaveData& saveData);
    bool quickLoad(GameSaveData& saveData);

    // Auto save functionality
    bool autoSave(const GameSaveData& saveData);
    bool hasAutoSave() const;
    bool loadAutoSave(GameSaveData& saveData);

    // Save file management
    std::vector<std::string> getSaveFileList() const;
    bool deleteSaveFile(const std::string& saveFilename);
    bool saveExists(const std::string& saveFilename) const;

    // Settings
    void setSaveDirectory(const std::string& directory);
    std::string getSaveDirectory() const { return saveDirectory; }

    // Validation and compatibility
    bool isSaveFileValid(const std::string& saveFilename) const;
    uint32_t getSaveFileVersion(const std::string& saveFilename) const;
    bool isSaveCompatible(const std::string& saveFilename) const;

    // Debugging and information
    void printSaveInfo(const GameSaveData& saveData) const;
    size_t getSaveFileSize(const std::string& saveFilename) const;
    std::string getSaveTimestamp(const std::string& saveFilename) const;
};