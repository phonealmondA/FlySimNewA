// NetworkManager.h - State Synchronization Version with Fuel System
#pragma once
#include <SFML/Network.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>
#include <string>
#include <memory>
#include <map>
#include <algorithm>
#include <cmath>

enum class NetworkRole {
    NONE,
    HOST,
    CLIENT
};

enum class ConnectionStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED
};

enum class MessageType : uint8_t {
    HELLO = 0,
    PLAYER_STATE = 1,      // Individual player state updates
    PLAYER_SPAWN = 2,      // Player joining game
    PLAYER_DISCONNECT = 3, // Player leaving game
    TRANSFORM = 4,         // Transform requests
    SATELLITE_CONVERSION = 6, // Satellite conversion events
    DISCONNECT = 5    
};

// Simplified data structures for state sync - UPDATED WITH FUEL FIELDS
struct PlayerState {
    int playerID = 0;
    sf::Vector2f position;
    sf::Vector2f velocity;
    float rotation = 0.0f;
    bool isRocket = true;
    bool isOnGround = false;
    float mass = 1.0f;
    float thrustLevel = 0.0f;

    // FUEL SYSTEM FIELDS
    float currentFuel = 100.0f;
    float maxFuel = 100.0f;
    bool isCollectingFuel = false;

    // Satellite state fields
    bool isSatellite = false;
    int satelliteID = -1;
    float orbitAccuracy = 100.0f;
    bool needsMaintenance = false;
    // Satellite conversion tracking
    bool requestingSatelliteConversion = false;
    int newSatelliteID = -1;
    sf::Vector2f newRocketSpawnPos;
};

struct PlayerSpawnInfo {
    int playerID;
    sf::Vector2f spawnPosition;
    bool isHost = false;
};
struct SatelliteConversionInfo {
    int playerID;
    int satelliteID;
    sf::Vector2f satellitePosition;
    sf::Vector2f satelliteVelocity;
    float satelliteFuel;
    sf::Vector2f newRocketPosition;
    std::string satelliteName;
};


class NetworkManager {
private:
    NetworkRole role;
    ConnectionStatus status;

    // Host components
    std::unique_ptr<sf::TcpListener> listener;
    std::unique_ptr<sf::TcpSocket> clientSocket;

    // Client components  
    std::unique_ptr<sf::TcpSocket> serverSocket;

    // Network state - simplified for state sync
    std::map<int, PlayerState> lastReceivedStates;  // State per player ID
    int localPlayerID;
    int nextPlayerID;

    // Connection state
    bool clientConnected;
    float lastHeartbeat;
    static const float HEARTBEAT_INTERVAL;
    static const unsigned short DEFAULT_PORT = 54000;

    // Player spawn tracking
    PlayerSpawnInfo pendingSpawnInfo;
    bool hasNewSpawnInfo;
    // Satellite conversion tracking
    SatelliteConversionInfo pendingSatelliteConversion;
    bool hasPendingConversion;


public:
    NetworkManager();
    ~NetworkManager();

    // Connection management
    bool attemptAutoConnect();
    void update(float deltaTime);
    void disconnect();

    // State synchronization - the new core!
    bool sendPlayerState(const PlayerState& state);
    bool receivePlayerState(int playerID, PlayerState& outState);
    bool hasStateForPlayer(int playerID) const;

    // Player management
    bool sendPlayerSpawn(const PlayerSpawnInfo& spawnInfo);
    bool sendPlayerDisconnect(int playerID);
    bool sendTransformRequest(int playerID, bool toRocket);

    // Satellite network synchronization
    bool sendSatelliteCreated(int satelliteID, sf::Vector2f position, sf::Vector2f velocity, float fuel);
    bool sendSatelliteState(const PlayerState& satelliteState);
    bool receiveSatelliteState(int satelliteID, PlayerState& outState);
    void syncSatelliteStates(const std::vector<PlayerState>& satelliteStates);
    // Satellite conversion synchronization
    bool sendSatelliteConversion(const SatelliteConversionInfo& conversionInfo);
    bool receiveSatelliteConversion(SatelliteConversionInfo& outInfo);
    bool hasPendingSatelliteConversion() const;
    SatelliteConversionInfo getPendingSatelliteConversion();


    // Player ID management
    int getLocalPlayerID() const { return localPlayerID; }
    int assignNewPlayerID();
    std::vector<sf::Vector2f> generateSpawnPositions(sf::Vector2f planetCenter, float planetRadius, int numPlayers);

    // Player spawn management
    void assignClientPlayerID();
    bool hasNewPlayer() const { return hasNewSpawnInfo; }
    PlayerSpawnInfo getNewPlayerInfo();

    // Status getters
    NetworkRole getRole() const { return role; }
    ConnectionStatus getStatus() const { return status; }
    bool isConnected() const { return status == ConnectionStatus::CONNECTED && (role == NetworkRole::HOST ? clientConnected : true); }

    // Network statistics
    float getLatency() const { return 0.0f; }
    int getPacketsPerSecond() const { return 0; }

private:
    void resetConnection();
    bool startAsHost();
    bool connectAsClient();

    // Message handling
    bool sendMessage(MessageType type, sf::Packet& packet);
    bool receiveMessage(MessageType& outType, sf::Packet& outPacket);

    // Serialization helpers - UPDATED WITH FUEL FIELDS
    void serializePlayerState(sf::Packet& packet, const PlayerState& state);
    void deserializePlayerState(sf::Packet& packet, PlayerState& state);
    void serializePlayerSpawnInfo(sf::Packet& packet, const PlayerSpawnInfo& spawnInfo);
    void deserializePlayerSpawnInfo(sf::Packet& packet, PlayerSpawnInfo& spawnInfo);

    void serializeSatelliteConversion(sf::Packet& packet, const SatelliteConversionInfo& conversionInfo);
    void deserializeSatelliteConversion(sf::Packet& packet, SatelliteConversionInfo& conversionInfo);

    // Connection management
    void handleHeartbeat(float deltaTime);
    bool sendHeartbeat();
    void checkConnectionHealth();

public:
    // Legacy hello system
    bool sendHello();
    bool receiveHello();
};