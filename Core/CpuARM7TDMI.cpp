#include "CpuARM7TDMI.h"
#include "CpuArmDisassembler.h"
#include "CpuArmInterpreter.h"

namespace
{
#define INSTRUCTION(index, insn, addr)  static_cast<uint8_t>(EMU_GET_MACRO_ARG_TYPE(addr)::getAddr()),
    const uint8_t CpuArmDisassembler::addrTable[] =
    {
#include "CpuArm7Tables.inl"
    };
#undef INSTRUCTION

#define INSTRUCTION(index, insn, addr)  static_cast<uint8_t>(Insn::##insn),
    const uint8_t CpuArmDisassembler::insnTable[] =
    {
#include "CpuArm7Tables.inl"
    };
#undef INSTRUCTION

#define INSTRUCTION(index, insn, addr)  &CpuArmInterpreter::insn_##insn<EMU_GET_MACRO_ARG_TYPE(addr)>,
    const CpuArmInterpreter::InterpretedFunction CpuArmInterpreter::insnTable[] =
    {
#include "CpuArm7Tables.inl"
    };
#undef INSTRUCTION
}

namespace emu
{
    uint32_t ARM7TDMI::disassemble(char* buffer, size_t size, uint32_t addr, bool thumb)
    {
        return static_cast<CpuArmDisassembler*>(static_cast<CpuArm*>(this))->disassembleImpl(buffer, size, addr, thumb);
    }

    uint32_t ARM7TDMI::execute()
    {
        return static_cast<CpuArmInterpreter*>(static_cast<CpuArm*>(this))->interpretImpl();
    }
}
