#include "CpuArm.h"

namespace emu
{
    CpuArm::Config::Config()
        : family(Family::Unknown)
    {
    }

    CpuArm::CpuArm()
    {
    }

    CpuArm::~CpuArm()
    {
    }

    bool CpuArm::create(const Config& config, MemoryBus& memory, Clock& clock, uint32_t clockDivider)
    {
        mConfig = config;
        EMU_VERIFY(config.family != Family::Unknown);

        mMemory = &memory;

        mClock = &clock;
        mClockDivider = clockDivider;
        mExecutedTick = 0;
        return true;
    }

    int32_t CpuArm::execute(int32_t tick)
    {
        mExecutedTick = tick;
        return tick;
    }

    void CpuArm::advance(int32_t tick)
    {
        mExecutedTick -= tick;
    }

    uint32_t CpuArm::disassemble(char* buffer, size_t size, uint32_t addr)
    {
        char temp[64];
        char* text = temp;
        text += sprintf(text, "???");

        if (size--)
        {
            strncpy(buffer, text, size);
            buffer[size] = 0;
        }

        return addr;
    }
}