#include "Game.h"

int main(){
    Game* g = new Game();
    g->init();
    g->run();

    delete g;

    return 0;
}
