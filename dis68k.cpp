// dis68k.cpp

static const char versionName[] = "Motorola 68000 disassembler";

#define MAC_A_TRAPS // comment to disable classic Mac OS A-Traps

//#define LAZY_EXTWORD // uncomment to allow invalid extension word bits on 68000/68010

#include "discpu.h"

struct InstrRec {
    uint16_t        andWith;
    uint16_t        cmpWith;
    int             typ;        // typ
    const char      *op;        // mnemonic
    uint8_t         lfref;      // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;


class Dis68K : public CPU {
public:
    Dis68K(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);

private:
    uint16_t FetchWord(int addr, int &len);
    addr_t FetchLong(int addr, int &len);
    static int GetBits(int num, int pos, int len);
    addr_t FetchSize(int addr, int &len, int size);
    void AddrMode(int mode, int reg, int size, char *parms, int addr, int &len, bool &invalid,
                  int &lfref, addr_t &refaddr);
    void SrcAddr(int opcd, int size, char *parms, int addr, int &len, bool &invalid,
                      int &lfref, addr_t &refaddr);
    void DstAddr(int opcd, int size, char *parms, int addr, int &len, bool &invalid,
                      int &lfref, addr_t &refaddr);
    static void MOVEMRegs(int mode, char *line, int regs);
    void DisATrap(uint16_t opcd, char *opcode, char *line, addr_t addr);
    InstrPtr FindInstr(uint16_t opcd);
};


enum {
    CPU_68000,
    CPU_68010,
    CPU_68020,
};

Dis68K cpu_68K  ("68K",    CPU_68000, BIG_END, ADDR_24, '*', '$', "DC.B", "DC.W", "DC.L");
Dis68K cpu_68000("68000",  CPU_68000, BIG_END, ADDR_24, '*', '$', "DC.B", "DC.W", "DC.L");
Dis68K cpu_68010("68010",  CPU_68010, BIG_END, ADDR_24, '*', '$', "DC.B", "DC.W", "DC.L");
Dis68K cpu_68020("68020",  CPU_68020, BIG_END, ADDR_24, '*', '$', "DC.B", "DC.W", "DC.L");


Dis68K::Dis68K(const char *name, int subtype, int endian, int addrwid,
               char curAddrChr, char hexChr, const char *byteOp,
               const char *wordOp, const char *longOp)
{
    _file    = __FILE__;
    _name    = name;
    _version = versionName;
    _subtype = subtype;
    _dbopcd  = byteOp;
    _dwopcd  = wordOp;
    _dlopcd  = longOp;
    _curpc   = curAddrChr;
    _endian  = endian;
    _hexchr  = hexChr;
    _addrwid = addrwid;
    _radix   = RAD_HEX16BE;

    add_cpu();
}


// =====================================================


enum InstSize {
    s_Byte, s_Word, s_Long, s_UseCCR=4, s_UseSR
};

static const char opSizes[] = "BWL?";

enum InstType {
    o_Invalid,      // 0
    o_Implied,      // 1
    o_Immed,        // 2
    o_Dn_EA,        // 3
    o_MOVEP,        // 4
    o_StatBit,      // 5
    o_MOVES,        // 6
    o_MOVE,         // 7
    o_OneAddrSz,    // 8
    o_MoveFromSR,   // 9
    o_MoveToSR,     // 10
    o_EA_Dn,        // 11
    o_LEA,          // 12
    o_OneAddr,      // 13
    o_One_Dn,       // 14
    o_MOVEM,        // 15
    o_TRAP,         // 16
    o_LINK,         // 17
    o_UNLK,         // 18
    o_MOVE_USP,     // 19
    o_MOVEC,        // 20
    o_ADDQ_SUBQ,    // 21
    o_DBcc,         // 22
    o_Bcc,          // 23
    o_MOVEQ,        // 24
    o_EA_Dn_Sz,     // 25
    o_Dn_EA_Sz,     // 26
    o_STOP,         // 27
    o_ABCD_SBCD,    // 28
    o_ADDX_SUBX,    // 29
    o_CMPM,         // 30
    o_EXG,          // 31
    o_ShiftRotImm,  // 32
    o_ShiftRotReg,  // 33
    o_A_Trap,       // 34
    o_ADDA_SUBA,    // 35

    o_BitField,     // 68020 bit field instructions
    o_PMOVE,        // 68030 Move to/from MMU registers
    o_BKPT,         // 68020 BKPT
    o_CALLM,        // 68020 CALLM
    o_CAS,          // 68020 CAS
    o_CAS2,         // 68020 CAS2
    o_CMP2,         // 68020 CMP2/CHK2
    o_PACK,         // 68020 PACK/UNPK
    o_RTM,          // 68020 RTM
    o_TRAPcc,       // 68020 TRAPcc
    o_EA_Dn_020,    // 68020 CHK.L
    o_One_Dn_020,   // 68020 EXTB.L
    o_MUL_020,      // 68020 MUL/DIV long
};

struct TrapRec {
    uint16_t        trap;       // opcode of trap
    char            bit_9;      // name of bit 9
    char            bit_10;     // name of bit 10
    const char      *name;      // name of trap
};
typedef const struct TrapRec *TrapPtr;


static const struct InstrRec M68K_opcdTable[] =
{
    // 68020 opcodes

    {0xFFF0, 0x06C0, o_RTM,         "RTM"    , LFFLAG },
    {0xFFC0, 0x06C0, o_CALLM,       "CALLM"  , 0 },
    {0xF9C0, 0x00C0, o_CMP2,        "CMP2"   , 0 }, // CMP2/CHK2
    {0xF9FF, 0x08FC, o_CAS2,        "CAS2"   , 0 },
    {0xF9C0, 0x08C0, o_CAS,         "CAS"    , 0 },
    {0xF1C0, 0x4100, o_EA_Dn_020,   "CHK.L"  , 0 },
    {0xFFF8, 0x4808, o_LINK,        "LINK"   , 0 },
    {0xFFF8, 0x4848, o_BKPT,        "BKPT"   , 0 },
    {0xFFF8, 0x49C0, o_One_Dn_020,  "EXTB.L" , 0 },
    {0xFFC0, 0x4C00, o_MUL_020,     "MUL"    , 0 },
    {0xFFC0, 0x4C40, o_MUL_020,     "DIV"    , 0 },
    {0xFFF8, 0x50F8, o_TRAPcc,      "TRAPT"  , 0 },
    {0xFFF8, 0x51F8, o_TRAPcc,      "TRAPF"  , 0 },
    {0xFFF8, 0x52F8, o_TRAPcc,      "TRAPHI" , 0 },
    {0xFFF8, 0x53F8, o_TRAPcc,      "TRAPLS" , 0 },
    {0xFFF8, 0x54F8, o_TRAPcc,      "TRAPCC" , 0 }, // also TRAPHS
    {0xFFF8, 0x55F8, o_TRAPcc,      "TRAPCS" , 0 }, // also TRAPLO
    {0xFFF8, 0x56F8, o_TRAPcc,      "TRAPNE" , 0 },
    {0xFFF8, 0x57F8, o_TRAPcc,      "TRAPEQ" , 0 },
    {0xFFF8, 0x58F8, o_TRAPcc,      "TRAPVC" , 0 },
    {0xFFF8, 0x59F8, o_TRAPcc,      "TRAPVS" , 0 },
    {0xFFF8, 0x5AF8, o_TRAPcc,      "TRAPPL" , 0 },
    {0xFFF8, 0x5BF8, o_TRAPcc,      "TRAPMI" , 0 },
    {0xFFF8, 0x5CF8, o_TRAPcc,      "TRAPGE" , 0 },
    {0xFFF8, 0x5DF8, o_TRAPcc,      "TRAPLT" , 0 },
    {0xFFF8, 0x5EF8, o_TRAPcc,      "TRAPGT" , 0 },
    {0xFFF8, 0x5FF8, o_TRAPcc,      "TRAPLE" , 0 },
    {0xF1F0, 0x8140, o_PACK,        "PACK"   , 0 },
    {0xF1F0, 0x8180, o_PACK,        "UNPK"   , 0 },
    {0xFFC0, 0xE8C0, o_BitField,    "BFTST"  , 0 },
    {0xFFC0, 0xE9C0, o_BitField,    "BFEXTU" , 0 },
    {0xFFC0, 0xEAC0, o_BitField,    "BFCHG"  , 0 },
    {0xFFC0, 0xEBC0, o_BitField,    "BFEXTS" , 0 },
    {0xFFC0, 0xECC0, o_BitField,    "BFCLR"  , 0 },
    {0xFFC0, 0xEDC0, o_BitField,    "BFFFO"  , 0 },
    {0xFFC0, 0xEEC0, o_BitField,    "BFSET"  , 0 },
    {0xFFC0, 0xEFC0, o_BitField,    "BFINS"  , 0 },
    {0xFFC0, 0xF000, o_PMOVE,       "PMOVE"  , 0 },

// -----------------------------------------------------

    // and     cmp     typ            op       lfref
    {0xFFC0, 0x00C0, o_Invalid,     ""       , 0 },
    {0xFF00, 0x0000, o_Immed,       "ORI"    , 0 },
    {0xFFC0, 0x02C0, o_Invalid,     ""       , 0 },
    {0xFF00, 0x0200, o_Immed,       "ANDI"   , 0 },
    {0xFFC0, 0x04C0, o_Invalid,     ""       , 0 },
    {0xFF00, 0x0400, o_Immed,       "SUBI"   , 0 },
    {0xFFC0, 0x06C0, o_Invalid,     ""       , 0 },
    {0xFF00, 0x0600, o_Immed,       "ADDI"   , 0 },
    {0xFFC0, 0x0AC0, o_Invalid,     ""       , 0 },
    {0xFF00, 0x0A00, o_Immed,       "EORI"   , 0 },
    {0xFFC0, 0x0CC0, o_Invalid,     ""       , 0 },
    {0xFF00, 0x0C00, o_Immed,       "CMPI"   , 0 },

    {0xF138, 0x0108, o_MOVEP,       "MOVEP"  , 0 },
    {0xF1C0, 0x0100, o_Dn_EA,       "BTST"   , 0 },
    {0xF1C0, 0x0140, o_Dn_EA,       "BCHG"   , 0 },
    {0xF1C0, 0x0180, o_Dn_EA,       "BCLR"   , 0 },
    {0xF1C0, 0x01C0, o_Dn_EA,       "BSET"   , 0 },

    {0xFFC0, 0x0800, o_StatBit,     "BTST"   , 0 },
    {0xFFC0, 0x0840, o_StatBit,     "BCHG"   , 0 },
    {0xFFC0, 0x0880, o_StatBit,     "BCLR"   , 0 },
    {0xFFC0, 0x08C0, o_StatBit,     "BSET"   , 0 },

    {0xFFC0, 0x0EC0, o_Invalid,     ""       , 0 },
    {0xFF00, 0x0E00, o_MOVES,       "MOVES"  , 0 },

    {0xF1C0, 0x1040, o_Invalid,     ""       , 0 },
    {0xF000, 0x1000, o_MOVE,        "MOVE"   , 0 },
    {0xE1C0, 0x2040, o_MOVE,        "MOVEA"  , 0 },
    {0xE000, 0x2000, o_MOVE,        "MOVE"   , 0 },

    {0xF1C0, 0x4180, o_EA_Dn,       "CHK"    , 0 },
    {0xF1C0, 0x41C0, o_LEA,         "LEA"    , 0 },
    {0xFFC0, 0x40C0, o_MoveFromSR,  "MOVE"   , 0 },
    {0xFF00, 0x4000, o_OneAddrSz,   "NEGX"   , 0 },
    {0xFFC0, 0x42C0, o_MoveFromSR,  "MOVE"   , 0 },
    {0xFF00, 0x4200, o_OneAddrSz,   "CLR"    , 0 },
    {0xFFC0, 0x44C0, o_MoveToSR,    "MOVE"   , 0 },
    {0xFF00, 0x4400, o_OneAddrSz,   "NEG"    , 0 },
    {0xFFC0, 0x46C0, o_MoveToSR,    "MOVE"   , 0 },
    {0xFF00, 0x4600, o_OneAddrSz,   "NOT"    , 0 },
    {0xFFC0, 0x4800, o_OneAddr,     "NBCD"   , 0 },
    {0xFFF8, 0x4840, o_One_Dn,      "SWAP"   , 0 },
    {0xFFC0, 0x4840, o_OneAddr,     "PEA"    , 0 },
    {0xFFF8, 0x4880, o_One_Dn,      "EXT.W"  , 0 },
    {0xFFF8, 0x48C0, o_One_Dn,      "EXT.L"  , 0 },
    {0xFFFF, 0x4AFC, o_Implied,     "ILLEGAL", 0 },
    {0xFFC0, 0x4AC0, o_OneAddr,     "TAS"    , 0 },
    {0xFFF0, 0x4E40, o_TRAP,        "TRAP"   , 0 },
    {0xFF00, 0x4A00, o_OneAddrSz,   "TST"    , 0 },
    {0xFB80, 0x4880, o_MOVEM,       "MOVEM"  , 0 },
    {0xFFF8, 0x4E50, o_LINK,        "LINK"   , 0 },
    {0xFFF8, 0x4E58, o_UNLK,        "UNLK"   , 0 },
    {0xFFF0, 0x4E60, o_MOVE_USP,    "MOVE"   , 0 },

    {0xFFFF, 0x4E70, o_Implied,     "RESET"  , LFFLAG},
    {0xFFFF, 0x4E71, o_Implied,     "NOP"    , 0 },
    {0xFFFF, 0x4E72, o_STOP,        "STOP"   , LFFLAG},
    {0xFFFF, 0x4E73, o_Implied,     "RTE"    , LFFLAG},
    {0xFFFF, 0x4E74, o_STOP,        "RTD"    , LFFLAG},
    {0xFFFF, 0x4E75, o_Implied,     "RTS"    , LFFLAG},
    {0xFFFF, 0x4E76, o_Implied,     "TRAPV"  , 0 },
    {0xFFFF, 0x4E77, o_Implied,     "RTR"    , LFFLAG},
    {0xFFFE, 0x4E7A, o_MOVEC,       "MOVEC"  , 0 },

    {0xFFC0, 0x4E80, o_OneAddr,     "JSR"    , REFFLAG | CODEREF},
    {0xFFC0, 0x4EC0, o_OneAddr,     "JMP"    , LFFLAG | REFFLAG | CODEREF},

    {0xFFF8, 0x50C8, o_DBcc,        "DBT"    , REFFLAG | CODEREF},
    {0xFFF8, 0x51C8, o_DBcc,        "DBRA"   , REFFLAG | CODEREF},
    {0xFFF8, 0x52C8, o_DBcc,        "DBHI"   , REFFLAG | CODEREF},
    {0xFFF8, 0x53C8, o_DBcc,        "DBLS"   , REFFLAG | CODEREF},
    {0xFFF8, 0x54C8, o_DBcc,        "DBCC"   , REFFLAG | CODEREF}, // aka DBHS
    {0xFFF8, 0x55C8, o_DBcc,        "DBCS"   , REFFLAG | CODEREF}, // aka DBLO
    {0xFFF8, 0x56C8, o_DBcc,        "DBNE"   , REFFLAG | CODEREF},
    {0xFFF8, 0x57C8, o_DBcc,        "DBEQ"   , REFFLAG | CODEREF},
    {0xFFF8, 0x58C8, o_DBcc,        "DBVC"   , REFFLAG | CODEREF},
    {0xFFF8, 0x59C8, o_DBcc,        "DBVS"   , REFFLAG | CODEREF},
    {0xFFF8, 0x5AC8, o_DBcc,        "DBPL"   , REFFLAG | CODEREF},
    {0xFFF8, 0x5BC8, o_DBcc,        "DBMI"   , REFFLAG | CODEREF},
    {0xFFF8, 0x5CC8, o_DBcc,        "DBGE"   , REFFLAG | CODEREF},
    {0xFFF8, 0x5DC8, o_DBcc,        "DBLT"   , REFFLAG | CODEREF},
    {0xFFF8, 0x5EC8, o_DBcc,        "DBGT"   , REFFLAG | CODEREF},
    {0xFFF8, 0x5FC8, o_DBcc,        "DBLE"   , REFFLAG | CODEREF},

    {0xFFC0, 0x50C0, o_OneAddr,     "ST"     , 0 },
    {0xFFC0, 0x51C0, o_OneAddr,     "SF"     , 0 },
    {0xFFC0, 0x52C0, o_OneAddr,     "SHI"    , 0 },
    {0xFFC0, 0x53C0, o_OneAddr,     "SLS"    , 0 },
    {0xFFC0, 0x54C0, o_OneAddr,     "SCC"    , 0 }, // aka SHS
    {0xFFC0, 0x55C0, o_OneAddr,     "SCS"    , 0 }, // aks SLO
    {0xFFC0, 0x56C0, o_OneAddr,     "SNE"    , 0 },
    {0xFFC0, 0x57C0, o_OneAddr,     "SEQ"    , 0 },
    {0xFFC0, 0x58C0, o_OneAddr,     "SVC"    , 0 },
    {0xFFC0, 0x59C0, o_OneAddr,     "SVS"    , 0 },
    {0xFFC0, 0x5AC0, o_OneAddr,     "SPL"    , 0 },
    {0xFFC0, 0x5BC0, o_OneAddr,     "SMI"    , 0 },
    {0xFFC0, 0x5CC0, o_OneAddr,     "SGE"    , 0 },
    {0xFFC0, 0x5DC0, o_OneAddr,     "SLT"    , 0 },
    {0xFFC0, 0x5EC0, o_OneAddr,     "SGT"    , 0 },
    {0xFFC0, 0x5FC0, o_OneAddr,     "SLE"    , 0 },

    {0xF100, 0x5000, o_ADDQ_SUBQ,   "ADDQ"   , 0 },
    {0xF100, 0x5100, o_ADDQ_SUBQ,   "SUBQ"   , 0 },

    {0xFF00, 0x6000, o_Bcc,         "BRA"    , LFFLAG | REFFLAG | CODEREF},
    {0xFF00, 0x6100, o_Bcc,         "BSR"    , REFFLAG | CODEREF},
    {0xFF00, 0x6200, o_Bcc,         "BHI"    , REFFLAG | CODEREF},
    {0xFF00, 0x6300, o_Bcc,         "BLS"    , REFFLAG | CODEREF},
    {0xFF00, 0x6400, o_Bcc,         "BCC"    , REFFLAG | CODEREF}, // aka BHS
    {0xFF00, 0x6500, o_Bcc,         "BCS"    , REFFLAG | CODEREF}, // aka BLO
    {0xFF00, 0x6600, o_Bcc,         "BNE"    , REFFLAG | CODEREF},
    {0xFF00, 0x6700, o_Bcc,         "BEQ"    , REFFLAG | CODEREF},
    {0xFF00, 0x6800, o_Bcc,         "BVC"    , REFFLAG | CODEREF},
    {0xFF00, 0x6900, o_Bcc,         "BVS"    , REFFLAG | CODEREF},
    {0xFF00, 0x6A00, o_Bcc,         "BPL"    , REFFLAG | CODEREF},
    {0xFF00, 0x6B00, o_Bcc,         "BMI"    , REFFLAG | CODEREF},
    {0xFF00, 0x6C00, o_Bcc,         "BGE"    , REFFLAG | CODEREF},
    {0xFF00, 0x6D00, o_Bcc,         "BLT"    , REFFLAG | CODEREF},
    {0xFF00, 0x6E00, o_Bcc,         "BGT"    , REFFLAG | CODEREF},
    {0xFF00, 0x6F00, o_Bcc,         "BLE"    , REFFLAG | CODEREF},

    {0xF100, 0x7000, o_MOVEQ,       "MOVEQ"  , 0 },
    {0xF1F0, 0x8100, o_ABCD_SBCD,   "SBCD"   , 0 },
    {0xF1C0, 0x80C0, o_EA_Dn,       "DIVU"   , 0 },
    {0xF1C0, 0x81C0, o_EA_Dn,       "DIVS"   , 0 },
    {0xF100, 0x8000, o_EA_Dn_Sz,    "OR"     , 0 },
    {0xF100, 0x8100, o_Dn_EA_Sz,    "OR"     , 0 },
    {0xF0C0, 0x90C0, o_ADDA_SUBA,   "SUBA"   , 0 },
    {0xF130, 0x9100, o_ADDX_SUBX,   "SUBX"   , 0 },
    {0xF100, 0x9000, o_EA_Dn_Sz,    "SUB"    , 0 },
    {0xF100, 0x9100, o_Dn_EA_Sz,    "SUB"    , 0 },

    {0xFC00, 0xAC00, o_A_Trap,      "A-Trap" , LFFLAG},
    {0xFFFF, 0xA9F0, o_A_Trap,      "A-Trap" , LFFLAG},
    {0xFFFF, 0xA9F2, o_A_Trap,      "A-Trap" , LFFLAG},
    {0xFFFF, 0xA9F3, o_A_Trap,      "A-Trap" , LFFLAG},
    {0xFFFF, 0xA9F4, o_A_Trap,      "A-Trap" , LFFLAG},
    {0xF000, 0xA000, o_A_Trap,      "A-Trap" , 0 },

    {0xF0C0, 0xB0C0, o_ADDA_SUBA,   "CMPA"   , 0 },
    {0xF138, 0xB108, o_CMPM,        "CMPM"   , 0 },
    {0xF100, 0xB000, o_EA_Dn_Sz,    "CMP"    , 0 },
    {0xF100, 0xB100, o_Dn_EA_Sz,    "EOR"    , 0 },
    {0xF1F0, 0xC100, o_ABCD_SBCD,   "ABCD"   , 0 },
    {0xF1F0, 0xC140, o_EXG,         "EXG"    , 0 },
    {0xF1F8, 0xC188, o_EXG,         "EXG"    , 0 },
    {0xF1C0, 0xC0C0, o_EA_Dn,       "MULU"   , 0 },
    {0xF1C0, 0xC1C0, o_EA_Dn,       "MULS"   , 0 },
    {0xF100, 0xC000, o_EA_Dn_Sz,    "AND"    , 0 },
    {0xF100, 0xC100, o_Dn_EA_Sz,    "AND"    , 0 },
    {0xF0C0, 0xD0C0, o_ADDA_SUBA,   "ADDA"   , 0 },
    {0xF130, 0xD100, o_ADDX_SUBX,   "ADDX"   , 0 },
    {0xF100, 0xD000, o_EA_Dn_Sz,    "ADD"    , 0 },
    {0xF100, 0xD100, o_Dn_EA_Sz,    "ADD"    , 0 },

    {0xFFC0, 0xE0C0, o_OneAddr,     "ASR"    , 0 },
    {0xFFC0, 0xE1C0, o_OneAddr,     "ASL"    , 0 },
    {0xFFC0, 0xE2C0, o_OneAddr,     "LSR"    , 0 },
    {0xFFC0, 0xE3C0, o_OneAddr,     "LSL"    , 0 },
    {0xFFC0, 0xE4C0, o_OneAddr,     "ROXR"   , 0 },
    {0xFFC0, 0xE5C0, o_OneAddr,     "ROXL"   , 0 },
    {0xFFC0, 0xE6C0, o_OneAddr,     "ROR"    , 0 },
    {0xFFC0, 0xE7C0, o_OneAddr,     "ROL"    , 0 },
    {0xF8C0, 0xE8C0, o_Invalid,     ""       , 0 },

    {0xF138, 0xE000, o_ShiftRotImm, "ASR"    , 0 },
    {0xF138, 0xE100, o_ShiftRotImm, "ASL"    , 0 },
    {0xF138, 0xE008, o_ShiftRotImm, "LSR"    , 0 },
    {0xF138, 0xE108, o_ShiftRotImm, "LSL"    , 0 },
    {0xF138, 0xE010, o_ShiftRotImm, "ROXR"   , 0 },
    {0xF138, 0xE110, o_ShiftRotImm, "ROXL"   , 0 },
    {0xF138, 0xE018, o_ShiftRotImm, "ROR"    , 0 },
    {0xF138, 0xE118, o_ShiftRotImm, "ROL"    , 0 },
    {0xF138, 0xE020, o_ShiftRotReg, "ASR"    , 0 },
    {0xF138, 0xE120, o_ShiftRotReg, "ASL"    , 0 },
    {0xF138, 0xE028, o_ShiftRotReg, "LSR"    , 0 },
    {0xF138, 0xE128, o_ShiftRotReg, "LSL"    , 0 },
    {0xF138, 0xE030, o_ShiftRotReg, "ROXR"   , 0 },
    {0xF138, 0xE130, o_ShiftRotReg, "ROXL"   , 0 },
    {0xF138, 0xE038, o_ShiftRotReg, "ROR"    , 0 },
    {0xF138, 0xE138, o_ShiftRotReg, "ROL"    , 0 },

//  {0xF000, 0xF000, o_F_Trap,      "F-Trap" , 0 },

    {0x0000, 0x0000, o_Invalid,     ""       , 0 }
};


#ifdef MAC_A_TRAPS
static const struct TrapRec trapTable[] =
{
    {0xA000, 'H', 'A', "_Open"},
    {0xA001, 'I', 'A', "_Close"},
    {0xA002, 'I', 'A', "_Read"},
    {0xA003, 'I', 'A', "_Write"},
    {0xA004, 'I', 'A', "_Control"},
    {0xA005, 'I', 'A', "_Status"},
    {0xA006, 'I', 'A', "_KillIO"},
    {0xA007, 'H', 'A', "_GetVolInfo"},
    {0xA008, 'H', 'A', "_Create"},
    {0xA009, 'H', 'A', "_Delete"},
    {0xA00A, 'H', 'A', "_OpenRF"},
    {0xA00B, 'H', 'A', "_Rename"},
    {0xA00C, 'H', 'A', "_GetFileInfo"},
    {0xA00D, 'H', 'A', "_SetFileInfo"},
    {0xA00E, '.', '.', "_UnmountVol"},
    {0xA00F, '.', '.', "_MountVol"},
    {0xA010, 'I', 'A', "_Allocate"},
    {0xA011, 'I', 'A', "_GetEOF"},
    {0xA012, 'I', 'A', "_SetEOF"},
    {0xA013, 'I', 'A', "_FlushVol"},
    {0xA014, 'H', 'A', "_GetVol"},
    {0xA015, 'H', 'A', "_SetVol"},
    {0xA016, '.', '.', "_InitQueue"},
    {0xA017, 'I', 'A', "_Eject"},
    {0xA018, 'I', 'A', "_GetFPos"},
    {0xA019, '.', '.', "_InitZone"},
    {0xA11A, '.', '.', "_GetZone"},
    {0xA01B, '.', '.', "_SetZone"},
    {0xA01C, '.', 'S', "_FreeMem"},
    {0xA11D, '.', 'S', "_MaxMem"},
    {0xA11E, 'C', 'S', "_NewPtr"},
    {0xA01F, '.', '.', "_DisposPtr"},
    {0xA020, '.', '.', "_SetPtrSize"},
    {0xA021, '.', '.', "_GetPtrSize"},
    {0xA122, 'C', 'S', "_NewHandle"},
    {0xA023, '.', '.', "_DisposHandle"},
    {0xA024, '.', '.', "_SetHandleSize"},
    {0xA025, '.', '.', "_GetHandleSize"},
    {0xA126, '.', '.', "_HandleZone"},
    {0xA027, '.', '.', "_ReallocHandle"},
    {0xA128, '.', 'S', "_RecoverHandle"},
    {0xA029, '.', '.', "_HLock"},
    {0xA02A, '.', '.', "_HUnlock"},
    {0xA02B, '.', '.', "_EmptyHandle"},
    {0xA02C, '.', '.', "_InitApplZone"},
    {0xA02D, '.', '.', "_SetApplLimit"},
    {0xA02E, '.', '.', "_BlockMove"},
    {0xA02F, '.', '.', "_PostEvent"},
    {0xA12F, '.', '.', "_PPostEvent"},
    {0xA030, '.', '.', "_OSEventAvail"},
    {0xA031, '.', '.', "_GetOSEvent"},
    {0xA032, '.', '.', "_FlushEvents"},
    {0xA033, '.', '.', "_VInstall"},
    {0xA034, '.', '.', "_VRemove"},
    {0xA035, 'I', 'A', "_Offline"},
    {0xA036, '.', '.', "_MoreMasters"},
    {0xA037, '.', '.', "_ReadParam"},
    {0xA038, '.', '.', "_WriteParam"},
    {0xA039, '.', '.', "_ReadDateTime"},
    {0xA03A, '.', '.', "_SetDateTime"},
    {0xA03B, '.', '.', "_Delay"},
    {0xA03C, 'M', 'K', "_CmpString"},
    {0xA03D, '.', '.', "_DrvrInstall"},
    {0xA03E, '.', '.', "_DrvrRemove"},
    {0xA03F, '.', '.', "_InitUtil"},
    {0xA040, '.', 'S', "_ResrvMem"},
    {0xA041, 'I', 'A', "_SetFilLock"},
    {0xA042, 'I', 'A', "_RstFilLock"},
    {0xA043, 'I', 'A', "_SetFilType"},
    {0xA044, 'I', 'A', "_SetFPos"},
    {0xA045, 'A', 'I', "_FlushFile"},
    {0xA146, 'O', 'T', "_GetTrapAddress"},
    {0xA047, 'O', 'T', "_SetTrapAddress"},
    {0xA148, '.', '.', "_PtrZone"},
    {0xA049, '.', '.', "_HPurge"},
    {0xA04A, '.', '.', "_HNoPurge"},
    {0xA04B, '.', '.', "_SetGrowZone"},
    {0xA04C, '.', 'S', "_CompactMem"},
    {0xA04D, '.', 'S', "_PurgeMem"},
    {0xA04E, '.', '.', "_AddDrive"},
    {0xA04F, '.', '.', "_RDrvrInstall"},
    {0xA850, '.', '.', "_InitCursor"},
    {0xA851, '.', '.', "_SetCursor"},
    {0xA852, '.', '.', "_HideCursor"},
    {0xA853, '.', '.', "_ShowCursor"},
    {0xA054, 'M', 'K', "_UprString"},
    {0xA855, '.', '.', "_ShieldCursor"},
    {0xA856, '.', '.', "_ObscureCursor"},
    {0xA057, '.', '.', "_SetAppBase"},
    {0xA858, '.', '.', "_BitAnd"},
    {0xA859, '.', '.', "_BitXor"},
    {0xA85A, '.', '.', "_BitNot"},
    {0xA85B, '.', '.', "_BitOr"},
    {0xA85C, '.', '.', "_BitShift"},
    {0xA85D, '.', '.', "_BitTst"},
    {0xA85E, '.', '.', "_BitSet"},
    {0xA85F, '.', '.', "_BitClr"},
    {0xA861, '.', '.', "_Random"},
    {0xA862, '.', '.', "_ForeColor"},
    {0xA863, '.', '.', "_BackColor"},
    {0xA864, '.', '.', "_ColorBit"},
    {0xA865, '.', '.', "_GetPixel"},
    {0xA866, '.', '.', "_StuffHex"},
    {0xA867, '.', '.', "_LongMul"},
    {0xA868, '.', '.', "_FixMul"},
    {0xA869, '.', '.', "_FixRatio"},
    {0xA86A, '.', '.', "_HiWord"},
    {0xA86B, '.', '.', "_LoWord"},
    {0xA86C, '.', '.', "_FixRound"},
    {0xA86D, '.', '.', "_InitPort"},
    {0xA86E, '.', '.', "_InitGraf"},
    {0xA86F, '.', '.', "_OpenPort"},
    {0xA870, '.', '.', "_LocalToGlobal"},
    {0xA871, '.', '.', "_GlobalToLocal"},
    {0xA872, '.', '.', "_GrafDevice"},
    {0xA873, '.', '.', "_SetPort"},
    {0xA874, '.', '.', "_GetPort"},
    {0xA875, '.', '.', "_SetPBits"},
    {0xA876, '.', '.', "_PortSize"},
    {0xA877, '.', '.', "_MovePortTo"},
    {0xA878, '.', '.', "_SetOrigin"},
    {0xA879, '.', '.', "_SetClip"},
    {0xA87A, '.', '.', "_GetClip"},
    {0xA87B, '.', '.', "_ClipRect"},
    {0xA87C, '.', '.', "_BackPat"},
    {0xA87D, '.', '.', "_ClosePort"},
    {0xA87E, '.', '.', "_AddPt"},
    {0xA87F, '.', '.', "_SubPt"},
    {0xA880, '.', '.', "_SetPt"},
    {0xA881, '.', '.', "_EqualPt"},
    {0xA882, '.', '.', "_StdText"},
    {0xA883, '.', '.', "_DrawChar"},
    {0xA884, '.', '.', "_DrawString"},
    {0xA885, '.', '.', "_DrawText"},
    {0xA886, '.', '.', "_TextWidth"},
    {0xA887, '.', '.', "_TextFont"},
    {0xA888, '.', '.', "_TextFace"},
    {0xA889, '.', '.', "_TextMode"},
    {0xA88A, '.', '.', "_TextSize"},
    {0xA88B, '.', '.', "_GetFontInfo"},
    {0xA88C, '.', '.', "_StringWidth"},
    {0xA88D, '.', '.', "_CharWidth"},
    {0xA88E, '.', '.', "_SpaceExtra"},
    {0xA890, '.', '.', "_StdLine"},
    {0xA891, '.', '.', "_LineTo"},
    {0xA892, '.', '.', "_Line"},
    {0xA893, '.', '.', "_MoveTo"},
    {0xA894, '.', '.', "_Move"},
    {0xA896, '.', '.', "_HidePen"},
    {0xA897, '.', '.', "_ShowPen"},
    {0xA898, '.', '.', "_GetPenState"},
    {0xA899, '.', '.', "_SetPenState"},
    {0xA89A, '.', '.', "_GetPen"},
    {0xA89B, '.', '.', "_PenSize"},
    {0xA89C, '.', '.', "_PenMode"},
    {0xA89D, '.', '.', "_PenPat"},
    {0xA89E, '.', '.', "_PenNormal"},
    {0xA89F, '.', '.', "_UnimplTrap"},
    {0xA8A0, '.', '.', "_StdRect"},
    {0xA8A1, '.', '.', "_FrameRect"},
    {0xA8A2, '.', '.', "_PaintRect"},
    {0xA8A3, '.', '.', "_EraseRect"},
    {0xA8A4, '.', '.', "_InverRect"},
    {0xA8A5, '.', '.', "_FillRect"},
    {0xA8A6, '.', '.', "_EqualRect"},
    {0xA8A7, '.', '.', "_SetRect"},
    {0xA8A8, '.', '.', "_OffsetRect"},
    {0xA8A9, '.', '.', "_InsetRect"},
    {0xA8AA, '.', '.', "_SectRect"},
    {0xA8AB, '.', '.', "_UnionRect"},
    {0xA8AC, '.', '.', "_Pt2Rect"},
    {0xA8AD, '.', '.', "_PtInRect"},
    {0xA8AE, '.', '.', "_EmptyRect"},
    {0xA8AF, '.', '.', "_StdRRect"},
    {0xA8B0, '.', '.', "_FrameRoundRect"},
    {0xA8B1, '.', '.', "_PaintRoundRect"},
    {0xA8B2, '.', '.', "_EraseRoundRect"},
    {0xA8B3, '.', '.', "_InverRoundRect"},
    {0xA8B4, '.', '.', "_FillRoundRect"},
    {0xA8B6, '.', '.', "_StdOval"},
    {0xA8B7, '.', '.', "_FrameOval"},
    {0xA8B8, '.', '.', "_PaintOval"},
    {0xA8B9, '.', '.', "_EraseOval"},
    {0xA8BA, '.', '.', "_InvertOval"},
    {0xA8BB, '.', '.', "_FillOval"},
    {0xA8BC, '.', '.', "_SlopeFromAngle"},
    {0xA8BD, '.', '.', "_StdArc"},
    {0xA8BE, '.', '.', "_FrameArc"},
    {0xA8BF, '.', '.', "_PaintArc"},
    {0xA8C0, '.', '.', "_EraseArc"},
    {0xA8C1, '.', '.', "_InvertArc"},
    {0xA8C2, '.', '.', "_FillArc"},
    {0xA8C3, '.', '.', "_PtToAngle"},
    {0xA8C4, '.', '.', "_AngleFromSlope"},
    {0xA8C5, '.', '.', "_StdPoly"},
    {0xA8C6, '.', '.', "_FramePoly"},
    {0xA8C7, '.', '.', "_PaintPoly"},
    {0xA8C8, '.', '.', "_ErasePoly"},
    {0xA8C9, '.', '.', "_InvertPoly"},
    {0xA8CA, '.', '.', "_FillPoly"},
    {0xA8CB, '.', '.', "_OpenPoly"},
    {0xA8CC, '.', '.', "_ClosePgon"},
    {0xA8CD, '.', '.', "_KillPoly"},
    {0xA8CE, '.', '.', "_OffsetPoly"},
    {0xA8CF, '.', '.', "_PackBits"},
    {0xA8D0, '.', '.', "_UnpackBits"},
    {0xA8D1, '.', '.', "_StdRgn"},
    {0xA8D2, '.', '.', "_FrameRgn"},
    {0xA8D3, '.', '.', "_PaintRgn"},
    {0xA8D4, '.', '.', "_EraseRgn"},
    {0xA8D5, '.', '.', "_InverRgn"},
    {0xA8D6, '.', '.', "_FillRgn"},
    {0xA8D8, '.', '.', "_NewRgn"},
    {0xA8D9, '.', '.', "_DisposRgn"},
    {0xA8DA, '.', '.', "_OpenRgn"},
    {0xA8DB, '.', '.', "_CloseRgn"},
    {0xA8DC, '.', '.', "_CopyRgn"},
    {0xA8DD, '.', '.', "_SetEmptyRgn"},
    {0xA8DE, '.', '.', "_SetRecRgn"},
    {0xA8DF, '.', '.', "_RectRgn"},
    {0xA8E0, '.', '.', "_OfsetRgn"},
    {0xA8E1, '.', '.', "_InsetRgn"},
    {0xA8E2, '.', '.', "_EmptyRgn"},
    {0xA8E3, '.', '.', "_EqualRgn"},
    {0xA8E4, '.', '.', "_SectRgn"},
    {0xA8E5, '.', '.', "_UnionRgn"},
    {0xA8E6, '.', '.', "_DiffRgn"},
    {0xA8E7, '.', '.', "_XorRgn"},
    {0xA8E8, '.', '.', "_PtInRgn"},
    {0xA8E9, '.', '.', "_RectInRgn"},
    {0xA8EA, '.', '.', "_SetStdProcs"},
    {0xA8EB, '.', '.', "_StdBits"},
    {0xA8EC, '.', '.', "_CopyBits"},
    {0xA8ED, '.', '.', "_StdTxMeas"},
    {0xA8EE, '.', '.', "_StdGetPic"},
    {0xA8EF, '.', '.', "_ScrollRect"},
    {0xA8F0, '.', '.', "_StdPutPic"},
    {0xA8F1, '.', '.', "_StdComment"},
    {0xA8F2, '.', '.', "_PicComment"},
    {0xA8F3, '.', '.', "_OpenPicture"},
    {0xA8F4, '.', '.', "_ClosePicture"},
    {0xA8F5, '.', '.', "_KillPicture"},
    {0xA8F6, '.', '.', "_DrawPicture"},
    {0xA8F8, '.', '.', "_ScalePt"},
    {0xA8F9, '.', '.', "_MapPt"},
    {0xA8FA, '.', '.', "_MapRect"},
    {0xA8FB, '.', '.', "_MapRgn"},
    {0xA8FC, '.', '.', "_MapPoly"},
    {0xA8FE, '.', '.', "_InitFonts"},
    {0xA8FF, '.', '.', "_GetFName"},
    {0xA900, '.', '.', "_GetFNum"},
    {0xA901, '.', '.', "_FMSwapFont"},
    {0xA902, '.', '.', "_RealFont"},
    {0xA903, '.', '.', "_SetFontLock"},
    {0xA904, '.', '.', "_DrawGrowIcon"},
    {0xA905, '.', '.', "_DragGrayRgn"},
    {0xA906, '.', '.', "_NewString"},
    {0xA907, '.', '.', "_SetString"},
    {0xA908, '.', '.', "_ShowHide"},
    {0xA909, '.', '.', "_CalcVis"},
    {0xA90A, '.', '.', "_CalcVBehind"},
    {0xA90B, '.', '.', "_ClipAbove"},
    {0xA90C, '.', '.', "_PaintOne"},
    {0xA90D, '.', '.', "_PaintBehind"},
    {0xA90E, '.', '.', "_SaveOld"},
    {0xA90F, '.', '.', "_DrawNew"},
    {0xA910, '.', '.', "_GetWMgrPort"},
    {0xA911, '.', '.', "_CheckUpdate"},
    {0xA912, '.', '.', "_InitWindows"},
    {0xA913, '.', '.', "_NewWindow"},
    {0xA914, '.', '.', "_DisposWindow"},
    {0xA915, '.', '.', "_ShowWindow"},
    {0xA916, '.', '.', "_HideWindow"},
    {0xA917, '.', '.', "_GetWRefCon"},
    {0xA918, '.', '.', "_SetWRefCon"},
    {0xA919, '.', '.', "_GetWTitle"},
    {0xA91A, '.', '.', "_SetWTitle"},
    {0xA91B, '.', '.', "_MoveWindow"},
    {0xA91C, '.', '.', "_HiliteWindow"},
    {0xA91D, '.', '.', "_SizeWindow"},
    {0xA91E, '.', '.', "_TrackGoAway"},
    {0xA91F, '.', '.', "_SelectWindow"},
    {0xA920, '.', '.', "_BringToFront"},
    {0xA921, '.', '.', "_SendBehind"},
    {0xA922, '.', '.', "_BeginUpdate"},
    {0xA923, '.', '.', "_EndUpdate"},
    {0xA924, '.', '.', "_FrontWindow"},
    {0xA925, '.', '.', "_DragWindow"},
    {0xA926, '.', '.', "_DragTheRgn"},
    {0xA927, '.', '.', "_InvalRgn"},
    {0xA928, '.', '.', "_InvalRect"},
    {0xA929, '.', '.', "_ValidRgn"},
    {0xA92A, '.', '.', "_ValidRect"},
    {0xA92B, '.', '.', "_GrowWindow"},
    {0xA92C, '.', '.', "_FindWindow"},
    {0xA92D, '.', '.', "_CloseWindow"},
    {0xA92E, '.', '.', "_SetWindowPic"},
    {0xA92F, '.', '.', "_GetWindowPic"},
    {0xA930, '.', '.', "_InitMenus"},
    {0xA931, '.', '.', "_NewMenu"},
    {0xA932, '.', '.', "_DisposMenu"},
    {0xA933, '.', '.', "_AppendMenu"},
    {0xA934, '.', '.', "_ClearMenuBar"},
    {0xA935, '.', '.', "_InsertMenu"},
    {0xA936, '.', '.', "_DeleteMenu"},
    {0xA937, '.', '.', "_DrawMenuBar"},
    {0xA938, '.', '.', "_HiliteMenu"},
    {0xA939, '.', '.', "_EnableItem"},
    {0xA93A, '.', '.', "_DisableItem"},
    {0xA93B, '.', '.', "_GetMenuBar"},
    {0xA93C, '.', '.', "_SetMenuBar"},
    {0xA93D, '.', '.', "_MenuSelect"},
    {0xA93E, '.', '.', "_MenuKey"},
    {0xA93F, '.', '.', "_GetItmIcon"},
    {0xA940, '.', '.', "_SetItmIcon"},
    {0xA941, '.', '.', "_GetItmStyle"},
    {0xA942, '.', '.', "_SetItmStyle"},
    {0xA943, '.', '.', "_GetItmMark"},
    {0xA944, '.', '.', "_SetItmMark"},
    {0xA945, '.', '.', "_CheckItem"},
    {0xA946, '.', '.', "_GetItem"},
    {0xA947, '.', '.', "_SetItem"},
    {0xA948, '.', '.', "_CalcMenuSize"},
    {0xA949, '.', '.', "_GetMHandle"},
    {0xA94A, '.', '.', "_SetMFlash"},
    {0xA94B, '.', '.', "_PlotIcon"},
    {0xA94C, '.', '.', "_FlashMenuBar"},
    {0xA94D, '.', '.', "_AddResMenu"},
    {0xA94E, '.', '.', "_PinRect"},
    {0xA94F, '.', '.', "_DeltaPoint"},
    {0xA950, '.', '.', "_CountMItems"},
    {0xA951, '.', '.', "_InsertResMenu"},
    {0xA954, '.', '.', "_NewControl"},
    {0xA955, '.', '.', "_DisposControl"},
    {0xA956, '.', '.', "_KillControls"},
    {0xA957, '.', '.', "_ShowControl"},
    {0xA958, '.', '.', "_HideControl"},
    {0xA959, '.', '.', "_MoveControl"},
    {0xA95A, '.', '.', "_GetCRefCon"},
    {0xA95B, '.', '.', "_SetCRefCon"},
    {0xA95C, '.', '.', "_SizeControl"},
    {0xA95D, '.', '.', "_HiliteControl"},
    {0xA95E, '.', '.', "_GetCTitle"},
    {0xA95F, '.', '.', "_SetCTitle"},
    {0xA960, '.', '.', "_GetCtlValue"},
    {0xA961, '.', '.', "_GetMinCtl"},
    {0xA962, '.', '.', "_GetMaxCtl"},
    {0xA963, '.', '.', "_SetCtlValue"},
    {0xA964, '.', '.', "_SetMinCtl"},
    {0xA965, '.', '.', "_SetMaxCtl"},
    {0xA966, '.', '.', "_TestControl"},
    {0xA967, '.', '.', "_DragControl"},
    {0xA968, '.', '.', "_TrackControl"},
    {0xA969, '.', '.', "_DrawControls"},
    {0xA96A, '.', '.', "_GetCtlAction"},
    {0xA96B, '.', '.', "_SetCtlAction"},
    {0xA96C, '.', '.', "_FindControl"},
    {0xA96E, '.', '.', "_Dequeue"},
    {0xA96F, '.', '.', "_Enqueue"},
    {0xA970, '.', '.', "_GetNextEvent"},
    {0xA971, '.', '.', "_EventAvail"},
    {0xA972, '.', '.', "_GetMouse"},
    {0xA973, '.', '.', "_StillDown"},
    {0xA974, '.', '.', "_Button"},
    {0xA975, '.', '.', "_TickCount"},
    {0xA976, '.', '.', "_GetKeys"},
    {0xA977, '.', '.', "_WaitMouseUp"},
    {0xA979, '.', '.', "_CouldDialog"},
    {0xA97A, '.', '.', "_FreeDialog"},
    {0xA97B, '.', '.', "_InitDialogs"},
    {0xA97C, '.', '.', "_GetNewDialog"},
    {0xA97D, '.', '.', "_NewDialog"},
    {0xA97E, '.', '.', "_SelIText"},
    {0xA97F, '.', '.', "_IsDialogEvent"},
    {0xA980, '.', '.', "_DialogSelect"},
    {0xA981, '.', '.', "_DrawDialog"},
    {0xA982, '.', '.', "_CloseDialog"},
    {0xA983, '.', '.', "_DisposDialog"},
    {0xA985, '.', '.', "_Alert"},
    {0xA986, '.', '.', "_StopAlert"},
    {0xA987, '.', '.', "_NoteAlert"},
    {0xA988, '.', '.', "_CautionAlert"},
    {0xA989, '.', '.', "_CouldAlert"},
    {0xA98A, '.', '.', "_FreeAlert"},
    {0xA98B, '.', '.', "_ParamText"},
    {0xA98C, '.', '.', "_ErrorSound"},
    {0xA98D, '.', '.', "_GetDItem"},
    {0xA98E, '.', '.', "_SetDItem"},
    {0xA98F, '.', '.', "_SetIText"},
    {0xA990, '.', '.', "_GetIText"},
    {0xA991, '.', '.', "_ModalDialog"},
    {0xA992, '.', '.', "_DetachResource"},
    {0xA993, '.', '.', "_SetResPurge"},
    {0xA994, '.', '.', "_CurResFile"},
    {0xA995, '.', '.', "_InitResources"},
    {0xA996, '.', '.', "_RsrcZoneInit"},
    {0xA997, '.', '.', "_OpenResFile"},
    {0xA998, '.', '.', "_UseResFile"},
    {0xA999, '.', '.', "_UpdateResFile"},
    {0xA99A, '.', '.', "_CloseResFile"},
    {0xA99B, '.', '.', "_SetResLoad"},
    {0xA99C, '.', '.', "_CountResources"},
    {0xA99D, '.', '.', "_GetIndResource"},
    {0xA99E, '.', '.', "_CountTypes"},
    {0xA99F, '.', '.', "_GetIndType"},
    {0xA9A0, '.', '.', "_GetResource"},
    {0xA9A1, '.', '.', "_GetNamedResource"},
    {0xA9A2, '.', '.', "_LoadResource"},
    {0xA9A3, '.', '.', "_ReleaseResource"},
    {0xA9A4, '.', '.', "_HomeResFile"},
    {0xA9A5, '.', '.', "_SizeRsrc"},
    {0xA9A6, '.', '.', "_GetResAttrs"},
    {0xA9A7, '.', '.', "_SetResAttrs"},
    {0xA9A8, '.', '.', "_GetResInfo"},
    {0xA9A9, '.', '.', "_SetResInfo"},
    {0xA9AA, '.', '.', "_ChangedResource"},
    {0xA9AB, '.', '.', "_AddResource"},
    {0xA9AC, '.', '.', "_AddReference"},
    {0xA9AD, '.', '.', "_RmveResource"},
    {0xA9AE, '.', '.', "_RmveReference"},
    {0xA9AF, '.', '.', "_ResError"},
    {0xA9B0, '.', '.', "_WriteResource"},
    {0xA9B1, '.', '.', "_CreateResFile"},
    {0xA9B2, '.', '.', "_SystemEvent"},
    {0xA9B3, '.', '.', "_SystemClick"},
    {0xA9B4, '.', '.', "_SystemTask"},
    {0xA9B5, '.', '.', "_SystemMenu"},
    {0xA9B6, '.', '.', "_OpenDeskAcc"},
    {0xA9B7, '.', '.', "_CloseDeskAcc"},
    {0xA9B8, '.', '.', "_GetPattern"},
    {0xA9B9, '.', '.', "_GetCursor"},
    {0xA9BA, '.', '.', "_GetString"},
    {0xA9BB, '.', '.', "_GetIcon"},
    {0xA9BC, '.', '.', "_GetPicture"},
    {0xA9BD, '.', '.', "_GetNewWindow"},
    {0xA9BE, '.', '.', "_GetNewControl"},
    {0xA9BF, '.', '.', "_GetRMenu"},
    {0xA9C0, '.', '.', "_GetNewMBar"},
    {0xA9C1, '.', '.', "_UniqueID"},
    {0xA9C2, '.', '.', "_SysEdit"},
    {0xA9C6, '.', '.', "_Secs2Date"},
    {0xA9C7, '.', '.', "_Date2Secs"},
    {0xA9C8, '.', '.', "_SysBeep"},
    {0xA9C9, '.', '.', "_SysError"},
    {0xA9CA, '.', '.', "_PutIcon"},
    {0xA9CB, '.', '.', "_TEGetText"},
    {0xA9CC, '.', '.', "_TEInit"},
    {0xA9CD, '.', '.', "_TEDispose"},
    {0xA9CE, '.', '.', "_TextBox"},
    {0xA9CF, '.', '.', "_TESetText"},
    {0xA9D0, '.', '.', "_TECalText"},
    {0xA9D1, '.', '.', "_TESetSelect"},
    {0xA9D2, '.', '.', "_TENew"},
    {0xA9D3, '.', '.', "_TEUpdate"},
    {0xA9D4, '.', '.', "_TEClick"},
    {0xA9D5, '.', '.', "_TECopy"},
    {0xA9D6, '.', '.', "_TECut"},
    {0xA9D7, '.', '.', "_TEDelete"},
    {0xA9D8, '.', '.', "_TEActivate"},
    {0xA9D9, '.', '.', "_TEDeactivate"},
    {0xA9DA, '.', '.', "_TEIdle"},
    {0xA9DB, '.', '.', "_TEPaste"},
    {0xA9DC, '.', '.', "_TEKey"},
    {0xA9DD, '.', '.', "_TEScroll"},
    {0xA9DE, '.', '.', "_TEInsert"},
    {0xA9DF, '.', '.', "_TESetJust"},
    {0xA9E0, '.', '.', "_Munger"},
    {0xA9E1, '.', '.', "_HandToHand"},
    {0xA9E2, '.', '.', "_PtrToXHand"},
    {0xA9E3, '.', '.', "_PtrToHand"},
    {0xA9E4, '.', '.', "_HandAndHand"},
    {0xA9E5, '.', '.', "_InitPack"},
    {0xA9E6, '.', '.', "_InitAllPacks"},
    {0xA9E7, '.', '.', "_Pack0"},
    {0xA9E8, '.', '.', "_Pack1"},
    {0xA9E9, '.', '.', "_Pack2"},  //(Disk Initialization)"},
    {0xA9EA, '.', '.', "_Pack3"},  //(Standard File Package)"},
    {0xA9EB, '.', '.', "_Pack4"},  //(FP68K)"},
    {0xA9EC, '.', '.', "_Pack5"},  //(Elems68K)"},
    {0xA9ED, '.', '.', "_Pack6"},  //(Intl Utilites Package)"},
    {0xA9EE, '.', '.', "_Pack7"},  //(Bin/Dec Conversion)"},
    {0xA9EF, '.', '.', "_PtrAndHand"},
    {0xA9F0, '.', '.', "_LoadSeg "},
    {0xA9F1, '.', '.', "_UnloadSeg"},
    {0xA9F2, '.', '.', "_Launch"},
    {0xA9F3, '.', '.', "_Chain"},
    {0xA9F4, '.', '.', "_ExitToShell"},
    {0xA9F5, '.', '.', "_GetAppParms"},
    {0xA9F6, '.', '.', "_GetResFileAttrs"},
    {0xA9F7, '.', '.', "_SetResFileAttrs"},
    {0xA9F9, '.', '.', "_InfoScrap"},
    {0xA9FA, '.', '.', "_UnlodeScrap"},
    {0xA9FB, '.', '.', "_LodeScrap"},
    {0xA9FC, '.', '.', "_ZeroScrap"},
    {0xA9FD, '.', '.', "_GetScrap"},
    {0xA9FE, '.', '.', "_PutScrap"},
    {0xA9FF, '.', '.', "_MacsBug"},
    {0xABFF, '.', '.', "_MacsBugStr"},
    {0xA050, 'M', 'K', "_RelString"},
    {0xA051, '.', '.', "_ReadXPram"},
    {0xA052, '.', '.', "_WriteXPram"},
    {0xA053, '.', '.', "_ClkNoMem"},
    {0xA058, '.', '.', "_InsTime"},
    {0xA059, '.', '.', "_RmvTime"},
    {0xA05A, '.', '.', "_PrimeTime"},
    {0xA060, 'H', 'A', "_HFSDispatch"},
    {0xA061, '.', 'S', "_MaxBlock"},
    {0xA062, '.', 'S', "_PurgeSpace"},
    {0xA063, '.', '.', "_MaxApplZone"},
    {0xA064, '.', '.', "_MoveHHi"},
    {0xA065, '.', '.', "_StackSpace"},
    {0xA166, '.', 'S', "_NewEmptyHandle"},
    {0xA067, '.', '.', "_HSetRBit"},
    {0xA068, '.', '.', "_HClrRBit"},
    {0xA069, '.', '.', "_HGetState"},
    {0xA06A, '.', '.', "_HSetState"},
    {0xA06C, '.', '.', "_InitFS"},
    {0xA06D, '.', '.', "_InitEvents"},
    {0xA80D, '.', '.', "_Count1Resources"},
    {0xA80E, '.', '.', "_Get1IndResource"},
    {0xA80F, '.', '.', "_Get1IndType"},
    {0xA810, '.', '.', "_Unique1ID"},
    {0xA811, '.', '.', "_TESelView"},
    {0xA812, '.', '.', "_TEPinScroll"},
    {0xA813, '.', '.', "_TEAutoView"},
    {0xA815, '.', '.', "_SCSIDispatch"},
    {0xA816, '.', '.', "_Pack8"},
    {0xA817, '.', '.', "_CopyMask"},
    {0xA818, '.', '.', "_FixAtan2"},
    {0xA819, '.', '.', "_XMunger"},
    {0xA81A, '.', '.', "_GetZone"},
    {0xA81B, '.', '.', "_SetZone"},
    {0xA81C, '.', '.', "_Count1Types"},
    {0xA81D, '.', 'S', "_MaxMem"},
    {0xA81E, 'C', 'S', "_NewPtr"},
    {0xA81F, '.', '.', "_Get1Resource"},
    {0xA820, '.', '.', "_Get1NamedResource"},
    {0xA821, '.', '.', "_MaxSizeRsrc"},
    {0xA822, 'C', 'S', "_NewHandle"},
    {0xA823, '.', '.', "_DisposHandle"},
    {0xA824, '.', '.', "_SetHandleSize"},
    {0xA825, '.', '.', "_GetHandleSize"},
    {0xA826, '.', '.', "_InsMenuItem"},
    {0xA827, '.', '.', "_HideDItem"},
    {0xA828, '.', '.', "_ShowDItem"},
    {0xA829, '.', '.', "_HLock"},
    {0xA82A, '.', '.', "_HUnlock"},
    {0xA82B, '.', '.', "_Pack9"},
    {0xA82C, '.', '.', "_Pack10"},
    {0xA82D, '.', '.', "_Pack11"},
    {0xA82E, '.', '.', "_Pack12"},
    {0xA82F, '.', '.', "_Pack13"},
    {0xA830, '.', '.', "_Pack14"},
    {0xA831, '.', '.', "_Pack15"},
    {0xA832, '.', '.', "_FlushEvents"},
    {0xA833, '.', '.', "_ScrnBitMap"},
    {0xA834, '.', '.', "_SetFScaleDisable"},
    {0xA835, '.', '.', "_FontMetrics"},
    {0xA836, '.', '.', "_GetMaskTable"},
    {0xA837, '.', '.', "_MeasureText"},
    {0xA838, '.', '.', "_CalcMask"},
    {0xA839, '.', '.', "_SeedFill"},
    {0xA83A, '.', '.', "_ZoomWindow"},
    {0xA83B, '.', '.', "_TrackBox"},
    {0xA83F, '.', '.', "_Long2Fix"},
    {0xA840, '.', '.', "_Fix2Long"},
    {0xA841, '.', '.', "_Fix2Frac"},
    {0xA842, '.', '.', "_Frac2Fix"},
    {0xA843, '.', '.', "_Fix2X"},
    {0xA844, '.', '.', "_X2Fix"},
    {0xA845, '.', '.', "_Frac2X"},
    {0xA846, '.', '.', "_X2Frac"},
    {0xA847, '.', '.', "_FracCos"},
    {0xA848, '.', '.', "_FracSin"},
    {0xA849, '.', '.', "_FracSqrt"},
    {0xA84A, '.', '.', "_FracMul"},
    {0xA84B, '.', '.', "_FracDiv"},
    {0xA84C, '.', 'S', "_CompactMem"},
    {0xA84D, '.', '.', "_FixDiv"},
    {0xA952, '.', '.', "_DelMenuItem"},
    {0xA953, '.', '.', "_UpdtControls"},
    {0xA978, '.', '.', "_UpdtDialog"},
    {0xA984, '.', '.', "_FindDItem"},
    {0xA9C4, '.', '.', "_OpenRFPerm"},
    {0xA9C5, '.', '.', "_RsrcMapEntry"},
    {0xA9F8, '.', '.', "_MethodDispatch"},

// "Sys 4.1"
    {0xA055, '.', '.', "_StripAddress"},
    {0xA808, '.', '.', "_InitProcMe"},
    {0xA809, '.', '.', "_GetCVariant"},
    {0xA80A, '.', '.', "_GetWVariant"},
    {0xA80B, '.', '.', "PopUpMenuSelect"},
    {0xA80C, '.', '.', "_RGetResource"},
    {0xA834, '.', '.', "_SetFScaleEnable"},
    {0xA83C, '.', '.', "_TEGetOffset"},
    {0xA83D, '.', '.', "_TEDispatch"},
    {0xA84E, '.', '.', "_GetItemCmd"},
    {0xA84F, '.', '.', "_SetItemCmd"},
    {0xA895, '.', '.', "_ShutDown"},
    {0xA8B5, '.', '.', "_ScriptUtil"},
    {0xA8FD, '.', '.', "_PrGlue"},

// "Mac II/SE"
    {0xA05B, '.', '.', "_PowerOff"},
    {0xA05D, '.', '.', "_SwapMMUMode"},
    {0xA06E, '.', '.', "_SlotManager"},
    {0xA06F, '.', '.', "_SlotVInstall"},
    {0xA070, '.', '.', "_SlotVRemove"},
    {0xA071, '.', '.', "AttachVBL"},
    {0xA072, '.', '.', "DoVBLTask"},
    {0xA075, '.', '.', "SIntInstall"},
    {0xA076, '.', '.', "SIntRemove"},
    {0xA077, '.', '.', "CountADBs"},
    {0xA078, '.', '.', "GetIndADB"},
    {0xA079, '.', '.', "GetADBInfo"},
    {0xA07A, '.', '.', "SetADBInfo"},
    {0xA07B, '.', '.', "ADBReInit"},
    {0xA07C, '.', '.', "ADBOp"},
    {0xA07D, '.', '.', "_GetDefStartup"},
    {0xA07E, '.', '.', "_SetDefStartup"},
    {0xA07F, '.', '.', "_InternalWa"},
    {0xA080, '.', '.', "_GetVidDefault"},
    {0xA081, '.', '.', "_SetVidDefault"},
    {0xA082, '.', '.', "_DTInstall"},
    {0xA083, '.', '.', "SetOSDefault"},
    {0xA084, '.', '.', "_GetOSDefault"},
    {0xA801, '.', '.', "_SndDispose"},
    {0xA802, '.', '.', "_SndAddModi"},
    {0xA803, '.', '.', "_SndDoCommand"},
    {0xA804, '.', '.', "_SndDoImmed"},
    {0xA805, '.', '.', "_SndPlay"},
    {0xA806, '.', '.', "_SndControl"},
    {0xA807, '.', '.', "_SndNewChannel"},
    {0xAA00, '.', '.', "_OpenCPort"},
    {0xAA01, '.', '.', "_InitCPort"},
    {0xAA02, '.', '.', "_CloseCPort"},
    {0xAA03, '.', '.', "_NewPixMap"},
    {0xAA04, '.', '.', "_DisposPixMap"},
    {0xAA05, '.', '.', "_CopyPixMap"},
    {0xAA06, '.', '.', "_SetCPortPix"},
    {0xAA07, '.', '.', "_NewPixPat"},
    {0xAA08, '.', '.', "_DisposPixPat"},
    {0xAA09, '.', '.', "_CopyPixPat"},
    {0xAA0A, '.', '.', "_PenPixPat"},
    {0xAA0B, '.', '.', "_BackPixPat"},
    {0xAA0C, '.', '.', "_GetPixPat"},
    {0xAA0D, '.', '.', "_MakeRGBPat"},
    {0xAA0E, '.', '.', "_FillCRect"},
    {0xAA0F, '.', '.', "_FillCOval"},
    {0xAA10, '.', '.', "_FillCRoundRect"},
    {0xAA11, '.', '.', "_FillCArc"},
    {0xAA12, '.', '.', "_FillCRgn"},
    {0xAA13, '.', '.', "_FillCPoly"},
    {0xAA14, '.', '.', "_RGBForeColor"},
    {0xAA15, '.', '.', "_RGBBackColor"},
    {0xAA16, '.', '.', "_SetCPixel"},
    {0xAA17, '.', '.', "_GetCPixel"},
    {0xAA18, '.', '.', "_GetCTable"},
    {0xAA19, '.', '.', "_GetForeColor"},
    {0xAA1A, '.', '.', "_GetBackColor"},
    {0xAA1B, '.', '.', "_GetCCursor"},
    {0xAA1C, '.', '.', "_SetCCursor"},
    {0xAA1D, '.', '.', "_AllocCursor"},
    {0xAA1E, '.', '.', "_GetCIcon"},
    {0xAA1F, '.', '.', "_PlotCIcon"},
    {0xAA21, '.', '.', "_OpColor"},
    {0xAA22, '.', '.', "_HiliteColor"},
    {0xAA23, '.', '.', "_CharExtra"},
    {0xAA24, '.', '.', "_DisposCTab"},
    {0xAA25, '.', '.', "_DisposCIcon"},
    {0xAA26, '.', '.', "_DisposCCursor"},
    {0xAA27, '.', '.', "_GetMaxDevice"},
    {0xAA28, '.', '.', "_GetCTSeed"},
    {0xAA29, '.', '.', "_GetDeviceL"},
    {0xAA2A, '.', '.', "_GetMainDevice"},
    {0xAA2B, '.', '.', "_GetNextDevice"},
    {0xAA2C, '.', '.', "_TestDevice"},
    {0xAA2D, '.', '.', "_SetDeviceA"},
    {0xAA2E, '.', '.', "_InitGDevice"},
    {0xAA2F, '.', '.', "_NewGDevice"},
    {0xAA30, '.', '.', "_DisposGDevice"},
    {0xAA31, '.', '.', "_SetGDevice"},
    {0xAA32, '.', '.', "_GetGDevice"},
    {0xAA33, '.', '.', "_Color2Index"},
    {0xAA34, '.', '.', "_Index2Color"},
    {0xAA35, '.', '.', "_InvertColor"},
    {0xAA36, '.', '.', "_RealColor"},
    {0xAA37, '.', '.', "_GetSubTable"},
    {0xAA39, '.', '.', "_MakeITable"},
    {0xAA3A, '.', '.', "_AddSearch"},
    {0xAA3B, '.', '.', "_AddComp"},
    {0xAA3C, '.', '.', "_SetClientI"},
    {0xAA3D, '.', '.', "_ProtectEntry"},
    {0xAA3E, '.', '.', "_ReserveEntry"},
    {0xAA3F, '.', '.', "_SetEntries"},
    {0xAA40, '.', '.', "_QDError"},
    {0xAA41, '.', '.', "_SetWinColor"},
    {0xAA42, '.', '.', "_GetAuxWin"},
    {0xAA43, '.', '.', "_SetCtlColor"},
    {0xAA44, '.', '.', "_GetAuxCtl"},
    {0xAA45, '.', '.', "_NewCWindow"},
    {0xAA46, '.', '.', "_GetNewCWindow"},
    {0xAA47, '.', '.', "_SetDeskCPat"},
    {0xAA48, '.', '.', "_GetCWMgrPort"},
    {0xAA49, '.', '.', "_SaveEntries"},
    {0xAA4A, '.', '.', "_RestoreEntries"},
    {0xAA4B, '.', '.', "_NewCDialog"},
    {0xAA4C, '.', '.', "_DelSearch"},
    {0xAA4D, '.', '.', "_DelComp"},
    {0xAA4E, '.', '.', "_SetStdCProcs"},
    {0xAA4F, '.', '.', "_CalcCMask"},
    {0xAA50, '.', '.', "_SeedCFill"},
    {0xAA60, '.', '.', "_DelMCEntries"},
    {0xAA61, '.', '.', "_GetMCInfo"},
    {0xAA62, '.', '.', "_SetMCInfo"},
    {0xAA63, '.', '.', "_DispMCInfo"},
    {0xAA64, '.', '.', "_GetMCEntry"},
    {0xAA65, '.', '.', "_SetMCEntries"},
    {0xAA66, '.', '.', "_MenuChoice"},
    {0xAA90, '.', '.', "_InitPalette"},
    {0xAA91, '.', '.', "_NewPalette"},
    {0xAA92, '.', '.', "_GetNewPalette"},
    {0xAA93, '.', '.', "_DisposePalette"},
    {0xAA94, '.', '.', "_ActivatePalette"},
    {0xAA95, '.', '.', "_SetPalette"},
    {0xAA96, '.', '.', "_GetPalette"},
    {0xAA97, '.', '.', "_PmForeColor"},
    {0xAA98, '.', '.', "_PmBackColor"},
    {0xAA99, '.', '.', "_AnimateEntry"},
    {0xAA9A, '.', '.', "_AnimatePalette"},
    {0xAA9B, '.', '.', "_GetEntryColor"},
    {0xAA9C, '.', '.', "_SetEntryColor"},
    {0xAA9D, '.', '.', "_GetEntryUsage"},
    {0xAA9E, '.', '.', "_SetEntryUsage"},
    {0xAA9F, '.', '.', "_CTab2Palette"},
    {0xAAA0, '.', '.', "_Palette2CTab"},

// "Juggler"
    {0xA860, '.', '.', "_WaitNextEvent"},
    {0xA88F, '.', '.', "_JugglerDispatch"},

    {0x0000, '.', '.', ""}
};
#endif // MAC_A_TRAPS


uint16_t Dis68K::FetchWord(int addr, int &len)
{
    uint16_t w;

    w = ReadWord(addr + len);
    len += 2;
    return w;
}


addr_t Dis68K::FetchLong(int addr, int &len)
{
    addr_t l;

    l = ReadLong(addr + len);
    len += 4;
    return l;
}


int Dis68K::GetBits(int num, int pos, int len)
{
    return (num >> pos) & (0xFFFF >> (16-len));
}


addr_t Dis68K::FetchSize(int addr, int &len, int size)
{
    switch (size % 4) {
        case s_Byte:
            return FetchWord(addr, len) & 0xFF;

        case s_Word:
            return FetchWord(addr, len);

        case s_Long:
            return FetchLong(addr, len);

        default:
            break;
    }
    return 0;
}


// NOTE: MOVE can have two refaddrs, but only the last one will become the refaddr
// The instructions that have this problem should only be ones that were removed for Coldfire

void Dis68K::AddrMode(int mode, int reg, int size, char *parms, int addr, int &len, bool &invalid,
                      int &lfref, addr_t &refaddr)
{
    int     n, reg2;
    char    r;
    const char *sz;
    char    s[256];
    addr_t  ra;
    int     addr0 = addr + len; // address of extension word
    int     hint = rom.get_hint(addr);

//    if (size == 3 || invalid) {
//        invalid = true;
//    } else
    switch (mode) {
        case 0: // 000 nnn = Dn
            sprintf(parms, "D%d", reg);
            break;

        case 1: // 001 nnn = An
            sprintf(parms, "A%d", reg);
            break;

        case 2: // 010 nnn = (An)
            sprintf(parms, "(A%d)", reg);
            break;

        case 3: // 011 nnn = (An)+
            sprintf(parms, "(A%d)+", reg);
            break;

        case 4: // 100 nnn = -(A0)
            sprintf(parms, "-(A%d)", reg);
            break;

        case 5: // 101 nnn xxxx = d16(An)
            n = FetchWord(addr, len);
            if (n > 32767) {
                sprintf(parms, "-$%.4X(A%d)", 65536-n, reg);
            } else {
                sprintf(parms, "$%.4X(A%d)", n, reg);
            }
            break;

        case 6: // 110 nnn xxxx = d8(An,Xn)
            n = FetchWord(addr, len);
            if (n & 0x8000) {
                r = 'A';
            } else {
                r = 'D';
            }
            if (n & 0x0800) {
                sz = ".L";
            } else {
                sz = ".W";
            }
            reg2 = GetBits(n, 12, 3);

            if (((n & 0x0108) == 0x0100) && _subtype >= CPU_68020) { // "full" extension word

                bool fallthrough = false;
                if (n & 0x0100) { // full extension flag

                    bool bs = (n >> 7) & 1;
                    // bool is = (n >> 6) & 1;
                    int bds = (n >> 4) & 3;
                    // int iis = n & 7;

                    ra = 0;
                    switch (bds) {
                        case 0: invalid = true; break;
                        case 1:                 break;
                        case 2: ra = FetchWord(addr, len); break;
                        case 3: ra = FetchLong(addr, len); break;
                    }

                    // IS bit with I/IS bits
                    switch (((n & 0x40) >> 2) | (n & 7)) {

                        case 0x00: // no memory indirect action
                        case 0x10: // no memory indirect action
                        // (these need to just do the short extension stuff?)
                            fallthrough = true;
                            break;
// these are really complicated
#if 0
                        case 0x01: // indirect preindexed with null outer displacement
//                          ra = FetchWord(addr, len);
//                          lfref &= ~CODEREF; // not a code address if it's indexed!
                            RefStr(ra, s, lfref, refaddr);
                            sprintf(parms, "([%s])", s);
                            break;
#endif
#if 0
                        case 0x02: // indirect preindexed with word outer displacement
                        case 0x03: // indirect preindexed with long outer displacement
                        case 0x04: // reserved
                        case 0x05: // indirect postindexed with null outer displacement
                        case 0x06: // indirect postindexed with word outer displacement
                        case 0x07: // indirect postindexed with long outer displacement
#endif
                        case 0x11: // -> memory indirect with null outer displacement
                            RefStr(ra, s, lfref, refaddr);
//                          lfref &= ~CODEREF; // not a code address if it's indexed!
                            if (bs) sprintf(parms, "([%s])", s);
                               else sprintf(parms, "([%s,A%d])", s, reg);
                            break;
#if 0
                        case 0x12: // memory indirect with word outer displacement
                        case 0x13: // memory indirect with long outer displacement
#endif
                        // 14-17 reserved
                        default:
                            // specific combination not yet implemented!
                            invalid = true;
                        break;
                    }
                }
                if (!fallthrough)
                    break;
            }

            // bit 8 is zero, check for 68020 scale factor
            if ((n & 0x0600) && _subtype >= CPU_68020) {
                int scale = GetBits(n, 9, 2);
                scale = 1 << scale;
                n = n & 0xFF;
                if (n >= 128) {
                    n = 256-n;
                    sprintf(parms, "-$%.2X(A%d,%c%d%s*%d)", n, reg, r, reg2, sz, scale);
                } else {
                    sprintf(parms, "$%.2X(A%d,%c%d%s*%d)", n, reg, r, reg2, sz, scale);
                }
                break;
            }

            // flag errors in extension word on 68000/68010
            // these bits are ignored by 68000, but can't be re-assembled
            // instead of setting illegal, a flag character is added
            if (GetBits(n, 8, 3)) {
#ifdef LAZY_EXTWORD
                // invalid bits in extension word were set
                strcat(parms, "!");
                lfref |= RIPSTOP;
                parms++;
#else
                invalid = true;
                break;
#endif
            }

            n = n & 0xFF;
            if (n >= 128) {
                n = 256-n;
                sprintf(parms, "-$%.2X(A%d,%c%d%s)", n, reg, r, reg2, sz);
            } else {
                sprintf(parms, "$%.2X(A%d,%c%d%s)", n, reg, r, reg2, sz);
            }
            break;

        case 7:
            switch (reg) {
                case 0: // 111 000 = abs.W
                    n = FetchWord(addr, len);
                    if (n & 0x8000) {
                        n |= 0xFF0000;
                    }
                    ra = n;
                    RefStr(ra, s, lfref, refaddr);
                    sprintf(parms, "%s.W", s);
                    break;

                case 1: // 111 001 = abs.L
                    ra = FetchLong(addr, len) & 0x00FFFFFF;
                    RefStr(ra, s, lfref, refaddr);
                    sprintf(parms, "%s.L", s);
                    break;

                case 2: // 111 010 = d16(PC)
                    n = FetchWord(addr, len);
                    if (n & 0x8000) {
                        n |= 0xFFFF0000;
                    }
                    ra = (addr0 + n) & 0x00FFFFFF;
                    RefStr(ra, s, lfref, refaddr);
                    sprintf(parms, "%s(PC)", s);                    
                    break;

                case 3: // 111 011 = d8(PC,Xn)
                    n = FetchWord(addr, len);
                    if (n & 0x8000) {
                        r = 'A';
                    } else {
                        r = 'D';
                    }
                    if (n & 0x0800) {
                        sz = ".L";
                    } else {
                        sz = ".W";
                    }
                    reg2 = GetBits(n, 12, 3);
                    n = n & 0xFF;
                    if (n >= 128) {
                        n = n - 256;
                    }
                    ra = (addr0 + n) & 0x00FFFFFF;
                    lfref &= ~CODEREF; // not a code address if it's indexed!
                    RefStr(ra, s, lfref, refaddr);
                    sprintf(parms, "%s(PC,%c%d%s)", s, r, reg2, sz);
                    break;

                case 4: // 111 100 = immediate
                    switch (size) {
                        case s_Byte:
                            n = FetchWord(addr, len) & 0xFF;
                            sprintf(parms, "#$%.2X", n);
                            break;

                        case s_Word:
                            n = FetchWord(addr, len);
                            sprintf(parms, "#$%.4X", n);
                            break;

                        case s_Long:
                            n = FetchLong(addr, len);
                            // parse immediate as a potential address reference
                            if (n >= 0x1000 && // arbitrary cut-off to avoid making labels from constants
                                !(hint & 1) ) { // disable reference if odd hint
                                ra = n;
                                RefStr(ra, s, lfref, refaddr);
                                sprintf(parms, "#%s", s);
                            } else {
                                // parse immediate as an immediate
                                sprintf(parms, "#$%.8X", n);
                            }
                            break;

                        case s_UseCCR:
                            strcpy(parms, "CCR");
                            break;

                        case s_UseSR:
                            strcpy(parms, "SR");
                            break;

                        default:
                            invalid = true;
                    }
                    break;

                case 5: // 111 101
                case 6: // 111 110
                case 7: // 111 111
                default:
                    invalid = true;
                    break;
            }
            break;

        default:
            invalid = true;
            break;
    }
}


void Dis68K::SrcAddr(int opcd, int size, char *parms, int addr, int &len, bool &invalid,
                      int &lfref, addr_t &refaddr)
{
    int     mode, reg;

    mode = GetBits(opcd, 3, 3);
    reg  = GetBits(opcd, 0, 3);
    AddrMode(mode, reg, size, parms, addr, len, invalid, lfref, refaddr);
}


void Dis68K::DstAddr(int opcd, int size, char *parms, int addr, int &len, bool &invalid,
                      int &lfref, addr_t &refaddr)
{
    int     mode, reg;

    mode = GetBits(opcd, 6, 3);
    reg  = GetBits(opcd, 9, 3);
    AddrMode(mode, reg, size, parms, addr, len, invalid, lfref, refaddr);
}


void Dis68K::MOVEMRegs(int mode, char *parms, int regs)
{
    const char *names;
    char    regList[32];
    bool    slashFlag;
    int     i, j;
    char    *p;

    p = parms;
    *p = 0;
    *regList = 0;

    if (mode == 4) {
        names = "A7A6A5A4A3A2A1A0D7D6D5D4D3D2D1D0";
    } else {
        names = "D0D1D2D3D4D5D6D7A0A1A2A3A4A5A6A7";
    }

    if (regs == 0) {
        strcpy(parms, "<No regs>");
    } else {
        slashFlag = false;
        i = 0;
        while (i <= 15) {
            if (regs & (1 << i)) {

                j = i;
                while ((j == i || j % 8) && (regs & (1 << j))) {
                    j++;
                }

                if (slashFlag) {
                    *p++ = '/';
                }
                slashFlag = true;

                *p++ = names[i*2];
                *p++ = names[i*2+1];

                if (i != j-1) {
                    *p++ = '-';
                    *p++ = names[j*2-2];
                    *p++ = names[j*2-1];
                }

                i = j;
            } else {
                i++;
            }
        }
    }

    *p = 0;
}


InstrPtr Dis68K::FindInstr(uint16_t opcd)
{
    InstrPtr p;

    p = M68K_opcdTable;

    while (p->andWith && ((opcd & p->andWith) != p->cmpWith)) {
        p++;
    }

    return p;
}


void Dis68K::DisATrap(uint16_t opcd, char *opcode, char *parms, addr_t addr)
{
    char *p;
    TrapPtr t;

    sprintf(parms, "$%.4X", opcd);

#ifdef MAC_A_TRAPS // change to 0 to disable Mac A-traps
    t = trapTable;
    if (opcd & 0x0800) {
        // Pascal traps
        while (t->trap && t->trap != (opcd & 0xFBFF)) {
            t++;
        }
    } else {
        // register traps
        while (t->trap && (t->trap & 0xFEFF) != (opcd & 0xF8FF)) {
            t++;
        }
    }

    if (t->trap) {
        *parms = 0;
        p = stpcpy(opcode, t->name);
        if ((opcd & 0x0800) == 0) {
            if (opcd & 0x0400) { // bit 10
                *p++ = ',';
                *p++ = t->bit_10;
                *p = 0;
            }
            if (opcd & 0x0200) { // bit 9
                *p++ = ',';
                *p++ = t->bit_9;
                *p = 0;
            }
        }
    }

    // provide human-redable translation of CODE 0 usage of _LoadSeg
    if (opcd == 0xA9F0) {
        sprintf(parms, "; $%.4X(A5) -> %2d:%.4X", (int) addr + 12,
                   ReadWord(addr - 2), ReadWord(addr - 6) + 4);
    }
#endif
}


int Dis68K::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    int         opcd;
    int         i, n;
    InstrPtr    instr;
    char        s[256], s2[256];
    int         opSize;
    addr_t      ra;

    opcode[0] = 0;
    parms[0] = 0;
    int len = 0;
    lfref   = 0;
    refaddr = 0;
    opSize  = -1;

    bool invalid = true;
    int hint = rom.get_hint(addr);

    opcd = FetchWord(addr, len);
    instr = FindInstr(opcd);

    if (!(addr & 0x01) &&               // require even alignment!
        instr && instr->typ && *(instr->op)) {
        invalid = false;
        strcpy(opcode, instr->op);
        lfref  = instr->lfref;

        switch (instr->typ) {
            case o_Implied:             // 1
                break;

            case o_Immed:               // 2
                // An mode is specifically not allowed
                if ((opcd & 0x38) == 0x08) {
                    invalid = true;
                    break;
                }
                opSize = GetBits(opcd, 6, 2);
                n = FetchSize(addr, len, opSize);
                SrcAddr(opcd, opSize+4, s, addr, len, invalid, lfref, refaddr);
                switch (opSize) {
                    case s_Byte:
                        if (n & 0xFF00) {
                            invalid = true; // byte with word parameter
                        }
                        sprintf(parms, "#$%.2X,%s", n, s);
                        break;

                    case s_Word:
                        sprintf(parms, "#$%.4X,%s", n, s);
                        break;

                    case s_Long:
                        // parse immediate as a potential address reference
                        if (n >= 0x1000 && // arbitrary cut-off to avoid making labels from constants
                            !(hint & 1) && // disable reference if odd hint
                            (n & 0xFF000000) == 0) {
                            ra = n;
                            RefStr(ra, s2, lfref, refaddr);
                            sprintf(parms, "#%s,%s", s2, s);
                        } else
                        sprintf(parms, "#$%.8X,%s", n, s);
                        break;
                }
                break;

            case o_Dn_EA:               // 3
                SrcAddr(opcd, s_Word, s, addr, len, invalid, lfref, refaddr);
                sprintf(parms, "D%d,%s", GetBits(opcd, 9, 3), s);
                break;

            case o_MOVEP:               // 4
                opSize = GetBits(opcd, 6, 1) + 1;
                AddrMode(5, GetBits(opcd, 0, 3), opSize, s, addr, len, invalid, lfref, refaddr);
                if (opcd & 0x0080) {
                    sprintf(parms, "D%d,%s", GetBits(opcd, 9, 3), s);
                } else {
                    sprintf(parms, "%s,D%d", s, GetBits(opcd, 9, 3));
                }
                break;

            case o_StatBit:             // 5
                n = FetchWord(addr, len);
                SrcAddr(opcd, s_Word, s, addr, len, invalid, lfref, refaddr);
                sprintf(parms, "#%d,%s", GetBits(n, 0, 5), s);
                break;

            case o_MOVES:               // 6
                opSize = GetBits(opcd, 6, 1) + 1;
                n = FetchWord(addr, len);

                if (n & 0x0800) {
                    SrcAddr(opcd, opSize, s, addr, len, invalid, lfref, refaddr);
                    if (n & 0x8000) {
                        sprintf(parms, "%s,A%d", s,  GetBits(n, 9, 3));
                    } else {
                        sprintf(parms, "%s,D%d", s,  GetBits(n, 9, 3));
                    }
                } else {
                    SrcAddr(opcd, opSize, s, addr, len, invalid, lfref, refaddr);
                    if (n & 0x8000) {
                        sprintf(parms, "A%d,%s", GetBits(n, 9, 3), s);
                    } else {
                        sprintf(parms, "D%d,%s", GetBits(n, 9, 3), s);
                    }
                }

                break;

            case o_MOVE:                // 7
                opSize = GetBits(opcd, 12, 2);
                if (opSize == 1) {
                    opSize = s_Byte;
                }
                if (opSize == 3) {
                    opSize = s_Word;
                }
                SrcAddr(opcd, opSize, parms, addr, len, invalid, lfref, refaddr);
                strcat(parms, ",");
                DstAddr(opcd, opSize, s, addr, len, invalid, lfref, refaddr);
                strcat(parms, s);
                break;

            case o_OneAddrSz:           // 8
                opSize = GetBits(opcd, 6, 2);
                SrcAddr(opcd, opSize, parms, addr, len, invalid, lfref, refaddr);
                break;

            case o_MoveFromSR:          // 9
                opSize = 1 - GetBits(opcd, 9, 1);
                AddrMode(7, 4, opSize+4, parms, addr, len, invalid, lfref, refaddr);
                strcat(parms, ",");
                SrcAddr(opcd, opSize, s, addr, len, invalid, lfref, refaddr);
                strcat(parms, s);
                opSize = -1;
                break;

            case o_MoveToSR:            // 10
                opSize = GetBits(opcd, 9, 1);
                SrcAddr(opcd, opSize, parms, addr, len, invalid, lfref, refaddr);
                strcat(parms, ",");
                AddrMode(7, 4, opSize+4, s, addr, len, invalid, lfref, refaddr);
                strcat(parms, s);
                opSize = -1;
                break;

            case o_EA_Dn_020:
                if (_subtype < CPU_68020) {
                    invalid = true;
                    break;
                }
                // fallthrough

            case o_EA_Dn:               // 11
                SrcAddr(opcd, s_Word, s, addr, len, invalid, lfref,  refaddr);
                sprintf(parms, "%s,D%d", s,  GetBits(opcd, 9, 3));
                break;

            case o_LEA:                 // 12
                SrcAddr(opcd, s_Word, s, addr, len, invalid, lfref,  refaddr);
                sprintf(parms, "%s,A%d", s,  GetBits(opcd, 9, 3));
                break;

            case o_OneAddr:             // 13
                SrcAddr(opcd, s_Byte, parms, addr, len, invalid, lfref, refaddr);
                break;

            case o_One_Dn_020:          // 14
                if (_subtype < CPU_68020) {
                    invalid = true;
                    break;
                }
                // fallthrough

            case o_One_Dn:              // 14
                sprintf(parms, "D%d", GetBits(opcd, 0, 3));
                break;

            case o_MOVEM:               // 15
                n = FetchWord(addr, len);
                opSize = GetBits(opcd, 6, 1) + s_Word;
                if (opcd & 0x0400) { // bit 10
                    SrcAddr(opcd, opSize, parms, addr, len, invalid, lfref, refaddr);
                    strcat(parms, ",");
                    MOVEMRegs(GetBits(opcd, 3, 3), s, n);
                    strcat(parms, s);
                } else {
                    MOVEMRegs(GetBits(opcd, 3, 3), parms, n);
                    strcat(parms, ",");
                    SrcAddr(opcd, opSize, s, addr, len, invalid, lfref, refaddr);
                    strcat(parms, s);
                }

                break;

            case o_TRAP:                // 16
                sprintf(parms, "#%d", GetBits(opcd, 0, 4));
                break;

            case o_LINK:                // 17
                n = FetchWord(addr, len);
                if (opcd & 0x0008) {
                    if (_subtype < CPU_68020) {
                        invalid = true;
                        break;
                    }
                    opSize = s_Long;
                    n = (n << 16) + FetchWord(addr, len);
                    sprintf(parms, "A%d,#-$%.8X", GetBits(opcd, 0, 3), 0xFFFFFFFF & -n);
                    break;
                }
                if (n > 32767) {
                    sprintf(parms, "A%d,#-$%.4X", GetBits(opcd, 0, 3), 65536-n);
                } else {
                    sprintf(parms, "A%d,#-$%.4X", GetBits(opcd, 0, 3), 0xFFFF & -n);
                }
                break;

            case o_UNLK:                // 18
                sprintf(parms, "A%d", GetBits(opcd, 0, 3));
                break;

            case o_MOVE_USP:            // 19
                if (opcd & 0x0008) {
                    sprintf(parms, "USP,A%d", GetBits(opcd, 0, 3));
                } else {
                    sprintf(parms, "A%d,USP", GetBits(opcd, 0, 3));
                }
                break;

            case o_MOVEC:               // 20
                n = FetchWord(addr, len);

                i = GetBits(n, 0, 12);
                switch (i) {
                    case 0x000: strcpy(s, "SFC");   break;
                    case 0x001: strcpy(s, "DFC");   break;
                    case 0x002: strcpy(s, "CACR");  break;
                    case 0x003: strcpy(s, "TC");    break;
                    case 0x004: strcpy(s, "ITT0");  break; // EC040: IACR0
                    case 0x005: strcpy(s, "ITT1");  break; // EC040: IACR1
                    case 0x006: strcpy(s, "DTT0");  break; // EC040: DACR0
                    case 0x007: strcpy(s, "DTT1");  break; // EC040: DACR1
                    case 0x800: strcpy(s, "USP");   break;
                    case 0x801: strcpy(s, "VBR");   break;
                    case 0x802: strcpy(s, "CAAR");  break;
                    case 0x803: strcpy(s, "MSP");   break;
                    case 0x804: strcpy(s, "ISP");   break;
                    case 0x805: strcpy(s, "MMUSR"); break;
                    case 0x806: strcpy(s, "URP");   break;
                    case 0x807: strcpy(s, "SRP");   break;
                    default:    sprintf(s, "r%.3X", i); // invalid = true;
                }

                if (opcd & 1) {
                    if (n & 0x8000) {
                        sprintf(parms, "A%d,%s", GetBits(n, 9, 3), s);
                    } else {
                        sprintf(parms, "D%d,%s", GetBits(n, 9, 3), s);
                    }
                } else {
                    if (n & 0x8000) {
                        sprintf(parms, "%s,A%d", s, GetBits(n, 9, 3));
                    } else {
                        sprintf(parms, "%s,D%d", s, GetBits(n, 9, 3));
                    }
                }
                break;

            case o_ADDQ_SUBQ:           // 21
                opSize = GetBits(opcd, 6, 2);
                n = GetBits(opcd, 9, 3);
                if (n == 0) {
                    n = 8;
                }
                SrcAddr(opcd, opSize, s, addr, len, invalid, lfref, refaddr);
                sprintf(parms, "#%d,%s", n, s);
                break;

            case o_DBcc:                // 22
                n = FetchWord(addr, len);
                if (n & 0x8000) {
                    n |= 0xFFFF0000;
                }
                ra = addr + 2 + n;
                RefStr(ra, s, lfref, refaddr);
                sprintf(parms, "D%d,%s", GetBits(opcd, 0, 3), s);
                // rip-stop for odd address
                if (ra & 1) {
                    lfref |= RIPSTOP;
                }
                break;

            case o_Bcc:                 // 23
                n = opcd & 0xFF;
                if (n == 0) { // word branch
                    n = FetchWord(addr, len);
                    if (n > 32767) {
                        n = n - 65536;
                    }
                    ra = addr + 2 + n;
                    RefStr(ra, parms, lfref, refaddr);
                    opSize = s_Word;
                } else if (n == 255 && _subtype >= CPU_68020) { // 68020 long branch
                    n = FetchLong(addr, len);
                    ra = addr + 2 + n;
                    RefStr(ra, parms, lfref, refaddr);
                    opSize = s_Long;
                } else if (n & 1) { // odd offset
#if 0
                    // allow disassembly but rip-stop for odd offset
                    lfref |= RIPSTOP;
#else
                    // odd offset is invalid (does 68008 even allow it?)
                    invalid = true;
#endif
                } else { // byte branch
                    if (n > 127) {
                        n = n - 256;
                    }
                    ra = addr + 2 + n;
                    RefStr(ra, parms, lfref, refaddr);
                    opSize = s_Byte;
                }
                break;

            case o_MOVEQ:               // 24
                n = opcd & 0xFF;
                if (n > 127) {
                    n = 256 - n;
                    sprintf(parms, "#-$%.2X,D%d", n, GetBits(opcd, 9, 3));
                } else {
                    sprintf(parms,  "#$%.2X,D%d", n, GetBits(opcd, 9, 3));
                }
                break;

            case o_EA_Dn_Sz:            // 25
                opSize = GetBits(opcd, 6, 2);
                SrcAddr(opcd, opSize, s, addr, len, invalid, lfref, refaddr);
                sprintf(parms, "%s,D%d", s,  GetBits(opcd, 9, 3));
                break;

            case o_Dn_EA_Sz:            // 26
                opSize = GetBits(opcd, 6, 2);
                SrcAddr(opcd, opSize, s, addr, len, invalid, lfref, refaddr);
                sprintf(parms, "D%d,%s", GetBits(opcd, 9, 3), s);
                break;

            case o_STOP:                // 27
                n = FetchWord(addr, len);
                sprintf(parms, "#$%.4X", n);
                break;

            case o_ADDX_SUBX:           // 29
                opSize = GetBits(opcd, 6, 2);
                FALLTHROUGH;
            case o_ABCD_SBCD:           // 28
                if (opcd & 0x0008) {
                    sprintf(parms, "-(A%d),-(A%d)", GetBits(opcd, 0, 3), GetBits(opcd, 9, 3));
                } else {
                    sprintf(parms, "D%d,D%d", GetBits(opcd, 0, 3), GetBits(opcd, 9, 3));
                }
                break;

            case o_CMPM:                // 30
                opSize = GetBits(opcd, 6, 2);
                sprintf(parms, "(A%d)+,(A%d)+", GetBits(opcd, 0, 3), GetBits(opcd, 9, 3));
                break;

            case o_EXG:                 // 31
                SrcAddr(opcd, 0, s, addr, len, invalid, lfref, refaddr);
                if (GetBits(opcd, 3, 5) == 9) {
                    sprintf(parms, "A%d,%s", GetBits(opcd, 9, 3), s);
                } else {
                    sprintf(parms, "D%d,%s", GetBits(opcd, 9, 3), s);
                }
                break;

            case o_ShiftRotImm:         // 32
                opSize = GetBits(opcd, 6, 2);
                n = GetBits(opcd, 9, 3);
                if (n == 0) {
                    n = 8;
                }
                sprintf(parms, "#%d,D%d", n, GetBits(opcd, 0, 3));
                break;

            case o_ShiftRotReg:         // 33
                opSize = GetBits(opcd, 6, 2);
                sprintf(parms, "D%d,D%d", GetBits(opcd, 9, 3), GetBits(opcd, 0, 3));
                break;

            case o_A_Trap:              // 34
//              sprintf(parms, "$%.4X", opcd);
                DisATrap(opcd, opcode, parms, addr);
                break;

            case o_ADDA_SUBA:           // 35
                opSize = GetBits(opcd, 8, 1) + s_Word;
                SrcAddr(opcd, opSize, s, addr, len, invalid, lfref, refaddr);
                sprintf(parms, "%s,A%d", s, GetBits(opcd, 9, 3));
                break;

// -----------------------------------------------------

            case o_BitField:
                if (_subtype < CPU_68020) {
                    invalid = true;
                    break;
                }

                // get extension word
                n = FetchWord(addr, len);

                // parse ea
                SrcAddr(opcd, opSize, parms, addr, len, invalid, lfref, refaddr);
                if (invalid) break;

                if (n & 0x8000) { // high bit of extension word must be zero
                    invalid = true;
                    break;
                } else { // {Dofs:Dwid},Dreg
                    strcat(parms, "{");

                    int ofs = GetBits(n, 6, 5);
                    if (n & 0x0800)
                        strcat(parms, "D");
                    sprintf(s, "%d:", ofs);
                    strcat(parms, s);

                    int wid = GetBits(n, 0, 5);
                    if (n & 0x0020)
                        strcat(parms, "D");
                    sprintf(s, "%d}", wid);
                    strcat(parms, s);

                    if (opcd & 0x0100) { // BFEXTU, BFEXTS, BFFFO, BFINS
                        int reg = GetBits(n, 12, 3);
                        sprintf(s, ",D%d", reg);
                        strcat(parms, s);
                    }
                }
                break;

            case o_PMOVE:
            {
                if (_subtype < CPU_68020) {
                    invalid = true;
                    break;
                }

                // get extension word
                int n = FetchWord(addr, len);
                int fmt   = GetBits(n, 13, 3);
                int p_reg = GetBits(n, 10, 3);
                bool rw = (n >> 9) & 1;
                bool fd = (n >> 8) & 1;
                // bits 7..0 should be == 0x00

                if (fd) {
                    strcpy(opcode, "PMOVEFD");
                }

                const char *reg = "???";
                switch (fmt << 4 | p_reg) {
                    default:
                        break;

// this stuff depends a bit too much on the specific CPU type
#if 1 // 68030 MMU registers
                    case 0x20: reg = "TC";    break;
                    case 0x22: reg = "SRP";   break;
                    case 0x23: reg = "CRP";   break;
                    case 0x02: reg = "TT0";   break;
                    case 0x03: reg = "TT1";   break;
                    case 0x30: reg = "ACUSR"; break;
#endif
#if 0 // 68851 MMU registers
                    case 0x20: reg = "TCR";   break;
                    case 0x21: reg = "DRP";   break;
                    case 0x22: reg = "SRP";   break;
                    case 0x23: reg = "CRP";   break;
                    case 0x24: reg = "CAL";   break;
                    case 0x25: reg = "VAL";   break;
                    case 0x26: reg = "SCC";   break;
                    case 0x27: reg = "AC";    break;
                    case 0x01: reg = "AC0";   break;
                    case 0x03: reg = "AC1";   break;
                    case 0x30: reg = "PSR";   break;
                    case 0x31: reg = "PCSR";  break;
#endif
                }

                if (rw) {
                     parms = stpcpy(parms, reg);
                     parms = stpcpy(parms, ",");
                }

                // parse ea
                SrcAddr(opcd, opSize, parms, addr, len, invalid, lfref, refaddr);
                if (invalid) break;

                if (!rw) {
                     parms += strlen(parms);
                     parms = stpcpy(parms, ",");
                     parms = stpcpy(parms, reg);
                }

                break;
            }

            case o_BKPT:
                sprintf(parms, "#%d", opcd & 0x07);
                break;

            case o_RTM:
                sprintf(parms, "%c%d", opcd & 0x0008 ? 'A' : 'D', opcd & 0x0007);
                break;

            case o_CALLM:
                if (_subtype < CPU_68020) {
                    invalid = true;
                    break;
                }

                n = FetchWord(addr, len);
                if (n & 0xFF00) {
                    invalid = true;
                    break;
                }
                SrcAddr(opcd, -1, s, addr, len, invalid, lfref, refaddr);
                sprintf(parms, "#%d,%s", n, s);
                break;

            case o_CAS:
                if (_subtype < CPU_68020) {
                    invalid = true;
                    break;
                }

                n = FetchWord(addr, len);
                opSize = GetBits(opcd, 9, 2) - 1;
                if (opSize < 0
                     || (n & 0xFC38) != 0x0000 ) {
                    invalid = true;
                    break;
                }
                SrcAddr(opcd, opSize, s, addr, len, invalid, lfref, refaddr);
                sprintf(parms, "D%d,D%d,%s", n & 0x07, (n >> 6) & 0x07, s);
                break;

            case o_CAS2:
                if (_subtype < CPU_68020) {
                    invalid = true;
                    break;
                }

                n = FetchWord(addr, len);
                i = FetchWord(addr, len);
                opSize = GetBits(opcd, 9, 2) - 1;
                if (opSize < 0
                     || (n & 0x0E1C) != 0
                     || (i & 0x0E1C) != 0 ) {
                    invalid = true;
                    break;
                }
                sprintf(parms, "D%d:D%d,D%d:D%d,(%c%d):(%c%d)",
                               GetBits(n, 0, 3), GetBits(i, 0, 3),
                               GetBits(n, 6, 3), GetBits(i ,6, 3),
                               (n & 0x8000) ? 'A':'D', GetBits(n, 12, 3),
                               (i & 0x8000) ? 'A':'D', GetBits(i, 12, 3) );
                break;

            case o_CMP2:
                if (_subtype < CPU_68020) {
                    invalid = true;
                    break;
                }

                n = FetchWord(addr, len);
                opSize = GetBits(opcd, 9, 2);
                if (opSize < 0
                     || (n & 0x07FF) != 0x0000 ) {
                    invalid = true;
                    break;
                }
                if (n & 0x0800) {
                    strcpy(opcode, "CHK2");
                }
                SrcAddr(opcd, opSize, s, addr, len, invalid, lfref, refaddr);
                sprintf(parms, "%s,%c%d", s, (n & 0x8000) ? 'A':'D', GetBits(n, 12, 3));
                break;

            case o_PACK:
                if (_subtype < CPU_68020) {
                    invalid = true;
                    break;
                }

                n = FetchWord(addr, len);
                H4Str(n, s);
                if (opcd & 0x0008) {
                    // -(Ax),-(Ay),#n
                    sprintf(parms, "-(A%d),-(A%d),#%s", GetBits(opcd, 0, 3),
                                                        GetBits(opcd, 9, 3), s);
                } else {
                    // Dx,Dy,#n
                    sprintf(parms, "D%d,D%d,#%s", GetBits(opcd, 0, 3),
                                                  GetBits(opcd, 9, 3), s);
                }
                break;

            case o_TRAPcc:
                if (_subtype < CPU_68020) {
                    invalid = true;
                    break;
                }

                if (opcd & 0x0002) {
                    n = FetchWord(addr, len);
                }
                if (opcd & 0x0001) {
                    n = (n << 16) + FetchWord(addr, len);
                }
                switch (opcd & 0x0007) {
                    case 0x0002: // .W
                        strcat(opcode, ".W");
                        sprintf(parms, "#%4X", n);
                        break;
                    case 0x0003: // .L
                        strcat(opcode, ".L");
                        sprintf(parms, "#%8X", n);
                        break;
                    case 0x0004: // no words
                        break;
                    default:
                        invalid = true;
                        break;
                }
                break;

            case o_MUL_020:
                if (_subtype < CPU_68020) {
                    invalid = true;
                    break;
                }
                n = FetchWord(addr, len);
                opSize = s_Long;

                if (n & 0x0800) {
                     strcat(opcode,"U");
                } else {
                     strcat(opcode,"S");
                }
                if (n & 0x0400) {
                     strcat(opcode,"L");
                }

                SrcAddr(opcd, opSize, s, addr, len, invalid, lfref, refaddr);
                if (!(opcd & 0x0040) && !(n & 0x0400)) {
                    sprintf(parms, "%s,D%d", s, GetBits(n, 0, 3));
                } else {
                    sprintf(parms, "%s,D%d:D%d", s, GetBits(n, 0, 3), GetBits(n, 12,3));
                }
                break;

            default:
                break;
        }
    }

    if (invalid || rom.AddrOutRange(addr)) {
        strcpy(opcode, "???");
        sprintf(parms, "$%.4X", ReadWord(addr));
        len      = 0;
        lfref    = 0;
        refaddr  = 0;
        opSize   = -1;
    }

    // rip-stop checks
    if (opcode[0]) {
        //n = ReadWord(addr + 2);
        switch (ReadWord(addr)) {
            case 0x0000: // ORI.B #0,D0
                if (ReadWord(addr+2) == 0x0000) {
                    lfref |= RIPSTOP;
                }
                break;

            case 0x4878: // do not allow PEA abs.W to be a reference
                lfref &= ~(REFFLAG | CODEREF);
                break;
        }
    }

    if (opSize >= 0) {
        i = strlen(opcode);
        opcode[i]   = '.';
        opcode[i+1] = opSizes[opSize];
        if (opSize == s_Byte && instr->typ == o_Bcc) {
            opcode[i+1] = 'S';
        }
        opcode[i+2] = 0;
    }

    return len;
}
