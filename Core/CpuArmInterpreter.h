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

    static const uint32_t KNOWN_BITS_MASK = 0x0ff000f0;

    template <uint8_t bit, typename T> constexpr T KNOWN_BIT(T value)
    {
        static_assert(((1 << bit) & ~KNOWN_BITS_MASK) == 0, "Invalid bit passed to KNOWN_BIT");
        return BIT<bit>(value);
    }

    template <uint8_t bitEnd, uint8_t bitStart, typename T> constexpr T KNOWN_BITS(T value)
    {
        static_assert((((((1 << (bitEnd - bitStart + 1)) - 1) << bitStart)) & ~KNOWN_BITS_MASK) == 0, "Invalid bits passed to KNOWN_BIT");
        return BITS<bitEnd, bitStart>(value);
    }

    struct CpuArmInterpreter : public emu::CpuArm
    {
        // Helpers /////////////////////////////////////////////////////////////

#if EMU_CONFIG_LITTLE_ENDIAN
        static const uint32_t MEM_ACCESS_ENDIAN_8 = 0x00000000;
        static const uint32_t MEM_ACCESS_ENDIAN_16 = 0x00000000;
#else
        static const uint32_t MEM_ACCESS_ENDIAN_8 = 0x00000003;
        static const uint32_t MEM_ACCESS_ENDIAN_16 = 0x00000002;
#endif

        bool conditionFlagsPassed()
        {
            switch (BITS<31, 28>(mOpcode))
            {
            case 0xe: return true;
            default:
                EMU_NOT_IMPLEMENTED();
            }
            return false;
        }

        uint32_t getRegister(uint32_t regIndex)
        {
            return mRegisters.r[regIndex];
        }

        void setRegister(uint32_t regIndex, uint32_t value)
        {
            mRegisters.r[regIndex] = value;
        }

        struct ShiftResult
        {
            uint32_t value;
            uint32_t cf;
        };

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

        ShiftResult LSL(uint32_t value, uint32_t shift)
        {
            if (shift)
                return { shift < 32 ? value << shift : 0, shift <= 32 ? BIT(32 - shift, value) : 0 };
            else
                return { value, mRegisters.flag_c };
        }

        ShiftResult LSR(uint32_t value, uint32_t shift)
        {
            if (shift)
                return { shift < 32 ? uint32_t(value) >> uint32_t(shift) : 0, shift <= 32 ? BIT(shift - 1, value) : 0 };
            else
                return { value, mRegisters.flag_c };
        }

        ShiftResult ASR(uint32_t value, uint32_t shift)
        {
            if (shift)
                return { shift < 32 ? int32_t(value) >> int32_t(shift) : BIT(31, value), shift <= 32 ? BIT(shift - 1, value) : BIT(31, value) };
            else
                return { uint32_t(-int32_t(mRegisters.flag_c)) /* 0 -> 0x00000000, 1 -> 0xffffffff */, BIT(31, value) };
        }

        ShiftResult ROR(uint32_t value, uint32_t shift)
        {
            return { evalRORImm32(value, shift), BIT(shift - 1, value) };
        }

        ShiftResult RRX(uint32_t value)
        {
            return { (value >> 1) | (mRegisters.flag_c << 31), BIT(0, value) };
        }

        template <uint32_t type>
        ShiftResult evalImmShift(uint32_t value, uint32_t shift)
        {
            if (type == 0) return LSL(value, shift);
            if (type == 1) return LSR(value, shift ? shift : 32);
            if (type == 2) return ASR(value, shift ? shift : 32);
            if (shift) return ROR(value, shift);
            return RRX(value);
        }

        template <uint32_t type>
        ShiftResult evalRegShift(uint32_t value, uint32_t shift)
        {
            if (type == 0) return LSL(value, shift);
            if (type == 1) return LSR(value, shift);
            if (type == 2) return ASR(value, shift);
            return ROR(value, shift);
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
        void armDataProcRegImm()
        {
            if (!conditionFlagsPassed()) return;
            uint32_t Rm = BITS<3, 0>(mOpcode);
            uint32_t Is = BITS<11, 7>(mOpcode);
            constexpr uint32_t ShiftType = KNOWN_BITS<6, 5>(TKnownBits);
            ShiftResult shiftResult = evalImmShift<ShiftType>(mRegisters.r[Rm], Is);
            armALU<TKnownBits>(shiftResult.value, shiftResult.cf);
        }

        template <uint32_t TKnownBits>
        void armDataProcRegReg()
        {
            if (!conditionFlagsPassed()) return;
            uint32_t Rm = BITS<3, 0>(mOpcode);
            uint32_t Rs = BITS<11, 8>(mOpcode);
            constexpr uint32_t ShiftType = KNOWN_BITS<6, 5>(TKnownBits);
            ShiftResult shiftResult = evalRegShift<ShiftType>(mRegisters.r[Rm], mRegisters.r[Rs]);
            armALU<TKnownBits>(shiftResult.value, shiftResult.cf);
        }

        template <uint32_t TKnownBits>
        void armDataProcImm()
        {
            if (!conditionFlagsPassed()) return;
            uint32_t Is = BITS<11, 8>(mOpcode);
            uint32_t Immediate = BITS<7, 0>(mOpcode);
            auto value = evalRORImm32(Immediate, Is);
            armALU<TKnownBits>(value, mRegisters.flag_c);
        }

        template <uint32_t TKnownBits>
        void armALU()
        {
            constexpr bool I = KNOWN_BIT<25>(TKnownBits);
            if (I == 0)
            {
                constexpr bool R = KNOWN_BIT<4>(TKnownBits);
                if (R == 0)
                    armDataProcRegImm<TKnownBits>();
                else
                    armDataProcRegReg<TKnownBits>();
            }
            else
            {
                armDataProcImm<TKnownBits>();
            }
        }

        uint32_t addWithCarry(uint32_t a, uint32_t b, uint32_t c, bool& carry_out, bool& overflow_out)
        {
            uint64_t result_unsigned = static_cast<uint64_t>(a) + static_cast<uint64_t>(b) + static_cast<uint64_t>(c);
            int64_t result_signed = static_cast<int64_t>(static_cast<int32_t>(a)) + static_cast<int64_t>(static_cast<int32_t>(b)) + static_cast<int64_t>(c);
            uint64_t result = result_unsigned & 0xffffffff;
            carry_out = result != result_unsigned;
            overflow_out = static_cast<int64_t>(static_cast<int32_t>(result)) != result_signed;
            return static_cast<uint32_t>(result);
        }

        template <bool saveResult, bool S>
        void armLogic(uint32_t& rd, uint32_t result, uint32_t cf)
        {
            if (S)
            {
                mRegisters.flag_n = BIT<31>(result);
                mRegisters.flag_z = result == 0;
                mRegisters.flag_c = cf == 0;
            }
            if (saveResult)
                rd = result;
        }

        template <bool saveResult, bool S>
        void armArithmetic(uint32_t& rd, uint32_t a, uint32_t b, uint32_t c)
        {
            bool carry, overflow;
            uint32_t result = addWithCarry(a, b, c, carry, overflow);
            if (S)
            {
                mRegisters.flag_n = BIT<31>(result);
                mRegisters.flag_z = result == 0;
                mRegisters.flag_c = carry;
                mRegisters.flag_v = overflow;
            }
            if (saveResult)
                rd = result;
        }

        template <uint32_t TKnownBits>
        void armALU(uint32_t op2, uint32_t cf)
        {
            constexpr uint32_t Opcode = KNOWN_BITS<24, 21>(TKnownBits);
            constexpr bool S = KNOWN_BIT<20>(TKnownBits);
            uint32_t Rn = BITS<19, 16>(mOpcode);
            uint32_t Rd = BITS<15, 12>(mOpcode);
            uint32_t& rd = mRegisters.r[Rd];
            uint32_t rn = mRegisters.r[Rn];

            if (Opcode == 0x0) return armLogic<true, S>(rd, rn & op2, cf);
            if (Opcode == 0x1) return armLogic<true, S>(rd, rn ^ op2, cf);
            if (Opcode == 0x2) return armArithmetic<true, S>(rd, rn, ~op2, 1);
            if (Opcode == 0x3) return armArithmetic<true, S>(rd, ~rn, op2, 1);
            if (Opcode == 0x4) return armArithmetic<true, S>(rd, rn, op2, 0);
            if (Opcode == 0x5) return armArithmetic<true, S>(rd, rn, op2, cf);
            if (Opcode == 0x6) return armArithmetic<true, S>(rd, rn, ~op2, cf);
            if (Opcode == 0x7) return armArithmetic<true, S>(rd, ~rn, op2, cf);
            if (Opcode == 0x8) return armLogic<false, true>(rd, rn & op2, cf);
            if (Opcode == 0x9) return armLogic<false, true>(rd, rn ^ op2, cf);
            if (Opcode == 0xa) return armArithmetic<false, true>(rd, rn, ~op2, 1);
            if (Opcode == 0xb) return armArithmetic<false, true>(rd, rn, op2, 0);
            if (Opcode == 0xc) return armLogic<true, S>(rd, rn | op2, cf);
            if (Opcode == 0xd) return armLogic<true, S>(rd, op2, cf);
            if (Opcode == 0xe) return armLogic<true, S>(rd, rn & ~op2, cf);
            if (Opcode == 0xf) return armLogic<true, S>(rd, ~op2, cf);

            if (Rd == 15)
            {
                EMU_NOT_IMPLEMENTED();
            }
        }

        template <uint32_t TKnownBits> void armDataProc()
        {
            armALU<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_and()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_eor()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_sub()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_rsb()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_add()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_adc()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_sbc()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_rsc()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_tst()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_teq()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_cmp()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_cmn()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_orr()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_mov()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_bic()
        {
            armDataProc<TKnownBits>();
        }

        template <uint32_t TKnownBits> void insn_mvn()
        {
            armDataProc<TKnownBits>();
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

        template <uint32_t TKnownBits>
        struct MemorySDT
        {
            MemorySDT(CpuArmInterpreter& cpu)
            {
                uint32_t opcode = cpu.mOpcode;
                uint32_t Rn = BITS<19, 16>(opcode);
                uint32_t Rd = BITS<15, 12>(opcode);

                bool I = KNOWN_BIT<25>(TKnownBits);
                bool P = KNOWN_BIT<24>(TKnownBits);
                bool U = KNOWN_BIT<23>(TKnownBits);
                bool B = KNOWN_BIT<22>(TKnownBits);

                // Offset
                uint32_t offset;
                if (I == 0)
                {
                    uint32_t Immediate = BITS<11, 0>(opcode);
                    offset = U ? Immediate : (0 - Immediate);
                }
                else
                {
                    uint32_t Is = BITS<11, 7>(opcode);
                    constexpr uint32_t ShiftType = KNOWN_BITS<6, 5>(TKnownBits);
                    uint32_t Rm = BITS<3, 0>(opcode);
                    offset = cpu.evalImmShift<ShiftType>(cpu.getRegister(Rm), Is).value;
                }

                // Pre-increment
                uint32_t address = cpu.getRegister(Rn);
                if (P == 1)
                {
                    address += offset;
                    bool W = KNOWN_BIT<21>(TKnownBits);
                    if (W)
                        cpu.setRegister(Rn, address);
                }
                else
                {
                    // TODO: Force non priviledged access
                    bool T = KNOWN_BIT<21>(TKnownBits);
                    if (T)
                    {
                        EMU_NOT_IMPLEMENTED();
                    }
                }

                // Memory access
                bool L = KNOWN_BIT<20>(TKnownBits);
                if (L == 0)
                {
                    // Store
                    if (B)
                    {
                        // STRB
                        cpu.write8(address ^ MEM_ACCESS_ENDIAN_8, static_cast<uint8_t>(Rd));
                    }
                    else
                    {
                        // STR
                        cpu.write32(address, Rd);
                    }
                }
                else
                {
                    // Load
                    if (B)
                    {
                        // LDRB
                        Rd = static_cast<uint32_t>(cpu.read8(address ^ MEM_ACCESS_ENDIAN_8));
                    }
                    else
                    {
                        // LDR
                        Rd = cpu.read32(address);
                    }
                }

                // Post-increment
                if (P == 0)
                {
                    // TODO: Restore priviledge access
                    address += offset;
                    cpu.setRegister(Rn, address);
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


        template <uint32_t TKnownBits> void insn_str()
        {
            if (!conditionFlagsPassed()) return;
            MemorySDT<TKnownBits> memory(*this);
        }

        template <uint32_t TKnownBits> void insn_ldr()
        {
            if (!conditionFlagsPassed()) return;
            MemorySDT<TKnownBits> memory(*this);
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