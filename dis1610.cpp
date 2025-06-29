// dis1610.cpp

static const char versionName[] = "GI CP1610 disassembler";

#include "discpu.h"

struct InstrRec {
    uint16_t        andWith;
    uint16_t        cmpWith;
    int             typ;        // typ
    const char      *op;        // mnemonic
    uint16_t        lfref;      // lfFlag/refFlag/codeRef/etc.
};
typedef const struct InstrRec *InstrPtr;


class Dis1610 : public CPU {
public:
    Dis1610(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);

private:
    uint16_t FetchWord(int addr, int &len);
    uint16_t FetchImm(int addr, int &len, bool sdbd);
    InstrPtr FindInstr(uint16_t opcd);
};


enum {
    CPU_1610,
};


//#define OCTAL
#ifdef OCTAL
Dis1610 cpu_1610("CP1610", CPU_1610, BIG_END, ADDR_16, '*', 'O', "DB", "DW", "DL");
#else
Dis1610 cpu_1610("CP1610", CPU_1610, BIG_END, ADDR_16, '*', 'H', "DB", "DW", "DL");
#endif

Dis1610::Dis1610(const char *name, int subtype, int endian, int addrwid,
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
#ifdef OCTAL
    _radix   = RAD_OCT16BE;
#else
    _radix   = RAD_HEX16BE;
#endif
    _wordsize = WS_16BE;

    add_cpu();
}


// =====================================================


enum InstType {
    o_Invalid,
    o_Jump,         // jump/jump-to-subroutine
    o_Implied,      // no operands
    o_NOP_SIN,      // NOP/SIN opcodes
    o_OneReg,       // single register operand
    o_Shift,        // shift and rotate
    o_TwoReg,       // register-to-register
    o_Branch,       // relative branch
    o_Direct,       // register-to-memory
    o_PSH_PUL,      // PSHR/PULR is a special case of indirect R6
    o_Indirect,     // indirect through R1-R6
    o_Immed,        // immediate decle/word
};


static const struct InstrRec CP1610_opcdTable[] =
{
    // and     cmp     typ            op       lfref
    { 0x03FF, 0x0000, o_Implied,  "HLT",  0 },
    { 0x03FF, 0x0001, o_Implied,  "SDBD", 0 },
    { 0x03FF, 0x0002, o_Implied,  "EIS",  0 },
    { 0x03FF, 0x0003, o_Implied,  "DIS",  0 },

    { 0x03FF, 0x0004, o_Jump,     "J",    REFFLAG | CODEREF },

    { 0x03FF, 0x0005, o_Implied,  "TCI",   0 },
    { 0x03FF, 0x0006, o_Implied,  "CLRC",  0 },
    { 0x03FF, 0x0007, o_Implied,  "SETC",  0 },

    { 0x03F8, 0x0008, o_OneReg,   "INCR",  0 },
    { 0x03F8, 0x0010, o_OneReg,   "DECR",  0 },
    { 0x03F8, 0x0018, o_OneReg,   "COMR",  0 },
    { 0x03F8, 0x0020, o_OneReg,   "NEGR",  0 },
    { 0x03F8, 0x0028, o_OneReg,   "ADCR",  0 },
    { 0x03FC, 0x0030, o_OneReg,   "GSWD",  0 },
    { 0x03FE, 0x0034, o_NOP_SIN,  "NOP",   0 },
    { 0x03FE, 0x0036, o_NOP_SIN,  "SIN",   0 },
    { 0x03F8, 0x0038, o_OneReg,   "RSWD",  0 },

    { 0x03F8, 0x0040, o_Shift,    "SWAP",  0 },
    { 0x03F8, 0x0048, o_Shift,    "SLL",   0 },
    { 0x03F8, 0x0050, o_Shift,    "RLC",   0 },
    { 0x03F8, 0x0058, o_Shift,    "SLLC",  0 },
    { 0x03F8, 0x0060, o_Shift,    "SLR",   0 },
    { 0x03F8, 0x0068, o_Shift,    "SAR",   0 },
    { 0x03F8, 0x0070, o_Shift,    "RRC",   0 },
    { 0x03F8, 0x0078, o_Shift,    "SARC",  0 },

    { 0x03C7, 0x0087, o_TwoReg,   "JR",   LFFLAG }, // MOV sss,R7
    { 0x03C0, 0x0080, o_TwoReg,   "MOVR", 0 },
    { 0x03C0, 0x0080, o_TwoReg,   "TSTR", 0 }, // MOV sss,sss
    { 0x03C0, 0x00C0, o_TwoReg,   "ADDR", 0 },
    { 0x03C0, 0x0100, o_TwoReg,   "SUBR", 0 },
    { 0x03C0, 0x0140, o_TwoReg,   "CMPR", 0 },
    { 0x03C0, 0x0180, o_TwoReg,   "ANDR", 0 },
    { 0x03C0, 0x01C0, o_TwoReg,   "XORR", 0 },
    { 0x03C0, 0x01C0, o_TwoReg,   "CLRR", 0 }, // XORR sss,sss

    { 0x03DF, 0x0200, o_Branch,   "B",    REFFLAG | CODEREF | LFFLAG },
    { 0x03DF, 0x0208, o_Branch,   "NOPP", 0 }, // 2-word NOP, equivalent of BRN on Motorola
    { 0x03DF, 0x0201, o_Branch,   "BC",   REFFLAG | CODEREF }, // BC/BLGT
    { 0x03DF, 0x0209, o_Branch,   "BNC",  REFFLAG | CODEREF }, // BNC/BLLT
    { 0x03DF, 0x0202, o_Branch,   "BOV",  REFFLAG | CODEREF },
    { 0x03DF, 0x020A, o_Branch,   "BNOV", REFFLAG | CODEREF },
    { 0x03DF, 0x0203, o_Branch,   "BPL",  REFFLAG | CODEREF },
    { 0x03DF, 0x020B, o_Branch,   "BMI",  REFFLAG | CODEREF },
    { 0x03DF, 0x0204, o_Branch,   "BEQ",  REFFLAG | CODEREF }, // BZE/BEQ
    { 0x03DF, 0x020C, o_Branch,   "BNEQ", REFFLAG | CODEREF }, // BNZE/BNEQ
    { 0x03DF, 0x0205, o_Branch,   "BLT",  REFFLAG | CODEREF },
    { 0x03DF, 0x020D, o_Branch,   "BGE",  REFFLAG | CODEREF },
    { 0x03DF, 0x0206, o_Branch,   "BLE",  REFFLAG | CODEREF },
    { 0x03DF, 0x020E, o_Branch,   "BGT",  REFFLAG | CODEREF },
    { 0x03DF, 0x0207, o_Branch,   "BUSC", REFFLAG | CODEREF },
    { 0x03DF, 0x020F, o_Branch,   "BESC", REFFLAG | CODEREF },
    { 0x03D0, 0x0210, o_Branch,   "BEXT", REFFLAG | CODEREF },

    { 0x03F8, 0x0240, o_Direct,   "MVO",  0 }, // sss,A
    { 0x03F8, 0x0280, o_Direct,   "MVI",  0 }, // A,ddd
    { 0x03F8, 0x02C0, o_Direct,   "ADD",  0 },
    { 0x03F8, 0x0300, o_Direct,   "SUB",  0 },
    { 0x03F8, 0x0340, o_Direct,   "CMP",  0 },
    { 0x03F8, 0x0380, o_Direct,   "AND",  0 },
    { 0x03F8, 0x03C0, o_Direct,   "XOR",  0 },

    { 0x03F8, 0x0278, o_Immed,    "MVOI", 0 }, // sss,i
    { 0x03F8, 0x02B8, o_Immed,    "MVII", 0 }, // i,ddd
    { 0x03F8, 0x02F8, o_Immed,    "ADDI", 0 },
    { 0x03F8, 0x0338, o_Immed,    "SUBI", 0 },
    { 0x03F8, 0x0378, o_Immed,    "CMPI", 0 },
    { 0x03F8, 0x03B8, o_Immed,    "ANDI", 0 },
    { 0x03F8, 0x03F8, o_Immed,    "XORI", 0 },

    { 0x03F8, 0x0270, o_PSH_PUL,  "PSHR", 0 }, //sss
    { 0x03F8, 0x02B0, o_PSH_PUL,  "PULR", 0 }, //ddd
    { 0x03C0, 0x0240, o_Indirect, "MVO@", 0 }, //sss,mmm
    { 0x03C0, 0x0280, o_Indirect, "MVI@", 0 }, //mmm,ddd
    { 0x03C0, 0x02C0, o_Indirect, "ADD@", 0 },
    { 0x03C0, 0x0300, o_Indirect, "SUB@", 0 },
    { 0x03C0, 0x0340, o_Indirect, "CMP@", 0 },
    { 0x03C0, 0x0380, o_Indirect, "AND@", 0 },
    { 0x03C0, 0x03C0, o_Indirect, "XOR@", 0 },

    { 0x0000, 0x0000, o_Invalid,  ""    , 0 }
};


uint16_t Dis1610::FetchWord(int addr, int &len)
{
    uint16_t w;

    // read a word in big-endian order
    w = (ReadByte(addr + len) << 8) + ReadByte(addr + len + 1);
    len += 2;
    return w;
}


uint16_t Dis1610::FetchImm(int addr, int &len, bool sdbd)
{
    uint16_t w;

    w = FetchWord(addr, len);
    if (sdbd) {
        // SDBD is in little-endian order
        w = (w & 0xFF) + (FetchWord(addr, len) & 0xFF) * 256;
    }

    return w;
}


InstrPtr Dis1610::FindInstr(uint16_t opcd)
{
    InstrPtr p;

    p = CP1610_opcdTable;

    while (p->andWith && ((opcd & p->andWith) != p->cmpWith)) {
        p++;
    }

    return p;
}


int Dis1610::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
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

        // peek at previous opcode
        int prevopcd = 0;
        // must be in-range, marked for this disassembler, and start of instruction
        if (!rom.AddrOutRange(addr - 2)
              && rom.get_type(addr - 2) == _id
              && !rom.test_attr(addr - 2, ATTR_CONT)) {
            prevopcd = (ReadByte(addr - 2) << 8) + ReadByte(addr - 1);
        }
        bool sdbd = (prevopcd == 0x0001);

        switch (instr->typ) {
            case o_Implied:     // - no operands
                break;

            case o_NOP_SIN:
                sprintf(parms, "%d", opcd & 1);
                break;

            case o_Jump:
                n = FetchWord(addr, len);
                // J/JSR is in big-endian order
                ra = (FetchWord(addr, len) & 0x3FF) + ((n & 0x00FC) << 8);

                if (((n >> 8) & 3) != 3) {
                    strcpy(opcode, "JSR");
                    sprintf(parms, "R%d,", ((n >> 8) & 3) + 4);
                    parms += strlen(parms);
                } else {
                    lfref |= LFFLAG;
                }
#if 0
                if ((n & 0x03) == 0x03) {
                    invalid = true;
                    break;
                }
#endif
                RefStr(ra, parms, lfref, refaddr);

                if (n & 0x01) {
                    strcat(opcode, "E");
                }
                if (n & 0x02) {
                    strcat(opcode, "D");
                }
                break;

            case o_OneReg:
                n = opcd & 7;
                sprintf(parms, "R%d", opcd & 0x07);
                break;

            case o_Shift:
                sprintf(parms, "R%d", opcd & 0x03);
                if (opcd & 0x0004) {
                     strcat(parms, ",2");
                }
                break;

            case o_TwoReg:
                if ((opcd & 7) == 7 && (opcd & 0x03C0) != 0x0080 // R7 but not TSTR
                                    && (opcd & 0x03C0) != 0x0240) { //      or CMPR
//                  strcpy(parms, "PC");
                    lfref |= LFFLAG;
                }

                n = (opcd & 0x03C7) == 0x0087; // JR
                if (((opcd >> 3) & 0x07) == (opcd & 0x07)) {
                    switch (opcd & 0x03C0) {
                        case 0x0080: // MOVR
                            instr += n; // advance JR to MOVR so that it can become TSTR
                            // fallthrough
                        case 0x01C0: // XORR
                            // use next opcode
                            strcpy(opcode, instr[1].op);
                            n++;
                            break;
                        default:
                            break;
                    }
                }
                if (n) {
                    sprintf(parms, "R%d", (opcd >> 3) & 0x07);
                } else {
                    sprintf(parms, "R%d,R%d", (opcd >> 3) & 0x07, opcd & 0x07);
                }
                break;

            case o_Branch:
                n = FetchWord(addr, len); // can potentially be 16 bits
                if (opcd & 0x0020) {
                    n = ~n;
                }
                if (n == -2) {
                    sprintf(parms, "%c", _curpc);
                } else if (n == 0) {
                    sprintf(parms, "%c+2", _curpc);
                    // add skip reference without a label so that B *+2 works
                    lfref |= 1 << SKPSHFT;
                } else {
                    ra = addr/2 + len/2 + n;
                    RefStr(ra, parms, lfref, refaddr);
                }
                if (opcd & 0x0010) {
                    parms += strlen(parms);
                    sprintf(parms, ",%d", opcd & 0x0F);
                }
                break;

            case o_Direct:
                if ((opcd & 7) == 7 && (opcd & 0x03F8) != 0x0240) { // R7 but not MVO
//                  strcpy(parms, "PC");
                    lfref |= LFFLAG;
                }
                ra = FetchWord(addr, len);
                RefStr(ra, s, lfref, refaddr);
                if ((opcd & 0x03F8) == 0x0240) { // MVO
                    sprintf(parms, "R%d,%s", opcd & 0x07, s);
                } else {
                    sprintf(parms, "%s,R%d", s, opcd & 0x07);
                }
                break;

            case o_PSH_PUL:
                if ((opcd & 0x03FF) == 0x02B7) { // PULR R7
                    strcpy(parms, "PC");
                    lfref |= LFFLAG;
                } else {
                    sprintf(parms, "R%d", opcd & 0x07);
                }
                break;

            case o_Indirect:
                if ((opcd & 7) == 7 && (opcd & 0x03C0) != 0x0240) { // R7 but not MVO@
//                  strcpy(parms, "PC");
                    lfref |= LFFLAG;
                }
                n = (opcd >> 3) & 0x07;
                switch (n) {
                    case 1:
                    case 2:
                    case 3:
                        sprintf(s, "R%d", n);
                        break;
                    case 4:
                    case 5:
                        sprintf(s, "R%d+", n);
                        break;
                    case 6:
                        if ((opcd & 0x03F8) == 0x0240) { // MVO
                            sprintf(s, "R%d+", n);
                        } else {
                            sprintf(s, "-R%d", n);
                        }
                        break;
                    default:
                    case 0: // 0 should always be o_Direct opcodes
                    case 7: // 7 should always be o_Immed opcodes
                        strcpy(parms, "...");
                        break;
                }
                if ((opcd & 0x03F8) == 0x0240) { // MVO
                    sprintf(parms, "R%d,%s", opcd & 0x07, s);
                } else {
                    sprintf(parms, "%s,R%d", s, opcd & 0x07);
                }
                break;

            case o_Immed:
                if ((opcd & 7) == 7 && (opcd & 0x03F8) != 0x0278) { // R7 but not MVOI
//                  strcpy(parms, "PC");
                    lfref |= LFFLAG;
                }
                if ((opcd & 0x03F8) == 0x0278) { // MVOI
                    // MVOI does not work with SDBD
                    sdbd = false;
                }
                n = FetchImm(addr, len, sdbd); // can potentially be 16 bits

                // check for sequence MOVR(ADDR) R7,Rn / ADDI(SUBI) xxxx,Rn
                // this is a standard way to get a PC-relative address
                if ( (((prevopcd & 0x03F8) == 0x00B8) ||  // MOVR R7,Rn
                      ((prevopcd & 0x03F8) == 0x00F8))    // ADDR R7,Rn
                        && ((prevopcd & 7) == (opcd & 7)) // Rn=Rn
                        && !(hint & 1) ) {                // odd hint disables this
                    if ( ((opcd & 0x03F8) == 0x02F8) ||   // ADDI nnnn,Rn
                         ((opcd & 0x03F8) == 0x0338) ) {  // SUBI nnnn,Rn
                        if (opcd & 0x0100) { // SUBI
                            RefStr(addr/2 - n, s, lfref, refaddr);
                            sprintf(parms, "%c-%s,R%d", _curpc, s, opcd & 0x07);
                        } else { // ADDI
                            RefStr(addr/2 + n, s, lfref, refaddr);
                            sprintf(parms, "%s-%c,R%d", s, _curpc, opcd & 0x07);
                        }
                        break;
                    }
                }

                if (hint & 1) { // odd hint enables immediate as reference
                    RefStr(n, s, lfref, refaddr);
                } else {
                    H4Str(n, s);
                }
                if ((opcd & 0x03F8) == 0x0278) { // MVOI
                    sprintf(parms, "R%d,#%s", opcd & 0x07, s);
                } else {
                    sprintf(parms, "#%s,R%d", s, opcd & 0x07);
                }
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
