#include "GameInfoDisplay.h"
#include "UIManager.h" 
#include "VehicleManager.h"
#include "SplitScreenManager.h"
#include "Player.h"
#include "Planet.h"
#include "Rocket.h"
#include "Car.h"
#include "VectorHelper.h"
#include "NetworkManager.h"
#include "GameConstants.h"
#include <sstream>
#include <iomanip>
#include <limits>
#include <cmath>


GameInfoDisplay::GameInfoDisplay(const sf::Font& gameFont, sf::Vector2u windowSize)
    : font(&gameFont), fontLoaded(true)
{
    // Create all panels with better spacing and sizes
    rocketInfoPanel = std::make_unique<TextPanel>(*font, 11, sf::Vector2f(10, 10), sf::Vector2f(270, 180));
    planetInfoPanel = std::make_unique<TextPanel>(*font, 11, sf::Vector2f(10, 200), sf::Vector2f(270, 120));
    orbitInfoPanel = std::make_unique<TextPanel>(*font, 11, sf::Vector2f(10, 330), sf::Vector2f(270, 100));
    controlsPanel = std::make_unique<TextPanel>(*font, 10, sf::Vector2f(10, 440), sf::Vector2f(270, 180));
    networkInfoPanel = std::make_unique<TextPanel>(*font, 11, sf::Vector2f(10, 630), sf::Vector2f(270, 80));

    // Set up controls panel with satellite system controls for ALL game modes
    controlsPanel->setText(
        "CONTROLS:\n"
        "Arrows/WASD: Move/Rotate\n"
        "1-9,0,=: Thrust level & fuel rate\n"
        ".: Collect fuel from planet\n"
        ",: Give fuel to planet\n"
        "L/K: Transform rocket/car\n"
        "T/Y: Convert rocket to satellite\n"
        "Mouse wheel: Zoom\n"
        "F1: Toggle UI, F2: Debug\n"
        "F3: Satellite status\n"
        "ESC: Menu"
    );
}


void GameInfoDisplay::updateAllPanels(GameState currentState, VehicleManager* vehicleManager,
    SplitScreenManager* splitScreenManager, Player* localPlayer,
    const std::vector<Planet*>& planets, NetworkManager* networkManager)
{
    if (!fontLoaded) return;

    updateVehicleInfoPanel(currentState, vehicleManager, splitScreenManager, localPlayer);
    updatePlanetInfoPanel(currentState, vehicleManager, splitScreenManager, localPlayer, planets);
    updateOrbitInfoPanel(currentState, vehicleManager, splitScreenManager, networkManager, localPlayer);
    updateNetworkInfoPanel(networkManager, localPlayer);
    updateSatelliteInfoPanel(currentState, vehicleManager, splitScreenManager, localPlayer);
}

void GameInfoDisplay::updateVehicleInfoPanel(GameState currentState, VehicleManager* vehicleManager,
    SplitScreenManager* splitScreenManager, Player* localPlayer)
{
    std::string content = generateVehicleInfo(currentState, vehicleManager, splitScreenManager, localPlayer);
    rocketInfoPanel->setText(content);
}

void GameInfoDisplay::updatePlanetInfoPanel(GameState currentState, VehicleManager* vehicleManager,
    SplitScreenManager* splitScreenManager, Player* localPlayer,
    const std::vector<Planet*>& planets)
{
    std::string content = generatePlanetInfo(currentState, vehicleManager, splitScreenManager, localPlayer, planets);
    planetInfoPanel->setText(content);
}

void GameInfoDisplay::updateOrbitInfoPanel(GameState currentState, VehicleManager* vehicleManager,
    SplitScreenManager* splitScreenManager, NetworkManager* networkManager, Player* localPlayer)
{
    std::string content = generateOrbitInfo(currentState, vehicleManager, splitScreenManager, networkManager, localPlayer);
    orbitInfoPanel->setText(content);
}

void GameInfoDisplay::updateNetworkInfoPanel(NetworkManager* networkManager, Player* localPlayer)
{
    std::string content = generateNetworkInfo(networkManager, localPlayer);
    networkInfoPanel->setText(content);
}

void GameInfoDisplay::updateSatelliteInfoPanel(GameState currentState, VehicleManager* vehicleManager,
    SplitScreenManager* splitScreenManager, Player* localPlayer)
{
    std::string content = generateSatelliteInfo(currentState, vehicleManager, splitScreenManager, localPlayer);
    // For now, append to existing network panel - could create separate panel later
    std::string currentContent = generateNetworkInfo(nullptr, localPlayer);
    networkInfoPanel->setText(currentContent + "\n" + content);
}

std::string GameInfoDisplay::generateVehicleInfo(GameState currentState, VehicleManager* vehicleManager,
    SplitScreenManager* splitScreenManager, Player* localPlayer) const
{
    std::stringstream ss;

    // Check game state and display appropriate vehicle info
    if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
        // Split-screen mode vehicle info with satellite support
        VehicleManager* p1 = splitScreenManager->getPlayer1();
        VehicleManager* p2 = splitScreenManager->getPlayer2();

        ss << "SPLIT-SCREEN MODE (SATELLITES)\n";
        ss << "P1 (Arrows + L + T): " << (p1->getActiveVehicleType() == VehicleType::ROCKET ? "ROCKET" : "CAR") << "\n";
        ss << "P2 (WASD + K + Y): " << (p2->getActiveVehicleType() == VehicleType::ROCKET ? "ROCKET" : "CAR") << "\n";

        if (p1->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* rocket = p1->getRocket();
            float thrustLevel = rocket->getThrustLevel();
            ss << "Shared Thrust: " << std::fixed << std::setprecision(0) << thrustLevel * 100.0f << "%\n";
            ss << "P1 Fuel: " << std::setprecision(1) << rocket->getCurrentFuel() << "/"
                << rocket->getMaxFuel() << " (" << std::setprecision(0) << rocket->getFuelPercentage() << "%)\n";
            ss << "P1 Mass: " << std::setprecision(1) << rocket->getMass() << "/" << rocket->getMaxMass() << "\n";
            ss << "P1 Can Convert: " << (rocket->getCurrentFuel() > 10.0f ? "YES" : "NO") << "\n";
        }

        if (p2->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* rocket2 = p2->getRocket();
            ss << "P2 Fuel: " << std::setprecision(1) << rocket2->getCurrentFuel() << "/"
                << rocket2->getMaxFuel() << " (" << std::setprecision(0) << rocket2->getFuelPercentage() << "%)\n";
            ss << "P2 Mass: " << std::setprecision(1) << rocket2->getMass() << "/" << rocket2->getMaxMass() << "\n";
            ss << "P2 Can Convert: " << (rocket2->getCurrentFuel() > 10.0f ? "YES" : "NO") << "\n";
        }

        ss << "Satellites auto-fuel both players";
    }
    else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
        // Network multiplayer vehicle info with satellite support
        std::string modeStr = (currentState == GameState::LAN_MULTIPLAYER) ? "LAN" : "ONLINE";

        if (localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* rocket = localPlayer->getVehicleManager()->getRocket();
            float speed = std::sqrt(rocket->getVelocity().x * rocket->getVelocity().x +
                rocket->getVelocity().y * rocket->getVelocity().y);

            ss << modeStr << " MULTIPLAYER (SATELLITES)\n"
                << "Player: " << localPlayer->getName() << "\n"
                << "Speed: " << std::fixed << std::setprecision(1) << speed << " units/s\n"
                << "Thrust: " << std::setprecision(0) << rocket->getThrustLevel() * 100.0f << "%\n"
                << "Fuel: " << std::setprecision(1) << rocket->getCurrentFuel() << "/"
                << rocket->getMaxFuel() << " (" << std::setprecision(0) << rocket->getFuelPercentage() << "%)\n"
                << "Mass: " << std::setprecision(1) << rocket->getMass() << "/" << rocket->getMaxMass() << "\n"
                << "Collecting: " << (rocket->getIsCollectingFuel() ? "YES" : "NO") << "\n"
                << "Can Convert: " << (rocket->getCurrentFuel() > 10.0f ? "YES" : "NO");
        }
        else {
            Car* car = localPlayer->getVehicleManager()->getCar();
            ss << modeStr << " MULTIPLAYER (CAR)\n"
                << "Player: " << localPlayer->getName() << "\n"
                << "On Ground: " << (car->isOnGround() ? "Yes" : "No") << "\n"
                << "Transform to rocket for satellites";
        }
    }
    else if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        // Single player rocket info (unchanged)
        Rocket* rocket = vehicleManager->getRocket();
        float speed = std::sqrt(rocket->getVelocity().x * rocket->getVelocity().x +
            rocket->getVelocity().y * rocket->getVelocity().y);

        ss << "ROCKET INFO (FUEL SYSTEM)\n"
            << "Speed: " << std::fixed << std::setprecision(1) << speed << " units/s\n"
            << "Thrust Level: " << std::setprecision(0) << rocket->getThrustLevel() * 100.0f << "%\n"
            << "Fuel: " << std::setprecision(1) << rocket->getCurrentFuel() << "/"
            << rocket->getMaxFuel() << " (" << std::setprecision(0) << rocket->getFuelPercentage() << "%)\n"
            << "Mass: " << std::setprecision(1) << rocket->getMass() << "/" << rocket->getMaxMass() << "\n"
            << "Transferring: " << (rocket->isTransferringFuel() ? "YES" : "NO") << "\n"
            << "Rate: " << std::setprecision(1) << rocket->getCurrentTransferRate() << " units/s\n"
            << "Can Thrust: " << (rocket->canThrust() ? "YES" : "NO");
    }
    else if (vehicleManager) {
        // Car info (unchanged)
        Car* car = vehicleManager->getCar();
        ss << "CAR INFO\n"
            << "On Ground: " << (car->isOnGround() ? "Yes" : "No") << "\n"
            << "Position: (" << std::fixed << std::setprecision(1)
            << car->getPosition().x << ", " << car->getPosition().y << ")\n"
            << "Orientation: " << std::setprecision(1) << car->getRotation() << " degrees\n"
            << "Press L to transform to rocket\n"
            << "Cars don't use fuel system";
    }

    return ss.str();
}


std::string GameInfoDisplay::generatePlanetInfo(GameState currentState, VehicleManager* vehicleManager,
    SplitScreenManager* splitScreenManager, Player* localPlayer,
    const std::vector<Planet*>& planets) const
{
    std::stringstream ss;
    Planet* closestPlanet = nullptr;
    float closestDistance = std::numeric_limits<float>::max();

    // Determine active vehicle position
    sf::Vector2f activeVehiclePos;
    bool validPosition = false;

    if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
        activeVehiclePos = splitScreenManager->getPlayer1()->getActiveVehicle()->getPosition();
        validPosition = true;
    }
    else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
        activeVehiclePos = localPlayer->getVehicleManager()->getActiveVehicle()->getPosition();
        validPosition = true;
    }
    else if (vehicleManager) {
        activeVehiclePos = vehicleManager->getActiveVehicle()->getPosition();
        validPosition = true;
    }

    if (validPosition) {
        closestPlanet = findClosestPlanet(activeVehiclePos, planets);

        if (closestPlanet) {
            float centerDistance = distance(activeVehiclePos, closestPlanet->getPosition());
            std::string planetName = "Planet"; // Could be enhanced to identify specific planets
            float speed = std::sqrt(closestPlanet->getVelocity().x * closestPlanet->getVelocity().x +
                closestPlanet->getVelocity().y * closestPlanet->getVelocity().y);
            float surfaceDistance = centerDistance - closestPlanet->getRadius() - GameConstants::ROCKET_SIZE;
            float fuelRange = closestPlanet->getFuelCollectionRange();
            bool inFuelRange = centerDistance <= fuelRange;

            ss << "NEAREST PLANET: " << planetName << "\n"
                << "Distance: " << std::fixed << std::setprecision(0) << surfaceDistance << " units\n"
                << "Mass: " << std::setprecision(0) << closestPlanet->getMass() << " units\n"
                << "Radius: " << std::setprecision(0) << closestPlanet->getRadius() << " units\n"
                << "Speed: " << std::setprecision(1) << speed << " units/s\n"
                << "Can collect fuel: " << (closestPlanet->canCollectFuel() ? "YES" : "NO") << "\n"
                << "In fuel range: " << (inFuelRange ? "YES" : "NO") << "\n"
                << "Fuel range: " << std::setprecision(0) << (fuelRange - closestPlanet->getRadius()) << " units";
        }
    }

    return ss.str();
}


std::string GameInfoDisplay::generateOrbitInfo(GameState currentState, VehicleManager* vehicleManager,
    SplitScreenManager* splitScreenManager, NetworkManager* networkManager, Player* localPlayer) const
{
    std::stringstream ss;

    if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
        ss << "SPLIT-SCREEN SATELLITES\n"
            << "Camera centered between players\n"
            << "Mouse wheel to zoom\n"
            << "Numbers 1-9,0,= control thrust\n"
            << "T/Y: Convert to satellite\n"
            << "Satellites auto-fuel rockets!";
    }
    else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && networkManager) {
        std::string modeStr = (currentState == GameState::LAN_MULTIPLAYER) ? "LAN" : "ONLINE";
        ss << modeStr << " SATELLITES\n"
            << "Role: " << (networkManager->getRole() == NetworkRole::HOST ? "HOST" : "CLIENT") << "\n"
            << "Status: " << (networkManager->isConnected() ? "CONNECTED" : "DISCONNECTED") << "\n";
        if (localPlayer) {
            ss << "Playing as: " << localPlayer->getName() << "\n";
        }
        ss << "T: Convert to satellite\n"
            << "Satellite network synced";
    }
    else if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        ss << "SATELLITE SYSTEM ACTIVE\n"
            << ".: Collect fuel from planet\n"
            << ",: Give fuel to planet\n"
            << "T: Convert rocket to satellite\n"
            << "1-9,0,=: Set thrust & transfer rate\n"
            << "Mouse wheel to zoom\n"
            << "Dynamic mass affects physics!";
    }
    else {
        ss << "CAR MODE\n"
            << "No fuel system in car mode\n"
            << "Transform to rocket (L) for fuel\n"
            << "Rockets start empty - need fuel!\n"
            << "Use T to create satellites!";
    }

    return ss.str();
}


std::string GameInfoDisplay::generateNetworkInfo(NetworkManager* networkManager, Player* localPlayer) const
{
    std::stringstream ss;

    if (networkManager) {
        std::string roleStr = "NONE";
        std::string statusStr = "DISCONNECTED";

        switch (networkManager->getRole()) {
        case NetworkRole::HOST: roleStr = "HOST"; break;
        case NetworkRole::CLIENT: roleStr = "CLIENT"; break;
        case NetworkRole::NONE: roleStr = "NONE"; break;
        }

        switch (networkManager->getStatus()) {
        case ConnectionStatus::CONNECTED: statusStr = "CONNECTED"; break;
        case ConnectionStatus::CONNECTING: statusStr = "CONNECTING"; break;
        case ConnectionStatus::FAILED: statusStr = "FAILED"; break;
        case ConnectionStatus::DISCONNECTED: statusStr = "DISCONNECTED"; break;
        }

        ss << "NETWORK SATELLITES\n"
            << "Role: " << roleStr << "\n"
            << "Status: " << statusStr << "\n";

        if (localPlayer) {
            ss << "Player ID: " << localPlayer->getID() << "\n";
        }

        ss << "Satellite sync: ACTIVE";

        // Show pending satellite conversions
        if (networkManager->hasPendingSatelliteConversion()) {
            ss << "\nPending conversion detected";
        }
    }
    else {
        ss << "SATELLITE SYSTEM\n"
            << "T: Convert rocket to satellite\n"
            << "F3: Show satellite status\n"
            << "F4: Optimize fuel network\n"
            << "Single player active";
    }

    return ss.str();
}


std::string GameInfoDisplay::generateSatelliteInfo(GameState currentState, VehicleManager* vehicleManager,
    SplitScreenManager* splitScreenManager, Player* localPlayer) const
{
    std::stringstream ss;

    // Show satellite fuel transfer status for all game modes
    if (currentState == GameState::SINGLE_PLAYER && vehicleManager &&
        vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {

        Rocket* rocket = vehicleManager->getRocket();
        if (rocket) {
            ss << "SATELLITE FUEL SUPPORT\n";
            ss << "Rocket fuel: " << std::fixed << std::setprecision(1)
                << rocket->getCurrentFuel() << "/" << rocket->getMaxFuel() << "\n";
            ss << "Satellites active for auto-fuel\n";
            ss << "T: Convert rocket to satellite";
        }
    }
    else if (currentState == GameState::LOCAL_PC_MULTIPLAYER && splitScreenManager) {
        ss << "SPLIT-SCREEN SATELLITES\n";
        ss << "Auto-fueling both players\n";

        if (splitScreenManager->getPlayer1()->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* p1Rocket = splitScreenManager->getPlayer1()->getRocket();
            ss << "P1: " << std::setprecision(0) << p1Rocket->getFuelPercentage() << "% fuel\n";
        }

        if (splitScreenManager->getPlayer2()->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* p2Rocket = splitScreenManager->getPlayer2()->getRocket();
            ss << "P2: " << std::setprecision(0) << p2Rocket->getFuelPercentage() << "% fuel\n";
        }

        ss << "T/Y: Convert to satellites";
    }
    else if ((currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) && localPlayer) {
        ss << "NETWORK SATELLITES\n";
        if (localPlayer->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* rocket = localPlayer->getVehicleManager()->getRocket();
            ss << "Fuel: " << std::setprecision(0) << rocket->getFuelPercentage() << "%\n";
            ss << "Auto-fuel from satellites active\n";
            ss << "T: Convert to satellite";
        }
        else {
            ss << "Switch to rocket for satellite fuel\n";
            ss << "T: Convert rocket to satellite";
        }
    }
    else {
        ss << "SATELLITE SYSTEM\n";
        ss << "Create satellites with T key\n";
        ss << "Satellites auto-fuel rockets";
    }

    return ss.str();
}

void GameInfoDisplay::drawAllPanels(sf::RenderWindow& window, GameState currentState,
    NetworkManager* networkManager) const
{
    if (!fontLoaded) return;

    rocketInfoPanel->draw(window);
    planetInfoPanel->draw(window);
    orbitInfoPanel->draw(window);
    controlsPanel->draw(window);

    if (networkManager || currentState == GameState::LAN_MULTIPLAYER || currentState == GameState::ONLINE_MULTIPLAYER) {
        networkInfoPanel->draw(window);
    }
}

Planet* GameInfoDisplay::findClosestPlanet(sf::Vector2f position, const std::vector<Planet*>& planets) const
{
    Planet* closest = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (const auto& planet : planets) {
        sf::Vector2f direction = planet->getPosition() - position;
        float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        if (dist < minDistance) {
            minDistance = dist;
            closest = planet;
        }
    }

    return closest;
}

float GameInfoDisplay::calculateApoapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G) const
{
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
    float energy = 0.5f * speed * speed - G * planetMass / distance;

    float semiMajor = -G * planetMass / (2 * energy);

    if (energy >= 0) return -1.0f;

    sf::Vector2f eVec;
    float vSquared = speed * speed;
    eVec.x = (vSquared * pos.x - (pos.x * vel.x + pos.y * vel.y) * vel.x) / (G * planetMass) - pos.x / distance;
    eVec.y = (vSquared * pos.y - (pos.x * vel.x + pos.y * vel.y) * vel.y) / (G * planetMass) - pos.y / distance;
    float ecc = std::sqrt(eVec.x * eVec.x + eVec.y * eVec.y);

    return semiMajor * (1 + ecc);
}

float GameInfoDisplay::calculatePeriapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G) const
{
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
    float energy = 0.5f * speed * speed - G * planetMass / distance;

    float semiMajor = -G * planetMass / (2 * energy);

    if (energy >= 0) return -1.0f;

    sf::Vector2f eVec;
    float vSquared = speed * speed;
    eVec.x = (vSquared * pos.x - (pos.x * vel.x + pos.y * vel.y) * vel.x) / (G * planetMass) - pos.x / distance;
    eVec.y = (vSquared * pos.y - (pos.x * vel.x + pos.y * vel.y) * vel.y) / (G * planetMass) - pos.y / distance;
    float ecc = std::sqrt(eVec.x * eVec.x + eVec.y * eVec.y);

    return semiMajor * (1 - ecc);
}

void GameInfoDisplay::repositionPanels(sf::Vector2u newWindowSize)
{
    // Adjust panel positions and sizes based on window size
    float windowHeight = static_cast<float>(newWindowSize.y);
    float panelWidth = std::min(280.0f, static_cast<float>(newWindowSize.x) * 0.22f);

    // Calculate spacing to fit all panels
    float availableHeight = windowHeight - 20.0f; // Leave 20px margin
    float totalPanelHeight = 180 + 120 + 100 + 180 + 80; // Sum of all panel heights
    float spacing = std::max(10.0f, (availableHeight - totalPanelHeight) / 4.0f);

    float currentY = 10.0f;

    // Reposition all panels
    rocketInfoPanel->setPosition(sf::Vector2f(10, currentY));
    rocketInfoPanel->setSize(sf::Vector2f(panelWidth, 180));
    currentY += 180 + spacing;

    planetInfoPanel->setPosition(sf::Vector2f(10, currentY));
    planetInfoPanel->setSize(sf::Vector2f(panelWidth, 120));
    currentY += 120 + spacing;

    orbitInfoPanel->setPosition(sf::Vector2f(10, currentY));
    orbitInfoPanel->setSize(sf::Vector2f(panelWidth, 100));
    currentY += 100 + spacing;

    controlsPanel->setPosition(sf::Vector2f(10, currentY));
    controlsPanel->setSize(sf::Vector2f(panelWidth, 180));
    currentY += 180 + spacing;

    networkInfoPanel->setPosition(sf::Vector2f(10, currentY));
    networkInfoPanel->setSize(sf::Vector2f(panelWidth, 80));
}

void GameInfoDisplay::adjustLayoutForWindowSize(sf::Vector2u windowSize)
{
    repositionPanels(windowSize);
}

void GameInfoDisplay::showAllPanels()
{
    // Implementation for showing all panels (could involve visibility flags)
}

void GameInfoDisplay::hideAllPanels()
{
    // Implementation for hiding all panels
}

void GameInfoDisplay::showPanel(const std::string& panelName)
{
    // Implementation for showing specific panel
}

void GameInfoDisplay::hidePanel(const std::string& panelName)
{
    // Implementation for hiding specific panel
}