#include "Game.h"

#include <iostream>

int main()
{
    Game game;

    if (!game.initialize())
    {
        std::cerr << "ALTERDUNE could not start correctly.\n";
        return 1;
    }

    game.run();
    return 0;
}
