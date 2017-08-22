#pragma once

#include "CpuArm.h"

#pragma warning(push)
#pragma warning(disable:4127)

namespace
{
    template <uint8_t bitEnd, uint8_t bitStart, typename T> constexpr T BITS(T value)
    {
        return static_cast<T>((value >> bitStart) & ((1 << (bitEnd - bitStart + 1)) - 1));
    }

    template <uint8_t bit, typename T> constexpr T BIT(T value)
    {
        return static_cast<T>((value >> bit) & 0x01);
    }

    struct CpuArmInterpreter : public emu::CpuArm
    {
        // Helpers /////////////////////////////////////////////////////////////
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

        template <uint32_t type>
        uint32_t evalShift(uint32_t& c, uint32_t value, uint32_t shift)
        {
            switch (type)
            {
            case 0: // LSL
                if (shift)
                {
                    c = EMU_BIT_GET(32 - shift, value);
                    return value << (shift & 31);
                }
                else
                {
                    c = mRegisters.flag_c;
                    return value;
                }

            case 1: // LSR
                if (shift)
                {
                    c = EMU_BIT_GET(shift - 1, value);
                    return static_cast<uint32_t>(value) >> static_cast<uint32_t>(shift);
                }
                else
                {
                    c = EMU_BIT_GET(31, value);
                    return 0;
                }

            case 2: // ASR
                if (shift)
                {
                    c = EMU_BIT_GET(shift - 1, value);
                    return static_cast<int32_t>(value) >> static_cast<int32_t>(shift);
                }
                else
                {
                    c = EMU_BIT_GET(31, value);
                    return -static_cast<int32_t>(c);  // 0 -> 0x00000000, 1 -> 0xffffffff
                }

            case 3: // ROR
                if (shift)
                {
                    c = EMU_BIT_GET(shift - 1, value);
                    return evalRORImm32(value, shift);
                }
                else
                {
                    c = EMU_BIT_GET(0, value);
                    return (value >> 1) | (mRegisters.flag_c << 31);
                }
            }
            return 0;
        }

        // Addressing modes ////////////////////////////////////////////////////

        struct Invalid
        {
        };

        struct BranchOffset
        {
        };

        struct BranchReg
        {
        };

        struct SWI
        {
        };

        struct BKPT
        {
        };

        template <uint32_t Opcode, uint32_t S>
        struct ALUBase
        {
            CpuArmInterpreter& cpu;
            uint32_t rn;
            uint32_t rn_value;
            uint32_t rd;
            uint32_t op2;
            uint32_t clock;
            uint32_t c;

            ALUBase(CpuArmInterpreter& _cpu, uint32_t data)
                : cpu(_cpu)
            {
                c = cpu.mRegisters.flag_c;
                rn = EMU_BITS_GET(16, 4, data);
                rn_value = cpu.mRegisters.r[rn];
                if (rn == 15)
                    rn_value += 4;
                rd = EMU_BITS_GET(12, 4, data);
            }

            void save(uint32_t reg, uint32_t result)
            {
                cpu.mRegisters.r[reg] = result;
                if (reg == 15)
                {
                    cpu.mPCNext = result;
                    clock += 2;
                }
            }

            void flags(uint32_t flags_zn)
            {
                cpu.mRegisters.flag_c = c;
                cpu.mRegisters.flag_z = flags_zn;
                cpu.mRegisters.flag_n = flags_zn;
            }
        };

        template <uint32_t Opcode, uint32_t S, uint32_t ShiftType, uint32_t R>
        struct ALURegImm : ALUBase<Opcode, S>
        {
            ALURegImm(CpuArmInterpreter& _cpu, uint32_t data)
                : ALUBase(_cpu, data)
            {
                uint32_t imm = EMU_BITS_GET(7, 5, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                uint32_t rm_value = cpu.mRegisters.r[rm];
                if (rm == 15)
                    rm_value += 4;
                op2 = cpu.evalShift<ShiftType>(c, rm_value, imm);
                clock = 2;
            }
        };

        template <uint32_t Opcode, uint32_t S, uint32_t ShiftType, uint32_t R>
        struct ALURegReg : ALUBase<Opcode, S>
        {
            ALURegReg(CpuArmInterpreter& _cpu, uint32_t data)
                : ALUBase(_cpu, data)
            {
                uint32_t rs = EMU_BITS_GET(8, 4, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                uint32_t rm_value = cpu.mRegisters.r[rm];
                if (rm == 15)
                    rm_value += 4;
                op2 = cpu.evalShift<ShiftType>(c, rm_value, cpu.mRegisters.r[rs]);
                clock = 3;
            }
        };

        template <uint32_t Opcode, uint32_t S>
        struct ALUImm : ALUBase<Opcode, S>
        {
            ALUImm(CpuArmInterpreter& _cpu, uint32_t data)
                : ALUBase(_cpu, data)
            {
                uint32_t shift = EMU_BITS_GET(8, 4, data);
                uint32_t imm = EMU_BITS_GET(0, 8, data);
                op2 = cpu.evalRORImm32(imm, shift);
                clock = 2;
            }
        };

        struct MulRdRmRs
        {
        };

        struct MulRdRmRsRn
        {
        };

        struct MulRnRdRmRs
        {
        };

        struct CLZ
        {
        };

        struct QALU
        {
        };

        template <uint32_t PSR>
        struct MRS
        {
        };

        template <uint32_t PSR>
        struct MSRReg
        {
        };

        template <uint32_t PSR>
        struct MSRImm
        {
        };

        template <uint32_t P, uint32_t U>
        struct MemBase
        {
            CpuArmInterpreter& cpu;
            uint32_t imm;
            uint32_t rd_value;
            uint32_t rn;
            uint32_t mem;

            MemBase(CpuArmInterpreter& _cpu, uint32_t data)
                : cpu(_cpu)
            {
                imm = 0;
                rd_value = cpu.mRegisters.r[EMU_BITS_GET(12, 4, data)];
                rn = EMU_BITS_GET(16, 4, data);
                mem = cpu.mRegisters.r[rn];
                if (P)
                    save_mem();
            }

            void save_mem()
            {
                if (!U)
                    mem -= imm;
                else
                    mem += imm;
                cpu.mRegisters.r[rn] = mem;
            }

            void pre_mem()
            {
                if (!P)
                    save_mem();
            }

            void post_mem()
            {
                if (!P)
                    save_mem();
            }
        };

        template <uint32_t P, uint32_t U, uint32_t W>
        struct MemImm : MemBase<P, U>
        {
            MemImm(CpuArmInterpreter& _cpu, uint32_t data)
                : MemBase(_cpu, data)
            {
                imm = EMU_BITS_GET(0, 12, data);
                pre_mem();
            }
        };

        template <uint32_t P, uint32_t U, uint32_t W, uint32_t ShiftType>
        struct MemReg : MemBase<P, U>
        {
            MemReg(CpuArmInterpreter& _cpu, uint32_t data)
                : MemBase(_cpu, data)
            {
                uint32_t shift = EMU_BITS_GET(7, 5, data);
                uint32_t rm = EMU_BITS_GET(0, 4, data);
                uint32_t rm_value = cpu.mRegisters.r[rm];
                if (rm == 15)
                    rm_value += 4;
                imm = cpu.evalShift<ShiftType>(cpu.mRegisters.flag_c, rm_value, shift);
                pre_mem();
            }
        };

        template <uint32_t P, uint32_t U, uint32_t W>
        struct MemExReg
        {
        };

        template <uint32_t P, uint32_t U, uint32_t W>
        struct MemExImm
        {
        };

        template <uint32_t P, uint32_t U, uint32_t S, uint32_t W>
        struct MemBlock
        {
        };

        struct SWP
        {
        };

        template <uint32_t P, uint32_t U, uint32_t N, uint32_t W>
        struct CoDataTrans
        {
        };

        template <uint32_t P, uint32_t U, uint32_t N, uint32_t W>
        struct CoDataTrans2
        {
        };

        template <uint32_t CPOPC, uint32_t CP>
        struct CoRegTrans
        {
        };

        template <uint32_t CPOPC, uint32_t CP>
        struct CoRegTrans2
        {
        };

        template <uint32_t CPOPC, uint32_t CP>
        struct CoDataOp
        {
        };

        template <uint32_t CPOPC, uint32_t CP>
        struct CoDataOp2
        {
        };

        struct STREx
        {
        };

        struct LDREx
        {
        };

        // Instructions ////////////////////////////////////////////////////////

        template <uint32_t TKnownBits> void insn_invalid()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_b()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_bl()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_bx()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_blx()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_swi()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_bkpt()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_and()
        {
            EMU_NOT_IMPLEMENTED();
            //Addr addr(*this, insn);
            //uint32_t result = addr.rn_value & addr.op2;
            //addr.save(addr.rd, result);
            //if (S) addr.flags(result); // or do this inside addr.flags()
            //return addr.clock;
        }

        template <uint32_t TKnownBits> void insn_eor()
        {
            EMU_NOT_IMPLEMENTED();
            //Addr addr(*this, insn);
            //uint32_t result = addr.rn_value ^ addr.op2;
            //addr.save(addr.rd, result);
            //if (S) addr.flags(result); // or do this inside addr.flags()
            //return addr.clock;
        }

        template <uint32_t TKnownBits> void insn_sub()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_rsb()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_add()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_adc()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_sbc()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_rsc()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_tst()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_teq()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_cmp()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_cmn()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_orr()
        {
            EMU_NOT_IMPLEMENTED();
            //Addr addr(*this, insn);
            //uint32_t result = addr.rn_value | addr.op2;
            //addr.save(addr.rd, result);
            //if (S) addr.flags(result); // or do this inside addr.flags()
            //return addr.clock;
        }

        template <uint32_t TKnownBits> void insn_mov()
        {
            EMU_NOT_IMPLEMENTED();
            //Addr addr(*this, insn);
            //uint32_t result = addr.op2;
            //addr.save(addr.rd, result);
            //if (S) addr.flags(result); // or do this inside addr.flags()
            //return addr.clock;
        }

        template <uint32_t TKnownBits> void insn_bic()
        {
            EMU_NOT_IMPLEMENTED();
            //Addr addr(*this, insn);
            //uint32_t result = addr.rn_value & ~addr.op2;
            //addr.save(addr.rd, result);
            //if (S) addr.flags(result); // or do this inside addr.flags()
            //return addr.clock;
        }

        template <uint32_t TKnownBits> void insn_mvn()
        {
            EMU_NOT_IMPLEMENTED();
            //Addr addr(*this, insn);
            //uint32_t result = ~addr.op2;
            //addr.save(addr.rd, result);
            //if (S) addr.flags(result); // or do this inside addr.flags()
            //return addr.clock;
        }

        template <uint32_t TKnownBits> void insn_mul()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_mla()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_umull()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_umlal()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smull()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smlal()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smlabb()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smlatb()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smlabt()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smlatt()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smlawb()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smulwb()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smlawt()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smulwt()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smlalbb()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smlaltb()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smlalbt()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smlaltt()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smulbb()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smultb()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smulbt()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_smultt()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_clz()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_qadd()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_qsub()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_qdadd()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_qdsub()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_mrs()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_msr()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_str()
        {
            EMU_NOT_IMPLEMENTED();
            //Addr addr(*this, insn);
            //mMemory->write32(addr.mem, addr.rd_value);
            //addr.post_mem();
            //return 2;
        }

        template <uint32_t TKnownBits> void insn_ldr()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_stm()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_ldm()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_swp()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_swpb()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_stc2()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_ldc2()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_cdp2()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_mcr2()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_mrc2()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_strex()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_ldrex()
        {
            EMU_NOT_IMPLEMENTED();
        }
        
        template <uint32_t TKnownBits> void insn_stc()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_ldc()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_cdp()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_mcr()
        {
            EMU_NOT_IMPLEMENTED();
        }

        template <uint32_t TKnownBits> void insn_mrc()
        {
            EMU_NOT_IMPLEMENTED();
        }

        // Interpreter /////////////////////////////////////////////////////////

        typedef void(CpuArmInterpreter::* InterpretedFunction)();

        static const InterpretedFunction insnTable[];

        uint32_t interpretImpl()
        {
            mOpcode = read32(mPC);
            uint32_t entry = (EMU_BITS_GET(20, 8, mOpcode) << 4) | EMU_BITS_GET(4, 4, mOpcode);
            (this->*insnTable[entry])();
            return 1;
        }
    };
}

#pragma warning(pop)