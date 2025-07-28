// NetworkManager.cpp
#include "NetworkManager.h"
#include <iostream>

const float NetworkManager::HEARTBEAT_INTERVAL = 1.0f; // Send heartbeat every second

NetworkManager::NetworkManager()
    : role(NetworkRole::NONE),
    status(ConnectionStatus::DISCONNECTED),
    inputSequenceNumber(0),
    worldStateSequenceNumber(0),
    clientConnected(false),
    lastHeartbeat(0.0f),
    localPlayerID(-1),  // Initialize as invalid
    nextPlayerID(0) {   // Host will assign IDs starting from 0
}
}

NetworkManager::~NetworkManager() {
    disconnect();
}

bool NetworkManager::attemptAutoConnect() {
    std::cout << "Network: Attempting auto-discovery..." << std::endl;

    // Try to connect as client first
    if (connectAsClient()) {
        std::cout << "Network: Connected as CLIENT" << std::endl;
        return true;
    }

    // If connection failed, become host
    if (startAsHost()) {
        std::cout << "Network: Started as HOST" << std::endl;
        return true;
    }

    std::cout << "Network: Auto-connect failed" << std::endl;
    return false;
}

bool NetworkManager::startAsHost() {
    listener = std::make_unique<sf::TcpListener>();

    if (listener->listen(DEFAULT_PORT) != sf::Socket::Status::Done) {
        std::cout << "Network: Failed to start host on port " << DEFAULT_PORT << std::endl;
        return false;
    }

    listener->setBlocking(false); // Non-blocking for polling
    role = NetworkRole::HOST;
    status = ConnectionStatus::CONNECTED;
    clientConnected = false;
    localPlayerID = 0;  // Host is always Player 0
    nextPlayerID = 1;   // Next client will be Player 1

    std::cout << "Network: Host started on port " << DEFAULT_PORT << std::endl;
    return true;
}

bool NetworkManager::connectAsClient() {
    serverSocket = std::make_unique<sf::TcpSocket>();

    sf::Socket::Status result = serverSocket->connect(sf::IpAddress::LocalHost, DEFAULT_PORT, sf::seconds(1.0f));

    if (result == sf::Socket::Status::Done) {
        role = NetworkRole::CLIENT;
        status = ConnectionStatus::CONNECTED;
        serverSocket->setBlocking(false);
        localPlayerID = -1;  // Will be assigned by host
        std::cout << "Network: Client connected to host" << std::endl;

        // Send initial hello
        sendHello();
        return true;
    }

    serverSocket.reset();
    return false;
}

void NetworkManager::update(float deltaTime) {
    if (role == NetworkRole::HOST && listener) {
        // Check for new client connections (non-blocking)
        if (!clientConnected && !clientSocket) {
            clientSocket = std::make_unique<sf::TcpSocket>();
            if (listener->accept(*clientSocket) == sf::Socket::Status::Done) {
                std::cout << "Network: Client connected to host!" << std::endl;
                clientSocket->setBlocking(false);
                clientConnected = true;
                sendHello(); // Send hello when client connects
            }
            else {
                clientSocket.reset(); // No connection yet
            }
        }
    }

    // Handle heartbeat and connection health
    handleHeartbeat(deltaTime);

    // Process incoming messages
    MessageType msgType;
    sf::Packet packet;
    while (receiveMessage(msgType, packet)) {
        switch (msgType) {
        case MessageType::HELLO:
            receiveHello();
            break;
        case MessageType::INPUT_UPDATE: {
            PlayerInput input;
            deserializePlayerInput(packet, input);
            lastReceivedInputs[input.playerID] = input;
            break;
        }
        case MessageType::WORLD_STATE:
            deserializeWorldState(packet, lastReceivedWorldState);
            break;
        case MessageType::PLAYER_SPAWN: {
            // TODO: Handle player spawn when Player class is ready
            /*
            PlayerSpawnInfo spawnInfo;
            deserializePlayerSpawnInfo(packet, spawnInfo);
            // Notify game to create new player
            */
            break;
        }
        case MessageType::PLAYER_DISCONNECT: {
            int playerID;
            packet >> playerID;
            lastReceivedInputs.erase(playerID);
            std::cout << "Network: Player " << playerID << " disconnected" << std::endl;
            break;
        }
        case MessageType::TRANSFORM: {
            int playerID;
            bool toRocket;
            packet >> playerID >> toRocket;
            // TODO: Handle transform request when Player class is ready
            break;
        }
        case MessageType::DISCONNECT:
            std::cout << "Network: Remote player disconnected" << std::endl;
            disconnect();
            break;
        default:
            std::cout << "Network: Unknown message type received" << std::endl;
            break;
        }
    }
}

bool NetworkManager::sendPlayerInput(int playerID, const PlayerInput& input) {
    if (!isConnected()) return false;

    sf::Packet packet;
    serializePlayerInput(packet, input);
    return sendMessage(MessageType::INPUT_UPDATE, packet);
}

bool NetworkManager::receivePlayerInput(int playerID, PlayerInput& outInput) {
    if (!isConnected()) return false;

    auto it = lastReceivedInputs.find(playerID);
    if (it != lastReceivedInputs.end()) {
        outInput = it->second;
        return true;
    }
    return false;
}

bool NetworkManager::hasInputForPlayer(int playerID) const {
    return lastReceivedInputs.find(playerID) != lastReceivedInputs.end();
}

bool NetworkManager::sendWorldState(const WorldState& state) {
    if (!isConnected() || role != NetworkRole::HOST) return false;

    sf::Packet packet;
    serializeWorldState(packet, state);
    return sendMessage(MessageType::WORLD_STATE, packet);
}

bool NetworkManager::receiveWorldState(WorldState& outState) {
    if (!isConnected() || role != NetworkRole::CLIENT) return false;

    // Return the last received world state
    outState = lastReceivedWorldState;
    return true;
}

bool NetworkManager::sendPlayerSpawn(const PlayerSpawnInfo& spawnInfo) {
    if (!isConnected()) return false;

    sf::Packet packet;
    serializePlayerSpawnInfo(packet, spawnInfo);
    return sendMessage(MessageType::PLAYER_SPAWN, packet);
}

bool NetworkManager::receivePlayerSpawn(PlayerSpawnInfo& outSpawnInfo) {
    // This would be handled in the main message loop
    // This method is for external access to spawn info
    return false; // TODO: Implement buffering if needed
}

bool NetworkManager::sendPlayerDisconnect(int playerID) {
    if (!isConnected()) return false;

    sf::Packet packet;
    packet << playerID;
    return sendMessage(MessageType::PLAYER_DISCONNECT, packet);
}

bool NetworkManager::sendTransformRequest(int playerID, bool toRocket) {
    if (!isConnected()) return false;

    sf::Packet packet;
    packet << playerID << toRocket;
    return sendMessage(MessageType::TRANSFORM, packet);
}

int NetworkManager::assignNewPlayerID() {
    if (role != NetworkRole::HOST) return -1;
    return nextPlayerID++;
}

std::vector<sf::Vector2f> NetworkManager::generateSpawnPositions(sf::Vector2f planetCenter, float planetRadius, int numPlayers) {
    std::vector<sf::Vector2f> spawns;
    const float PI = 3.14159265358979323846f;
    float angleStep = 2 * PI / numPlayers;

    for (int i = 0; i < numPlayers; i++) {
        float angle = i * angleStep;
        sf::Vector2f direction(std::cos(angle), std::sin(angle));
        sf::Vector2f spawn = planetCenter + direction * (planetRadius + 30.0f); // 30.0f = rocket clearance
        spawns.push_back(spawn);
    }
    return spawns;
}

bool NetworkManager::sendMessage(MessageType type, sf::Packet& packet) {
    if (!isConnected()) return false;

    // Prepend message type to packet
    sf::Packet finalPacket;
    finalPacket << static_cast<uint8_t>(type);
    finalPacket.append(packet.getData(), packet.getDataSize());

    sf::Socket::Status result = sf::Socket::Status::Error;

    if (role == NetworkRole::HOST && clientSocket && clientConnected) {
        result = clientSocket->send(finalPacket);
    }
    else if (role == NetworkRole::CLIENT && serverSocket) {
        result = serverSocket->send(finalPacket);
    }

    return result == sf::Socket::Status::Done;
}

bool NetworkManager::receiveMessage(MessageType& outType, sf::Packet& outPacket) {
    if (!isConnected()) return false;

    sf::Packet rawPacket;
    sf::Socket::Status result = sf::Socket::Status::Error;

    if (role == NetworkRole::HOST && clientSocket && clientConnected) {
        result = clientSocket->receive(rawPacket);
    }
    else if (role == NetworkRole::CLIENT && serverSocket) {
        result = serverSocket->receive(rawPacket);
    }

    if (result == sf::Socket::Status::Done) {
        uint8_t typeValue;
        rawPacket >> typeValue;
        outType = static_cast<MessageType>(typeValue);

        // Copy remaining data to output packet
        size_t remainingSize = rawPacket.getDataSize() - sizeof(uint8_t);
        if (remainingSize > 0) {
            const char* data = static_cast<const char*>(rawPacket.getData()) + sizeof(uint8_t);
            outPacket.clear();
            outPacket.append(data, remainingSize);
        }

        return true;
    }

    return false;
}

void NetworkManager::serializePlayerInput(sf::Packet& packet, const PlayerInput& input) {
    packet << input.playerID << input.thrust << input.reverseThrust << input.rotateLeft
        << input.rotateRight << input.transform << input.thrustLevel << input.sequenceNumber;
}

void NetworkManager::deserializePlayerInput(sf::Packet& packet, PlayerInput& input) {
    packet >> input.playerID >> input.thrust >> input.reverseThrust >> input.rotateLeft
        >> input.rotateRight >> input.transform >> input.thrustLevel >> input.sequenceNumber;
}

void NetworkManager::serializeWorldState(sf::Packet& packet, const WorldState& state) {
    // Serialize number of players first
    uint32_t numPlayers = static_cast<uint32_t>(state.players.size());
    packet << numPlayers;

    // Serialize each player
    for (const auto& player : state.players) {
        serializePlayerState(packet, player);
    }

    // Serialize planet data
    packet << state.planet1Position.x << state.planet1Position.y;
    packet << state.planet1Velocity.x << state.planet1Velocity.y;
    packet << state.planet2Position.x << state.planet2Position.y;
    packet << state.planet2Velocity.x << state.planet2Velocity.y;
    packet << state.sequenceNumber << state.maxPlayers;
}

void NetworkManager::deserializeWorldState(sf::Packet& packet, WorldState& state) {
    // Deserialize number of players
    uint32_t numPlayers;
    packet >> numPlayers;

    // Clear and resize players vector
    state.players.clear();
    state.players.resize(numPlayers);

    // Deserialize each player
    for (uint32_t i = 0; i < numPlayers; i++) {
        deserializePlayerState(packet, state.players[i]);
    }

    // Deserialize planet data
    packet >> state.planet1Position.x >> state.planet1Position.y;
    packet >> state.planet1Velocity.x >> state.planet1Velocity.y;
    packet >> state.planet2Position.x >> state.planet2Position.y;
    packet >> state.planet2Velocity.x >> state.planet2Velocity.y;
    packet >> state.sequenceNumber >> state.maxPlayers;
}

void NetworkManager::serializePlayerState(sf::Packet& packet, const PlayerState& state) {
    packet << state.playerID << state.position.x << state.position.y;
    packet << state.velocity.x << state.velocity.y;
    packet << state.rotation << state.isRocket << state.isOnGround << state.mass;
    packet << state.thrustLevel;
}

void NetworkManager::deserializePlayerState(sf::Packet& packet, PlayerState& state) {
    packet >> state.playerID >> state.position.x >> state.position.y;
    packet >> state.velocity.x >> state.velocity.y;
    packet >> state.rotation >> state.isRocket >> state.isOnGround >> state.mass;
    packet >> state.thrustLevel;
}

void NetworkManager::serializePlayerSpawnInfo(sf::Packet& packet, const PlayerSpawnInfo& spawnInfo) {
    packet << spawnInfo.playerID << spawnInfo.spawnPosition.x << spawnInfo.spawnPosition.y;
    packet << spawnInfo.isHost;
}

void NetworkManager::deserializePlayerSpawnInfo(sf::Packet& packet, PlayerSpawnInfo& spawnInfo) {
    packet >> spawnInfo.playerID >> spawnInfo.spawnPosition.x >> spawnInfo.spawnPosition.y;
    packet >> spawnInfo.isHost;
}(sf::Packet& packet, PlayerSpawnInfo& spawnInfo) {
    packet >> spawnInfo.playerID >> spawnInfo.spawnPosition.x >> spawnInfo.spawnPosition.y;
    packet >> spawnInfo.playerColor.r >> spawnInfo.playerColor.g >> spawnInfo.playerColor.b >> spawnInfo.playerColor.a;
    packet >> spawnInfo.isHost;
}

void NetworkManager::handleHeartbeat(float deltaTime) {
    lastHeartbeat += deltaTime;

    if (lastHeartbeat >= HEARTBEAT_INTERVAL) {
        sendHeartbeat();
        lastHeartbeat = 0.0f;
    }

    checkConnectionHealth();
}

bool NetworkManager::sendHeartbeat() {
    sf::Packet packet;
    std::string heartbeatMsg = "HEARTBEAT";
    packet << heartbeatMsg;
    return sendMessage(MessageType::HELLO, packet); // Reuse hello message type for heartbeat
}

void NetworkManager::checkConnectionHealth() {
    // TODO: Implement connection timeout logic
    // Check if we haven't received messages for too long
}

float NetworkManager::getLatency() const {
    // TODO: Implement RTT calculation using ping/pong messages
    return 0.0f;
}

int NetworkManager::getPacketsPerSecond() const {
    // TODO: Implement packet rate tracking
    return 0;
}

bool NetworkManager::sendHello() {
    if (!isConnected()) return false;

    sf::Packet packet;
    std::string helloMsg = (role == NetworkRole::HOST) ? "HOST_HELLO" : "CLIENT_HELLO";
    packet << helloMsg;

    return sendMessage(MessageType::HELLO, packet);
}

bool NetworkManager::receiveHello() {
    // Hello messages are now handled through the main message processing loop
    // This method is kept for compatibility but the actual processing happens in update()
    return true;
}

void NetworkManager::disconnect() {
    // Send disconnect message before closing
    if (isConnected()) {
        sf::Packet packet;
        std::string disconnectMsg = "DISCONNECT";
        packet << disconnectMsg;
        sendMessage(MessageType::DISCONNECT, packet);
    }

    if (clientSocket) {
        clientSocket->disconnect();
        clientSocket.reset();
    }

    if (serverSocket) {
        serverSocket->disconnect();
        serverSocket.reset();
    }

    if (listener) {
        listener->close();
        listener.reset();
    }

    role = NetworkRole::NONE;
    status = ConnectionStatus::DISCONNECTED;
    clientConnected = false;
    std::cout << "Network: Disconnected" << std::endl;
}

void NetworkManager::resetConnection() {
    disconnect();
}