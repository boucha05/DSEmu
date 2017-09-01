#pragma once

#include "Clock.h"
#include "Core.h"
#include "MemoryBus.h"

namespace emu
{
    class CpuArm : public Clock::IClocked
    {
    public:
        struct Config
        {
        };

        struct Registers
        {
            uint32_t        r[16];
            uint32_t        cpsr;
            uint32_t        spsr;
            struct
            {
                uint32_t    r[16];
                uint32_t    cpsr;
            }               user;
            struct
            {
                uint32_t    r8;
                uint32_t    r9;
                uint32_t    r10;
                uint32_t    r11;
                uint32_t    r12;
                uint32_t    r13;
                uint32_t    r14;
                uint32_t    spsr;
            }               fiq;
            struct
            {
                uint32_t    r13;
                uint32_t    r14;
                uint32_t    spsr;
            }               svc;
            struct
            {
                uint32_t    r13;
                uint32_t    r14;
                uint32_t    spsr;
            }               abt;
            struct
            {
                uint32_t    r13;
                uint32_t    r14;
                uint32_t    spsr;
            }               irq;
            struct
            {
                uint32_t    r13;
                uint32_t    r14;
                uint32_t    spsr;
            }               und;
            uint32_t        flag_v;
            uint32_t        flag_c;
            uint32_t        flag_z;
            uint32_t        flag_n;
        };

        CpuArm();
        ~CpuArm();
        bool create(const Config& config, MemoryBus32& memory, Clock& clock, uint32_t clockDivider);
        virtual int32_t execute(int32_t tick) override;
        virtual void advance(int32_t tick) override;
        void reset();
        void setPC(uint32_t addr);
        uint8_t read8(uint32_t addr);
        uint16_t read16(uint32_t addr);
        uint32_t read32(uint32_t addr);
        void write8(uint32_t addr, uint8_t value);
        void write16(uint32_t addr, uint16_t value);
        void write32(uint32_t addr, uint32_t value);
        virtual uint32_t disassemble(char* buffer, size_t size, uint32_t addr, bool thumb) = 0;
        virtual uint32_t execute() = 0;

        const Registers& getRegisters() const
        {
            return mRegisters;
        }

        MemoryBus32& getMemoryBus()
        {
            return *mMemory;
        }

    protected:
        static const uint32_t   MODE_USR = 0x10;
        static const uint32_t   MODE_FIQ = 0x11;
        static const uint32_t   MODE_IRQ = 0x12;
        static const uint32_t   MODE_SVC = 0x13;
        static const uint32_t   MODE_ABT = 0x17;
        static const uint32_t   MODE_UND = 0x1b;
        static const uint32_t   MODE_SYS = 0x1f;

        static const uint32_t   CPSR_N = 31;
        static const uint32_t   CPSR_Z = 30;
        static const uint32_t   CPSR_C = 29;
        static const uint32_t   CPSR_V = 28;
        static const uint32_t   CPSR_Q = 27;
        static const uint32_t   CPSR_I = 7;
        static const uint32_t   CPSR_F = 6;
        static const uint32_t   CPSR_T = 5;
        static const uint32_t   CPSR_MODE_SHIFT = 0;
        static const uint32_t   CPSR_MODE_SIZE = 5;

        void flagsExport();
        void flagsImport();
        void regExport(uint32_t mode);
        void regImport(uint32_t mode);
        void prefetch32();
        void trace();

        Config          mConfig;
        MemoryBus32*    mMemory;
        Clock*          mClock;
        uint32_t        mClockDivider;
        uint32_t        mExecutedTick;
        Registers       mRegisters;
        uint32_t        mPC;
        uint32_t        mPCNext;
        uint32_t        mOpcode = 0;
    };
}