// dis9900.cpp

static const char versionName[] = "TMS9900 disassembler";

#include "discpu.h"

struct InstrRec {
    uint16_t        andWith;
    uint16_t        cmpWith;
    int             typ;        // typ
    const char      *op;        // mnemonic
    uint8_t         lfref;      // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;


class Dis9900 : public CPU {
public:
    Dis9900(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);

private:
    uint16_t FetchWord(int addr, int &len);
    void AddrMode(int mode, char *parms, int addr, int &len, int &lfref, addr_t &refaddr);
    InstrPtr FindInstr(uint16_t opcd);
};


enum {
    CPU_9900,
};

Dis9900 cpu_9900("9900", CPU_9900, BIG_END, ADDR_16, '*', '>', "DB", "DW", "DL");


Dis9900::Dis9900(const char *name, int subtype, int endian, int addrwid,
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


enum InstType {
    o_Invalid,      // 0
    o_None,         // - no operands    .... .... .... ....
    o_CRU,          // - SBO/SBZ/TB     .... .... cccc cccc
    o_D,            // - D              .... .... ..aa dddd
    o_DCnt,         // - D,CNT          .... ..ee eeaa dddd
    o_Data16,       // - DATA16         .... .... .... .... XX
    o_Data4,        // - DATA4          .... .... .... .... XX
    o_Disp,         // - DISP (jumps)   .... .... cccc cccc
    o_R,            // - R              .... .... .... rrrr
    o_RCnt,         // - R,CNT          .... .... eeee rrrr
    o_RData16,      // - R,DATA16       .... .... .... rrrr XX
    o_S,            // - S              .... .... .... ssss
    o_SCnt,         // - S,CNT          .... ..ee eebb ssss
    o_SD,           // - S,D            .... aadd ddbb ssss
    o_SR,           // - S,R            .... ..rr rrbb ssss
};


static const struct InstrRec TI9900_opcdTable[] =
{
    // and     cmp     typ            op       lfref

    // 0000
    { 0xFFF0, 0x0080, o_R,       "LST",  0 },
    { 0xFFF0, 0x0090, o_R,       "LWP",  0 },
    // 0100

    { 0xFFF0, 0x0200, o_RData16, "LI"  , 0 },
    { 0xFFF0, 0x0220, o_RData16, "AI"  , 0 },
    { 0xFFF0, 0x0240, o_RData16, "ANDI", 0 },
    { 0xFFF0, 0x0260, o_RData16, "ORI" , 0 },
    { 0xFFF0, 0x0280, o_RData16, "CI"  , 0 },
    { 0xFFF0, 0x02A0, o_R,       "STWP", 0 },
    { 0xFFF0, 0x02C0, o_R,       "STST", 0 },
    { 0xFFFF, 0x02E0, o_Data16,  "LWPI", 0 },

    { 0xFFF0, 0x0300, o_Data4,   "LIMI", 0 },
    // 0320
    { 0xFFFF, 0x0340, o_None,    "IDLE", 0 },
    { 0xFFFF, 0x0360, o_None,    "RSET", 0 },
    { 0xFFFF, 0x0380, o_None,    "RTWP", 0 },
    { 0xFFFF, 0x03A0, o_None,    "CKON", 0 },
    { 0xFFFF, 0x03C0, o_None,    "CKOF", 0 },
    { 0xFFFF, 0x03E0, o_None,    "LREX", 0 },

    { 0xFFC0, 0x0400, o_S,       "BLWP", 0 },
    { 0xFFFF, 0x045B, o_None,    "RT",   LFFLAG }, // aka "B *R11"
    { 0xFFC0, 0x0440, o_S,       "B",    REFFLAG | CODEREF | LFFLAG },
    { 0xFFC0, 0x0480, o_S,       "X",    0 },
    { 0xFFC0, 0x04C0, o_D,       "CLR",  0 },
    { 0xFFC0, 0x0500, o_D,       "NEG",  0 },
    { 0xFFC0, 0x0540, o_D,       "INV",  0 },
    { 0xFFC0, 0x0580, o_D,       "INC",  0 },
    { 0xFFC0, 0x05C0, o_D,       "INCT", 0 },
    { 0xFFC0, 0x0600, o_D,       "DEC",  0 },
    { 0xFFC0, 0x0640, o_D,       "DECT", 0 },
    { 0xFFC0, 0x0680, o_D,       "BL",   REFFLAG | CODEREF },
    { 0xFFC0, 0x06C0, o_D,       "SWPB", 0 },
    { 0xFFC0, 0x0700, o_D,       "SETO", 0 },
    { 0xFFC0, 0x0740, o_D,       "ABS",  0 },
    // 0780
    // 07C0

    { 0xFF00, 0x0800, o_RCnt,    "SRA",  0 },
    { 0xFF00, 0x0900, o_RCnt,    "SRL",  0 },
    { 0xFF00, 0x0A00, o_RCnt,    "SLA",  0 },
    { 0xFF00, 0x0B00, o_RCnt,    "SRC",  0 },
    // 0C00
    // 0D00
    // 0E00
    // 0F00

    { 0xFFFF, 0x1000, o_None,    "NOP",  0 }, // aka "JMP $+2"
    { 0xFF00, 0x1000, o_Disp,    "JMP",  REFFLAG | CODEREF | LFFLAG},	
    { 0xFF00, 0x1100, o_Disp,    "JLT",  REFFLAG | CODEREF},
    { 0xFF00, 0x1200, o_Disp,    "JLE",  REFFLAG | CODEREF},
    { 0xFF00, 0x1300, o_Disp,    "JEQ",  REFFLAG | CODEREF},
    { 0xFF00, 0x1400, o_Disp,    "JHE",  REFFLAG | CODEREF},
    { 0xFF00, 0x1500, o_Disp,    "JGT",  REFFLAG | CODEREF},
    { 0xFF00, 0x1600, o_Disp,    "JNE",  REFFLAG | CODEREF},
    { 0xFF00, 0x1700, o_Disp,    "JNC",  REFFLAG | CODEREF},
    { 0xFF00, 0x1800, o_Disp,    "JOC",  REFFLAG | CODEREF},
    { 0xFF00, 0x1900, o_Disp,    "JNO",  REFFLAG | CODEREF},
    { 0xFF00, 0x1A00, o_Disp,    "JL",   REFFLAG | CODEREF},
    { 0xFF00, 0x1B00, o_Disp,    "JH",   REFFLAG | CODEREF},
    { 0xFF00, 0x1C00, o_Disp,    "JOP",  REFFLAG | CODEREF},
    { 0xFF00, 0x1D00, o_CRU,     "SBO",  0},
    { 0xFF00, 0x1E00, o_CRU,     "SBZ",  0},
    { 0xFF00, 0x1F00, o_CRU,     "TB",   0},

    { 0xFC00, 0x2000, o_SR,      "COC",  0 },
    { 0xFC00, 0x2400, o_SR,      "CZC",  0 },
    { 0xFC00, 0x2800, o_SR,      "XOR",  0 },
    { 0xFC00, 0x2C00, o_SR,      "XOP",  0 },
    { 0xFC00, 0x3000, o_SCnt,    "LDCR", 0 },
    { 0xFC00, 0x3400, o_DCnt,    "STCR", 0 },
    { 0xFC00, 0x3800, o_SR,      "MPY",  0 },
    { 0xFC00, 0x3C00, o_SR,      "DIV",  0 },

    { 0xF000, 0x4000, o_SD,      "SZC",  0 },
    { 0xF000, 0x5000, o_SD,      "SZCB", 0 },
    { 0xF000, 0x6000, o_SD,      "S",    0 },
    { 0xF000, 0x7000, o_SD,      "SB",   0 },
    { 0xF000, 0x8000, o_SD,      "C",    0 },
    { 0xF000, 0x9000, o_SD,      "CB",   0 },
    { 0xF000, 0xA000, o_SD,      "A",    0 },
    { 0xF000, 0xB000, o_SD,      "AB",   0 },
    { 0xF000, 0xC000, o_SD,      "MOV",  0 },
    { 0xF000, 0xD000, o_SD,      "MOVB", 0 },
    { 0xF000, 0xE000, o_SD,      "SOC",  0 },
    { 0xF000, 0xF000, o_SD,      "SOCB", 0 },

    { 0x0000, 0x0000, o_Invalid, ""    , 0 }
};


uint16_t Dis9900::FetchWord(int addr, int &len)
{
    uint16_t w;

    // read a word in big-endian order
    w = (ReadByte(addr + len) << 8) + ReadByte(addr + len + 1);
    len += 2;
    return w;
}


// parms is string to write
// mode is aadddd or bbssss
// len is updated if an immediate is loaded
void Dis9900::AddrMode(int mode, char *parms, int addr, int &len, int &lfref, addr_t &refaddr)
{
    int reg = mode & 0x0F;
    int n;
    char s[256];

    switch ((mode >> 4) & 3) {
//      default:
        case 0: // Rn
            sprintf(parms, "R%d", reg);
            break;
        case 1: // *Rn
            sprintf(parms, "*R%d", reg);
            break;
        case 2: // symbolic direct or indexed
            n = FetchWord(addr, len);
            if (reg == 0) {
                // r==0 symbolic direct @Label
                lfref |= REFFLAG;
                refaddr = n;
                RefStr(n, s, lfref, refaddr);
                sprintf(parms, "@%s", s);
            } else {
                // @idx(Rn)
                if (n & 0x8000) {
                    // handle negative offsets
                    n = -n;
                    *parms++ = '-';
                }
                int hint = rom.get_hint(addr);
                if (hint & 1) {
                    RefStr(n, s, lfref, refaddr);
                } else {
                    H4Str(n, s);
                }
                sprintf(parms, "@%s(R%d)", s, reg);
            }
            break;
        case 3: // *Rn+
            sprintf(parms, "*R%d+", reg);
            break;
    }
}


InstrPtr Dis9900::FindInstr(uint16_t opcd)
{
    InstrPtr p;

    p = TI9900_opcdTable;

    while (p->andWith && ((opcd & p->andWith) != p->cmpWith)) {
        p++;
    }

    return p;
}


int Dis9900::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    int         n;
    char        s[256];
    addr_t      ra;

    opcode[0] = 0;
    parms[0] = 0;
    int len = 0;
    lfref   = 0;
    refaddr = 0;

    bool invalid = true;

    int opcd = FetchWord(addr, len);
    InstrPtr instr = FindInstr(opcd);
    int hint = rom.get_hint(addr);

    if (!(addr & 0x01) &&       // require even alignment!
        instr && instr->typ && *(instr->op)) {
        invalid = false;
        strcpy(opcode, instr->op);
        lfref = instr->lfref;

        switch (instr->typ) {
            case o_None:        // - no operands    .... .... .... ....
                break;

            case o_CRU:         // - SBO/SBZ/TB     .... .... cccc cccc
                n = opcd & 0xFF;
                if (n & 0x80) {
                    n = -n;
                }
                sprintf(parms, "%d", n);	
                break;

            case o_D:           // - D              .... .... ..aa dddd
            case o_S:           // - S              .... .... ..bb ssss
                AddrMode(opcd & 0x3F, parms, addr, len, lfref, refaddr);
                break;

            case o_Data16:      // - DATA16         .... .... .... .... XX
                ra = FetchWord(addr, len);
                if (hint & 1) {
                    RefStr(ra, s, lfref, refaddr);
                } else {
                    H4Str(ra, s);
                }
                break;

            case o_Data4:       // - DATA4          .... .... .... .... XX (low 4 bits of XX)
                ra = FetchWord(addr, len);
                H4Str(ra, parms);
//              RefStr(ra, parms, lfref, refaddr);
                break;

            case o_Disp:        // - DISP (jumps)   .... .... cccc cccc
                n = opcd & 0xFF;
                if (n & 0x80) {
                    n = n - 256;
                }
                ra = addr + len + n*2;
                RefStr(ra, parms, lfref, refaddr);
                break;

            case o_R:           // - R              .... .... .... rrrr
                sprintf(parms, "R%d", opcd & 0x0F);	
                break;

            case o_RCnt:        // - R,CNT          .... .... eeee rrrr
                sprintf(parms, "R%d,%d", opcd & 0x0F, (opcd & 0xF0) >> 4);	
                break;

            case o_RData16:     // - R,DATA16       .... .... .... rrrr XX
                ra = FetchWord(addr, len);
                if ((lfref & CODEREF) || (hint & 1)) {
                    RefStr(ra, s, lfref, refaddr);
                } else {
                    H4Str(ra, s);
                }
                sprintf(parms, "R%d,%s", opcd & 0x0F, s);
                break;

            case o_DCnt:        // - D,CNT          .... ..ee eeaa dddd
            case o_SCnt:        // - S,CNT          .... ..ee eebb ssss
                AddrMode(opcd & 0x3F, parms, addr, len, lfref, refaddr);
                parms += strlen(parms);
                sprintf(parms, ",%d", (opcd >> 6) & 0x0F);
                break;

            case o_SD:          // - S,D            .... aadd ddbb ssss
                AddrMode(opcd & 0x3F, parms, addr, len, lfref, refaddr);
                strcat(parms, ",");
                parms += strlen(parms);
                AddrMode((opcd >> 6) & 0x3F, parms, addr, len, lfref, refaddr);
                break;

            case o_SR:         // - S,R            .... ..rr rrbb ssss
                AddrMode(opcd & 0x3F, parms, addr, len, lfref, refaddr);
                parms += strlen(parms);
                sprintf(parms, ",R%d", (opcd >> 6) & 0x0F);
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
