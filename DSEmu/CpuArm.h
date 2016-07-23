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
        };

        CpuArm();
        ~CpuArm();
        bool create(const Config& config, MemoryBus& memory, Clock& clock, uint32_t clockDivider);
        virtual int32_t execute(int32_t tick) override;
        virtual void advance(int32_t tick) override;
        void reset();
        void setPC(uint32_t addr);
        uint32_t disassemble(char* buffer, size_t size, uint32_t addr, bool thumb);

        const Registers& getRegisters() const
        {
            return mRegisters;
        }

    protected:
        #include "CpuArmSymbols.inl"
        static const char** getInsnNameTable();
        static const char** getInsnSuffixTable();

    private:
        static const uint32_t   MODE_USR = 0x10;
        static const uint32_t   MODE_FIQ = 0x11;
        static const uint32_t   MODE_IRQ = 0x12;
        static const uint32_t   MODE_SVC = 0x13;
        static const uint32_t   MODE_ABT = 0x17;
        static const uint32_t   MODE_UND = 0x1b;
        static const uint32_t   MODE_SYS = 0x1f;

        static const uint32_t   CPSR_N = EMU_BIT(31);
        static const uint32_t   CPSR_Z = EMU_BIT(30);
        static const uint32_t   CPSR_C = EMU_BIT(29);
        static const uint32_t   CPSR_V = EMU_BIT(28);
        static const uint32_t   CPSR_Q = EMU_BIT(27);
        static const uint32_t   CPSR_I = EMU_BIT(7);
        static const uint32_t   CPSR_F = EMU_BIT(6);
        static const uint32_t   CPSR_T = EMU_BIT(5);
        static const uint32_t   CPSR_MODE_SHIFT = 0;
        static const uint32_t   CPSR_MODE_SIZE = 5;

        void regExport(uint32_t mode);
        void regImport(uint32_t mode);
        uint32_t read32(uint32_t addr);
        void prefetch32();
        void trace();

        Config          mConfig;
        MemoryBus*      mMemory;
        Clock*          mClock;
        uint32_t        mClockDivider;
        uint32_t        mExecutedTick;
        Registers       mRegisters;
        uint32_t        mPC;
        uint32_t        mPCNext;
    };
}