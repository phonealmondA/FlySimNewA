#pragma once
#ifndef SAVE_GAME_MANAGER_H
#define SAVE_GAME_MANAGER_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <ctime>
#include "NetworkManager.h" // For PlayerState and serialization patterns
#include "SatelliteManager.h"
#include "Planet.h"

// Forward declarations
class Game;
class VehicleManager;
class SplitScreenManager;
class Player;

// Save file metadata
struct SaveFileInfo {
    std::string filename;
    std::string displayName;
    std::time_t saveTime;
    GameState gameMode;
    int playerCount;
    int satelliteCount;
    float gameTimeElapsed;
    sf::Vector2f cameraPosition;
    float zoomLevel;

    SaveFileInfo() : saveTime(0), gameMode(GameState::SINGLE_PLAYER),
        playerCount(1), satelliteCount(0), gameTimeElapsed(0.0f),
        cameraPosition(0.0f, 0.0f), zoomLevel(1.0f) {
    }
};

// Planet state for saving
struct PlanetSaveState {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float mass;
    float radius;
    sf::Color color;

    // Serialize to binary
    void serialize(std::ofstream& file) const;
    void deserialize(std::ifstream& file);
};

// Satellite state for saving (extends PlayerState)
struct SatelliteSaveState {
    PlayerState baseState;  // Use existing PlayerState as foundation
    std::string satelliteName;
    float stationKeepingEfficiency;
    float transferRange;
    bool enableAutomaticMaintenance;
    bool enableAutomaticCollection;

    // Target orbit information
    float targetSemiMajorAxis;
    float targetEccentricity;
    float targetInclination;

    void serialize(std::ofstream& file) const;
    void deserialize(std::ifstream& file);
};

// Complete game state for saving
struct GameSaveState {
    // Save metadata
    SaveFileInfo metadata;

    // Game mode and configuration
    GameState currentGameMode;
    float gameTimeElapsed;

    // Camera state
    sf::Vector2f cameraCenter;
    float zoomLevel;
    float targetZoom;

    // Planet states
    std::vector<PlanetSaveState> planets;

    // Player states (works for single/multiplayer)
    std::vector<PlayerState> players;
    std::vector<std::string> playerNames;

    // Satellite network state
    std::vector<SatelliteSaveState> satellites;
    SatelliteNetworkStats networkStats;

    // Game settings
    bool showTrajectories;
    bool showVelocityVectors;
    bool showGravityVectors;
    bool showSatelliteOrbits;

    // Multiplayer specific (if applicable)
    int localPlayerID;
    bool isMultiplayerGame;

    GameSaveState() : currentGameMode(GameState::SINGLE_PLAYER), gameTimeElapsed(0.0f),
        cameraCenter(0.0f, 0.0f), zoomLevel(1.0f), targetZoom(1.0f),
        showTrajectories(false), showVelocityVectors(false),
        showGravityVectors(false), showSatelliteOrbits(true),
        localPlayerID(0), isMultiplayerGame(false) {
    }
};

class SaveGameManager {
private:
    static const std::string SAVE_DIRECTORY;
    static const std::string SAVE_EXTENSION;
    static const uint32_t SAVE_FILE_VERSION;
    static const uint32_t SAVE_FILE_MAGIC; // Magic number to verify save files

    // Internal helpers
    std::string generateSaveFileName(const std::string& baseName) const;
    std::string getSaveFilePath(const std::string& filename) const;
    bool createSaveDirectory() const;
    bool isValidSaveFile(const std::string& filepath) const;

    // Serialization helpers (following NetworkManager patterns)
    void serializePlayerState(std::ofstream& file, const PlayerState& state) const;
    void deserializePlayerState(std::ifstream& file, PlayerState& state) const;
    void serializeVector2f(std::ofstream& file, const sf::Vector2f& vec) const;
    void deserializeVector2f(std::ifstream& file, sf::Vector2f& vec) const;
    void serializeColor(std::ofstream& file, const sf::Color& color) const;
    void deserializeColor(std::ifstream& file, sf::Color& color) const;

public:
    SaveGameManager();
    ~SaveGameManager() = default;

    // Core save/load functionality
    bool saveGame(const GameSaveState& gameState, const std::string& saveName);
    bool loadGame(const std::string& filename, GameSaveState& outGameState);
    bool deleteSave(const std::string& filename);

    // Save file management
    std::vector<SaveFileInfo> getAvailableSaves() const;
    std::vector<SaveFileInfo> getSavesByGameMode(GameState gameMode) const;
    bool saveExists(const std::string& filename) const;
    SaveFileInfo getSaveInfo(const std::string& filename) const;

    // Quick save/load
    bool quickSave(const GameSaveState& gameState);
    bool quickLoad(GameSaveState& outGameState);
    bool hasQuickSave() const;

    // Auto-save functionality
    bool autoSave(const GameSaveState& gameState);
    bool hasAutoSave() const;
    bool loadAutoSave(GameSaveState& outGameState);
    void setAutoSaveInterval(float intervalMinutes) { autoSaveInterval = intervalMinutes; }
    bool shouldAutoSave(float deltaTime);

    // Utility functions
    std::string formatSaveTime(std::time_t saveTime) const;
    size_t getTotalSaveFileSize() const;
    void cleanupOldSaves(int maxSaves = 10);

    // Game state extraction (these will be implemented in .cpp)
    GameSaveState extractGameState(const Game* game) const;
    GameSaveState extractSinglePlayerState(const VehicleManager* vehicleManager,
        const std::vector<Planet*>& planets,
        const SatelliteManager* satelliteManager,
        float gameTime, sf::Vector2f cameraCenter,
        float zoom) const;
    GameSaveState extractMultiplayerState(const std::vector<Player*>& players,
        const std::vector<Planet*>& planets,
        const SatelliteManager* satelliteManager,
        float gameTime, sf::Vector2f cameraCenter,
        float zoom, GameState gameMode) const;

private:
    float autoSaveInterval;     // Minutes between auto-saves
    float timeSinceAutoSave;    // Time accumulator for auto-save
    static const std::string QUICKSAVE_NAME;
    static const std::string AUTOSAVE_NAME;
};

// Game state extraction helpers (inline for performance)
inline PlayerState extractPlayerStateFromVehicleManager(const VehicleManager* vm, int playerID) {
    PlayerState state;
    state.playerID = playerID;

    if (vm && vm->getActiveVehicleType() == VehicleType::ROCKET) {
        const Rocket* rocket = vm->getRocket();
        state.position = rocket->getPosition();
        state.velocity = rocket->getVelocity();
        state.rotation = rocket->getRotation();
        state.isRocket = true;
        state.isOnGround = rocket->getIsOnGround();
        state.mass = rocket->getMass();
        state.thrustLevel = rocket->getThrustLevel();
        state.currentFuel = rocket->getCurrentFuel();
        state.maxFuel = rocket->getMaxFuel();
        state.isCollectingFuel = rocket->getIsCollectingFuel();
    }
    else if (vm && vm->getActiveVehicleType() == VehicleType::CAR) {
        const Car* car = vm->getCar();
        state.position = car->getPosition();
        state.velocity = car->getVelocity();
        state.rotation = car->getRotation();
        state.isRocket = false;
        state.isOnGround = true;
        state.mass = 1.0f; // Cars have fixed mass
        state.thrustLevel = 0.0f;
        state.currentFuel = 0.0f;
        state.maxFuel = 0.0f;
        state.isCollectingFuel = false;
    }

    return state;
}

inline PlanetSaveState extractPlanetState(const Planet* planet) {
    PlanetSaveState state;
    state.position = planet->getPosition();
    state.velocity = planet->getVelocity();
    state.mass = planet->getMass();
    state.radius = planet->getRadius();
    state.color = planet->getColor();
    return state;
}

#endif // SAVE_GAME_MANAGER_H