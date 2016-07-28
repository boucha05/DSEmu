#include "Clock.h"
#include "CpuArm.h"

namespace
{
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
            uint32_t rn_value;
            uint32_t rd;
            uint32_t op2;
            uint32_t clock;
            uint32_t c;

            ALUBase(CpuArmInterpreter& _cpu, uint32_t data)
                : cpu(_cpu)
            {
                c = cpu.mRegisters.flag_c;
                uint32_t rn = EMU_BITS_GET(16, 4, data);
                rn_value = cpu.mRegisters.r[rn];
                if (rn == 15)
                    rn_value += 4;
                rd = EMU_BITS_GET(12, 4, data);
            }

            void save(uint32_t result)
            {
                cpu.mRegisters.r[rd] = result;
                if (rd == 15)
                {
                    cpu.mPCNext = result;
                    clock += 2;
                }
            }

            void flags(uint32_t flag_c, uint32_t flags_zn)
            {
                cpu.mRegisters.flag_c = flag_c;
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

        template <uint32_t P, uint32_t U, uint32_t W>
        struct MemImm
        {
        };

        template <uint32_t P, uint32_t U, uint32_t W, uint32_t Type>
        struct MemReg
        {
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

        template <typename Addr> uint32_t insn_invalid(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_b(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_bl(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_bx(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_blx(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_swi(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_bkpt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_and(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ands(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_eor(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_eors(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_sub(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_subs(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_rsb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_rsbs(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_add(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_adds(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_adc(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_adcs(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_sbc(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_sbcs(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_rsc(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_rscs(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_tsts(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_teqs(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_cmps(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_cmns(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_orr(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_orrs(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_mov(uint32_t insn)
        {
            Addr addr(*this, insn);
            addr.save(addr.op2);
            return addr.clock;
        }

        template <typename Addr> uint32_t insn_movs(uint32_t insn)
        {
            Addr addr(*this, insn);
            addr.save(addr.op2);
            addr.flags(addr.c, addr.op2);
            return addr.clock;
        }

        template <typename Addr> uint32_t insn_bic(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_bics(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_mvn(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_mvns(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_mul(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_muls(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_mla(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_mlas(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_umull(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_umulls(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_umlal(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_umlals(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smull(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smulls(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smlal(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smlals(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smlabb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smlatb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smlabt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smlatt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smlawb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smulwb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smlawt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smulwt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smlalbb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smlaltb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smlalbt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smlaltt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smulbb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smultb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smulbt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_smultt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_clz(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_qadd(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_qsub(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_qdadd(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_qdsub(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_mrs(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_msr(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_str(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldr(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_strt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldrt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_strb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldrb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_strbt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldrbt(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_strh(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldrd(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_strd(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldrh(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldrsb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldrsh(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_stmda(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldmda(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_stmia(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldmia(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_stmdb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldmdb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_stmib(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldmib(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_swp(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_swpb(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_stc2(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldc2(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_stc2l(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldc2l(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_cdp2(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_mcr2(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_mrc2(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_strex(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldrex(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_stc(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldc(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_stcl(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_ldcl(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_cdp(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_mcr(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        template <typename Addr> uint32_t insn_mrc(uint32_t insn)
        {
            EMU_UNUSED(insn);
            EMU_NOT_IMPLEMENTED();
            return 1;
        }

        // Interpreter /////////////////////////////////////////////////////////

        typedef uint32_t (CpuArmInterpreter::* InterpretedFunction)(uint32_t insn);

        static const InterpretedFunction insnTable7[];
        static const InterpretedFunction insnTable9[];

        uint32_t interpretArm(const InterpretedFunction* insnTable)
        {
            uint32_t insn = read32(mPC);
            uint32_t entry = (EMU_BITS_GET(20, 8, insn) << 4) | EMU_BITS_GET(4, 4, insn);
            return (this->*insnTable[entry])(insn);
        }

        uint32_t interpretArm7()
        {
            return interpretArm(insnTable7);
        }

        uint32_t interpretArm9()
        {
            return interpretArm(insnTable9);
        }
    };

#define INSTRUCTION(insn, addr)     &CpuArmInterpreter::insn_##insn<EMU_GET_MACRO_ARG_TYPE(addr)>,
    const CpuArmInterpreter::InterpretedFunction CpuArmInterpreter::insnTable7[] =
    {
#include "CpuArm7Tables.inl"
    };

    const CpuArmInterpreter::InterpretedFunction CpuArmInterpreter::insnTable9[] =
    {
#include "CpuArm9Tables.inl"
    };
#undef INSTRUCTION
}

namespace emu
{
    uint32_t CpuArm::execute()
    {
        if (mConfig.family == Family::ARMv5)
            return static_cast<CpuArmInterpreter*>(this)->interpretArm7();
        else
            return static_cast<CpuArmInterpreter*>(this)->interpretArm9();
    }
}
