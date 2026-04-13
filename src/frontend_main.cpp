#include "FrontendApp.h"

#include <iostream>

int main()
{
    FrontendApp app;
    if (!app.initialize())
    {
        std::cerr << "ALTERDUNE frontend preview could not start.\n";
        return 1;
    }

    app.run();
    return 0;
}
