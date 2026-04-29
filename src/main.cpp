#include "Game.h"

#include <iostream>
#include <string>

using namespace std;

int main(int argc, char* argv[])
{
    Game game;

    if (argc >= 3 && string(argv[1]) == "--test-ending")
    {
        if (!game.initializeForFrontend("EndingTest"))
        {
            cerr << "ALTERDUNE could not start the ending test.\n";
            return 1;
        }

        if (argc >= 4)
        {
            game.setLanguage(argv[3]);
        }

        return game.runEndingTest(argv[2]) ? 0 : 1;
    }

    if (argc >= 3 && string(argv[1]) == "--test-ending-2")
    {
        if (!game.initializeForFrontend("EndingTest"))
        {
            cerr << "ALTERDUNE could not start the 2-fight ending test.\n";
            return 1;
        }

        if (argc >= 4)
        {
            game.setLanguage(argv[3]);
        }

        game.enableEndingPreviewMode();
        return game.runEndingTest(argv[2]) ? 0 : 1;
    }

    bool endingPreviewMode = false;
    for (int i = 1; i < argc; ++i)
    {
        if (string(argv[i]) == "--ending-preview")
        {
            endingPreviewMode = true;
        }
    }

    if (!game.initialize())
    {
        cerr << "ALTERDUNE could not start correctly.\n";
        return 1;
    }

    if (endingPreviewMode)
    {
        game.enableEndingPreviewMode();
        cout << "Ending preview mode enabled: the game ends after 2 victories.\n";
    }

    game.run();
    return 0;
}
