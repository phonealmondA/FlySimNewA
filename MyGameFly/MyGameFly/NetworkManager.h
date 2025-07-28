// NetworkManager.h
#pragma once
#include <SFML/Network.hpp>
#include <string>
#include <memory>

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

class NetworkManager {
private:
    NetworkRole role;
    ConnectionStatus status;

    // Host components
    std::unique_ptr<sf::TcpListener> listener;
    std::unique_ptr<sf::TcpSocket> clientSocket;

    // Client components  
    std::unique_ptr<sf::TcpSocket> serverSocket;

    static const unsigned short DEFAULT_PORT = 54000;

public:
    NetworkManager();
    ~NetworkManager();

    // Auto-discover and connect (ONLY HELLO TEST IMPLEMENTED)
    bool attemptAutoConnect();

    // Connection management placeholders
    void update();
    void disconnect();

    // Simple hello messaging (ONLY THING THAT WORKS)
    bool sendHello();
    bool receiveHello();

    // Status getters
    NetworkRole getRole() const { return role; }
    ConnectionStatus getStatus() const { return status; }
    bool isConnected() const { return status == ConnectionStatus::CONNECTED; }

    // Future functions (PLACEHOLDERS ONLY)
    bool sendGameState(/* game state data */) {
        // TODO: Serialize and send complete game state
        return false;
    }

    bool receiveGameState(/* game state data */) {
        // TODO: Receive and deserialize game state
        return false;
    }

    bool sendPlayerInput(/* input data */) {
        // TODO: Send player input commands
        return false;
    }

    bool receivePlayerInput(/* input data */) {
        // TODO: Receive remote player inputs
        return false;
    }

    void handleReconnection() {
        // TODO: Handle client reconnecting after disconnect
    }

    void setTimeoutSettings(float timeout) {
        // TODO: Configure network timeouts
    }

    bool isHostWaitingForClients() const {
        // TODO: Check if host is waiting for more players
        return false;
    }

private:
    void resetConnection();
    bool startAsHost();
    bool connectAsClient();
};