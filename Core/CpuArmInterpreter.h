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

    template <typename T> constexpr T BITS(uint32_t bitEnd, uint32_t bitStart, T value)
    {
        return static_cast<T>((value >> bitStart) & ((1 << (bitEnd - bitStart + 1)) - 1));
    }

    template <typename T> constexpr T BIT(uint32_t bit, T value)
    {
        return static_cast<T>((value >> bit) & 0x01);
    }

    struct CpuArmInterpreter : public emu::CpuArm
    {
        // Helpers /////////////////////////////////////////////////////////////

        uint32_t getRegister(uint32_t regIndex)
        {
            return mRegisters.r[regIndex];
        }

        void setRegister(uint32_t regIndex, uint32_t value)
        {
            mRegisters.r[regIndex] = value;
        }

        static uint32_t evalRORImm32(uint32_t imm, uint32_t shift)
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
        uint32_t evalImmShift(uint32_t value, uint32_t shift)
        {
            switch (type)
            {
            case 0: // LSL
                if (shift)
                {
                    mRegisters.flag_c = BIT(32 - shift, value);
                    return value << (shift & 31);
                }
                else
                {
                    return value;
                }

            case 1: // LSR
                if (shift)
                {
                    mRegisters.flag_c = BIT(shift - 1, value);
                    return static_cast<uint32_t>(value) >> static_cast<uint32_t>(shift);
                }
                else
                {
                    mRegisters.flag_c = BIT(31, value);
                    return 0;
                }

            case 2: // ASR
                if (shift)
                {
                    mRegisters.flag_c = BIT(shift - 1, value);
                    return static_cast<int32_t>(value) >> static_cast<int32_t>(shift);
                }
                else
                {
                    mRegisters.flag_c = BIT(31, value);
                    return -static_cast<int32_t>(mRegisters.flag_c);  // 0 -> 0x00000000, 1 -> 0xffffffff
                }

            case 3: // ROR
                if (shift)
                {
                    mRegisters.flag_c = BIT(shift - 1, value);
                    return evalRORImm32(value, shift);
                }
                else
                {
                    mRegisters.flag_c = BIT(0, value);
                    return (value >> 1) | (mRegisters.flag_c << 31);
                }
            }
            return 0;
        }

        template <uint32_t type>
        uint32_t evalRegShift(uint32_t value, uint32_t shift)
        {
            switch (type)
            {
            case 0: // LSL
                mRegisters.flag_c = BIT(32 - shift, value);
                return value << (shift & 31);

            case 1: // LSR
                mRegisters.flag_c = BIT(shift - 1, value);
                return static_cast<uint32_t>(value) >> static_cast<uint32_t>(shift);

            case 2: // ASR
                mRegisters.flag_c = BIT(shift - 1, value);
                return static_cast<int32_t>(value) >> static_cast<int32_t>(shift);

            case 3: // ROR
                mRegisters.flag_c = BIT(shift - 1, value);
                return evalRORImm32(value, shift);
            }
            return 0;
        }

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

        template <uint32_t TKnownBits>
        struct ALU
        {
            CpuArmInterpreter& cpu;
            uint32_t Rn;
            uint32_t Rd;
            uint32_t Op2;
            bool S;

            ALU(CpuArmInterpreter& _cpu)
                : cpu(_cpu)
            {
                uint32_t opcode = cpu.mOpcode;
                Rn = BITS<19, 16>(opcode);
                Rd = BITS<15, 12>(opcode);
                S = BIT<20>(TKnownBits);
                bool I = BIT<25>(TKnownBits);
                if (I == 0)
                {
                    constexpr uint32_t ShiftType = BITS<6, 5>(TKnownBits);
                    uint32_t Rm = BITS<3, 0>(opcode);
                    bool R = BIT<4>(TKnownBits);
                    if (R == 0)
                    {
                        uint32_t Is = BITS<11, 7>(opcode);
                        Op2 = cpu.evalImmShift<ShiftType>(cpu.getRegister(Rm), Is);
                    }
                    else
                    {
                        uint32_t Rs = BITS<11, 8>(opcode);
                        cpu.prefetch32();
                        Op2 = cpu.evalRegShift<ShiftType>(cpu.getRegister(Rm), cpu.getRegister(Rs));
                    }
                }
                else
                {
                    uint32_t Is = BITS<11, 8>(opcode);
                    uint32_t Immediate = BITS<7, 0>(opcode);
                    Op2 = evalRORImm32(Immediate, Is);
                }
            }

            void saveResult(uint32_t result)
            {
                if (Rd == 15)
                {
                    EMU_NOT_IMPLEMENTED();
                }
                cpu.setRegister(Rd, result);
            }

            void saveFlagsLogical(uint32_t result)
            {
                if (S)
                {
                    cpu.mRegisters.flag_n = BIT<31>(result);
                    cpu.mRegisters.flag_z = result == 0;
                }
            }

            void saveFlagsArithmetic(uint32_t result, bool carry, bool overflow)
            {
                if (S)
                {
                    cpu.mRegisters.flag_n = BIT<31>(result);
                    cpu.mRegisters.flag_z = result == 0;
                    cpu.mRegisters.flag_c = carry;
                    cpu.mRegisters.flag_v = overflow;
                }
            }
        };

        uint32_t addWithCarry(uint32_t a, uint32_t b, uint32_t c, bool& carry_out, bool& overflow_out)
        {
            uint64_t result_unsigned = static_cast<uint64_t>(a) + static_cast<uint64_t>(b) + static_cast<uint64_t>(c);
            int64_t result_signed = static_cast<int64_t>(static_cast<int32_t>(a)) + static_cast<int64_t>(static_cast<int32_t>(b)) + static_cast<int64_t>(c);
            uint64_t result = result_unsigned & 0xffffffff;
            carry_out = result != result_unsigned;
            overflow_out = static_cast<int64_t>(static_cast<int32_t>(result)) != result_signed;
            return static_cast<uint32_t>(result);
        }

        template <uint32_t TKnownBits> void insn_and()
        {
            ALU<TKnownBits> alu(*this);
            uint32_t result = alu.Rn & alu.Op2;
            alu.saveResult(result);
            alu.saveFlagsLogical(result);
        }

        template <uint32_t TKnownBits> void insn_eor()
        {
            ALU<TKnownBits> alu(*this);
            uint32_t result = alu.Rn ^ alu.Op2;
            alu.saveResult(result);
            alu.saveFlagsLogical(result);
        }

        template <uint32_t TKnownBits> void insn_sub()
        {
            ALU<TKnownBits> alu(*this);
            bool carry, overflow;
            uint32_t result = addWithCarry(alu.Rn, ~alu.Op2, 1, carry, overflow);
            alu.saveResult(result);
            alu.saveFlagsArithmetic(result, carry, overflow);
        }

        template <uint32_t TKnownBits> void insn_rsb()
        {
            ALU<TKnownBits> alu(*this);
            bool carry, overflow;
            uint32_t result = addWithCarry(~alu.Rn, alu.Op2, 1, carry, overflow);
            alu.saveResult(result);
            alu.saveFlagsArithmetic(result, carry, overflow);
        }

        template <uint32_t TKnownBits> void insn_add()
        {
            ALU<TKnownBits> alu(*this);
            bool carry, overflow;
            uint32_t result = addWithCarry(alu.Rn, alu.Op2, 0, carry, overflow);
            alu.saveResult(result);
            alu.saveFlagsArithmetic(result, carry, overflow);
        }

        template <uint32_t TKnownBits> void insn_adc()
        {
            ALU<TKnownBits> alu(*this);
            bool carry, overflow;
            uint32_t result = addWithCarry(~alu.Rn, alu.Op2, mRegisters.flag_c, carry, overflow);
            alu.saveResult(result);
            alu.saveFlagsArithmetic(result, carry, overflow);
        }

        template <uint32_t TKnownBits> void insn_sbc()
        {
            ALU<TKnownBits> alu(*this);
            bool carry, overflow;
            uint32_t result = addWithCarry(alu.Rn, ~alu.Op2, 1, carry, overflow);
            alu.saveResult(result);
            alu.saveFlagsArithmetic(result, carry, overflow);
        }

        template <uint32_t TKnownBits> void insn_rsc()
        {
            ALU<TKnownBits> alu(*this);
            bool carry, overflow;
            uint32_t result = addWithCarry(~alu.Rn, alu.Op2, mRegisters.flag_c, carry, overflow);
            alu.saveResult(result);
            alu.saveFlagsArithmetic(result, carry, overflow);
        }

        template <uint32_t TKnownBits> void insn_tst()
        {
            ALU<TKnownBits> alu(*this);
            uint32_t result = alu.Rn & alu.Op2;
            alu.saveFlagsLogical(result);
        }

        template <uint32_t TKnownBits> void insn_teq()
        {
            ALU<TKnownBits> alu(*this);
            uint32_t result = alu.Rn ^ alu.Op2;
            alu.saveFlagsLogical(result);
        }

        template <uint32_t TKnownBits> void insn_cmp()
        {
            ALU<TKnownBits> alu(*this);
            bool carry, overflow;
            uint32_t result = addWithCarry(alu.Rn, ~alu.Op2, 1, carry, overflow);
            alu.saveFlagsArithmetic(result, carry, overflow);
        }

        template <uint32_t TKnownBits> void insn_cmn()
        {
            ALU<TKnownBits> alu(*this);
            bool carry, overflow;
            uint32_t result = addWithCarry(alu.Rn, alu.Op2, 0, carry, overflow);
            alu.saveFlagsArithmetic(result, carry, overflow);
        }

        template <uint32_t TKnownBits> void insn_orr()
        {
            ALU<TKnownBits> alu(*this);
            uint32_t result = alu.Rn | alu.Op2;
            alu.saveResult(result);
            alu.saveFlagsLogical(result);
        }

        template <uint32_t TKnownBits> void insn_mov()
        {
            ALU<TKnownBits> alu(*this);
            uint32_t result = alu.Op2;
            alu.saveResult(result);
            alu.saveFlagsLogical(result);
        }

        template <uint32_t TKnownBits> void insn_bic()
        {
            ALU<TKnownBits> alu(*this);
            uint32_t result = alu.Rn & ~alu.Op2;
            alu.saveResult(result);
            alu.saveFlagsLogical(result);
        }

        template <uint32_t TKnownBits> void insn_mvn()
        {
            ALU<TKnownBits> alu(*this);
            uint32_t result = ~alu.Op2;
            alu.saveResult(result);
            alu.saveFlagsLogical(result);
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