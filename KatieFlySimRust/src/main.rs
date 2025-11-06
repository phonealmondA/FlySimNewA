// KatieFlySimRust - Main Entry Point
// Rust port of FlySimNewA space flight simulator
// Fully integrated single player game

use sfml::graphics::{Color, Font, RenderTarget, RenderWindow};
use sfml::system::{Clock, Vector2f};
use sfml::window::{Event, Style};

use katie_fly_sim_rust::game_constants::GameConstants;
use katie_fly_sim_rust::game_modes::{SinglePlayerGame, SinglePlayerResult};
use katie_fly_sim_rust::game_state::{GameMode, GameState};
use katie_fly_sim_rust::menus::{MainMenu, SavesMenu, SavesMenuResult};
use katie_fly_sim_rust::save_system::GameSaveData;

fn main() {
    // Initialize logger
    env_logger::init();
    log::info!("Starting KatieFlySimRust v0.1.0");
    log::info!("Rust port of FlySimNewA - Space Flight Simulator");

    // Create window
    let window_width = 1920;
    let window_height = 1080;
    let mut window = RenderWindow::new(
        (window_width, window_height),
        "KatieFlySimRust - Space Flight Simulator",
        Style::CLOSE,
        &Default::default(),
    );
    window.set_framerate_limit(60);

    log::info!("Window created: {}x{} @ 60 FPS", window_width, window_height);
    log::info!("Gravitational constant G = {}", GameConstants::G);

    // Load font (using default system font - in real implementation would load from file)
    let font = match Font::from_memory(include_bytes!("../assets/font.ttf")) {
        Some(f) => f,
        None => {
            // Fallback: try to load from system
            log::warn!("Could not load embedded font, trying default");
            match Font::from_file("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf") {
                Some(f) => f,
                None => {
                    log::error!("Could not load any font! UI will not display text.");
                    // In real implementation, would exit here
                    // For now, we'll create a dummy font
                    return;
                }
            }
        }
    };

    let window_size = Vector2f::new(window_width as f32, window_height as f32);

    // Game state
    let mut game_state = GameState::MainMenu;
    let mut main_menu = MainMenu::new(window_size, &font);
    let mut saves_menu = SavesMenu::new(window_size, &font);
    let mut single_player_game: Option<SinglePlayerGame> = None;

    // Game loop clock
    let mut clock = Clock::start();
    let mut frame_count = 0u64;

    log::info!("Entering main game loop");

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
                _ => {
                    // Pass events to current game state
                    if game_state == GameState::Playing {
                        if let Some(ref mut game) = single_player_game {
                            match game.handle_event(&event) {
                                SinglePlayerResult::ReturnToMenu => {
                                    log::info!("Returning to main menu");
                                    game_state = GameState::MainMenu;
                                    main_menu.reset();
                                }
                                SinglePlayerResult::Quit => {
                                    window.close();
                                }
                                _ => {}
                            }
                        }
                    }
                }
            }
        }

        // Update based on game state
        match game_state {
            GameState::MainMenu => {
                let selected = main_menu.update(&window);
                match selected {
                    GameMode::SinglePlayer => {
                        log::info!("Single Player mode selected");
                        game_state = GameState::SavesMenu;
                        saves_menu.refresh_saves(&font, window_size);
                    }
                    GameMode::Multiplayer => {
                        log::info!("Multiplayer not yet implemented");
                    }
                    GameMode::Quit => {
                        log::info!("Quit selected");
                        window.close();
                    }
                    GameMode::None => {}
                }
            }

            GameState::SavesMenu => {
                let result = saves_menu.update(&window);
                match result {
                    SavesMenuResult::NewGame => {
                        log::info!("Starting new game");
                        let mut new_game = SinglePlayerGame::new(window_size, &font);
                        new_game.initialize_new_game();
                        single_player_game = Some(new_game);
                        game_state = GameState::Playing;
                    }
                    SavesMenuResult::LoadGame(save_name) => {
                        log::info!("Loading game: {}", save_name);
                        match GameSaveData::load_from_file(&save_name) {
                            Ok(save_data) => {
                                let mut loaded_game = SinglePlayerGame::new(window_size, &font);
                                loaded_game.load_from_save(save_data, save_name);
                                single_player_game = Some(loaded_game);
                                game_state = GameState::Playing;
                            }
                            Err(e) => {
                                log::error!("Failed to load save: {}", e);
                            }
                        }
                    }
                    SavesMenuResult::Back => {
                        log::info!("Returning to main menu from saves");
                        game_state = GameState::MainMenu;
                    }
                    SavesMenuResult::None => {}
                }
            }

            GameState::Playing => {
                if let Some(ref mut game) = single_player_game {
                    game.update(delta_time);
                }
            }

            GameState::Paused => {
                // Paused state - don't update game
            }

            GameState::Quit => {
                window.close();
            }
        }

        // Render based on game state
        window.clear(Color::BLACK);

        match game_state {
            GameState::MainMenu => {
                main_menu.draw(&mut window);
            }

            GameState::SavesMenu => {
                saves_menu.draw(&mut window);
            }

            GameState::Playing | GameState::Paused => {
                if let Some(ref game) = single_player_game {
                    game.render(&mut window);
                }
            }

            GameState::Quit => {}
        }

        window.display();

        // Log FPS every 60 frames
        if frame_count % 60 == 0 {
            let fps = 1.0 / delta_time;
            log::debug!("FPS: {:.2} | State: {:?}", fps, game_state);
        }
    }

    log::info!("Game exited cleanly");
    log::info!("Total frames: {}", frame_count);
    log::info!("Total playtime: {:.1} seconds", frame_count as f32 / 60.0);
}
