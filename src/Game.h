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

class Game{
public:
    int run();
private:

};
