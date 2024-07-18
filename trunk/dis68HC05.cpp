// dis68HC05.cpp

static const char versionName[] = "Motorola 68HC05 disassembler";

#include "disline.h"
#define asmMode (!(disline.line_cols & disline.B_HEX))

#include "discpu.h"

class Dis68HC05 : public CPU {
public:
    Dis68HC05(const char *name, int subtype, int endian, int addrwid,
              char curAddrChr, char hexChr, const char *byteOp,
              const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


enum {
    // CPU types for instructions
    CPU_68HC05 = 1, // 68HC05
};


Dis68HC05 cpu_6805  ("6805",   CPU_68HC05, BIG_END, ADDR_16, '*', '$', "FCB", "FDB", "DL");
Dis68HC05 cpu_68HC05("68HC05", CPU_68HC05, BIG_END, ADDR_16, '*', '$', "FCB", "FDB", "DL");


Dis68HC05::Dis68HC05(const char *name, int subtype, int endian, int addrwid,
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
    _usefcc  = true;

    add_cpu();
}


// =====================================================


enum InstType
{
    iInherent,   // (no operand)
    iDirect,     // $dir
    iImmediate,  // #imm
    iLImmediate, // #imm16
    iRelative,   // reladdr
    iIX,         // ,X
    iIX1,        // $ofs8,X
    iIX2,        // $ofs16,X
    iExtended,   // addr16
    iBRelative,  // bit,$dir,reladdr
    iBSETBCLR,   // bit, $dir
};

struct InstrRec {
    const char      *op;    // mnemonic
    enum InstType   typ;    // instruction type
    uint8_t         cpu;    // cpu type flags (currently not used)
    uint8_t         lfref;  // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;


static const struct InstrRec M68HC05_opcdTable[] =
{
        // op        typ   cpu    lfref
/*00*/  {"BRSET", iBRelative , 0, REFFLAG | CODEREF},
/*01*/  {"BRCLR", iBRelative , 0, REFFLAG | CODEREF},
/*02*/  {"BRSET", iBRelative , 0, REFFLAG | CODEREF},
/*03*/  {"BRCLR", iBRelative , 0, REFFLAG | CODEREF},
/*04*/  {"BRSET", iBRelative , 0, REFFLAG | CODEREF},
/*05*/  {"BRCLR", iBRelative , 0, REFFLAG | CODEREF},
/*06*/  {"BRSET", iBRelative , 0, REFFLAG | CODEREF},
/*07*/  {"BRCLR", iBRelative , 0, REFFLAG | CODEREF},
/*08*/  {"BRSET", iBRelative , 0, REFFLAG | CODEREF},
/*09*/  {"BRCLR", iBRelative , 0, REFFLAG | CODEREF},
/*0A*/  {"BRSET", iBRelative , 0, REFFLAG | CODEREF},
/*0B*/  {"BRCLR", iBRelative , 0, REFFLAG | CODEREF},
/*0C*/  {"BRSET", iBRelative , 0, REFFLAG | CODEREF},
/*0D*/  {"BRCLR", iBRelative , 0, REFFLAG | CODEREF},
/*0E*/  {"BRSET", iBRelative , 0, REFFLAG | CODEREF},
/*0F*/  {"BRCLR", iBRelative , 0, REFFLAG | CODEREF},

/*10*/  {"BSET" , iBSETBCLR  , 0, 0                },
/*11*/  {"BCLR" , iBSETBCLR  , 0, 0                },
/*12*/  {"BSET" , iBSETBCLR  , 0, 0                },
/*13*/  {"BCLR" , iBSETBCLR  , 0, 0                },
/*14*/  {"BSET" , iBSETBCLR  , 0, 0                },
/*15*/  {"BCLR" , iBSETBCLR  , 0, 0                },
/*16*/  {"BSET" , iBSETBCLR  , 0, 0                },
/*17*/  {"BCLR" , iBSETBCLR  , 0, 0                },
/*18*/  {"BSET" , iBSETBCLR  , 0, 0                },
/*19*/  {"BCLR" , iBSETBCLR  , 0, 0                },
/*1A*/  {"BSET" , iBSETBCLR  , 0, 0                },
/*1B*/  {"BCLR" , iBSETBCLR  , 0, 0                },
/*1C*/  {"BSET" , iBSETBCLR  , 0, 0                },
/*1D*/  {"BCLR" , iBSETBCLR  , 0, 0                },
/*1E*/  {"BSET" , iBSETBCLR  , 0, 0                },
/*1F*/  {"BCLR" , iBSETBCLR  , 0, 0                },

/*20*/  {"BRA"  , iRelative  , 0, LFFLAG | REFFLAG | CODEREF},
/*21*/  {"BRN"  , iRelative  , 0, 0                },
/*22*/  {"BHI"  , iRelative  , 0, REFFLAG | CODEREF},
/*23*/  {"BLS"  , iRelative  , 0, REFFLAG | CODEREF},
/*24*/  {"BCC"  , iRelative  , 0, REFFLAG | CODEREF}, // BCC / BHS
/*25*/  {"BCS"  , iRelative  , 0, REFFLAG | CODEREF}, // BCS / BLO
/*26*/  {"BNE"  , iRelative  , 0, REFFLAG | CODEREF},
/*27*/  {"BEQ"  , iRelative  , 0, REFFLAG | CODEREF},
/*28*/  {"BHCC" , iRelative  , 0, REFFLAG | CODEREF},
/*29*/  {"BHCS" , iRelative  , 0, REFFLAG | CODEREF},
/*2A*/  {"BPL"  , iRelative  , 0, REFFLAG | CODEREF},
/*2B*/  {"BMI"  , iRelative  , 0, REFFLAG | CODEREF},
/*2C*/  {"BMC"  , iRelative  , 0, REFFLAG | CODEREF},
/*2D*/  {"BMS"  , iRelative  , 0, REFFLAG | CODEREF},
/*2E*/  {"BIL"  , iRelative  , 0, REFFLAG | CODEREF},
/*2F*/  {"BIH"  , iRelative  , 0, REFFLAG | CODEREF},

/*30*/  {"NEG"  , iDirect    , 0, 0                },
/*31*/  {""     , iDirect    , 0, 0                },
/*32*/  {""     , iDirect    , 0, 0                },
/*33*/  {"COM"  , iDirect    , 0, 0                },
/*34*/  {"LSR"  , iDirect    , 0, 0                },
/*35*/  {""     , iDirect    , 0, 0                },
/*36*/  {"ROR"  , iDirect    , 0, 0                },
/*37*/  {"ASR"  , iDirect    , 0, 0                },
/*38*/  {"ASL"  , iDirect    , 0, 0                }, // ASL/LSL
/*39*/  {"ROL"  , iDirect    , 0, 0                },
/*3A*/  {"DEC"  , iDirect    , 0, 0                },
/*3B*/  {""     , iDirect    , 0, 0                },
/*3C*/  {"INC"  , iDirect    , 0, 0                },
/*3D*/  {"TST"  , iDirect    , 0, 0                },
/*3E*/  {""     , iDirect    , 0, 0                },
/*3F*/  {"CLR"  , iDirect    , 0, 0                },

/*40*/  {"NEGA" , iInherent  , 0, 0                },
/*41*/  {""     , iInherent  , 0, 0                },
/*42*/  {"MUL"  , iInherent  , 0, 0                },
/*43*/  {"COMA" , iInherent  , 0, 0                },
/*44*/  {"LSRA" , iInherent  , 0, 0                },
/*45*/  {""     , iInherent  , 0, 0                },
/*46*/  {"RORA" , iInherent  , 0, 0                },
/*47*/  {"ASRA" , iInherent  , 0, 0                },
/*48*/  {"ASLA" , iInherent  , 0, 0                }, // ASL/LSL
/*49*/  {"ROLA" , iInherent  , 0, 0                },
/*4A*/  {"DECA" , iInherent  , 0, 0                },
/*4B*/  {""     , iInherent  , 0, 0                },
/*4C*/  {"INCA" , iInherent  , 0, 0                },
/*4D*/  {"TSTA" , iInherent  , 0, 0                },
/*4E*/  {""     , iInherent  , 0, 0                },
/*4F*/  {"CLRA" , iInherent  , 0, 0                },

/*50*/  {"NEGX" , iInherent  , 0, 0                },
/*51*/  {""     , iInherent  , 0, 0                },
/*52*/  {""     , iInherent  , 0, 0                },
/*53*/  {"COMX" , iInherent  , 0, 0                },
/*54*/  {"LSRX" , iInherent  , 0, 0                },
/*55*/  {""     , iInherent  , 0, 0                },
/*56*/  {"RORX" , iInherent  , 0, 0                },
/*57*/  {"ASRX" , iInherent  , 0, 0                },
/*58*/  {"ASLX" , iInherent  , 0, 0                }, // ASL/LSL
/*59*/  {"ROLX" , iInherent  , 0, 0                },
/*5A*/  {"DECX" , iInherent  , 0, 0                },
/*5B*/  {""     , iInherent  , 0, 0                },
/*5C*/  {"INCX" , iInherent  , 0, 0                },
/*5D*/  {"TSTX" , iInherent  , 0, 0                },
/*5E*/  {""     , iInherent  , 0, 0                },
/*5F*/  {"CLRX" , iInherent  , 0, 0                },

/*60*/  {"NEG"  , iIX1       , 0, 0                },
/*61*/  {""     , iIX1       , 0, 0                },
/*62*/  {""     , iIX1       , 0, 0                },
/*63*/  {"COM"  , iIX1       , 0, 0                },
/*64*/  {"LSR"  , iIX1       , 0, 0                },
/*65*/  {""     , iIX1       , 0, 0                },
/*66*/  {"ROR"  , iIX1       , 0, 0                },
/*67*/  {"ASR"  , iIX1       , 0, 0                },
/*68*/  {"ASL"  , iIX1       , 0, 0                }, // ASL/LSL
/*69*/  {"ROL"  , iIX1       , 0, 0                },
/*6A*/  {"DEC"  , iIX1       , 0, 0                },
/*6B*/  {""     , iIX1       , 0, 0                },
/*6C*/  {"INC"  , iIX1       , 0, 0                },
/*6D*/  {"TST"  , iIX1       , 0, 0                },
/*6E*/  {""     , iIX1       , 0, 0                },
/*6F*/  {"CLR"  , iIX1       , 0, 0                },

/*70*/  {"NEG"  , iIX        , 0, 0                },
/*71*/  {""     , iIX        , 0, 0                },
/*72*/  {""     , iIX        , 0, 0                },
/*73*/  {"COM"  , iIX        , 0, 0                },
/*74*/  {"LSR"  , iIX        , 0, 0                },
/*75*/  {""     , iIX        , 0, 0                },
/*76*/  {"ROR"  , iIX        , 0, 0                },
/*77*/  {"ASR"  , iIX        , 0, 0                },
/*78*/  {"ASL"  , iIX        , 0, 0                }, // ASL/LSL
/*79*/  {"ROL"  , iIX        , 0, 0                },
/*7A*/  {"DEC"  , iIX        , 0, 0                },
/*7B*/  {""     , iIX        , 0, 0                },
/*7C*/  {"INC"  , iIX        , 0, 0                },
/*7D*/  {"TST"  , iIX        , 0, 0                },
/*7E*/  {""     , iIX        , 0, 0                },
/*7F*/  {"CLR"  , iIX        , 0, 0                },

/*80*/  {"RTI"  , iInherent  , 0, LFFLAG           },
/*81*/  {"RTS"  , iInherent  , 0, LFFLAG           },
/*82*/  {""     , iInherent  , 0, 0                },
/*83*/  {"SWI"  , iInherent  , 0, 0                },
/*84*/  {""     , iInherent  , 0, 0                },
/*85*/  {""     , iInherent  , 0, 0                },
/*86*/  {""     , iInherent  , 0, 0                },
/*87*/  {""     , iInherent  , 0, 0                },
/*88*/  {""     , iInherent  , 0, 0                },
/*89*/  {""     , iInherent  , 0, 0                },
/*8A*/  {""     , iInherent  , 0, 0                },
/*8B*/  {""     , iInherent  , 0, 0                },
/*8C*/  {""     , iInherent  , 0, 0                },
/*8D*/  {""     , iInherent  , 0, 0                },
/*8E*/  {"STOP" , iInherent  , 0, 0                },
/*8F*/  {"WAIT" , iInherent  , 0, 0                },

/*90*/  {""     , iInherent  , 0, 0                },
/*91*/  {""     , iInherent  , 0, 0                },
/*92*/  {""     , iInherent  , 0, 0                },
/*93*/  {""     , iInherent  , 0, 0                },
/*94*/  {""     , iInherent  , 0, 0                },
/*95*/  {""     , iInherent  , 0, 0                },
/*96*/  {""     , iInherent  , 0, 0                },
/*97*/  {"TAX"  , iInherent  , 0, 0                },
/*98*/  {"CLC"  , iInherent  , 0, 0                },
/*99*/  {"SEC"  , iInherent  , 0, 0                },
/*9A*/  {"CLI"  , iInherent  , 0, 0                },
/*9B*/  {"SEI"  , iInherent  , 0, 0                },
/*9C*/  {"RSP"  , iInherent  , 0, 0                },
/*9D*/  {"NOP"  , iInherent  , 0, 0                },
/*9E*/  {""     , iInherent  , 0, 0                },
/*9F*/  {"TXA"  , iInherent  , 0, 0                },

/*A0*/  {"SUB"  , iImmediate , 0, 0                },
/*A1*/  {"CMP"  , iImmediate , 0, 0                },
/*A2*/  {"SBC"  , iImmediate , 0, 0                },
/*A3*/  {"CPX"  , iImmediate , 0, 0                },
/*A4*/  {"AND"  , iImmediate , 0, 0                },
/*A5*/  {"BIT"  , iImmediate , 0, 0                },
/*A6*/  {"LDA"  , iImmediate , 0, 0                },
/*A7*/  {""     , iImmediate , 0, 0                },
/*A8*/  {"EOR"  , iImmediate , 0, 0                },
/*A9*/  {"ADC"  , iImmediate , 0, 0                },
/*AA*/  {"ORA"  , iImmediate , 0, 0                },
/*AB*/  {"ADD"  , iImmediate , 0, 0                },
/*AC*/  {""     , iImmediate , 0, 0                },
/*AD*/  {"BSR"  , iRelative  , 0, REFFLAG | CODEREF},
/*AE*/  {"LDX"  , iImmediate , 0, 0                },
/*AF*/  {""     , iImmediate , 0, 0                },

/*B0*/  {"SUB"  , iDirect    , 0, 0                },
/*B1*/  {"CMP"  , iDirect    , 0, 0                },
/*B2*/  {"SBC"  , iDirect    , 0, 0                },
/*B3*/  {"CPX"  , iDirect    , 0, 0                },
/*B4*/  {"AND"  , iDirect    , 0, 0                },
/*B5*/  {"BIT"  , iDirect    , 0, 0                },
/*B6*/  {"LDA"  , iDirect    , 0, 0                },
/*B7*/  {"STA"  , iDirect    , 0, 0                },
/*B8*/  {"EOR"  , iDirect    , 0, 0                },
/*B9*/  {"ADC"  , iDirect    , 0, 0                },
/*BA*/  {"ORA"  , iDirect    , 0, 0                },
/*BB*/  {"ADD"  , iDirect    , 0, 0                },
/*BC*/  {"JMP"  , iDirect    , 0, LFFLAG           },
/*BD*/  {"JSR"  , iDirect    , 0, REFFLAG | CODEREF},
/*BE*/  {"LDX"  , iDirect    , 0, 0                },
/*BF*/  {"STX"  , iDirect    , 0, 0                },

/*C0*/  {"SUB"  , iExtended  , 0, 0                },
/*C1*/  {"CMP"  , iExtended  , 0, 0                },
/*C2*/  {"SBC"  , iExtended  , 0, 0                },
/*C3*/  {"CPX"  , iExtended  , 0, 0                },
/*C4*/  {"AND"  , iExtended  , 0, 0                },
/*C5*/  {"BIT"  , iExtended  , 0, 0                },
/*C6*/  {"LDA"  , iExtended  , 0, 0                },
/*C7*/  {"STA"  , iExtended  , 0, 0                },
/*C8*/  {"EOR"  , iExtended  , 0, 0                },
/*C9*/  {"ADC"  , iExtended  , 0, 0                },
/*CA*/  {"ORA"  , iExtended  , 0, 0                },
/*CB*/  {"ADD"  , iExtended  , 0, 0                },
/*CC*/  {"JMP"  , iExtended  , 0, LFFLAG | REFFLAG | CODEREF},
/*CD*/  {"JSR"  , iExtended  , 0, REFFLAG | CODEREF},
/*CE*/  {"LDX"  , iExtended  , 0, 0                },
/*CF*/  {"STX"  , iExtended  , 0, 0                },

/*D0*/  {"SUB"  , iIX2       , 0, 0                },
/*D1*/  {"CMP"  , iIX2       , 0, 0                },
/*D2*/  {"SBC"  , iIX2       , 0, 0                },
/*D3*/  {"CPX"  , iIX2       , 0, 0                },
/*D4*/  {"AND"  , iIX2       , 0, 0                },
/*D5*/  {"BIT"  , iIX2       , 0, 0                },
/*D6*/  {"LDA"  , iIX2       , 0, 0                },
/*D7*/  {"STA"  , iIX2       , 0, 0                },
/*D8*/  {"EOR"  , iIX2       , 0, 0                },
/*D9*/  {"ADC"  , iIX2       , 0, 0                },
/*DA*/  {"ORA"  , iIX2       , 0, 0                },
/*DB*/  {"ADD"  , iIX2       , 0, 0                },
/*DC*/  {"JMP"  , iIX2       , 0, LFFLAG           },
/*DD*/  {"JSR"  , iIX2       , 0, 0                },
/*DE*/  {"LDX"  , iIX2       , 0, 0                },
/*DF*/  {"STX"  , iIX2       , 0, 0                },

/*E0*/  {"SUB"  , iIX1       , 0, 0                },
/*E1*/  {"CMP"  , iIX1       , 0, 0                },
/*E2*/  {"SBC"  , iIX1       , 0, 0                },
/*E3*/  {"CPX"  , iIX1       , 0, 0                },
/*E4*/  {"AND"  , iIX1       , 0, 0                },
/*E5*/  {"BIT"  , iIX1       , 0, 0                },
/*E6*/  {"LDA"  , iIX1       , 0, 0                },
/*E7*/  {"STA"  , iIX1       , 0, 0                },
/*E8*/  {"EOR"  , iIX1       , 0, 0                },
/*E9*/  {"ADC"  , iIX1       , 0, 0                },
/*EA*/  {"ORA"  , iIX1       , 0, 0                },
/*EB*/  {"ADD"  , iIX1       , 0, 0                },
/*EC*/  {"JMP"  , iIX1       , 0, LFFLAG           },
/*ED*/  {"JSR"  , iIX1       , 0, 0                },
/*EE*/  {"LDX"  , iIX1       , 0, 0                },
/*EF*/  {"STX"  , iIX1       , 0, 0                },

/*F0*/  {"SUB"  , iIX        , 0, 0                },
/*F1*/  {"CMP"  , iIX        , 0, 0                },
/*F2*/  {"SBC"  , iIX        , 0, 0                },
/*F3*/  {"CPX"  , iIX        , 0, 0                },
/*F4*/  {"AND"  , iIX        , 0, 0                },
/*F5*/  {"BIT"  , iIX        , 0, 0                },
/*F6*/  {"LDA"  , iIX        , 0, 0                },
/*F7*/  {"STA"  , iIX        , 0, 0                },
/*F8*/  {"EOR"  , iIX        , 0, 0                },
/*F9*/  {"ADC"  , iIX        , 0, 0                },
/*FA*/  {"ORA"  , iIX        , 0, 0                },
/*FB*/  {"ADD"  , iIX        , 0, 0                },
/*FC*/  {"JMP"  , iIX        , 0, LFFLAG           },
/*FD*/  {"JSR"  , iIX        , 0, 0                },
/*FE*/  {"LDX"  , iIX        , 0, 0                },
/*FF*/  {"STX"  , iIX        , 0, 0                },
};


int Dis68HC05::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    addr_t      ad;
    int         i, j, k;
    InstrPtr    instr;
    char        s[256];
    addr_t      ra;

    ad    = addr;
    i     = ReadByte(ad++);
    int len = 1;

    // get main page opcode
    instr = &M68HC05_opcdTable[i];

    strcpy(opcode, instr->op);
    lfref    = instr->lfref;
    parms[0] = 0;
    refaddr  = 0;

    // check if instruction is supported for this CPU
    if (opcode[0]) {
        switch(instr->typ) {
            case iInherent:
                /* nothing */;
                break;

            case iDirect:
                i = ReadByte(ad++); // direct addr
                if (asmMode) {
                    sprintf(parms, "<$%.2X", i);
                } else {
                    sprintf(parms,  "$%.2X", i);
                }
                len++;
                break;

            case iImmediate: // #imm
                i = ReadByte(ad++); // imm
                sprintf(parms, "#$%.2X", i);
                len++;
                break;

            case iLImmediate: // #imm16
                ra = ReadWord(ad); // imm16
                len += 2;
                i = ReadByte(addr);
                if (i == 0x83 || i == 0xC3 || i == 0xCC ||
                   (i == 0x10 && ReadByte(addr+1) == 0x83)) {
                    // SUBD, ADD, LDD, CMPD do not generate references
                    sprintf(parms, "#$%.4X", (int) ra);
                } else {
                    RefStr(ra, s, lfref, refaddr);
                    sprintf(parms, "#%s", s);
                }
                break;

            case iRelative:
                i = ReadByte(ad++); // branch
                if (i == 0xFF && (lfref & REFFLAG)) {
                    // offset FF is suspicious
                    lfref |= RIPSTOP;
                }
                len++;
                if (i >= 128) {
                    i = i - 256;
                }
                ra = (ad + i) & 0xFFFF;
                i = lfref; // ignore flag changes from RefStr
                RefStr(ra, parms, lfref, refaddr);
                lfref = i;
                break;

            case iIX: // ,X
                sprintf(parms, ",X");
                break;

            case iIX1: // ofs8,X
                i = ReadByte(ad++); // ofs8
                sprintf(parms, "$%.2X,X", i);
                len++;
                break;

            case iIX2: // ofs16,X
                i = ReadWord(ad); // ofs16
                sprintf(parms, "$%.4X,X", i);
                ad += 2;
                len += 2;
                break;

            case iBRelative: // bit,dir,addr
                i = (i >> 1) & 7;   // bit
                j = ReadByte(ad++); // direct addr
                k = ReadByte(ad++); // branch
                if (k == 0xFF) {
                    // offset FF is suspicious
                    lfref |= RIPSTOP;
                }
                len += 2;
                if (k >= 128) {
                    k = k - 256;
                }
                ra = (ad + k) & 0xFFFF;
                RefStr(ra, s, lfref, refaddr);
                sprintf(parms, "%d,$%.2X,%s", i, j, s);
                break;

            case iExtended:
                ra = ReadWord(ad); // addr16
                ad += 2;
                len += 2;
                if (ra < 0x0100) { // add ">" to prevent direct addressing
                    strcpy(parms,">");
                }
                RefStr(ra, parms+strlen(parms), lfref, refaddr);
                break;

            case iBSETBCLR: // bit,$dir
                i = (i >> 1) & 7;   // bit
                j = ReadByte(ad++); // dir
                sprintf(parms, "%d,$%.2X", i, j);
                len += 1;
                break;
        }
    } else {
        opcode[0] = 0;
    }

    // rip-stop checks
    if (opcode[0]) {
        switch (ReadByte(addr)) {
            case 0xFF: // STX ,X
                if ((ReadByte(addr+1) == 0xFF) &&
                    (ReadByte(addr+2) == 0xFF)) {
                    lfref |= RIPSTOP;
                }
                break;
        }
    }

    if (opcode[0]==0 || rom.AddrOutRange(addr)) {
        strcpy(opcode, "???");
        sprintf(parms, "$%.2X", ReadByte(addr));
        len     = 0;
        lfref   = 0;
        refaddr = 0;
    }

    return len;
}
