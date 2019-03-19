#include "Game.h"


#ifdef _WIN32
#include <Windows.h>
#define ERROR_MSG(x) MessageBox(nullptr, TEXT(x), TEXT("ERROR"), MB_OK);
#else
#define ERROR_MSG(x) std::cerr << x << std::endl;
#endif

GameMode Game::get_GameMode() {
    return game_mode;
}

float GetVol() {
  if (!music_on) {
    return 0.0f;
  } else if (game_mode == PAUSED) {
    return music_vol / 4;
  } else {
    return music_vol;
  }
}

void LockMouse(sf::RenderWindow& window) {
  window.setMouseCursorVisible(false);
  const sf::Vector2u size = window.getSize();
  mouse_pos = sf::Vector2i(size.x / 2, size.y / 2);
  sf::Mouse::setPosition(mouse_pos);
}
void UnlockMouse(sf::RenderWindow& window) {
  window.setMouseCursorVisible(true);
}
int dirExists(const char *path) {
  struct stat info;
  if (stat(path, &info) != 0) {
    return 0;
  } else if (info.st_mode & S_IFDIR) {
    return 1;
  }
  return 0;
}

int Game::init(){
    //Make sure shader is supported
    if (!sf::Shader::isAvailable()) {
      ERROR_MSG("Graphics card does not support shaders");
      return 1;
    }
    //Load the vertex shader
    if (!shader.loadFromFile(vert_glsl, sf::Shader::Vertex)) {
      ERROR_MSG("Failed to compile vertex shader");
      return 1;
    }
    //Load the fragment shader
    if (!shader.loadFromFile(frag_glsl, sf::Shader::Fragment)) {
      ERROR_MSG("Failed to compile fragment shader");
      return 1;
    }

    //Load the font
    if (!font.loadFromFile(Orbitron_Bold_ttf)) {
      ERROR_MSG("Unable to load font");
      return 1;
    }
    //Load the mono font
    if (!font_mono.loadFromFile(Inconsolata_Bold_ttf)) {
      ERROR_MSG("Unable to load mono font");
      return 1;
    }

    //Load the music
    menu_music.openFromFile(menu_ogg);
    menu_music.setLoop(true);
    level1_music.openFromFile(level1_ogg);
    level1_music.setLoop(true);
    level2_music.openFromFile(level2_ogg);
    level2_music.setLoop(true);
    credits_music.openFromFile(credits_ogg);
    credits_music.setLoop(true);

    //Get the directory for saving and loading high scores
  #ifdef _WIN32
    const std::string save_dir = std::string(std::getenv("APPDATA")) + "\\MarbleMarcher";
  #else
    const std::string save_dir = std::string(std::getenv("HOME")) + "/.MarbleMarcher";
  #endif

    if (!dirExists(save_dir.c_str())) {
  #if defined(_WIN32)
      bool success = CreateDirectory(save_dir.c_str(), NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
  #else
      bool success = mkdir(save_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
  #endif
      if (!success) {
        ERROR_MSG("Failed to create save directory");
        return 1;
      }
    }
    const std::string save_file = save_dir + "/scores.bin";

    //Load scores if available
    high_scores.Load(save_file);

    return 0;
}

const Resolution* Game::get_resolution(){
    SelectRes select_res(&font_mono);
    const Resolution* resolution = select_res.Run();
    fullscreen = select_res.FullScreen();
	 return resolution;
}

int Game::run(const Resolution* resolution){
    //Have user select the resolution
	 if (resolution == nullptr) {
      return 0;
    }

    //GL settings
    settings.majorVersion = 2;
    settings.minorVersion = 0;

    //Create the window
    const sf::Vector2i screen_center(resolution->width / 2, resolution->height / 2);
    if (fullscreen) {
      screen_size = sf::VideoMode::getDesktopMode();
      window_style = sf::Style::Fullscreen;
    } else {
      screen_size = sf::VideoMode(resolution->width, resolution->height, 24);
      window_style = sf::Style::Close;
    }
    sf::RenderWindow window(screen_size, "Marble Marcher", window_style, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);
    window.requestFocus();

    //If native resolution is the same, then we don't need a render texture
    if (resolution->width == screen_size.width && resolution->height == screen_size.height) {
      fullscreen = false;
    }

    //Create the render texture if needed
    sf::RenderTexture renderTexture;
    if (fullscreen) {
      renderTexture.create(resolution->width, resolution->height, settings);
      renderTexture.setSmooth(true);
      renderTexture.setActive(true);
      window.setActive(false);
    }

    gameWindow = &window;

    //Create the fractal scene
    Scene scene(&level1_music, &level2_music);
    const sf::Glsl::Vec2 window_res((float)resolution->width, (float)resolution->height);
    shader.setUniform("iResolution", window_res);
    scene.Write(shader);

    gameScene = &scene;

    //Create the menus
    Overlays overlays(&font, &font_mono);
    overlays.SetScale(float(screen_size.width) / 1280.0f);
    menu_music.setVolume(GetVol());
    menu_music.play();

    gameOverlays = &overlays;

    //Main loop
    smooth_fps = 60.0f;
    lag_ms = 0;
    loop(resolution, window, renderTexture, screen_center, scene, overlays, window_res);

    //Stop all music
    menu_music.stop();
    level1_music.stop();
    level2_music.stop();
    credits_music.stop();

    //Get the directory for saving and loading high scores
  #ifdef _WIN32
    const std::string save_dir = std::string(std::getenv("APPDATA")) + "\\MarbleMarcher";
  #else
    const std::string save_dir = std::string(std::getenv("HOME")) + "/.MarbleMarcher";
  #endif

    const std::string save_file = save_dir + "/scores.bin";
    high_scores.Save(save_file);

  #ifdef _DEBUG
    system("pause");
  #endif
    return 0;
}

void Game::main_menu_event_Play(sf::RenderWindow& window, Scene &scene){
				game_mode = PLAYING;
            menu_music.stop();
            scene.StartNewGame();
            scene.GetCurMusic().setVolume(GetVol());
            scene.GetCurMusic().play();
            LockMouse(window);
}
 void Game::main_menu_event_Level(Scene &scene){
				game_mode = LEVELS;
            scene.SetExposure(0.5f);
 }

void Game::main_menu_event(sf::Event& event, sf::RenderWindow& window, Scene& scene, Overlays& overlays){
    if (event.type == sf::Event::KeyPressed) {
      const sf::Keyboard::Key keycode = event.key.code;
      if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
      if (keycode == sf::Keyboard::Escape) {
          window.close();
          return;
      }
      all_keys[keycode] = true;
    } else if (event.type == sf::Event::KeyReleased) {
        const sf::Keyboard::Key keycode = event.key.code;
        if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
        all_keys[keycode] = false;
    } else if (event.type == sf::Event::MouseButtonPressed) {
      if (event.mouseButton.button == sf::Mouse::Left) {
        mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
          const Overlays::Texts selected = overlays.GetOption(Overlays::PLAY, Overlays::EXIT);
          if (selected == Overlays::PLAY) {
				main_menu_event_Play(window, scene);
			 } else if (selected == Overlays::CONTROLS) {
            game_mode = CONTROLS;
          } else if (selected == Overlays::LEVELS) {
				main_menu_event_Level(scene);
          } else if (selected == Overlays::SCREEN_SAVER) {
            game_mode = SCREEN_SAVER;
            scene.SetMode(Scene::SCREEN_SAVER);
          } else if (selected == Overlays::EXIT) {
            window.close();
            return;
          }

      }
    }else if (event.type == sf::Event::MouseButtonReleased) {
      mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
    } else if (event.type == sf::Event::MouseMoved) {
      mouse_pos = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
    } else if (event.type == sf::Event::MouseWheelScrolled) {
      mouse_wheel += event.mouseWheelScroll.delta;
    }
}

// USE THIS FUNCTION TO PAUSE
void Game::pause(sf::RenderWindow &window, Scene &scene){
          game_mode = PAUSED;
          scene.GetCurMusic().setVolume(GetVol());
          UnlockMouse(window);
          scene.SetExposure(0.5f);
}

void Game::playing_event(sf::Event& event, sf::RenderWindow& window, Scene& scene){
    if (event.type == sf::Event::KeyPressed) {
      const sf::Keyboard::Key keycode = event.key.code;
      if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
      if (keycode == sf::Keyboard::Escape) {
			pause(window, scene);

     }else if (keycode == sf::Keyboard::R) {
          scene.ResetLevel();
      }
      all_keys[keycode] = true;
    }else if (event.type == sf::Event::KeyReleased) {
        const sf::Keyboard::Key keycode = event.key.code;
        if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
        all_keys[keycode] = false;
    }else if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Right) {
            scene.ResetLevel();
        }
    }else if (event.type == sf::Event::MouseButtonReleased) {
      mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
    } else if (event.type == sf::Event::MouseMoved) {
      mouse_pos = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
    } else if (event.type == sf::Event::MouseWheelScrolled) {
      mouse_wheel += event.mouseWheelScroll.delta;
    }
}

// TEST THIS FUNC
void Game::paused_event_Escape(sf::RenderWindow& window, Scene& scene, Overlays& overlays){
          game_mode = PLAYING;
          scene.GetCurMusic().setVolume(GetVol());
          scene.SetExposure(1.0f);
          LockMouse(window);
}

// TEST THIS FUNC
void Game::paused_event_MousePressedLeft_Continue(sf::RenderWindow& window, Scene& scene, Overlays& overlays){
	         game_mode = PLAYING;
            scene.GetCurMusic().setVolume(GetVol());
            scene.SetExposure(1.0f);
            LockMouse(window);
}
// Test this func
void Game::paused_event_MousePressedLeft_Restart(sf::RenderWindow& window, Scene& scene, Overlays &overlays){
            game_mode = PLAYING;
            scene.ResetLevel();
            scene.GetCurMusic().setVolume(GetVol());
            scene.SetExposure(1.0f);
            LockMouse(window);
} 

// Test this func
void Game::paused_event_MousePressedLeft_Quit(sf::RenderWindow& window, Scene& scene, Overlays &overlays){
				if (scene.IsSinglePlay()) {
              game_mode = LEVELS;
            } else {
              game_mode = MAIN_MENU;
              scene.SetExposure(1.0f);
            }
            scene.SetMode(Scene::INTRO);
            scene.StopAllMusic();
            menu_music.setVolume(GetVol());
            menu_music.play();
}


// Test this func
void Game::paused_event_MousePressedLeft_Music(sf::RenderWindow& window, Scene& scene, Overlays &overlays){
				music_on = !music_on;
            level1_music.setVolume(GetVol());
            level2_music.setVolume(GetVol());
}


void Game::paused_event(sf::Event& event, sf::RenderWindow& window, Scene& scene, Overlays& overlays){
    if (event.type == sf::Event::KeyPressed) {
      const sf::Keyboard::Key keycode = event.key.code;
      if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
      if (keycode == sf::Keyboard::Escape) {
			paused_event_Escape(window, scene, overlays);
     }
      all_keys[keycode] = true;
    }else if (event.type == sf::Event::KeyReleased) {
        const sf::Keyboard::Key keycode = event.key.code;
        if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
        all_keys[keycode] = false;
    }else if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
          mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
          const Overlays::Texts selected = overlays.GetOption(Overlays::CONTINUE, Overlays::MOUSE);
          if (selected == Overlays::CONTINUE) {
					paused_event_MousePressedLeft_Continue(window, scene, overlays);
		    } else if (selected == Overlays::RESTART) {
					paused_event_MousePressedLeft_Restart(window, scene, overlays);
         } else if (selected == Overlays::QUIT) {
					paused_event_MousePressedLeft_Quit(window, scene, overlays);
			}
			else if (selected == Overlays::MUSIC) {
					paused_event_MousePressedLeft_Music(window, scene, overlays);
			} else if (selected == Overlays::MOUSE) {
            mouse_setting = (mouse_setting + 1) % 3;
          }
		}

    }else if (event.type == sf::Event::MouseButtonReleased) {
      mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
    } else if (event.type == sf::Event::MouseMoved) {
      mouse_pos = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
    } else if (event.type == sf::Event::MouseWheelScrolled) {
      mouse_wheel += event.mouseWheelScroll.delta;
    }
}

void Game::screen_saver_event(sf::Event& event, Scene& scene){
    if (event.type == sf::Event::KeyPressed) {
      const sf::Keyboard::Key keycode = event.key.code;
      if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
      if (keycode == sf::Keyboard::Escape) {
          game_mode = MAIN_MENU;
          scene.SetMode(Scene::INTRO);
      }
      all_keys[keycode] = true;
    }else if (event.type == sf::Event::KeyReleased) {
        const sf::Keyboard::Key keycode = event.key.code;
        if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
        all_keys[keycode] = false;
    }else if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
          mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
          scene.SetMode(Scene::INTRO);
          game_mode = MAIN_MENU;
        }
    }else if (event.type == sf::Event::MouseButtonReleased) {
      mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
    } else if (event.type == sf::Event::MouseMoved) {
      mouse_pos = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
    } else if (event.type == sf::Event::MouseWheelScrolled) {
      mouse_wheel += event.mouseWheelScroll.delta;
    }
}

void Game::controls_event(sf::Event& event, Scene& scene, Overlays& overlays){
    if (event.type == sf::Event::KeyPressed) {
      const sf::Keyboard::Key keycode = event.key.code;
      if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
      if (keycode == sf::Keyboard::Escape) {
          game_mode = MAIN_MENU;
          scene.SetExposure(1.0f);
      }
      all_keys[keycode] = true;
    }else if (event.type == sf::Event::KeyReleased) {
        const sf::Keyboard::Key keycode = event.key.code;
        if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
        all_keys[keycode] = false;
    }else if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
          mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
          const Overlays::Texts selected = overlays.GetOption(Overlays::BACK, Overlays::BACK);
          if (selected == Overlays::BACK) {
            game_mode = MAIN_MENU;
          }
        }
    }else if (event.type == sf::Event::MouseButtonReleased) {
      mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
    } else if (event.type == sf::Event::MouseMoved) {
      mouse_pos = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
    } else if (event.type == sf::Event::MouseWheelScrolled) {
      mouse_wheel += event.mouseWheelScroll.delta;
    }
}

void Game::choose_level(sf::RenderWindow &window, Scene &scene, const Overlays::Texts& selected){
			if (selected == Overlays::BACK2) {
            game_mode = MAIN_MENU;
            scene.SetExposure(1.0f);
          } else if (selected >= Overlays::L0 && selected <= Overlays::L14) {
            if (high_scores.HasUnlocked(selected - Overlays::L0)) {
              game_mode = PLAYING;
              menu_music.stop();
              scene.SetExposure(1.0f);
              scene.StartSingle(selected - Overlays::L0);
              scene.GetCurMusic().setVolume(GetVol());
              scene.GetCurMusic().play();
              LockMouse(window);
            }
          }
}

void Game::levels_event(sf::Event& event, sf::RenderWindow& window, Scene& scene, Overlays& overlays){
    if (event.type == sf::Event::KeyPressed) {
      const sf::Keyboard::Key keycode = event.key.code;
      if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
      if (keycode == sf::Keyboard::Escape) {
          game_mode = MAIN_MENU;
          scene.SetExposure(1.0f);
      }
      all_keys[keycode] = true;
    }else if (event.type == sf::Event::KeyReleased) {
        const sf::Keyboard::Key keycode = event.key.code;
        if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
        all_keys[keycode] = false;
    }else if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
          mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
          const Overlays::Texts selected = overlays.GetOption(Overlays::L0, Overlays::BACK2);
			 choose_level(window, scene, selected); 
        }
    }else if (event.type == sf::Event::MouseButtonReleased) {
      mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
    } else if (event.type == sf::Event::MouseMoved) {
      mouse_pos = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
    } else if (event.type == sf::Event::MouseWheelScrolled) {
      mouse_wheel += event.mouseWheelScroll.delta;
    }
}

void Game::credits_event(sf::Event& event, sf::RenderWindow& window, Scene& scene){
    if (event.type == sf::Event::KeyPressed) {
      const sf::Keyboard::Key keycode = event.key.code;
      if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
      game_mode = MAIN_MENU;
      UnlockMouse(window);
      scene.SetMode(Scene::INTRO);
      scene.SetExposure(1.0f);
      credits_music.stop();
      menu_music.setVolume(GetVol());
      menu_music.play();
      all_keys[keycode] = true;
    }else if (event.type == sf::Event::KeyReleased) {
        const sf::Keyboard::Key keycode = event.key.code;
        if (event.key.code < 0 || event.key.code >= sf::Keyboard::KeyCount) { return; }
        all_keys[keycode] = false;
    } else if (event.type == sf::Event::MouseButtonReleased) {
      mouse_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
    } else if (event.type == sf::Event::MouseMoved) {
      mouse_pos = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
    } else if (event.type == sf::Event::MouseWheelScrolled) {
      mouse_wheel += event.mouseWheelScroll.delta;
    }
}

void Game::handleEvent(sf::Event& event, sf::RenderWindow& window, Scene& scene, Overlays& overlays){
    if(game_mode == MAIN_MENU){
        main_menu_event(event, window, scene, overlays);
    }
    else if(game_mode == PLAYING){
        playing_event(event, window, scene);
    }
    else if(game_mode == PAUSED){
        paused_event(event, window, scene, overlays);
    }
    else if(game_mode == SCREEN_SAVER){
        screen_saver_event(event, scene);
    }
    else if(game_mode == CONTROLS){
        controls_event(event, scene, overlays);
    }
    else if(game_mode == LEVELS){
        levels_event(event, window, scene, overlays);
    }
    else if(game_mode == CREDITS){
        credits_event(event, window, scene);
    }
}

void Game::gameUpdate(Scene& scene, Overlays& overlays, const sf::Vector2i& screen_center, sf::RenderWindow& window){
    if (game_mode == MAIN_MENU) {
      scene.UpdateCamera();
      overlays.UpdateMenu((float)mouse_pos.x, (float)mouse_pos.y);
    } else if (game_mode == CONTROLS) {
      scene.UpdateCamera();
      overlays.UpdateControls((float)mouse_pos.x, (float)mouse_pos.y);
    } else if (game_mode == LEVELS) {
      scene.UpdateCamera();
      overlays.UpdateLevels((float)mouse_pos.x, (float)mouse_pos.y);
    } else if (game_mode == SCREEN_SAVER) {
      scene.UpdateCamera();
    } else if (game_mode == PLAYING || game_mode == CREDITS) {
      //Collect keyboard input
      const float force_lr =
        (all_keys[sf::Keyboard::Left] || all_keys[sf::Keyboard::A] ? -1.0f : 0.0f) +
        (all_keys[sf::Keyboard::Right] || all_keys[sf::Keyboard::D] ? 1.0f : 0.0f);
      const float force_ud =
        (all_keys[sf::Keyboard::Down] || all_keys[sf::Keyboard::S] ? -1.0f : 0.0f) +
        (all_keys[sf::Keyboard::Up] || all_keys[sf::Keyboard::W] ? 1.0f : 0.0f);

      //Collect mouse input
      const sf::Vector2i mouse_delta = mouse_pos - screen_center;
      sf::Mouse::setPosition(screen_center, window);
      float ms = mouse_sensitivity;
      if (mouse_setting == 1) {
        ms *= 0.5f;
      } else if (mouse_setting == 2) {
        ms *= 0.25f;
      }
      const float cam_lr = float(-mouse_delta.x) * ms;
      const float cam_ud = float(-mouse_delta.y) * ms;
      const float cam_z = mouse_wheel * wheel_sensitivity;

      //Apply forces to marble and camera
      scene.UpdateMarble(force_lr, force_ud);
      scene.UpdateCamera(cam_lr, cam_ud, cam_z);
    } else if (game_mode == PAUSED) {
      overlays.UpdatePaused((float)mouse_pos.x, (float)mouse_pos.y);
    }
}

void Game::updateOverlays(Overlays& overlays, sf::RenderWindow& window, Scene& scene){
    if (game_mode == MAIN_MENU) {
      overlays.DrawMenu(window);
    } else if (game_mode == CONTROLS) {
      overlays.DrawControls(window);
    } else if (game_mode == LEVELS) {
      overlays.DrawLevels(window);
    } else if (game_mode == PLAYING) {
      if (scene.GetMode() == Scene::ORBIT && scene.GetMarble().x() < 998.0f) {
        overlays.DrawLevelDesc(window, scene.GetLevel());
      } else if (scene.GetMode() == Scene::MARBLE) {
        overlays.DrawArrow(window, scene.GetGoalDirection());
      }
      overlays.DrawTimer(window, scene.GetCountdownTime(), scene.IsHighScore());
    } else if (game_mode == PAUSED) {
      overlays.DrawPaused(window);
    } else if (game_mode == CREDITS) {
      overlays.DrawCredits(window);
    }
    overlays.DrawFPS(window, int(smooth_fps + 0.5f));
}

void Game::loop(const Resolution* resolution, sf::RenderWindow& window, sf::RenderTexture& renderTexture, const sf::Vector2i& screen_center, Scene& scene, Overlays& overlays, const sf::Glsl::Vec2& window_res){
    while (window.isOpen()) {
      sf::Event event;
      mouse_wheel = 0.0f;
      while (window.pollEvent(event)) {
          if (event.type == sf::Event::Closed) {
            window.close();
            break;
          }
          else{
              handleEvent(event, window, scene, overlays);
          }
      }
      //Check if the game was beat
      if (scene.GetMode() == Scene::FINAL && game_mode != CREDITS) {
        game_mode = CREDITS;
        scene.StopAllMusic();
        scene.SetExposure(0.5f);
        credits_music.play();
      }
      gameUpdate(scene, overlays, screen_center, window);

      bool skip_frame = false;
      if (lag_ms >= 16) {
        //If there is too much lag, just do another frame of physics and skip the draw
        lag_ms -= 16;
        skip_frame = true;
      } else {
        //Update the shader values
        scene.Write(shader);

        //Setup full-screen shader
        sf::RenderStates states = sf::RenderStates::Default;
        states.shader = &shader;
        sf::RectangleShape rect;
        rect.setSize(window_res);
        rect.setPosition(0, 0);

        //Draw the fractal
        if (fullscreen) {
          //Draw to the render texture
          renderTexture.draw(rect, states);
          renderTexture.display();

          //Draw render texture to main window
          sf::Sprite sprite(renderTexture.getTexture());
          sprite.setScale(float(screen_size.width) / float(resolution->width),
                          float(screen_size.height) / float(resolution->height));
          window.draw(sprite);
        } else {
          //Draw directly to the main window
          window.draw(rect, states);
        }
      }

      //Draw text overlays to the window
      updateOverlays(overlays, window, scene);

      if (!skip_frame) {
        //Finally display to the screen
        window.display();

        //If V-Sync is running higher than desired fps, slow down!
        const float s = clock.restart().asSeconds();
        if (s > 0.0f) {
          smooth_fps = smooth_fps*0.9f + std::min(1.0f / s, 60.0f)*0.1f;
        }
        const int time_diff_ms = int(16.66667f - s*1000.0f);
        if (time_diff_ms > 0) {
          sf::sleep(sf::milliseconds(time_diff_ms));
          lag_ms = std::max(lag_ms - time_diff_ms, 0);
        } else if (time_diff_ms < 0) {
          lag_ms += std::max(-time_diff_ms, 0);
        }
      }
    }
}
