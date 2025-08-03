// VehicleManager.h
#pragma once
#include "Rocket.h"
#include "Car.h"
#include "Planet.h"
#include <memory>
#include <vector>

enum class VehicleType {
    ROCKET,
    CAR
};
class SatelliteManager;
class VehicleManager {
private:
    std::unique_ptr<Rocket> rocket;
    std::unique_ptr<Car> car;
    VehicleType activeVehicle;
    std::vector<Planet*> planets;
    SatelliteManager* satelliteManager;

public:
    VehicleManager(sf::Vector2f initialPos, const std::vector<Planet*>& planetList, SatelliteManager* satManager = nullptr);

    void switchVehicle();
    void update(float deltaTime);
    void draw(sf::RenderWindow& window);
    void drawWithConstantSize(sf::RenderWindow& window, float zoomLevel);

    // Pass through functions to active vehicle
    void applyThrust(float amount);
    void rotate(float amount);
    void drawVelocityVector(sf::RenderWindow& window, float scale = 1.0f);

    Rocket* getRocket() { return rocket.get(); }
    Car* getCar() { return car.get(); }
    GameObject* getActiveVehicle();
    // Satellite integration
    Rocket* getCurrentRocket() const;
    VehicleType getActiveVehicleType() const { return activeVehicle; }

    // Satellite system integration
    void setSatelliteManager(SatelliteManager* satManager) { satelliteManager = satManager; }
    SatelliteManager* getSatelliteManager() const { return satelliteManager; }
    bool canConvertToSatellite() const;
    int convertRocketToSatellite();
    sf::Vector2f findNearestPlanetSurface() const;
    void updateSatelliteManager();
};