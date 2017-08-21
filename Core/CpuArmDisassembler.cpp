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
        struct ALURegImm { constexpr static Addr getAddr() { return Addr::ALURegImm; } };

        template <uint32_t Opcode, uint32_t S, uint32_t ShiftType, uint32_t R>
        struct ALURegReg { constexpr static Addr getAddr() { return Addr::ALURegReg; } };

        template <uint32_t Opcode, uint32_t S>
        struct ALUImm { constexpr static Addr getAddr() { return Addr::ALUImm; } };

        struct MulRdRmRs { constexpr static Addr getAddr() { return Addr::MulRdRmRs; } };
        struct MulRdRmRsRn { constexpr static Addr getAddr() { return Addr::MulRdRmRsRn; } };
        struct MulRnRdRmRs { constexpr static Addr getAddr() { return Addr::MulRnRdRmRs; } };
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

        uint32_t evalRORImm32(uint32_t imm, uint32_t shift)
        {
            if (!shift)
                return imm;
            shift <<= 1;
            uint32_t lo = (imm >> shift);
            uint32_t hi = imm << (32 - shift);
            uint32_t result = hi | lo;
            return result;
        }

        void disassembleArm(char* instruction, char* operands, uint32_t pc, const uint8_t* insnTable, const uint8_t* addrTable)
        {
            static const char* conditionText[] =
            {
                "eq",   "ne",   "cs",   "cc",
                "mi",   "pl",   "vs",   "vc",
                "hi",   "ls",   "ge",   "lt",
                "gt",   "le",   "",     ""
            };

            static const char* shiftText[] =
            {
                "lsl", "lsr", "asr", "ror"
            };

            static bool rdUsedALU[] =
            {
                true, true, true, true, true, true, true, true,
                false, false, false, false, true, true, true, true,
            };

            static bool rnUsedALU[] =
            {
                true, true, true, true, true, true, true, true,
                true, true, true, true, true, false, true, false,
            };

            static const char* flagsMSR[] =
            {
                "", "_c", "_x", "_xc", "_s", "_sc", "_sx", "_sxc",
                "_f", "_fc", "_fx", "_fxc", "_fs", "_fsc", "_fsx", "_fsxc",
            };

            uint32_t data = read32(pc);
            uint32_t entry = (EMU_BITS_GET(20, 8, data) << 4) | EMU_BITS_GET(4, 4, data);
            uint32_t condition = EMU_BITS_GET(28, 4, data);
            Insn insn = static_cast<Insn>(insnTable[entry]);
            instruction += sprintf(instruction, "%s%s%s", getInsnNameTable()[static_cast<uint32_t>(insn)], conditionText[condition], getInsnSuffixTable()[static_cast<uint32_t>(insn)]);

            operands[0] = 0;
            Addr addr = static_cast<Addr>(addrTable[entry]);
            switch (addr)
            {
            case Addr::Invalid:
            {
                break;
            }
            case Addr::BranchOffset:
            {
                int32_t offset = (static_cast<int32_t>(EMU_BITS_GET(0, 24, data) << 8) >> 6);
                uint32_t label = pc + 8 + offset;
                operands += sprintf(operands, "#0x%x", label);
                break;
            }
            case Addr::BranchReg:
            {
                uint32_t rn = EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "r%d", rn);
                break;
            }
            case Addr::SWI:
            {
                uint32_t imm = EMU_BITS_GET(0, 24, data);
                operands += sprintf(operands, "#0x%x", imm);
                break;
            }
            case Addr::BKPT:
            {
                uint32_t imm = (EMU_BITS_GET(8, 12, data) << 4) | EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "#0x%x", imm);
                break;
            }
            case Addr::ALURegImm:
            {
                uint32_t opcode = EMU_BITS_GET(21, 4, data);
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                uint32_t imm = EMU_BITS_GET(7, 5, data);
                uint32_t shift = EMU_BITS_GET(5, 2, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                if (rdUsedALU[opcode])
                    operands += sprintf(operands, "r%d, ", rd);
                if (rnUsedALU[opcode])
                    operands += sprintf(operands, "r%d, ", rn);
                operands += sprintf(operands, "r%d", rm);
                if (imm)
                    operands += sprintf(operands, ", %s #0x%x", shiftText[shift], imm);
                break;
            }
            case Addr::ALURegReg:
            {
                uint32_t opcode = EMU_BITS_GET(21, 4, data);
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                uint32_t rs = EMU_BITS_GET(8, 4, data);
                uint32_t shift = EMU_BITS_GET(5, 2, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                if (rdUsedALU[opcode])
                    operands += sprintf(operands, "r%d, ", rd);
                if (rnUsedALU[opcode])
                    operands += sprintf(operands, "r%d, ", rn);
                operands += sprintf(operands, "r%d, %s r%d", rm, shiftText[shift], rs);
                break;
            }
            case Addr::ALUImm:
            {
                uint32_t opcode = EMU_BITS_GET(21, 4, data);
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                uint32_t shift = EMU_BITS_GET(8, 4, data);
                uint32_t imm = EMU_BITS_GET(0, 8, data);
                if (rdUsedALU[opcode])
                    operands += sprintf(operands, "r%d, ", rd);
                if (rnUsedALU[opcode])
                    operands += sprintf(operands, "r%d, ", rn);
                operands += sprintf(operands, "#0x%x", evalRORImm32(imm, shift));
                break;
            }
            case Addr::MulRdRmRs:
            {
                uint32_t rd = EMU_BITS_GET(16, 4, data);
                uint32_t rs = EMU_BITS_GET(8, 4, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "r%d, r%d, r%d", rd, rm, rs);
                break;
            }
            case Addr::MulRdRmRsRn:
            {
                uint32_t rd = EMU_BITS_GET(16, 4, data);
                uint32_t rn = EMU_BITS_GET(12, 4, data);
                uint32_t rs = EMU_BITS_GET(8, 4, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "r%d, r%d, r%d, r%d", rd, rm, rs, rn);
                break;
            }
            case Addr::MulRnRdRmRs:
            {
                uint32_t rd = EMU_BITS_GET(16, 4, data);
                uint32_t rn = EMU_BITS_GET(12, 4, data);
                uint32_t rs = EMU_BITS_GET(8, 4, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "r%d, r%d, r%d, r%d", rn, rd, rm, rs);
                break;
            }
            case Addr::CLZ:
            {
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "r%d, r%d", rd, rm);
                break;
            }
            case Addr::QALU:
            {
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "r%d, r%d, r%d", rd, rm, rn);
                break;
            }
            case Addr::MRS:
            {
                uint32_t psr = EMU_BITS_GET(22, 1, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                operands += sprintf(operands, "r%d, %s", rd, psr ? "spsr" : "cpsr");
                break;
            }
            case Addr::MSRReg:
            {
                uint32_t psr = EMU_BITS_GET(22, 1, data);
                uint32_t flags = EMU_BITS_GET(16, 4, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "%s%s, r%d", psr ? "spsr" : "cpsr", flagsMSR[flags], rm);
                break;
            }
            case Addr::MSRImm:
            {
                uint32_t psr = EMU_BIT_GET(22, data);
                uint32_t flags = EMU_BITS_GET(16, 4, data);
                uint32_t shift = EMU_BITS_GET(8, 4, data);
                uint32_t imm = EMU_BITS_GET(0, 8, data);
                operands += sprintf(operands, "%s%s, #0x%x", psr ? "spsr" : "cpsr", flagsMSR[flags], evalRORImm32(imm, shift));
                break;
            }
            case Addr::MemImm:
            {
                uint32_t p = EMU_BIT_GET(24, data);
                uint32_t u = EMU_BIT_GET(23, data);
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                uint32_t imm = EMU_BITS_GET(0, 12, data);
                operands += sprintf(operands, "r%d, ", rd);
                if (p)
                {
                    uint32_t w = EMU_BIT_GET(22, data);
                    if (!imm)
                        operands += sprintf(operands, "[r%d]", rn);
                    else
                        operands += sprintf(operands, "[r%d, #%s0x%x]%s", rn, u ? "" : "-", imm, w ? "!" : "");
                }
                else
                    operands += sprintf(operands, "[r%d], #%s0x%x", rn, u ? "" : "-", imm);
                break;
            }
            case Addr::MemReg:
            {
                uint32_t p = EMU_BIT_GET(24, data);
                uint32_t u = EMU_BIT_GET(23, data);
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                uint32_t is = EMU_BITS_GET(7, 5, data);
                uint32_t shift = EMU_BITS_GET(5, 2, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "r%d, ", rd);
                if (p)
                {
                    uint32_t w = EMU_BIT_GET(22, data);
                    operands += sprintf(operands, "[r%d, %sr%d", rn, u ? "" : "-", rm);
                    if (is)
                        operands += sprintf(operands, ", %s #0x%x", shiftText[shift], is);
                    operands += sprintf(operands, "]%s", w ? "!" : "");
                }
                else
                {
                    operands += sprintf(operands, "[r%d], %sr%d", rn, u ? "" : "-", rm);
                    if (is)
                        operands += sprintf(operands, ", %s #0x%x", shiftText[shift], is);
                }
                break;
            }
            case Addr::MemExReg:
            {
                uint32_t p = EMU_BIT_GET(24, data);
                uint32_t u = EMU_BIT_GET(23, data);
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "r%d, ", rd);
                if (p)
                {
                    uint32_t w = EMU_BIT_GET(22, data);
                    operands += sprintf(operands, "[r%d, %sr%d]%s", rn, u ? "" : "-", rm, w ? "!" : "");
                }
                else
                    operands += sprintf(operands, "[r%d], %sr%d", rn, u ? "" : "-", rm);
                break;
            }
            case Addr::MemExImm:
            {
                uint32_t p = EMU_BIT_GET(24, data);
                uint32_t u = EMU_BIT_GET(23, data);
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                uint32_t imm = (EMU_BITS_GET(8, 4, data) << 4) | EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "r%d, ", rd);
                if (p)
                {
                    uint32_t w = EMU_BIT_GET(22, data);
                    if (!imm)
                        operands += sprintf(operands, "[r%d]", rn);
                    else
                        operands += sprintf(operands, "[r%d, #%s0x%x]%s", rn, u ? "" : "-", imm, w ? "!" : "");
                }
                else
                    operands += sprintf(operands, "[r%d], #%s0x%x", rn, u ? "" : "-", imm);
                break;
            }
            case Addr::MemBlock:
            {
                uint32_t s = EMU_BIT_GET(22, data);
                uint32_t w = EMU_BIT_GET(21, data);
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t rlist = EMU_BITS_GET(0, 16, data);
                operands += sprintf(operands, "r%d%s, {", rn, w ? "!" : "");
                bool first = true;
                while (rlist)
                {
                    size_t pos;
                    emu::platform::findFirstBit(pos, rlist);
                    if (first)
                        first = false;
                    else
                        operands += sprintf(operands, ", ");
                    operands += sprintf(operands, "r%d", static_cast<uint32_t>(pos));
                    rlist &= ~(1 << pos);
                }
                operands += sprintf(operands, "}%s", s ? "^" : "");
                break;
            }
            case Addr::SWP:
            {
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "r%d, r%d, r%d", rd, rm, rn);
            }
            case Addr::CoRegTrans:
            case Addr::CoRegTrans2:
            case Addr::CoDataOp:
            case Addr::CoDataOp2:
            {
                uint32_t cpopc = EMU_BITS_GET(21, 3, data);
                uint32_t cn = EMU_BITS_GET(16, 4, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                uint32_t pn = EMU_BITS_GET(8, 4, data);
                uint32_t cp = EMU_BITS_GET(5, 3, data);
                uint32_t cm = EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "p%d, #%d, r%d, c%d, c%d", pn, cpopc, rd, cn, cm);
                if (cp)
                    operands += sprintf(operands, ", #%d", cp);
                break;
            }
            case Addr::CoDataTrans:
            case Addr::CoDataTrans2:
            {
                uint32_t p = EMU_BIT_GET(24, data);
                uint32_t u = EMU_BIT_GET(23, data);
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t cd = EMU_BITS_GET(12, 4, data);
                uint32_t pn = EMU_BITS_GET(8, 4, data);
                uint32_t imm = EMU_BITS_GET(0, 8, data) << 2;
                operands += sprintf(operands, "p%d, c%d", pn, cd);
                if (p)
                {
                    uint32_t w = EMU_BIT_GET(21, data);
                    if (!imm)
                        operands += sprintf(operands, "[r%d]", rn);
                    else
                        operands += sprintf(operands, "[r%d, #%s0x%x]%s", rn, u ? "" : "-", imm, w ? "!" : "");
                }
                else
                    operands += sprintf(operands, "[r%d], #%s0x%x", rn, u ? "" : "-", imm);
                break;
            }
            case Addr::STREx:
            {
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                operands += sprintf(operands, "r%d, r%d, [r%d]", rd, rm, rn);
                break;
            }
            case Addr::LDREx:
            {
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                uint32_t rd = EMU_BITS_GET(12, 4, data);
                operands += sprintf(operands, "r%d, [r%d]", rd, rn);
                break;
            }
            default:
                EMU_ASSERT(false);
            }
        }

        void disassembleArm7(char* instruction, char* operands, uint32_t addr)
        {
            disassembleArm(instruction, operands, addr, insnTable7, addrTable7);
        }

        void disassembleArm9(char* instruction, char* operands, uint32_t addr)
        {
            disassembleArm(instruction, operands, addr, insnTable9, addrTable9);
        }
    };

#define INSTRUCTION(index, insn, addr)  static_cast<uint8_t>(EMU_GET_MACRO_ARG_TYPE(addr)::getAddr()),
    const uint8_t CpuArmDisassembler::addrTable7[] =
    {
#include "CpuArm7Tables.inl"
    };

    const uint8_t CpuArmDisassembler::addrTable9[] =
    {
#include "CpuArm9Tables.inl"
    };
#undef INSTRUCTION


#define INSTRUCTION(index, insn, addr)  static_cast<uint8_t>(Insn::##insn),
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

        char instruction[32];
        char operands[32];
        char temp[64];
        char* text = temp;
        if (mConfig.family == Family::ARMv5)
            static_cast<CpuArmDisassembler*>(this)->disassembleArm9(instruction, operands, addr);
        else
            static_cast<CpuArmDisassembler*>(this)->disassembleArm7(instruction, operands, addr);
        if (operands[0])
            text += sprintf(text, "%-7s %s", instruction, operands);
        else
            text += sprintf(text, "%s", instruction);

        if (size--)
        {
            strncpy(buffer, temp, size);
            buffer[size] = 0;
        }

        return addr;
    }
}