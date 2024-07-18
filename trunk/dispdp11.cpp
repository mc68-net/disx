// dispdp11.cpp

static const char versionName[] = "DEC PDP-11 disassembler";

#include "discpu.h"

struct InstrRec {
    uint16_t        andWith;
    uint16_t        cmpWith;
    int             typ;        // typ
    const char      *op;        // mnemonic
    uint8_t         lfref;      // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;


class DisPDP11 : public CPU {
public:
    DisPDP11(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);

private:
    uint16_t FetchWord(int addr, int &len);
    static int GetBits(int num, int pos, int len);
    addr_t FetchSize(int addr, int &len, int size);
    void AddrMode(int mode, int reg, char *parms, int addr, int &len, bool &invalid,
                  int &lfref, addr_t &refaddr);
    void SrcAddr(int opcd, char *parms, int addr, int &len, bool &invalid,
                      int &lfref, addr_t &refaddr);
    void DstAddr(int opcd, char *parms, int addr, int &len, bool &invalid,
                      int &lfref, addr_t &refaddr);
    InstrPtr FindInstr(uint16_t opcd);
};


enum {
    CPU_PDP11,
};

DisPDP11 cpu_PDP11("PDP11", CPU_PDP11, LITTLE_END, ADDR_16, '*', '$', "DB", "DW", "DL");


DisPDP11::DisPDP11(const char *name, int subtype, int endian, int addrwid,
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

    add_cpu();
}


// =====================================================


enum InstType {
    o_Invalid,      // 0
    o_Implied,      // - no operands
    o_OneReg,       // - one operand: reg in b0-b2
    o_OneAddr,      // - one operand: adrmode in b0-b5
    o_TwoAddr,      // - two operands: src in b6-b11, dst in b0-b5
    o_RegAddr,      // - two operands: reg in b6-b8, adrmode in b0-b5
    o_AddrReg,      // - two operands: adrmode in b0-b5, reg in b6-b8
    o_Branch,       // - 8-bit offset branch
    o_SOB,          // - decrement and 6-bit offset branch
    o_SPL,          // - one operand: 0-7
    o_EMT_TRAP,     // - EMT and TRAP opcodes, one operand in b0-b7
};

struct TrapRec {
    uint16_t        trap;       // opcode of trap
    char            bit_9;      // name of bit 9
    char            bit_10;     // name of bit 10
    const char      *name;      // name of trap
};
typedef const struct TrapRec *TrapPtr;


static const struct InstrRec PDP11_opcdTable[] =
{
    // and     cmp     typ            op       lfref
    {0177777, 0000000, o_Implied, "HALT",  0      }, // LFFLAG },
    {0177777, 0000001, o_Implied, "WAIT",  0      },
    {0177777, 0000002, o_Implied, "RTI",   LFFLAG },
    {0177777, 0000003, o_Implied, "BPT",   0      },
    {0177777, 0000004, o_Implied, "IOT",   0      },
    {0177777, 0000005, o_Implied, "RESET", 0      }, // LFFLAG },
    {0177777, 0000006, o_Implied, "RTT",   LFFLAG }, // 11/40, 11/45
//  {0177777, 0000007, 

    {0177700, 0000100, o_OneAddr, "JMP",   REFFLAG | CODEREF | LFFLAG }, // 0001DD
// *** JMP may need its own op-type, mode 0 not allowed, and CODEREF may only be for some modes

    {0177770, 0000200, o_OneReg,  "RTS",   LFFLAG }, // 00020R
//  {0177770, 0000210, 
//  {0177770, 0000220, 
//  {0177770, 0000230, o_SPL,     "SPL",   0      }, // 00023N 11/45
    {0177777, 0000240, o_Implied, "NOP",   0      }, // "clear no CC flags"
// there are other valid combinations of 000240-000277 with no defined mnemonics
    {0177777, 0000241, o_Implied, "CLC",   0      }, 
    {0177777, 0000242, o_Implied, "CLV",   0      }, 
    {0177777, 0000244, o_Implied, "CLZ",   0      }, 
    {0177777, 0000250, o_Implied, "CLN",   0      }, 
    {0177777, 0000257, o_Implied, "CCC",   0      }, 
    {0177777, 0000261, o_Implied, "SEC",   0      }, 
    {0177777, 0000262, o_Implied, "SEV",   0      }, 
    {0177777, 0000264, o_Implied, "SEZ",   0      }, 
    {0177777, 0000270, o_Implied, "SEN",   0      }, 
    {0177777, 0000277, o_Implied, "SCC",   0      }, 

    {0177700, 0000300, o_OneAddr, "SWAB",  0      },

    {0177400, 0000400, o_Branch,  "BR",    REFFLAG | CODEREF | LFFLAG },
    {0177400, 0001000, o_Branch,  "BNE",   REFFLAG | CODEREF },
    {0177400, 0001400, o_Branch,  "BEQ",   REFFLAG | CODEREF },
    {0177400, 0002000, o_Branch,  "BGE",   REFFLAG | CODEREF },
    {0177400, 0002400, o_Branch,  "BLT",   REFFLAG | CODEREF },
    {0177400, 0003000, o_Branch,  "BGT",   REFFLAG | CODEREF },
    {0177400, 0003400, o_Branch,  "BLE",   REFFLAG | CODEREF },

    {0177000, 0004000, o_RegAddr, "JSR",   REFFLAG | CODEREF }, // 004Rdd

    {0177700, 0005000, o_OneAddr, "CLR",  0 },
    {0177700, 0005100, o_OneAddr, "COM",  0 },
    {0177700, 0005200, o_OneAddr, "INC",  0 },
    {0177700, 0005300, o_OneAddr, "DEC",  0 },
    {0177700, 0005400, o_OneAddr, "NEG",  0 },
    {0177700, 0005500, o_OneAddr, "ADC",  0 },
    {0177700, 0005600, o_OneAddr, "SBC",  0 },
    {0177700, 0005700, o_OneAddr, "TST",  0 },

    {0177700, 0006000, o_OneAddr, "ROR",  0 },
    {0177700, 0006100, o_OneAddr, "ROL",  0 },
    {0177700, 0006200, o_OneAddr, "ASR",  0 },
    {0177700, 0006300, o_OneAddr, "ASL",  0 },
    {0177700, 0006400, o_OneAddr, "MARK", 0 }, // * 0064NN 11/40, 11/45
    {0177700, 0006500, o_OneAddr, "MFPI", 0 }, // * 0065SS 11/40, 11/45
    {0177700, 0006600, o_OneAddr, "MTPI", 0 }, // * 0066DD 11/40, 11/45
    {0177700, 0006700, o_OneAddr, "SXT",  0 }, // * 0067DD 11/40, 11/45

//  {0170000, 0007000, 

    {0170000, 0010000, o_TwoAddr, "MOV",  0 },
    {0170000, 0020000, o_TwoAddr, "CMP",  0 },
    {0170000, 0030000, o_TwoAddr, "BIT",  0 },
    {0170000, 0040000, o_TwoAddr, "BIC",  0 },
    {0170000, 0050000, o_TwoAddr, "BIS",  0 },
    {0170000, 0060000, o_TwoAddr, "ADD",  0 },

    {0177000, 0070000, o_AddrReg, "MUL",  0 }, // 11/40, 11/45
    {0177000, 0071000, o_AddrReg, "DIV",  0 }, // 11/40, 11/45
    {0177000, 0072000, o_AddrReg, "ASH",  0 }, // 11/40, 11/45
    {0177000, 0073000, o_AddrReg, "ASHC", 0 }, // 11/40, 11/45
    {0177000, 0074000, o_AddrReg, "XOR",  0 }, // 11/40, 11/45
    {0177770, 0075000, o_OneReg,  "FADD", 0 }, // 11/40, FP
    {0177770, 0075010, o_OneReg,  "FSUB", 0 }, // 11/40, FP
    {0177770, 0075020, o_OneReg,  "FMUL", 0 }, // 11/40, FP
    {0177770, 0075030, o_OneReg,  "FDIV", 0 }, // 11/40, FP
//  {0177000, 0075000, 
//  {0177000, 0076000, 
    {0177000, 0077000, o_SOB,     "SOB",  REFFLAG | CODEREF }, // 077Rrr 11/40, 11/45

    {0177400, 0104000, o_EMT_TRAP, "EMT",  0 },
    {0177400, 0104400, o_EMT_TRAP, "TRAP", 0 },

    {0177400, 0100000, o_Branch,  "BPL",  REFFLAG | CODEREF },
    {0177400, 0100400, o_Branch,  "BMI",  REFFLAG | CODEREF },
    {0177400, 0101000, o_Branch,  "BHI",  REFFLAG | CODEREF },
    {0177400, 0101400, o_Branch,  "BLOS", REFFLAG | CODEREF },
    {0177400, 0102000, o_Branch,  "BVC",  REFFLAG | CODEREF },
    {0177400, 0102400, o_Branch,  "BVS",  REFFLAG | CODEREF },
    {0177400, 0103000, o_Branch,  "BCC",  REFFLAG | CODEREF }, // aka BHIS
    {0177400, 0103400, o_Branch,  "BCS",  REFFLAG | CODEREF }, // aka BLO

    {0177700, 0105000, o_OneAddr, "CLRB", 0 },
    {0177700, 0105100, o_OneAddr, "COMB", 0 },
    {0177700, 0105200, o_OneAddr, "INCB", 0 },
    {0177700, 0105300, o_OneAddr, "DECB", 0 },
    {0177700, 0105400, o_OneAddr, "NEGB", 0 },
    {0177700, 0105500, o_OneAddr, "ADCB", 0 },
    {0177700, 0105600, o_OneAddr, "SBCB", 0 },
    {0177700, 0105700, o_OneAddr, "TSTB", 0 },

    {0177700, 0106000, o_OneAddr, "RORB", 0 },
    {0177700, 0106100, o_OneAddr, "ROLB", 0 },
    {0177700, 0106200, o_OneAddr, "ASRB", 0 },
    {0177700, 0106300, o_OneAddr, "ASLB", 0 },
//  {0177700, 0106400, o_OneAddr, "", },
    {0177700, 0106500, o_OneAddr, "MFPD", 0 }, // * 1065SS 11/45
    {0177700, 0106600, o_OneAddr, "MTPD", 0 }, // * 1066DD 11/45
//  {0177700, 0106700, o_OneAddr, "", },

    {0170000, 0110000, o_TwoAddr, "MOVB", 0 }, 
    {0170000, 0120000, o_TwoAddr, "CMPB", 0 }, 
    {0170000, 0130000, o_TwoAddr, "BITB", 0 }, 
    {0170000, 0140000, o_TwoAddr, "BICB", 0 }, 
    {0170000, 0150000, o_TwoAddr, "BISB", 0 }, 
    {0170000, 0160000, o_TwoAddr, "SUB" , 0 }, 
//  {0170000, 0170000, // floating point

    {0x0000, 0x0000, o_Invalid,     ""       , 0 }
};


uint16_t DisPDP11::FetchWord(int addr, int &len)
{
    uint16_t w;

    // read a word in little-endian order
    w = ReadByte(addr + len) + (ReadByte(addr + len + 1) << 8);
    len += 2;
    return w;
}


int DisPDP11::GetBits(int num, int pos, int len)
{
    return (num >> pos) & (0xFFFF >> (16-len));
}


// NOTE: MOVE can have two refaddrs, the result is that only the last one will become refaddr

void DisPDP11::AddrMode(int mode, int reg, char *parms, int addr, int &len, bool &invalid,
                      int &lfref, addr_t &refaddr)
{
    int     n;
    char    s[256];
    addr_t  ra;

    switch (mode) {
        case 0: // 000 nnn = Rn
            sprintf(parms, "R%d", reg);
            break;

        case 1: // 001 nnn = (Rn)
            sprintf(parms, "(R%d)", reg);
            break;

        case 2: // 010 nnn = (Rn)+
            if (reg == 7) { // PC = immediate
                ra = FetchWord(addr, len);
                H4Str(ra, s);
                sprintf(parms, "#%s", s);
            } else {
                sprintf(parms, "(R%d)+", reg);
            }
            break;

        case 3: // 011 nnn = @(Rn)+
            if (reg == 7) { // PC = absolute
                ra = FetchWord(addr, len);
                RefStr(ra, s, lfref, refaddr);
                sprintf(parms, "@#%s", s);
            } else {
                sprintf(parms, "@(R%d)+", reg);
            }
            break;

        case 4: // 100 nnn = -(Rn)
            sprintf(parms, "-(R%d)", reg);
            break;

        case 5: // 101 nnn = @-(Rn)
            sprintf(parms, "@-(R%d)", reg);
            break;

        case 6: // 110 nnn xxxx = nnnn(Rn)
            if (reg == 7) { // PC = relative
                ra = FetchWord(addr, len);
                ra += addr + len;
                RefStr(ra, parms, lfref, refaddr);
            } else {
                n = FetchWord(addr, len);
                sprintf(parms, "%.4X(R%d)", n, reg);
            }
            break;

        case 7:
            if (reg == 7) { // PC = relative
                ra = FetchWord(addr, len);
                ra += addr;
                RefStr(ra, parms, lfref, refaddr);
            } else {
                n = FetchWord(addr, len);
                sprintf(parms, "%.4X(R%d)", n, reg);
            }
            break;

        default:
            invalid = true;
            break;
    }
}


// NOTE: in two-op instructions, src is 11-6 and dest is 5-0
// this should probably get straightened out sometime
// Also, which consumes operand words first?
void DisPDP11::SrcAddr(int opcd, char *parms, int addr, int &len, bool &invalid,
                      int &lfref, addr_t &refaddr)
{
    int     mode, reg;

    mode = GetBits(opcd, 3, 3);
    reg  = GetBits(opcd, 0, 3);
    AddrMode(mode, reg, parms, addr, len, invalid, lfref, refaddr);
}


void DisPDP11::DstAddr(int opcd, char *parms, int addr, int &len, bool &invalid,
                      int &lfref, addr_t &refaddr)
{
    int     mode, reg;

    mode = GetBits(opcd, 9, 3);
    reg  = GetBits(opcd, 6, 3);
    AddrMode(mode, reg, parms, addr, len, invalid, lfref, refaddr);
}


InstrPtr DisPDP11::FindInstr(uint16_t opcd)
{
    InstrPtr p;

    p = PDP11_opcdTable;

    while (p->andWith && ((opcd & p->andWith) != p->cmpWith)) {
        p++;
    }

    return p;
}


int DisPDP11::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    int         opcd;
//    int         i;
    int         n;
    InstrPtr    instr;
    char        s[256];
//  char        s2[256];
    addr_t      ra;

    opcode[0] = 0;
    parms[0] = 0;
    int len = 0;
    lfref   = 0;
    refaddr = 0;

    bool invalid = true;

    opcd = FetchWord(addr, len);
    instr = FindInstr(opcd);

    if (!(addr & 0x01) &&               // require even alignment!
        instr && instr->typ && *(instr->op)) {
        invalid = false;
        strcpy(opcode, instr->op);
        lfref = instr->lfref;

        switch (instr->typ) {
            case o_Implied:             // no operands
                break;

            case o_OneReg:              // one operand: reg in b0-b2
                sprintf(parms, "R%d", opcd & 7);
                break;

            case o_OneAddr:             // one operand: adrmode in b0-b5
                SrcAddr(opcd,parms,addr,len,invalid,lfref,refaddr);
//                sprintf(parms, "<R%d>", opcd & 7);
                break;

            case o_TwoAddr:             // one operand: adrmode in b0-b5
                DstAddr(opcd,parms,addr,len,invalid,lfref,refaddr);
                SrcAddr(opcd,s,addr,len,invalid,lfref,refaddr);
                sprintf(parms + strlen(parms), ",%s", s);
                break;

            case o_RegAddr:             // two operands: reg in b6-b8, adrmode in b0-b5
// note: currently only used by JSR
                SrcAddr(opcd,s,addr,len,invalid,lfref,refaddr);
                if ((opcd & 0177077) == 004067) { // JSR Rn,nnnn(PC)
                    lfref |= CODEREF;
                }
//                sprintf(parms, "R%d,<R%d>", (opcd >> 6) & 7, opcd & 7);
                sprintf(parms, "R%d,%s", (opcd >> 6) & 7, s);
                break;

            case o_AddrReg:             // two operands: adrmode in b0-b5, reg in b6-b8
                SrcAddr(opcd,s,addr,len,invalid,lfref,refaddr);
                sprintf(parms, "%s,R%d", s, (opcd >> 6) & 7);
                break;

            case o_Branch:              // 8-bit offset branch
                n = opcd & 0xFF;
                if (n > 127) {
                    n = n - 256;
                }
                ra = addr + 2 + n*2;
                RefStr(ra, parms, lfref, refaddr);
                break;

            case o_SOB:
                n = opcd & 0x3F;
                ra = addr + 2 - n*2; // offset is always negative!
                RefStr(ra, s, lfref, refaddr);
                sprintf(parms, "R%d,%s", (opcd & 0x01C0) >> 6, s);
                break;

            case o_SPL:                 // one operand, 0-7
                sprintf(parms, "%d", opcd & 7);
                break;

            case o_EMT_TRAP:
                H2Str(opcd & 0xFF, parms);
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
