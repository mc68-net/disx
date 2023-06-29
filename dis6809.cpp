// dis6809.cpp

static const char versionName[] = "Motorola 6809 disassembler";

// changes based on being in asm vs listing mode
#if 1
 const bool asmMode = true;
#else
 #include "disline.h"
 #define asmMode (!(disline.line_cols & disline.B_HEX))
#endif

#include "discpu.h"

class Dis6809 : public CPU {
public:
    Dis6809(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


Dis6809 cpu_6809("6809", 0, BIG_END, ADDR_16, '*', '$', "FCB", "FDB", "DL");


enum InstType {
    iInherent, iDirect, iImmediate, iLImmediate, iRelative, iLRelative,
    iIndexed, iExtended, iTfrExg, iPshPul
};


Dis6809::Dis6809(const char *name, int subtype, int endian, int addrwid,
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


struct InstrRec {
    const char      *op;    // mnemonic
    enum InstType   typ;    // typ
    uint8_t         lfref;  // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;

static bool using_dpreg = false; // true if DPREG specified
static int dpreg = 0; // direct page register for identifying direct page references

static const struct InstrRec M6809_opcdTable[] =
{
        // op       lf    ref     typ
/*00*/  {"NEG"  , iDirect  , 0                },
/*01*/  {""     , iInherent, 0                }, // NEG
/*02*/  {""     , iInherent, 0                }, // COM
/*03*/  {"COM"  , iDirect  , 0                },
/*04*/  {"LSR"  , iDirect  , 0                },
/*05*/  {""     , iInherent, 0                }, // LSR
/*06*/  {"ROR"  , iDirect  , 0                },
/*07*/  {"ASR"  , iDirect  , 0                },
/*08*/  {"ASL"  , iDirect  , 0                },
/*09*/  {"ROL"  , iDirect  , 0                },
/*0A*/  {"DEC"  , iDirect  , 0                },
/*0B*/  {""     , iInherent, 0                }, // DEC
/*0C*/  {"INC"  , iDirect  , 0                },
/*0D*/  {"TST"  , iDirect  , 0                },
/*0E*/  {"JMP"  , iDirect  , LFFLAG           },
/*0F*/  {"CLR"  , iDirect  , 0                },

/*10*/  {""     , iInherent , 0                },    // PAGE10
/*11*/  {""     , iInherent , 0                },    // PAGE11
/*12*/  {"NOP"  , iInherent , 0                },
/*13*/  {"SYNC" , iInherent , 0                },
/*14*/  {""     , iInherent , 0                },    // TEST
/*15*/  {""     , iInherent , 0                },    // TEST
/*16*/  {"LBRA" , iLRelative, LFFLAG | REFFLAG | CODEREF},
/*17*/  {"LBSR" , iLRelative, REFFLAG | CODEREF},
/*18*/  {""     , iInherent , 0                },    // ???}
/*19*/  {"DAA"  , iInherent , 0                },
/*1A*/  {"ORCC" , iImmediate, 0                },
/*1B*/  {""     , iInherent , 0                },    // ???
/*1C*/  {"ANDCC", iImmediate, 0                },
/*1D*/  {"SEX"  , iInherent , 0                },
/*1E*/  {"EXG"  , iTfrExg   , 0                },
/*1F*/  {"TFR"  , iTfrExg   , 0                },

/*20*/  {"BRA"  , iRelative, LFFLAG | REFFLAG | CODEREF},
/*21*/  {"BRN"  , iRelative, 0                },
/*22*/  {"BHI"  , iRelative, REFFLAG | CODEREF},
/*23*/  {"BLS"  , iRelative, REFFLAG | CODEREF},
/*24*/  {"BCC"  , iRelative, REFFLAG | CODEREF}, // also BHS
/*25*/  {"BCS"  , iRelative, REFFLAG | CODEREF}, // also BLO
/*26*/  {"BNE"  , iRelative, REFFLAG | CODEREF},
/*27*/  {"BEQ"  , iRelative, REFFLAG | CODEREF},
/*28*/  {"BVC"  , iRelative, REFFLAG | CODEREF},
/*29*/  {"BVS"  , iRelative, REFFLAG | CODEREF},
/*2A*/  {"BPL"  , iRelative, REFFLAG | CODEREF},
/*2B*/  {"BMI"  , iRelative, REFFLAG | CODEREF},
/*2C*/  {"BGE"  , iRelative, REFFLAG | CODEREF},
/*2D*/  {"BLT"  , iRelative, REFFLAG | CODEREF},
/*2E*/  {"BGT"  , iRelative, REFFLAG | CODEREF},
/*2F*/  {"BLE"  , iRelative, REFFLAG | CODEREF},

/*30*/  {"LEAX" , iIndexed  , 0                },
/*31*/  {"LEAY" , iIndexed  , 0                },
/*32*/  {"LEAS" , iIndexed  , 0                },
/*33*/  {"LEAU" , iIndexed  , 0                },
/*34*/  {"PSHS" , iPshPul   , 0                },
/*35*/  {"PULS" , iPshPul   , 0                },
/*36*/  {"PSHU" , iPshPul   , 0                },
/*37*/  {"PULU" , iPshPul   , 0                },
/*38*/  {""     , iInherent , 0                }, // RTS
/*39*/  {"RTS"  , iInherent , LFFLAG           },
/*3A*/  {"ABX"  , iInherent , 0                },
/*3B*/  {"RTI"  , iInherent , LFFLAG           },
/*3C*/  {"CWAI" , iImmediate, 0                },
/*3D*/  {"MUL"  , iInherent , 0                },
/*3E*/  {""     , iInherent , 0                }, // SWR
/*3F*/  {"SWI"  , iInherent , 0                },

/*40*/  {"NEGA" , iInherent, 0                },
/*41*/  {""     , iInherent, 0                }, // NEGA
/*42*/  {""     , iInherent, 0                }, // COMA
/*43*/  {"COMA" , iInherent, 0                },
/*44*/  {"LSRA" , iInherent, 0                },
/*45*/  {""     , iInherent, 0                }, // LSRA
/*46*/  {"RORA" , iInherent, 0                },
/*47*/  {"ASRA" , iInherent, 0                },
/*48*/  {"ASLA" , iInherent, 0                },
/*49*/  {"ROLA" , iInherent, 0                },
/*4A*/  {"DECA" , iInherent, 0                },
/*4B*/  {""     , iInherent, 0                }, // DECA
/*4C*/  {"INCA" , iInherent, 0                },
/*4D*/  {"TSTA" , iInherent, 0                },
/*4E*/  {""     , iInherent, 0                }, // JMPA
/*4F*/  {"CLRA" , iInherent, 0                },

/*50*/  {"NEGB" , iInherent, 0                },
/*51*/  {""     , iInherent, 0                }, // NEGB
/*52*/  {""     , iInherent, 0                }, // COMB
/*53*/  {"COMB" , iInherent, 0                },
/*54*/  {"LSRB" , iInherent, 0                },
/*55*/  {""     , iInherent, 0                }, // LSRB
/*56*/  {"RORB" , iInherent, 0                },
/*57*/  {"ASRB" , iInherent, 0                },
/*58*/  {"ASLB" , iInherent, 0                },
/*59*/  {"ROLB" , iInherent, 0                },
/*5A*/  {"DECB" , iInherent, 0                },
/*5B*/  {""     , iInherent, 0                }, // DECB
/*5C*/  {"INCB" , iInherent, 0                },
/*5D*/  {"TSTB" , iInherent, 0                },
/*5E*/  {""     , iInherent, 0                }, // JMPB
/*5F*/  {"CLRB" , iInherent, 0                },

/*60*/  {"NEG"  , iIndexed , 0                },
/*61*/  {""     , iInherent, 0                }, // NEG
/*62*/  {""     , iInherent, 0                }, // COM
/*63*/  {"COM"  , iIndexed , 0                },
/*64*/  {"LSR"  , iIndexed , 0                },
/*65*/  {""     , iInherent, 0                }, // LSR
/*66*/  {"ROR"  , iIndexed , 0                },
/*67*/  {"ASR"  , iIndexed , 0                },
/*68*/  {"ASL"  , iIndexed , 0                },
/*69*/  {"ROL"  , iIndexed , 0                },
/*6A*/  {"DEC"  , iIndexed , 0                },
/*6B*/  {""     , iInherent, 0                }, // DEC
/*6C*/  {"INC"  , iIndexed , 0                },
/*6D*/  {"TST"  , iIndexed , 0                },
/*6E*/  {"JMP"  , iIndexed , LFFLAG           },
/*6F*/  {"CLR"  , iIndexed , 0                },

/*70*/  {"NEG"  , iExtended, 0                },
/*71*/  {""     , iInherent, 0                }, // NEG
/*72*/  {""     , iInherent, 0                }, // COM
/*73*/  {"COM"  , iExtended, 0                },
/*74*/  {"LSR"  , iExtended, 0                },
/*75*/  {""     , iInherent, 0                }, // LSR
/*76*/  {"ROR"  , iExtended, 0                },
/*77*/  {"ASR"  , iExtended, 0                },
/*78*/  {"ASL"  , iExtended, 0                },
/*79*/  {"ROL"  , iExtended, 0                },
/*7A*/  {"DEC"  , iExtended, 0                },
/*7B*/  {""     , iInherent, 0                }, // DEC
/*7C*/  {"INC"  , iExtended, 0                },
/*7D*/  {"TST"  , iExtended, 0                },
/*7E*/  {"JMP"  , iExtended, LFFLAG | REFFLAG | CODEREF},
/*7F*/  {"CLR"  , iExtended, 0                },

/*80*/  {"SUBA" , iImmediate , 0                },
/*81*/  {"CMPA" , iImmediate , 0                },
/*82*/  {"SBCA" , iImmediate , 0                },
/*83*/  {"SUBD" , iLImmediate, 0                },
/*84*/  {"ANDA" , iImmediate , 0                },
/*85*/  {"BITA" , iImmediate , 0                },
/*86*/  {"LDA"  , iImmediate , 0                },
/*87*/  {""     , iInherent  , 0                },    // STA
/*88*/  {"EORA" , iImmediate , 0                },
/*89*/  {"ADCA" , iImmediate , 0                },
/*8A*/  {"ORA"  , iImmediate , 0                },
/*8B*/  {"ADDA" , iImmediate , 0                },
/*8C*/  {"CMPX" , iLImmediate, 0                },
/*8D*/  {"BSR"  , iRelative  , REFFLAG | CODEREF},
/*8E*/  {"LDX"  , iLImmediate, 0                },
/*8F*/  {""     , iInherent  , 0                },    // STX

/*90*/  {"SUBA" , iDirect, 0                },
/*91*/  {"CMPA" , iDirect, 0                },
/*92*/  {"SBCA" , iDirect, 0                },
/*93*/  {"SUBD" , iDirect, 0                },
/*94*/  {"ANDA" , iDirect, 0                },
/*95*/  {"BITA" , iDirect, 0                },
/*96*/  {"LDA"  , iDirect, 0                },
/*97*/  {"STA"  , iDirect, 0                },
/*98*/  {"EORA" , iDirect, 0                },
/*99*/  {"ADCA" , iDirect, 0                },
/*9A*/  {"ORA"  , iDirect, 0                },
/*9B*/  {"ADDA" , iDirect, 0                },
/*9C*/  {"CMPX" , iDirect, 0                },
/*9D*/  {"JSR"  , iDirect, 0                },
/*9E*/  {"LDX"  , iDirect, 0                },
/*9F*/  {"STX"  , iDirect, 0                },

/*A0*/  {"SUBA" , iIndexed, 0                },
/*A1*/  {"CMPA" , iIndexed, 0                },
/*A2*/  {"SBCA" , iIndexed, 0                },
/*A3*/  {"SUBD" , iIndexed, 0                },
/*A4*/  {"ANDA" , iIndexed, 0                },
/*A5*/  {"BITA" , iIndexed, 0                },
/*A6*/  {"LDA"  , iIndexed, 0                },
/*A7*/  {"STA"  , iIndexed, 0                },
/*A8*/  {"EORA" , iIndexed, 0                },
/*A9*/  {"ADCA" , iIndexed, 0                },
/*AA*/  {"ORA"  , iIndexed, 0                },
/*AB*/  {"ADDA" , iIndexed, 0                },
/*AC*/  {"CMPX" , iIndexed, 0                },
/*AD*/  {"JSR"  , iIndexed, 0                },
/*AE*/  {"LDX"  , iIndexed, 0                },
/*AF*/  {"STX"  , iIndexed, 0                },

/*B0*/  {"SUBA" , iExtended, 0                },
/*B1*/  {"CMPA" , iExtended, 0                },
/*B2*/  {"SBCA" , iExtended, 0                },
/*B3*/  {"SUBD" , iExtended, 0                },
/*B4*/  {"ANDA" , iExtended, 0                },
/*B5*/  {"BITA" , iExtended, 0                },
/*B6*/  {"LDA"  , iExtended, 0                },
/*B7*/  {"STA"  , iExtended, 0                },
/*B8*/  {"EORA" , iExtended, 0                },
/*B9*/  {"ADCA" , iExtended, 0                },
/*BA*/  {"ORA"  , iExtended, 0                },
/*BB*/  {"ADDA" , iExtended, 0                },
/*BC*/  {"CMPX" , iExtended, 0                },
/*BD*/  {"JSR"  , iExtended, REFFLAG | CODEREF},
/*BE*/  {"LDX"  , iExtended, 0                },
/*BF*/  {"STX"  , iExtended, 0                },

/*C0*/  {"SUBB" , iImmediate , 0                },
/*C1*/  {"CMPB" , iImmediate , 0                },
/*C2*/  {"SBCB" , iImmediate , 0                },
/*C3*/  {"ADDD" , iLImmediate, 0                },
/*C4*/  {"ANDB" , iImmediate , 0                },
/*C5*/  {"BITB" , iImmediate , 0                },
/*C6*/  {"LDB"  , iImmediate , 0                },
/*C7*/  {""     , iInherent  , 0                },    // STB
/*C8*/  {"EORB" , iImmediate , 0                },
/*C9*/  {"ADCB" , iImmediate , 0                },
/*CA*/  {"ORB"  , iImmediate , 0                },
/*CB*/  {"ADDB" , iImmediate , 0                },
/*CC*/  {"LDD"  , iLImmediate, 0                },
/*CD*/  {""     , iInherent  , 0                },    // STD
/*CE*/  {"LDU"  , iLImmediate, 0                },
/*CF*/  {""     , iInherent  , 0                },    // STU

/*D0*/  {"SUBB" , iDirect, 0                },
/*D1*/  {"CMPB" , iDirect, 0                },
/*D2*/  {"SBCB" , iDirect, 0                },
/*D3*/  {"ADDD" , iDirect, 0                },
/*D4*/  {"ANDB" , iDirect, 0                },
/*D5*/  {"BITB" , iDirect, 0                },
/*D6*/  {"LDB"  , iDirect, 0                },
/*D7*/  {"STB"  , iDirect, 0                },
/*D8*/  {"EORB" , iDirect, 0                },
/*D9*/  {"ADCB" , iDirect, 0                },
/*DA*/  {"ORB"  , iDirect, 0                },
/*DB*/  {"ADDB" , iDirect, 0                },
/*DC*/  {"LDD"  , iDirect, 0                },
/*DD*/  {"STD"  , iDirect, 0                },
/*DE*/  {"LDU"  , iDirect, 0                },
/*DF*/  {"STU"  , iDirect, 0                },

/*E0*/  {"SUBB" , iIndexed, 0                },
/*E1*/  {"CMPB" , iIndexed, 0                },
/*E2*/  {"SBCB" , iIndexed, 0                },
/*E3*/  {"ADDD" , iIndexed, 0                },
/*E4*/  {"ANDB" , iIndexed, 0                },
/*E5*/  {"BITB" , iIndexed, 0                },
/*E6*/  {"LDB"  , iIndexed, 0                },
/*E7*/  {"STB"  , iIndexed, 0                },
/*E8*/  {"EORB" , iIndexed, 0                },
/*E9*/  {"ADCB" , iIndexed, 0                },
/*EA*/  {"ORB"  , iIndexed, 0                },
/*EB*/  {"ADDB" , iIndexed, 0                },
/*EC*/  {"LDD"  , iIndexed, 0                },
/*ED*/  {"STD"  , iIndexed, 0                },
/*EE*/  {"LDU"  , iIndexed, 0                },
/*EF*/  {"STU"  , iIndexed, 0                },

/*F0*/  {"SUBB" , iExtended, 0                },
/*F1*/  {"CMPB" , iExtended, 0                },
/*F2*/  {"SBCB" , iExtended, 0                },
/*F3*/  {"ADDD" , iExtended, 0                },
/*F4*/  {"ANDB" , iExtended, 0                },
/*F5*/  {"BITB" , iExtended, 0                },
/*F6*/  {"LDB"  , iExtended, 0                },
/*F7*/  {"STB"  , iExtended, 0                },
/*F8*/  {"EORB" , iExtended, 0                },
/*F9*/  {"ADCB" , iExtended, 0                },
/*FA*/  {"ORB"  , iExtended, 0                },
/*FB*/  {"ADDB" , iExtended, 0                },
/*FC*/  {"LDD"  , iExtended, 0                },
/*FD*/  {"STD"  , iExtended, 0                },
/*FE*/  {"LDU"  , iExtended, 0                },
/*FF*/  {"STU"  , iExtended, 0                }
};

static const struct InstrRec M6809_opcdTable10[] =
{
            // op       lf    ref     typ
/*1000*/    {""     , iInherent, 0                },
/*1001*/    {""     , iInherent, 0                },
/*1002*/    {""     , iInherent, 0                },
/*1003*/    {""     , iInherent, 0                },
/*1004*/    {""     , iInherent, 0                },
/*1005*/    {""     , iInherent, 0                },
/*1006*/    {""     , iInherent, 0                },
/*1007*/    {""     , iInherent, 0                },
/*1008*/    {""     , iInherent, 0                },
/*1009*/    {""     , iInherent, 0                },
/*100A*/    {""     , iInherent, 0                },
/*100B*/    {""     , iInherent, 0                },
/*100C*/    {""     , iInherent, 0                },
/*100D*/    {""     , iInherent, 0                },
/*100E*/    {""     , iInherent, 0                },
/*100F*/    {""     , iInherent, 0                },

/*1010*/    {""     , iInherent, 0                },
/*1011*/    {""     , iInherent, 0                },
/*1012*/    {""     , iInherent, 0                },
/*1013*/    {""     , iInherent, 0                },
/*1014*/    {""     , iInherent, 0                },
/*1015*/    {""     , iInherent, 0                },
/*1016*/    {""     , iInherent, 0                },
/*1017*/    {""     , iInherent, 0                },
/*1018*/    {""     , iInherent, 0                },
/*1019*/    {""     , iInherent, 0                },
/*101A*/    {""     , iInherent, 0                },
/*101B*/    {""     , iInherent, 0                },
/*101C*/    {""     , iInherent, 0                },
/*101D*/    {""     , iInherent, 0                },
/*101E*/    {""     , iInherent, 0                },
/*101F*/    {""     , iInherent, 0                },

/*1020*/    {"LBRA" , iLRelative, LFFLAG | REFFLAG | CODEREF},
/*1021*/    {"LBRN" , iLRelative, 0                },
/*1022*/    {"LBHI" , iLRelative, REFFLAG | CODEREF},
/*1023*/    {"LBLS" , iLRelative, REFFLAG | CODEREF},
/*1024*/    {"LBCC" , iLRelative, REFFLAG | CODEREF},    // also LBHS
/*1025*/    {"LBCS" , iLRelative, REFFLAG | CODEREF},    // also LBLO
/*1026*/    {"LBNE" , iLRelative, REFFLAG | CODEREF},
/*1027*/    {"LBEQ" , iLRelative, REFFLAG | CODEREF},
/*1028*/    {"LBVC" , iLRelative, REFFLAG | CODEREF},
/*1029*/    {"LBVS" , iLRelative, REFFLAG | CODEREF},
/*102A*/    {"LBPL" , iLRelative, REFFLAG | CODEREF},
/*102B*/    {"LBMI" , iLRelative, REFFLAG | CODEREF},
/*102C*/    {"LBGE" , iLRelative, REFFLAG | CODEREF},
/*102D*/    {"LBLT" , iLRelative, REFFLAG | CODEREF},
/*102E*/    {"LBGT" , iLRelative, REFFLAG | CODEREF},
/*102F*/    {"LBLE" , iLRelative, REFFLAG | CODEREF},

/*1030*/    {""     , iInherent, 0                },
/*1031*/    {""     , iInherent, 0                },
/*1032*/    {""     , iInherent, 0                },
/*1033*/    {""     , iInherent, 0                },
/*1034*/    {""     , iInherent, 0                },
/*1035*/    {""     , iInherent, 0                },
/*1036*/    {""     , iInherent, 0                },
/*1037*/    {""     , iInherent, 0                },
/*1038*/    {""     , iInherent, 0                },
/*1039*/    {""     , iInherent, 0                },
/*103A*/    {""     , iInherent, 0                },
/*103B*/    {""     , iInherent, 0                },
/*103C*/    {""     , iInherent, 0                },
/*103D*/    {""     , iInherent, 0                },
/*103E*/    {""     , iInherent, 0                },
/*103F*/    {"SWI2" , iInherent, 0                },

/*1040*/    {""     , iInherent, 0                },
/*1041*/    {""     , iInherent, 0                },
/*1042*/    {""     , iInherent, 0                },
/*1043*/    {""     , iInherent, 0                },
/*1044*/    {""     , iInherent, 0                },
/*1045*/    {""     , iInherent, 0                },
/*1046*/    {""     , iInherent, 0                },
/*1047*/    {""     , iInherent, 0                },
/*1048*/    {""     , iInherent, 0                },
/*1049*/    {""     , iInherent, 0                },
/*104A*/    {""     , iInherent, 0                },
/*104B*/    {""     , iInherent, 0                },
/*104C*/    {""     , iInherent, 0                },
/*104D*/    {""     , iInherent, 0                },
/*104E*/    {""     , iInherent, 0                },
/*104F*/    {""     , iInherent, 0                },

/*1050*/    {""     , iInherent, 0                },
/*1051*/    {""     , iInherent, 0                },
/*1052*/    {""     , iInherent, 0                },
/*1053*/    {""     , iInherent, 0                },
/*1054*/    {""     , iInherent, 0                },
/*1055*/    {""     , iInherent, 0                },
/*1056*/    {""     , iInherent, 0                },
/*1057*/    {""     , iInherent, 0                },
/*1058*/    {""     , iInherent, 0                },
/*1059*/    {""     , iInherent, 0                },
/*105A*/    {""     , iInherent, 0                },
/*105B*/    {""     , iInherent, 0                },
/*105C*/    {""     , iInherent, 0                },
/*105D*/    {""     , iInherent, 0                },
/*105E*/    {""     , iInherent, 0                },
/*105F*/    {""     , iInherent, 0                },

/*1060*/    {""     , iInherent, 0                },
/*1061*/    {""     , iInherent, 0                },
/*1062*/    {""     , iInherent, 0                },
/*1063*/    {""     , iInherent, 0                },
/*1064*/    {""     , iInherent, 0                },
/*1065*/    {""     , iInherent, 0                },
/*1066*/    {""     , iInherent, 0                },
/*1067*/    {""     , iInherent, 0                },
/*1068*/    {""     , iInherent, 0                },
/*1069*/    {""     , iInherent, 0                },
/*106A*/    {""     , iInherent, 0                },
/*106B*/    {""     , iInherent, 0                },
/*106C*/    {""     , iInherent, 0                },
/*106D*/    {""     , iInherent, 0                },
/*106E*/    {""     , iInherent, 0                },
/*106F*/    {""     , iInherent, 0                },

/*1070*/    {""     , iInherent, 0                },
/*1071*/    {""     , iInherent, 0                },
/*1072*/    {""     , iInherent, 0                },
/*1073*/    {""     , iInherent, 0                },
/*1074*/    {""     , iInherent, 0                },
/*1075*/    {""     , iInherent, 0                },
/*1076*/    {""     , iInherent, 0                },
/*1077*/    {""     , iInherent, 0                },
/*1078*/    {""     , iInherent, 0                },
/*1079*/    {""     , iInherent, 0                },
/*107A*/    {""     , iInherent, 0                },
/*107B*/    {""     , iInherent, 0                },
/*107C*/    {""     , iInherent, 0                },
/*107D*/    {""     , iInherent, 0                },
/*107E*/    {""     , iInherent, 0                },
/*107F*/    {""     , iInherent, 0                },

/*1080*/    {""     , iInherent  , 0                },
/*1081*/    {""     , iInherent  , 0                },
/*1082*/    {""     , iInherent  , 0                },
/*1083*/    {"CMPD" , iLImmediate, 0                },
/*1084*/    {""     , iInherent  , 0                },
/*1085*/    {""     , iInherent  , 0                },
/*1086*/    {""     , iInherent  , 0                },
/*1087*/    {""     , iInherent  , 0                },
/*1088*/    {""     , iInherent  , 0                },
/*1089*/    {""     , iInherent  , 0                },
/*108A*/    {""     , iInherent  , 0                },
/*108B*/    {""     , iInherent  , 0                },
/*108C*/    {"CMPY" , iLImmediate, 0                },
/*108D*/    {""     , iInherent  , 0                },
/*108E*/    {"LDY"  , iLImmediate, 0                },
/*108F*/    {""     , iInherent  , 0                }, // STY

/*1090*/    {""     , iInherent, 0                },
/*1091*/    {""     , iInherent, 0                },
/*1092*/    {""     , iInherent, 0                },
/*1093*/    {"CMPD" , iDirect  , 0                },
/*1094*/    {""     , iInherent, 0                },
/*1095*/    {""     , iInherent, 0                },
/*1096*/    {""     , iInherent, 0                },
/*1097*/    {""     , iInherent, 0                },
/*1098*/    {""     , iInherent, 0                },
/*1099*/    {""     , iInherent, 0                },
/*109A*/    {""     , iInherent, 0                },
/*109B*/    {""     , iInherent, 0                },
/*109C*/    {"CMPY" , iDirect  , 0                },
/*109D*/    {""     , iInherent, 0                },
/*109E*/    {"LDY"  , iDirect  , 0                },
/*109F*/    {"STY"  , iDirect  , 0                },

/*10A0*/    {""     , iInherent, 0                },
/*10A1*/    {""     , iInherent, 0                },
/*10A2*/    {""     , iInherent, 0                },
/*10A3*/    {"CMPD" , iIndexed , 0                },
/*10A4*/    {""     , iInherent, 0                },
/*10A5*/    {""     , iInherent, 0                },
/*10A6*/    {""     , iInherent, 0                },
/*10A7*/    {""     , iInherent, 0                },
/*10A8*/    {""     , iInherent, 0                },
/*10A9*/    {""     , iInherent, 0                },
/*10AA*/    {""     , iInherent, 0                },
/*10AB*/    {""     , iInherent, 0                },
/*10AC*/    {"CMPY" , iIndexed , 0                },
/*10AD*/    {""     , iInherent, 0                },
/*10AE*/    {"LDY"  , iIndexed , 0                },
/*10AF*/    {"STY"  , iIndexed , 0                },

/*10B0*/    {""     , iInherent, 0                },
/*10B1*/    {""     , iInherent, 0                },
/*10B2*/    {""     , iInherent, 0                },
/*10B3*/    {"CMPD" , iExtended, 0                },
/*10B4*/    {""     , iInherent, 0                },
/*10B5*/    {""     , iInherent, 0                },
/*10B6*/    {""     , iInherent, 0                },
/*10B7*/    {""     , iInherent, 0                },
/*10B8*/    {""     , iInherent, 0                },
/*10B9*/    {""     , iInherent, 0                },
/*10BA*/    {""     , iInherent, 0                },
/*10BB*/    {""     , iInherent, 0                },
/*10BC*/    {"CMPY" , iExtended, 0                },
/*10BD*/    {""     , iInherent, 0                },
/*10BE*/    {"LDY"  , iExtended, 0                },
/*10BF*/    {"STY"  , iExtended, 0                },

/*10C0*/    {""     , iInherent  , 0                },
/*10C1*/    {""     , iInherent  , 0                },
/*10C2*/    {""     , iInherent  , 0                },
/*10C3*/    {""     , iInherent  , 0                },
/*10C4*/    {""     , iInherent  , 0                },
/*10C5*/    {""     , iInherent  , 0                },
/*10C6*/    {""     , iInherent  , 0                },
/*10C7*/    {""     , iInherent  , 0                },
/*10C8*/    {""     , iInherent  , 0                },
/*10C9*/    {""     , iInherent  , 0                },
/*10CA*/    {""     , iInherent  , 0                },
/*10CB*/    {""     , iInherent  , 0                },
/*10CC*/    {""     , iInherent  , 0                },
/*10CD*/    {""     , iInherent  , 0                },
/*10CE*/    {"LDS"  , iLImmediate, 0                },
/*10CF*/    {""     , iInherent  , 0                }, // STS

/*10D0*/    {""     , iInherent, 0                },
/*10D1*/    {""     , iInherent, 0                },
/*10D2*/    {""     , iInherent, 0                },
/*10D3*/    {""     , iInherent, 0                },
/*10D4*/    {""     , iInherent, 0                },
/*10D5*/    {""     , iInherent, 0                },
/*10D6*/    {""     , iInherent, 0                },
/*10D7*/    {""     , iInherent, 0                },
/*10D8*/    {""     , iInherent, 0                },
/*10D9*/    {""     , iInherent, 0                },
/*10DA*/    {""     , iInherent, 0                },
/*10DB*/    {""     , iInherent, 0                },
/*10DC*/    {""     , iInherent, 0                },
/*10DD*/    {""     , iInherent, 0                },
/*10DE*/    {"LDS"  , iDirect  , 0                },
/*10DF*/    {"STS"  , iDirect  , 0                },

/*10E0*/    {""     , iInherent, 0                },
/*10E1*/    {""     , iInherent, 0                },
/*10E2*/    {""     , iInherent, 0                },
/*10E3*/    {""     , iInherent, 0                },
/*10E4*/    {""     , iInherent, 0                },
/*10E5*/    {""     , iInherent, 0                },
/*10E6*/    {""     , iInherent, 0                },
/*10E7*/    {""     , iInherent, 0                },
/*10E8*/    {""     , iInherent, 0                },
/*10E9*/    {""     , iInherent, 0                },
/*10EA*/    {""     , iInherent, 0                },
/*10EB*/    {""     , iInherent, 0                },
/*10EC*/    {""     , iInherent, 0                },
/*10ED*/    {""     , iInherent, 0                },
/*10EE*/    {"LDS"  , iIndexed , 0                },
/*10EF*/    {"STS"  , iIndexed , 0                },

/*10F0*/    {""     , iInherent, 0                },
/*10F1*/    {""     , iInherent, 0                },
/*10F2*/    {""     , iInherent, 0                },
/*10F3*/    {""     , iInherent, 0                },
/*10F4*/    {""     , iInherent, 0                },
/*10F5*/    {""     , iInherent, 0                },
/*10F6*/    {""     , iInherent, 0                },
/*10F7*/    {""     , iInherent, 0                },
/*10F8*/    {""     , iInherent, 0                },
/*10F9*/    {""     , iInherent, 0                },
/*10FA*/    {""     , iInherent, 0                },
/*10FB*/    {""     , iInherent, 0                },
/*10FC*/    {""     , iInherent, 0                },
/*10FD*/    {""     , iInherent, 0                },
/*10FE*/    {"LDS"  , iExtended, 0                },
/*10FF*/    {"STS"  , iExtended, 0                }
};

static const struct InstrRec M6809_opcdTable11[] =
{
            // op       lf    ref     typ
/*1100*/    {""     , iInherent, 0                },
/*1101*/    {""     , iInherent, 0                },
/*1102*/    {""     , iInherent, 0                },
/*1103*/    {""     , iInherent, 0                },
/*1104*/    {""     , iInherent, 0                },
/*1105*/    {""     , iInherent, 0                },
/*1106*/    {""     , iInherent, 0                },
/*1107*/    {""     , iInherent, 0                },
/*1108*/    {""     , iInherent, 0                },
/*1109*/    {""     , iInherent, 0                },
/*110A*/    {""     , iInherent, 0                },
/*110B*/    {""     , iInherent, 0                },
/*110C*/    {""     , iInherent, 0                },
/*110D*/    {""     , iInherent, 0                },
/*110E*/    {""     , iInherent, 0                },
/*110F*/    {""     , iInherent, 0                },

/*1110*/    {""     , iInherent, 0                },
/*1111*/    {""     , iInherent, 0                },
/*1112*/    {""     , iInherent, 0                },
/*1113*/    {""     , iInherent, 0                },
/*1114*/    {""     , iInherent, 0                },
/*1115*/    {""     , iInherent, 0                },
/*1116*/    {""     , iInherent, 0                },
/*1117*/    {""     , iInherent, 0                },
/*1118*/    {""     , iInherent, 0                },
/*1119*/    {""     , iInherent, 0                },
/*111A*/    {""     , iInherent, 0                },
/*111B*/    {""     , iInherent, 0                },
/*111C*/    {""     , iInherent, 0                },
/*111D*/    {""     , iInherent, 0                },
/*111E*/    {""     , iInherent, 0                },
/*111F*/    {""     , iInherent, 0                },

/*1120*/    {""     , iInherent, 0                },
/*1121*/    {""     , iInherent, 0                },
/*1122*/    {""     , iInherent, 0                },
/*1123*/    {""     , iInherent, 0                },
/*1124*/    {""     , iInherent, 0                },
/*1125*/    {""     , iInherent, 0                },
/*1126*/    {""     , iInherent, 0                },
/*1127*/    {""     , iInherent, 0                },
/*1128*/    {""     , iInherent, 0                },
/*1129*/    {""     , iInherent, 0                },
/*112A*/    {""     , iInherent, 0                },
/*112B*/    {""     , iInherent, 0                },
/*112C*/    {""     , iInherent, 0                },
/*112D*/    {""     , iInherent, 0                },
/*112E*/    {""     , iInherent, 0                },
/*112F*/    {""     , iInherent, 0                },

/*1130*/    {""     , iInherent, 0                },
/*1131*/    {""     , iInherent, 0                },
/*1132*/    {""     , iInherent, 0                },
/*1133*/    {""     , iInherent, 0                },
/*1134*/    {""     , iInherent, 0                },
/*1135*/    {""     , iInherent, 0                },
/*1136*/    {""     , iInherent, 0                },
/*1137*/    {""     , iInherent, 0                },
/*1138*/    {""     , iInherent, 0                },
/*1139*/    {""     , iInherent, 0                },
/*113A*/    {""     , iInherent, 0                },
/*113B*/    {""     , iInherent, 0                },
/*113C*/    {""     , iInherent, 0                },
/*113D*/    {""     , iInherent, 0                },
/*113E*/    {""     , iInherent, 0                },
/*113F*/    {"SWI3" , iInherent, 0                },

/*1140*/    {""     , iInherent, 0                },
/*1141*/    {""     , iInherent, 0                },
/*1142*/    {""     , iInherent, 0                },
/*1143*/    {""     , iInherent, 0                },
/*1144*/    {""     , iInherent, 0                },
/*1145*/    {""     , iInherent, 0                },
/*1146*/    {""     , iInherent, 0                },
/*1147*/    {""     , iInherent, 0                },
/*1148*/    {""     , iInherent, 0                },
/*1149*/    {""     , iInherent, 0                },
/*114A*/    {""     , iInherent, 0                },
/*114B*/    {""     , iInherent, 0                },
/*114C*/    {""     , iInherent, 0                },
/*114D*/    {""     , iInherent, 0                },
/*114E*/    {""     , iInherent, 0                },
/*114F*/    {""     , iInherent, 0                },

/*1150*/    {""     , iInherent, 0                },
/*1151*/    {""     , iInherent, 0                },
/*1152*/    {""     , iInherent, 0                },
/*1153*/    {""     , iInherent, 0                },
/*1154*/    {""     , iInherent, 0                },
/*1155*/    {""     , iInherent, 0                },
/*1156*/    {""     , iInherent, 0                },
/*1157*/    {""     , iInherent, 0                },
/*1158*/    {""     , iInherent, 0                },
/*1159*/    {""     , iInherent, 0                },
/*115A*/    {""     , iInherent, 0                },
/*115B*/    {""     , iInherent, 0                },
/*115C*/    {""     , iInherent, 0                },
/*115D*/    {""     , iInherent, 0                },
/*115E*/    {""     , iInherent, 0                },
/*115F*/    {""     , iInherent, 0                },

/*1160*/    {""     , iInherent, 0                },
/*1161*/    {""     , iInherent, 0                },
/*1162*/    {""     , iInherent, 0                },
/*1163*/    {""     , iInherent, 0                },
/*1164*/    {""     , iInherent, 0                },
/*1165*/    {""     , iInherent, 0                },
/*1166*/    {""     , iInherent, 0                },
/*1167*/    {""     , iInherent, 0                },
/*1168*/    {""     , iInherent, 0                },
/*1169*/    {""     , iInherent, 0                },
/*116A*/    {""     , iInherent, 0                },
/*116B*/    {""     , iInherent, 0                },
/*116C*/    {""     , iInherent, 0                },
/*116D*/    {""     , iInherent, 0                },
/*116E*/    {""     , iInherent, 0                },
/*116F*/    {""     , iInherent, 0                },

/*1170*/    {""     , iInherent, 0                },
/*1171*/    {""     , iInherent, 0                },
/*1172*/    {""     , iInherent, 0                },
/*1173*/    {""     , iInherent, 0                },
/*1174*/    {""     , iInherent, 0                },
/*1175*/    {""     , iInherent, 0                },
/*1176*/    {""     , iInherent, 0                },
/*1177*/    {""     , iInherent, 0                },
/*1178*/    {""     , iInherent, 0                },
/*1179*/    {""     , iInherent, 0                },
/*117A*/    {""     , iInherent, 0                },
/*117B*/    {""     , iInherent, 0                },
/*117C*/    {""     , iInherent, 0                },
/*117D*/    {""     , iInherent, 0                },
/*117E*/    {""     , iInherent, 0                },
/*117F*/    {""     , iInherent, 0                },

/*1180*/    {""     , iInherent  , 0                },
/*1181*/    {""     , iInherent  , 0                },
/*1182*/    {""     , iInherent  , 0                },
/*1183*/    {"CMPU" , iLImmediate, 0                },
/*1184*/    {""     , iInherent  , 0                },
/*1185*/    {""     , iInherent  , 0                },
/*1186*/    {""     , iInherent  , 0                },
/*1187*/    {""     , iInherent  , 0                },
/*1188*/    {""     , iInherent  , 0                },
/*1189*/    {""     , iInherent  , 0                },
/*118A*/    {""     , iInherent  , 0                },
/*118B*/    {""     , iInherent  , 0                },
/*118C*/    {"CMPS" , iLImmediate, 0                },
/*118D*/    {""     , iInherent  , 0                },
/*118E*/    {""     , iInherent  , 0                },
/*118F*/    {""     , iInherent  , 0                },

/*1190*/    {""     , iInherent, 0                },
/*1191*/    {""     , iInherent, 0                },
/*1192*/    {""     , iInherent, 0                },
/*1193*/    {"CMPU" , iDirect  , 0                },
/*1194*/    {""     , iInherent, 0                },
/*1195*/    {""     , iInherent, 0                },
/*1196*/    {""     , iInherent, 0                },
/*1197*/    {""     , iInherent, 0                },
/*1198*/    {""     , iInherent, 0                },
/*1199*/    {""     , iInherent, 0                },
/*119A*/    {""     , iInherent, 0                },
/*119B*/    {""     , iInherent, 0                },
/*119C*/    {"CMPS" , iDirect  , 0                },
/*119D*/    {""     , iInherent, 0                },
/*119E*/    {""     , iInherent, 0                },
/*119F*/    {""     , iInherent, 0                },

/*11A0*/    {""     , iInherent, 0                },
/*11A1*/    {""     , iInherent, 0                },
/*11A2*/    {""     , iInherent, 0                },
/*11A3*/    {"CMPU" , iIndexed , 0                },
/*11A4*/    {""     , iInherent, 0                },
/*11A5*/    {""     , iInherent, 0                },
/*11A6*/    {""     , iInherent, 0                },
/*11A7*/    {""     , iInherent, 0                },
/*11A8*/    {""     , iInherent, 0                },
/*11A9*/    {""     , iInherent, 0                },
/*11AA*/    {""     , iInherent, 0                },
/*11AB*/    {""     , iInherent, 0                },
/*11AC*/    {"CMPS" , iIndexed , 0                },
/*11AD*/    {""     , iInherent, 0                },
/*11AE*/    {""     , iInherent, 0                },
/*11AF*/    {""     , iInherent, 0                },

/*11B0*/    {""     , iInherent, 0                },
/*11B1*/    {""     , iInherent, 0                },
/*11B2*/    {""     , iInherent, 0                },
/*11B3*/    {"CMPU" , iExtended, 0                },
/*11B4*/    {""     , iInherent, 0                },
/*11B5*/    {""     , iInherent, 0                },
/*11B6*/    {""     , iInherent, 0                },
/*11B7*/    {""     , iInherent, 0                },
/*11B8*/    {""     , iInherent, 0                },
/*11B9*/    {""     , iInherent, 0                },
/*11BA*/    {""     , iInherent, 0                },
/*11BB*/    {""     , iInherent, 0                },
/*11BC*/    {"CMPS" , iExtended, 0                },
/*11BD*/    {""     , iInherent, 0                },
/*11BE*/    {""     , iInherent, 0                },
/*11BF*/    {""     , iInherent, 0                },

/*11C0*/    {""     , iInherent, 0                },
/*11C1*/    {""     , iInherent, 0                },
/*11C2*/    {""     , iInherent, 0                },
/*11C3*/    {""     , iInherent, 0                },
/*11C4*/    {""     , iInherent, 0                },
/*11C5*/    {""     , iInherent, 0                },
/*11C6*/    {""     , iInherent, 0                },
/*11C7*/    {""     , iInherent, 0                },
/*11C8*/    {""     , iInherent, 0                },
/*11C9*/    {""     , iInherent, 0                },
/*11CA*/    {""     , iInherent, 0                },
/*11CB*/    {""     , iInherent, 0                },
/*11CC*/    {""     , iInherent, 0                },
/*11CD*/    {""     , iInherent, 0                },
/*11CE*/    {""     , iInherent, 0                },
/*11CF*/    {""     , iInherent, 0                },

/*11D0*/    {""     , iInherent, 0                },
/*11D1*/    {""     , iInherent, 0                },
/*11D2*/    {""     , iInherent, 0                },
/*11D3*/    {""     , iInherent, 0                },
/*11D4*/    {""     , iInherent, 0                },
/*11D5*/    {""     , iInherent, 0                },
/*11D6*/    {""     , iInherent, 0                },
/*11D7*/    {""     , iInherent, 0                },
/*11D8*/    {""     , iInherent, 0                },
/*11D9*/    {""     , iInherent, 0                },
/*11DA*/    {""     , iInherent, 0                },
/*11DB*/    {""     , iInherent, 0                },
/*11DC*/    {""     , iInherent, 0                },
/*11DD*/    {""     , iInherent, 0                },
/*11DE*/    {""     , iInherent, 0                },
/*11DF*/    {""     , iInherent, 0                },

/*11E0*/    {""     , iInherent, 0                },
/*11E1*/    {""     , iInherent, 0                },
/*11E2*/    {""     , iInherent, 0                },
/*11E3*/    {""     , iInherent, 0                },
/*11E4*/    {""     , iInherent, 0                },
/*11E5*/    {""     , iInherent, 0                },
/*11E6*/    {""     , iInherent, 0                },
/*11E7*/    {""     , iInherent, 0                },
/*11E8*/    {""     , iInherent, 0                },
/*11E9*/    {""     , iInherent, 0                },
/*11EA*/    {""     , iInherent, 0                },
/*11EB*/    {""     , iInherent, 0                },
/*11EC*/    {""     , iInherent, 0                },
/*11ED*/    {""     , iInherent, 0                },
/*11EE*/    {""     , iInherent, 0                },
/*11EF*/    {""     , iInherent, 0                },

/*11F0*/    {""     , iInherent, 0                },
/*11F1*/    {""     , iInherent, 0                },
/*11F2*/    {""     , iInherent, 0                },
/*11F3*/    {""     , iInherent, 0                },
/*11F4*/    {""     , iInherent, 0                },
/*11F5*/    {""     , iInherent, 0                },
/*11F6*/    {""     , iInherent, 0                },
/*11F7*/    {""     , iInherent, 0                },
/*11F8*/    {""     , iInherent, 0                },
/*11F9*/    {""     , iInherent, 0                },
/*11FA*/    {""     , iInherent, 0                },
/*11FB*/    {""     , iInherent, 0                },
/*11FC*/    {""     , iInherent, 0                },
/*11FD*/    {""     , iInherent, 0                },
/*11FE*/    {""     , iInherent, 0                },
/*11FF*/    {""     , iInherent, 0                },
};

static const struct InstrRec M6809_opcdTableX[] =
{
            // op       lf    ref     typ
/*ix00*/    {"0,X"  , iInherent, 0                },
/*ix01*/    {"1,X"  , iInherent, 0                },
/*ix02*/    {"2,X"  , iInherent, 0                },
/*ix03*/    {"3,X"  , iInherent, 0                },
/*ix04*/    {"4,X"  , iInherent, 0                },
/*ix05*/    {"5,X"  , iInherent, 0                },
/*ix06*/    {"6,X"  , iInherent, 0                },
/*ix07*/    {"7,X"  , iInherent, 0                },
/*ix08*/    {"8,X"  , iInherent, 0                },
/*ix09*/    {"9,X"  , iInherent, 0                },
/*ix0A*/    {"10,X" , iInherent, 0                },
/*ix0B*/    {"11,X" , iInherent, 0                },
/*ix0C*/    {"12,X" , iInherent, 0                },
/*ix0D*/    {"13,X" , iInherent, 0                },
/*ix0E*/    {"14,X" , iInherent, 0                },
/*ix0F*/    {"15,X" , iInherent, 0                },

/*ix10*/    {"-16,X", iInherent, 0                },
/*ix11*/    {"-15,X", iInherent, 0                },
/*ix12*/    {"-14,X", iInherent, 0                },
/*ix13*/    {"-13,X", iInherent, 0                },
/*ix14*/    {"-12,X", iInherent, 0                },
/*ix15*/    {"-11,X", iInherent, 0                },
/*ix16*/    {"-10,X", iInherent, 0                },
/*ix17*/    {"-9,X" , iInherent, 0                },
/*ix18*/    {"-8,X" , iInherent, 0                },
/*ix19*/    {"-7,X" , iInherent, 0                },
/*ix1A*/    {"-6,X" , iInherent, 0                },
/*ix1B*/    {"-5,X" , iInherent, 0                },
/*ix1C*/    {"-4,X" , iInherent, 0                },
/*ix1D*/    {"-3,X" , iInherent, 0                },
/*ix1E*/    {"-2,X" , iInherent, 0                },
/*ix1F*/    {"-1,X" , iInherent, 0                },

/*ix20*/    {"0,Y"  , iInherent, 0                },
/*ix21*/    {"1,Y"  , iInherent, 0                },
/*ix22*/    {"2,Y"  , iInherent, 0                },
/*ix23*/    {"3,Y"  , iInherent, 0                },
/*ix24*/    {"4,Y"  , iInherent, 0                },
/*ix25*/    {"5,Y"  , iInherent, 0                },
/*ix26*/    {"6,Y"  , iInherent, 0                },
/*ix27*/    {"7,Y"  , iInherent, 0                },
/*ix28*/    {"8,Y"  , iInherent, 0                },
/*ix29*/    {"9,Y"  , iInherent, 0                },
/*ix2A*/    {"10,Y" , iInherent, 0                },
/*ix2B*/    {"11,Y" , iInherent, 0                },
/*ix2C*/    {"12,Y" , iInherent, 0                },
/*ix2D*/    {"13,Y" , iInherent, 0                },
/*ix2E*/    {"14,Y" , iInherent, 0                },
/*ix2F*/    {"15,Y" , iInherent, 0                },

/*ix30*/    {"-16,Y", iInherent, 0                },
/*ix31*/    {"-15,Y", iInherent, 0                },
/*ix32*/    {"-14,Y", iInherent, 0                },
/*ix33*/    {"-13,Y", iInherent, 0                },
/*ix34*/    {"-12,Y", iInherent, 0                },
/*ix35*/    {"-11,Y", iInherent, 0                },
/*ix36*/    {"-10,Y", iInherent, 0                },
/*ix37*/    {"-9,Y" , iInherent, 0                },
/*ix38*/    {"-8,Y" , iInherent, 0                },
/*ix39*/    {"-7,Y" , iInherent, 0                },
/*ix3A*/    {"-6,Y" , iInherent, 0                },
/*ix3B*/    {"-5,Y" , iInherent, 0                },
/*ix3C*/    {"-4,Y" , iInherent, 0                },
/*ix3D*/    {"-3,Y" , iInherent, 0                },
/*ix3E*/    {"-2,Y" , iInherent, 0                },
/*ix3F*/    {"-1,Y" , iInherent, 0                },

/*ix40*/    {"0,U"  , iInherent, 0                },
/*ix41*/    {"1,U"  , iInherent, 0                },
/*ix42*/    {"2,U"  , iInherent, 0                },
/*ix43*/    {"3,U"  , iInherent, 0                },
/*ix44*/    {"4,U"  , iInherent, 0                },
/*ix45*/    {"5,U"  , iInherent, 0                },
/*ix46*/    {"6,U"  , iInherent, 0                },
/*ix47*/    {"7,U"  , iInherent, 0                },
/*ix48*/    {"8,U"  , iInherent, 0                },
/*ix49*/    {"9,U"  , iInherent, 0                },
/*ix4A*/    {"10,U" , iInherent, 0                },
/*ix4B*/    {"11,U" , iInherent, 0                },
/*ix4C*/    {"12,U" , iInherent, 0                },
/*ix4D*/    {"13,U" , iInherent, 0                },
/*ix4E*/    {"14,U" , iInherent, 0                },
/*ix4F*/    {"15,U" , iInherent, 0                },

/*ix50*/    {"-16,U", iInherent, 0                },
/*ix51*/    {"-15,U", iInherent, 0                },
/*ix52*/    {"-14,U", iInherent, 0                },
/*ix53*/    {"-13,U", iInherent, 0                },
/*ix54*/    {"-12,U", iInherent, 0                },
/*ix55*/    {"-11,U", iInherent, 0                },
/*ix56*/    {"-10,U", iInherent, 0                },
/*ix57*/    {"-9,U" , iInherent, 0                },
/*ix58*/    {"-8,U" , iInherent, 0                },
/*ix59*/    {"-7,U" , iInherent, 0                },
/*ix5A*/    {"-6,U" , iInherent, 0                },
/*ix5B*/    {"-5,U" , iInherent, 0                },
/*ix5C*/    {"-4,U" , iInherent, 0                },
/*ix5D*/    {"-3,U" , iInherent, 0                },
/*ix5E*/    {"-2,U" , iInherent, 0                },
/*ix5F*/    {"-1,U" , iInherent, 0                },

/*ix60*/    {"0,S"  , iInherent, 0                },
/*ix61*/    {"1,S"  , iInherent, 0                },
/*ix62*/    {"2,S"  , iInherent, 0                },
/*ix63*/    {"3,S"  , iInherent, 0                },
/*ix64*/    {"4,S"  , iInherent, 0                },
/*ix65*/    {"5,S"  , iInherent, 0                },
/*ix66*/    {"6,S"  , iInherent, 0                },
/*ix67*/    {"7,S"  , iInherent, 0                },
/*ix68*/    {"8,S"  , iInherent, 0                },
/*ix69*/    {"9,S"  , iInherent, 0                },
/*ix6A*/    {"10,S" , iInherent, 0                },
/*ix6B*/    {"11,S" , iInherent, 0                },
/*ix6C*/    {"12,S" , iInherent, 0                },
/*ix6D*/    {"13,S" , iInherent, 0                },
/*ix6E*/    {"14,S" , iInherent, 0                },
/*ix6F*/    {"15,S" , iInherent, 0                },

/*ix70*/    {"-16,S", iInherent, 0                },
/*ix71*/    {"-15,S", iInherent, 0                },
/*ix72*/    {"-14,S", iInherent, 0                },
/*ix73*/    {"-13,S", iInherent, 0                },
/*ix74*/    {"-12,S", iInherent, 0                },
/*ix75*/    {"-11,S", iInherent, 0                },
/*ix76*/    {"-10,S", iInherent, 0                },
/*ix77*/    {"-9,S" , iInherent, 0                },
/*ix78*/    {"-8,S" , iInherent, 0                },
/*ix79*/    {"-7,S" , iInherent, 0                },
/*ix7A*/    {"-6,S" , iInherent, 0                },
/*ix7B*/    {"-5,S" , iInherent, 0                },
/*ix7C*/    {"-4,S" , iInherent, 0                },
/*ix7D*/    {"-3,S" , iInherent, 0                },
/*ix7E*/    {"-2,S" , iInherent, 0                },
/*ix7F*/    {"-1,S" , iInherent, 0                },

/*ix80*/    {",X+"  , iInherent , 0                },
/*ix81*/    {",X++" , iInherent , 0                },
/*ix82*/    {",-X"  , iInherent , 0                },
/*ix83*/    {",--X" , iInherent , 0                },
/*ix84*/    {",X"   , iInherent , 0                },
/*ix85*/    {"B,X"  , iInherent , 0                },
/*ix86*/    {"A,X"  , iInherent , 0                },
/*ix87*/    {""     , iInherent , 0                },
/*ix88*/    {",X"   , iImmediate, 0                },    // $xx,X
/*ix89*/    {",X"   , iExtended , 0                },     // $xxxx,X
/*ix8A*/    {""     , iInherent , 0                },
/*ix8B*/    {"D,X"  , iInherent , 0                },
/*ix8C*/    {",PCR" , iRelative , 0                },     // $xx,PC
/*ix8D*/    {",PCR" , iLRelative, 0                },    // $xxxx,PC
/*ix8E*/    {""     , iInherent , 0                },
/*ix8F*/    {""     , iExtended , 0                },     // $xxxx

/*ix90*/    {""     , iInherent , 0                },     // [,X+]
/*ix91*/    {",X++" , iInherent , 0                },
/*ix92*/    {""     , iInherent , 0                },     // [,-X]
/*ix93*/    {",--X" , iInherent , 0                },
/*ix94*/    {",X"   , iInherent , 0                },
/*ix95*/    {"B,X"  , iInherent , 0                },
/*ix96*/    {"A,X"  , iInherent , 0                },
/*ix97*/    {""     , iInherent , 0                },
/*ix98*/    {",X"   , iImmediate, 0                },    // [$xx,X]
/*ix99*/    {",X"   , iExtended , 0                },     // [$xxxx,X]
/*ix9A*/    {""     , iInherent , 0                },
/*ix9B*/    {"D,X"  , iInherent , 0                },
/*ix9C*/    {",PCR" , iRelative , 0                },     // [$xx,PC]
/*ix9D*/    {",PCR" , iLRelative, 0                },    // [$xxxx,PC]
/*ix9E*/    {""     , iInherent , 0                },
/*ix9F*/    {""     , iExtended , 0                },     // [$xxxx]

/*ixA0*/    {",Y+"  , iInherent , 0                },
/*ixA1*/    {",Y++" , iInherent , 0                },
/*ixA2*/    {",-Y"  , iInherent , 0                },
/*ixA3*/    {",--Y" , iInherent , 0                },
/*ixA4*/    {",Y"   , iInherent , 0                },
/*ixA5*/    {"B,Y"  , iInherent , 0                },
/*ixA6*/    {"A,Y"  , iInherent , 0                },
/*ixA7*/    {""     , iInherent , 0                },
/*ixA8*/    {",Y"   , iImmediate, 0                },    // $xx,Y
/*ixA9*/    {",Y"   , iExtended , 0                },     // $xxxx,Y
/*ixAA*/    {""     , iInherent , 0                },
/*ixAB*/    {"D,Y"  , iInherent , 0                },
/*ixAC*/    {",PCR" , iRelative , 0                },     // $xx,PC
/*ixAD*/    {",PCR" , iLRelative, 0                },    // $xxxx,PC
/*ixAE*/    {""     , iInherent , 0                },
/*ixAF*/    {""     , iExtended , 0                },     // $xxxx

/*ixB0*/    {""     , iInherent , 0                },     // [,Y+]
/*ixB1*/    {",Y++" , iInherent , 0                },
/*ixB2*/    {""     , iInherent , 0                },     // [,-Y]
/*ixB3*/    {",--Y" , iInherent , 0                },
/*ixB4*/    {",Y"   , iInherent , 0                },
/*ixB5*/    {"B,Y"  , iInherent , 0                },
/*ixB6*/    {"A,Y"  , iInherent , 0                },
/*ixB7*/    {""     , iInherent , 0                },
/*ixB8*/    {",Y"   , iImmediate, 0                },    // [$xx,Y]
/*ixB9*/    {",Y"   , iExtended , 0                },     // [$xxxx,Y]
/*ixBA*/    {""     , iInherent , 0                },
/*ixBB*/    {"D,Y"  , iInherent , 0                },
/*ixBC*/    {",PCR" , iRelative , 0                },     // [$xx,PC]
/*ixBD*/    {",PCR" , iLRelative, 0                },    // [$xxxx,PC]
/*ixBE*/    {""     , iInherent , 0                },
/*ixBF*/    {""     , iExtended , 0                },     // [$xxxx]

/*ixC0*/    {",U+"  , iInherent , 0                },
/*ixC1*/    {",U++" , iInherent , 0                },
/*ixC2*/    {",-U"  , iInherent , 0                },
/*ixC3*/    {",--U" , iInherent , 0                },
/*ixC4*/    {",U"   , iInherent , 0                },
/*ixC5*/    {"B,U"  , iInherent , 0                },
/*ixC6*/    {"A,U"  , iInherent , 0                },
/*ixC7*/    {""     , iInherent , 0                },
/*ixC8*/    {",U"   , iImmediate, 0                },    // $xx,U
/*ixC9*/    {",U"   , iExtended , 0                },     // $xxxx,U
/*ixCA*/    {""     , iInherent , 0                },
/*ixCB*/    {"D,U"  , iInherent , 0                },
/*ixCC*/    {",PCR" , iRelative , 0                },     // $xx,PC
/*ixCD*/    {",PCR" , iLRelative, 0                },    // $xxxx,PC
/*ixCE*/    {""     , iInherent , 0                },
/*ixCF*/    {""     , iExtended , 0                },     // $xxxx

/*ixD0*/    {""     , iInherent , 0                },     // [,U+]
/*ixD1*/    {",U++" , iInherent , 0                },
/*ixD2*/    {""     , iInherent , 0                },     // [,-U]
/*ixD3*/    {",--U" , iInherent , 0                },
/*ixD4*/    {",U"   , iInherent , 0                },
/*ixD5*/    {"B,U"  , iInherent , 0                },
/*ixD6*/    {"A,U"  , iInherent , 0                },
/*ixD7*/    {""     , iInherent , 0                },
/*ixD8*/    {",U"   , iImmediate, 0                },    // [$xx,U]
/*ixD9*/    {",U"   , iExtended , 0                },     // [$xxxx,U]
/*ixDA*/    {""     , iInherent , 0                },
/*ixDB*/    {"D,U"  , iInherent , 0                },
/*ixDC*/    {",PCR" , iRelative , 0                },     // [$xx,PC]
/*ixDD*/    {",PCR" , iLRelative, 0                },    // [$xxxx,PC]
/*ixDE*/    {""     , iInherent , 0                },
/*ixDF*/    {""     , iExtended , 0                },     // [$xxxx]

/*ixE0*/    {",S+"  , iInherent , 0                },
/*ixE1*/    {",S++" , iInherent , 0                },
/*ixE2*/    {",-S"  , iInherent , 0                },
/*ixE3*/    {",--S" , iInherent , 0                },
/*ixE4*/    {",S"   , iInherent , 0                },
/*ixE5*/    {"B,S"  , iInherent , 0                },
/*ixE6*/    {"A,S"  , iInherent , 0                },
/*ixE7*/    {""     , iInherent , 0                },
/*ixE8*/    {",S"   , iImmediate, 0                },    // $xx,S
/*ixE9*/    {",S"   , iExtended , 0                },     // $xxxx,S
/*ixEA*/    {""     , iInherent , 0                },
/*ixEB*/    {"D,S"  , iInherent , 0                },
/*ixEC*/    {",PCR" , iRelative , 0                },     // $xx,PC
/*ixED*/    {",PCR" , iLRelative, 0                },    // $xxxx,PC
/*ixEE*/    {""     , iInherent , 0                },
/*ixEF*/    {""     , iExtended , 0                },     // $xxxx

/*ixF0*/    {""     , iInherent , 0                },     // [,S+]
/*ixF1*/    {",S++" , iInherent , 0                },
/*ixF2*/    {""     , iInherent , 0                },     // [,-S]
/*ixF3*/    {",--S" , iInherent , 0                },
/*ixF4*/    {",S"   , iInherent , 0                },
/*ixF5*/    {"B,S"  , iInherent , 0                },
/*ixF6*/    {"A,S"  , iInherent , 0                },
/*ixF7*/    {""     , iInherent , 0                },
/*ixF8*/    {",S"   , iImmediate, 0                },    // [$xx,S]
/*ixF9*/    {",S"   , iExtended , 0                },     // [$xxxx,S]
/*ixFA*/    {""     , iInherent , 0                },
/*ixFB*/    {"D,S"  , iInherent , 0                },
/*ixFC*/    {",PCR" , iRelative , 0                },     // [$xx,PC]
/*ixFD*/    {",PCR" , iLRelative, 0                },    // [$xxxx,PC]
/*ixFE*/    {""     , iInherent , 0                },
/*ixFF*/    {""     , iExtended , 0                }      // [$xxxx]
};

static const struct InstrRec tfrReg[] =
{
    {"D" , iLImmediate, 0                },
    {"X" , iLImmediate, 0                },
    {"Y" , iLImmediate, 0                },
    {"U" , iLImmediate, 0                },
    {"S" , iLImmediate, 0                },
    {"PC", iLImmediate, LFFLAG           },
    {""  , iInherent  , 0                },
    {""  , iInherent  , 0                },
    {"A" , iImmediate , 0                },
    {"B" , iImmediate , 0                },
    {"CC", iImmediate , 0                },
    {"DP", iImmediate , 0                },
    {""  , iInherent  , 0                },
    {""  , iInherent  , 0                },
    {""  , iInherent  , 0                },
    {""  , iInherent  , 0                },
};

static const struct InstrRec pshsReg[] =
{
    {"CC", iImmediate , 0                },
    {"A" , iImmediate , 0                },
    {"B" , iImmediate , 0                },
    {"DP", iImmediate , 0                },
    {"X" , iLImmediate, 0                },
    {"Y" , iLImmediate, 0                },
    {"U" , iLImmediate, 0                },
    {"PC", iLImmediate, LFFLAG           },
};


int Dis6809::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    unsigned int    ad;
    int             i;
    InstrPtr        instr;
    char            s[256];
    int             r1, r2;
    addr_t          ra;

    ad    = addr;
    i     = ReadByte(ad++);
    int len = 1;

    switch (i) {
        default:
            instr = &M6809_opcdTable[i];
            break;

        case 0x10:
            i     = ReadByte(ad++);
            len   = 2;
            instr = &M6809_opcdTable10[i];
            break;

        case 0x11:
            i     = ReadByte(ad++);
            len   = 2;
            instr = &M6809_opcdTable11[i];
            break;
    }

    strcpy(opcode, instr->op);
    lfref  = instr->lfref;
    parms[0]  = 0;
    refaddr = 0;

    switch (instr->typ) {
        case iInherent:
            /* nothing */;
            break;

        case iDirect:
            if (using_dpreg) {
                ra = ReadByte(ad++) + dpreg;
                RefStr(ra, s, lfref, refaddr);
                if (asmMode) {
                    sprintf(parms, "<%s", s);
                } else {
                    strcpy(parms, s);
                }
            } else {
                H2Str(ReadByte(ad++), s);
                if (asmMode) {
                    sprintf(parms, "<%s", s);
                } else {
                    sprintf(parms,  "%s", s);
                }
            }
            len++;
            break;

        case iImmediate:
            H2Str(ReadByte(ad++), s);
            sprintf(parms, "#%s", s);
            len++;
            break;

        case iLImmediate:
            ra = ReadWord(ad);
            len += 2;
            i = ReadByte(addr);
            if (i == 0x83 || i == 0xC3 || i == 0xCC ||
                (i == 0x10 && ReadByte(addr+1) == 0x83)) {
                // SUBD, ADD, LDD, CMPD
                H4Str(ra, s);
                sprintf(parms, "#%s", s);
            } else {
                RefStr(ra, s, lfref, refaddr);
                sprintf(parms, "#%s", s);
            }
            break;

        case iRelative:
            i = ReadByte(ad++);
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
            // parms += strlen(parms);
            lfref = i;
            break;

        case iLRelative:
            i = ReadWord(ad);
            ad += 2;
            len += 2;
            ra = (ad + i) & 0xFFFF;
            i = lfref; // ignore flag changes from RefStr
            RefStr(ra, parms, lfref, refaddr);
            // parms += strlen(parms);
            lfref = i;
            break;

        case iIndexed:
            r1 = ReadByte(ad++);
            len++;
            instr = &M6809_opcdTableX[r1];
            lfref |= instr->lfref;
            if (instr->typ == iInherent && instr->op[0] == 0) {
                opcode[0] = 0;  // illegal
                len = 0;
            } else {
                switch (instr->typ) {
                    default:
                    case iInherent:
                        strcpy(parms, instr->op);
                        break;

                    case iImmediate:    // add $xx in front
                        i = ReadByte(ad++);
                        len++;
                        // note that this is a signed offset!
                        if (i >= 128) {
                             strcat(parms, "-");
                             i = 256 - i;
                        }
                        H2Str(i, s);
                        strcat(parms, s);
                        strcat(parms, instr->op);
                        break;

                    case iExtended:     // add $xxxx in front
                        ra = ReadWord(ad);
                        ad += 2;
                        len += 2;
#if 0
                        // this can be signed offset too, sometimes
                        if (ra >= 32768) {
                             strcat(parms, "-");
                             ra = 65536 - ra;
                        }
#endif
                        RefStr(ra, parms, lfref, refaddr);
                        strcat(parms, instr->op);
                        break;

                    case iRelative:     // add PC+$xx
                        i = ReadByte(ad++);
                        len++;
                        if (i >= 128) {
                            i = i - 256;
                        }
                        ra = (ad + i) & 0xFFFF;
                        RefStr(ra, s, lfref, refaddr);
                        sprintf(parms, "<%s%s", s, instr->op);
                        break;

                    case iLRelative:    // add PC+$xxxx
                        i = ReadWord(ad);
                        ad += 2;
                        len += 2;
                        ra = (ad + i) & 0xFFFF;
                        RefStr(ra, s, lfref, refaddr);
                        sprintf(parms, ">%s%s", s, instr->op);
                        break;
                }

                if ((r1 & 0x90) == 0x90) {
                    strcpy(s, parms);
                    sprintf(parms, "[%s]", s);
                }
            }
            break;

        case iExtended:
            ra = ReadWord(ad);
            ad += 2;
            len += 2;
            RefStr(ra, s, lfref, refaddr);
            if (((ra >> 8) & 0xFF) == dpreg) {
                sprintf(parms, ">%s", s);
            } else {
                strcpy(parms, s);
            }
            break;

        case iTfrExg:
            i = ReadByte(ad++);
            len++;
            r1 = (i >> 4) & 15;
            r2 = (i & 15);
            if (tfrReg[r1].typ == iInherent || tfrReg[r2].typ == iInherent ||
                tfrReg[r1].typ != tfrReg[r2].typ) {
                opcode[0] = 0;  // illegal
                len = 0;
            } else {
                sprintf(parms, "%s,%s", tfrReg[r1].op, tfrReg[r2].op);
                lfref |= tfrReg[r2].lfref;  // TFR r,PC
            }
            break;

        case iPshPul:
            r2 = i;
            r1 = ReadByte(ad++);
            len++;

            for (i = 0; i <= 7; i++) {
                if (r1 & (1 << i)) {
                    if (i == 1 && (r1 & 0x06) == 0x06) {
                        strcat(parms, "D");
                        i++;
                    } else {
                        if (i == 6 && (r2 & 0xFE) == 0x36) {
                            strcat(parms, "S");
                        } else {
                            strcat(parms, pshsReg[i].op);
                        }
                    }

                    if (r1 & (0xFF << (i+1))) {
                        strcat(parms, ",");
                    }
                }

                if ((r1 & 0x80) && (r2 == 0x35 || r2 == 0x37)) {
                    lfref |= LFFLAG;  // PULS/PULU PC
                }
            }
            break;

    }

    if (opcode[0] == 0 || rom.AddrOutRange(addr)) {
        strcpy(opcode, "???");
        H2Str(ReadByte(addr), parms);
        len     = 0;
        lfref   = 0;
        refaddr = 0;
    }

    return len;
}


/*
int Dis6809::CustomCommand(char *word)
{
    // note that these values currently would not be saved in the .ctl file
         if (strcmp(word, "DPREG") == 0) { GetWord(word); using_dpreg = true; dpreg = HexVal(word) * 256; }
    else if (strcmp(word, "SETDP") == 0) { GetWord(word); using_dpreg = true; dpreg = HexVal(word) * 256; }
    else return 0;

    return 1;
}
*/


/*
void Dis6809::InitPass(int pass)
{
    char  s[256];

    switch (pass) {
        case 0:
            using_dpreg = false;
            dpreg = 0;
            break;

        case 1:
            break;

        case 2:
            if (using_dpreg) {
                sprintf(s, "$%.2X", dpreg >> 8);
                printline(";", "SETDP", s, "");
            }
            break;
    }
}
*/


