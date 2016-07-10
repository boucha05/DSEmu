#pragma once

#include "Core.h"
#include <string>

class Application
{
public:
    struct Config
    {
        Config();

        std::string     romFolder;
        std::string     rom;
    };

    Application();
    ~Application();
    bool run(const Config& config);

private:
    void initialize();
    bool create(const Config& config);
    void destroy();

    emu::ISystem*   mSystem;
    emu::IContext*  mContext;
};
