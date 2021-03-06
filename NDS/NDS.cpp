#include <Core/Clock.h>
#include <Core/CpuARM7TDMI.h>
#include <Core/CpuARM946ES.h>
#include <Core/MemoryBus.h>
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
            mAccessorMainRAM.setName("Main RAM").setMemoryWritable(mMainRAM.data());

            EMU_VERIFY(mClock.create());

            EMU_VERIFY(mArm7Memory.create(28, 23));
            EMU_VERIFY(mArm7Memory.addRange(0x02000000, MAIN_RAM_SIZE, mAccessorMainRAM));

            EMU_VERIFY(mArm9Memory.create(28, 22));
            EMU_VERIFY(mArm9Memory.addRange(0x02000000, MAIN_RAM_SIZE, mAccessorMainRAM));

            EMU_VERIFY(mArm7Cpu.create(CpuArm::Config(), mArm7Memory, mClock, TICKS_PER_FRAME / ARM7_TICKS_PER_FRAME));
            EMU_VERIFY(mArm9Cpu.create(CpuArm::Config(), mArm9Memory, mClock, TICKS_PER_FRAME / ARM7_TICKS_PER_FRAME));

            // Load startup code into RAM
            mArm7Memory.write(header.ARM7RAMAddress, reinterpret_cast<const uint32_t*>(mROM.getContent().data() + header.ARM7ROMOffset), divideUp(header.ARM7Size, 4));
            mArm9Memory.write(header.ARM9RAMAddress, reinterpret_cast<const uint32_t*>(mROM.getContent().data() + header.ARM9ROMOffset), divideUp(header.ARM9Size, 4));

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
        Rom                     mROM;
        std::vector<uint8_t>    mMainRAM;
        Clock                   mClock;
        MemoryBus32             mArm7Memory;
        MemoryBus32             mArm9Memory;
        MemoryBus32::Accessor   mAccessorMainRAM;
        ARM7TDMI                mArm7Cpu;
        ARM946ES                mArm9Cpu;
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
