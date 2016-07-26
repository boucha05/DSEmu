#include "Clock.h"
#include "CpuArm.h"

namespace
{
#include "CpuArmTables.inl"
}

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
        mClock->addClocked(*this, true);
        return true;
    }

    int32_t CpuArm::execute(int32_t tick)
    {
        for (uint32_t index = 0; index < 50; ++index)
        {
            trace();
            mExecutedTick += execute();
            prefetch32();
        }
        printf("\n");

        mExecutedTick = tick;
        return tick;
    }

    void CpuArm::advance(int32_t tick)
    {
        mExecutedTick -= tick;
    }

    void CpuArm::reset()
    {
        for (uint32_t index = 0; index < 16; ++index)
        {
            mRegisters.r[index] = 0;
            mRegisters.cpsr = 0;
            mRegisters.spsr = 0;
        }
        regExport(MODE_USR);
        regExport(MODE_FIQ);
        regExport(MODE_IRQ);
        regExport(MODE_SVC);
        regExport(MODE_ABT);
        regExport(MODE_UND);
        regExport(MODE_SYS);
    }

    const char** CpuArm::getInsnNameTable()
    {
        return ::InsnName;
    }

    const char** CpuArm::getInsnSuffixTable()
    {
        return ::InsnSuffix;
    }

    void CpuArm::setPC(uint32_t addr)
    {
        mPC = mPCNext = mRegisters.r[15] = addr;
        prefetch32();
    }

    uint32_t CpuArm::read32(uint32_t addr)
    {
        return mMemory->read32(addr);
    }

    void CpuArm::prefetch32()
    {
        uint32_t pc = mPCNext;
        mPC = pc;
        mPCNext = pc + 4;
        mRegisters.r[15] = pc + 8;
    }

    void CpuArm::regExport(uint32_t mode)
    {
        switch (mode)
        {
        case MODE_USR:
        case MODE_SYS:
            break;

        case MODE_FIQ:
            mRegisters.fiq.r8 = mRegisters.r[8];
            mRegisters.fiq.r9 = mRegisters.r[9];
            mRegisters.fiq.r10 = mRegisters.r[10];
            mRegisters.fiq.r11 = mRegisters.r[11];
            mRegisters.fiq.r12 = mRegisters.r[12];
            mRegisters.fiq.r13 = mRegisters.r[13];
            mRegisters.fiq.r14 = mRegisters.r[14];
            mRegisters.fiq.spsr = mRegisters.spsr;
            break;

        case MODE_SVC:
            mRegisters.svc.r13 = mRegisters.r[13];
            mRegisters.svc.r14 = mRegisters.r[14];
            mRegisters.svc.spsr = mRegisters.spsr;
            break;

        case MODE_ABT:
            mRegisters.abt.r13 = mRegisters.r[13];
            mRegisters.abt.r14 = mRegisters.r[14];
            mRegisters.abt.spsr = mRegisters.spsr;
            break;

        case MODE_IRQ:
            mRegisters.irq.r13 = mRegisters.r[13];
            mRegisters.irq.r14 = mRegisters.r[14];
            mRegisters.irq.spsr = mRegisters.spsr;
            break;

        case MODE_UND:
            mRegisters.svc.r13 = mRegisters.r[13];
            mRegisters.svc.r14 = mRegisters.r[14];
            mRegisters.svc.spsr = mRegisters.spsr;
            break;

        default:
            EMU_ASSERT(false);
        }
    }

    void CpuArm::regImport(uint32_t mode)
    {
        switch (mode)
        {
        case MODE_USR:
        case MODE_SYS:
            break;

        case MODE_FIQ:
            mRegisters.r[8] = mRegisters.fiq.r8;
            mRegisters.r[9] = mRegisters.fiq.r9;
            mRegisters.r[10] = mRegisters.fiq.r10;
            mRegisters.r[11] = mRegisters.fiq.r11;
            mRegisters.r[12] = mRegisters.fiq.r12;
            mRegisters.r[13] = mRegisters.fiq.r13;
            mRegisters.r[14] = mRegisters.fiq.r14;
            mRegisters.spsr = mRegisters.fiq.spsr;
            break;

        case MODE_SVC:
            mRegisters.r[13] = mRegisters.svc.r13;
            mRegisters.r[14] = mRegisters.svc.r14;
            mRegisters.spsr = mRegisters.svc.spsr;
            break;

        case MODE_ABT:
            mRegisters.r[13] = mRegisters.abt.r13;
            mRegisters.r[14] = mRegisters.abt.r14;
            mRegisters.spsr = mRegisters.abt.spsr;
            break;

        case MODE_IRQ:
            mRegisters.r[13] = mRegisters.irq.r13;
            mRegisters.r[14] = mRegisters.irq.r14;
            mRegisters.spsr = mRegisters.irq.spsr;
            break;

        case MODE_UND:
            mRegisters.r[13] = mRegisters.und.r13;
            mRegisters.r[14] = mRegisters.und.r14;
            mRegisters.spsr = mRegisters.und.spsr;
            break;

        default:
            EMU_ASSERT(false);
        }
    }

    void CpuArm::trace()
    {
        bool thumb = (mRegisters.cpsr & CPSR_T) != 0;
        char disassembly[32];
        uint32_t data = read32(mPC);
        disassemble(disassembly, sizeof(disassembly), mPC, thumb);
        printf("%d %08X %08X %-32s\n", mConfig.family, data, mPC, disassembly);
    }
}