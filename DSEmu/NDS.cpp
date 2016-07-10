#include "MemoryBus.h"
#include "Rom.h"
#include <memory>

namespace
{
    using namespace emu;
    using namespace nds;

    class Context : public IContext
    {
    public:
        bool create(const char* path)
        {
            EMU_VERIFY(mROM.create(path));
            const auto& header = mROM.getHeader();

            mMainRAM.resize(MAIN_RAM_SIZE);

            EMU_VERIFY(mArm7Memory.create(28, 23));
            EMU_VERIFY(mArm7Memory.addRange(0x02000000, MAIN_RAM_SIZE, mArm7AccessorMainRAM.setName("Main RAM").setMemory(mMainRAM.data())));

            EMU_VERIFY(mArm9Memory.create(28, 22));
            EMU_VERIFY(mArm9Memory.addRange(0x02000000, MAIN_RAM_SIZE, mArm9AccessorMainRAM.setName("Main RAM").setMemory(mMainRAM.data())));

            // Load startup code into RAM
            mArm7Memory.import32(header.ARM7RAMAddress, mROM.getContent().data() + header.ARM7ROMOffset, divideUp(header.ARM7Size, 4));
            mArm9Memory.import32(header.ARM9RAMAddress, mROM.getContent().data() + header.ARM9ROMOffset, divideUp(header.ARM9Size, 4));

            return true;
        }

        void reset()
        {
        }

        void executeFrame()
        {
        }

    private:
        Rom                             mROM;
        std::vector<uint8_t>            mMainRAM;
        MemoryBus                       mArm7Memory;
        MemoryBus::ReadWriteAccessor    mArm7AccessorMainRAM;
        MemoryBus                       mArm9Memory;
        MemoryBus::ReadWriteAccessor    mArm9AccessorMainRAM;
    };

    class System : public ISystem
    {
    public:
        virtual IContext* loadGame(const char* path) override
        {
            auto instance = std::make_unique<Context>();
            if (!instance->create(path))
                return nullptr;
            return instance.release();
        }

        virtual void unloadGame(IContext& instance) override
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
