#include "Clock.h"
#include "CpuArm.h"

namespace
{
    struct CpuArmDisassembler : public emu::CpuArm
    {
        struct Invalid { constexpr static Addr getAddr() { return Addr::Invalid; } };
        struct BranchOffset { constexpr static Addr getAddr() { return Addr::BranchOffset; } };
        struct BranchReg { constexpr static Addr getAddr() { return Addr::BranchReg; } };
        struct SWI { constexpr static Addr getAddr() { return Addr::SWI; } };
        struct BKPT { constexpr static Addr getAddr() { return Addr::BKPT; } };

        template <uint32_t Opcode, uint32_t S, uint32_t ShiftType, uint32_t R>
        struct ALUReg { constexpr static Addr getAddr() { return Addr::ALUReg; } };

        template <uint32_t Opcode, uint32_t S>
        struct ALUImm { constexpr static Addr getAddr() { return Addr::ALUImm; } };

        struct Multiply { constexpr static Addr getAddr() { return Addr::Multiply; } };
        struct CLZ { constexpr static Addr getAddr() { return Addr::CLZ; } };
        struct QALU { constexpr static Addr getAddr() { return Addr::QALU; } };

        template <uint32_t PSR>
        struct MRS { constexpr static Addr getAddr() { return Addr::MRS; } };

        template <uint32_t PSR>
        struct MSRReg { constexpr static Addr getAddr() { return Addr::MSRReg; } };

        template <uint32_t PSR>
        struct MSRImm { constexpr static Addr getAddr() { return Addr::MSRImm; } };

        template <uint32_t P, uint32_t U, uint32_t W>
        struct MemImm { constexpr static Addr getAddr() { return Addr::MemImm; } };

        template <uint32_t P, uint32_t U, uint32_t W, uint32_t Type>
        struct MemReg { constexpr static Addr getAddr() { return Addr::MemReg; } };

        template <uint32_t P, uint32_t U, uint32_t W>
        struct MemExReg { constexpr static Addr getAddr() { return Addr::MemExReg; } };

        template <uint32_t P, uint32_t U, uint32_t W>
        struct MemExImm { constexpr static Addr getAddr() { return Addr::MemExImm; } };

        template <uint32_t P, uint32_t U, uint32_t S, uint32_t W>
        struct MemBlock { constexpr static Addr getAddr() { return Addr::MemBlock; } };

        struct SWP { constexpr static Addr getAddr() { return Addr::SWP; } };

        template <uint32_t P, uint32_t U, uint32_t N, uint32_t W>
        struct CoDataTrans { constexpr static Addr getAddr() { return Addr::CoDataTrans; } };

        template <uint32_t P, uint32_t U, uint32_t N, uint32_t W>
        struct CoDataTrans2 { constexpr static Addr getAddr() { return Addr::CoDataTrans2; } };

        template <uint32_t CPOPC, uint32_t CP>
        struct CoRegTrans { constexpr static Addr getAddr() { return Addr::CoRegTrans; } };

        template <uint32_t CPOPC, uint32_t CP>
        struct CoRegTrans2 { constexpr static Addr getAddr() { return Addr::CoRegTrans2; } };

        template <uint32_t CPOPC, uint32_t CP>
        struct CoDataOp { constexpr static Addr getAddr() { return Addr::CoDataOp; } };

        template <uint32_t CPOPC, uint32_t CP>
        struct CoDataOp2 { constexpr static Addr getAddr() { return Addr::CoDataOp2; } };

        struct STREx { constexpr static Addr getAddr() { return Addr::STREx; } };
        struct LDREx { constexpr static Addr getAddr() { return Addr::LDREx; } };

        static const uint8_t insnTable7[];
        static const uint8_t insnTable9[];
        static const uint8_t addrTable7[];
        static const uint8_t addrTable9[];
    };

#define INSTRUCTION(insn, addr)     static_cast<uint8_t>(EMU_GET_MACRO_ARG_TYPE(addr)::getAddr()),
    const uint8_t CpuArmDisassembler::addrTable7[] =
    {
#include "CpuArm7Tables.inl"
    };

    const uint8_t CpuArmDisassembler::addrTable9[] =
    {
#include "CpuArm9Tables.inl"
    };
#undef INSTRUCTION


#define INSTRUCTION(insn, addr)     static_cast<uint8_t>(Insn::##insn),
    const uint8_t CpuArmDisassembler::insnTable7[] =
    {
#include "CpuArm7Tables.inl"
    };

    const uint8_t CpuArmDisassembler::insnTable9[] =
    {
#include "CpuArm9Tables.inl"
    };
#undef INSTRUCTION
}

namespace emu
{
    uint32_t CpuArm::disassemble(char* buffer, size_t size, uint32_t addr, bool thumb)
    {
        if (thumb)
        {
            EMU_INVOKE_ONCE(printf("Thumb not implemented!\n"));
        }

        char temp[64];
        char* text = temp;
        text += sprintf(text, "???");

        if (size--)
        {
            strncpy(buffer, temp, size);
            buffer[size] = 0;
        }

        return addr;
    }
}