#include "Clock.h"
#include "CpuArm.h"
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

            EMU_VERIFY(mClock.create());

            EMU_VERIFY(mArm7Memory.create(28, 23));
            EMU_VERIFY(mArm7Memory.addRange(0x02000000, MAIN_RAM_SIZE, mAccessorMainRAM.setName("Main RAM").setMemory(mMainRAM.data())));

            EMU_VERIFY(mArm9Memory.create(28, 22));
            EMU_VERIFY(mArm9Memory.addRange(0x02000000, MAIN_RAM_SIZE, mAccessorMainRAM));

            CpuArm::Config configCpuArm7;
            configCpuArm7.family = CpuArm::Family::ARMv4;
            EMU_VERIFY(mArm7Cpu.create(configCpuArm7, mArm7Memory, mClock, TICKS_PER_FRAME / ARM7_TICKS_PER_FRAME));

            CpuArm::Config configCpuArm9;
            configCpuArm9.family = CpuArm::Family::ARMv5;
            EMU_VERIFY(mArm9Cpu.create(configCpuArm9, mArm9Memory, mClock, TICKS_PER_FRAME / ARM7_TICKS_PER_FRAME));

            // Load startup code into RAM
            mArm7Memory.import32(header.ARM7RAMAddress, mROM.getContent().data() + header.ARM7ROMOffset, divideUp(header.ARM7Size, 4));
            mArm9Memory.import32(header.ARM9RAMAddress, mROM.getContent().data() + header.ARM9ROMOffset, divideUp(header.ARM9Size, 4));

            reset();

            return true;
        }

        void reset()
        {
            const auto& header = mROM.getHeader();
            mArm7Cpu.reset();
            mArm7Cpu.setPC(header.ARM7EntryAddress);
            mArm9Cpu.reset();
            mArm9Cpu.setPC(header.ARM9EntryAddress);
        }

        void executeFrame()
        {
            mClock.execute(TICKS_PER_FRAME);
            mClock.advance(TICKS_PER_FRAME);
        }

    private:
        Rom                             mROM;
        std::vector<uint8_t>            mMainRAM;
        Clock                           mClock;
        MemoryBus                       mArm7Memory;
        MemoryBus                       mArm9Memory;
        MemoryBus::ReadWriteAccessor    mAccessorMainRAM;
        CpuArm                          mArm7Cpu;
        CpuArm                          mArm9Cpu;
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
