#include "NDS.h"
#include "Rom.h"
#include <memory>

namespace
{
    using namespace nds;

    class Context : public emu::IContext
    {
    public:
        bool create(const char* path)
        {
            EMU_VERIFY(mRom.create(path));
            return true;
        }

        void reset()
        {
        }

        void executeFrame()
        {
        }

    private:
        Rom         mRom;
    };

    class System : public emu::ISystem
    {
    public:
        virtual emu::IContext* loadGame(const char* path) override
        {
            auto instance = std::make_unique<Context>();
            if (!instance->create(path))
                return nullptr;
            return instance.release();
        }

        virtual void unloadGame(emu::IContext& instance) override
        {
            delete static_cast<Context*>(&instance);
        }
    };
}

namespace nds
{
    emu::ISystem& getSystem()
    {
        static ::System instance;
        return instance;
    }
}
