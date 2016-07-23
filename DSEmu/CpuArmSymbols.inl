// CpuArmSymbols.inl generated automatically at Sat Jul 23 01:47:11 2016

enum class Insn : uint8_t
{
    INVALID,    B,          BL,         BX,         BLX,        SWI,        AND,        ANDS,
    EOR,        EORS,       SUB,        SUBS,       RSB,        RSBS,       ADD,        ADDS,
    ADC,        ADCS,       SBC,        SBCS,       RSC,        RSCS,       TSTS,       TEQS,
    CMPS,       CMNS,       ORR,        ORRS,       MOV,        MOVS,       BIC,        BICS,
    MVN,        MVNS,       MUL,        MULS,       MLA,        MLAS,       UMULL,      UMULLS,
    UMLAL,      UMLALS,     SMULL,      SMULLS,     SMLAL,      SMLALS,     SMLABB,     SMLATB,
    SMLABT,     SMLATT,     SMLAWB,     SMULWB,     SMLAWT,     SMULWT,     SMLALBB,    SMLALTB,
    SMLALBT,    SMLALTT,    SMULBB,     SMULTB,     SMULBT,     SMULTT,     MRS,        MSR,
    STR,        LDR,        STRT,       LDRT,       STRB,       LDRB,       STRBT,      LDRBT,
    STRH,       LDRD,       STRD,       LDRH,       LDRSB,      LDRSH,      STMDA,      LDMDA,
    STMIA,      LDMIA,      STMDB,      LDMDB,      STMIB,      LDMIB,      SWP,        SWPB,
    STC,        LDC,        STCL,       LDCL,       CDP,        MCR,        MRC,        STREX,
    LDREX,      BKPT,       CLZ,        QADD,       QSUB,       QDADD,      QDSUB,      STC2,
    LDC2,       STC2L,      LDC2L,      CDP2,       MCR2,       MRC2,
};

enum class Addr : uint8_t
{
    Invalid,        BranchOffset,   BranchReg,      SWI,
    ALUReg,         ALUImm,         Multiply,       MRS,
    MSRReg,         MSRImm,         MemImm,         MemReg,
    MemExReg,       MemExImm,       MemBlock,       SWP,
    CoDataTrans,    CoDataOp,       CoRegTrans,     STREx,
    LDREx,          BKPT,           CLZ,            QALU,
    CoDataTrans2,   CoDataOp2,      CoRegTrans2,
};
