#include "CpuARM946ES.h"
#include "CpuArmDisassembler.h"
#include "CpuArmInterpreter.h"

namespace
{
#define INSTRUCTION(known, prefix, insn, addr)  static_cast<uint8_t>(EMU_GET_MACRO_ARG_TYPE(addr)::getAddr()),
    const uint8_t CpuArmDisassembler::addrTable[] =
    {
#include "CpuTablesARM946ES.inl"
    };
#undef INSTRUCTION

#define INSTRUCTION(known, prefix, insn, addr)  static_cast<uint8_t>(Insn::##insn),
    const uint8_t CpuArmDisassembler::insnTable[] =
    {
#include "CpuTablesARM946ES.inl"
    };
#undef INSTRUCTION

#define INSTRUCTION(known, prefix, insn, addr)  &CpuArmInterpreter::insn_##insn<EMU_GET_MACRO_ARG_TYPE(addr)>,
    const CpuArmInterpreter::InterpretedFunction CpuArmInterpreter::insnTable[] =
    {
#include "CpuTablesARM946ES.inl"
    };
#undef INSTRUCTION
}

namespace emu
{
    uint32_t ARM946ES::disassemble(char* buffer, size_t size, uint32_t addr, bool thumb)
    {
        return static_cast<CpuArmDisassembler*>(static_cast<CpuArm*>(this))->disassembleImpl(buffer, size, addr, thumb);
    }

    uint32_t ARM946ES::execute()
    {
        return static_cast<CpuArmInterpreter*>(static_cast<CpuArm*>(this))->interpretImpl();
    }
}
