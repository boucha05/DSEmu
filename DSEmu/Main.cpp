#include "Application.h"

int main()
{
    Application::Config config;
    config.romFolder = "C:\\emu\\NintendoDS\\roms";
    config.rom = "0037 - Super Mario 64 DS (U)(Trashman).nds";

    Application application;
    auto success = application.run(config);
    return success ? 0 : 1;
}
