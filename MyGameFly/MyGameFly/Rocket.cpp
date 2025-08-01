#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <cmath>

Rocket::Rocket(sf::Vector2f pos, sf::Vector2f vel, sf::Color col, float m)
    : GameObject(pos, vel, col), rotation(0), angularVelocity(0), thrustLevel(0.0f), mass(m),
    currentFuel(GameConstants::ROCKET_STARTING_FUEL), maxFuel(GameConstants::ROCKET_MAX_FUEL),
    isCollectingFuel(false), fuelSourcePlanet(nullptr)
{
    // Create rocket body (a simple triangle)
    body.setPointCount(3);
    body.setPoint(0, { 0, -GameConstants::ROCKET_SIZE });
    body.setPoint(1, { -GameConstants::ROCKET_SIZE / 2, GameConstants::ROCKET_SIZE });
    body.setPoint(2, { GameConstants::ROCKET_SIZE / 2, GameConstants::ROCKET_SIZE });
    body.setFillColor(color);
    body.setOrigin({ 0, 0 });
    body.setPosition(position);

    // Add default engine
    addPart(std::make_unique<Engine>(sf::Vector2f(0, GameConstants::ROCKET_SIZE), GameConstants::ENGINE_THRUST_POWER));
}

void Rocket::addPart(std::unique_ptr<RocketPart> part)
{
    parts.push_back(std::move(part));
}

void Rocket::applyThrust(float amount)
{
    // Check if we have fuel to thrust
    if (!canThrust()) {
        return;  // No fuel, no thrust
    }

    // Calculate thrust direction based on rocket rotation
    float radians = rotation * 3.14159f / 180.0f;

    // In SFML, 0 degrees points up, 90 degrees points right
    // So we need to use -sin for x and -cos for y to get the direction
    sf::Vector2f thrustDir(std::sin(radians), -std::cos(radians));

    // Apply force and convert to acceleration by dividing by mass (F=ma -> a=F/m)
    velocity += thrustDir * amount * thrustLevel / mass;
}

void Rocket::rotate(float amount)
{
    angularVelocity += amount;
}

void Rocket::setThrustLevel(float level)
{
    // Clamp level between 0.0 and 1.0
    thrustLevel = std::max(0.0f, std::min(1.0f, level));
}

float Rocket::calculateFuelConsumption() const
{
    // No fuel consumption if thrust level is below minimum threshold
    if (thrustLevel < GameConstants::FUEL_CONSUMPTION_MIN_THRESHOLD) {
        return 0.0f;
    }

    // Base consumption at 10% thrust, then exponential increase
    // This makes higher thrust levels much more fuel-intensive
    float normalizedThrust = thrustLevel; // 0.0 to 1.0
    float consumption = GameConstants::FUEL_CONSUMPTION_BASE +
        (GameConstants::FUEL_CONSUMPTION_MULTIPLIER * normalizedThrust * normalizedThrust);

    return consumption;
}

void Rocket::consumeFuel(float deltaTime)
{
    if (thrustLevel > GameConstants::FUEL_CONSUMPTION_MIN_THRESHOLD && currentFuel > 0.0f) {
        float consumption = calculateFuelConsumption() * deltaTime;
        currentFuel = std::max(0.0f, currentFuel - consumption);
    }
}

void Rocket::collectFuelFromPlanets(float deltaTime)
{
    isCollectingFuel = false;
    fuelSourcePlanet = nullptr;

    // Don't collect if fuel is already full
    if (currentFuel >= maxFuel) {
        return;
    }

    // Check each nearby planet for fuel collection
    for (auto& planet : nearbyPlanets) {
        sf::Vector2f direction = position - planet->getPosition();
        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        // Check if we're within fuel collection range
        float collectionRange = planet->getRadius() + GameConstants::FUEL_COLLECTION_RANGE;

        if (distance <= collectionRange &&
            planet->getMass() > GameConstants::MIN_PLANET_MASS_FOR_COLLECTION) {

            // Calculate fuel collection amount
            float fuelToCollect = GameConstants::FUEL_COLLECTION_RATE * deltaTime;

            // Don't collect more fuel than we can hold
            fuelToCollect = std::min(fuelToCollect, maxFuel - currentFuel);

            // Calculate mass to remove from planet
            float massToRemove = fuelToCollect * GameConstants::FUEL_COLLECTION_MASS_RATIO;

            // Don't remove more mass than the planet can afford
            float availableMass = planet->getMass() - GameConstants::MIN_PLANET_MASS_FOR_COLLECTION;
            massToRemove = std::min(massToRemove, availableMass);

            // Recalculate actual fuel collected based on available mass
            float actualFuelCollected = massToRemove / GameConstants::FUEL_COLLECTION_MASS_RATIO;

            if (actualFuelCollected > 0.0f) {
                // Collect the fuel
                currentFuel += actualFuelCollected;

                // Remove mass from planet (this will also update its radius)
                planet->setMass(planet->getMass() - massToRemove);

                // Set collection status
                isCollectingFuel = true;
                fuelSourcePlanet = planet;

                // Only collect from one planet at a time
                break;
            }
        }
    }
}

bool Rocket::checkCollision(const Planet& planet)
{
    float dist = distance(position, planet.getPosition());
    // Simple collision check based on distance
    return dist < planet.getRadius() + GameConstants::ROCKET_SIZE; // Use constant for rocket size // 15 = approximate rocket size
}

bool Rocket::isColliding(const Planet& planet)
{
    return checkCollision(planet);
}

Rocket* Rocket::mergeWith(Rocket* other)
{
    // Create a new rocket with combined properties
    sf::Vector2f mergedPosition = (position + other->getPosition()) / 2.0f;

    // Conservation of momentum: (m1v1 + m2v2) / (m1 + m2)
    sf::Vector2f mergedVelocity = (velocity * mass + other->getVelocity() * other->getMass())
        / (mass + other->getMass());

    // Use the color of the more massive rocket
    sf::Color mergedColor = (mass > other->getMass()) ? color : other->color;

    // Create a new rocket with combined mass
    float mergedMass = mass + other->getMass();
    Rocket* mergedRocket = new Rocket(mergedPosition, mergedVelocity, mergedColor, mergedMass);

    // Combine fuel from both rockets
    float combinedFuel = currentFuel + other->getCurrentFuel();
    mergedRocket->setFuel(combinedFuel);

    // Combine thrust capabilities by adding an engine with combined thrust power
    float combinedThrust = 0.0f;

    // This is simplified - in a real implementation, you'd need to loop through
    // all engines from both rockets and sum their thrust values
    for (const auto& part : parts) {
        if (auto* engine = dynamic_cast<Engine*>(part.get())) {
            combinedThrust += engine->getThrust();
        }
    }

    for (const auto& part : other->parts) {
        if (auto* engine = dynamic_cast<Engine*>(part.get())) {
            combinedThrust += engine->getThrust();
        }
    }

    // Add a more powerful engine to the merged rocket
    mergedRocket->addPart(std::make_unique<Engine>(sf::Vector2f(0, GameConstants::ROCKET_SIZE), combinedThrust));

    return mergedRocket;
}

void Rocket::update(float deltaTime)
{
    bool resting = false;

    // Process fuel consumption first
    consumeFuel(deltaTime);

    // Process fuel collection from nearby planets
    collectFuelFromPlanets(deltaTime);

    // Check if we're resting on any planet
    for (const auto& planet : nearbyPlanets) {
        sf::Vector2f direction = position - planet->getPosition();
        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        // If we're at or below the surface of the planet
        if (distance <= (planet->getRadius() + GameConstants::ROCKET_SIZE)) {
            // Calculate normal force direction (away from planet center)
            sf::Vector2f normal = normalize(direction);

            // Project velocity onto normal to see if we're moving into the planet
            float velDotNormal = velocity.x * normal.x + velocity.y * normal.y;

            if (velDotNormal < 0) {
                // Remove velocity component toward the planet
                velocity -= normal * velDotNormal;

                // Apply a small friction to velocity parallel to surface
                sf::Vector2f tangent(-normal.y, normal.x);
                float velDotTangent = velocity.x * tangent.x + velocity.y * tangent.y;
                velocity = tangent * velDotTangent * 0.98f;

                // Position correction to stay exactly on surface
                position = planet->getPosition() + normal * (planet->getRadius() + GameConstants::ROCKET_SIZE);

                resting = true;
            }
        }
    }

    // Only apply normal updates if not resting on a planet
    if (!resting) {
        position += velocity * deltaTime;
    }

    // Update rotation based on angular velocity
    rotation += angularVelocity * deltaTime;

    // Apply some damping to angular velocity
    angularVelocity *= 0.98f;

    // Update body position and rotation
    body.setPosition(position);
    body.setRotation(sf::degrees(rotation));
}

void Rocket::draw(sf::RenderWindow& window)
{
    // Draw rocket body
    window.draw(body);

    // Draw all rocket parts
    for (const auto& part : parts) {
        part->draw(window, position, rotation);
    }
}

void Rocket::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel)
{
    // Store original position and scale
    sf::ConvexShape scaledBody = body;

    // Scale the body based on zoom level to maintain visual size
    float scaleMultiplier = zoomLevel;

    // Apply the scaling to the body shape
    // We're adjusting the points directly to keep the rocket centered properly
    for (size_t i = 0; i < scaledBody.getPointCount(); i++) {
        sf::Vector2f point = body.getPoint(i);
        scaledBody.setPoint(i, point * scaleMultiplier);
    }

    // Draw the scaled body
    window.draw(scaledBody);

    // Draw rocket parts with appropriate scaling
    for (const auto& part : parts) {
        part->draw(window, position, rotation, scaleMultiplier);
    }
}

void Rocket::drawVelocityVector(sf::RenderWindow& window, float scale)
{
    sf::VertexArray line(sf::PrimitiveType::LineStrip);

    sf::Vertex startVertex;
    startVertex.position = position;
    startVertex.color = sf::Color::Yellow;
    line.append(startVertex);

    sf::Vertex endVertex;
    endVertex.position = position + velocity * scale;
    endVertex.color = sf::Color::Red;
    line.append(endVertex);

    window.draw(line);
}

void Rocket::drawGravityForceVectors(sf::RenderWindow& window, const std::vector<Planet*>& planets, float scale)
{
    // Gravitational constant - same as in GravitySimulator
    const float G = GameConstants::G;  // Use the constant from the header

    // Draw gravity force vector for each planet
    for (const auto& planet : planets) {
        // Calculate direction and distance
        sf::Vector2f direction = planet->getPosition() - position;
        float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        // Skip if we're inside the planet or too close
        if (dist <= planet->getRadius() + 15.0f) {
            continue;
        }

        // Calculate gravitational force
        float forceMagnitude = G * planet->getMass() * mass / (dist * dist);
        sf::Vector2f forceVector = normalize(direction) * forceMagnitude;

        // Scale the force for visualization
        forceVector *= scale / mass;

        // Create a vertex array for the force line
        sf::VertexArray forceLine(sf::PrimitiveType::LineStrip);

        // Start point (at rocket position)
        sf::Vertex startVertex;
        startVertex.position = position;
        startVertex.color = sf::Color::Magenta; // Pink color
        forceLine.append(startVertex);

        // End point (force direction and magnitude)
        sf::Vertex endVertex;
        endVertex.position = position + forceVector;
        endVertex.color = sf::Color(255, 20, 147); // Deep pink
        forceLine.append(endVertex);

        // Draw the force vector
        window.draw(forceLine);
    }
}

void Rocket::drawTrajectory(sf::RenderWindow& window, const std::vector<Planet*>& planets,
    float timeStep, int steps, bool detectSelfIntersection) {
    // Create a vertex array for the trajectory line
    sf::VertexArray trajectory(sf::PrimitiveType::LineStrip);

    // Start with current position and velocity
    sf::Vector2f simPosition = position;
    sf::Vector2f simVelocity = velocity;

    // Add the starting point
    sf::Vertex startPoint;
    startPoint.position = simPosition;
    startPoint.color = sf::Color::Blue; // Blue at the beginning
    trajectory.append(startPoint);

    // Store previous positions
    std::vector<sf::Vector2f> previousPositions;
    previousPositions.push_back(simPosition);

    // Use the same gravitational constant as defined in GameConstants
    const float G = GameConstants::G;
    const float selfIntersectionThreshold = GameConstants::TRAJECTORY_COLLISION_RADIUS;

    // Create simulated planet positions and velocities
    std::vector<sf::Vector2f> simPlanetPositions;
    std::vector<sf::Vector2f> simPlanetVelocities;

    // Initialize simulated planet data
    for (const auto& planet : planets) {
        simPlanetPositions.push_back(planet->getPosition());
        simPlanetVelocities.push_back(planet->getVelocity());
    }

    // Simulate future positions using more accurate physics
    for (int i = 0; i < steps; i++) {
        // Update simulated planet positions
        for (size_t j = 0; j < planets.size(); j++) {
            simPlanetPositions[j] += simPlanetVelocities[j] * timeStep;
        }

        // Calculate gravitational interactions between planets
        for (size_t j = 0; j < planets.size(); j++) {
            // Skip the first planet (index 0) if it's pinned in place
            if (j == 0) continue;

            sf::Vector2f totalPlanetAcceleration(0, 0);

            // Calculate gravity from other planets
            for (size_t k = 0; k < planets.size(); k++) {
                if (j == k) continue; // Skip self

                sf::Vector2f direction = simPlanetPositions[k] - simPlanetPositions[j];
                float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                if (distance > planets[k]->getRadius() + planets[j]->getRadius()) {
                    float forceMagnitude = G * planets[k]->getMass() * planets[j]->getMass() / (distance * distance);
                    sf::Vector2f acceleration = normalize(direction) * forceMagnitude / planets[j]->getMass();
                    totalPlanetAcceleration += acceleration;
                }
            }

            // Update planet velocity
            simPlanetVelocities[j] += totalPlanetAcceleration * timeStep;
        }

        // Calculate gravitational forces from all planets
        sf::Vector2f totalAcceleration(0, 0);
        bool collisionDetected = false;

        // Calculate gravitational interactions between planets and rocket
        for (size_t j = 0; j < planets.size(); j++) {
            const auto& planet = planets[j];
            sf::Vector2f planetPos = simPlanetPositions[j];

            sf::Vector2f direction = planetPos - simPosition;
            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            // Check for planet collision using consistent collision radius
            if (distance <= planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                collisionDetected = true;
                break;
            }

            // Calculate gravitational force with proper inverse square law
            // F = G * (m1 * m2) / r^2
            float forceMagnitude = G * planet->getMass() * mass / (distance * distance);

            // Convert force to acceleration (a = F/m)
            sf::Vector2f acceleration = normalize(direction) * forceMagnitude / mass;
            totalAcceleration += acceleration;
        }

        // Stop if we hit a planet
        if (collisionDetected) {
            break;
        }

        // Use velocity Verlet integration for better numerical stability
        sf::Vector2f halfStepVelocity = simVelocity + totalAcceleration * (timeStep * 0.5f);
        simPosition += halfStepVelocity * timeStep;

        // Recalculate acceleration at new position for higher accuracy
        sf::Vector2f newAcceleration(0, 0);

        for (size_t j = 0; j < planets.size(); j++) {
            const auto& planet = planets[j];
            sf::Vector2f planetPos = simPlanetPositions[j];

            sf::Vector2f direction = planetPos - simPosition;
            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            if (distance <= planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                collisionDetected = true;
                break;
            }

            float forceMagnitude = G * planet->getMass() * mass / (distance * distance);
            sf::Vector2f acceleration = normalize(direction) * forceMagnitude / mass;
            newAcceleration += acceleration;
        }

        if (collisionDetected) {
            break;
        }

        // Complete the velocity update with the new acceleration
        simVelocity = halfStepVelocity + newAcceleration * (timeStep * 0.5f);

        // Self-intersection check if enabled
        if (detectSelfIntersection) {
            for (size_t j = 0; j < previousPositions.size() - 10; j++) {
                float distToPoint = distance(simPosition, previousPositions[j]);
                if (distToPoint < selfIntersectionThreshold) {
                    collisionDetected = true;
                    break;
                }
            }

            if (collisionDetected) {
                break;
            }
        }

        // Store this position for future self-intersection checks
        previousPositions.push_back(simPosition);

        // Calculate color gradient from blue to pink
        float ratio = static_cast<float>(i) / steps;
        sf::Color pointColor(
            51 + 204 * ratio,  // R: 51 (blue) to 255 (pink)
            51 + 0 * ratio,    // G: 51 (blue) to 51 (pink)
            255 - 155 * ratio  // B: 255 (blue) to 100 (pink)
        );

        // Add point to trajectory
        sf::Vertex point;
        point.position = simPosition;
        point.color = pointColor;
        trajectory.append(point);
    }

    // Draw the trajectory
    window.draw(trajectory);
}