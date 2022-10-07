// disz80.cpp

static const char versionName[] = "Zilog Z80 and Intel 8085 disassembler";

#include "discpu.h"

class DisZ80 : public CPU {
public:
    DisZ80(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *revwordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


enum { // cur_CPU->subtype
    CPU_8080,
    CPU_8085,
    CPU_Z80,
    CPU_Z180,
    CPU_GB,
};

enum {
    INSTR_NORMAL,
    INSTR_CB,
    INSTR_DD,
    INSTR_FD,
    INSTR_ED,
};


DisZ80 cpu_8080 ("8080",  CPU_8080, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DRW", "DL");
DisZ80 cpu_Z8080("Z8080", CPU_8080, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DRW", "DL");
DisZ80 cpu_8085 ("8085",  CPU_8085, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DRW", "DL");
DisZ80 cpu_Z8085("Z8085", CPU_8085, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DRW", "DL");
DisZ80 cpu_z80  ("Z80",   CPU_Z80,  LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DRW", "DL");
DisZ80 cpu_z180 ("Z180",  CPU_Z180, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DRW", "DL");
DisZ80 cpu_GB   ("GB",    CPU_GB,   LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DRW", "DL");
DisZ80 cpu_GBZ80("GBZ80", CPU_GB,   LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DRW", "DL");


DisZ80::DisZ80(const char *name, int subtype, int endian, int addrwid,
               char curAddrChr, char hexChr, const char *byteOp,
               const char *wordOp, const char *revwordOp, const char *longOp)
{
    _file    = __FILE__;
    _name    = name;
    _version = versionName;
    _subtype = subtype;
    _dbopcd  = byteOp;
    _dwopcd  = wordOp;
    _dlopcd  = longOp;
    _drwopcd = revwordOp;
    _curpc   = curAddrChr;
    _endian  = endian;
    _hexchr  = hexChr;
    _addrwid = addrwid;

    add_cpu();
}


// =====================================================


struct InstrRec {
    // table always 256 entries
    const char      *op;    // mnemonic
    const char      *parms; // parms
    uint8_t         lfref;  // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;


// sparse instructions searched by opcode
// table is terminated by instr.op == NULL
struct SearchInstrRec {
    int             opcode;
    struct InstrRec instr;
};


// =====================================================
static const struct InstrRec *SearchInstr(const struct SearchInstrRec *table, int opcode)
{
    for ( ; table->instr.op; table++) {
        if (table->opcode == opcode) {
            return &table->instr;
        }
    }

    return NULL;
}


// 8085 main page instruction changes
static const struct SearchInstrRec Z80_opcdTable_8085[] =
{
    { 0x08, {"DSUB", ""        , 0                } },
    { 0x10, {"ARHL", ""        , 0                } },
    { 0x18, {"RDEL", ""        , 0                } },
    { 0x20, {"RIM",  ""        , 0                } },
    { 0x28, {"LDHI", "b"       , 0                } },
    { 0x30, {"SIM",  ""        , 0                } },
    { 0x38, {"LDSI", "b"       , 0                } },
    { 0xCB, {"RSTV", ""        , 0                } },
    { 0xD9, {"SHLX", ""        , 0                } },
    { 0xDD, {"JNX5", "w"       , REFFLAG | CODEREF} },
    { 0xED, {"LHLX", ""        , 0                } },
    { 0xFD, {"JX5",  "w"       , REFFLAG | CODEREF} },
    { /* END */ }
};

// 8080 instruction deletion for main page (using 8085 list)
static const InstrRec Z80_badinstr_8080 =
    { "",     ""        , 0                 };

static const struct SearchInstrRec Z80_opcdTable_Z180[] =
{   // EDxx
    { 0x00, {"IN0",  "B,(b)"   , 0                } },
    { 0x01, {"OUT0", "(b),B"   , 0                } },
    { 0x04, {"TST",  "B"       , 0                } },
    { 0x08, {"IN0",  "C,(b)"   , 0                } },
    { 0x09, {"OUT0", "(b),C"   , 0                } },
    { 0x0C, {"TST",  "C"       , 0                } },

    { 0x10, {"IN0",  "D,(b)"   , 0                } },
    { 0x11, {"OUT0", "(b),D"   , 0                } },
    { 0x14, {"TST",  "D"       , 0                } },
    { 0x18, {"IN0",  "E,(b)"   , 0                } },
    { 0x19, {"OUT0", "(b),E"   , 0                } },
    { 0x1C, {"TST",  "E"       , 0                } },

    { 0x20, {"IN0",  "H,(b)"   , 0                } },
    { 0x21, {"OUT0", "(b),H"   , 0                } },
    { 0x24, {"TST",  "H"       , 0                } },
    { 0x28, {"IN0",  "L,(b)"   , 0                } },
    { 0x29, {"OUT0", "(b),L"   , 0                } },
    { 0x2C, {"TST",  "L"       , 0                } },

    { 0x30, {"IN0",  "F,(b)"   , 0                } },
    { 0x31, {"OUT0", "(b),F"   , 0                } },
    { 0x34, {"TST",  "(HL)"    , 0                } },
    { 0x38, {"IN0",  "A,(b)"   , 0                } },
    { 0x39, {"OUT0", "(b),A"   , 0                } },
    { 0x3C, {"TST",  "A"       , 0                } },

    { 0x4C, {"MLT",  "BC"      , 0                } },

    { 0x5C, {"MLT",  "DE"      , 0                } },

    { 0x64, {"TST",  "(C),b"   , 0                } },
    { 0x6C, {"MLT",  "HL"      , 0                } },

    { 0x74, {"TSTIO","b"       , 0                } },
    { 0x76, {"SLP",  ""        , 0                } },
    { 0x7C, {"MLT",  "SP"      , 0                } },

    { 0x83, {"OTIM", ""        , 0                } },
    { 0x8B, {"OTDM", ""        , 0                } },

    { 0x93, {"OTIMR",""        , 0                } },
    { 0x9B, {"OTDMR",""        , 0                } },

    { /* END */ }
};

// GB-Z80 main page instruction changes
static const struct SearchInstrRec Z80_opcdTable_GB[] =
{   // CBxx
    { 0x08, {"LD",   "(w),SP"   , 0                } },
    { 0x10, {"STOP", "; b"      , REFFLAG | CODEREF} },
    { 0x22, {"LD",   "(HL+),A"  , 0                } },
    { 0x2A, {"LD",   "(HL-),A"  , 0                } },
    { 0x32, {"LD",   "(HL-),A"  , 0                } },
    { 0x3A, {"LD",   "A,(HL-)"  , 0                } },
    { 0xD9, {"RETI", ""         , LFFLAG           } },
    { 0xE0, {"LDH",  "(b),A"    , 0                } }, // $FF00+n
    { 0xE2, {"LD",   "(C),A"    , REFFLAG | CODEREF} }, // $FF00+C
    { 0xE8, {"ADD",  "SP,b"     , 0                } },
    { 0xEA, {"LD",   "(w),A"    , REFFLAG | CODEREF} },
    { 0xF0, {"LDH",  "A,(b)"    , 0                } }, // $FF00+n
    { 0xF2, {"LD",   "A,(C)"    , REFFLAG | CODEREF} }, // $FF00+C
    { 0xF8, {"LD",   "HL,SP+b"  , 0                } },
    { 0xFA, {"LD",   "A,(w)"    , REFFLAG | CODEREF} },
    { /* END */ }
};

// GB-Z80 CB page instruction changes
static const struct SearchInstrRec Z80_opcdTable_GB_CB[] =
{   // CBxx
    { 0x30, {"SWAP", "B"        , 0                } },
    { 0x31, {"SWAP", "C"        , 0                } },
    { 0x32, {"SWAP", "D"        , 0                } },
    { 0x33, {"SWAP", "E"        , 0                } },
    { 0x34, {"SWAP", "H"        , 0                } },
    { 0x35, {"SWAP", "L"        , 0                } },
    { 0x36, {"SWAP", "(HL)"     , 0                } },
    { 0x37, {"SWAP", "A"        , 0                } },
    { /* END */ }
};

static const struct InstrRec Z80_opcdTable[] =
{           // op     parms            lfref
/*00*/      {"NOP",  ""        , 0                },
/*01*/      {"LD",   "BC,w"    , 0                },
/*02*/      {"LD",   "(BC),A"  , 0                },
/*03*/      {"INC",  "BC"      , 0                },
/*04*/      {"INC",  "B"       , 0                },
/*05*/      {"DEC",  "B"       , 0                },
/*06*/      {"LD",   "B,b"     , 0                },
/*07*/      {"RLCA", ""        , 0                },
/*08*/      {"EX",   "AF,AF'"  , 0                },
/*09*/      {"ADD",  "HL,BC"   , 0                },
/*0A*/      {"LD",   "A,(BC)"  , 0                },
/*0B*/      {"DEC",  "BC"      , 0                },
/*0C*/      {"INC",  "C"       , 0                },
/*0D*/      {"DEC",  "C"       , 0                },
/*0E*/      {"LD",   "C,b"     , 0                },
/*0F*/      {"RRCA", ""        , 0                },

/*10*/      {"DJNZ", "r"       , REFFLAG | CODEREF},
/*11*/      {"LD",   "DE,w"    , 0                },
/*12*/      {"LD",   "(DE),A"  , 0                },
/*13*/      {"INC",  "DE"      , 0                },
/*14*/      {"INC",  "D"       , 0                },
/*15*/      {"DEC",  "D"       , 0                },
/*16*/      {"LD",   "D,b"     , 0                },
/*17*/      {"RLA",  ""        , 0                },
/*18*/      {"JR",   "r"       , LFFLAG | REFFLAG | CODEREF},
/*19*/      {"ADD",  "HL,DE"   , 0                },
/*1A*/      {"LD",   "A,(DE)"  , 0                },
/*1B*/      {"DEC",  "DE"      , 0                },
/*1C*/      {"INC",  "E"       , 0                },
/*1D*/      {"DEC",  "E"       , 0                },
/*1E*/      {"LD",   "E,b"     , 0                },
/*1F*/      {"RRA",  ""        , 0                },

/*20*/      {"JR",   "NZ,r"    , REFFLAG | CODEREF},
/*21*/      {"LD",   "HL,w"    , 0                },
/*22*/      {"LD",   "(w),HL"  , 0                },
/*23*/      {"INC",  "HL"      , 0                },
/*24*/      {"INC",  "H"       , 0                },
/*25*/      {"DEC",  "H"       , 0                },
/*26*/      {"LD",   "H,b"     , 0                },
/*27*/      {"DAA",  ""        , 0                },
/*28*/      {"JR",   "Z,r"     , REFFLAG | CODEREF},
/*29*/      {"ADD",  "HL,HL"   , 0                },
/*2A*/      {"LD",   "HL,(w)"  , 0                },
/*2B*/      {"DEC",  "HL"      , 0                },
/*2C*/      {"INC",  "L"       , 0                },
/*2D*/      {"DEC",  "L"       , 0                },
/*2E*/      {"LD",   "L,b"     , 0                },
/*2F*/      {"CPL",  ""        , 0                },

/*30*/      {"JR",   "NC,r"    , REFFLAG | CODEREF},
/*31*/      {"LD",   "SP,w"    , 0                },
/*32*/      {"LD",   "(w),A"   , 0                },
/*33*/      {"INC",  "SP"      , 0                },
/*34*/      {"INC",  "(HL)"    , 0                },
/*35*/      {"DEC",  "(HL)"    , 0                },
/*36*/      {"LD",   "(HL),b"  , 0                },
/*37*/      {"SCF",  ""        , 0                },
/*38*/      {"JR",   "C,r"     , REFFLAG | CODEREF},
/*39*/      {"ADD",  "HL,SP"   , 0                },
/*3A*/      {"LD",   "A,(w)"   , 0                },
/*3B*/      {"DEC",  "SP"      , 0                },
/*3C*/      {"INC",  "A"       , 0                },
/*3D*/      {"DEC",  "A"       , 0                },
/*3E*/      {"LD",   "A,b"     , 0                },
/*3F*/      {"CCF",  ""        , 0                },

/*40*/      {"LD",   "B,B"     , RIPSTOP          },
/*41*/      {"LD",   "B,C"     , 0                },
/*42*/      {"LD",   "B,D"     , 0                },
/*43*/      {"LD",   "B,E"     , 0                },
/*44*/      {"LD",   "B,H"     , 0                },
/*45*/      {"LD",   "B,L"     , 0                },
/*46*/      {"LD",   "B,(HL)"  , 0                },
/*47*/      {"LD",   "B,A"     , 0                },
/*48*/      {"LD",   "C,B"     , 0                },
/*49*/      {"LD",   "C,C"     , RIPSTOP          },
/*4A*/      {"LD",   "C,D"     , 0                },
/*4B*/      {"LD",   "C,E"     , 0                },
/*4C*/      {"LD",   "C,H"     , 0                },
/*4D*/      {"LD",   "C,L"     , 0                },
/*4E*/      {"LD",   "C,(HL)"  , 0                },
/*4F*/      {"LD",   "C,A"     , 0                },

/*50*/      {"LD",   "D,B"     , 0                },
/*51*/      {"LD",   "D,C"     , 0                },
/*52*/      {"LD",   "D,D"     , RIPSTOP          },
/*53*/      {"LD",   "D,E"     , 0                },
/*54*/      {"LD",   "D,H"     , 0                },
/*55*/      {"LD",   "D,L"     , 0                },
/*56*/      {"LD",   "D,(HL)"  , 0                },
/*57*/      {"LD",   "D,A"     , 0                },
/*58*/      {"LD",   "E,B"     , 0                },
/*59*/      {"LD",   "E,C"     , 0                },
/*5A*/      {"LD",   "E,D"     , 0                },
/*5B*/      {"LD",   "E,E"     , RIPSTOP          },
/*5C*/      {"LD",   "E,H"     , 0                },
/*5D*/      {"LD",   "E,L"     , 0                },
/*5E*/      {"LD",   "E,(HL)"  , 0                },
/*5F*/      {"LD",   "E,A"     , 0                },

/*60*/      {"LD",   "H,B"     , 0                },
/*61*/      {"LD",   "H,C"     , 0                },
/*62*/      {"LD",   "H,D"     , 0                },
/*63*/      {"LD",   "H,E"     , 0                },
/*64*/      {"LD",   "H,H"     , RIPSTOP          },
/*65*/      {"LD",   "H,L"     , 0                },
/*66*/      {"LD",   "H,(HL)"  , 0                },
/*67*/      {"LD",   "H,A"     , 0                },
/*68*/      {"LD",   "L,B"     , 0                },
/*69*/      {"LD",   "L,C"     , 0                },
/*6A*/      {"LD",   "L,D"     , 0                },
/*6B*/      {"LD",   "L,E"     , 0                },
/*6C*/      {"LD",   "L,H"     , 0                },
/*6D*/      {"LD",   "L,L"     , RIPSTOP          },
/*6E*/      {"LD",   "L,(HL)"  , 0                },
/*6F*/      {"LD",   "L,A"     , 0                },

/*70*/      {"LD",   "(HL),B"  , 0                },
/*71*/      {"LD",   "(HL),C"  , 0                },
/*72*/      {"LD",   "(HL),D"  , 0                },
/*73*/      {"LD",   "(HL),E"  , 0                },
/*74*/      {"LD",   "(HL),H"  , 0                },
/*75*/      {"LD",   "(HL),L"  , 0                },
/*76*/      {"HALT", ""        , 0                },
/*77*/      {"LD",   "(HL),A"  , 0                },
/*78*/      {"LD",   "A,B"     , 0                },
/*79*/      {"LD",   "A,C"     , 0                },
/*7A*/      {"LD",   "A,D"     , 0                },
/*7B*/      {"LD",   "A,E"     , 0                },
/*7C*/      {"LD",   "A,H"     , 0                },
/*7D*/      {"LD",   "A,L"     , 0                },
/*7E*/      {"LD",   "A,(HL)"  , 0                },
/*7F*/      {"LD",   "A,A"     , 0                },

/*80*/      {"ADD",  "A,B"     , 0                },
/*81*/      {"ADD",  "A,C"     , 0                },
/*82*/      {"ADD",  "A,D"     , 0                },
/*83*/      {"ADD",  "A,E"     , 0                },
/*84*/      {"ADD",  "A,H"     , 0                },
/*85*/      {"ADD",  "A,L"     , 0                },
/*86*/      {"ADD",  "A,(HL)"  , 0                },
/*87*/      {"ADD",  "A,A"     , 0                },
/*88*/      {"ADC",  "A,B"     , 0                },
/*89*/      {"ADC",  "A,C"     , 0                },
/*8A*/      {"ADC",  "A,D"     , 0                },
/*8B*/      {"ADC",  "A,E"     , 0                },
/*8C*/      {"ADC",  "A,H"     , 0                },
/*8D*/      {"ADC",  "A,L"     , 0                },
/*8E*/      {"ADC",  "A,(HL)"  , 0                },
/*8F*/      {"ADC",  "A,A"     , 0                },

/*90*/      {"SUB",  "B"       , 0                },
/*91*/      {"SUB",  "C"       , 0                },
/*92*/      {"SUB",  "D"       , 0                },
/*93*/      {"SUB",  "E"       , 0                },
/*94*/      {"SUB",  "H"       , 0                },
/*95*/      {"SUB",  "L"       , 0                },
/*96*/      {"SUB",  "(HL)"    , 0                },
/*97*/      {"SUB",  "A"       , 0                },
/*98*/      {"SBC",  "A,B"     , 0                },
/*99*/      {"SBC",  "A,C"     , 0                },
/*9A*/      {"SBC",  "A,D"     , 0                },
/*9B*/      {"SBC",  "A,E"     , 0                },
/*9C*/      {"SBC",  "A,H"     , 0                },
/*9D*/      {"SBC",  "A,L"     , 0                },
/*9E*/      {"SBC",  "A,(HL)"  , 0                },
/*9F*/      {"SBC",  "A,A"     , 0                },

/*A0*/      {"AND",  "B"       , 0                },
/*A1*/      {"AND",  "C"       , 0                },
/*A2*/      {"AND",  "D"       , 0                },
/*A3*/      {"AND",  "E"       , 0                },
/*A4*/      {"AND",  "H"       , 0                },
/*A5*/      {"AND",  "L"       , 0                },
/*A6*/      {"AND",  "(HL)"    , 0                },
/*A7*/      {"AND",  "A"       , 0                },
/*A8*/      {"XOR",  "B"       , 0                },
/*A9*/      {"XOR",  "C"       , 0                },
/*AA*/      {"XOR",  "D"       , 0                },
/*AB*/      {"XOR",  "E"       , 0                },
/*AC*/      {"XOR",  "H"       , 0                },
/*AD*/      {"XOR",  "L"       , 0                },
/*AE*/      {"XOR",  "(HL)"    , 0                },
/*AF*/      {"XOR",  "A"       , 0                },

/*B0*/      {"OR",   "B"       , 0                },
/*B1*/      {"OR",   "C"       , 0                },
/*B2*/      {"OR",   "D"       , 0                },
/*B3*/      {"OR",   "E"       , 0                },
/*B4*/      {"OR",   "H"       , 0                },
/*B5*/      {"OR",   "L"       , 0                },
/*B6*/      {"OR",   "(HL)"    , 0                },
/*B7*/      {"OR",   "A"       , 0                },
/*B8*/      {"CP",   "B"       , 0                },
/*B9*/      {"CP",   "C"       , 0                },
/*BA*/      {"CP",   "D"       , 0                },
/*BB*/      {"CP",   "E"       , 0                },
/*BC*/      {"CP",   "H"       , 0                },
/*BD*/      {"CP",   "L"       , 0                },
/*BE*/      {"CP",   "(HL)"    , 0                },
/*BF*/      {"CP",   "A"       , 0                },

/*C0*/      {"RET",  "NZ"      , 0                },
/*C1*/      {"POP",  "BC"      , 0                },
/*C2*/      {"JP",   "NZ,w"    , REFFLAG | CODEREF},
/*C3*/      {"JP",   "w"       , LFFLAG | REFFLAG | CODEREF},
/*C4*/      {"CALL", "NZ,w"    , REFFLAG | CODEREF},
/*C5*/      {"PUSH", "BC"      , 0                },
/*C6*/      {"ADD",  "A,b"     , 0                },
/*C7*/      {"RST",  "00Hx"    , LFFLAG           },
/*C8*/      {"RET",  "Z"       , 0                },
/*C9*/      {"RET",  ""        , LFFLAG           },
/*CA*/      {"JP",   "Z,w"     , REFFLAG | CODEREF},
/*CB*/      {"",     ""        , 0                },
/*CC*/      {"CALL", "Z,w"     , REFFLAG | CODEREF},
/*CD*/      {"CALL", "w"       , REFFLAG | CODEREF},
/*CE*/      {"ADC",  "A,b"     , 0                },
/*CF*/      {"RST",  "08Hx"    , 0                },

/*D0*/      {"RET",  "NC"      , 0                },
/*D1*/      {"POP",  "DE"      , 0                },
/*D2*/      {"JP",   "NC,w"    , REFFLAG | CODEREF},
/*D3*/      {"OUT",  "(b),A"   , 0                },
/*D4*/      {"CALL", "NC,w"    , REFFLAG | CODEREF},
/*D5*/      {"PUSH", "DE"      , 0                },
/*D6*/      {"SUB",  "b"       , 0                },
/*D7*/      {"RST",  "10Hx"    , 0                },
/*D8*/      {"RET",  "C"       , 0                },
/*D9*/      {"EXX",  ""        , 0                },
/*DA*/      {"JP",   "C,w"     , REFFLAG | CODEREF},
/*DB*/      {"IN",   "A,(b)"   , 0                },
/*DC*/      {"CALL", "C,w"     , REFFLAG | CODEREF},
/*DD*/      {"",     ""        , 0                },
/*DE*/      {"SBC",  "A,b"     , 0                },
/*DF*/      {"RST",  "18Hx"    , 0                },

/*E0*/      {"RET",  "PO"      , 0                },
/*E1*/      {"POP",  "HL"      , 0                },
/*E2*/      {"JP",   "PO,w"    , REFFLAG | CODEREF},
/*E3*/      {"EX",   "(SP),HL" , 0                },
/*E4*/      {"CALL", "PO,w"    , REFFLAG | CODEREF},
/*E5*/      {"PUSH", "HL"      , 0                },
/*E6*/      {"AND",  "b"       , 0                },
/*E7*/      {"RST",  "20Hx"    , 0                },
/*E8*/      {"RET",  "PE"      , 0                },
/*E9*/      {"JP",   "(HL)"    , LFFLAG           },
/*EA*/      {"JP",   "PE,w"    , REFFLAG | CODEREF},
/*EB*/      {"EX",   "DE,HL"   , 0                },
/*EC*/      {"CALL", "PE,w"    , REFFLAG | CODEREF},
/*ED*/      {"",     ""        , 0                },
/*EE*/      {"XOR",  "b"       , 0                },
/*EF*/      {"RST",  "28Hx"    , 0                },

/*F0*/      {"RET",  "P"       , 0                },
/*F1*/      {"POP",  "AF"      , 0                },
/*F2*/      {"JP",   "P,w"     , REFFLAG | CODEREF},
/*F3*/      {"DI",   ""        , 0                },
/*F4*/      {"CALL", "P,w"     , REFFLAG | CODEREF},
/*F5*/      {"PUSH", "AF"      , 0                },
/*F6*/      {"OR",   "b"       , 0                },
/*F7*/      {"RST",  "30Hx"    , 0                },
/*F8*/      {"RET",  "M"       , 0                },
/*F9*/      {"LD",   "SP,HL"   , 0                },
/*FA*/      {"JP",   "M,w"     , REFFLAG | CODEREF},
/*FB*/      {"EI",   ""        , 0                },
/*FC*/      {"CALL", "M,w"     , REFFLAG | CODEREF},
/*FD*/      {"",     ""        , 0                },
/*FE*/      {"CP",   "b"       , 0                },
/*FF*/      {"RST",  "38Hx"    , 0                }
};

static const struct InstrRec Z80_opcdTableCB[] =
{           // op     parms            lfref
/*CB00*/    {"RLC",  "B"       , 0                },
/*CB01*/    {"RLC",  "C"       , 0                },
/*CB02*/    {"RLC",  "D"       , 0                },
/*CB03*/    {"RLC",  "E"       , 0                },
/*CB04*/    {"RLC",  "H"       , 0                },
/*CB05*/    {"RLC",  "L"       , 0                },
/*CB06*/    {"RLC",  "(HL)"    , 0                },
/*CB07*/    {"RLC",  "A"       , 0                },
/*CB08*/    {"RRC",  "B"       , 0                },
/*CB09*/    {"RRC",  "C"       , 0                },
/*CB0A*/    {"RRC",  "D"       , 0                },
/*CB0B*/    {"RRC",  "E"       , 0                },
/*CB0C*/    {"RRC",  "H"       , 0                },
/*CB0D*/    {"RRC",  "L"       , 0                },
/*CB0E*/    {"RRC",  "(HL)"    , 0                },
/*CB0F*/    {"RRC",  "A"       , 0                },

/*CB10*/    {"RL",   "B"       , 0                },
/*CB11*/    {"RL",   "C"       , 0                },
/*CB12*/    {"RL",   "D"       , 0                },
/*CB13*/    {"RL",   "E"       , 0                },
/*CB14*/    {"RL",   "H"       , 0                },
/*CB15*/    {"RL",   "L"       , 0                },
/*CB16*/    {"RL",   "(HL)"    , 0                },
/*CB17*/    {"RL",   "A"       , 0                },
/*CB18*/    {"RR",   "B"       , 0                },
/*CB19*/    {"RR",   "C"       , 0                },
/*CB1A*/    {"RR",   "D"       , 0                },
/*CB1B*/    {"RR",   "E"       , 0                },
/*CB1C*/    {"RR",   "H"       , 0                },
/*CB1D*/    {"RR",   "L"       , 0                },
/*CB1E*/    {"RR",   "(HL)"    , 0                },
/*CB1F*/    {"RR",   "A"       , 0                },

/*CB20*/    {"SLA",  "B"       , 0                },
/*CB21*/    {"SLA",  "C"       , 0                },
/*CB22*/    {"SLA",  "D"       , 0                },
/*CB23*/    {"SLA",  "E"       , 0                },
/*CB24*/    {"SLA",  "H"       , 0                },
/*CB25*/    {"SLA",  "L"       , 0                },
/*CB26*/    {"SLA",  "(HL)"    , 0                },
/*CB27*/    {"SLA",  "A"       , 0                },
/*CB28*/    {"SRA",  "B"       , 0                },
/*CB29*/    {"SRA",  "C"       , 0                },
/*CB2A*/    {"SRA",  "D"       , 0                },
/*CB2B*/    {"SRA",  "E"       , 0                },
/*CB2C*/    {"SRA",  "H"       , 0                },
/*CB2D*/    {"SRA",  "L"       , 0                },
/*CB2E*/    {"SRA",  "(HL)"    , 0                },
/*CB2F*/    {"SRA",  "A"       , 0                },

/*CB30*/    {"",     ""        , 0                },
/*CB31*/    {"",     ""        , 0                },
/*CB32*/    {"",     ""        , 0                },
/*CB33*/    {"",     ""        , 0                },
/*CB34*/    {"",     ""        , 0                },
/*CB35*/    {"",     ""        , 0                },
/*CB36*/    {"",     ""        , 0                },
/*CB37*/    {"",     ""        , 0                },
/*CB38*/    {"SRL",  "B"       , 0                },
/*CB39*/    {"SRL",  "C"       , 0                },
/*CB3A*/    {"SRL",  "D"       , 0                },
/*CB3B*/    {"SRL",  "E"       , 0                },
/*CB3C*/    {"SRL",  "H"       , 0                },
/*CB3D*/    {"SRL",  "L"       , 0                },
/*CB3E*/    {"SRL",  "(HL)"    , 0                },
/*CB3F*/    {"SRL",  "A"       , 0                },

/*CB40*/    {"BIT",  "0,B"     , 0                },
/*CB41*/    {"BIT",  "0,C"     , 0                },
/*CB42*/    {"BIT",  "0,D"     , 0                },
/*CB43*/    {"BIT",  "0,E"     , 0                },
/*CB44*/    {"BIT",  "0,H"     , 0                },
/*CB45*/    {"BIT",  "0,L"     , 0                },
/*CB46*/    {"BIT",  "0,(HL)"  , 0                },
/*CB47*/    {"BIT",  "0,A"     , 0                },
/*CB48*/    {"BIT",  "1,B"     , 0                },
/*CB49*/    {"BIT",  "1,C"     , 0                },
/*CB4A*/    {"BIT",  "1,D"     , 0                },
/*CB4B*/    {"BIT",  "1,E"     , 0                },
/*CB4C*/    {"BIT",  "1,H"     , 0                },
/*CB4D*/    {"BIT",  "1,L"     , 0                },
/*CB4E*/    {"BIT",  "1,(HL)"  , 0                },
/*CB4F*/    {"BIT",  "1,A"     , 0                },

/*CB50*/    {"BIT",  "2,B"     , 0                },
/*CB51*/    {"BIT",  "2,C"     , 0                },
/*CB52*/    {"BIT",  "2,D"     , 0                },
/*CB53*/    {"BIT",  "2,E"     , 0                },
/*CB54*/    {"BIT",  "2,H"     , 0                },
/*CB55*/    {"BIT",  "2,L"     , 0                },
/*CB56*/    {"BIT",  "2,(HL)"  , 0                },
/*CB57*/    {"BIT",  "2,A"     , 0                },
/*CB58*/    {"BIT",  "3,B"     , 0                },
/*CB59*/    {"BIT",  "3,C"     , 0                },
/*CB5A*/    {"BIT",  "3,D"     , 0                },
/*CB5B*/    {"BIT",  "3,E"     , 0                },
/*CB5C*/    {"BIT",  "3,H"     , 0                },
/*CB5D*/    {"BIT",  "3,L"     , 0                },
/*CB5E*/    {"BIT",  "3,(HL)"  , 0                },
/*CB5F*/    {"BIT",  "3,A"     , 0                },

/*CB60*/    {"BIT",  "4,B"     , 0                },
/*CB61*/    {"BIT",  "4,C"     , 0                },
/*CB62*/    {"BIT",  "4,D"     , 0                },
/*CB63*/    {"BIT",  "4,E"     , 0                },
/*CB64*/    {"BIT",  "4,H"     , 0                },
/*CB65*/    {"BIT",  "4,L"     , 0                },
/*CB66*/    {"BIT",  "4,(HL)"  , 0                },
/*CB67*/    {"BIT",  "4,A"     , 0                },
/*CB68*/    {"BIT",  "5,B"     , 0                },
/*CB69*/    {"BIT",  "5,C"     , 0                },
/*CB6A*/    {"BIT",  "5,D"     , 0                },
/*CB6B*/    {"BIT",  "5,E"     , 0                },
/*CB6C*/    {"BIT",  "5,H"     , 0                },
/*CB6D*/    {"BIT",  "5,L"     , 0                },
/*CB6E*/    {"BIT",  "5,(HL)"  , 0                },
/*CB6F*/    {"BIT",  "5,A"     , 0                },

/*CB70*/    {"BIT",  "6,B"     , 0                },
/*CB71*/    {"BIT",  "6,C"     , 0                },
/*CB72*/    {"BIT",  "6,D"     , 0                },
/*CB73*/    {"BIT",  "6,E"     , 0                },
/*CB74*/    {"BIT",  "6,H"     , 0                },
/*CB75*/    {"BIT",  "6,L"     , 0                },
/*CB76*/    {"BIT",  "6,(HL)"  , 0                },
/*CB77*/    {"BIT",  "6,A"     , 0                },
/*CB78*/    {"BIT",  "7,B"     , 0                },
/*CB79*/    {"BIT",  "7,C"     , 0                },
/*CB7A*/    {"BIT",  "7,D"     , 0                },
/*CB7B*/    {"BIT",  "7,E"     , 0                },
/*CB7C*/    {"BIT",  "7,H"     , 0                },
/*CB7D*/    {"BIT",  "7,L"     , 0                },
/*CB7E*/    {"BIT",  "7,(HL)"  , 0                },
/*CB7F*/    {"BIT",  "7,A"     , 0                },

/*CB80*/    {"RES",  "0,B"     , 0                },
/*CB81*/    {"RES",  "0,C"     , 0                },
/*CB82*/    {"RES",  "0,D"     , 0                },
/*CB83*/    {"RES",  "0,E"     , 0                },
/*CB84*/    {"RES",  "0,H"     , 0                },
/*CB85*/    {"RES",  "0,L"     , 0                },
/*CB86*/    {"RES",  "0,(HL)"  , 0                },
/*CB87*/    {"RES",  "0,A"     , 0                },
/*CB88*/    {"RES",  "1,B"     , 0                },
/*CB89*/    {"RES",  "1,C"     , 0                },
/*CB8A*/    {"RES",  "1,D"     , 0                },
/*CB8B*/    {"RES",  "1,E"     , 0                },
/*CB8C*/    {"RES",  "1,H"     , 0                },
/*CB8D*/    {"RES",  "1,L"     , 0                },
/*CB8E*/    {"RES",  "1,(HL)"  , 0                },
/*CB8F*/    {"RES",  "1,A"     , 0                },

/*CB90*/    {"RES",  "2,B"     , 0                },
/*CB91*/    {"RES",  "2,C"     , 0                },
/*CB92*/    {"RES",  "2,D"     , 0                },
/*CB93*/    {"RES",  "2,E"     , 0                },
/*CB94*/    {"RES",  "2,H"     , 0                },
/*CB95*/    {"RES",  "2,L"     , 0                },
/*CB96*/    {"RES",  "2,(HL)"  , 0                },
/*CB97*/    {"RES",  "2,A"     , 0                },
/*CB98*/    {"RES",  "3,B"     , 0                },
/*CB99*/    {"RES",  "3,C"     , 0                },
/*CB9A*/    {"RES",  "3,D"     , 0                },
/*CB9B*/    {"RES",  "3,E"     , 0                },
/*CB9C*/    {"RES",  "3,H"     , 0                },
/*CB9D*/    {"RES",  "3,L"     , 0                },
/*CB9E*/    {"RES",  "3,(HL)"  , 0                },
/*CB9F*/    {"RES",  "3,A"     , 0                },

/*CBA0*/    {"RES",  "4,B"     , 0                },
/*CBA1*/    {"RES",  "4,C"     , 0                },
/*CBA2*/    {"RES",  "4,D"     , 0                },
/*CBA3*/    {"RES",  "4,E"     , 0                },
/*CBA4*/    {"RES",  "4,H"     , 0                },
/*CBA5*/    {"RES",  "4,L"     , 0                },
/*CBA6*/    {"RES",  "4,(HL)"  , 0                },
/*CBA7*/    {"RES",  "4,A"     , 0                },
/*CBA8*/    {"RES",  "5,B"     , 0                },
/*CBA9*/    {"RES",  "5,C"     , 0                },
/*CBAA*/    {"RES",  "5,D"     , 0                },
/*CBAB*/    {"RES",  "5,E"     , 0                },
/*CBAC*/    {"RES",  "5,H"     , 0                },
/*CBAD*/    {"RES",  "5,L"     , 0                },
/*CBAE*/    {"RES",  "5,(HL)"  , 0                },
/*CBAF*/    {"RES",  "5,A"     , 0                },

/*CBB0*/    {"RES",  "6,B"     , 0                },
/*CBB1*/    {"RES",  "6,C"     , 0                },
/*CBB2*/    {"RES",  "6,D"     , 0                },
/*CBB3*/    {"RES",  "6,E"     , 0                },
/*CBB4*/    {"RES",  "6,H"     , 0                },
/*CBB5*/    {"RES",  "6,L"     , 0                },
/*CBB6*/    {"RES",  "6,(HL)"  , 0                },
/*CBB7*/    {"RES",  "6,A"     , 0                },
/*CBB8*/    {"RES",  "7,B"     , 0                },
/*CBB9*/    {"RES",  "7,C"     , 0                },
/*CBBA*/    {"RES",  "7,D"     , 0                },
/*CBBB*/    {"RES",  "7,E"     , 0                },
/*CBBC*/    {"RES",  "7,H"     , 0                },
/*CBBD*/    {"RES",  "7,L"     , 0                },
/*CBBE*/    {"RES",  "7,(HL)"  , 0                },
/*CBBF*/    {"RES",  "7,A"     , 0                },

/*CBC0*/    {"SET",  "0,B"     , 0                },
/*CBC1*/    {"SET",  "0,C"     , 0                },
/*CBC2*/    {"SET",  "0,D"     , 0                },
/*CBC3*/    {"SET",  "0,E"     , 0                },
/*CBC4*/    {"SET",  "0,H"     , 0                },
/*CBC5*/    {"SET",  "0,L"     , 0                },
/*CBC6*/    {"SET",  "0,(HL)"  , 0                },
/*CBC7*/    {"SET",  "0,A"     , 0                },
/*CBC8*/    {"SET",  "1,B"     , 0                },
/*CBC9*/    {"SET",  "1,C"     , 0                },
/*CBCA*/    {"SET",  "1,D"     , 0                },
/*CBCB*/    {"SET",  "1,E"     , 0                },
/*CBCC*/    {"SET",  "1,H"     , 0                },
/*CBCD*/    {"SET",  "1,L"     , 0                },
/*CBCE*/    {"SET",  "1,(HL)"  , 0                },
/*CBCF*/    {"SET",  "1,A"     , 0                },

/*CBD0*/    {"SET",  "2,B"     , 0                },
/*CBD1*/    {"SET",  "2,C"     , 0                },
/*CBD2*/    {"SET",  "2,D"     , 0                },
/*CBD3*/    {"SET",  "2,E"     , 0                },
/*CBD4*/    {"SET",  "2,H"     , 0                },
/*CBD5*/    {"SET",  "2,L"     , 0                },
/*CBD6*/    {"SET",  "2,(HL)"  , 0                },
/*CBD7*/    {"SET",  "2,A"     , 0                },
/*CBD8*/    {"SET",  "3,B"     , 0                },
/*CBD9*/    {"SET",  "3,C"     , 0                },
/*CBDA*/    {"SET",  "3,D"     , 0                },
/*CBDB*/    {"SET",  "3,E"     , 0                },
/*CBDC*/    {"SET",  "3,H"     , 0                },
/*CBDD*/    {"SET",  "3,L"     , 0                },
/*CBDE*/    {"SET",  "3,(HL)"  , 0                },
/*CBDF*/    {"SET",  "3,A"     , 0                },

/*CBE0*/    {"SET",  "4,B"     , 0                },
/*CBE1*/    {"SET",  "4,C"     , 0                },
/*CBE2*/    {"SET",  "4,D"     , 0                },
/*CBE3*/    {"SET",  "4,E"     , 0                },
/*CBE4*/    {"SET",  "4,H"     , 0                },
/*CBE5*/    {"SET",  "4,L"     , 0                },
/*CBE6*/    {"SET",  "4,(HL)"  , 0                },
/*CBE7*/    {"SET",  "4,A"     , 0                },
/*CBE8*/    {"SET",  "5,B"     , 0                },
/*CBE9*/    {"SET",  "5,C"     , 0                },
/*CBEA*/    {"SET",  "5,D"     , 0                },
/*CBEB*/    {"SET",  "5,E"     , 0                },
/*CBEC*/    {"SET",  "5,H"     , 0                },
/*CBED*/    {"SET",  "5,L"     , 0                },
/*CBEE*/    {"SET",  "5,(HL)"  , 0                },
/*CBEF*/    {"SET",  "5,A"     , 0                },

/*CBF0*/    {"SET",  "6,B"     , 0                },
/*CBF1*/    {"SET",  "6,C"     , 0                },
/*CBF2*/    {"SET",  "6,D"     , 0                },
/*CBF3*/    {"SET",  "6,E"     , 0                },
/*CBF4*/    {"SET",  "6,H"     , 0                },
/*CBF5*/    {"SET",  "6,L"     , 0                },
/*CBF6*/    {"SET",  "6,(HL)"  , 0                },
/*CBF7*/    {"SET",  "6,A"     , 0                },
/*CBF8*/    {"SET",  "7,B"     , 0                },
/*CBF9*/    {"SET",  "7,C"     , 0                },
/*CBFA*/    {"SET",  "7,D"     , 0                },
/*CBFB*/    {"SET",  "7,E"     , 0                },
/*CBFC*/    {"SET",  "7,H"     , 0                },
/*CBFD*/    {"SET",  "7,L"     , 0                },
/*CBFE*/    {"SET",  "7,(HL)"  , 0                },
/*CBFF*/    {"SET",  "7,A"     , 0                }
};

static const struct InstrRec Z80_opcdTableDD[] =
{           // op     parms            lfref
/*DD00*/    {"",     ""        , 0                },
/*DD01*/    {"",     ""        , 0                },
/*DD02*/    {"",     ""        , 0                },
/*DD03*/    {"",     ""        , 0                },
/*DD04*/    {"",     ""        , 0                },
/*DD05*/    {"",     ""        , 0                },
/*DD06*/    {"",     ""        , 0                },
/*DD07*/    {"",     ""        , 0                },
/*DD08*/    {"",     ""        , 0                },
/*DD09*/    {"ADD",  "IX,BC"   , 0                },
/*DD0A*/    {"",     ""        , 0                },
/*DD0B*/    {"",     ""        , 0                },
/*DD0C*/    {"",     ""        , 0                },
/*DD0D*/    {"",     ""        , 0                },
/*DD0E*/    {"",     ""        , 0                },
/*DD0F*/    {"",     ""        , 0                },

/*DD10*/    {"",     ""        , 0                },
/*DD11*/    {"",     ""        , 0                },
/*DD12*/    {"",     ""        , 0                },
/*DD13*/    {"",     ""        , 0                },
/*DD14*/    {"",     ""        , 0                },
/*DD15*/    {"",     ""        , 0                },
/*DD16*/    {"",     ""        , 0                },
/*DD17*/    {"",     ""        , 0                },
/*DD18*/    {"",     ""        , 0                },
/*DD19*/    {"ADD",  "IX,DE"   , 0                },
/*DD1A*/    {"",     ""        , 0                },
/*DD1B*/    {"",     ""        , 0                },
/*DD1C*/    {"",     ""        , 0                },
/*DD1D*/    {"",     ""        , 0                },
/*DD1E*/    {"",     ""        , 0                },
/*DD1F*/    {"",     ""        , 0                },

/*DD20*/    {"",     ""        , 0                },
/*DD21*/    {"LD",   "IX,w"    , 0                },
/*DD22*/    {"LD",   "(w),IX"  , 0                },
/*DD23*/    {"INC",  "IX"      , 0                },
/*DD24*/    {"",     ""        , 0                },
/*DD25*/    {"",     ""        , 0                },
/*DD26*/    {"",     ""        , 0                },
/*DD27*/    {"",     ""        , 0                },
/*DD28*/    {"",     ""        , 0                },
/*DD29*/    {"ADD",  "IX,IX"   , 0                },
/*DD2A*/    {"LD",   "IX,(w)"  , 0                },
/*DD2B*/    {"DEC",  "IX"      , 0                },
/*DD2C*/    {"",     ""        , 0                },
/*DD2D*/    {"",     ""        , 0                },
/*DD2E*/    {"",     ""        , 0                },
/*DD2F*/    {"",     ""        , 0                },

/*DD30*/    {"",     ""        , 0                },
/*DD31*/    {"",     ""        , 0                },
/*DD32*/    {"",     ""        , 0                },
/*DD33*/    {"",     ""        , 0                },
/*DD34*/    {"INC",  "(IX+d)"  , 0                },
/*DD35*/    {"DEC",  "(IX+d)"  , 0                },
/*DD36*/    {"LD",   "(IX+d),b", 0                },
/*DD37*/    {"",     ""        , 0                },
/*DD38*/    {"",     ""        , 0                },
/*DD39*/    {"ADD",  "IX,SP"   , 0                },
/*DD3A*/    {"",     ""        , 0                },
/*DD3B*/    {"",     ""        , 0                },
/*DD3C*/    {"",     ""        , 0                },
/*DD3D*/    {"",     ""        , 0                },
/*DD3E*/    {"",     ""        , 0                },
/*DD3F*/    {"",     ""        , 0                },

/*DD40*/    {"",     ""        , 0                },
/*DD41*/    {"",     ""        , 0                },
/*DD42*/    {"",     ""        , 0                },
/*DD43*/    {"",     ""        , 0                },
/*DD44*/    {"LD",   "B,IXH"   , 0                }, // undocumented
/*DD45*/    {"LD",   "B,IXL"   , 0                }, // undocumented
/*DD46*/    {"LD",   "B,(IX+d)", 0                },
/*DD47*/    {"",     ""        , 0                },
/*DD48*/    {"",     ""        , 0                },
/*DD49*/    {"",     ""        , 0                },
/*DD4A*/    {"",     ""        , 0                },
/*DD4B*/    {"",     ""        , 0                },
/*DD4C*/    {"LD",   "C,IXH"   , 0                }, // undocumented
/*DD4D*/    {"LD",   "C,IXL"   , 0                }, // undocumented
/*DD4E*/    {"LD",   "C,(IX+d)", 0                },
/*DD4F*/    {"",     ""        , 0                },

/*DD50*/    {"",     ""        , 0                },
/*DD51*/    {"",     ""        , 0                },
/*DD52*/    {"",     ""        , 0                },
/*DD53*/    {"",     ""        , 0                },
/*DD54*/    {"LD",   "D,IXH"   , 0                }, // undocumented
/*DD55*/    {"LD",   "D,IXL"   , 0                }, // undocumented
/*DD56*/    {"LD",   "D,(IX+d)", 0                },
/*DD57*/    {"",     ""        , 0                },
/*DD58*/    {"",     ""        , 0                },
/*DD59*/    {"",     ""        , 0                },
/*DD5A*/    {"",     ""        , 0                },
/*DD5B*/    {"",     ""        , 0                },
/*DD5C*/    {"LD",   "E,IXH"   , 0                }, // undocumented
/*DD5D*/    {"LD",   "E,IXL"   , 0                }, // undocumented
/*DD5E*/    {"LD",   "E,(IX+d)", 0                },
/*DD5F*/    {"",     ""        , 0                },

/*DD60*/    {"LD",   "IXH,B"   , 0                }, // undocumented
/*DD61*/    {"LD",   "IXH,C"   , 0                }, // undocumented
/*DD62*/    {"LD",   "IXH,D"   , 0                }, // undocumented
/*DD63*/    {"LD",   "IXH,E"   , 0                }, // undocumented
/*DD64*/    {"LD",   "IXH,IXH" , 0                }, // undocumented
/*DD65*/    {"LD",   "IXL,IXL" , 0                }, // undocumented
/*DD66*/    {"LD",   "H,(IX+d)", 0                },
/*DD67*/    {"LD",   "IXH,A"   , 0                }, // undocumented
/*DD68*/    {"LD",   "IXL,B"   , 0                }, // undocumented
/*DD69*/    {"LD",   "IXL,C"   , 0                }, // undocumented
/*DD6A*/    {"LD",   "IXL,D"   , 0                }, // undocumented
/*DD6B*/    {"LD",   "IXL,E"   , 0                }, // undocumented
/*DD6C*/    {"LD",   "IXL,IXH" , 0                }, // undocumented
/*DD6D*/    {"LD",   "IXL,IXL" , 0                }, // undocumented
/*DD6E*/    {"LD",   "L,(IX+d)", 0                },
/*DD6F*/    {"LD",   "IXL,A"   , 0                }, // undocumented

/*DD70*/    {"LD",   "(IX+d),B", 0                },
/*DD71*/    {"LD",   "(IX+d),C", 0                },
/*DD72*/    {"LD",   "(IX+d),D", 0                },
/*DD73*/    {"LD",   "(IX+d),E", 0                },
/*DD74*/    {"LD",   "(IX+d),H", 0                },
/*DD75*/    {"LD",   "(IX+d),L", 0                },
/*DD76*/    {"",     ""        , 0                },
/*DD77*/    {"LD",   "(IX+d),A", 0                },
/*DD78*/    {"",     ""        , 0                },
/*DD79*/    {"",     ""        , 0                },
/*DD7A*/    {"",     ""        , 0                },
/*DD7B*/    {"",     ""        , 0                },
/*DD7C*/    {"LD",   "A,IXH"   , 0                }, // undocumented
/*DD7D*/    {"LD",   "A,IXL"   , 0                }, // undocumented
/*DD7E*/    {"LD",   "A,(IX+d)", 0                },
/*DD7F*/    {"",     ""        , 0                },

/*DD80*/    {"",     ""        , 0                },
/*DD81*/    {"",     ""        , 0                },
/*DD82*/    {"",     ""        , 0                },
/*DD83*/    {"",     ""        , 0                },
/*DD84*/    {"",     ""        , 0                },
/*DD85*/    {"",     ""        , 0                },
/*DD86*/    {"ADD",  "A,(IX+d)", 0                },
/*DD87*/    {"",     ""        , 0                },
/*DD88*/    {"",     ""        , 0                },
/*DD89*/    {"",     ""        , 0                },
/*DD8A*/    {"",     ""        , 0                },
/*DD8B*/    {"",     ""        , 0                },
/*DD8C*/    {"",     ""        , 0                },
/*DD8D*/    {"",     ""        , 0                },
/*DD8E*/    {"ADC",  "A,(IX+d)", 0                },
/*DD8F*/    {"",     ""        , 0                },

/*DD90*/    {"",     ""        , 0                },
/*DD91*/    {"",     ""        , 0                },
/*DD92*/    {"",     ""        , 0                },
/*DD93*/    {"",     ""        , 0                },
/*DD94*/    {"",     ""        , 0                },
/*DD95*/    {"",     ""        , 0                },
/*DD96*/    {"SUB",  "(IX+d)",   0                },
/*DD97*/    {"",     ""        , 0                },
/*DD98*/    {"",     ""        , 0                },
/*DD99*/    {"",     ""        , 0                },
/*DD9A*/    {"",     ""        , 0                },
/*DD9B*/    {"",     ""        , 0                },
/*DD9C*/    {"",     ""        , 0                },
/*DD9D*/    {"",     ""        , 0                },
/*DD9E*/    {"SBC",  "A,(IX+d)", 0                },
/*DD9F*/    {"",     ""        , 0                },

/*DDA0*/    {"",     ""        , 0                },
/*DDA1*/    {"",     ""        , 0                },
/*DDA2*/    {"",     ""        , 0                },
/*DDA3*/    {"",     ""        , 0                },
/*DDA4*/    {"",     ""        , 0                },
/*DDA5*/    {"",     ""        , 0                },
/*DDA6*/    {"AND",  "(IX+d)"  , 0                },
/*DDA7*/    {"",     ""        , 0                },
/*DDA8*/    {"",     ""        , 0                },
/*DDA9*/    {"",     ""        , 0                },
/*DDAA*/    {"",     ""        , 0                },
/*DDAB*/    {"",     ""        , 0                },
/*DDAC*/    {"",     ""        , 0                },
/*DDAD*/    {"",     ""        , 0                },
/*DDAE*/    {"XOR",  "(IX+d)"  , 0                },
/*DDAF*/    {"",     ""        , 0                },

/*DDB0*/    {"",     ""        , 0                },
/*DDB1*/    {"",     ""        , 0                },
/*DDB2*/    {"",     ""        , 0                },
/*DDB3*/    {"",     ""        , 0                },
/*DDB4*/    {"",     ""        , 0                },
/*DDB5*/    {"",     ""        , 0                },
/*DDB6*/    {"OR",   "(IX+d)"  , 0                },
/*DDB7*/    {"",     ""        , 0                },
/*DDB8*/    {"",     ""        , 0                },
/*DDB9*/    {"",     ""        , 0                },
/*DDBA*/    {"",     ""        , 0                },
/*DDBB*/    {"",     ""        , 0                },
/*DDBC*/    {"",     ""        , 0                },
/*DDBD*/    {"",     ""        , 0                },
/*DDBE*/    {"CP",   "(IX+d)"  , 0                },
/*DDBF*/    {"",     ""        , 0                },

/*DDC0*/    {"",     ""        , 0                },
/*DDC1*/    {"",     ""        , 0                },
/*DDC2*/    {"",     ""        , 0                },
/*DDC3*/    {"",     ""        , 0                },
/*DDC4*/    {"",     ""        , 0                },
/*DDC5*/    {"",     ""        , 0                },
/*DDC6*/    {"",     ""        , 0                },
/*DDC7*/    {"",     ""        , 0                },
/*DDC8*/    {"",     ""        , 0                },
/*DDC9*/    {"",     ""        , 0                },
/*DDCA*/    {"",     ""        , 0                },
/*DDCB*/    {"***",  "(IX+d)"  , 0                },
/*
/ *DDCB06* /    {"RLC",  "(IX+d)"  , 0                },
/ *DDCB0E* /    {"RRC",  "(IX+d)"  , 0                },
/ *DDCB16* /    {"RL",   "(IX+d)"  , 0                },
/ *DDCB1E* /    {"RR",   "(IX+d)"  , 0                },
/ *DDCB26* /    {"SLA",  "(IX+d)"  , 0                },
/ *DDCB2E* /    {"SRA",  "(IX+d)"  , 0                },
/ *DDCB3E* /    {"SRL",  "(IX+d)"  , 0                },
/ *DDCB46* /    {"BIT",  "0,(IX+d)", 0                },
/ *DDCB4E* /    {"BIT",  "1,(IX+d)", 0                },
/ *DDCB56* /    {"BIT",  "2,(IX+d)", 0                },
/ *DDCB5E* /    {"BIT",  "3,(IX+d)", 0                },
/ *DDCB66* /    {"BIT",  "4,(IX+d)", 0                },
/ *DDCB6E* /    {"BIT",  "5,(IX+d)", 0                },
/ *DDCB76* /    {"BIT",  "6,(IX+d)", 0                },
/ *DDCB7E* /    {"BIT",  "7,(IX+d)", 0                },
/ *DDCB86* /    {"RES",  "0,(IX+d)", 0                },
/ *DDCB8E* /    {"RES",  "1,(IX+d)", 0                },
/ *DDCB96* /    {"RES",  "2,(IX+d)", 0                },
/ *DDCB9E* /    {"RES",  "3,(IX+d)", 0                },
/ *DDCBA6* /    {"RES",  "4,(IX+d)", 0                },
/ *DDCBAE* /    {"RES",  "5,(IX+d)", 0                },
/ *DDCBB6* /    {"RES",  "6,(IX+d)", 0                },
/ *DDCBBE* /    {"RES",  "7,(IX+d)", 0                },
/ *DDCBC6* /    {"SET",  "0,(IX+d)", 0                },
/ *DDCBCE* /    {"SET",  "1,(IX+d)", 0                },
/ *DDCBD6* /    {"SET",  "2,(IX+d)", 0                },
/ *DDCBDE* /    {"SET",  "3,(IX+d)", 0                },
/ *DDCBE6* /    {"SET",  "4,(IX+d)", 0                },
/ *DDCBEE* /    {"SET",  "5,(IX+d)", 0                },
/ *DDCBF6* /    {"SET",  "6,(IX+d)", 0                },
/ *DDCBFE* /    {"SET",  "7,(IX+d)", 0                },
*/
/*DDCC*/    {"",     ""        , 0                },
/*DDCD*/    {"",     ""        , 0                },
/*DDCE*/    {"",     ""        , 0                },
/*DDCF*/    {"",     ""        , 0                },

/*DDD0*/    {"",     ""        , 0                },
/*DDD1*/    {"",     ""        , 0                },
/*DDD2*/    {"",     ""        , 0                },
/*DDD3*/    {"",     ""        , 0                },
/*DDD4*/    {"",     ""        , 0                },
/*DDD5*/    {"",     ""        , 0                },
/*DDD6*/    {"",     ""        , 0                },
/*DDD7*/    {"",     ""        , 0                },
/*DDD8*/    {"",     ""        , 0                },
/*DDD9*/    {"",     ""        , 0                },
/*DDDA*/    {"",     ""        , 0                },
/*DDDB*/    {"",     ""        , 0                },
/*DDDC*/    {"",     ""        , 0                },
/*DDDD*/    {"",     ""        , 0                },
/*DDDE*/    {"",     ""        , 0                },
/*DDDF*/    {"",     ""        , 0                },

/*DDE0*/    {"",     ""        , 0                },
/*DDE1*/    {"POP",  "IX"      , 0                },
/*DDE2*/    {"",     ""        , 0                },
/*DDE3*/    {"EX",   "(SP),IX" , 0                },
/*DDE4*/    {"",     ""        , 0                },
/*DDE5*/    {"PUSH", "IX"      , 0                },
/*DDE6*/    {"",     ""        , 0                },
/*DDE7*/    {"",     ""        , 0                },
/*DDE8*/    {"",     ""        , 0                },
/*DDE9*/    {"JP",   "(IX)"    , LFFLAG           },
/*DDEA*/    {"",     ""        , 0                },
/*DDEB*/    {"",     ""        , 0                },
/*DDEC*/    {"",     ""        , 0                },
/*DDED*/    {"",     ""        , 0                },
/*DDEE*/    {"",     ""        , 0                },
/*DDEF*/    {"",     ""        , 0                },

/*DDF0*/    {"",     ""        , 0                },
/*DDF1*/    {"",     ""        , 0                },
/*DDF2*/    {"",     ""        , 0                },
/*DDF3*/    {"",     ""        , 0                },
/*DDF4*/    {"",     ""        , 0                },
/*DDF5*/    {"",     ""        , 0                },
/*DDF6*/    {"",     ""        , 0                },
/*DDF7*/    {"",     ""        , 0                },
/*DDF8*/    {"",     ""        , 0                },
/*DDF9*/    {"LD",   "SP,IX"   , 0                },
/*DDFA*/    {"",     ""        , 0                },
/*DDFB*/    {"",     ""        , 0                },
/*DDFC*/    {"",     ""        , 0                },
/*DDFD*/    {"",     ""        , 0                },
/*DDFE*/    {"",     ""        , 0                },
/*DDFF*/    {"",     ""        , 0                }
};

static const struct InstrRec Z80_opcdTableED[] =
{
            // op     line         lf    ref
/*ED00*/    {"IN0",  "B,(b)"   , 0                }, // Z180
/*ED01*/    {"OUT0", "(b),B"   , 0                }, // Z180
/*ED02*/    {"",     ""        , 0                },
/*ED03*/    {"",     ""        , 0                },
/*ED04*/    {"TST",  "B"       , 0                }, // Z180
/*ED05*/    {"",     ""        , 0                },
/*ED06*/    {"",     ""        , 0                },
/*ED07*/    {"",     ""        , 0                },
/*ED08*/    {"IN0",  "C,(b)"   , 0                }, // Z180
/*ED09*/    {"OUT0", "(b),C"   , 0                }, // Z180
/*ED0A*/    {"",     ""        , 0                },
/*ED0B*/    {"",     ""        , 0                },
/*ED0C*/    {"TST",  "C"       , 0                }, // Z180
/*ED0D*/    {"",     ""        , 0                },
/*ED0E*/    {"",     ""        , 0                },
/*ED0F*/    {"",     ""        , 0                },

/*ED10*/    {"IN0",  "D,(b)"   , 0                }, // Z180
/*ED11*/    {"OUT0", "(b),D"   , 0                }, // Z180
/*ED12*/    {"",     ""        , 0                },
/*ED13*/    {"",     ""        , 0                },
/*ED14*/    {"TST",  "D"       , 0                }, // Z180
/*ED15*/    {"",     ""        , 0                },
/*ED16*/    {"",     ""        , 0                },
/*ED17*/    {"",     ""        , 0                },
/*ED18*/    {"IN0",  "E,(b)"   , 0                }, // Z180
/*ED19*/    {"OUT0", "(b),E"   , 0                }, // Z180
/*ED1A*/    {"",     ""        , 0                },
/*ED1B*/    {"",     ""        , 0                },
/*ED1C*/    {"TST",  "E"       , 0                }, // Z180
/*ED1D*/    {"",     ""        , 0                },
/*ED1E*/    {"",     ""        , 0                },
/*ED1F*/    {"",     ""        , 0                },

/*ED20*/    {"IN0",  "H,(b)"   , 0                }, // Z180
/*ED21*/    {"OUT0", "(b),H"   , 0                }, // Z180
/*ED22*/    {"",     ""        , 0                },
/*ED23*/    {"",     ""        , 0                },
/*ED24*/    {"TST",  "H"       , 0                }, // Z180
/*ED25*/    {"",     ""        , 0                },
/*ED26*/    {"",     ""        , 0                },
/*ED27*/    {"",     ""        , 0                },
/*ED28*/    {"IN0",  "L,(b)"   , 0                }, // Z180
/*ED29*/    {"OUT0", "(b),L"   , 0                }, // Z180
/*ED2A*/    {"",     ""        , 0                },
/*ED2B*/    {"",     ""        , 0                },
/*ED2C*/    {"TST",  "L"       , 0                }, // Z180
/*ED2D*/    {"",     ""        , 0                },
/*ED2E*/    {"",     ""        , 0                },
/*ED2F*/    {"",     ""        , 0                },

/*ED30*/    {"IN0",  "F,(b)"   , 0                }, // Z180
/*ED31*/    {"OUT0", "(b),0"   , 0                }, // Z180
/*ED32*/    {"",     ""        , 0                },
/*ED33*/    {"",     ""        , 0                },
/*ED34*/    {"TST",  "(HL)"    , 0                }, // Z180
/*ED35*/    {"",     ""        , 0                },
/*ED36*/    {"",     ""        , 0                },
/*ED37*/    {"",     ""        , 0                },
/*ED38*/    {"IN0",  "A,(b)"   , 0                }, // Z180
/*ED39*/    {"OUT0", "(b),A"   , 0                }, // Z180
/*ED3A*/    {"",     ""        , 0                },
/*ED3B*/    {"",     ""        , 0                },
/*ED3C*/    {"TST",  "A"       , 0                }, // Z180
/*ED3D*/    {"",     ""        , 0                },
/*ED3E*/    {"",     ""        , 0                },
/*ED3F*/    {"",     ""        , 0                },

/*ED40*/    {"IN",   "B,(C)"   , 0                },
/*ED41*/    {"OUT",  "(C),B"   , 0                },
/*ED42*/    {"SBC",  "HL,BC"   , 0                },
/*ED43*/    {"LD",   "(w),BC"  , 0                },
/*ED44*/    {"NEG",  ""        , 0                },
/*ED45*/    {"RETN", ""        , LFFLAG           },
/*ED46*/    {"IM",   "0"       , 0                },
/*ED47*/    {"LD",   "I,A"     , 0                },
/*ED48*/    {"IN",   "C,(C)"   , 0                },
/*ED49*/    {"OUT",  "(C),C"   , 0                },
/*ED4A*/    {"ADC",  "HL,BC"   , 0                },
/*ED4B*/    {"LD",   "BC,(w)"  , 0                },
/*ED4C*/    {"MLT",  "BC"      , 0                }, // Z180
/*ED4D*/    {"RETI", ""        , LFFLAG           },
/*ED4E*/    {"",     ""        , 0                },
/*ED4F*/    {"LD",   "R,A"     , 0                },

/*ED50*/    {"IN",   "D,(C)"   , 0                },
/*ED51*/    {"OUT",  "(C),D"   , 0                },
/*ED52*/    {"SBC",  "HL,DE"   , 0                },
/*ED53*/    {"LD",   "(w),DE"  , 0                },
/*ED54*/    {"",     ""        , 0                },
/*ED55*/    {"",     ""        , 0                }, // RETN
/*ED56*/    {"IM",   "1"       , 0                },
/*ED57*/    {"LD",   "A,I"     , 0                },
/*ED58*/    {"IN",   "E,(C)"   , 0                },
/*ED59*/    {"OUT",  "(C),E"   , 0                },
/*ED5A*/    {"ADC",  "HL,DE"   , 0                },
/*ED5B*/    {"LD",   "DE,(w)"  , 0                },
/*ED5C*/    {"MLT",  "DE"      , 0                }, // Z180
/*ED5D*/    {"",     ""        , 0                }, // RETI
/*ED5E*/    {"IM",   "2"       , 0                },
/*ED5F*/    {"LD",   "A,R"     , 0                },

/*ED60*/    {"IN",   "H,(C)"   , 0                },
/*ED61*/    {"OUT",  "(C),H"   , 0                },
/*ED62*/    {"SBC",  "HL,HL"   , 0                },
/*ED63*/    {"LD",   "(w),HL"  , 0                }, // undocumented
/*ED64*/    {"TST",  "b"       , 0                }, // Z180
/*ED65*/    {"",     ""        , 0                }, // RETN
/*ED66*/    {"",     ""        , 0                },
/*ED67*/    {"RRD",  ""        , 0                },
/*ED68*/    {"IN",   "L,(C)"   , 0                },
/*ED69*/    {"OUT",  "(C),L"   , 0                },
/*ED6A*/    {"ADC",  "HL,HL"   , 0                },
/*ED6B*/    {"LD",   "HL,(w)"  , 0                }, // undocumented
/*ED6C*/    {"MLT",  "HL"      , 0                }, // Z180
/*ED6D*/    {"",     ""        , 0                }, // RETI
/*ED6E*/    {"",     ""        , 0                },
/*ED6F*/    {"RLD",  ""        , 0                },

/*ED70*/    {"",     ""        , 0                }, // IN  0,(C)
/*ED71*/    {"",     ""        , 0                }, // OUT (C),0
/*ED72*/    {"SBC",  "HL,SP"   , 0                },
/*ED73*/    {"LD",   "(w),SP"  , 0                },
/*ED74*/    {"TSTIO","b"       , 0                },
/*ED75*/    {"",     ""        , 0                }, // RETN
/*ED76*/    {"SLP",     ""        , 0                }, // Z180
/*ED77*/    {"",     ""        , 0                },
/*ED78*/    {"IN",   "A,(C)"   , 0                },
/*ED79*/    {"OUT",  "(C),A"   , 0                },
/*ED7A*/    {"ADC",  "HL,SP"   , 0                },
/*ED7B*/    {"LD",   "SP,(w)"  , 0                },
/*ED7C*/    {"",     ""        , 0                }, // Z180 MLT SP (useless!)
/*ED7D*/    {"",     ""        , 0                }, // RETI
/*ED7E*/    {"",     ""        , 0                },
/*ED7F*/    {"",     ""        , 0                },

/*ED80*/    {"",     ""        , 0                },
/*ED81*/    {"",     ""        , 0                },
/*ED82*/    {"",     ""        , 0                },
/*ED83*/    {"OTIM",     ""        , 0                }, // Z180
/*ED84*/    {"",     ""        , 0                },
/*ED85*/    {"",     ""        , 0                },
/*ED86*/    {"",     ""        , 0                },
/*ED87*/    {"",     ""        , 0                },
/*ED88*/    {"",     ""        , 0                },
/*ED89*/    {"",     ""        , 0                },
/*ED8A*/    {"",     ""        , 0                },
/*ED8B*/    {"OTDM",     ""        , 0                }, // Z180
/*ED8C*/    {"",     ""        , 0                },
/*ED8D*/    {"",     ""        , 0                },
/*ED8E*/    {"",     ""        , 0                },
/*ED8F*/    {"",     ""        , 0                },

/*ED90*/    {"",     ""        , 0                },
/*ED91*/    {"",     ""        , 0                },
/*ED92*/    {"",     ""        , 0                },
/*ED93*/    {"OTIMR",     ""        , 0                }, // Z180
/*ED94*/    {"",     ""        , 0                },
/*ED95*/    {"",     ""        , 0                },
/*ED96*/    {"",     ""        , 0                },
/*ED97*/    {"",     ""        , 0                },
/*ED98*/    {"",     ""        , 0                },
/*ED99*/    {"",     ""        , 0                },
/*ED9A*/    {"",     ""        , 0                },
/*ED9B*/    {"OTDMR",     ""        , 0                }, // Z180
/*ED9C*/    {"",     ""        , 0                },
/*ED9D*/    {"",     ""        , 0                },
/*ED9E*/    {"",     ""        , 0                },
/*ED9F*/    {"",     ""        , 0                },

/*EDA0*/    {"LDI",  ""        , 0                },
/*EDA1*/    {"CPI",  ""        , 0                },
/*EDA2*/    {"INI",  ""        , 0                },
/*EDA3*/    {"OUTI", ""        , 0                },
/*EDA4*/    {"",     ""        , 0                },
/*EDA5*/    {"",     ""        , 0                },
/*EDA6*/    {"",     ""        , 0                },
/*EDA7*/    {"",     ""        , 0                },
/*EDA8*/    {"LDD",  ""        , 0                },
/*EDA9*/    {"CPD",  ""        , 0                },
/*EDAA*/    {"IND",  ""        , 0                },
/*EDAB*/    {"OUTD", ""        , 0                },
/*EDAC*/    {"",     ""        , 0                },
/*EDAD*/    {"",     ""        , 0                },
/*EDAE*/    {"",     ""        , 0                },
/*EDAF*/    {"",     ""        , 0                },

/*EDB0*/    {"LDIR", ""        , 0                },
/*EDB1*/    {"CPIR", ""        , 0                },
/*EDB2*/    {"INIR", ""        , 0                },
/*EDB3*/    {"OTIR", ""        , 0                },
/*EDB4*/    {"",     ""        , 0                },
/*EDB5*/    {"",     ""        , 0                },
/*EDB6*/    {"",     ""        , 0                },
/*EDB7*/    {"",     ""        , 0                },
/*EDB8*/    {"LDDR", ""        , 0                },
/*EDB9*/    {"CPDR", ""        , 0                },
/*EDBA*/    {"INDR", ""        , 0                },
/*EDBB*/    {"OTDR", ""        , 0                },
/*EDBC*/    {"",     ""        , 0                },
/*EDBD*/    {"",     ""        , 0                },
/*EDBE*/    {"",     ""        , 0                },
/*EDBF*/    {"",     ""        , 0                },

/*EDC0*/    {"",     ""        , 0                },
/*EDC1*/    {"",     ""        , 0                },
/*EDC2*/    {"",     ""        , 0                },
/*EDC3*/    {"",     ""        , 0                },
/*EDC4*/    {"",     ""        , 0                },
/*EDC5*/    {"",     ""        , 0                },
/*EDC6*/    {"",     ""        , 0                },
/*EDC7*/    {"",     ""        , 0                },
/*EDC8*/    {"",     ""        , 0                },
/*EDC9*/    {"",     ""        , 0                },
/*EDCA*/    {"",     ""        , 0                },
/*EDCB*/    {"",     ""        , 0                },
/*EDCC*/    {"",     ""        , 0                },
/*EDCD*/    {"",     ""        , 0                },
/*EDCE*/    {"",     ""        , 0                },
/*EDCF*/    {"",     ""        , 0                },

/*EDD0*/    {"",     ""        , 0                },
/*EDD1*/    {"",     ""        , 0                },
/*EDD2*/    {"",     ""        , 0                },
/*EDD3*/    {"",     ""        , 0                },
/*EDD4*/    {"",     ""        , 0                },
/*EDD5*/    {"",     ""        , 0                },
/*EDD6*/    {"",     ""        , 0                },
/*EDD7*/    {"",     ""        , 0                },
/*EDD8*/    {"",     ""        , 0                },
/*EDD9*/    {"",     ""        , 0                },
/*EDDA*/    {"",     ""        , 0                },
/*EDDB*/    {"",     ""        , 0                },
/*EDDC*/    {"",     ""        , 0                },
/*EDDD*/    {"",     ""        , 0                },
/*EDDE*/    {"",     ""        , 0                },
/*EDDF*/    {"",     ""        , 0                },

/*EDE0*/    {"",     ""        , 0                },
/*EDE1*/    {"",     ""        , 0                },
/*EDE2*/    {"",     ""        , 0                },
/*EDE3*/    {"",     ""        , 0                },
/*EDE4*/    {"",     ""        , 0                },
/*EDE5*/    {"",     ""        , 0                },
/*EDE6*/    {"",     ""        , 0                },
/*EDE7*/    {"",     ""        , 0                },
/*EDE8*/    {"",     ""        , 0                },
/*EDE9*/    {"",     ""        , 0                },
/*EDEA*/    {"",     ""        , 0                },
/*EDEB*/    {"",     ""        , 0                },
/*EDEC*/    {"",     ""        , 0                },
/*EDED*/    {"",     ""        , 0                },
/*EDEE*/    {"",     ""        , 0                },
/*EDEF*/    {"",     ""        , 0                },

/*EDF0*/    {"",     ""        , 0                },
/*EDF1*/    {"",     ""        , 0                },
/*EDF2*/    {"",     ""        , 0                },
/*EDF3*/    {"",     ""        , 0                },
/*EDF4*/    {"",     ""        , 0                },
/*EDF5*/    {"",     ""        , 0                },
/*EDF6*/    {"",     ""        , 0                },
/*EDF7*/    {"",     ""        , 0                },
/*EDF8*/    {"",     ""        , 0                },
/*EDF9*/    {"",     ""        , 0                },
/*EDFA*/    {"",     ""        , 0                },
/*EDFB*/    {"",     ""        , 0                },
/*EDFC*/    {"",     ""        , 0                },
/*EDFD*/    {"",     ""        , 0                },
/*EDFE*/    {"",     ""        , 0                },
/*EDFF*/    {"",     ""        , 0                }
};


int DisZ80::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    addr_t      ad;
    addr_t      ra;
    int         i, n;
    bool        iy = false;
    int         d = 0;
    bool        fixCB = false;
    InstrPtr    instr; instr = NULL;
    char        *p, *l;
    char        s[256];
    int         instrType = INSTR_NORMAL;

    // get first byte of instruction
    ad    = addr;
    i     = ReadByte(ad++);
    int len = 1;

    // handle first byte of opcode according to CPU type
    switch (_subtype) {
        case CPU_8080:
            // look up 8085 instruction overrides, replace with illegal instruction
            instr = SearchInstr(Z80_opcdTable_8085, i);
            if (instr) {
                instr = &Z80_badinstr_8080;
            }
            break;

        case CPU_8085:
            // look up 8085 instruction overrides
            instr = SearchInstr(Z80_opcdTable_8085, i);
            break;

        case CPU_Z180:
        default:
        case CPU_Z80:
            // check for pre-byte
            switch (i) {
                 case 0xCB: instrType = INSTR_CB; break;
                 case 0xDD: instrType = INSTR_DD; break;
                 case 0xED: instrType = INSTR_ED; break;
                 case 0xFD: instrType = INSTR_FD; break;
                 default:   break;
            }
            break;

        case CPU_GB:
            if (i == 0xCB) {
                instrType = INSTR_CB;
            } else {
                instr = SearchInstr(Z80_opcdTable_GB, i);
            }
            break;
    }

    if (!instr) {
        instr = &Z80_opcdTable[i];
    }

    // handle opcode according to instruction type
    switch (instrType) {
        case INSTR_CB:
            i = ReadByte(ad++);
            len = 2;

            instr = NULL;
            if (_subtype == CPU_GB) {
                instr = SearchInstr(Z80_opcdTable_GB_CB, i);
            }

            if (!instr) {
                instr = &Z80_opcdTableCB[i];
            }
            break;

        case INSTR_DD:
        case INSTR_FD:
        {
            iy = (i == 0xFD);

            i = ReadByte(ad++);
            len = 2;
            instr = &Z80_opcdTableDD[i];

            const char *p = instr->parms;
            while (*p) {
                if (*p == 'd') {
                    d = ReadByte(ad++);
                    len++;
                }
                p++;
            }

            // handle DDCB/FDCB
            if (i == 0xCB) {
                i = ReadByte(ad++);
                len++;

                instr = &Z80_opcdTableCB[i];

                i = strlen(instr->parms);
                if (i >= 3 && instr->parms[i-4]=='(' && instr->parms[i-3]=='H'
                           && instr->parms[i-2]=='L' && instr->parms[i-1]==')') {
                    fixCB = true;
                } else {
                    instr = &Z80_opcdTableDD[0];  // illegal
                }
            }
            break;
        }

        case INSTR_ED:
            i = ReadByte(ad++);
            len = 2;
            instr = NULL;
            // check for Z180 opcode first
            if (_subtype == CPU_Z180) {
                instr = SearchInstr(Z80_opcdTable_Z180, i);
            }
            // if Z180 opcode not found, use regular Z80 opcode
            if (!instr) {
                instr = &Z80_opcdTableED[i];
            }
            break;
    }

    strcpy(opcode, instr->op);
    strcpy(parms,  instr->parms);
    lfref   = instr->lfref;
    refaddr = 0;

    // fixup for DDED/FDED
    if (fixCB) {
        strcpy(parms+strlen(parms)-4, "(IX+d)");
    }

    // change IX to IY
    if (iy) {
        p = parms;
        while (*p) {
            if (*p == 'X') *p = 'Y';
            p++;
        }
    }

    // handle substitutions
    p = s;
    l = parms;
    while (*l) {
        switch (*l) {
            // === IX/IY indexed offset ===
            case 'd':
                if (d >= 128) {
                    p[-1] = '-';
                    d = 256 - d;
                }

                H2Str(d, p);
                p += strlen(p);
                break;

            // === 8-bit byte ===
            case 'b':
                i = ReadByte(ad++);
                len++;

                H2Str(i, p);
                p += strlen(p);
                break;

            // === 16-bit word which might be an address ===
            case 'w':
                ra = ReadWord(ad);
                ad += 2;
                len += 2;

                RefStr(ra, p, lfref, refaddr);
                p += strlen(p);
                break;

            // === 1-byte relative branch offset ===
            case 'r':
                i = ReadByte(ad++);
                len++;
                if (i >= 128) {
                    i = i - 256;
                }
                // compute offset but preserve bank address
                ra = ((ad + i) & 0xFFFF) | (ad & 0xFF0000);

                RefStr(ra, p, lfref, refaddr);
                p += strlen(p);

                // rip-stop check for suspicious branch offsets
                if (i == 0 || i == -1) {
                    lfref |= RIPSTOP;
                }
                // rip-stop check for DJNZ forward
                if (ReadByte(addr) == 0x10 && i > 0) {
                    lfref |= RIPSTOP;
                }

                break;

            // === RST nn extra bytes ===
            case 'x':
                n = (ReadByte(ad-1) >> 3) & 7;
                for (i=0; i < rom._rst_xtra[n]; i++) {
                    *p++ = ',';
                    H2Str(ReadByte(ad++), p);
                    p += strlen(p);
                    len++;
                }
                break;

            default:
                *p++ = *l;
                break;
        }
        l++;
    }
    *p = 0;

    strcpy(parms, s);

    // rip-stop checks
    if (opcode[0]) {
        // find the previous instruction for this CPU
        addr_t prev = find_prev_instr(addr);
        if (prev) {
            int op = ReadByte(addr);
            // don't allow doubled LD r,r instruction
            if (0x40 <= op && op <= 0x7F) {
                if (ReadByte(prev) == op) {
                    lfref |= RIPSTOP;
                }
            } else
            switch (op) {
                case 0x00: // three NOP in a row
                    if (ReadByte(prev) == op) {
                        prev = find_prev_instr(prev);
                        if (ReadByte(prev) == op) {
                            lfref |= RIPSTOP;
                        }
                    }
                    break;
                case 0xFF: // two RST 38H in a row
                    if (ReadByte(prev) == op) {
                        lfref |= RIPSTOP;
                    }
                    break;
            }
        }
    }

    // invalid instruction handler, including the case where it ran out of bytes
    if (opcode[0]==0 || rom.AddrOutRange(addr)) {
        strcpy(opcode, "???");
        H2Str(ReadByte(addr), parms);
        len      = 0; // illegal instruction
        lfref    = 0;
        refaddr  = 0;
    }

    return len;
}


