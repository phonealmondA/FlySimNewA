# C++ to Rust Pattern Translation Guide

Quick reference for converting common C++ patterns from FlySimNewA to Rust.

---

## Table of Contents
1. [Basic Types](#basic-types)
2. [Classes and Structs](#classes-and-structs)
3. [Inheritance](#inheritance)
4. [Pointers and References](#pointers-and-references)
5. [Collections](#collections)
6. [Memory Management](#memory-management)
7. [Error Handling](#error-handling)
8. [Constants](#constants)
9. [SFML Types](#sfml-types)
10. [Common Patterns](#common-patterns)

---

## Basic Types

### C++
```cpp
int count = 0;
float mass = 100.0f;
double precision = 0.000001;
bool isActive = true;
```

### Rust
```rust
let count: i32 = 0;
let mass: f32 = 100.0;
let precision: f64 = 0.000001;
let is_active: bool = true;

// Rust uses type inference, so types are often optional:
let count = 0;      // i32 by default
let mass = 100.0;   // f64 by default (use 100.0f32 for f32)
```

---

## Classes and Structs

### C++ Class
```cpp
// Planet.h
class Planet {
private:
    sf::Vector2f position;
    float mass;
    float radius;

public:
    Planet(sf::Vector2f pos, float m, float r);
    void update(float deltaTime);
    float getMass() const { return mass; }
    void setMass(float m) { mass = m; }
};
```

### Rust Struct + Impl
```rust
// planet.rs
use sfml::system::Vector2f;

pub struct Planet {
    position: Vector2f,
    mass: f32,
    radius: f32,
}

impl Planet {
    // Constructor equivalent
    pub fn new(pos: Vector2f, m: f32, r: f32) -> Self {
        Planet {
            position: pos,
            mass: m,
            radius: r,
        }
    }

    pub fn update(&mut self, delta_time: f32) {
        // Implementation
    }

    // Getter (no need for const in Rust)
    pub fn mass(&self) -> f32 {
        self.mass
    }

    // Setter (requires &mut self)
    pub fn set_mass(&mut self, m: f32) {
        self.mass = m;
    }
}
```

---

## Inheritance

### C++ Inheritance
```cpp
// GameObject.h
class GameObject {
public:
    virtual void update(float deltaTime) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;
};

// Rocket.h
class Rocket : public GameObject {
public:
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;
};
```

### Rust Trait (Preferred)
```rust
// game_object.rs
use sfml::graphics::RenderWindow;

pub trait GameObject {
    fn update(&mut self, delta_time: f32);
    fn draw(&self, window: &mut RenderWindow);
}

// rocket.rs
pub struct Rocket {
    // fields
}

impl GameObject for Rocket {
    fn update(&mut self, delta_time: f32) {
        // Implementation
    }

    fn draw(&self, window: &mut RenderWindow) {
        // Implementation
    }
}
```

### Rust Enum (Alternative - Better for Rust)
```rust
pub enum GameObject {
    Rocket(Rocket),
    Planet(Planet),
    Satellite(Satellite),
}

impl GameObject {
    pub fn update(&mut self, delta_time: f32) {
        match self {
            GameObject::Rocket(r) => r.update(delta_time),
            GameObject::Planet(p) => p.update(delta_time),
            GameObject::Satellite(s) => s.update(delta_time),
        }
    }
}
```

---

## Pointers and References

### C++ Pointers
```cpp
// Raw pointer
Planet* planet = new Planet(pos, mass, radius);

// Reference parameter
void applyGravity(const Planet& planet);

// Pointer in collection
std::vector<Planet*> planets;
```

### Rust References and Ownership
```rust
// Ownership (no pointer needed)
let planet = Planet::new(pos, mass, radius);

// Immutable reference parameter
fn apply_gravity(planet: &Planet) {
    // Read-only access
}

// Mutable reference parameter
fn update_planet(planet: &mut Planet) {
    // Can modify
}

// Collection with ownership
let planets: Vec<Planet> = vec![];

// Collection with references (requires lifetime)
let planets: Vec<&Planet> = vec![];

// Collection with shared ownership (avoid if possible)
use std::rc::Rc;
use std::cell::RefCell;
let planets: Vec<Rc<RefCell<Planet>>> = vec![];
```

### C++ Problem: Circular References
```cpp
class Rocket {
    std::vector<Planet*> nearbyPlanets; // Raw pointers - danger!
};

class Planet {
    std::vector<Rocket*> orbitingRockets; // Circular reference
};
```

### Rust Solution: Entity IDs
```rust
pub type EntityId = usize;

pub struct Rocket {
    nearby_planet_ids: Vec<EntityId>, // Store IDs, not references
}

pub struct Planet {
    orbiting_rocket_ids: Vec<EntityId>,
}

pub struct World {
    planets: HashMap<EntityId, Planet>,
    rockets: HashMap<EntityId, Rocket>,
    next_id: EntityId,
}

impl World {
    pub fn add_planet(&mut self, planet: Planet) -> EntityId {
        let id = self.next_id;
        self.planets.insert(id, planet);
        self.next_id += 1;
        id
    }

    pub fn get_planet(&self, id: EntityId) -> Option<&Planet> {
        self.planets.get(&id)
    }

    pub fn get_planet_mut(&mut self, id: EntityId) -> Option<&mut Planet> {
        self.planets.get_mut(&id)
    }
}
```

---

## Collections

### C++ STL Containers
```cpp
#include <vector>
#include <map>
#include <memory>

std::vector<Planet> planets;
std::map<int, Player> players;
std::unique_ptr<Rocket> rocket;
```

### Rust Collections
```rust
use std::collections::HashMap;

let mut planets: Vec<Planet> = Vec::new();
let mut players: HashMap<i32, Player> = HashMap::new();
let rocket: Box<Rocket> = Box::new(Rocket::new()); // Usually not needed
let rocket = Rocket::new(); // Just use ownership directly
```

### Iterating Collections

**C++:**
```cpp
for (const auto& planet : planets) {
    planet.update(deltaTime);
}

for (auto& planet : planets) {
    planet.update(deltaTime); // Mutable
}
```

**Rust:**
```rust
// Immutable iteration
for planet in &planets {
    planet.update(delta_time);
}

// Mutable iteration
for planet in &mut planets {
    planet.update(delta_time);
}

// Consuming iteration (moves values)
for planet in planets {
    // planets is consumed after this loop
}
```

---

## Memory Management

### C++ Smart Pointers
```cpp
#include <memory>

std::unique_ptr<Planet> planet = std::make_unique<Planet>(pos, mass);
std::shared_ptr<Rocket> rocket = std::make_shared<Rocket>(pos, vel);

planet->update(deltaTime);
```

### Rust Ownership
```rust
// No pointer needed - direct ownership
let planet = Planet::new(pos, mass);

// Box for heap allocation (rarely needed)
let planet = Box::new(Planet::new(pos, mass));

// Rc for shared ownership (avoid if possible)
use std::rc::Rc;
let rocket = Rc::new(Rocket::new(pos, vel));
let rocket_clone = Rc::clone(&rocket); // Reference count increases

planet.update(delta_time); // Direct access
```

### Key Difference: Rust Uses Move Semantics by Default

**C++:**
```cpp
Planet p1(pos, mass);
Planet p2 = p1; // Copy (if copy constructor exists)
```

**Rust:**
```rust
let p1 = Planet::new(pos, mass);
let p2 = p1; // Move (p1 is no longer valid!)
// println!("{:?}", p1); // ERROR: p1 was moved

// To copy, implement Clone:
#[derive(Clone)]
struct Planet { ... }

let p1 = Planet::new(pos, mass);
let p2 = p1.clone(); // Explicit copy
println!("{:?}", p1); // OK: p1 still valid
```

---

## Error Handling

### C++ Exceptions
```cpp
#include <stdexcept>

Planet* loadPlanet(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }
    // ...
    return new Planet(...);
}

try {
    Planet* p = loadPlanet("planet.dat");
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

### Rust Result Type
```rust
use std::fs::File;
use std::io::Read;

fn load_planet(filename: &str) -> Result<Planet, String> {
    let mut file = File::open(filename)
        .map_err(|e| format!("Failed to open file: {}", e))?;

    // ... parse file

    Ok(Planet::new(pos, mass))
}

// Usage with pattern matching
match load_planet("planet.dat") {
    Ok(planet) => println!("Loaded planet"),
    Err(e) => eprintln!("Error: {}", e),
}

// Or use ? operator to propagate errors
fn load_all_planets() -> Result<Vec<Planet>, String> {
    let planet1 = load_planet("planet1.dat")?;
    let planet2 = load_planet("planet2.dat")?;
    Ok(vec![planet1, planet2])
}
```

---

## Constants

### C++ Constants
```cpp
// GameConstants.h
namespace GameConstants {
    constexpr float G = 100.0f;
    constexpr float ROCKET_BASE_MASS = 1.0f;

    // Runtime constant
    const float PLANET_ORBIT_DISTANCE = calculateOrbitDistance();
}
```

### Rust Constants
```rust
// game_constants.rs
pub const G: f32 = 100.0;
pub const ROCKET_BASE_MASS: f32 = 1.0;

// For runtime initialization, use lazy_static
use lazy_static::lazy_static;

lazy_static! {
    pub static ref PLANET_ORBIT_DISTANCE: f32 = calculate_orbit_distance();
}

// Or use once_cell (more modern)
use once_cell::sync::Lazy;

pub static PLANET_ORBIT_DISTANCE: Lazy<f32> = Lazy::new(|| {
    calculate_orbit_distance()
});
```

---

## SFML Types

### C++ SFML
```cpp
#include <SFML/Graphics.hpp>

sf::Vector2f position(100.0f, 200.0f);
sf::Color color(255, 0, 0); // Red
sf::RenderWindow window(sf::VideoMode(1920, 1080), "Game");

sf::CircleShape circle(50.0f);
circle.setFillColor(sf::Color::Green);
window.draw(circle);
```

### Rust SFML
```rust
use sfml::graphics::*;
use sfml::system::Vector2f;

let position = Vector2f::new(100.0, 200.0);
let color = Color::rgb(255, 0, 0); // Red
let mut window = RenderWindow::new(
    (1920, 1080),
    "Game",
    Style::DEFAULT,
    &Default::default(),
);

let mut circle = CircleShape::new(50.0, 30);
circle.set_fill_color(Color::GREEN);
window.draw(&circle);
```

### Vector Operations

**C++:**
```cpp
sf::Vector2f v1(10.0f, 20.0f);
sf::Vector2f v2(5.0f, 15.0f);
sf::Vector2f sum = v1 + v2;
float length = std::sqrt(v1.x * v1.x + v1.y * v1.y);
```

**Rust:**
```rust
use sfml::system::Vector2f;

let v1 = Vector2f::new(10.0, 20.0);
let v2 = Vector2f::new(5.0, 15.0);
let sum = v1 + v2;
let length = (v1.x * v1.x + v1.y * v1.y).sqrt();

// Or create helper module
pub mod vector_helper {
    use sfml::system::Vector2f;

    pub fn magnitude(v: Vector2f) -> f32 {
        (v.x * v.x + v.y * v.y).sqrt()
    }

    pub fn normalize(v: Vector2f) -> Vector2f {
        let mag = magnitude(v);
        if mag > 0.0 {
            Vector2f::new(v.x / mag, v.y / mag)
        } else {
            Vector2f::new(0.0, 0.0)
        }
    }
}
```

---

## Common Patterns

### Pattern 1: Game Loop

**C++:**
```cpp
int main() {
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Game");
    sf::Clock clock;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Update
        game.update(deltaTime);

        // Render
        window.clear(sf::Color::Black);
        game.render(window);
        window.display();
    }
}
```

**Rust:**
```rust
use sfml::graphics::*;
use sfml::window::*;
use sfml::system::Clock;

fn main() {
    let mut window = RenderWindow::new(
        (1920, 1080),
        "Game",
        Style::CLOSE,
        &Default::default(),
    );
    window.set_framerate_limit(60);

    let mut clock = Clock::start();

    while window.is_open() {
        let delta_time = clock.restart().as_seconds();

        // Handle events
        while let Some(event) = window.poll_event() {
            match event {
                Event::Closed => window.close(),
                _ => {}
            }
        }

        // Update
        game.update(delta_time);

        // Render
        window.clear(Color::BLACK);
        game.render(&mut window);
        window.display();
    }
}
```

### Pattern 2: Serialization

**C++ (manual):**
```cpp
void Planet::save(std::ofstream& file) {
    file.write(reinterpret_cast<const char*>(&mass), sizeof(mass));
    file.write(reinterpret_cast<const char*>(&radius), sizeof(radius));
}
```

**Rust (with serde):**
```rust
use serde::{Serialize, Deserialize};

#[derive(Serialize, Deserialize)]
pub struct Planet {
    mass: f32,
    radius: f32,
    position: (f32, f32), // SFML Vector2f doesn't impl Serialize
}

impl Planet {
    pub fn save(&self, path: &str) -> Result<(), std::io::Error> {
        let json = serde_json::to_string_pretty(self)?;
        std::fs::write(path, json)?;
        Ok(())
    }

    pub fn load(path: &str) -> Result<Self, std::io::Error> {
        let json = std::fs::read_to_string(path)?;
        let planet = serde_json::from_str(&json)?;
        Ok(planet)
    }
}
```

### Pattern 3: Builder Pattern

**C++:**
```cpp
Planet planet;
planet.setMass(100.0f);
planet.setRadius(50.0f);
planet.setColor(sf::Color::Blue);
```

**Rust (Builder):**
```rust
pub struct Planet {
    mass: f32,
    radius: f32,
    color: Color,
}

impl Planet {
    pub fn new() -> Self {
        Planet {
            mass: 100.0,
            radius: 50.0,
            color: Color::WHITE,
        }
    }

    pub fn with_mass(mut self, mass: f32) -> Self {
        self.mass = mass;
        self
    }

    pub fn with_radius(mut self, radius: f32) -> Self {
        self.radius = radius;
        self
    }

    pub fn with_color(mut self, color: Color) -> Self {
        self.color = color;
        self
    }
}

// Usage
let planet = Planet::new()
    .with_mass(100.0)
    .with_radius(50.0)
    .with_color(Color::BLUE);
```

---

## Key Takeaways

1. **No implicit copying** - Use `.clone()` explicitly
2. **No null pointers** - Use `Option<T>` instead
3. **No exceptions** - Use `Result<T, E>` instead
4. **No inheritance** - Use traits or enums
5. **Explicit mutability** - `&self` vs `&mut self`
6. **Entity IDs over raw pointers** - Solves circular reference issues
7. **Derive common traits** - `#[derive(Debug, Clone, Copy)]`
8. **Exhaustive pattern matching** - Compiler ensures all cases handled

---

## Helpful Macros

```rust
// Debug printing
println!("Position: {:?}", position);
dbg!(position); // Prints variable name and value

// Vector creation
use sfml::system::Vector2f;
let pos = Vector2f::new(10.0, 20.0);

// Panic with message (like assert)
assert!(fuel > 0.0, "Fuel cannot be negative");
panic!("Critical error occurred");

// Conditional compilation
#[cfg(debug_assertions)]
fn debug_info() {
    println!("Debug mode");
}
```

---

## Resources

- [Rust Book - Ownership](https://doc.rust-lang.org/book/ch04-00-understanding-ownership.html)
- [Rust by Example - Traits](https://doc.rust-lang.org/rust-by-example/trait.html)
- [SFML Rust Docs](https://docs.rs/sfml/)
- [Serde Documentation](https://serde.rs/)

---

**Good luck with the port! 🦀**
