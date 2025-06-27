// disimp.cpp

static const char versionName[] = "NS IMP-16 disassembler";

#include "discpu.h"

struct InstrRec {
    uint16_t        andWith;
    uint16_t        cmpWith;
    int             typ;        // typ
    const char      *op;        // mnemonic
    uint16_t        lfref;      // lfFlag/refFlag/codeRef/etc.
};
typedef const struct InstrRec *InstrPtr;


class DisImp16 : public CPU {
public:
    DisImp16(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);

private:
    uint16_t FetchWord(int addr, int &len);
    InstrPtr FindInstr(uint16_t opcd);
    bool XRmode(addr_t addr, int opcd, bool indirect, char *parm, int &lfref, addr_t &refaddr);
};


enum {
    CPU_IMP16,
};


DisImp16 cpu_Imp16("IMP16", CPU_IMP16, BIG_END, ADDR_16, '.', 'H', "DB", "DW", "DL");

DisImp16::DisImp16(const char *name, int subtype, int endian, int addrwid,
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
    _wordsize = WS_16BE;

    add_cpu();
}


// =====================================================


enum InstType {
    o_Invalid,      // 
    o_None,         // no operands      . . . .|. . . .|. . . .|. . . . FFFF
    o_RegToReg,     // format 1         ---OP--|-SR|-DR|.|x x x x x| OP F003
    o_Imm8,         // format 2         -----OP----|-R-|-displacement-- FC00
    o_Stack,        // format 3         -----OP----|-R-|x x x x x x x x FC00
    o_Shift,        //                  -----OP----|-R-|-shift-amount-- FC80
    o_MemRef1,      // format 4         ---OP--|-R-|-XR|-displacement-- F000
    o_MemRef1a,     //                  ----OP---|R|-XR|-displacement-- F800
    o_MemRef2,      // format 5         -----OP----|-XR|-displacement-- FC00
    o_Branch,       // format 6         ---OP--|---CC--|-displacement-- F000
    o_CtlFlags,     // format 7         ----OP---|--FC-|O|-----ctl----- F880
    o_IOMisc,       // format 8         --------OP-------|-----ctl----- FF80
    o_JSRI,         //                  0 0 0 0 0 0 1 1 1|-----ctl----- FF80
//  o_MemRef,       // format 9         -----OP----|-XR|---OP--|x x x x FCF0
//  o_Bit,          // format 10        0 0 0 0 0|------OP-----|---n--- FFF0
//  o_JSRP,         // format 11        --------OP-------|x x x x x x x FF80
#if 0
    o_OneAcc,       //                  x x x AC- ---op-- 0 0 0|0 0 0 1
    o_NoAccEA,	    // No-acc-EA        0 0 0 OP- @ IDX|[displacement-]
    o_OneAccEA,	    // One-acc-EA       0 0 0 AC- @ IDX|[displacement-]
    o_IOFormat,	    // IO-format        0 1 1 AC- OP- -F- [device-code]
    o_AriLog,	    // Arith/Logical    1 ACS ACD OP- SH- -C- #|[SKIP-]
    o_MAP,          //                  0 1 1 n|n 0 0 0|1 1 1 1|1 1 1 1
    o_IOTFormat,    //                  0 1 1 0|0 1 1 1|t t 1 1|1 1 1 1
    o_TRAP,         //                  1 s s d|d q q q|q q q q|1 0 0 0
#endif
};


static const struct InstrRec Imp16_opcdTable[] =
{
    // and     cmp     typ            op       lfref
    { 0xFFFF, 0x3081, o_None,     "NOP",     0 },

    { 0xFF80, 0x0000, o_IOMisc,   "HALT",    0 },
    { 0xFFFF, 0x0080, o_IOMisc,   "PUSHF",   0 },
    { 0xFF80, 0x0100, o_IOMisc,   "RTI",     LFFLAG },
    { 0xFF80, 0x0200, o_IOMisc,   "RTS",     LFFLAG },
    { 0xFFFF, 0x0280, o_IOMisc,   "PULLF",   0 },
    { 0xFF80, 0x0380, o_JSRI,     "JSRI",    0 },
    { 0xFF80, 0x0400, o_IOMisc,   "RIN",     0 },
    { 0xFF80, 0x0600, o_IOMisc,   "ROUT",    0 },

    { 0xF880, 0x0800, o_CtlFlags, "SFLG",    0 },
    { 0xF880, 0x0880, o_CtlFlags, "PFLG",    0 },

    { 0xF000, 0x1000, o_Branch,   "BOC",     REFFLAG | CODEREF},

    { 0xF800, 0x2000, o_MemRef2,  "JMP",     REFFLAG | CODEREF | LFFLAG},
    { 0xF800, 0x2800, o_MemRef2,  "JSR",     REFFLAG | CODEREF},

    { 0xF083, 0x3000, o_RegToReg, "RADD",  0 },
    { 0xF083, 0x3080, o_RegToReg, "RXCH",  0 },
    { 0xF083, 0x3081, o_RegToReg, "RCPY",  0 },
    { 0xF083, 0x3082, o_RegToReg, "RXOR",  0 },
    { 0xF083, 0x3083, o_RegToReg, "RAND",  0 },

    { 0xFCFF, 0x4000, o_Stack,    "PUSH",  0 },
    { 0xFCFF, 0x4400, o_Stack,    "PULL",  0 },
    { 0xFC00, 0x4800, o_Imm8,     "AISZ",  SKPBYTES(2) },
    { 0xFC00, 0x4C00, o_Imm8,     "LI",    0 },
    { 0xFC00, 0x5000, o_Imm8,     "CAI",   0 },
    { 0xFCFF, 0x5400, o_Stack,    "XCHRS", 0 },
    { 0xFC80, 0x5800, o_Shift,    "ROL",   0 },
    { 0xFC80, 0x5880, o_Shift,    "ROR",   0 },
    { 0xFC80, 0x5C00, o_Shift,    "SHL",   0 },
    { 0xFC80, 0x5C80, o_Shift,    "SHR",   0 },

    { 0xF800, 0x6000, o_MemRef1a, "AND",   0 },
    { 0xF800, 0x6800, o_MemRef1a, "OR",    0 },
    { 0xF800, 0x7000, o_MemRef1a, "SKAZ",  SKPBYTES(2) },

    { 0xFC00, 0x7800, o_MemRef2,  "ISZ",   SKPBYTES(2) },
    { 0xFC00, 0x7C00, o_MemRef2,  "DSZ",   SKPBYTES(2) },

    { 0xE000, 0x8000, o_MemRef1,  "LD",    0 },
    { 0xE000, 0xA000, o_MemRef1,  "ST",    0 },
    { 0xF000, 0xC000, o_MemRef1,  "ADD",   0 },
    { 0xF000, 0xD000, o_MemRef1,  "SUB",   0 },
    { 0xF000, 0xE000, o_MemRef1,  "SKG",   SKPBYTES(2) },
    { 0xF000, 0xF000, o_MemRef1,  "SKNE",  SKPBYTES(2) },

#if 0
    // extended instruction set (to be implemented later)

    { 0xFF80, 0x037F, o_IOMisc,   "JSRP",    0 },å

    { 0xFCFF, 0x0480, o_MemRef,   "MPY",     0 },
    { 0xFCFF, 0x0490, o_MemRef,   "DIV",     0 },
    { 0xFCFF, 0x04A0, o_MemRef,   "DADD",    0 },
    { 0xFCFF, 0x04B0, o_MemRef,   "DSUB",    0 },
    { 0xFCFF, 0x04C0, o_MemRef,   "LDB",     0 },
    { 0xFCFF, 0x04D0, o_MemRef,   "STB" ,    0 },

    { 0xFFF0, 0x0500, o_Bit,      "JMPP",    0 },
    { 0xFFF0, 0x0510, o_Bit,      "ISCAN",   0 },
    { 0xFFF0, 0x0520, o_Bit,      "JINT",    0 },

    { 0xFFF0, 0x0700, o_Bit,      "SETST",   0 },
    { 0xFFF0, 0x0710, o_Bit,      "CLRST",   0 },
    { 0xFFF0, 0x0720, o_Bit,      "SETBIT",  0 },
    { 0xFFF0, 0x0730, o_Bit,      "CLRBIT",  0 },
    { 0xFFF0, 0x0740, o_Bit,      "SKSTF",   0 },
    { 0xFFF0, 0x0750, o_Bit,      "SKBIT",   0 },
    { 0xFFF0, 0x0760, o_Bit,      "CMPBIT",  0 },
#endif

    { 0x0000, 0x0000, o_Invalid,  "",        0 }
};


// BOC condition codes
static const char *boc_cc[] = {
    /*0*/ "0",    // Interrupt Line = 1
    /*1*/ "ZRO",  // (AC0) = 0
    /*2*/ "POS",  // (AC0) >= 0
    /*3*/ "BIT0", // Bit 0 of AC0 = 1 (odd)
    /*4*/ "BIT1", // Bit 1 of AC0 = 1
    /*5*/ "NZRO", // (AC0) != 0
    /*6*/ "6",    // Control panel interrupt line = 1
    /*7*/ "7",    // Control panel start = 1
    /*8*/ "8",    // Stack full line = 1
    /*9*/ "IEN",  // Interrupt enable = 1
    /*A*/ "CYOV", // Carry if SEL=0 / overflow if SEL = 1
    /*B*/ "NEG",  // (AC0) <= 0
    /*C*/ "12",   // User
    /*D*/ "13",   // User
    /*E*/ "14",   // User
    /*F*/ "15"    // User
};


// SFLG/PFLG flags
static const char *sflg_flag[] = {
    /*0*/ "F8",   // User Specified
    /*1*/ "IEN",  // Interrupt Enable
    /*2*/ "SEL",  // Select
    /*3*/ "F11",  // User Specified
    /*4*/ "F12",  // User Specified
    /*5*/ "F13",  // User Specified
    /*6*/ "F14",  // User Specified
    /*7*/ "F15"   // User Specified
};


uint16_t DisImp16::FetchWord(int addr, int &len)
{
    uint16_t w;

    // read a word in big-endian order
    w = (ReadByte(addr + len) << 8) + ReadByte(addr + len + 1);
    len += 2;
    return w;
}


InstrPtr DisImp16::FindInstr(uint16_t opcd)
{
    InstrPtr p;

    p = Imp16_opcdTable;

    while (p->andWith && ((opcd & p->andWith) != p->cmpWith)) {
        p++;
    }

    return p;
}


bool DisImp16::XRmode(addr_t addr, int opcd, bool indirect, char *parms, int &lfref, addr_t &refaddr)
{
    int disp = opcd & 0xFF;
    int reg = (opcd >> 8) & 3;
    int hint = rom.get_hint(addr);
    addr_t ra;
    char s[256];

    if (indirect) {
        *parms++ = '@';
        *parms = 0;
    }

    switch (reg) {
        default:
        case 0: // zero page
            if (hint & 1) {
                H2Str(disp, parms);
            } else {
                ra = disp;
                RefStr2(ra, parms, lfref, refaddr);
            }
            break;
        case 1: // PC relative
            if (disp & 0x80) {
                disp = disp - 256;
            }
            if (disp || indirect) {
                ra = addr/2 + 1 + disp;
                RefStr(ra, parms, lfref, refaddr);
            } else {
                strcat(parms, ".+1");
                if (!indirect) lfref &= ~LFFLAG;
            }
            if (indirect) lfref |= INDWBE;
            break;
        case 2: // AC2 relative
        case 3: // AC3 relative
            if (disp == 0) {
                s[0] = 0;
            } else {
                if (disp & 0x80) {
                    *parms++ = '-';
                    *parms = 0;
                    disp = 256 - disp;
                }
                if (disp < 16) {
                    sprintf(s, "%d", disp);
                } else {
                    H2Str(disp, s);
                }
            }
            sprintf(parms, "%s(AC%d)", s, reg);
            break;
    }

    return false; // not invalid
}


int DisImp16::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    int         n;
//  char        s[256];
    addr_t      ra;
//  const char  *p;

    opcode[0] = 0;
    parms[0] = 0;
    int len = 0;
    lfref   = 0;
    refaddr = 0;

    bool invalid = true;

    int opcd = FetchWord(addr, len);
    InstrPtr instr = FindInstr(opcd);
//  int hint = rom.get_hint(addr);

    if (!(addr & 0x01) &&       // require even alignment!
        instr && instr->typ && *(instr->op)) {
        invalid = false;
        strcpy(opcode, instr->op);
        lfref = instr->lfref;

        switch (instr->typ) {
            case o_None:
                break;

            case o_RegToReg:
                sprintf(parms, "AC%d,AC%d", (opcd >> 10) & 3, (opcd >> 8) & 3);
                break;

            case o_Imm8:
            case o_Stack:
                sprintf(parms, "AC%d", (opcd >> 8) & 3);
                if (instr->typ == o_Imm8) {
                    strcat(parms, ",");
                    parms += strlen(parms);
                    n = opcd & 0xFF;
                    if (n & 0x80) {
                         *parms++ = '-';
                         n = 256 - n;
                    }
                    if (n < 16) {
                        sprintf(parms, "%d", n);
                    } else {
                        H2Str(n, parms);
                    }
                }
                break;

            case o_Shift:
                n = opcd & 0xFF;
                if (n & 0x80) {
                    // note: left vs right shift was already determined in the opcode table
                    n = 256 - n;
                }
                sprintf(parms, "AC%d,%d", (opcd >> 8) & 3, n);
                break;

            case o_MemRef1:
            case o_MemRef1a:
                n = (opcd >> 10) & 3;
                if (instr->typ == o_MemRef1a) {
                    // o_MemRef1 uses AC0/AC1 only
                    n &= 1;
                }
                sprintf(parms, "AC%d,", n);
                parms += strlen(parms);
                invalid = XRmode(addr, opcd, (opcd & 0xD000) == 0x9000, parms, lfref, refaddr);
                break;

            case o_MemRef2:
                n = ((opcd & 0xF000) == 0x2000) && (opcd & 0x0400);
                invalid = XRmode(addr, opcd, n, parms, lfref, refaddr);
                if (invalid) break;
#if 0
                if ((opcd & 0xE000) == 0xE000) {
                    // add skip reference
// ***need to confirm this traces properly!
                    lfref |= SKPBYTES(2);
                }
#endif
                break;

            case o_Branch:
                sprintf(parms, "%s,", boc_cc[(opcd >> 8) & 0x0F]);
                n = opcd & 0xFF;
                if (n == 0xFF) {
                    strcat(parms, ".");
                } else {
                    if (n & 0x80) {
                        n = n - 256;
                    }
                    ra = addr/2 + 1 + n;
                    RefStr(ra, parms + strlen(parms), lfref, refaddr);
                }
                break;

            case o_CtlFlags:
                strcpy(parms, sflg_flag[(opcd >> 8) & 0x07]);
                break;

            case o_IOMisc:
                n = opcd & 0x7F;
                if (n) {
                    if (n < 16) {
                        sprintf(parms, "%d", n);
                    } else {
                        H2Str(n, parms);
                    }
                }
                break;

            case o_JSRI:
                n = opcd & 0x7F;
                n += 0xFF80;
                H4Str(n, parms);
                break;

//          case o_MemRef:
                break;

//          case o_Bit:
                break;

//          case o_JSRP:
                break;

            default:
                invalid = true;
                break;
        }
    }

    if (invalid || rom.AddrOutRange(addr)) {
        strcpy(opcode, "???");
        sprintf(parms, "$%.4X", ReadWord(addr));
        len      = 0;
        lfref    = 0;
        refaddr  = 0;
    }

    // rip-stop checks
    if (opcode[0]) {
        int op = ReadWord(addr);
        switch (op) {
            case 0x0000: // repeated all zeros or all ones
            case 0xFFFF:
                if (ReadWord(addr+2) == op) {
                    lfref |= RIPSTOP;
                }
                break;
        }
    }

    return len;
}
