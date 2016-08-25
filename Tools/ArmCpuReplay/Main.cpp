#include <Contrib/CpuTrace/src/CpuTraceArm.h>
#include <Core/CpuArm.h>

namespace
{
    class Application
    {
    public:
        struct Config
        {
            Config()
            {
            }

            std::string     path;
        };

        Application()
            : mCpuTraceContext(CpuTrace::createContext())
        {
        }

        ~Application()
        {
            destroyContext(mCpuTraceContext);
        }

        bool run(const Config& config)
        {
            mConfig = config;
            return true;
        }

    private:
        CpuTrace::IContext&     mCpuTraceContext;
        Config                  mConfig;
    };
}

int main()
{
    Application::Config config;
    config.path = "D:\\CaptureARM9.trace";

    Application application;
    bool success = application.run(config);
    return success ? 0 : 1;
}