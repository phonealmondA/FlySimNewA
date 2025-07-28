// NetworkManager.cpp
#include "NetworkManager.h"
#include <iostream>

NetworkManager::NetworkManager()
    : role(NetworkRole::NONE), status(ConnectionStatus::DISCONNECTED) {
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
    status = ConnectionStatus::CONNECTED; // Host runs immediately, doesn't wait

    std::cout << "Network: Host started on port " << DEFAULT_PORT << std::endl;
    return true;
}

bool NetworkManager::connectAsClient() {
    serverSocket = std::make_unique<sf::TcpSocket>();

    sf::Socket::Status result = serverSocket->connect(sf::IpAddress::LocalHost, DEFAULT_PORT, sf::seconds(1.0f));

    if (result == sf::Socket::Status::Done) {
        role = NetworkRole::CLIENT;
        status = ConnectionStatus::CONNECTED;
        std::cout << "Network: Client connected to host" << std::endl;
        return true;
    }

    serverSocket.reset();
    return false;
}

void NetworkManager::update() {
    if (role == NetworkRole::HOST && listener) {
        // Check for new client connections (non-blocking)
        if (!clientSocket) {
            clientSocket = std::make_unique<sf::TcpSocket>();
            if (listener->accept(*clientSocket) == sf::Socket::Status::Done) {
                std::cout << "Network: Client connected to host!" << std::endl;
                sendHello(); // Send hello when client connects
            }
            else {
                clientSocket.reset(); // No connection yet
            }
        }
    }

    // Check for incoming hello messages
    receiveHello();
}

bool NetworkManager::sendHello() {
    if (!isConnected()) return false;

    sf::Packet packet;
    std::string helloMsg = (role == NetworkRole::HOST) ? "HOST_HELLO" : "CLIENT_HELLO";
    packet << helloMsg;

    sf::Socket::Status result = sf::Socket::Status::Error;

    if (role == NetworkRole::HOST && clientSocket) {
        result = clientSocket->send(packet);
    }
    else if (role == NetworkRole::CLIENT && serverSocket) {
        result = serverSocket->send(packet);
    }

    if (result == sf::Socket::Status::Done) {
        std::cout << "Network: Sent " << helloMsg << std::endl;
        return true;
    }

    return false;
}

bool NetworkManager::receiveHello() {
    if (!isConnected()) return false;

    sf::Packet packet;
    sf::Socket::Status result = sf::Socket::Status::Error;

    if (role == NetworkRole::HOST && clientSocket) {
        clientSocket->setBlocking(false);
        result = clientSocket->receive(packet);
    }
    else if (role == NetworkRole::CLIENT && serverSocket) {
        serverSocket->setBlocking(false);
        result = serverSocket->receive(packet);
    }

    if (result == sf::Socket::Status::Done) {
        std::string message;
        packet >> message;
        std::cout << "Network: Received " << message << std::endl;

        // Auto-reply to hello (just for testing)
        if (message == "CLIENT_HELLO" && role == NetworkRole::HOST) {
            sendHello();
        }
        else if (message == "HOST_HELLO" && role == NetworkRole::CLIENT) {
            sendHello();
        }

        return true;
    }

    return false;
}

void NetworkManager::disconnect() {
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
    std::cout << "Network: Disconnected" << std::endl;
}

void NetworkManager::resetConnection() {
    disconnect();
}

/*
INTEGRATION NOTES FOR main.cpp:

1. Add to Game class private members:
   std::unique_ptr<NetworkManager> networkManager;

2. In MainMenu handling, when "Local Area Network" is selected:
   networkManager = std::make_unique<NetworkManager>();
   if (networkManager->attemptAutoConnect()) {
       // Proceed to game with network enabled
   }

3. In main game loop update:
   if (networkManager) {
       networkManager->update();
   }

4. Test procedure:
   - Launch instance 1 ? becomes host, shows "Host started"
   - Launch instance 2 ? becomes client, shows "Client connected"
   - Should see "HOST_HELLO" and "CLIENT_HELLO" messages exchanged
*/