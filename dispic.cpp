// dispic.cpp

static const char versionName[] = "PIC disassembler";

#include "discpu.h"

//const int maxInstLen = 8;  // length in bytes of longest instruction

class DisPIC : public CPU {
public:
    DisPIC(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);

    enum InstType {
        o_Invalid,      // 0
        o_None,         // 1
        o_TRIS,         // 2
        o_RegF,         // 3
        o_RegFD,        // 4
        o_RegFB,        // 5
        o_Lit,          // 6
        o_CALL,         // 7
        o_GOTO,         // 8
    };
    
    struct InstrRec {
        uint16_t        andWith;
        uint16_t        cmpWith;
        int             typ;        // typ
        const char      *op;        // mnemonic
    uint8_t         lfref;  // lfFlag/refFlag/codeRef
    };
    typedef const struct InstrRec *InstrPtr;

    static const struct InstrRec PIC12_opcdTable[];
    static const struct InstrRec PIC14_opcdTable[];
    static const char PIC12_reg[][8];
    static const char PIC12_status[][8];    
    
    InstrPtr FindInstr(uint16_t opcd);
};

enum {
    CPU_PIC12,
    CPU_PIC14
};


DisPIC cpu_PIC12("PIC12",  CPU_PIC12, LITTLE_END, ADDR_16, '*', '$', "DC.B", "DC.W", "DC.L"); // LIST_16, PIC12_opcdTab);
DisPIC cpu_PIC14("PIC14",  CPU_PIC14, LITTLE_END, ADDR_16, '*', '$', "DC.B", "DC.W", "DC.L"); // LIST_16, PIC14_opcdTab);


DisPIC::DisPIC(const char *name, int subtype, int endian, int addrwid,
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


const struct DisPIC::InstrRec DisPIC::PIC12_opcdTable[] =
{
    // and     cmp     typ          lf ref op
    {0x0FFF, 0x0000, o_None,    "NOP"   , 0                },            // NOP
    {0x0FFF, 0x0002, o_None,    "OPTION", 0                },            // OPTION
    {0x0FFF, 0x0003, o_None,    "SLEEP" , 0                },            // SLEEP
    {0x0FFF, 0x0004, o_None,    "CLRWDT", 0                },            // CLRWDT
    {0x0FF8, 0x0000, o_TRIS,    "TRIS"  , 0                },            // TRIS     n       n = 5..9
    {0x0FE0, 0x0020, o_RegF,    "MOVWF" , 0                },            // MOVWF    f       f = 0..31
    {0x0FFF, 0x0040, o_None,    "CLRW"  , 0                },            // CLRW
    {0x0FE0, 0x0060, o_RegF,    "CLRF"  , 0                },            // CLRF     f
    {0x0FC0, 0x0080, o_RegFD,   "SUBWF" , 0                },            // SUBWF    f, d    f = 0..31, d = 0..1
    {0x0FC0, 0x00C0, o_RegFD,   "DECF"  , 0                },            // DECF     f, d
    {0x0FC0, 0x0100, o_RegFD,   "IORWF" , 0                },            // MOVF     f, d
    {0x0FC0, 0x0140, o_RegFD,   "ANDWF" , 0                },            // COMF     f, d
    {0x0FC0, 0x0180, o_RegFD,   "XORWF" , 0                },            // INCF     f, d
    {0x0FC0, 0x01C0, o_RegFD,   "ADDWF" , 0                },            // ADDWF    f, d
    {0x0FC0, 0x0200, o_RegFD,   "MOVF"  , 0                },            // MOVF     f, d
    {0x0FC0, 0x0240, o_RegFD,   "COMF"  , 0                },            // COMF     f, d
    {0x0FC0, 0x0280, o_RegFD,   "INCF"  , 0                },            // INCF     f, d
//  {0x0FC0, 0x02C0, o_RegFD,   "DECFSZ", REFFLAG | CODEREF},            // DECFSZ   f, d
    {0x0FC0, 0x02C0, o_RegFD,   "DECFSZ", 0                },            // DECFSZ   f, d
    {0x0FC0, 0x0300, o_RegFD,   "RRF"   , 0                },            // RRF      f, d
    {0x0FC0, 0x0340, o_RegFD,   "RLF"   , 0                },            // RLF      f, d
    {0x0FC0, 0x0380, o_RegFD,   "SWAPF" , 0                },            // SWAPF    f, d
//  {0x0FC0, 0x03C0, o_RegFD,   "INCFSZ", REFFLAG | CODEREF},            // INCFSZ   f, d
    {0x0FC0, 0x03C0, o_RegFD,   "INCFSZ", 0                },            // INCFSZ   f, d
    {0x0F00, 0x0400, o_RegFB,   "BCF"   , 0                },            // BCF      f, b
    {0x0F00, 0x0500, o_RegFB,   "BSF"   , 0                },            // BSF      f, b
    {0x0F00, 0x0600, o_RegFB,   "BTFSC" , REFFLAG | CODEREF},            // BTFSC    f, b
    {0x0F00, 0x0700, o_RegFB,   "BTFSS" , REFFLAG | CODEREF},            // BTFSS    f, b
    {0x0F00, 0x0800, o_Lit,     "RETLW" , LFFLAG           },            // RETLW    k
    {0x0F00, 0x0900, o_CALL,    "CALL"  , REFFLAG | CODEREF},            // CALL     k
    {0x0E00, 0x0A00, o_GOTO,    "GOTO"  , LFFLAG | REFFLAG | CODEREF},   // GOTO     k
    {0x0F00, 0x0C00, o_Lit,     "MOVLW" , 0                },            // MOVLW    k
    {0x0F00, 0x0D00, o_Lit,     "IORLW" , 0                },            // IORLW    k
    {0x0F00, 0x0E00, o_Lit,     "ANDLW" , 0                },            // ANDLW    k
    {0x0F00, 0x0F00, o_Lit,     "XORLW ", 0                },            // XORLW    k
    {0x0000, 0x0000, o_Invalid, ""      , 0                }
};


const struct DisPIC::InstrRec DisPIC::PIC14_opcdTable[] =
{
    // and     cmp     typ          lf ref op
    {0x3F9F, 0x0000, o_None,    "NOP"   , 0                },            // NOP
    {0x3FFF, 0x0008, o_None,    "RETURN", LFFLAG           },            // RETURN
    {0x3FFF, 0x0009, o_None,    "RETFIE", LFFLAG           },            // RETFIE
    {0x3FFF, 0x0063, o_None,    "SLEEP" , 0                },            // SLEEP
    {0x3FFF, 0x0064, o_None,    "CLRWDT", 0                },            // CLRWDT
    {0x03F8, 0x0080, o_RegF,    "MOVWF" , 0                },            // MOVWF    f       f = 0..127
    {0x03F8, 0x0100, o_None,    "CLRW"  , 0                },            // CLRW
    {0x03F8, 0x0180, o_RegF,    "CLRF"  , 0                },            // CLRF
    {0x03F0, 0x0200, o_RegFD,   "SUBWF" , 0                },            // SUBWF    f,d     f = 0..127, d = 0..1
    {0x03F0, 0x0300, o_RegFD,   "DECF"  , 0                },            // DECF     f,d
    {0x03F0, 0x0400, o_RegFD,   "IORWF" , 0                },            // IORWF    f,d
    {0x03F0, 0x0500, o_RegFD,   "ANDWF" , 0                },            // ANDWF    f,d
    {0x03F0, 0x0600, o_RegFD,   "XORWF" , 0                },            // XORWF    f,d
    {0x03F0, 0x0700, o_RegFD,   "ADDWF" , 0                },            // ADDWF    f,d
    {0x03F0, 0x0800, o_RegFD,   "MOVF"  , 0                },            // MOVF     f,d
    {0x03F0, 0x0900, o_RegFD,   "COMF"  , 0                },            // COMF     f,d
    {0x03F0, 0x0A00, o_RegFD,   "INCF"  , 0                },            // INCF     f,d
    {0x03F0, 0x0B00, o_RegFD,   "DECFSZ", 0                },            // DECFSZ   f,d
    {0x03F0, 0x0C00, o_RegFD,   "RRF"   , 0                },            // RRF      f,d
    {0x03F0, 0x0D00, o_RegFD,   "RLF"   , 0                },            // RLF      f,d
    {0x03F0, 0x0E00, o_RegFD,   "SWAPF" , 0                },            // SWAPF    f,d
    {0x03F0, 0x0F00, o_RegFD,   "INCFSZ", 0                },            // INCFSZ   f,d
    {0x03C0, 0x1000, o_RegFD,   "BCF"   , 0                },            // BCF      f,b     f = 0..127, b = 0..7
    {0x03C0, 0x1400, o_RegFD,   "BSF"   , 0                },            // BSF      f,b
    {0x03C0, 0x1800, o_RegFD,   "BTFSC" , 0                },            // BTFSC    f,b
    {0x03C0, 0x1C00, o_RegFD,   "BTFSS" , 0                },            // BTFSS    f,b
    {0x3800, 0x2000, o_CALL,    "CALL"  , REFFLAG | CODEREF},            // CALL     a       a = 0x0000..0x07FF
    {0x3800, 0x2800, o_GOTO,    "GOTO"  , LFFLAG | REFFLAG | CODEREF},   // GOTO     a       a = 0x0000..0x07FF
    {0x3C00, 0x3000, o_Lit,     "MOVLW" , 0                },            // MOVLW    k       k = 0..255
    {0x3C00, 0x3400, o_Lit,     "RETLW" , LFFLAG           },            // RETLW    k
    {0x3F00, 0x3800, o_Lit,     "IORLW" , 0                },            // IORLW    k
    {0x3F00, 0x3900, o_Lit,     "ANDLW" , 0                },            // ANDLW    k
    {0x3F00, 0x3A00, o_Lit,     "XORLW" , 0                },            // XORLW    k
    {0x3E00, 0x3C00, o_Lit,     "SUBLW" , 0                },            // SUBLW    k
    {0x3E00, 0x3E00, o_Lit,     "ADDLW" , 0                },            // ADDLW    k
    {0x0000, 0x0000, o_Invalid, ""      , 0                }
};


const char DisPIC::PIC12_reg[][8] =
{
    "INDF",     // 0x00
    "TMR0",     // 0x01
    "PCL",      // 0x02
    "STATUS",   // 0x03
    "FSR",      // 0x04
    "PORTA",    // 0x05
    "PORTB",    // 0x06
    "PORTC",    // 0x07
    "PORTD",    // 0x08
    "PORTE",    // 0x09
};


const char DisPIC::PIC12_status[][8] =
{
    "C", "DC", "Z", "!PD", "!TO", "PA0", "PA1", "PA2"
};


DisPIC::InstrPtr DisPIC::FindInstr(uint16_t opcd)
{
    InstrPtr p;

    switch (_subtype) {
        default:
        case CPU_PIC12:
            p = PIC12_opcdTable;
            break;

        case CPU_PIC14:
            p = PIC14_opcdTable;
            break;
    }

    while (p->andWith && ((opcd & p->andWith) != p->cmpWith)) p++;

    return p;
}


int DisPIC::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    addr_t      ad;
    int         opcd;
    int         reg_f, reg_d, reg_b, reg_k;
    InstrPtr    instr;
    addr_t      ra;

    *opcode = 0;
    *parms  = 0;
    int len = 0;
    lfref   = 0;
    refaddr = 0;

    ad = addr;
    opcd = ReadWord(addr);
    len = 2;
    instr = FindInstr(opcd);

    if (instr && instr->typ && *(instr->op)) {
        strcpy(opcode, instr->op);
        lfref  = instr->lfref;

        switch (_subtype) {
            default:
            case CPU_PIC12:
                // PIC12 opcd fields
                reg_f = opcd & 0x1F;
                reg_k = opcd & 0xFF;
                reg_b = (opcd >> 5) & 0x07;
                reg_d = (opcd >> 5) & 0x01;
                break;

            case CPU_PIC14:
                // PIC14 opcd fields
                reg_f = opcd & 0x7F;
                reg_k = opcd & 0xFF;
                reg_b = (opcd >> 7) & 0x07;
                reg_d = (opcd >> 7) & 0x01;
                break;
        }

        switch (instr->typ) {
            case o_None:        // 1
                break;

            case o_TRIS:        // 2
//              sprintf(parms, "%d", opcd & 0x07);
                strcpy(parms, PIC12_reg[opcd & 0x07]);
                break;

            case o_RegF:        // 3
                if (reg_f <= 0x09) {
                    strcpy(parms, PIC12_reg[reg_f]);
                } else {
                    sprintf(parms, "%d", reg_f);
                }
                break;

            case o_RegFD:       // 4
                if (reg_f <= 0x09) {
                    sprintf(parms, "%s, %d", PIC12_reg[reg_f], reg_d);
                } else {
                    sprintf(parms, "%d, %d", reg_f, reg_d);
                }
                if (lfref & REFFLAG) {
                    refaddr = ad+4;  // reference for skip instruction
                }
                break;

            case o_RegFB:       // 5
                if (reg_f <= 0x09) {
                    sprintf(parms, "%s, %d", PIC12_reg[reg_f], reg_b);
                } else {
                    sprintf(parms, "%d, %d", reg_f, reg_b);
                }
#if 0
                if (reg_f == 0x03) {
                    strcpy(comnt, PIC12_status[reg_b]);
                }
#endif
                if (lfref & REFFLAG) {
                    refaddr = ad+4;  // skip instruction
                }
                break;

            case o_Lit:         // 6
                sprintf(parms, "0x%.2X", reg_k);
                break;

            case o_CALL:        // 7
                if (_subtype == CPU_PIC12) {
                    ra = (opcd & 0x00FF) << 1;
                } else {
                    ra = (opcd & 0x07FF) << 1;
                }
                RefStr(ra, parms, lfref, refaddr);
                break;

            case o_GOTO:        // 8
                if (_subtype == CPU_PIC12) {
                    ra = (opcd & 0x01FF) << 1;
                } else {
                    ra = (opcd & 0x07FF) << 1;
                }
                RefStr(ra, parms, lfref, refaddr);
                break;

            case o_Invalid:
            default:
                len = 0;
                break;
        }
    }

    if (opcode==NULL || opcode[0]==0 || rom.AddrOutRange(addr)) {
        strcpy(opcode, "???");
        sprintf(parms, "$%.4X", ReadByte(addr));
        len     = 0;
        lfref   = 0;
        refaddr = 0;
    }

    return len;
}
