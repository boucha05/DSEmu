#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <unordered_set>
#include <vector>

namespace
{
    void toUpper(std::string& value)
    {
        std::transform(value.begin(), value.end(), value.begin(), ::toupper);
    }

    void toLower(std::string& value)
    {
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    }

    void format(std::string& dest, const char* format, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        int size = vsprintf(buffer, format, args);
        va_end(args);
        dest = std::string(buffer, size);
    }

    class FileWriter
    {
    public:
        FileWriter(const std::string& filename)
            : mPath("..\\..\\DSEmu\\" + filename)
            , mFile(fopen(mPath.c_str(), "w"))
        {
            auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            printf("// %s generated automatically at %s", filename.c_str(), std::ctime(&time));
        }

        ~FileWriter()
        {
            fclose(mFile);
        }

        FILE* file() const
        {
            return mFile;
        }

        void printf(const char* format, ...)
        {
            if (mFile)
            {
                va_list args;
                va_start(args, format);
                vfprintf(mFile, format, args);
                va_end(args);
            }
        }

    private:
        std::string     mPath;
        FILE*           mFile;
    };

    class CompactListWriter
    {
    public:
        CompactListWriter(FileWriter& file, uint32_t spacing, uint32_t rowCapacity)
            : mFile(file)
            , mSpacing(spacing)
            , mRowCapacity(rowCapacity)
            , mRowCount(0)
        {
        }

        ~CompactListWriter()
        {
            flush();
        }

        void append(const std::string& text)
        {
            if (!mLine.empty())
            {
                mLine.append(", ");
                auto padding = mLine.size() % mSpacing;
                if (padding)
                    padding = mSpacing - padding;
                mLine.append(std::string(padding, ' '));
            }
            mLine.append(text);
            if (++mRowCount >= mRowCapacity)
                flush();
        }

        void flush()
        {
            if (!mLine.empty())
            {
                mFile.printf("    %s,\n", mLine.c_str());
                mLine.clear();
                mRowCount = 0;
            }
        }

    private:
        FileWriter&     mFile;
        uint32_t        mSpacing;
        uint32_t        mRowCapacity;
        uint32_t        mRowCount;
        std::string     mLine;
    };
}

namespace ARM
{
    static const char* shiftTypeName[] =
    {
        "_LSL", "_LSR", "_ASR", "_ROR",
    };

    enum class Family : uint32_t
    {
        ARM7TDMI,
        ARM946ES,
    };

    struct Dictionary
    {
        std::unordered_set<std::string>     names;
        std::vector<std::string>            list;

        bool add(const std::string& name)
        {
            bool added = !names.count(name);
            if (added)
            {
                names.insert(name);
                list.push_back(name);
            }
            return added;
        }

        void dump()
        {
            for (size_t index = 0; index < list.size(); ++index)
            {
                printf("%3zd = %s\n", index, list[index].c_str());
            }
        }
    };

    class Shared
    {
    public:
        Shared()
        {
            mInsn.add("INVALID");
            mAddrEnum.add("Invalid");
            mAddr.add("Invalid");
            mVariant.add("OP_UND");
            mName.push_back("???");
            mSuffix.push_back("");
        }

        void addInstruction(std::string name, std::string suffix, const std::string& addr, const std::string& variant)
        {
            bool added = mInsn.add(name + suffix);
            mAddrEnum.add(addr.substr(0, addr.find_last_of('<')));
            mAddr.add(addr);
            mVariant.add(variant);
            if (added)
            {
                toLower(name);
                toLower(suffix);
                mName.push_back(name);
                mSuffix.push_back(suffix);
            }
        }

        auto& getInsnSet()
        {
            return mInsn;
        }

        auto& getAddrEnumSet()
        {
            return mAddrEnum;
        }

        auto& getAddrSet()
        {
            return mAddr;
        }

        auto& getVariantSet()
        {
            return mVariant;
        }

        auto& getNameTable()
        {
            return mName;
        }

        auto& getSuffixTable()
        {
            return mSuffix;
        }

        void exportSymbols(const char* filename)
        {
            auto file = FileWriter(filename);

            file.printf("\nenum class Insn : uint8_t\n{\n");
            {
                CompactListWriter listWriter(file, 12, 8);
                for (const auto& value : mInsn.list)
                    listWriter.append(value);
            }
            file.printf("};\n");

            file.printf("\nenum class Addr : uint8_t\n{\n");
            {
                CompactListWriter listWriter(file, 16, 4);
                for (const auto& value : mAddrEnum.list)
                    listWriter.append(value);
            }
            file.printf("};\n");
        }

        void exportTables(const char* filename)
        {
            auto file = FileWriter(filename);

            file.printf("\nstatic const char* InsnName[] =\n{\n");
            {
                CompactListWriter listWriter(file, 12, 8);
                for (const auto& value : mName)
                    listWriter.append('"' + value + '"');
            }
            file.printf("};\n");

            file.printf("\nstatic const char* InsnSuffix[] =\n{\n");
            {
                CompactListWriter listWriter(file, 12, 8);
                for (const auto& value : mSuffix)
                    listWriter.append('"' + value + '"');
            }
            file.printf("};\n");
        }

    private:
        Dictionary                  mInsn;
        Dictionary                  mAddrEnum;
        Dictionary                  mAddr;
        Dictionary                  mVariant;
        std::vector<std::string>    mName;
        std::vector<std::string>    mSuffix;

    };

    class Generator
    {
    public:
        Generator(Shared& shared, Family family)
            : mShared(shared)
            , ARMv4(false)
            , ARMv5(false)
            , ARMv5TE(false)
        {
            switch (family)
            {
            case Family::ARM7TDMI: ARMv4 = true; break;
            case Family::ARM946ES: ARMv4 = true; ARMv5 = true; ARMv5TE = true; break;
            }
        }

        struct Instruction
        {
            uint32_t        opcode;
            std::string     variant;
            std::string     name;
            std::string     suffix;
            std::string     addr;

            Instruction()
                : opcode(0xffffffff)
            {
            }

            Instruction(uint32_t _opcode, const std::string& _variant, const std::string& _name, std::string _addr)
                : opcode(_opcode)
                , variant(_variant)
                , name(_name)
                , addr(_addr)
            {
            }
        };

        void setInstruction(const Instruction& insn)
        {
            assert(!insn.addr.empty());
            std::string addr = insn.addr;
            std::string variant = "OP_" + insn.variant;

            mShared.addInstruction(insn.name, insn.suffix, addr, variant);

            if (insn.opcode > static_cast<uint32_t>(mVariantTable.size()))
            {
                printf("Invalid opcode 0x%03x, can't not exceeed 0x%03x\n", insn.opcode, static_cast<uint32_t>(mVariantTable.size() - 1));
                assert(false);
            }
            if (mVariantTable[insn.opcode] != "OP_UND")
            {
                printf("Can't assign %s to entry 0x%03x, %s already defined\n", variant.c_str(), insn.opcode, mVariantTable[insn.opcode].c_str());
                assert(false);
            }
            mInsnTable[insn.opcode] = insn.name + insn.suffix;
            mAddrTable[insn.opcode] = addr;
            mVariantTable[insn.opcode] = variant;
        }

        void genOpcodes_B_BL_BLX()
        {
            for (uint32_t l = 0; l < 2; ++l)
            {
                for (uint32_t imm = 0; imm < 256; ++imm)
                {
                    Instruction insn;
                    insn.name = "B";
                    insn.name += l ? "L" : "";
                    insn.addr = "BranchOffset";
                    insn.variant = insn.name;
                    insn.opcode = 0xa00 | (l << 8) | imm;
                    setInstruction(insn);
                }
            }
        }

        void genOpcodes_BX_BLX()
        {
            setInstruction(Instruction(0x121, "BX", "BX", "BranchReg"));
            setInstruction(Instruction(0x123, "BLX_REG", "BLX", "BranchReg"));
        }

        void genOpcodes_SWI()
        {
            for (uint32_t ignored = 0; ignored < 256; ++ignored)
            {
                Instruction insn;
                insn.name = "SWI";
                insn.addr = "SWI";
                insn.variant = insn.name;
                insn.opcode = 0xf00 | ignored;
                setInstruction(insn);
            }
        }

        void genOpcodes_BKPT()
        {
            if (ARMv5)
            {
                setInstruction(Instruction(0x127, "BKPT", "BKPT", "BKPT"));
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
                            bool unary = (op == 0xd) || (op == 0xf);

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

                            Instruction insn;
                            insn.name = opcodeALU[op];
                            insn.variant = insn.name;
                            insn.suffix = s ? "S" : "";
                            if (!i)
                                format(insn.addr, "ALUReg%s<%d,%d,%d,%d>", r ? "Reg" : "Imm", op, s, type, r);
                            else
                                format(insn.addr, "ALUImm<%d,%d>", op, s);
                            insn.variant += (s && !test) ? "_S" : "";
                            insn.variant += !i ? shiftTypeName[type] : "";
                            insn.variant += (!i && r) ? "_REG" : "_IMM";
                            insn.variant += i ? "_VAL" : "";

                            insn.opcode = 0x000 | (i << 9) | (op << 5) | (s << 4) | imm;
                            setInstruction(insn);
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
            static const char* opAddr[16] =
            {
                "MulRdRmRs", "MulRdRmRsRn", nullptr, nullptr,
                "MulRnRdRmRs", "MulRnRdRmRs", "MulRnRdRmRs", "MulRnRdRmRs",
                "MulRdRmRsRn", "", "MulRnRdRmRs", "MulRdRmRs",
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
                            Instruction insn;
                            insn.name = opInsn[op];
                            insn.addr = opAddr[op];
                            insn.variant = insn.name;

                            uint32_t operand = 0;
                            bool half = (op & 8) != 0;
                            if (half)
                            {
                                if (s)
                                    continue;
                                if (op == 9)
                                {
                                    insn.name = x ? "SMULW" : "SMLAW";
                                    insn.addr = x ? "MulRdRmRs" : "MulRdRmRsRn";
                                    insn.variant = insn.name;
                                }
                                else
                                {
                                    insn.name += x ? "T" : "B";
                                    insn.variant += x ? "_T" : "_B";
                                }
                                insn.name += y ? "T" : "B";
                                insn.variant += y ? "_T" : "_B";
                                operand = 0x008 | (y << 2) | (x << 1);
                            }
                            else
                            {
                                if (s)
                                {
                                    insn.suffix += "S";
                                    insn.variant += "_S";
                                }
                                operand = 0x009;
                                if (y || x)
                                    continue;
                            }

                            insn.opcode = 0x000 | (op << 5) | (s << 4) | operand;
                            setInstruction(insn);
                        }
                    }
                }
            }
        }

        void genOpcodes_CLZ()
        {
            if (ARMv5)
            {
                setInstruction(Instruction(0x161, "CLZ", "CLZ", "CLZ"));
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
                    Instruction insn;
                    insn.name = opInsn[op];
                    insn.addr = "QALU";
                    insn.variant = insn.name;

                    insn.opcode = 0x105 | (op << 5);
                    setInstruction(insn);
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

                            Instruction insn;
                            insn.name = op ? "MSR" : "MRS";
                            if (!op)
                                format(insn.addr, "MRS<%d>", psr);
                            else
                            {
                                if (!i)
                                    format(insn.addr, "MSRReg<%d>", psr);
                                else
                                    format(insn.addr, "MSRImm<%d>", psr);
                            }
                            insn.variant = insn.name;
                            insn.variant += psr ? "_SPSR" : "_CPSR";
                            insn.variant += i ? "_IMM_VAL" : "";

                            insn.opcode = 0x100 | (i << 9) | (psr << 6) | (op << 5) | imm;
                            setInstruction(insn);
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

                                        uint32_t type = (imm >> 1) & 3;

                                        Instruction insn;
                                        insn.name = l ? "LDR" : "STR";
                                        insn.suffix += b ? "B" : "";
                                        insn.suffix += (!p && w) ? "T" : "";
                                        if (!i)
                                            format(insn.addr, "MemImm<%d,%d,%d>", p, u, (p && w) ? 1 : 0);
                                        else
                                            format(insn.addr, "MemReg<%d,%d,%d,%d>", p, u, (p && w) ? 1 : 0, type);

                                        insn.variant = insn.name;
                                        insn.variant += b ? "B" : "";
                                        insn.variant += u ? "_P" : "_M";
                                        insn.variant += i ? shiftTypeName[type] : "";
                                        insn.variant += "_IMM_OFF";
                                        insn.variant += p ? (w ? "_PREIND" : "") : "_POSTIND";

                                        insn.opcode = 0x400 | (i << 9) | (p << 8) | (u << 7) | (b << 6) | (w << 5) | (l << 4) | imm;
                                        setInstruction(insn);
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

                                    Instruction insn;
                                    insn.name = opInsn[l][op];
                                    insn.suffix += opSize[l][op];
                                    format(insn.addr, "MemEx%s<%d,%d,%d>", !i ? "Reg" : "Imm", p, u, (p && w) ? 1 : 0);

                                    insn.variant = opInsn[l][op];
                                    insn.variant += opSize[l][op];
                                    if ((op & 2) && !l)
                                    {
                                        insn.variant = "LDR";
                                        insn.variant += opSize[l][op];
                                        insn.variant += "_STR";
                                        insn.variant += opSize[l][op];
                                        insn.variant += p ? "_OFFSET_PRE_INDEX" : "_POST_INDEX";
                                    }
                                    else
                                    {
                                        insn.variant += p ? (w ? "_PRE_INDE" : "") : "_POS_INDE";
                                        insn.variant += u ? "_P" : "_M";
                                        insn.variant += i ? "_IMM_OFF" : "_REG_OFF";
                                    }

                                    insn.opcode = 0x009 | (p << 8) | (u << 7) | (i << 6) | (w << 5) | (l << 4) | (op << 1);
                                    setInstruction(insn);
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
                                    Instruction insn;
                                    insn.name = l ? "LDM" : "STM";
                                    insn.suffix += u ? "I" : "D";
                                    insn.suffix += p ? "B" : "A";
                                    format(insn.addr, "MemBlock<%d,%d,%d,%d>", p, u, s, w);

                                    insn.variant = l ? "LDM" : "STM";
                                    insn.variant += u ? "I" : "D";
                                    insn.variant += p ? "B" : "A";
                                    insn.variant += s ? "2" : "";
                                    insn.variant += w ? "_W" : "";

                                    insn.opcode = 0x800 | (p << 8) | (u << 7) | (s << 6) | (w << 5) | (l << 4) | imm;
                                    setInstruction(insn);
                                }
                            }
                        }
                    }
                }
            }
        }

        void genOpcodes_TransSwp12()
        {
            setInstruction(Instruction(0x109, "SWP", "SWP", "SWP"));
            setInstruction(Instruction(0x149, "SWPB", "SWPB", "SWP"));
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
                                    Instruction insn;
                                    insn.name = l ? "LDC" : "STC";
                                    insn.name += ARMv5 ? "2" : "";
                                    insn.suffix = n ? "L" : "";
                                    insn.variant = l ? "LDC" : "STC";
                                    format(insn.addr, "CoDataTrans%s<%d,%d,%d,%d>", ARMv5 ? "2" : "", p, u, n, w);

                                    if (!w && !p)
                                    {
                                        insn.variant += "_OPTION";
                                    }
                                    else
                                    {
                                        insn.variant += u ? "_P" : "_M";
                                        insn.variant += w ? (p ? "_PREIND" : "_POSTIND") : "_IMM_OFF";
                                    }

                                    insn.opcode = 0xc00 | (p << 8) | (u << 7) | (n << 6) | (w << 5) | (l << 4) | imm;
                                    setInstruction(insn);
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
                        Instruction insn;
                        insn.name;
                        if (trans)
                        {
                            insn.name = (cpopc & 1) ? "MRC" : "MCR";
                            format(insn.addr, "CoRegTrans%s<%d,%d>", ARMv5 ? "2" : "", cpopc >> 1, cp);
                        }
                        else
                        {
                            insn.name = "CDP";
                            format(insn.addr, "CoDataOp%s<%d,%d>", ARMv5 ? "2" : "", cpopc, cp);
                        }

                        insn.variant = insn.name;
                        insn.name += ARMv5 ? "2" : "";

                        insn.opcode = 0xe00 | (cpopc << 4) | (cp << 1) | trans;
                        setInstruction(insn);
                    }
                }
            }
        }

        void genOpcodes_Undocumented()
        {
            // Special undocumented instructions emulated by DeSmuME
            setInstruction(Instruction(0x189, "STREX", "STREX", "STREx"));
            setInstruction(Instruction(0x199, "LDREX", "LDREX", "LDREx"));
        }

        void genOpcodes()
        {
            static const size_t tableSize = 4096;
            mInsnTable.resize(tableSize, "INVALID");
            mAddrTable.resize(tableSize, "Invalid");
            mVariantTable.resize(tableSize, "OP_UND");

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
        }

        void generate()
        {
            genOpcodes();
        }

        const std::vector<std::string>& getInsnTable() const
        {
            return mInsnTable;
        }

        const std::vector<std::string>& getAddrTable() const
        {
            return mAddrTable;
        }

        const std::vector<std::string>& getVariantTable() const
        {
            return mVariantTable;
        }

        void exportTables(const char* filename)
        {
            auto file = FileWriter(filename);

            file.printf("\n");
            for (size_t index = 0; index < mInsnTable.size(); ++index)
                file.printf("INSTRUCTION(%-8s (%s))\n", (mInsnTable[index] + ",").c_str(), mAddrTable[index].c_str());
        }

    private:
        bool                        ARMv4;
        bool                        ARMv5;
        bool                        ARMv5TE;
        Shared&                     mShared;
        std::vector<std::string>    mInsnTable;
        std::vector<std::string>    mAddrTable;
        std::vector<std::string>    mVariantTable;
    };
}

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
    ARM::Shared shared;

    ARM::Generator generatorARM7(shared, ARM::Family::ARM7TDMI);
    generatorARM7.generate();

    ARM::Generator generatorARM9(shared, ARM::Family::ARM946ES);
    generatorARM9.generate();

    printf("Instructions:\n");
    shared.getInsnSet().dump();
    printf("Variants:\n");
    shared.getVariantSet().dump();
    printf("Addressing modes:\n");
    shared.getAddrSet().dump();

    static const std::vector<std::string> expectedInsnTable =
    {
#define TABDECL(insn)  std::string(#insn)
#include "D:\OpenSource\desmume\desmume\src\instruction_tabdef.inc"
#undef TABDECL
    };

    assertSame(expectedInsnTable, generatorARM9.getVariantTable());

    shared.exportSymbols("CpuArmSymbols.inl");
    shared.exportTables("CpuArmTables.inl");
    generatorARM7.exportTables("CpuArm7Tables.inl");
    generatorARM9.exportTables("CpuArm9Tables.inl");

    return 0;
}