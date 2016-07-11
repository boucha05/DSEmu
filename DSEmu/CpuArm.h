#pragma once

#include "Clock.h"
#include "Core.h"
#include "MemoryBus.h"

namespace emu
{
    class CpuArm : public Clock::IClocked
    {
    public:
        enum class Family
        {
            Unknown,
            ARMv4,
            ARMv5,
        };

        struct Config
        {
            Family  family;

            Config();
        };

        CpuArm();
        ~CpuArm();
        bool create(const Config& config, MemoryBus& memory, Clock& clock, uint32_t clockDivider);
        virtual int32_t execute(int32_t tick) override;
        virtual void advance(int32_t tick) override;
        uint32_t disassemble(char* buffer, size_t size, uint32_t addr);

    private:
        Config          mConfig;
        MemoryBus*      mMemory;
        Clock*          mClock;
        uint32_t        mClockDivider;
        uint32_t        mExecutedTick;
    };
}