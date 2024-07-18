// disz8.cpp

static const char versionName[] = "Zilog Z8 disassembler";

#include "discpu.h"

class DisZ8 : public CPU {
public:
    DisZ8(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
    void Regnum(unsigned char reg, char *p);
};


DisZ8 cpu_Z8("Z8", 0, BIG_END, ADDR_16, '$', 'H', "DB", "DW", "DL");


DisZ8::DisZ8(const char *name, int subtype, int endian, int addrwid,
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


struct InstrRec {
    const char      *op;    // mnemonic
    const char      *parms; // parms
    uint8_t         lfref;  // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;

static const struct InstrRec Z8_opcdTable[] =
{
            // op     line         lf    ref

/*00*/      {"DEC",  "r"        , 0                },
/*01*/      {"DEC",  "@r"       , 0                },
/*02*/      {"ADD",  "+Rh,Rl"   , 0                },
/*03*/      {"ADD",  "+Rh,@Rl"  , 0                },
/*04*/      {"ADD",  "+r,x"     , 0                },
/*05*/      {"ADD",  "+r,@x"    , 0                },
/*06*/      {"ADD",  "r,#i"     , 0                },
/*07*/      {"ADD",  "@r,#i"    , 0                },
/*08*/      {"LD",   "R0,r"     , 0                },
/*09*/      {"LD",   "r,R0"     , 0                },
/*0A*/      {"DJNZ", "R0,o"     , REFFLAG | CODEREF},
/*0B*/      {"JR",   "F,o"      , REFFLAG | CODEREF}, // cc,R10 // skips 1 byte
/*0C*/      {"LD",   "R0,#i"    , 0                },
/*0D*/      {"JP",   "F,w"      , REFFLAG | CODEREF}, // cc,DA // skips 2 bytes
/*0E*/      {"INC",  "R0"       , 0                },
/*0F*/      {"",     ""         , 0                },

/*10*/      {"RLC",  "r"        , 0                },
/*11*/      {"RLC",  "@r"       , 0                },
/*12*/      {"ADC",  "+Rh,Rl"   , 0                },
/*13*/      {"ADC",  "+Rh,@Rl"  , 0                },
/*14*/      {"ADC",  "+r,x"     , 0                },
/*15*/      {"ADC",  "+r,@x"    , 0                },
/*16*/      {"ADC",  "r,#i"     , 0                },
/*17*/      {"ADC",  "@r,#i"    , 0                },
/*18*/      {"LD",   "R1,r"     , 0                },
/*19*/      {"LD",   "r,R1"     , 0                },
/*1A*/      {"DJNZ", "R1,o"     , REFFLAG | CODEREF},
/*1B*/      {"JR",   "LT,o"     , REFFLAG | CODEREF},
/*1C*/      {"LD",   "R1,#i"    , 0                },
/*1D*/      {"JP",   "LT,w"     , REFFLAG | CODEREF},
/*1E*/      {"INC",  "R1"       , 0                },
/*1F*/      {"",     ""         , 0                },

/*20*/      {"INC", "r"         , 0                },
/*21*/      {"INC", "@r"        , 0                },
/*22*/      {"SUB",  "+Rh,Rl"   , 0                },
/*23*/      {"SUB",  "+Rh,@Rl"  , 0                },
/*24*/      {"SUB",  "+r,x"     , 0                },
/*25*/      {"SUB",  "+r,@x"    , 0                },
/*26*/      {"SUB",  "r,#i"     , 0                },
/*27*/      {"SUB",  "@r,#i"    , 0                },
/*28*/      {"LD",   "R2,r"     , 0                },
/*29*/      {"LD",   "r,R2"     , 0                },
/*2A*/      {"DJNZ", "R2,o"     , REFFLAG | CODEREF},
/*2B*/      {"JR",   "LE,o"     , REFFLAG | CODEREF},
/*2C*/      {"LD",   "R2,#i"    , 0                },
/*2D*/      {"JP",   "LE,w"     , REFFLAG | CODEREF},
/*2E*/      {"INC",  "R2"       , 0                },
/*2F*/      {"",     ""         , 0                },

/*30*/      {"JP",   "@r"       , LFFLAG           },
/*31*/      {"SRP",  "#i"       , 0                },
/*32*/      {"SBC",  "+Rh,Rl"   , 0                },
/*33*/      {"SBC",  "+Rh,@Rl"  , 0                },
/*34*/      {"SBC",  "+r,x"     , 0                },
/*35*/      {"SBC",  "+r,@x"    , 0                },
/*36*/      {"SBC",  "r,#i"     , 0                },
/*37*/      {"SBC",  "@r,#i"    , 0                },
/*38*/      {"LD",   "R3,r"     , 0                },
/*39*/      {"LD",   "r,R3"     , 0                },
/*3A*/      {"DJNZ", "R3,o"     , REFFLAG | CODEREF},
/*3B*/      {"JR",   "ULE,o"    , REFFLAG | CODEREF},
/*3C*/      {"LD",   "R3,#i"    , 0                },
/*3D*/      {"JP",   "ULE,w"    , REFFLAG | CODEREF},
/*3E*/      {"INC",  "R3"       , 0                },
/*3F*/      {"",     ""         , 0                },

/*40*/      {"DA",   "r"        , 0                },
/*41*/      {"DA",   "@r"       , 0                },
/*42*/      {"OR",   "+Rh,Rl"   , 0                },
/*43*/      {"OR",   "+Rh,@Rl"  , 0                },
/*44*/      {"OR",   "+r,x"     , 0                },
/*45*/      {"OR",   "+r,@x"    , 0                },
/*46*/      {"OR",   "r,#i"     , 0                },
/*47*/      {"OR",   "@r,#i"    , 0                },
/*48*/      {"LD",   "R4,r"     , 0                },
/*49*/      {"LD",   "r,R4"     , 0                },
/*4A*/      {"DJNZ", "R4,o"     , REFFLAG | CODEREF},
/*4B*/      {"JR",   "OV,o"     , REFFLAG | CODEREF},
/*4C*/      {"LD",   "R4,#i"    , 0                },
/*4D*/      {"JP",   "OV,w"     , REFFLAG | CODEREF},
/*4E*/      {"INC",  "R4"       , 0                },
/*4F*/      {"WDH",  ""         , 0                },

/*50*/      {"POP",  "r"        , 0                },
/*51*/      {"POP",  "@r"       , 0                },
/*52*/      {"AND",  "+Rh,Rl"   , 0                },
/*53*/      {"AND",  "+Rh,@Rl"  , 0                },
/*54*/      {"AND",  "+r,x"     , 0                },
/*55*/      {"AND",  "+r,@x"    , 0                },
/*56*/      {"AND",  "r,#i"     , 0                },
/*57*/      {"AND",  "@r,#i"    , 0                },
/*58*/      {"LD",   "R5,r"     , 0                },
/*59*/      {"LD",   "r,R5"     , 0                },
/*5A*/      {"DJNZ", "R5,o"     , REFFLAG | CODEREF},
/*5B*/      {"JR",   "MI,o"     , REFFLAG | CODEREF},
/*5C*/      {"LD",   "R5,#i"    , 0                },
/*5D*/      {"JP",   "MI,w"     , REFFLAG | CODEREF},
/*5E*/      {"INC",  "R5"       , 0                },
/*5F*/      {"WDT",  ""         , 0                },

/*60*/      {"COM",  "r"        , 0                },
/*61*/      {"COM",  "@r"       , 0                },
/*62*/      {"TCM",  "+Rh,Rl"   , 0                },
/*63*/      {"TCM",  "+Rh,@Rl"  , 0                },
/*64*/      {"TCM",  "+r,x"     , 0                },
/*65*/      {"TCM",  "+r,@x"    , 0                },
/*66*/      {"TCM",  "r,#i"     , 0                },
/*67*/      {"TCM",  "@r,#i"    , 0                },
/*68*/      {"LD",   "R6,r"     , 0                },
/*69*/      {"LD",   "r,R6"     , 0                },
/*6A*/      {"DJNZ", "R6,o"     , REFFLAG | CODEREF},
/*6B*/      {"JR",   "EQ,o"     , REFFLAG | CODEREF},
/*6C*/      {"LD",   "R6,#i"    , 0                },
/*6D*/      {"JP",   "EQ,w"     , REFFLAG | CODEREF},
/*6E*/      {"INC",  "R6"       , 0                },
/*6F*/      {"STOP", ""         , 0                },

/*70*/      {"PUSH", "r"        , 0                },
/*71*/      {"PUSH", "@r"       , 0                },
/*72*/      {"TM",   "+Rh,Rl"   , 0                },
/*73*/      {"TM",   "+Rh,@Rl"  , 0                },
/*74*/      {"TM",   "+r,x"     , 0                },
/*75*/      {"TM",   "+r,@x"    , 0                },
/*76*/      {"TM",   "r,#i"     , 0                },
/*77*/      {"TM",   "@r,#i"    , 0                },
/*78*/      {"LD",   "R7,r"     , 0                },
/*79*/      {"LD",   "r,R7"     , 0                },
/*7A*/      {"DJNZ", "R7,o"     , REFFLAG | CODEREF},
/*7B*/      {"JR",   "ULT,o"    , REFFLAG | CODEREF},
/*7C*/      {"LD",   "R7,#i"    , 0                },
/*7D*/      {"JP",   "ULT,w"    , REFFLAG | CODEREF},
/*7E*/      {"INC",  "R7"       , 0                },
/*7F*/      {"HALT", ""         , 0                },

/*80*/      {"DECW", "r"        , 0                },
/*81*/      {"DECW", "@r"       , 0                },
/*82*/      {"LDE",  "+Rh,@RRl" , 0                },
/*83*/      {"LDEI", "+@Rh,@RRl", 0                },
/*84*/      {"",     ""         , 0                },
/*85*/      {"",     ""         , 0                },
/*86*/      {"",     ""         , 0                },
/*87*/      {"",     ""         , 0                },
/*88*/      {"LD",   "R8,r"     , 0                },
/*89*/      {"LD",   "r,R8"     , 0                },
/*8A*/      {"DJNZ", "R8,o"     , REFFLAG | CODEREF},
/*8B*/      {"JR",   "o"        , LFFLAG | REFFLAG | CODEREF},
/*8C*/      {"LD",   "R8,#i"    , 0                },
/*8D*/      {"JP",   "w"        , LFFLAG | REFFLAG | CODEREF},
/*8E*/      {"INC",  "R8"       , 0                },
/*8F*/      {"DI",   ""         , 0                },

/*90*/      {"RL",   "r"        , 0                },
/*91*/      {"RL",   "@r"       , 0                },
/*92*/      {"LDE",  "+@RRl,Rh" , 0                },
/*93*/      {"LDEI", "+@RRl,@Rh", 0                },
/*94*/      {"",     ""         , 0                },
/*95*/      {"",     ""         , 0                },
/*96*/      {"",     ""         , 0                },
/*97*/      {"",     ""         , 0                },
/*98*/      {"LD",   "R9,r"     , 0                },
/*99*/      {"LD",   "r,R9"     , 0                },
/*9A*/      {"DJNZ", "R9,o"     , REFFLAG | CODEREF},
/*9B*/      {"JR",   "GE,o"     , REFFLAG | CODEREF},
/*9C*/      {"LD",   "R9,#i"    , 0                },
/*9D*/      {"JP",   "GE,w"     , REFFLAG | CODEREF},
/*9E*/      {"INC",  "R9"       , 0                },
/*9F*/      {"EI",   ""         , 0                },

/*A0*/      {"INCW", "r"        , 0                },
/*A1*/      {"INCW", "@r"       , 0                },
/*A2*/      {"CP",   "+Rh,Rl"   , 0                },
/*A3*/      {"CP",   "+Rh,@Rl"  , 0                },
/*A4*/      {"CP",   "+r,x"     , 0                },
/*A5*/      {"CP",   "+r,@x"    , 0                },
/*A6*/      {"CP",   "r,#i"     , 0                },
/*A7*/      {"CP",   "@r,#i"    , 0                },
/*A8*/      {"LD",   "R10,r"    , 0                },
/*A9*/      {"LD",   "r,R10"    , 0                },
/*AA*/      {"DJNZ", "R10,o"    , REFFLAG | CODEREF},
/*AB*/      {"JR",   "GT,o"     , REFFLAG | CODEREF},
/*AC*/      {"LD",   "R10,#i"   , 0                },
/*AD*/      {"JP",   "GT,w"     , REFFLAG | CODEREF},
/*AE*/      {"INC",  "R10"      , 0                },
/*AF*/      {"RET",  ""         , LFFLAG           },

/*B0*/      {"CLR",  "r"        , 0                },
/*B1*/      {"CLR",  "@r"       , 0                },
/*B2*/      {"XOR",  "+Rh,Rl"   , 0                },
/*B3*/      {"XOR",  "+Rh,@Rl"  , 0                },
/*B4*/      {"XOR",  "+r,x"     , 0                },
/*B5*/      {"XOR",  "+r,@x"    , 0                },
/*B6*/      {"XOR",  "r,#i"     , 0                },
/*B7*/      {"XOR",  "@r,#i"    , 0                },
/*B8*/      {"LD",   "R11,r"    , 0                },
/*B9*/      {"LD",   "r,R11"    , 0                },
/*BA*/      {"DJNZ", "R11,o"    , REFFLAG | CODEREF},
/*BB*/      {"JR",   "UGT,o"    , REFFLAG | CODEREF},
/*BC*/      {"LD",   "R11,#i"   , 0                },
/*BD*/      {"JP",   "UGT,w"    , REFFLAG | CODEREF},
/*BE*/      {"INC",  "R11"      , 0                },
/*BF*/      {"IRET", ""         , LFFLAG           },

/*C0*/      {"RRC",  "r"        , 0                },
/*C1*/      {"RRC",  "@r"       , 0                },
/*C2*/      {"LDC",  "+Rh,@RRl" , 0                },
/*C3*/      {"LDCI", "+@Rh,@RRl", 0                },
/*C4*/      {"",     ""         , 0                },
/*C5*/      {"",     ""         , 0                },
/*C6*/      {"",     ""         , 0                },
/*C7*/      {"LD",   "+Rh,i(Rl)", 0                },
/*C8*/      {"LD",   "R12,r"    , 0                },
/*C9*/      {"LD",   "r,R12"    , 0                },
/*CA*/      {"DJNZ", "R12,o"    , REFFLAG | CODEREF},
/*CB*/      {"JR",   "NOV,o"    , REFFLAG | CODEREF},
/*CC*/      {"LD",   "R12,#i"   , 0                },
/*CD*/      {"JP",   "NOV,w"    , REFFLAG | CODEREF},
/*CE*/      {"INC",  "R12"      , 0                },
/*CF*/      {"RCF",  ""         , 0                },

/*D0*/      {"SR10",  "r"       , 0                },
/*D1*/      {"SR10",  "@r"      , 0                },
/*D2*/      {"LDC",  "+@RRl,Rh" , 0                },
/*D3*/      {"LDCI", "+@RRl,@Rh", 0                },
/*D4*/      {"CALL", "@r"       , 0                },
/*D5*/      {"",     ""         , 0                },
/*D6*/      {"CALL", "w"        , REFFLAG | CODEREF},
/*D7*/      {"LD",   "+i(Rl),Rh", 0                },
/*D8*/      {"LD",   "R13,r"    , 0                },
/*D9*/      {"LD",   "r,R13"    , 0                },
/*DA*/      {"DJNZ", "R13,o"    , REFFLAG | CODEREF},
/*DB*/      {"JR",   "PL,o"     , REFFLAG | CODEREF},
/*DC*/      {"LD",   "R13,#i"   , 0                },
/*DD*/      {"JP",   "PL,w"     , REFFLAG | CODEREF},
/*DE*/      {"INC",  "R13"      , 0                },
/*DF*/      {"SCF",  ""         , 0                },

/*E0*/      {"RR",   "r"        , 0                },
/*E1*/      {"RR",   "@r"       , 0                },
/*E2*/      {"",     ""         , 0                },
/*E3*/      {"LD",   "+Rh,@Rl"  , 0                },
// NOTE: The form "E4 dd rs" would disassemble to LD Rd,ss
// but there is already a form "d8 ss" so this can maybe refer
// to an E0-EF register just like the "x9" opcodes
/*E4*/      {"LD",   "+r,x"     , 0                },
/*E5*/      {"LD",   "+r,@x"    , 0                },
/*E6*/      {"LD",   "r,#i"     , 0                },
/*E7*/      {"LD",   "@r,#i"    , 0                },
/*E8*/      {"LD",   "R14,r"    , 0                },
/*E9*/      {"LD",   "r,R14"    , 0                },
/*EA*/      {"DJNZ", "R14,o"    , REFFLAG | CODEREF},
/*EB*/      {"JR",   "NZ,o"     , REFFLAG | CODEREF},
/*EC*/      {"LD",   "R14,#i"   , 0                },
/*ED*/      {"JP",   "NZ,w"     , REFFLAG | CODEREF},
/*EE*/      {"INC",  "R14"      , 0                },
/*EF*/      {"CCF",  ""         , 0                },

/*F0*/      {"SWAP", "r"        , 0                },
/*F1*/      {"SWAP", "@r"       , 0                },
/*F2*/      {"",     ""         , 0                },
/*F3*/      {"LD",   "+@Rh,Rl"  , 0                },
/*F4*/      {"",     ""         , 0                },
/*F5*/      {"LD",   "+@r,x"    , 0                },
/*F6*/      {"",     ""         , 0                },
/*F7*/      {"",     ""         , 0                },
/*F8*/      {"LD",   "R15,r"    , 0                },
/*F9*/      {"LD",   "r,R15"    , 0                },
/*FA*/      {"DJNZ", "R15,o"    , REFFLAG | CODEREF},
/*FB*/      {"JR",   "UGE,o"    , REFFLAG | CODEREF},
/*FC*/      {"LD",   "R15,#i"   , 0                },
/*FD*/      {"JP",   "UGE,w"    , REFFLAG | CODEREF},
/*FE*/      {"INC",  "R15"      , 0                },
/*FF*/      {"NOP",  ""         , 0                },
};


static const char *Fregs[16] = {
    "SIO"  , "TMR"  , "T1"   , "PRE1" , "T0"   , "PRE0" , "P2M"  , "P3M"  ,
    "P01M" , "IPR"  , "IRQ"  , "IMR"  , "FLAGS", "RP"   , "SPH"  , "SPL"
};


void DisZ8::Regnum(unsigned char reg, char *p)
{
    if (reg >= 0xF0) {
        strcpy(p, Fregs[reg - 0xF0]);
    } else {
        H2Str(reg, p);
    }
}


int DisZ8::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    unsigned short  ad;
    int             i, opcd, rpair;
    InstrPtr        instr;
    char            *p, *l;
    char            s[256];
    addr_t          ra;

    ad    = addr;
    opcd = ReadByte(ad++);
    int len = 1;
    instr = &Z8_opcdTable[opcd];

    strcpy(opcode, instr->op);
    strcpy(parms, instr->parms);
    lfref   = instr->lfref;
    refaddr = 0;

    rpair = 0;
    p = s;
    l = parms;
    while (*l) {
        switch (*l) {
#if 0
            case 'b':   // immediate byte without $
                i = ReadByte(ad++);
                len++;

//              H2Str(i, p);
                sprintf(p, "%.2X", i);
                p += strlen(p);
                break;
#endif
            case 'r': // 8-bit register number
                i = ReadByte(ad++);
                len++;

                // Ex = short register number except for x9 opcode (and E4 opcode?)
                if ((i & 0xF0) == 0xE0 && (opcd & 0x0F) != 0x09 && opcd != 0xE4) {
                    sprintf(p, "R%d", i & 0x0F);
                } else {
                    Regnum(i, p);
                }
                p += strlen(p);
                break;

            case 'x':   // saved register number
                i = rpair;

                // Ex = short register number
                if ((i & 0xF0) == 0xE0) {
                    sprintf(p, "R%d", i & 0x0F);
                } else {
                    Regnum(i, p);
                }
                p += strlen(p);
                break;

            case 'i':   // immediate byte
                i = ReadByte(ad++);
                len++;

                H2Str(i, p);
                p += strlen(p);
                break;
#if 0
            case 'j':   // short jump
                ra = ReadByte(ad++);
                len++;

                ra += ad & 0xFF00;
                RefStr(ra, p, lfref, refaddr);
                p += strlen(p);
                break;
#endif

            case '+':   // save a byte, usually register nibble pair
                rpair = ReadByte(ad++);
                len++;
                break;

            case 'h':   // high nibble of register pair
                i = (rpair >> 4) & 0x0F;

//              sprintf(p, "%.1X", i);
                sprintf(p, "%d", i);
                p += strlen(p);
                break;

            case 'l':   // low nibble of register pair
                i = rpair & 0x0F;

//              sprintf(p, "%.1X", i);
                sprintf(p, "%d", i);
                p += strlen(p);
                break;

#if 0
            case 'a':   // 8048 long jump
                ra = ((opcd << 3) & 0xFF00) | ReadByte(ad++);
                len++;

                RefStr(ra, p, lfref, refaddr);
                p += strlen(p);
                break;
#endif
            case 'w':   // immediate word
                ra = ReadWord(ad);
                ad += 2;
                len += 2;

                RefStr(ra, p, lfref, refaddr);
                p += strlen(p);
                break;

            case 'o':
                i = ReadByte(ad++);
                len++;
                if (i >= 128) {
                    i = i - 256;
                }
                ra = (ad + i) & 0xFFFF;

                RefStr(ra, p, lfref, refaddr);
                p += strlen(p);
                break;

            default:
                *p++ = *l;
                break;
        }
        l++;
    }
    *p = 0;

    strcpy(parms, s);

    if (opcode[0]==0 || rom.AddrOutRange(addr)) {
        strcpy(opcode, "???");
        H2Str(ReadByte(addr), parms);
        len     = 0;
        lfref   = 0;
        refaddr = 0;
    }

    return len;
}
