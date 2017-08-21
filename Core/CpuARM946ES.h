#pragma once

#include "CpuArm.h"

namespace emu
{
    class ARM946ES : public CpuArm
    {
    public:
        virtual uint32_t disassemble(char* buffer, size_t size, uint32_t addr, bool thumb) override;
        virtual uint32_t execute() override;
    };
}