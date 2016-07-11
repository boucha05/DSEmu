#include "Application.h"
#include "NDS.h"
#include "Rom.h"

Application::Config::Config()
{
}

Application::Application()
{
}

Application::~Application()
{
}

void Application::initialize()
{
    mSystem = nullptr;
    mContext = nullptr;
}

bool Application::create(const Config& config)
{
    auto path = config.romFolder;
    if (!path.empty() && (path.back() != EMU_DIR_SEPARATOR))
    {
        path += EMU_DIR_SEPARATOR;
    }
    path.append(config.rom);

    mSystem = &nds::getSystem();
    mContext = mSystem->loadGame(path.c_str());

    return true;
}

void Application::destroy()
{
    if (mContext)
        mSystem->unloadGame(*mContext);
}

bool Application::run(const Config& config)
{
    bool success = create(config);
    if (success)
    {
        mContext->executeFrame();
        printf("Success\n");
    }
    destroy();
    return success;
}
