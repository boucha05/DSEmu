#include <cassert>
#include <cstdio>
#include <cstdint>
#include <unordered_set>
#include <vector>

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

    void genOpcodes_BX_BLX()
    {
        setInstruction(0x121, "BX", "OP_BX");
        setInstruction(0x123, "BLX", "OP_BLX_REG");
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

    void genOpcodes_BKPT()
    {
        if (ARMv5)
        {
            setInstruction(0x127, "BLX", "OP_BKPT");
        }
    }

    void genOpcodes_DataProcShift()
    {
        for (uint32_t i = 0; i < 2; ++i)
        {
            for (uint32_t op = 0; op < 16; ++op)
            {
                for (uint32_t s = 0; s < 2; ++s)
                {
                    for (uint32_t imm = 0; imm < 16; ++imm)
                    {
                        uint32_t r = imm & 1;
                        uint32_t type = (imm >> 1) & 3;
                        bool test = (op >= 0x8) && (op <= 0xb);

                        if (!i && r && (imm & 8))
                            continue;
                        if (!s && test)
                            continue;

                        static const char* opcodeALU[] =
                        {
                            "AND",  "EOR",  "SUB",  "RSB",
                            "ADD",  "ADC",  "SBC",  "RSC",
                            "TST",  "TEQ",  "CMP",  "CMN",
                            "ORR",  "MOV",  "BIC",  "MVN",
                        };

                        std::string insn = opcodeALU[op];
                        std::string variant = "OP_" + insn;
                        insn += s ? "S" : "";
                        variant += (s && !test) ? "_S" : "";
                        variant += !i ? shiftTypeName[type] : "";
                        variant += (!i && r) ? "_REG" : "_IMM";
                        variant += i ? "_VAL" : "";

                        uint32_t opcode = 0x000 | (i << 9) | (op << 5) | (s << 4) | imm;
                        setInstruction(opcode, insn, variant);
                    }
                }
            }
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

    void genOpcodes_CLZ()
    {
        if (ARMv5)
        {
            setInstruction(0x161, "CLZ", "OP_CLZ");
        }
    }

    void genOpcodes_QALU()
    {
        if (ARMv5)
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
                        if (!op && (imm || i))
                            continue;
                        if (!i && imm)
                            continue;

                        std::string insn = op ? "MSR" : "MRS";

                        std::string variant = "OP_";
                        variant += insn;
                        variant += psr ? "_SPSR" : "_CPSR";
                        variant += i ? "_IMM_VAL" : "";

                        uint32_t opcode = 0x100 | (i << 9) | (psr << 6) | (op << 5) | imm;
                        setInstruction(opcode, insn, variant);
                    }
                }
            }
        }
    }

    void genOpcodes_TransImm9()
    {
        for (uint32_t i = 0; i < 2; ++i)
        {
            for (uint32_t p = 0; p < 2; ++p)
            {
                for (uint32_t u = 0; u < 2; ++u)
                {
                    for (uint32_t b = 0; b < 2; ++b)
                    {
                        for (uint32_t w = 0; w < 2; ++w)
                        {
                            for (uint32_t l = 0; l < 2; ++l)
                            {
                                for (uint32_t imm = 0; imm < 16; ++imm)
                                {
                                    if (i && (imm & 1))
                                        continue;
                                    std::string insn = l ? "LDR" : "STR";
                                    insn += b ? "B" : "";

                                    std::string variant = "OP_" + insn;
                                    variant += u ? "_P" : "_M";
                                    variant += i ? shiftTypeName[(imm >> 1) & 3] : "";
                                    variant += "_IMM_OFF";
                                    variant += p ? (w ? "_PREIND" : "") : "_POSTIND";

                                    uint32_t opcode = 0x400 | (i << 9) | (p << 8) | (u << 7) | (b << 6) | (w << 5) | (l << 4) | imm;
                                    setInstruction(opcode, insn, variant);
                                }
                            }
                        }
                    }
                }
            }
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
                        for (uint32_t l = 0; l < 2; ++l)
                        {
                            for (uint32_t op = 0; op < 4; ++op)
                            {
                                if (!p && w)
                                    continue;
                                if (!op)
                                    continue;

                                std::string insn = opInsn[l][op];
                                insn += opSize[l][op];

                                std::string variant = "OP_" + insn;
                                insn += w ? "W" : "";
                                if ((op & 2) && !l)
                                {
                                    variant = "OP_LDR";
                                    variant += opSize[l][op];
                                    variant += "_STR";
                                    variant += opSize[l][op];
                                    variant += p ? "_OFFSET_PRE_INDEX" : "_POST_INDEX";
                                }
                                else
                                {
                                    variant += p ? (w ? "_PRE_INDE" : "") : "_POS_INDE";
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

    void genOpcodes_TransSwp12()
    {
        setInstruction(0x109, "SWP", "OP_SWP");
        setInstruction(0x149, "SWPB", "OP_SWPB");
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

    void genOpcodes_Undocumented()
    {
        // Special undocumented instructions emulated by DeSmuME
        setInstruction(0x189, "STREX", "OP_STREX");
        setInstruction(0x199, "LDREX", "OP_LDREX");
    }

    void genOpcodes()
    {
        mInsn.add("UND");
        mVariant.add("OP_UND");
        mInsnTable.resize(4096, "OP_UND");

        genOpcodes_B_BL_BLX();
        genOpcodes_BX_BLX();
        genOpcodes_SWI();
        genOpcodes_BKPT();
        genOpcodes_DataProcShift();
        genOpcodes_Multiply();
        genOpcodes_CLZ();
        genOpcodes_QALU();
        genOpcodes_PSRImm();
        genOpcodes_TransImm9();
        genOpcodes_TransReg10();
        genOpcodes_BlockTrans();
        genOpcodes_TransSwp12();
        genOpcodes_CoDataTrans();
        genOpcodes_CoDataOp();
        genOpcodes_Undocumented();

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