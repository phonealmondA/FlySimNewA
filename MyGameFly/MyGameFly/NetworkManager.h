// NetworkManager.h
#pragma once
#include <SFML/Network.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>  // For sf::Color
#include <string>
#include <memory>
#include <map>      // For std::map
#include <algorithm> // For std::remove_if
#include <cmath>    // For std::cos, std::sin

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
    INPUT_UPDATE = 1,
    WORLD_STATE = 2,
    PLAYER_SPAWN = 3,      // New: Send spawn info for new player
    PLAYER_DISCONNECT = 4, // New: Player leaving game
    TRANSFORM = 5,         // Individual transform requests
    CONNECTION_SYNC = 6,
    DISCONNECT = 7
};

// Data structures for network messages
struct PlayerInput {
    int playerID = 0;
    bool thrust = false;
    bool reverseThrust = false;
    bool rotateLeft = false;
    bool rotateRight = false;
    bool transform = false;
    float thrustLevel = 0.0f;  // Individual player thrust level
    uint32_t sequenceNumber = 0;
};

struct PlayerState {
    int playerID = 0;
    sf::Vector2f position;
    sf::Vector2f velocity;
    float rotation = 0.0f;
    bool isRocket = true;  // true = rocket, false = car
    bool isOnGround = false;
    float mass = 1.0f;
    float thrustLevel = 0.0f;  // Current thrust level for this player
};

struct WorldState {
    std::vector<PlayerState> players;  // All players in the game
    sf::Vector2f planet1Position;
    sf::Vector2f planet1Velocity;
    sf::Vector2f planet2Position;
    sf::Vector2f planet2Velocity;
    uint32_t sequenceNumber = 0;
    int maxPlayers = 4;  // Future expansion capability
};

// Player connection/spawning data
struct PlayerSpawnInfo {
    int playerID;
    sf::Vector2f spawnPosition;
    bool isHost = false;
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

    // Network state
    std::map<int, PlayerInput> lastReceivedInputs;  // Input per player ID
    WorldState lastReceivedWorldState;
    PlayerInput pendingInput;
    uint32_t inputSequenceNumber;
    uint32_t worldStateSequenceNumber;
    int localPlayerID;  // Which player ID this instance controls
    int nextPlayerID;   // For assigning IDs to new players (host only)

    // Connection state
    bool clientConnected;
    float lastHeartbeat;
    static const float HEARTBEAT_INTERVAL;
    static const unsigned short DEFAULT_PORT = 54000;

public:
    NetworkManager();
    ~NetworkManager();

    // Connection management
    bool attemptAutoConnect();
    void update(float deltaTime);
    void disconnect();

    // Input synchronization (player-specific)
    bool sendPlayerInput(int playerID, const PlayerInput& input);
    bool receivePlayerInput(int playerID, PlayerInput& outInput);
    bool hasInputForPlayer(int playerID) const;

    // World state synchronization
    bool sendWorldState(const WorldState& state);
    bool receiveWorldState(WorldState& outState);

    // Player management
    bool sendPlayerSpawn(const PlayerSpawnInfo& spawnInfo);
    bool receivePlayerSpawn(PlayerSpawnInfo& outSpawnInfo);
    bool sendPlayerDisconnect(int playerID);

    // Individual player controls (no more shared thrust)
    bool sendTransformRequest(int playerID, bool toRocket);

    // Player ID management
    int getLocalPlayerID() const { return localPlayerID; }
    int assignNewPlayerID(); // Host only: assigns next available player ID
    std::vector<sf::Vector2f> generateSpawnPositions(sf::Vector2f planetCenter, float planetRadius, int numPlayers);

    // Status getters
    NetworkRole getRole() const { return role; }
    ConnectionStatus getStatus() const { return status; }
    bool isConnected() const { return status == ConnectionStatus::CONNECTED && (role == NetworkRole::HOST ? clientConnected : true); }
    bool isHostWaitingForClients() const { return role == NetworkRole::HOST && !clientConnected; }

    // Network statistics
    float getLatency() const; // TODO: Implement ping calculation
    int getPacketsPerSecond() const; // TODO: Implement packet counting

private:
    void resetConnection();
    bool startAsHost();
    bool connectAsClient();

    // Message handling
    bool sendMessage(MessageType type, sf::Packet& packet);
    bool receiveMessage(MessageType& outType, sf::Packet& outPacket);

    // Serialization helpers
    void serializePlayerInput(sf::Packet& packet, const PlayerInput& input);
    void deserializePlayerInput(sf::Packet& packet, PlayerInput& input);
    void serializeWorldState(sf::Packet& packet, const WorldState& state);
    void deserializeWorldState(sf::Packet& packet, WorldState& state);
    void serializePlayerState(sf::Packet& packet, const PlayerState& state);
    void deserializePlayerState(sf::Packet& packet, PlayerState& state);
    void serializePlayerSpawnInfo(sf::Packet& packet, const PlayerSpawnInfo& spawnInfo);
    void deserializePlayerSpawnInfo(sf::Packet& packet, PlayerSpawnInfo& spawnInfo);

    // Connection management
    void handleHeartbeat(float deltaTime);
    bool sendHeartbeat();
    void checkConnectionHealth();

public:
    // Legacy hello system (for initial handshake) - moved to public for compatibility
    bool sendHello();
    bool receiveHello();
};