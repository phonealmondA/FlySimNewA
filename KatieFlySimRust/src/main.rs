// KatieFlySimRust - Main Entry Point
// Rust port of FlySimNewA space flight simulator
// Now using macroquad for pure Rust graphics (no external dependencies!)

use macroquad::prelude::*;

use katie_fly_sim_rust::game_constants::GameConstants;
use katie_fly_sim_rust::game_modes::{SinglePlayerGame, SinglePlayerResult};
use katie_fly_sim_rust::game_state::{GameMode, GameState};
use katie_fly_sim_rust::menus::{MainMenu, SavesMenu, SavesMenuResult};
use katie_fly_sim_rust::save_system::GameSaveData;

// Window configuration
fn window_conf() -> Conf {
    Conf {
        window_title: "KatieFlySimRust - Space Flight Simulator".to_owned(),
        window_width: 1920,
        window_height: 1080,
        window_resizable: false,
        fullscreen: false,
        ..Default::default()
    }
}

#[macroquad::main(window_conf)]
async fn main() {
    // Initialize logger
    env_logger::init();
    log::info!("Starting KatieFlySimRust v0.1.0");
    log::info!("Rust port of FlySimNewA - Space Flight Simulator");
    log::info!("Using macroquad for pure Rust graphics (no SFML dependency!)");

    let window_width = screen_width();
    let window_height = screen_height();

    log::info!("Window created: {}x{}", window_width, window_height);
    log::info!("Gravitational constant G = {}", GameConstants::G);

    let window_size = Vec2::new(window_width, window_height);

    // Game state
    let mut game_state = GameState::MainMenu;
    let mut main_menu = MainMenu::new(window_size);
    let mut saves_menu = SavesMenu::new(window_size);
    let mut single_player_game: Option<SinglePlayerGame> = None;

    // Frame tracking
    let mut frame_count = 0u64;
    let mut fps_timer = 0.0f32;

    log::info!("Entering main game loop");

    // Main game loop
    loop {
        let delta_time = get_frame_time();
        frame_count += 1;
        fps_timer += delta_time;

        // Handle input based on game state
        match game_state {
            GameState::MainMenu => {
                let selected = main_menu.update();
                match selected {
                    GameMode::SinglePlayer => {
                        log::info!("Single Player mode selected");
                        game_state = GameState::SavesMenu;
                        saves_menu.refresh_saves();
                    }
                    GameMode::Multiplayer => {
                        log::info!("Multiplayer not yet implemented");
                    }
                    GameMode::Quit => {
                        log::info!("Quit selected");
                        break;
                    }
                    GameMode::None => {}
                }
            }

            GameState::SavesMenu => {
                let result = saves_menu.update();
                match result {
                    SavesMenuResult::NewGame => {
                        log::info!("Starting new game");
                        let mut new_game = SinglePlayerGame::new(window_size);
                        new_game.initialize_new_game();
                        single_player_game = Some(new_game);
                        game_state = GameState::Playing;
                    }
                    SavesMenuResult::LoadGame(save_name) => {
                        log::info!("Loading game: {}", save_name);
                        match GameSaveData::load_from_file(&save_name) {
                            Ok(save_data) => {
                                let mut loaded_game = SinglePlayerGame::new(window_size);
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
                    // Handle input
                    match game.handle_input() {
                        SinglePlayerResult::ReturnToMenu => {
                            log::info!("Returning to main menu");
                            game_state = GameState::MainMenu;
                            main_menu.reset();
                        }
                        SinglePlayerResult::Quit => {
                            log::info!("Quit requested from game");
                            break;
                        }
                        _ => {}
                    }

                    // Update game
                    game.update(delta_time);
                }
            }

            GameState::Paused => {
                // Paused state - don't update game, but still render
            }

            GameState::Quit => {
                break;
            }
        }

        // Render based on game state
        clear_background(BLACK);

        match game_state {
            GameState::MainMenu => {
                main_menu.draw();
            }

            GameState::SavesMenu => {
                saves_menu.draw();
            }

            GameState::Playing | GameState::Paused => {
                if let Some(ref game) = single_player_game {
                    game.render();
                }
            }

            GameState::Quit => {}
        }

        // Log FPS every second
        if fps_timer >= 1.0 {
            let fps = get_fps();
            log::debug!("FPS: {} | State: {:?}", fps, game_state);
            fps_timer = 0.0;
        }

        // Wait for next frame
        next_frame().await;
    }

    log::info!("Game exited cleanly");
    log::info!("Total frames: {}", frame_count);
    log::info!("Total playtime: {:.1} seconds", frame_count as f32 / 60.0);
}
