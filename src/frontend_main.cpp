#include "FrontendApp.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    bool endingPreviewMode = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--ending-preview")
        {
            endingPreviewMode = true;
        }
    }

    FrontendApp app(endingPreviewMode);
    if (!app.initialize())
    {
        std::cerr << "ALTERDUNE frontend preview could not start.\n";
        return 1;
    }

    app.run();
    return 0;
}
