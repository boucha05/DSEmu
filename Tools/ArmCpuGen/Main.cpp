#include <cassert>
#include <cstdio>
#include <cstdint>
#include <unordered_set>
#include <vector>

/*
ARM Binary Opcode Format
|..3 ..................2 ..................1 ..................0|
|1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
|_Cond__|0_0_0|___Op__|S|__Rn___|__Rd___|__Shift__|Typ|0|__Rm___| DataProc
|_Cond__|0_0_0|___Op__|S|__Rn___|__Rd___|__Rs___|0|Typ|1|__Rm___| DataProc
|_Cond__|0_0_1|___Op__|S|__Rn___|__Rd___|_Shift_|___Immediate___| DataProc
|_Cond__|0_0_1_1_0|P|1|0|_Field_|__Rd___|_Shift_|___Immediate___| PSR Imm
|_Cond__|0_0_0_1_0|P|L|0|_Field_|__Rd___|0_0_0_0|0_0_0_0|__Rm___| PSR Reg
|_Cond__|0_0_0_1_0_0_1_0_1_1_1_1_1_1_1_1_1_1_1_1|0_0|L|1|__Rn___| BX,BLX
|1_1_1_0|0_0_0_1_0_0_1_0|_____immediate_________|0_1_1_1|_immed_| BKPT ARM9
|_Cond__|0_0_0_1_0_1_1_0_1_1_1_1|__Rd___|1_1_1_1|0_0_0_1|__Rm___| CLZ  ARM9
|_Cond__|0_0_0_1_0|Op_|0|__Rn___|__Rd___|0_0_0_0|0_1_0_1|__Rm___| QALU ARM9
|_Cond__|0_0_0_0_0_0|A|S|__Rd___|__Rn___|__Rs___|1_0_0_1|__Rm___| Multiply
|_Cond__|0_0_0_0_1|U|A|S|_RdHi__|_RdLo__|__Rs___|1_0_0_1|__Rm___| MulLong
|_Cond__|0_0_0_1_0|Op_|0|Rd/RdHi|Rn/RdLo|__Rs___|1|y|x|0|__Rm___| MulHalfARM9
|_Cond__|0_0_0_1_0|B|0_0|__Rn___|__Rd___|0_0_0_0|1_0_0_1|__Rm___| TransSwp12
|_Cond__|0_0_0|P|U|0|W|L|__Rn___|__Rd___|0_0_0_0|1|S|H|1|__Rm___| TransReg10
|_Cond__|0_0_0|P|U|1|W|L|__Rn___|__Rd___|OffsetH|1|S|H|1|OffsetL| TransImm10
|_Cond__|0_1_0|P|U|B|W|L|__Rn___|__Rd___|_________Offset________| TransImm9
|_Cond__|0_1_1|P|U|B|W|L|__Rn___|__Rd___|__Shift__|Typ|0|__Rm___| TransReg9
|_Cond__|0_1_1|________________xxx____________________|1|__xxx__| Undefined
|_Cond__|1_0_0|P|U|S|W|L|__Rn___|__________Register_List________| BlockTrans
|_Cond__|1_0_1|L|___________________Offset______________________| B,BL,BLX
|_Cond__|1_1_0|P|U|N|W|L|__Rn___|__CRd__|__CP#__|____Offset_____| CoDataTrans
|_Cond__|1_1_0_0_0_1_0|L|__Rn___|__Rd___|__CP#__|_CPopc_|__CRm__| CoRR ARM9
|_Cond__|1_1_1_0|_CPopc_|__CRn__|__CRd__|__CP#__|_CP__|0|__CRm__| CoDataOp
|_Cond__|1_1_1_0|CPopc|L|__CRn__|__Rd___|__CP#__|_CP__|1|__CRm__| CoRegTrans
|_Cond__|1_1_1_1|_____________Ignored_by_Processor______________| SWI
*/

/*
Code Suffix Flags         Meaning
0:   EQ     Z=1           equal (zero) (same)
1:   NE     Z=0           not equal (nonzero) (not same)
2:   CS/HS  C=1           unsigned higher or same (carry set)
3:   CC/LO  C=0           unsigned lower (carry cleared)
4:   MI     N=1           negative (minus)
5:   PL     N=0           positive or zero (plus)
6:   VS     V=1           overflow (V set)
7:   VC     V=0           no overflow (V cleared)
8:   HI     C=1 and Z=0   unsigned higher
9:   LS     C=0 or Z=1    unsigned lower or same
A:   GE     N=V           greater or equal
B:   LT     N<>V          less than
C:   GT     Z=0 and N=V   greater than
D:   LE     Z=1 or N<>V   less or equal
E:   AL     -             always (the "AL" suffix can be omitted)
F:   NV     -             never (ARMv1,v2 only) (Reserved ARMv3 and up)
*/
enum CONDITION
{
    CONDITION_EQ,
    CONDITION_NE,
    CONDITION_CS,
    CONDITION_CC,
    CONDITION_MI,
    CONDITION_PL,
    CONDITION_VS,
    CONDITION_VC,
    CONDITION_HI,
    CONDITION_LS,
    CONDITION_GE,
    CONDITION_LT,
    CONDITION_GT,
    CONDITION_LE,
    CONDITION_AL,
    CONDITION_NV,
};

static const char* conditionName[] =
{
    "EQ",   "NE",   "CS",   "CC",
    "MI",   "PL",   "VS",   "VC",
    "HI",   "LS",   "GE",   "LT",
    "GT",   "LE",   "AL",   "NV",
};

static const char* opcodeALU[] =
{
    "AND",  "EOR",  "SUB",  "RSB",
    "ADD",  "ADC",  "SBC",  "RSC",
    "TST",  "TEQ",  "CMP",  "CMN",
    "ORR",  "MOV",  "BIC",  "MVN",
};

static const char* shiftTypeName[] =
{
    "_LSL", "_LSR", "_ASR", "_ROR",
};

enum class ARM : uint32_t
{
    ARM7TDMI,
    ARM946ES,
};

class Generator
{
public:
    Generator(ARM version)
        : ARMv4(false)
        , ARMv5(false)
        , ARMv5TE(false)
    {
        switch (version)
        {
        case ARM::ARM7TDMI: ARMv4 = true; break;
        case ARM::ARM946ES: ARMv4 = true; ARMv5 = true; ARMv5TE = true; break;
        }
    }

    void setInstruction(uint32_t opcode, const std::string& insn, const std::string& variant)
    {
        mInsn.add(insn);
        mVariant.add(variant);
        if (mInsnTable[opcode] != "OP_UND")
        {
            printf("Can't assign %s to entry 0x%03x, %s already defined\n", variant.c_str(), opcode, mInsnTable[opcode].c_str());
            assert(false);
        }
        mInsnTable[opcode] = variant;
    }

    void genOpcodes_DataProcShift()
    {
        for (uint32_t op = 0; op < 16; ++op)
        {
            for (uint32_t s = 0; s < 2; ++s)
            {
                if (!s && (op >= 0x08) && (op <= 0x0b))
                    continue;

                for (uint32_t type = 0; type < 4; ++type)
                {
                    std::string insn = opcodeALU[op];
                    if (s)
                        insn += "S";

                    std::string variant = opcodeALU[op];
                    if (s)
                        variant += "_S";
                    variant = "OP_" + variant + shiftTypeName[type] + "_IMM";

                    for (uint32_t shift = 0; shift < 2; ++shift)
                    {
                        uint32_t opcode = 0x000 | (op << 5) | (s << 4) | (type << 1) | (shift << 3);
                        setInstruction(opcode, insn, variant);
                    }
                }
            }
        }
    }

    void genOpcodes_DataProcReg()
    {
        for (uint32_t op = 0; op < 16; ++op)
        {
            for (uint32_t s = 0; s < 2; ++s)
            {
                if (!s && (op >= 0x08) && (op <= 0x0b))
                    continue;

                for (uint32_t type = 0; type < 4; ++type)
                {
                    std::string insn = opcodeALU[op];
                    if (s)
                        insn += "S";

                    std::string variant = opcodeALU[op];
                    if (s)
                        variant += "_S";
                    variant = "OP_" + variant + shiftTypeName[type] + "_REG";

                    uint32_t opcode = 0x001 | (op << 5) | (s << 4) | (type << 1);
                    setInstruction(opcode, insn, variant);
                }
            }
        }
    }

    void genOpcodes_DataProcImm()
    {
        for (uint32_t op = 0; op < 16; ++op)
        {
            for (uint32_t s = 0; s < 2; ++s)
            {
                if (!s && (op >= 0x08) && (op <= 0x0b))
                    continue;

                std::string insn = opcodeALU[op];
                if (s)
                    insn += "S";

                std::string variant = opcodeALU[op];
                if (s)
                    variant += "_S";
                variant = "OP_" + variant + "_IMM_VAL";

                for (uint32_t imm = 0; imm < 16; ++imm)
                {
                    uint32_t opcode = 0x200 | (op << 5) | (s << 4) | imm;
                    setInstruction(opcode, insn, variant);
                }
            }
        }
    }

    void genOpcodes_PSRImm()
    {
        for (uint32_t i = 0; i < 2; ++i)
        {
            for (uint32_t psr = 0; psr < 2; ++psr)
            {
                for (uint32_t op = 0; op < 2; ++op)
                {
                    for (uint32_t imm = 0; imm < 16; ++imm)
                    {
                        if (!i && imm)
                            continue;

                        std::string insn = opcodeALU[op];
                        insn = op ? "MSR" : "MRS";

                        std::string variant = "OP_";
                        variant += insn;
                        variant += psr ? "_SPSR" : "_CPSR";
                        variant += i ? "?I" : "";

                        uint32_t opcode = 0x100 | (i << 9) | (psr << 6) | (op << 5) | imm;
                        setInstruction(opcode, insn, variant);
                    }
                }
            }
        }
    }

    void genOpcodes_PSRReg()
    {
        //genOpcodes_PSRImm();
    }

    void genOpcodes_BX_BLX()
    {
    }

    void genOpcodes_BKPT()
    {
    }

    void genOpcodes_CLZ()
    {
    }

    void genOpcodes_QALU()
    {
        static const char* opInsn[4] =
        {
            "QADD", "QSUB", "QDADD", "QDSUB"
        };
        for (uint32_t op = 0; op < 4; ++op)
        {
            std::string insn = opInsn[op];
            std::string variant = "OP_" + insn;

            uint32_t opcode = 0x105 | (op << 5);
            setInstruction(opcode, insn, variant);
        }
    }

    void genOpcodes_Multiply()
    {
        static const char* opInsn[16] =
        {
            "MUL", "MLA", nullptr, nullptr,
            "UMULL", "UMLAL", "SMULL", "SMLAL",
            "SMLA", "", "SMLAL", "SMUL",
            nullptr, nullptr, nullptr, nullptr
        };
        for (uint32_t op = 0; op < 16; ++op)
        {
            if (!opInsn[op])
                continue;
            for (uint32_t s = 0; s < 2; ++s)
            {
                for (uint32_t y = 0; y < 2; ++y)
                {
                    for (uint32_t x = 0; x < 2; ++x)
                    {
                        std::string insn = opInsn[op];
                        std::string variant = "OP_" + insn;

                        uint32_t operand = 0;
                        if (op & 8)
                        {
                            if (s)
                                continue;
                            if (op == 9)
                            {
                                insn = x ? "SMULW" : "SMLAW";
                                variant = "OP_" + insn;
                            }
                            else
                            {
                                insn += x ? "T" : "B";
                                variant += x ? "_T" : "_B";
                            }
                            insn += y ? "T" : "B";
                            variant += y ? "_T" : "_B";
                            operand = 0x008 | (y << 2) | (x << 1);
                        }
                        else
                        {
                            operand = 0x009;
                            if (y || x)
                                continue;
                        }

                        if (s)
                        {
                            insn += "S";
                            variant += "_S";
                        }

                        uint32_t opcode = 0x000 | (op << 5) | (s << 4) | operand;
                        setInstruction(opcode, insn, variant);
                    }
                }
            }
        }
    }

    void genOpcodes_MulLong()
    {
        //genOpcodes_Multiply();
    }

    void genOpcodes_MulHalf()
    {
        //genOpcodes_Multiply();
    }

    void genOpcodes_TransSwp12()
    {
        for (uint32_t b = 0; b < 2; ++b)
        {
            std::string insn = "SWP";
            std::string variant = "OP_" + insn;
            insn += b ? "B" : "";
            variant += b ? "_B" : "";

            uint32_t opcode = 0x109 | (b << 10);
            setInstruction(opcode, insn, variant);
        }
    }

    void genOpcodes_TransReg10()
    {
        static const char* opInsn[2][4] =
        {
            { "", "STR", "LDR", "STR" },
            { "", "LDR", "LDR", "LDR" },
        };
        static const char* opSize[2][4] =
        {
            { "", "H", "D", "D" },
            { "", "H", "SB", "SH" },
        };

        for (uint32_t p = 0; p < 2; ++p)
        {
            for (uint32_t u = 0; u < 2; ++u)
            {
                for (uint32_t i = 0; i < 2; ++i)
                {
                    for (uint32_t w = 0; w < 2; ++w)
                    {
                        if ((p == 0) && (w != 0))
                            continue;

                        for (uint32_t l = 0; l < 2; ++l)
                        {
                            for (uint32_t op = 0; op < 4; ++op)
                            {
                                if (op == 0)
                                    continue;

                                std::string insn;
                                insn = opInsn[l][op];
                                insn += opSize[l][op];
                                insn += w ? "W" : "";

                                std::string variant = "OP_";
                                if (((op & 2) != 0) && !l)
                                {
                                    variant += "LDR";
                                    variant += opSize[l][op];
                                    variant += w ? "?W" : "";
                                    variant += "_STR";
                                    variant += opSize[l][op];
                                    variant += w ? "?W" : "";
                                    //variant += u ? "?U" : "";
                                    //variant += i ? "?i" : "";
                                    variant += p ? "_PRE_INDEX" : "_POST_INDEX";
                                }
                                else
                                {
                                    variant += opInsn[l][op];
                                    variant += opSize[l][op];
                                    variant += w ? "_W" : "";
                                    variant += p ? "_PRE_INDE" : "_POS_INDE";
                                    variant += u ? "_P" : "_M";
                                    variant += i ? "_IMM_OFF" : "_REG_OFF";
                                }

                                uint32_t opcode = 0x009 | (p << 8) | (u << 7) | (i << 6) | (w << 5) | (l << 4) | (op << 1);
                                setInstruction(opcode, insn, variant);
                            }
                        }
                    }
                }
            }
        }
    }

    void genOpcodes_TransImm10()
    {
        //genOpcodes_TransReg10();
    }

    void genOpcodes_TransImm9()
    {
    }

    void genOpcodes_TransReg9()
    {
    }

    void genOpcodes_Undefined()
    {
    }

    void genOpcodes_BlockTrans()
    {
        for (uint32_t p = 0; p < 2; ++p)
        {
            for (uint32_t u = 0; u < 2; ++u)
            {
                for (uint32_t s = 0; s < 2; ++s)
                {
                    for (uint32_t w = 0; w < 2; ++w)
                    {
                        for (uint32_t l = 0; l < 2; ++l)
                        {
                            for (uint32_t imm = 0; imm < 16; ++imm)
                            {
                                std::string insn = l ? "LDM" : "STM";
                                insn += u ? "I" : "D";
                                insn += p ? "B" : "A";
                                insn += s ? "2" : "";

                                std::string variant = "OP_" + insn;
                                if (w)
                                {
                                    insn += "W";
                                    variant += "_W";
                                }

                                uint32_t opcode = 0x800 | (p << 8) | (u << 7) | (s << 6) | (w << 5) | (l << 4) | imm;
                                setInstruction(opcode, insn, variant);
                            }
                        }
                    }
                }
            }
        }
    }

    void genOpcodes_B_BL_BLX()
    {
        for (uint32_t l = 0; l < 2; ++l)
        {
            for (uint32_t imm = 0; imm < 256; ++imm)
            {
                std::string insn = "B";
                insn += l ? "L" : "";
                std::string variant = "OP_" + insn;
                uint32_t opcode = 0xa00 | (l << 8) | imm;
                setInstruction(opcode, insn, variant);
            }
        }
    }

    void genOpcodes_CoDataTrans()
    {
        for (uint32_t p = 0; p < 2; ++p)
        {
            for (uint32_t u = 0; u < 2; ++u)
            {
                for (uint32_t n = 0; n < 2; ++n)
                {
                    for (uint32_t w = 0; w < 2; ++w)
                    {
                        for (uint32_t l = 0; l < 2; ++l)
                        {
                            for (uint32_t imm = 0; imm < 16; ++imm)
                            {
                                std::string insn = l ? "LDC" : "STC";
                                std::string variant = "OP_" + insn;

                                insn += w ? "W" : "";
                                insn += ARMv5 ? "2" : "";
                                if (!w && !p)
                                {
                                    variant += "_OPTION";
                                }
                                else
                                {
                                    variant += u ? "_P" : "_M";
                                    //variant += "_P";
                                    variant += w ? (p ? "_PREIND" : "_POSTIND") : "_IMM_OFF";
                                }

                                uint32_t opcode = 0xc00 | (p << 8) | (u << 7) | (n << 6) | (w << 5) | (l << 4) | imm;
                                setInstruction(opcode, insn, variant);
                            }
                        }
                    }
                }
            }
        }
    }

    void genOpcodes_CoRR()
    {
    }

    void genOpcodes_CoDataOp()
    {
        for (uint32_t cpopc = 0; cpopc < 16; ++cpopc)
        {
            for (uint32_t cp = 0; cp < 8; ++cp)
            {
                for (uint32_t trans = 0; trans < 2; ++trans)
                {
                    std::string insn;
                    if (trans)
                        insn = (cpopc & 1) ? "MRC" : "MCR";
                    else
                        insn = "CDP";

                    std::string variant = "OP_" + insn;
                    insn += ARMv5 ? "2" : "";

                    uint32_t opcode = 0xe00 | (cpopc << 4) | (cp << 1) | trans;
                    setInstruction(opcode, insn, variant);
                }
            }
        }
    }

    void genOpcodes_CoRegTrans()
    {
        //genOpcodes_CoDataOp();
    }

    void genOpcodes_SWI()
    {
        for (uint32_t ignored = 0; ignored < 256; ++ignored)
        {
            std::string insn = "SWI";
            std::string variant = "OP_" + insn;
            uint32_t opcode = 0xf00 | ignored;
            setInstruction(opcode, insn, variant);
        }
    }

    void genOpcodes()
    {
        mInsn.add("UND");
        mVariant.add("OP_UND");
        mInsnTable.resize(4096, "OP_UND");

#define ENCODING(expr)  genOpcodes_##expr()
#include "ARMInstructions.h"    
#undef ENCODING

        printf("Instructions:\n");
        mInsn.dump();
        printf("\nVariants:\n");
        mVariant.dump();
    }

    void generate()
    {
        genOpcodes();
    }

    const std::vector<std::string>& getInstructions() const
    {
        return mInsn.list;
    }

    const std::vector<std::string>& getVariants() const
    {
        return mVariant.list;
    }

    const std::vector<std::string>& getInsnTable() const
    {
        return mInsnTable;
    }

private:
    struct Dictionary
    {
        std::unordered_set<std::string>     names;
        std::vector<std::string>            list;

        void add(const std::string& name)
        {
            if (!names.count(name))
            {
                names.insert(name);
                list.push_back(name);
            }
        }

        void dump()
        {
            for (size_t index = 0; index < list.size(); ++index)
            {
                printf("%3zd = %s\n", index, list[index].c_str());
            }
        }
    };

    bool                        ARMv4;
    bool                        ARMv5;
    bool                        ARMv5TE;
    Dictionary                  mInsn;
    Dictionary                  mVariant;
    std::vector<std::string>    mInsnTable;
};

bool assertSame(const std::vector<std::string>& expected, const std::vector<std::string>& result)
{
    if (expected.size() != result.size())
    {
        printf("Table size mismatch, expected %zd, got %zd\n", expected.size(), result.size());
        return false;
    }
    uint32_t differences = 0;
    for (size_t index = 0; index < expected.size(); ++index)
    {
        if (expected[index] != result[index])
        {
            printf("Table entry 0x%zx (%zd) mismatch, expected %s, got %s\n", index, index, expected[index].c_str(), result[index].c_str());
            ++differences;
        }
    }
    if (differences)
    {
        printf("%d differences found!\n", differences);
    }
    return !differences;
}

int main()
{
    //Generator generatorARM7(ARM::ARM7TDMI);
    //generatorARM7.generate();

    Generator generatorARM9(ARM::ARM946ES);
    generatorARM9.generate();

    static const std::vector<std::string> expectedInsnTable =
    {
#define TABDECL(insn)  std::string(#insn)
#include "D:\OpenSource\desmume\desmume\src\instruction_tabdef.inc"
#undef TABDECL
    };

    assertSame(expectedInsnTable, generatorARM9.getInsnTable());

    return 0;
}