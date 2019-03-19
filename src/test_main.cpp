#include "Game.h"
#include <unistd.h>
#include <string>

std::string print_result(bool result) {
    if (result)
        return "PASSED";
    else
        return "FAILED";
}

void test_play(Game* g){
    std::ofstream testsFile;
    testsFile.open("test_output.txt");

    testsFile << "Game Class Tests:\n";
    
    // Play
    usleep(1000*1000);
    g->main_menu_event_Play(*(g->gameWindow), *(g->gameScene));
    testsFile << "Test 1, asserting that g->get_GameMode() == PLAYING: "
              << print_result(g->get_GameMode() == PLAYING) << "\n";
    
    // Pause
    usleep(1000*1000);
    g->pause(*(g->gameWindow), *(g->gameScene));
    testsFile << "Test 2, asserting that g->get_GameMode() == PAUSED: "
              << print_result(g->get_GameMode() == PAUSED) << "\n";
   
    // Escape from paused
    usleep(1000*1000);
    g->paused_event_Escape(*(g->gameWindow), *(g->gameScene), *(g->gameOverlays));
    testsFile << "Test 3, asserting that g->get_GameMode() == PLAYING: "
              << print_result(g->get_GameMode() == PLAYING) << "\n";

    // Continue from paused 
    usleep(1000*1000);
    g->pause(*(g->gameWindow), *(g->gameScene)); // Pause first
    usleep(1000*1000);
    g->paused_event_MousePressedLeft_Continue(*(g->gameWindow), *(g->gameScene), *(g->gameOverlays));
    testsFile << "Test 4, asserting that g->get_GameMode() == PLAYING: "
              << print_result(g->get_GameMode() == PLAYING) << "\n";

    // Restart from paused 
    usleep(1000*1000);
    g->pause(*(g->gameWindow), *(g->gameScene)); // Pause first
    usleep(1000*1000);
    g->paused_event_MousePressedLeft_Restart(*(g->gameWindow), *(g->gameScene), *(g->gameOverlays));
    testsFile << "Test 5, asserting that g->get_GameMode() == PLAYING: "
              << print_result(g->get_GameMode() == PLAYING) << "\n";

    // Quit from paused 
    usleep(1000*1000);
    g->pause(*(g->gameWindow), *(g->gameScene)); // Pause first
    usleep(1000*1000);
    g->paused_event_MousePressedLeft_Quit(*(g->gameWindow), *(g->gameScene), *(g->gameOverlays));
    testsFile << "Test 6, asserting that g->get_GameMode() == LEVELS or g->get_GameMode() == MAIN_MENU: "
              << print_result(g->get_GameMode() == LEVELS || g->get_GameMode() == MAIN_MENU) << "\n";

    testsFile.close();
}

int main(){
    Game* g = new Game();
    g->init();
    const Resolution resO = Resolution(640, 360, "GTX 960 or lower:");
    const Resolution* res = &resO;
    //const Resolution* res = g->get_resolution();
    std::thread first(test_play, g); //move mouse and click on 960
    g->run(res);
    first.join();
    delete g;

    return 0;
}
