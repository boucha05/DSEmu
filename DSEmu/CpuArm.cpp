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
        flagsImport();
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

    void CpuArm::flagsExport()
    {
        uint32_t value_v = mRegisters.flag_v ? 1 : 0;
        uint32_t value_c = mRegisters.flag_c ? 1 : 0;
        uint32_t value_z = mRegisters.flag_z ? 0 : 1;
        uint32_t value_n = static_cast<int32_t>(mRegisters.flag_n) < 0 ? 1 : 0;
        EMU_BIT_SET(CPSR_V, mRegisters.flag_v, value_v);
        EMU_BIT_SET(CPSR_C, mRegisters.flag_c, value_c);
        EMU_BIT_SET(CPSR_Z, mRegisters.flag_z, value_z);
        EMU_BIT_SET(CPSR_N, mRegisters.flag_n, value_n);
    }

    void CpuArm::flagsImport()
    {
        mRegisters.flag_v = EMU_BIT_GET(CPSR_V, mRegisters.cpsr) ? 1 : 0;
        mRegisters.flag_c = EMU_BIT_GET(CPSR_C, mRegisters.cpsr) ? 1 : 0;
        mRegisters.flag_z = EMU_BIT_GET(CPSR_Z, mRegisters.cpsr) ? 0 : 1;
        mRegisters.flag_n = EMU_BIT_GET(CPSR_N, mRegisters.cpsr) ? -1 : 0;
    }

    void CpuArm::regExport(uint32_t mode)
    {
        flagsExport();
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
        flagsImport();
    }

    void CpuArm::trace()
    {
        bool thumb = EMU_BIT_GET(CPSR_T, mRegisters.cpsr) != 0;
        char disassembly[32];
        uint32_t data = read32(mPC);
        disassemble(disassembly, sizeof(disassembly), mPC, thumb);
        printf("%d %08X %08X %-32s\n", mConfig.family, data, mPC, disassembly);
    }
}