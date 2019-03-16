#include "Game.h"

int main(){
    Game* g = new Game();
    g->run();

    delete g;

    return 0;
}
