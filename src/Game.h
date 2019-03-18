#pragma once
#include "Scene.h"
#include "Level.h"
#include "Overlays.h"
#include "Res.h"
#include "SelectRes.h"
#include "Scores.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>

enum GameMode {
  MAIN_MENU,
  PLAYING,
  PAUSED,
  SCREEN_SAVER,
  CONTROLS,
  LEVELS,
  CREDITS
};

static const float mouse_sensitivity = 0.005f;
static const float wheel_sensitivity = 0.2f;
static const float music_vol = 75.0f;

static sf::Vector2i mouse_pos;
static bool all_keys[sf::Keyboard::KeyCount] = { 0 };
static bool lock_mouse = false;
static GameMode game_mode = MAIN_MENU;

class Game{
public:
    int run();
    int init();
    void loop(const Resolution* resolution, sf::RenderWindow& window, sf::RenderTexture& renderTexture, const sf::Vector2i& screen_center, Scene& scene, Overlays& overlays, const sf::Glsl::Vec2& window_res);

	 void paused_event_Escape(sf::RenderWindow&, Scene&, Overlay&);
	 void paused_event_MousePressedLeft_Continue(sf::RenderWindow&, Scene&, Overlays&);
	 void paused_event_MousePressedLeft_Quit(sf::RenderWindow&, Scene&, Overlays&);
	 void paused_event_MousePressedLeft_Restart(sf::RenderWindow&, Scene&, Overlays&);
	 void paused_event_MousePressedLeft_Music(sf::RenderWindow&, Scene&, Overlays&);
    void main_menu_event_Play(sf::RenderWindow&, Scene &scene);
	 const Resolution* get_resolution();
		
	
	

protected:
    void handleEvent(sf::Event& event, sf::RenderWindow& window, Scene& scene, Overlays& overlays);
    void main_menu_event(sf::Event& event, sf::RenderWindow& window, Scene& scene, Overlays& overlays);
    void playing_event(sf::Event& event, sf::RenderWindow& window, Scene& scene);
    void paused_event(sf::Event& event, sf::RenderWindow& window, Scene& scene, Overlays& overlays);
    void screen_saver_event(sf::Event& event, Scene& scene);
    void controls_event(sf::Event& event, Scene& scene, Overlays& overlays);
    void levels_event(sf::Event& event, sf::RenderWindow& window, Scene& scene, Overlays& overlays);
    void credits_event(sf::Event& event, sf::RenderWindow& window, Scene& scene);
    void gameUpdate(Scene& scene, Overlays& overlays, const sf::Vector2i& screen_center, sf::RenderWindow& window);
    void updateOverlays(Overlays& overlays, sf::RenderWindow& window, Scene& scene);
    sf::Shader shader;
    sf::Font font;
    sf::Font font_mono;
    sf::Music menu_music;
    sf::Music level1_music;
    sf::Music level2_music;
    sf::Music credits_music;
    bool fullscreen;
    sf::ContextSettings settings;
    sf::VideoMode screen_size;
    sf::Uint32 window_style;
    sf::Clock clock;
    float smooth_fps;
    int lag_ms;
    float mouse_wheel;


};
