// NetworkManager.cpp - State Synchronization Version
#include "NetworkManager.h"
#include <iostream>

const float NetworkManager::HEARTBEAT_INTERVAL = 1.0f;

NetworkManager::NetworkManager()
    : role(NetworkRole::NONE),
    status(ConnectionStatus::DISCONNECTED),
    clientConnected(false),
    lastHeartbeat(0.0f),
    localPlayerID(-1),
    nextPlayerID(0),
    hasNewSpawnInfo(false),
    hasPendingConversion(false) {
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

    listener->setBlocking(false);
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
        sendHello();
        return true;
    }

    serverSocket.reset();
    return false;
}


bool NetworkManager::startAsHost(unsigned short port) {
    listener = std::make_unique<sf::TcpListener>();

    if (listener->listen(port) != sf::Socket::Status::Done) {
        std::cout << "Network: Failed to start host on port " << port << std::endl;
        return false;
    }

    listener->setBlocking(false);
    role = NetworkRole::HOST;
    status = ConnectionStatus::CONNECTED;
    clientConnected = false;
    localPlayerID = 0;  // Host is always Player 0
    nextPlayerID = 1;   // Next client will be Player 1

    std::cout << "Network: Host started on port " << port << std::endl;
    return true;
}

bool NetworkManager::connectAsClient(const std::string& ipAddress, unsigned short port) {
    serverSocket = std::make_unique<sf::TcpSocket>();

    // FIXED: Convert string IP to sf::IpAddress for SFML 3.0
    sf::IpAddress targetIP = sf::IpAddress::resolve(ipAddress).value_or(sf::IpAddress::Any);
    sf::Socket::Status result = serverSocket->connect(targetIP, port, sf::seconds(3.0f));

    if (result == sf::Socket::Status::Done) {
        role = NetworkRole::CLIENT;
        status = ConnectionStatus::CONNECTED;
        serverSocket->setBlocking(false);
        localPlayerID = -1;  // Will be assigned by host

        std::cout << "Network: Client connected to " << ipAddress << ":" << port << std::endl;
        sendHello();
        return true;
    }

    std::cout << "Network: Failed to connect to " << ipAddress << ":" << port << std::endl;
    serverSocket.reset();
    return false;
}

void NetworkManager::assignClientPlayerID() {
    if (role != NetworkRole::HOST || !clientConnected) return;

    PlayerSpawnInfo spawnInfo;
    spawnInfo.playerID = assignNewPlayerID();
    spawnInfo.isHost = false;

    // Generate spawn position
    const float PI = 3.14159265358979323846f;
    float angle = spawnInfo.playerID * (2 * PI / 4);
    sf::Vector2f planetCenter(400.0f, 300.0f);
    float spawnRadius = 100.0f + 30.0f;

    spawnInfo.spawnPosition = planetCenter + sf::Vector2f(
        std::cos(angle) * spawnRadius,
        std::sin(angle) * spawnRadius
    );

    sendPlayerSpawn(spawnInfo);
    std::cout << "Assigned Player ID " << spawnInfo.playerID << " to client" << std::endl;
}

void NetworkManager::update(float deltaTime) {
    if (role == NetworkRole::HOST && listener) {
        // Check for new client connections
        if (!clientConnected && !clientSocket) {
            clientSocket = std::make_unique<sf::TcpSocket>();
            if (listener->accept(*clientSocket) == sf::Socket::Status::Done) {
                std::cout << "Network: Client connected to host!" << std::endl;
                clientSocket->setBlocking(false);
                clientConnected = true;
                assignClientPlayerID();
            }
            else {
                clientSocket.reset();
            }
        }
    }

    handleHeartbeat(deltaTime);

    // Process incoming messages
    MessageType msgType;
    sf::Packet packet;
    while (receiveMessage(msgType, packet)) {
        switch (msgType) {
        case MessageType::HELLO:
            receiveHello();
            break;
        case MessageType::PLAYER_STATE: {
            PlayerState state;
            deserializePlayerState(packet, state);
            lastReceivedStates[state.playerID] = state;
            break;
        }
        case MessageType::PLAYER_SPAWN: {
            PlayerSpawnInfo spawnInfo;
            deserializePlayerSpawnInfo(packet, spawnInfo);

            if (role == NetworkRole::CLIENT && localPlayerID == -1) {
                localPlayerID = spawnInfo.playerID;
                std::cout << "Network: Assigned player ID " << localPlayerID << std::endl;
            }

            pendingSpawnInfo = spawnInfo;
            hasNewSpawnInfo = true;
            break;
        }
        case MessageType::PLAYER_DISCONNECT: {
            int playerID;
            packet >> playerID;
            lastReceivedStates.erase(playerID);
            std::cout << "Network: Player " << playerID << " disconnected" << std::endl;
            break;
        }
        case MessageType::TRANSFORM: {
            int playerID;
            bool toRocket;
            packet >> playerID >> toRocket;
            // Transform requests can be handled by the game layer
            break;
        }
        case MessageType::SATELLITE_CONVERSION: {
            SatelliteConversionInfo conversionInfo;
            deserializeSatelliteConversion(packet, conversionInfo);
            pendingSatelliteConversion = conversionInfo;
            hasPendingConversion = true;
            std::cout << "Network: Received satellite conversion from Player " << conversionInfo.playerID << std::endl;
            break;
        }
        case MessageType::DISCONNECT:
            std::cout << "Network: Remote player disconnected" << std::endl;
            disconnect();
            break;
        }
    }
}

// Core state synchronization method
bool NetworkManager::sendPlayerState(const PlayerState& state) {
    if (!isConnected()) return false;

    sf::Packet packet;
    serializePlayerState(packet, state);
    return sendMessage(MessageType::PLAYER_STATE, packet);
}

bool NetworkManager::receivePlayerState(int playerID, PlayerState& outState) {
    if (!isConnected()) return false;

    auto it = lastReceivedStates.find(playerID);
    if (it != lastReceivedStates.end()) {
        outState = it->second;
        return true;
    }
    return false;
}

bool NetworkManager::hasStateForPlayer(int playerID) const {
    return lastReceivedStates.find(playerID) != lastReceivedStates.end();
}

bool NetworkManager::sendPlayerSpawn(const PlayerSpawnInfo& spawnInfo) {
    if (!isConnected()) return false;

    sf::Packet packet;
    serializePlayerSpawnInfo(packet, spawnInfo);
    return sendMessage(MessageType::PLAYER_SPAWN, packet);
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
        sf::Vector2f spawn = planetCenter + direction * (planetRadius + 30.0f);
        spawns.push_back(spawn);
    }
    return spawns;
}

PlayerSpawnInfo NetworkManager::getNewPlayerInfo() {
    hasNewSpawnInfo = false;
    return pendingSpawnInfo;
}

bool NetworkManager::sendMessage(MessageType type, sf::Packet& packet) {
    if (!isConnected()) return false;

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

// FIXED: Fuel system serialization
void NetworkManager::serializePlayerState(sf::Packet& packet, const PlayerState& state) {
    packet << state.playerID << state.position.x << state.position.y;
    packet << state.velocity.x << state.velocity.y;
    packet << state.rotation << state.isRocket << state.isOnGround << state.mass;
    packet << state.thrustLevel;

    // ADD FUEL SYSTEM SERIALIZATION
    packet << state.currentFuel << state.maxFuel << state.isCollectingFuel;

    // ADD SATELLITE SYSTEM SERIALIZATION
    packet << state.isSatellite << state.satelliteID << state.orbitAccuracy << state.needsMaintenance;

    // ADD SATELLITE CONVERSION SERIALIZATION
    packet << state.requestingSatelliteConversion << state.newSatelliteID;
    packet << state.newRocketSpawnPos.x << state.newRocketSpawnPos.y;
}


void NetworkManager::deserializePlayerState(sf::Packet& packet, PlayerState& state) {
    packet >> state.playerID >> state.position.x >> state.position.y;
    packet >> state.velocity.x >> state.velocity.y;
    packet >> state.rotation >> state.isRocket >> state.isOnGround >> state.mass;
    packet >> state.thrustLevel;

    // ADD FUEL SYSTEM DESERIALIZATION
    packet >> state.currentFuel >> state.maxFuel >> state.isCollectingFuel;

    // ADD SATELLITE SYSTEM DESERIALIZATION
    packet >> state.isSatellite >> state.satelliteID >> state.orbitAccuracy >> state.needsMaintenance;

    // ADD SATELLITE CONVERSION DESERIALIZATION
    packet >> state.requestingSatelliteConversion >> state.newSatelliteID;
    packet >> state.newRocketSpawnPos.x >> state.newRocketSpawnPos.y;
}
void NetworkManager::serializePlayerSpawnInfo(sf::Packet& packet, const PlayerSpawnInfo& spawnInfo) {
    packet << spawnInfo.playerID << spawnInfo.spawnPosition.x << spawnInfo.spawnPosition.y;
    packet << spawnInfo.isHost;
}

void NetworkManager::deserializePlayerSpawnInfo(sf::Packet& packet, PlayerSpawnInfo& spawnInfo) {
    packet >> spawnInfo.playerID >> spawnInfo.spawnPosition.x >> spawnInfo.spawnPosition.y;
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
    return sendMessage(MessageType::HELLO, packet);
}

void NetworkManager::checkConnectionHealth() {
    // TODO: Implement connection timeout logic
}

bool NetworkManager::sendHello() {
    if (!isConnected()) return false;

    sf::Packet packet;
    std::string helloMsg = (role == NetworkRole::HOST) ? "HOST_HELLO" : "CLIENT_HELLO";
    packet << helloMsg;

    return sendMessage(MessageType::HELLO, packet);
}

bool NetworkManager::receiveHello() {
    return true;
}

void NetworkManager::disconnect() {
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

bool NetworkManager::sendSatelliteCreated(int satelliteID, sf::Vector2f position, sf::Vector2f velocity, float fuel) {
    if (!isConnected()) return false;

    sf::Packet packet;
    packet << satelliteID << position.x << position.y << velocity.x << velocity.y << fuel;

    return sendMessage(MessageType::PLAYER_SPAWN, packet); // Reuse spawn message type
}

bool NetworkManager::sendSatelliteState(const PlayerState& satelliteState) {
    if (!isConnected() || !satelliteState.isSatellite) return false;

    sf::Packet packet;
    serializePlayerState(packet, satelliteState);
    return sendMessage(MessageType::PLAYER_STATE, packet);
}

bool NetworkManager::receiveSatelliteState(int satelliteID, PlayerState& outState) {
    if (!isConnected()) return false;

    auto it = lastReceivedStates.find(satelliteID);
    if (it != lastReceivedStates.end() && it->second.isSatellite) {
        outState = it->second;
        return true;
    }
    return false;
}

void NetworkManager::syncSatelliteStates(const std::vector<PlayerState>& satelliteStates) {
    if (!isConnected()) return;

    for (const auto& state : satelliteStates) {
        if (state.isSatellite) {
            sendSatelliteState(state);
        }
    }

    std::cout << "Synchronized " << satelliteStates.size() << " satellite states" << std::endl;
}

bool NetworkManager::sendSatelliteConversion(const SatelliteConversionInfo& conversionInfo) {
    if (!isConnected()) return false;

    sf::Packet packet;
    serializeSatelliteConversion(packet, conversionInfo);
    return sendMessage(MessageType::SATELLITE_CONVERSION, packet);
}

bool NetworkManager::receiveSatelliteConversion(SatelliteConversionInfo& outInfo) {
    if (!hasPendingConversion) return false;

    outInfo = pendingSatelliteConversion;
    hasPendingConversion = false;
    return true;
}

bool NetworkManager::hasPendingSatelliteConversion() const {
    return hasPendingConversion;
}

SatelliteConversionInfo NetworkManager::getPendingSatelliteConversion() {
    hasPendingConversion = false;
    return pendingSatelliteConversion;
}

void NetworkManager::serializeSatelliteConversion(sf::Packet& packet, const SatelliteConversionInfo& conversionInfo) {
    packet << conversionInfo.playerID << conversionInfo.satelliteID;
    packet << conversionInfo.satellitePosition.x << conversionInfo.satellitePosition.y;
    packet << conversionInfo.satelliteVelocity.x << conversionInfo.satelliteVelocity.y;
    packet << conversionInfo.satelliteFuel;
    packet << conversionInfo.newRocketPosition.x << conversionInfo.newRocketPosition.y;
    packet << conversionInfo.satelliteName;
}

void NetworkManager::deserializeSatelliteConversion(sf::Packet& packet, SatelliteConversionInfo& conversionInfo) {
    packet >> conversionInfo.playerID >> conversionInfo.satelliteID;
    packet >> conversionInfo.satellitePosition.x >> conversionInfo.satellitePosition.y;
    packet >> conversionInfo.satelliteVelocity.x >> conversionInfo.satelliteVelocity.y;
    packet >> conversionInfo.satelliteFuel;
    packet >> conversionInfo.newRocketPosition.x >> conversionInfo.newRocketPosition.y;
    packet >> conversionInfo.satelliteName;
}