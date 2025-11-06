// KatieFlySimRust - Main Entry Point
// Rust port of FlySimNewA space flight simulator

use sfml::graphics::{Color, RenderTarget, RenderWindow};
use sfml::system::Clock;
use sfml::window::{Event, Style};

use katie_fly_sim_rust::game_constants::GameConstants;
use katie_fly_sim_rust::utils::vector_helper;

fn main() {
    // Initialize logger
    env_logger::init();
    log::info!("Starting KatieFlySimRust...");

    // Create window
    let mut window = RenderWindow::new(
        (1920, 1080),
        "KatieFlySimRust - Space Flight Simulator",
        Style::CLOSE,
        &Default::default(),
    );
    window.set_framerate_limit(60);

    log::info!("Window created: 1920x1080 @ 60 FPS");
    log::info!("Gravitational constant G = {}", GameConstants::G);

    // Game loop clock
    let mut clock = Clock::start();
    let mut frame_count = 0u64;

    // Main game loop
    while window.is_open() {
        let delta_time = clock.restart().as_seconds();
        frame_count += 1;

        // Handle events
        while let Some(event) = window.poll_event() {
            match event {
                Event::Closed => {
                    log::info!("Window closed by user");
                    window.close();
                }
                Event::KeyPressed { code, .. } => {
                    log::debug!("Key pressed: {:?}", code);
                    // TODO: Handle input
                }
                _ => {}
            }
        }

        // Update game state
        // TODO: Implement game logic

        // Render
        window.clear(Color::BLACK);

        // TODO: Draw game objects

        window.display();

        // Log FPS every 60 frames (once per second at 60 FPS)
        if frame_count % 60 == 0 {
            let fps = 1.0 / delta_time;
            log::debug!("FPS: {:.2}", fps);
        }
    }

    log::info!("Game exited cleanly. Total frames: {}", frame_count);
}
