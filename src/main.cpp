#include "Game.h"

#include <iostream>

using namespace std;

int main()
{
    Game game;

    if (!game.initialize())
    {
        cerr << "ALTERDUNE could not start correctly.\n";
        return 1;
    }

    game.run();
    return 0;
}
