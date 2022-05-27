// dis68HC11.cpp

static const char versionName[] = "Motorola 68HC11 disassembler";

// changes based on being in asm vs listing mode
#if 1
 const bool asmMode = true;
#else
 #include "disline.h"
 #define asmMode (!(disline.line_cols & disline.B_HEX))
#endif

#include "discpu.h"

class Dis68HC11 : public CPU {
public:
    Dis68HC11(const char *name, int subtype, int endian, int addrwid,
              char curAddrChr, char hexChr, const char *byteOp,
              const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


enum {
    // CPU types and type bits for instructions
    CPU_6800   = 1, // 6800, 6802, 6808
    CPU_6801   = 2, // 6801/68701, 6803
    CPU_6303   = 4, // Hitachi 6303, 63701
    CPU_68HC11 = 8, // 68HC11, etc.
    _PAGE      = 16, // page byte for 68HC11

    // CPU types allowed for each instruction
    _6800 = CPU_6800 | CPU_6801 | CPU_6303 | CPU_68HC11,
    _6801 = CPU_6801 | CPU_6303 | CPU_68HC11,
    _6303 = CPU_6303,
    _HC11 = CPU_68HC11,
//  _PAGE = _PAGE,   // 68HC11 page byte
    _P_03 = CPU_6303 | _PAGE, // 68HC11 page / 6303 18 XGDX, 1A SLP
};


Dis68HC11 cpu_6800  ("6800",   CPU_6800,   BIG_END, ADDR_16, '*', '$', "FCB", "FDB", "DL");
Dis68HC11 cpu_6801  ("6801",   CPU_6801,   BIG_END, ADDR_16, '*', '$', "FCB", "FDB", "DL");
Dis68HC11 cpu_6802  ("6802",   CPU_6800,   BIG_END, ADDR_16, '*', '$', "FCB", "FDB", "DL");
Dis68HC11 cpu_6803  ("6803",   CPU_6801,   BIG_END, ADDR_16, '*', '$', "FCB", "FDB", "DL");
Dis68HC11 cpu_6808  ("6808",   CPU_6800,   BIG_END, ADDR_16, '*', '$', "FCB", "FDB", "DL");
Dis68HC11 cpu_6303  ("6303",   CPU_6303,   BIG_END, ADDR_16, '*', '$', "FCB", "FDB", "DL");
Dis68HC11 cpu_68HC11("68HC11", CPU_68HC11, BIG_END, ADDR_16, '*', '$', "FCB", "FDB", "DL");


Dis68HC11::Dis68HC11(const char *name, int subtype, int endian, int addrwid,
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
    iIndexed,    // $ofs,X
    iExtended,   // addr16
    iBDRelative, // $dir,$mask,reladdr
    iBDirect,    // $dir,$mask
    iBXRelative, // $dir,X,$mask,reladdr
    iBYRelative, // $dir,Y,$mask,reladdr
    iBIndexed,   // $dir,X,$mask
    iBIndexedY,  // $dir,Y,$mask
    iIndexedY,   // 68HC11: $ofs,Y
    iMIndexed,   // 6303:   #bit,$ofs,X
    iMExtended   // 6303:   #bit,$dir
};

struct InstrRec {
    const char      *op;    // mnemonic
    enum InstType   typ;    // instruction type
    uint8_t         cpu;    // cpu type flags
    uint8_t         lfref;  // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;


// 6800 instruction sets:
//      6800 6802 6808 8105 = original instruction set
//      6801 6803 68701 68120 = 6800 plus a few instructions
//      6303 = 6801 plus a few instructions
//      HD63701 = 6303 plus undocumented "ADDX 1,S" ($12/$13)
//      68HC11 68HC711 68HC811 68HC99 = 6801 plus Y reg and a lot of instructions

// in 6800/6802/6808 group only, CMPX does not affect C flag
// 6809 is source-code compatible with 6800/6801, with some opcodes translated


static const struct InstrRec M6800_opcdTable[] =
{
        // op        typ        cpu    lfref
/*00*/  {""     , iInherent  , _HC11, 0                }, // TEST - 68HC11
/*01*/  {"NOP"  , iInherent  , _6800, 0                },
/*02*/  {"IDIV" , iInherent  , _HC11, 0                }, // 68HC11
/*03*/  {"FDIV" , iInherent  , _HC11, 0                }, // 68HC11
/*04*/  {"LSRD" , iInherent  , _6801, 0                }, // 68HC11 6303 6801
/*05*/  {"ASLD" , iInherent  , _6800, 0                }, // 68HC11 6303 6801 (also LSLD)
/*06*/  {"TAP"  , iInherent  , _6800, 0                },
/*07*/  {"TPA"  , iInherent  , _6800, 0                },
/*08*/  {"INX"  , iInherent  , _6800, 0                },
/*09*/  {"DEX"  , iInherent  , _6800, 0                },
/*0A*/  {"CLV"  , iInherent  , _6800, 0                },
/*0B*/  {"SEV"  , iInherent  , _6800, 0                },
/*0C*/  {"CLC"  , iInherent  , _6800, 0                },
/*0D*/  {"SEC"  , iInherent  , _6800, 0                },
/*0E*/  {"CLI"  , iInherent  , _6800, 0                },
/*0F*/  {"SEI"  , iInherent  , _6800, 0                },

/*10*/  {"SBA"  , iInherent  , _6800, 0                },
/*11*/  {"CBA"  , iInherent  , _6800, 0                },
/*12*/  {"BRSET", iBDRelative, _HC11, REFFLAG | CODEREF}, // 68HC11 (HD63701 X=X+[S+1])
/*13*/  {"BRCLR", iBDRelative, _HC11, REFFLAG | CODEREF}, // 68HC11 (HD63701 X=X+[S+1])
/*14*/  {"BSET" , iBDirect   , _HC11, 0                }, // 68HC11
/*15*/  {"BCLR" , iBDirect   , _HC11, 0                }, // 68HC11
/*16*/  {"TAB"  , iInherent  , _6800, 0                },
/*17*/  {"TBA"  , iInherent  , _6800, 0                },
/*18*/  {"XGDX" , iInherent  , _P_03, 0                }, // 68HC11 PAGE-18 (6303 XGDX)
/*19*/  {"DAA"  , iInherent  , _6800, 0                },
/*1A*/  {"SLP"  , iInherent  , _P_03, 0                }, // 68HC11 PAGE-1A (6303 SLP)
/*1B*/  {"ABA"  , iInherent  , _6800, 0                },
/*1C*/  {"BSET" , iBIndexed  , _HC11, 0                }, // 68HC11
/*1D*/  {"BCLR" , iBIndexed  , _HC11, 0                }, // 68HC11
/*1E*/  {"BRSET", iBXRelative, _HC11, REFFLAG | CODEREF}, // 68HC11
/*1F*/  {"BRCLR", iBXRelative, _HC11, REFFLAG | CODEREF}, // 68HC11

/*20*/  {"BRA"  , iRelative  , _6800, LFFLAG | REFFLAG | CODEREF},
/*21*/  {"BRN"  , iRelative  , _6801, 0                }, // 68HC11 6303 6801
/*22*/  {"BHI"  , iRelative  , _6800, REFFLAG | CODEREF},
/*23*/  {"BLS"  , iRelative  , _6800, REFFLAG | CODEREF},
/*24*/  {"BCC"  , iRelative  , _6800, REFFLAG | CODEREF}, // also BHS
/*25*/  {"BCS"  , iRelative  , _6800, REFFLAG | CODEREF}, // also BLO
/*26*/  {"BNE"  , iRelative  , _6800, REFFLAG | CODEREF},
/*27*/  {"BEQ"  , iRelative  , _6800, REFFLAG | CODEREF},
/*28*/  {"BVC"  , iRelative  , _6800, REFFLAG | CODEREF},
/*29*/  {"BVS"  , iRelative  , _6800, REFFLAG | CODEREF},
/*2A*/  {"BPL"  , iRelative  , _6800, REFFLAG | CODEREF},
/*2B*/  {"BMI"  , iRelative  , _6800, REFFLAG | CODEREF},
/*2C*/  {"BGE"  , iRelative  , _6800, REFFLAG | CODEREF},
/*2D*/  {"BLT"  , iRelative  , _6800, REFFLAG | CODEREF},
/*2E*/  {"BGT"  , iRelative  , _6800, REFFLAG | CODEREF},
/*2F*/  {"BLE"  , iRelative  , _6800, REFFLAG | CODEREF},

/*30*/  {"TSX"  , iInherent  , _6800, 0                },
/*31*/  {"INS"  , iInherent  , _6800, 0                },
/*32*/  {"PULA" , iInherent  , _6800, 0                },
/*33*/  {"PULB" , iInherent  , _6800, 0                },
/*34*/  {"DES"  , iInherent  , _6800, 0                },
/*35*/  {"TXS"  , iInherent  , _6800, 0                },
/*36*/  {"PSHA" , iInherent  , _6800, 0                },
/*37*/  {"PSHB" , iInherent  , _6800, 0                },
/*38*/  {"PULX" , iInherent  , _6801, 0                }, // 68HC11 6303 6801
/*39*/  {"RTS"  , iInherent  , _6800, LFFLAG           },
/*3A*/  {"ABX"  , iInherent  , _6801, 0                }, // 68HC11 6303 6801
/*3B*/  {"RTI"  , iInherent  , _6800, LFFLAG           },
/*3C*/  {"PSHX" , iInherent  , _6801, 0                }, // 68HC11 6303 6801
/*3D*/  {"MUL"  , iInherent  , _6801, 0                }, // 68HC11 6303 6801
/*3E*/  {"WAI"  , iInherent  , _6800, 0                },
/*3F*/  {"SWI"  , iInherent  , _6800, 0                },

/*40*/  {"NEGA" , iInherent  , _6800, 0                },
/*41*/  {""     , iInherent  ,   0  , 0                },
/*42*/  {""     , iInherent  ,   0  , 0                },
/*43*/  {"COMA" , iInherent  , _6800, 0                },
/*44*/  {"LSRA" , iInherent  , _6800, 0                },
/*45*/  {""     , iInherent  ,   0  , 0                },
/*46*/  {"RORA" , iInherent  , _6800, 0                },
/*47*/  {"ASRA" , iInherent  , _6800, 0                },
/*48*/  {"ASLA" , iInherent  , _6800, 0                },
/*49*/  {"ROLA" , iInherent  , _6800, 0                },
/*4A*/  {"DECA" , iInherent  , _6800, 0                },
/*4B*/  {""     , iInherent  ,   0  , 0                },
/*4C*/  {"INCA" , iInherent  , _6800, 0                },
/*4D*/  {"TSTA" , iInherent  , _6800, 0                },
/*4E*/  {""     , iInherent  ,   0  , 0                },
/*4F*/  {"CLRA" , iInherent  , _6800, 0                },

/*50*/  {"NEGB" , iInherent  , _6800, 0                },
/*51*/  {""     , iInherent  ,   0  , 0                },
/*52*/  {""     , iInherent  ,   0  , 0                },
/*53*/  {"COMB" , iInherent  , _6800, 0                },
/*54*/  {"LSRB" , iInherent  , _6800, 0                },
/*55*/  {""     , iInherent  ,   0  , 0                },
/*56*/  {"RORB" , iInherent  , _6800, 0                },
/*57*/  {"ASRB" , iInherent  , _6800, 0                },
/*58*/  {"ASLB" , iInherent  , _6800, 0                },
/*59*/  {"ROLB" , iInherent  , _6800, 0                },
/*5A*/  {"DECB" , iInherent  , _6800, 0                },
/*5B*/  {""     , iInherent  ,   0  , 0                },
/*5C*/  {"INCB" , iInherent  , _6800, 0                },
/*5D*/  {"TSTB" , iInherent  , _6800, 0                },
/*5E*/  {""     , iInherent  ,   0  , 0                },
/*5F*/  {"CLRB" , iInherent  , _6800, 0                },

/*60*/  {"NEG"  , iIndexed   , _6800, 0                },
/*61*/  {"AIM"  , iInherent  , _6303, 0                }, // (6303 AIM iMIndexed)
/*62*/  {"OIM"  , iInherent  , _6303, 0                }, // (6303 OIM iMIndexed)
/*63*/  {"COM"  , iIndexed   , _6800, 0                },
/*64*/  {"LSR"  , iIndexed   , _6800, 0                },
/*65*/  {"EIM"  , iInherent  , _6303, 0                }, // (6303 EIM iMIndexed)
/*66*/  {"ROR"  , iIndexed   , _6800, 0                },
/*67*/  {"ASR"  , iIndexed   , _6800, 0                },
/*68*/  {"ASL"  , iIndexed   , _6800, 0                },
/*69*/  {"ROL"  , iIndexed   , _6800, 0                },
/*6A*/  {"DEC"  , iIndexed   , _6800, 0                },
/*6B*/  {"TIM"  , iInherent  , _6303, 0                }, // (6303 TIM iMIndexed)
/*6C*/  {"INC"  , iIndexed   , _6800, 0                },
/*6D*/  {"TST"  , iIndexed   , _6800, 0                },
/*6E*/  {"JMP"  , iIndexed   , _6800, LFFLAG           },
/*6F*/  {"CLR"  , iIndexed   , _6800, 0                },

/*70*/  {"NEG"  , iExtended  , _6800, 0                },
/*71*/  {"AIM"  , iInherent  , _6303, 0                }, // (6303 AIM iMExtended)
/*72*/  {"OIM"  , iInherent  , _6303, 0                }, // (6303 OIM iMExtended)
/*73*/  {"COM"  , iExtended  , _6800, 0                },
/*74*/  {"LSR"  , iExtended  , _6800, 0                },
/*75*/  {"EIM"  , iInherent  , _6303, 0                }, // (6303 EIM iMExtended)
/*76*/  {"ROR"  , iExtended  , _6800, 0                },
/*77*/  {"ASR"  , iExtended  , _6800, 0                },
/*78*/  {"ASL"  , iExtended  , _6800, 0                },
/*79*/  {"ROL"  , iExtended  , _6800, 0                },
/*7A*/  {"DEC"  , iExtended  , _6800, 0                },
/*7B*/  {"TIM"  , iInherent  , _6303, 0                }, // (6303 TIM iMExtended)
/*7C*/  {"INC"  , iExtended  , _6800, 0                },
/*7D*/  {"TST"  , iExtended  , _6800, 0                },
/*7E*/  {"JMP"  , iExtended  , _6800, LFFLAG | REFFLAG | CODEREF},
/*7F*/  {"CLR"  , iExtended  , _6800, 0                },

/*80*/  {"SUBA" , iImmediate , _6800, 0                },
/*81*/  {"CMPA" , iImmediate , _6800, 0                },
/*82*/  {"SBCA" , iImmediate , _6800, 0                },
/*83*/  {"SUBD" , iLImmediate, _6801, 0                }, // 68HC11 6303 6801
/*84*/  {"ANDA" , iImmediate , _6800, 0                },
/*85*/  {"BITA" , iImmediate , _6800, 0                },
/*86*/  {"LDAA" , iImmediate , _6800, 0                },
/*87*/  {""     , iInherent  ,   0  , 0                },
/*88*/  {"EORA" , iImmediate , _6800, 0                },
/*89*/  {"ADCA" , iImmediate , _6800, 0                },
/*8A*/  {"ORAA" , iImmediate , _6800, 0                },
/*8B*/  {"ADDA" , iImmediate , _6800, 0                },
/*8C*/  {"CPX"  , iLImmediate, _6800, 0                },
/*8D*/  {"BSR"  , iRelative  , _6800, REFFLAG | CODEREF},
/*8E*/  {"LDS"  , iLImmediate, _6800, 0                },
/*8F*/  {"XGDX" , iInherent  , _HC11, 0                }, // 68HC11

/*90*/  {"SUBA" , iDirect    , _6800, 0                },
/*91*/  {"CMPA" , iDirect    , _6800, 0                },
/*92*/  {"SBCA" , iDirect    , _6800, 0                },
/*93*/  {"SUBD" , iDirect    , _6801, 0                }, // 68HC11 6303 6801
/*94*/  {"ANDA" , iDirect    , _6800, 0                },
/*95*/  {"BITA" , iDirect    , _6800, 0                },
/*96*/  {"LDAA" , iDirect    , _6800, 0                },
/*97*/  {"STAA" , iDirect    , _6800, 0                },
/*98*/  {"EORA" , iDirect    , _6800, 0                },
/*99*/  {"ADCA" , iDirect    , _6800, 0                },
/*9A*/  {"ORAA" , iDirect    , _6800, 0                },
/*9B*/  {"ADDA" , iDirect    , _6800, 0                },
/*9C*/  {"CPX"  , iDirect    , _6800, 0                },
/*9D*/  {"JSR"  , iDirect    , _6801, 0                }, // 68HC11 6303 6801
/*9E*/  {"LDS"  , iDirect    , _6800, 0                },
/*9F*/  {"STS"  , iDirect    , _6800, 0                },

/*A0*/  {"SUBA" , iIndexed   , _6800, 0                },
/*A1*/  {"CMPA" , iIndexed   , _6800, 0                },
/*A2*/  {"SBCA" , iIndexed   , _6800, 0                },
/*A3*/  {"SUBD" , iIndexed   , _6801, 0                }, // 68HC11 6303 6801
/*A4*/  {"ANDA" , iIndexed   , _6800, 0                },
/*A5*/  {"BITA" , iIndexed   , _6800, 0                },
/*A6*/  {"LDAA" , iIndexed   , _6800, 0                },
/*A7*/  {"STAA" , iIndexed   , _6800, 0                },
/*A8*/  {"EORA" , iIndexed   , _6800, 0                },
/*A9*/  {"ADCA" , iIndexed   , _6800, 0                },
/*AA*/  {"ORAA" , iIndexed   , _6800, 0                },
/*AB*/  {"ADDA" , iIndexed   , _6800, 0                },
/*AC*/  {"CPX"  , iIndexed   , _6800, 0                },
/*AD*/  {"JSR"  , iIndexed   , _6800, 0                },
/*AE*/  {"LDS"  , iIndexed   , _6800, 0                },
/*AF*/  {"STS"  , iIndexed   , _6800, 0                },

/*B0*/  {"SUBA" , iExtended  , _6800, 0                },
/*B1*/  {"CMPA" , iExtended  , _6800, 0                },
/*B2*/  {"SBCA" , iExtended  , _6800, 0                },
/*B3*/  {"SUBD" , iExtended  , _6801, 0                }, // 68HC11 6303 6801
/*B4*/  {"ANDA" , iExtended  , _6800, 0                },
/*B5*/  {"BITA" , iExtended  , _6800, 0                },
/*B6*/  {"LDAA" , iExtended  , _6800, 0                },
/*B7*/  {"STAA" , iExtended  , _6800, 0                },
/*B8*/  {"EORA" , iExtended  , _6800, 0                },
/*B9*/  {"ADCA" , iExtended  , _6800, 0                },
/*BA*/  {"ORAA" , iExtended  , _6800, 0                },
/*BB*/  {"ADDA" , iExtended  , _6800, 0                },
/*BC*/  {"CPX"  , iExtended  , _6800, 0                },
/*BD*/  {"JSR"  , iExtended  , _6800, REFFLAG | CODEREF},
/*BE*/  {"LDS"  , iExtended  , _6800, 0                },
/*BF*/  {"STS"  , iExtended  , _6800, 0                },

/*C0*/  {"SUBB" , iImmediate , _6800, 0                },
/*C1*/  {"CMPB" , iImmediate , _6800, 0                },
/*C2*/  {"SBCB" , iImmediate , _6800, 0                },
/*C3*/  {"ADDD" , iLImmediate, _6801, 0                }, // 68HC11 6303 6801
/*C4*/  {"ANDB" , iImmediate , _6800, 0                },
/*C5*/  {"BITB" , iImmediate , _6800, 0                },
/*C6*/  {"LDAB" , iImmediate , _6800, 0                },
/*C7*/  {""     , iInherent  ,   0  , 0                },
/*C8*/  {"EORB" , iImmediate , _6800, 0                },
/*C9*/  {"ADCB" , iImmediate , _6800, 0                },
/*CA*/  {"ORAB" , iImmediate , _6800, 0                },
/*CB*/  {"ADDB" , iImmediate , _6800, 0                },
/*CC*/  {"LDD"  , iLImmediate, _6801, 0                }, // 68HC11 6303 6801
/*CD*/  {""     , iInherent  , _PAGE, 0                }, // 68HC11 PAGE-CD
/*CE*/  {"LDX"  , iLImmediate, _6800, 0                },
/*CF*/  {"STOP" , iInherent  , _HC11, 0                }, // 68HC11

/*D0*/  {"SUBB" , iDirect    , _6800, 0                },
/*D1*/  {"CMPB" , iDirect    , _6800, 0                },
/*D2*/  {"SBCB" , iDirect    , _6800, 0                },
/*D3*/  {"ADDD" , iDirect    , _6801, 0                }, // 68HC11 6303 6801
/*D4*/  {"ANDB" , iDirect    , _6800, 0                },
/*D5*/  {"BITB" , iDirect    , _6800, 0                },
/*D6*/  {"LDAB" , iDirect    , _6800, 0                },
/*D7*/  {"STAB" , iDirect    , _6800, 0                },
/*D8*/  {"EORB" , iDirect    , _6800, 0                },
/*D9*/  {"ADCB" , iDirect    , _6800, 0                },
/*DA*/  {"ORAB" , iDirect    , _6800, 0                },
/*DB*/  {"ADDB" , iDirect    , _6800, 0                },
/*DC*/  {"LDD"  , iDirect    , _6801, 0                }, // 68HC11 6303 6801
/*DD*/  {"STD"  , iDirect    , _6801, 0                }, // 68HC11 6303 6801
/*DE*/  {"LDX"  , iDirect    , _6800, 0                },
/*DF*/  {"STX"  , iDirect    , _6800, 0                },

/*E0*/  {"SUBB" , iIndexed   , _6800, 0                },
/*E1*/  {"CMPB" , iIndexed   , _6800, 0                },
/*E2*/  {"SBCB" , iIndexed   , _6800, 0                },
/*E3*/  {"ADDD" , iIndexed   , _6801, 0                }, // 68HC11 6303 6801
/*E4*/  {"ANDB" , iIndexed   , _6800, 0                },
/*E5*/  {"BITB" , iIndexed   , _6800, 0                },
/*E6*/  {"LDAB" , iIndexed   , _6800, 0                },
/*E7*/  {"STAB" , iIndexed   , _6800, 0                },
/*E8*/  {"EORB" , iIndexed   , _6800, 0                },
/*E9*/  {"ADCB" , iIndexed   , _6800, 0                },
/*EA*/  {"ORAB" , iIndexed   , _6800, 0                },
/*EB*/  {"ADDB" , iIndexed   , _6800, 0                },
/*EC*/  {"LDD"  , iIndexed   , _6801, 0                }, // 68HC11 6303 6801
/*ED*/  {"STD"  , iIndexed   , _6801, 0                }, // 68HC11 6303 6801
/*EE*/  {"LDX"  , iIndexed   , _6800, 0                },
/*EF*/  {"STX"  , iIndexed   , _6800, 0                },

/*F0*/  {"SUBB" , iExtended  , _6800, 0                },
/*F1*/  {"CMPB" , iExtended  , _6800, 0                },
/*F2*/  {"SBCB" , iExtended  , _6800, 0                },
/*F3*/  {"ADDD" , iExtended  , _6801, 0                }, // 68HC11 6303 6801
/*F4*/  {"ANDB" , iExtended  , _6800, 0                },
/*F5*/  {"BITB" , iExtended  , _6800, 0                },
/*F6*/  {"LDAB" , iExtended  , _6800, 0                },
/*F7*/  {"STAB" , iExtended  , _6800, 0                },
/*F8*/  {"EORB" , iExtended  , _6800, 0                },
/*F9*/  {"ADCB" , iExtended  , _6800, 0                },
/*FA*/  {"ORAB" , iExtended  , _6800, 0                },
/*FB*/  {"ADDB" , iExtended  , _6800, 0                },
/*FC*/  {"LDD"  , iExtended  , _6801, 0                }, // 68HC11 6303 6801
/*FD*/  {"STD"  , iExtended  , _6801, 0                }, // 68HC11 6303 6801
/*FE*/  {"LDX"  , iExtended  , _6800, 0                },
/*FF*/  {"STX"  , iExtended  , _6800, 0                }
};

static const struct InstrRec M68HC11_opcdTable18[] =
{
          // op        typ      cpu     lfref
/*1800*/  {""     , iInherent  , 0, 0                 },
/*1801*/  {""     , iInherent  , 0, 0                 },
/*1802*/  {""     , iInherent  , 0, 0                 },
/*1803*/  {""     , iInherent  , 0, 0                 },
/*1804*/  {""     , iInherent  , 0, 0                 },
/*1805*/  {""     , iInherent  , 0, 0                 },
/*1806*/  {""     , iInherent  , 0, 0                 },
/*1807*/  {""     , iInherent  , 0, 0                 },
/*1808*/  {"INY"  , iInherent  , _HC11, 0             }, // 68HC11
/*1809*/  {"DEY"  , iInherent  , _HC11, 0             }, // 68HC11
/*180A*/  {""     , iInherent  , 0, 0                 },
/*180B*/  {""     , iInherent  , 0, 0                 },
/*180C*/  {""     , iInherent  , 0, 0                 },
/*180D*/  {""     , iInherent  , 0, 0                 },
/*180E*/  {""     , iInherent  , 0, 0                 },
/*180F*/  {""     , iInherent  , 0, 0                 },

/*1810*/  {""     , iInherent  , 0, 0                 },
/*1811*/  {""     , iInherent  , 0, 0                 },
/*1812*/  {""     , iInherent  , 0, 0                 },
/*1813*/  {""     , iInherent  , 0, 0                 },
/*1814*/  {""     , iInherent  , 0, 0                 },
/*1815*/  {""     , iInherent  , 0, 0                 },
/*1816*/  {""     , iInherent  , 0, 0                 },
/*1817*/  {""     , iInherent  , 0, 0                 },
/*1818*/  {""     , iInherent  , 0, 0                 },
/*1819*/  {""     , iInherent  , 0, 0                 },
/*181A*/  {""     , iInherent  , 0, 0                 },
/*181B*/  {""     , iInherent  , 0, 0                 },
/*181C*/  {"BSET" , iBIndexedY , _HC11, 0             }, // 68HC11
/*181D*/  {"BCLR" , iBIndexedY , _HC11, 0             }, // 68HC11
/*181E*/  {"BRSET", iBYRelative, _HC11, 0             }, // 68HC11
/*181F*/  {"BRCLR", iBYRelative, _HC11, 0             }, // 68HC11

/*1820*/  {""     , iInherent  , 0, 0                 },
/*1821*/  {""     , iInherent  , 0, 0                 },
/*1822*/  {""     , iInherent  , 0, 0                 },
/*1823*/  {""     , iInherent  , 0, 0                 },
/*1824*/  {""     , iInherent  , 0, 0                 },
/*1825*/  {""     , iInherent  , 0, 0                 },
/*1826*/  {""     , iInherent  , 0, 0                 },
/*1827*/  {""     , iInherent  , 0, 0                 },
/*1828*/  {""     , iInherent  , 0, 0                 },
/*1829*/  {""     , iInherent  , 0, 0                 },
/*182A*/  {""     , iInherent  , 0, 0                 },
/*182B*/  {""     , iInherent  , 0, 0                 },
/*182C*/  {""     , iInherent  , 0, 0                 },
/*182D*/  {""     , iInherent  , 0, 0                 },
/*182E*/  {""     , iInherent  , 0, 0                 },
/*182F*/  {""     , iInherent  , 0, 0                 },

/*1830*/  {"TSY"  , iInherent  , _HC11, 0             }, // 68HC11
/*1831*/  {""     , iInherent  , 0, 0                 },
/*1832*/  {""     , iInherent  , 0, 0                 },
/*1833*/  {""     , iInherent  , 0, 0                 },
/*1834*/  {""     , iInherent  , 0, 0                 },
/*1835*/  {"TYS"  , iInherent  , _HC11, 0             }, // 68HC11
/*1836*/  {""     , iInherent  , 0, 0                 },
/*1837*/  {""     , iInherent  , 0, 0                 },
/*1838*/  {"PULY" , iInherent  , _HC11, 0             }, // 68HC11
/*1839*/  {""     , iInherent  , 0, 0                 },
/*183A*/  {"ABY"  , iInherent  , 0, 0                 }, // 68HC11
/*183B*/  {""     , iInherent  , 0, 0                 },
/*183C*/  {"PSHY" , iInherent  , _HC11, 0             }, // 68HC11
/*183D*/  {""     , iInherent  , 0, 0                 },
/*183E*/  {""     , iInherent  , 0, 0                 },
/*183F*/  {""     , iInherent  , 0, 0                 },

/*1840*/  {""     , iInherent  , 0, 0                 },
/*1841*/  {""     , iInherent  , 0, 0                 },
/*1842*/  {""     , iInherent  , 0, 0                 },
/*1843*/  {""     , iInherent  , 0, 0                 },
/*1844*/  {""     , iInherent  , 0, 0                 },
/*1845*/  {""     , iInherent  , 0, 0                 },
/*1846*/  {""     , iInherent  , 0, 0                 },
/*1847*/  {""     , iInherent  , 0, 0                 },
/*1848*/  {""     , iInherent  , 0, 0                 },
/*1849*/  {""     , iInherent  , 0, 0                 },
/*184A*/  {""     , iInherent  , 0, 0                 },
/*184B*/  {""     , iInherent  , 0, 0                 },
/*184C*/  {""     , iInherent  , 0, 0                 },
/*184D*/  {""     , iInherent  , 0, 0                 },
/*184E*/  {""     , iInherent  , 0, 0                 },
/*184F*/  {""     , iInherent  , 0, 0                 },

/*1850*/  {""     , iInherent  , 0, 0                 },
/*1851*/  {""     , iInherent  , 0, 0                 },
/*1852*/  {""     , iInherent  , 0, 0                 },
/*1853*/  {""     , iInherent  , 0, 0                 },
/*1854*/  {""     , iInherent  , 0, 0                 },
/*1855*/  {""     , iInherent  , 0, 0                 },
/*1856*/  {""     , iInherent  , 0, 0                 },
/*1857*/  {""     , iInherent  , 0, 0                 },
/*1858*/  {""     , iInherent  , 0, 0                 },
/*1859*/  {""     , iInherent  , 0, 0                 },
/*185A*/  {""     , iInherent  , 0, 0                 },
/*185B*/  {""     , iInherent  , 0, 0                 },
/*185C*/  {""     , iInherent  , 0, 0                 },
/*185D*/  {""     , iInherent  , 0, 0                 },
/*185E*/  {""     , iInherent  , 0, 0                 },
/*185F*/  {""     , iInherent  , 0, 0                 },

/*1860*/  {"NEG"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*1861*/  {""     , iInherent  , 0, 0                 },
/*1862*/  {""     , iInherent  , 0, 0                 },
/*1863*/  {"COM"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*1864*/  {"LSR"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*1865*/  {""     , iInherent  , 0, 0                 },
/*1866*/  {"ROR"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*1867*/  {"ASR"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*1868*/  {"ASL"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*1869*/  {"ROL"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*186A*/  {"DEC"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*186B*/  {""     , iInherent  , 0, 0                 },
/*186C*/  {"INC"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*186D*/  {"TST"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*186E*/  {"JMP"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*186F*/  {"CLR"  , iIndexedY  , _HC11, 0             }, // 68HC11

/*1870*/  {""     , iInherent  , 0, 0                 },
/*1871*/  {""     , iInherent  , 0, 0                 },
/*1872*/  {""     , iInherent  , 0, 0                 },
/*1873*/  {""     , iInherent  , 0, 0                 },
/*1874*/  {""     , iInherent  , 0, 0                 },
/*1875*/  {""     , iInherent  , 0, 0                 },
/*1876*/  {""     , iInherent  , 0, 0                 },
/*1877*/  {""     , iInherent  , 0, 0                 },
/*1878*/  {""     , iInherent  , 0, 0                 },
/*1879*/  {""     , iInherent  , 0, 0                 },
/*187A*/  {""     , iInherent  , 0, 0                 },
/*187B*/  {""     , iInherent  , 0, 0                 },
/*187C*/  {""     , iInherent  , 0, 0                 },
/*187D*/  {""     , iInherent  , 0, 0                 },
/*187E*/  {""     , iInherent  , 0, 0                 },
/*187F*/  {""     , iInherent  , 0, 0                 },

/*1880*/  {""     , iInherent  , 0, 0                 },
/*1881*/  {""     , iInherent  , 0, 0                 },
/*1882*/  {""     , iInherent  , 0, 0                 },
/*1883*/  {""     , iInherent  , 0, 0                 },
/*1884*/  {""     , iInherent  , 0, 0                 },
/*1885*/  {""     , iInherent  , 0, 0                 },
/*1886*/  {""     , iInherent  , 0, 0                 },
/*1887*/  {""     , iInherent  , 0, 0                 },
/*1888*/  {""     , iInherent  , 0, 0                 },
/*1889*/  {""     , iInherent  , 0, 0                 },
/*188A*/  {""     , iInherent  , 0, 0                 },
/*188B*/  {""     , iInherent  , 0, 0                 },
/*188C*/  {"CPY"  , iLImmediate, 0, 0                 }, // 68HC11
/*188D*/  {""     , iInherent  , 0, 0                 },
/*188E*/  {""     , iInherent  , 0, 0                 },
/*188F*/  {"XGDY" , iInherent  , 0, 0                 }, // 68HC11

/*1890*/  {""     , iInherent  , 0, 0                 },
/*1891*/  {""     , iInherent  , 0, 0                 },
/*1892*/  {""     , iInherent  , 0, 0                 },
/*1893*/  {""     , iInherent  , 0, 0                 },
/*1894*/  {""     , iInherent  , 0, 0                 },
/*1895*/  {""     , iInherent  , 0, 0                 },
/*1896*/  {""     , iInherent  , 0, 0                 },
/*1897*/  {""     , iInherent  , 0, 0                 },
/*1898*/  {""     , iInherent  , 0, 0                 },
/*1899*/  {""     , iInherent  , 0, 0                 },
/*189A*/  {""     , iInherent  , 0, 0                 },
/*189B*/  {""     , iInherent  , 0, 0                 },
/*189C*/  {"CPY"  , iDirect    , 0, 0                 }, // 68HC11
/*189D*/  {""     , iInherent  , 0, 0                 },
/*189E*/  {""     , iInherent  , 0, 0                 },
/*189F*/  {""     , iInherent  , 0, 0                 },

/*18A0*/  {"SUBA" , iIndexedY  , 0, 0                 }, // 68HC11
/*18A1*/  {"CMPA" , iIndexedY  , 0, 0                 }, // 68HC11
/*18A2*/  {"SBCA" , iIndexedY  , 0, 0                 }, // 68HC11
/*18A3*/  {"SUBD" , iIndexedY  , 0, 0                 }, // 68HC11
/*18A4*/  {"ANDA" , iIndexedY  , 0, 0                 }, // 68HC11
/*18A5*/  {"BITA" , iIndexedY  , 0, 0                 }, // 68HC11
/*18A6*/  {"LDAA" , iIndexedY  , 0, 0                 }, // 68HC11
/*18A7*/  {"STAA" , iIndexedY  , 0, 0                 }, // 68HC11
/*18A8*/  {"EORA" , iIndexedY  , 0, 0                 }, // 68HC11
/*18A9*/  {"ADCA" , iIndexedY  , 0, 0                 }, // 68HC11
/*18AA*/  {"ORAA" , iIndexedY  , 0, 0                 }, // 68HC11
/*18AB*/  {"ADDA" , iIndexedY  , 0, 0                 }, // 68HC11
/*18AC*/  {"CPY"  , iIndexedY  , 0, 0                 }, // 68HC11
/*18AD*/  {"JSR"  , iIndexedY  , 0, REFFLAG | CODEREF }, // 68HC11
/*18AE*/  {"LDS"  , iIndexedY  , 0, 0                 }, // 68HC11
/*18AF*/  {"STS"  , iIndexedY  , 0, 0                 }, // 68HC11

/*18B0*/  {""     , iInherent  , 0, 0                 },
/*18B1*/  {""     , iInherent  , 0, 0                 },
/*18B2*/  {""     , iInherent  , 0, 0                 },
/*18B3*/  {""     , iInherent  , 0, 0                 },
/*18B4*/  {""     , iInherent  , 0, 0                 },
/*18B5*/  {""     , iInherent  , 0, 0                 },
/*18B6*/  {""     , iInherent  , 0, 0                 },
/*18B7*/  {""     , iInherent  , 0, 0                 },
/*18B8*/  {""     , iInherent  , 0, 0                 },
/*18B9*/  {""     , iInherent  , 0, 0                 },
/*18BA*/  {""     , iInherent  , 0, 0                 },
/*18BB*/  {""     , iInherent  , 0, 0                 },
/*18BC*/  {"CPY"  , iExtended  , 0, 0                 }, // 68HC11
/*18BD*/  {""     , iInherent  , 0, 0                 },
/*18BE*/  {""     , iInherent  , 0, 0                 },
/*18BF*/  {""     , iInherent  , 0, 0                 },

/*18C0*/  {""     , iInherent  , 0, 0                 },
/*18C1*/  {""     , iInherent  , 0, 0                 },
/*18C2*/  {""     , iInherent  , 0, 0                 },
/*18C3*/  {""     , iInherent  , 0, 0                 },
/*18C4*/  {""     , iInherent  , 0, 0                 },
/*18C5*/  {""     , iInherent  , 0, 0                 },
/*18C6*/  {""     , iInherent  , 0, 0                 },
/*18C7*/  {""     , iInherent  , 0, 0                 },
/*18C8*/  {""     , iInherent  , 0, 0                 },
/*18C9*/  {""     , iInherent  , 0, 0                 },
/*18CA*/  {""     , iInherent  , 0, 0                 },
/*18CB*/  {""     , iInherent  , 0, 0                 },
/*18CC*/  {""     , iInherent  , 0, 0                 },
/*18CD*/  {""     , iInherent  , 0, 0                 },
/*18CE*/  {"LDY"  , iLImmediate, _HC11, 0             }, // 68HC11
/*18CF*/  {""     , iInherent  , 0, 0                 },

/*18D0*/  {""     , iInherent  , 0, 0                 },
/*18D1*/  {""     , iInherent  , 0, 0                 },
/*18D2*/  {""     , iInherent  , 0, 0                 },
/*18D3*/  {""     , iInherent  , 0, 0                 },
/*18D4*/  {""     , iInherent  , 0, 0                 },
/*18D5*/  {""     , iInherent  , 0, 0                 },
/*18D6*/  {""     , iInherent  , 0, 0                 },
/*18D7*/  {""     , iInherent  , 0, 0                 },
/*18D8*/  {""     , iInherent  , 0, 0                 },
/*18D9*/  {""     , iInherent  , 0, 0                 },
/*18DA*/  {""     , iInherent  , 0, 0                 },
/*18DB*/  {""     , iInherent  , 0, 0                 },
/*18DC*/  {""     , iInherent  , 0, 0                 },
/*18DD*/  {""     , iInherent  , 0, 0                 },
/*18DE*/  {"LDY"  , iDirect    , _HC11, 0             }, // 68HC11
/*18DF*/  {"STY"  , iDirect    , _HC11, 0             }, // 68HC11

/*18E0*/  {"SUBB" , iIndexedY  , _HC11, 0             }, // 68HC11
/*18E1*/  {"CMPB" , iIndexedY  , _HC11, 0             }, // 68HC11
/*18E2*/  {"SBCB" , iIndexedY  , _HC11, 0             }, // 68HC11
/*18E3*/  {"ADDD" , iIndexedY  , _HC11, 0             }, // 68HC11
/*18E4*/  {"ANDB" , iIndexedY  , _HC11, 0             }, // 68HC11
/*18E5*/  {"BITB" , iIndexedY  , _HC11, 0             }, // 68HC11
/*18E6*/  {"LDAB" , iIndexedY  , _HC11, 0             }, // 68HC11
/*18E7*/  {"STAB" , iIndexedY  , _HC11, 0             }, // 68HC11
/*18E8*/  {"EORB" , iIndexedY  , _HC11, 0             }, // 68HC11
/*18E9*/  {"ADCB" , iIndexedY  , _HC11, 0             }, // 68HC11
/*18EA*/  {"ORAB" , iIndexedY  , _HC11, 0             }, // 68HC11
/*18EB*/  {"ADDB" , iIndexedY  , _HC11, 0             }, // 68HC11
/*18EC*/  {"LDD"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*18ED*/  {"STD"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*18EE*/  {"LDY"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*18EF*/  {"STY"  , iIndexedY  , _HC11, 0             }, // 68HC11

/*18F0*/  {""     , iInherent  , 0, 0                 },
/*18F1*/  {""     , iInherent  , 0, 0                 },
/*18F2*/  {""     , iInherent  , 0, 0                 },
/*18F3*/  {""     , iInherent  , 0, 0                 },
/*18F4*/  {""     , iInherent  , 0, 0                 },
/*18F5*/  {""     , iInherent  , 0, 0                 },
/*18F6*/  {""     , iInherent  , 0, 0                 },
/*18F7*/  {""     , iInherent  , 0, 0                 },
/*18F8*/  {""     , iInherent  , 0, 0                 },
/*18F9*/  {""     , iInherent  , 0, 0                 },
/*18FA*/  {""     , iInherent  , 0, 0                 },
/*18FB*/  {""     , iInherent  , 0, 0                 },
/*18FC*/  {""     , iInherent  , 0, 0                 },
/*18FD*/  {""     , iInherent  , 0, 0                 },
/*18FE*/  {"LDY"  , iExtended  , _HC11, 0             }, // 68HC11
/*18FF*/  {"STY"  , iExtended  , _HC11, 0             }  // 68HC11
};

static const struct InstrRec M68HC11_opcdTable1A[] =
{
          // op        typ      cpu     lfref
/*1A00*/  {""     , iInherent  , 0, 0                 },
/*1A01*/  {""     , iInherent  , 0, 0                 },
/*1A02*/  {""     , iInherent  , 0, 0                 },
/*1A03*/  {""     , iInherent  , 0, 0                 },
/*1A04*/  {""     , iInherent  , 0, 0                 },
/*1A05*/  {""     , iInherent  , 0, 0                 },
/*1A06*/  {""     , iInherent  , 0, 0                 },
/*1A07*/  {""     , iInherent  , 0, 0                 },
/*1A08*/  {""     , iInherent  , 0, 0                 },
/*1A09*/  {""     , iInherent  , 0, 0                 },
/*1A0A*/  {""     , iInherent  , 0, 0                 },
/*1A0B*/  {""     , iInherent  , 0, 0                 },
/*1A0C*/  {""     , iInherent  , 0, 0                 },
/*1A0D*/  {""     , iInherent  , 0, 0                 },
/*1A0E*/  {""     , iInherent  , 0, 0                 },
/*1A0F*/  {""     , iInherent  , 0, 0                 },

/*1A10*/  {""     , iInherent  , 0, 0                 },
/*1A11*/  {""     , iInherent  , 0, 0                 },
/*1A12*/  {""     , iInherent  , 0, 0                 },
/*1A13*/  {""     , iInherent  , 0, 0                 },
/*1A14*/  {""     , iInherent  , 0, 0                 },
/*1A15*/  {""     , iInherent  , 0, 0                 },
/*1A16*/  {""     , iInherent  , 0, 0                 },
/*1A17*/  {""     , iInherent  , 0, 0                 },
/*1A18*/  {""     , iInherent  , 0, 0                 },
/*1A19*/  {""     , iInherent  , 0, 0                 },
/*1A1A*/  {""     , iInherent  , 0, 0                 },
/*1A1B*/  {""     , iInherent  , 0, 0                 },
/*1A1C*/  {""     , iInherent  , 0, 0                 },
/*1A1D*/  {""     , iInherent  , 0, 0                 },
/*1A1E*/  {""     , iInherent  , 0, 0                 },
/*1A1F*/  {""     , iInherent  , 0, 0                 },

/*1A20*/  {""     , iInherent  , 0, 0                 },
/*1A21*/  {""     , iInherent  , 0, 0                 },
/*1A22*/  {""     , iInherent  , 0, 0                 },
/*1A23*/  {""     , iInherent  , 0, 0                 },
/*1A24*/  {""     , iInherent  , 0, 0                 },
/*1A25*/  {""     , iInherent  , 0, 0                 },
/*1A26*/  {""     , iInherent  , 0, 0                 },
/*1A27*/  {""     , iInherent  , 0, 0                 },
/*1A28*/  {""     , iInherent  , 0, 0                 },
/*1A29*/  {""     , iInherent  , 0, 0                 },
/*1A2A*/  {""     , iInherent  , 0, 0                 },
/*1A2B*/  {""     , iInherent  , 0, 0                 },
/*1A2C*/  {""     , iInherent  , 0, 0                 },
/*1A2D*/  {""     , iInherent  , 0, 0                 },
/*1A2E*/  {""     , iInherent  , 0, 0                 },
/*1A2F*/  {""     , iInherent  , 0, 0                 },

/*1A30*/  {""     , iInherent  , 0, 0                 },
/*1A31*/  {""     , iInherent  , 0, 0                 },
/*1A32*/  {""     , iInherent  , 0, 0                 },
/*1A33*/  {""     , iInherent  , 0, 0                 },
/*1A34*/  {""     , iInherent  , 0, 0                 },
/*1A35*/  {""     , iInherent  , 0, 0                 },
/*1A36*/  {""     , iInherent  , 0, 0                 },
/*1A37*/  {""     , iInherent  , 0, 0                 },
/*1A38*/  {""     , iInherent  , 0, 0                 },
/*1A39*/  {""     , iInherent  , 0, 0                 },
/*1A3A*/  {""     , iInherent  , 0, 0                 },
/*1A3B*/  {""     , iInherent  , 0, 0                 },
/*1A3C*/  {""     , iInherent  , 0, 0                 },
/*1A3D*/  {""     , iInherent  , 0, 0                 },
/*1A3E*/  {""     , iInherent  , 0, 0                 },
/*1A3F*/  {""     , iInherent  , 0, 0                 },

/*1A40*/  {""     , iInherent  , 0, 0                 },
/*1A41*/  {""     , iInherent  , 0, 0                 },
/*1A42*/  {""     , iInherent  , 0, 0                 },
/*1A43*/  {""     , iInherent  , 0, 0                 },
/*1A44*/  {""     , iInherent  , 0, 0                 },
/*1A45*/  {""     , iInherent  , 0, 0                 },
/*1A46*/  {""     , iInherent  , 0, 0                 },
/*1A47*/  {""     , iInherent  , 0, 0                 },
/*1A48*/  {""     , iInherent  , 0, 0                 },
/*1A49*/  {""     , iInherent  , 0, 0                 },
/*1A4A*/  {""     , iInherent  , 0, 0                 },
/*1A4B*/  {""     , iInherent  , 0, 0                 },
/*1A4C*/  {""     , iInherent  , 0, 0                 },
/*1A4D*/  {""     , iInherent  , 0, 0                 },
/*1A4E*/  {""     , iInherent  , 0, 0                 },
/*1A4F*/  {""     , iInherent  , 0, 0                 },

/*1A50*/  {""     , iInherent  , 0, 0                 },
/*1A51*/  {""     , iInherent  , 0, 0                 },
/*1A52*/  {""     , iInherent  , 0, 0                 },
/*1A53*/  {""     , iInherent  , 0, 0                 },
/*1A54*/  {""     , iInherent  , 0, 0                 },
/*1A55*/  {""     , iInherent  , 0, 0                 },
/*1A56*/  {""     , iInherent  , 0, 0                 },
/*1A57*/  {""     , iInherent  , 0, 0                 },
/*1A58*/  {""     , iInherent  , 0, 0                 },
/*1A59*/  {""     , iInherent  , 0, 0                 },
/*1A5A*/  {""     , iInherent  , 0, 0                 },
/*1A5B*/  {""     , iInherent  , 0, 0                 },
/*1A5C*/  {""     , iInherent  , 0, 0                 },
/*1A5D*/  {""     , iInherent  , 0, 0                 },
/*1A5E*/  {""     , iInherent  , 0, 0                 },
/*1A5F*/  {""     , iInherent  , 0, 0                 },

/*1A60*/  {""     , iInherent  , 0, 0                 },
/*1A61*/  {""     , iInherent  , 0, 0                 },
/*1A62*/  {""     , iInherent  , 0, 0                 },
/*1A63*/  {""     , iInherent  , 0, 0                 },
/*1A64*/  {""     , iInherent  , 0, 0                 },
/*1A65*/  {""     , iInherent  , 0, 0                 },
/*1A66*/  {""     , iInherent  , 0, 0                 },
/*1A67*/  {""     , iInherent  , 0, 0                 },
/*1A68*/  {""     , iInherent  , 0, 0                 },
/*1A69*/  {""     , iInherent  , 0, 0                 },
/*1A6A*/  {""     , iInherent  , 0, 0                 },
/*1A6B*/  {""     , iInherent  , 0, 0                 },
/*1A6C*/  {""     , iInherent  , 0, 0                 },
/*1A6D*/  {""     , iInherent  , 0, 0                 },
/*1A6E*/  {""     , iInherent  , 0, 0                 },
/*1A6F*/  {""     , iInherent  , 0, 0                 },

/*1A70*/  {""     , iInherent  , 0, 0                 },
/*1A71*/  {""     , iInherent  , 0, 0                 },
/*1A72*/  {""     , iInherent  , 0, 0                 },
/*1A73*/  {""     , iInherent  , 0, 0                 },
/*1A74*/  {""     , iInherent  , 0, 0                 },
/*1A75*/  {""     , iInherent  , 0, 0                 },
/*1A76*/  {""     , iInherent  , 0, 0                 },
/*1A77*/  {""     , iInherent  , 0, 0                 },
/*1A78*/  {""     , iInherent  , 0, 0                 },
/*1A79*/  {""     , iInherent  , 0, 0                 },
/*1A7A*/  {""     , iInherent  , 0, 0                 },
/*1A7B*/  {""     , iInherent  , 0, 0                 },
/*1A7C*/  {""     , iInherent  , 0, 0                 },
/*1A7D*/  {""     , iInherent  , 0, 0                 },
/*1A7E*/  {""     , iInherent  , 0, 0                 },
/*1A7F*/  {""     , iInherent  , 0, 0                 },

/*1A80*/  {""     , iInherent  , 0, 0                 },
/*1A81*/  {""     , iInherent  , 0, 0                 },
/*1A82*/  {""     , iInherent  , 0, 0                 },
/*1A83*/  {"CPD"  , iLImmediate, _HC11, 0             }, // 68HC11
/*1A84*/  {""     , iInherent  , 0, 0                 },
/*1A85*/  {""     , iInherent  , 0, 0                 },
/*1A86*/  {""     , iInherent  , 0, 0                 },
/*1A87*/  {""     , iInherent  , 0, 0                 },
/*1A88*/  {""     , iInherent  , 0, 0                 },
/*1A89*/  {""     , iInherent  , 0, 0                 },
/*1A8A*/  {""     , iInherent  , 0, 0                 },
/*1A8B*/  {""     , iInherent  , 0, 0                 },
/*1A8C*/  {""     , iInherent  , 0, 0                 },
/*1A8D*/  {""     , iInherent  , 0, 0                 },
/*1A8E*/  {""     , iInherent  , 0, 0                 },
/*1A8F*/  {""     , iInherent  , 0, 0                 },

/*1A90*/  {""     , iInherent  , 0, 0                 },
/*1A91*/  {""     , iInherent  , 0, 0                 },
/*1A92*/  {""     , iInherent  , 0, 0                 },
/*1A93*/  {"CPD"  , iDirect    , _HC11, 0             }, // 68HC11
/*1A94*/  {""     , iInherent  , 0, 0                 },
/*1A95*/  {""     , iInherent  , 0, 0                 },
/*1A96*/  {""     , iInherent  , 0, 0                 },
/*1A97*/  {""     , iInherent  , 0, 0                 },
/*1A98*/  {""     , iInherent  , 0, 0                 },
/*1A99*/  {""     , iInherent  , 0, 0                 },
/*1A9A*/  {""     , iInherent  , 0, 0                 },
/*1A9B*/  {""     , iInherent  , 0, 0                 },
/*1A9C*/  {""     , iInherent  , 0, 0                 },
/*1A9D*/  {""     , iInherent  , 0, 0                 },
/*1A9E*/  {""     , iInherent  , 0, 0                 },
/*1A9F*/  {""     , iInherent  , 0, 0                 },

/*1AA0*/  {""     , iInherent,   0, 0                 },
/*1AA1*/  {""     , iInherent,   0, 0                 },
/*1AA2*/  {""     , iInherent,   0, 0                 },
/*1AA3*/  {"CPD"  , iIndexed ,   0, 0                 }, // 68HC11
/*1AA4*/  {""     , iInherent,   0, 0                 },
/*1AA5*/  {""     , iInherent,   0, 0                 },
/*1AA6*/  {""     , iInherent,   0, 0                 },
/*1AA7*/  {""     , iInherent,   0, 0                 },
/*1AA8*/  {""     , iInherent,   0, 0                 },
/*1AA9*/  {""     , iInherent,   0, 0                 },
/*1AAA*/  {""     , iInherent,   0, 0                 },
/*1AAB*/  {""     , iInherent,   0, 0                 },
/*1AAC*/  {"CPY"  , iIndexed ,   _HC11, 0             }, // 68HC11
/*1AAD*/  {""     , iInherent,   0, 0                 },
/*1AAE*/  {""     , iInherent,   0, 0                 },
/*1AAF*/  {""     , iInherent,   0, 0                 },

/*1AB0*/  {""     , iInherent  , 0, 0                 },
/*1AB1*/  {""     , iInherent  , 0, 0                 },
/*1AB2*/  {""     , iInherent  , 0, 0                 },
/*1AB3*/  {"CPD"  , iExtended  , _HC11, 0             }, // 68HC11
/*1AB4*/  {""     , iInherent  , 0, 0                 },
/*1AB5*/  {""     , iInherent  , 0, 0                 },
/*1AB6*/  {""     , iInherent  , 0, 0                 },
/*1AB7*/  {""     , iInherent  , 0, 0                 },
/*1AB8*/  {""     , iInherent  , 0, 0                 },
/*1AB9*/  {""     , iInherent  , 0, 0                 },
/*1ABA*/  {""     , iInherent  , 0, 0                 },
/*1ABB*/  {""     , iInherent  , 0, 0                 },
/*1ABC*/  {""     , iInherent  , 0, 0                 },
/*1ABD*/  {""     , iInherent  , 0, 0                 },
/*1ABE*/  {""     , iInherent  , 0, 0                 },
/*1ABF*/  {""     , iInherent  , 0, 0                 },

/*1AC0*/  {""     , iInherent  , 0, 0                 },
/*1AC1*/  {""     , iInherent  , 0, 0                 },
/*1AC2*/  {""     , iInherent  , 0, 0                 },
/*1AC3*/  {""     , iInherent  , 0, 0                 },
/*1AC4*/  {""     , iInherent  , 0, 0                 },
/*1AC5*/  {""     , iInherent  , 0, 0                 },
/*1AC6*/  {""     , iInherent  , 0, 0                 },
/*1AC7*/  {""     , iInherent  , 0, 0                 },
/*1AC8*/  {""     , iInherent  , 0, 0                 },
/*1AC9*/  {""     , iInherent  , 0, 0                 },
/*1ACA*/  {""     , iInherent  , 0, 0                 },
/*1ACB*/  {""     , iInherent  , 0, 0                 },
/*1ACC*/  {""     , iInherent  , 0, 0                 },
/*1ACD*/  {""     , iInherent  , 0, 0                 },
/*1ACE*/  {""     , iInherent  , 0, 0                 },
/*1ACF*/  {""     , iInherent  , 0, 0                 },

/*1AD0*/  {""     , iInherent  , 0, 0                 },
/*1AD1*/  {""     , iInherent  , 0, 0                 },
/*1AD2*/  {""     , iInherent  , 0, 0                 },
/*1AD3*/  {""     , iInherent  , 0, 0                 },
/*1AD4*/  {""     , iInherent  , 0, 0                 },
/*1AD5*/  {""     , iInherent  , 0, 0                 },
/*1AD6*/  {""     , iInherent  , 0, 0                 },
/*1AD7*/  {""     , iInherent  , 0, 0                 },
/*1AD8*/  {""     , iInherent  , 0, 0                 },
/*1AD9*/  {""     , iInherent  , 0, 0                 },
/*1ADA*/  {""     , iInherent  , 0, 0                 },
/*1ADB*/  {""     , iInherent  , 0, 0                 },
/*1ADC*/  {""     , iInherent  , 0, 0                 },
/*1ADD*/  {""     , iInherent  , 0, 0                 },
/*1ADE*/  {""     , iInherent  , 0, 0                 },
/*1ADF*/  {""     , iInherent  , 0, 0                 },

/*1AE0*/  {""     , iInherent  , 0, 0                 },
/*1AE1*/  {""     , iInherent  , 0, 0                 },
/*1AE2*/  {""     , iInherent  , 0, 0                 },
/*1AE3*/  {""     , iInherent  , 0, 0                 },
/*1AE4*/  {""     , iInherent  , 0, 0                 },
/*1AE5*/  {""     , iInherent  , 0, 0                 },
/*1AE6*/  {""     , iInherent  , 0, 0                 },
/*1AE7*/  {""     , iInherent  , 0, 0                 },
/*1AE8*/  {""     , iInherent  , 0, 0                 },
/*1AE9*/  {""     , iInherent  , 0, 0                 },
/*1AEA*/  {""     , iInherent  , 0, 0                 },
/*1AEB*/  {""     , iInherent  , 0, 0                 },
/*1AEC*/  {""     , iInherent  , 0, 0                 },
/*1AED*/  {""     , iInherent  , 0, 0                 },
/*1AEE*/  {"LDY"  , iIndexed   , _HC11, 0             }, // 68HC11
/*1AEF*/  {"STY"  , iIndexed   , _HC11, 0             }, // 68HC11

/*1AF0*/  {""     , iInherent  , 0, 0                 },
/*1AF1*/  {""     , iInherent  , 0, 0                 },
/*1AF2*/  {""     , iInherent  , 0, 0                 },
/*1AF3*/  {""     , iInherent  , 0, 0                 },
/*1AF4*/  {""     , iInherent  , 0, 0                 },
/*1AF5*/  {""     , iInherent  , 0, 0                 },
/*1AF6*/  {""     , iInherent  , 0, 0                 },
/*1AF7*/  {""     , iInherent  , 0, 0                 },
/*1AF8*/  {""     , iInherent  , 0, 0                 },
/*1AF9*/  {""     , iInherent  , 0, 0                 },
/*1AFA*/  {""     , iInherent  , 0, 0                 },
/*1AFB*/  {""     , iInherent  , 0, 0                 },
/*1AFC*/  {""     , iInherent  , 0, 0                 },
/*1AFD*/  {""     , iInherent  , 0, 0                 },
/*1AFE*/  {""     , iInherent  , 0, 0                 },
/*1AFF*/  {""     , iInherent  , 0, 0                 }
};

static const struct InstrRec M68HC11_opcdTableCD[] =
{
          // op        typ      cpu     lfref
/*CD00*/  {""     , iInherent  , 0, 0                 },
/*CD01*/  {""     , iInherent  , 0, 0                 },
/*CD02*/  {""     , iInherent  , 0, 0                 },
/*CD03*/  {""     , iInherent  , 0, 0                 },
/*CD04*/  {""     , iInherent  , 0, 0                 },
/*CD05*/  {""     , iInherent  , 0, 0                 },
/*CD06*/  {""     , iInherent  , 0, 0                 },
/*CD07*/  {""     , iInherent  , 0, 0                 },
/*CD08*/  {""     , iInherent  , 0, 0                 },
/*CD09*/  {""     , iInherent  , 0, 0                 },
/*CD0A*/  {""     , iInherent  , 0, 0                 },
/*CD0B*/  {""     , iInherent  , 0, 0                 },
/*CD0C*/  {""     , iInherent  , 0, 0                 },
/*CD0D*/  {""     , iInherent  , 0, 0                 },
/*CD0E*/  {""     , iInherent  , 0, 0                 },
/*CD0F*/  {""     , iInherent  , 0, 0                 },

/*CD10*/  {""     , iInherent  , 0, 0                 },
/*CD11*/  {""     , iInherent  , 0, 0                 },
/*CD12*/  {""     , iInherent  , 0, 0                 },
/*CD13*/  {""     , iInherent  , 0, 0                 },
/*CD14*/  {""     , iInherent  , 0, 0                 },
/*CD15*/  {""     , iInherent  , 0, 0                 },
/*CD16*/  {""     , iInherent  , 0, 0                 },
/*CD17*/  {""     , iInherent  , 0, 0                 },
/*CD18*/  {""     , iInherent  , 0, 0                 },
/*CD19*/  {""     , iInherent  , 0, 0                 },
/*CD1A*/  {""     , iInherent  , 0, 0                 },
/*CD1B*/  {""     , iInherent  , 0, 0                 },
/*CD1C*/  {""     , iInherent  , 0, 0                 },
/*CD1D*/  {""     , iInherent  , 0, 0                 },
/*CD1E*/  {""     , iInherent  , 0, 0                 },
/*CD1F*/  {""     , iInherent  , 0, 0                 },

/*CD20*/  {""     , iInherent  , 0, 0                 },
/*CD21*/  {""     , iInherent  , 0, 0                 },
/*CD22*/  {""     , iInherent  , 0, 0                 },
/*CD23*/  {""     , iInherent  , 0, 0                 },
/*CD24*/  {""     , iInherent  , 0, 0                 },
/*CD25*/  {""     , iInherent  , 0, 0                 },
/*CD26*/  {""     , iInherent  , 0, 0                 },
/*CD27*/  {""     , iInherent  , 0, 0                 },
/*CD28*/  {""     , iInherent  , 0, 0                 },
/*CD29*/  {""     , iInherent  , 0, 0                 },
/*CD2A*/  {""     , iInherent  , 0, 0                 },
/*CD2B*/  {""     , iInherent  , 0, 0                 },
/*CD2C*/  {""     , iInherent  , 0, 0                 },
/*CD2D*/  {""     , iInherent  , 0, 0                 },
/*CD2E*/  {""     , iInherent  , 0, 0                 },
/*CD2F*/  {""     , iInherent  , 0, 0                 },

/*CD30*/  {""     , iInherent  , 0, 0                 },
/*CD31*/  {""     , iInherent  , 0, 0                 },
/*CD32*/  {""     , iInherent  , 0, 0                 },
/*CD33*/  {""     , iInherent  , 0, 0                 },
/*CD34*/  {""     , iInherent  , 0, 0                 },
/*CD35*/  {""     , iInherent  , 0, 0                 },
/*CD36*/  {""     , iInherent  , 0, 0                 },
/*CD37*/  {""     , iInherent  , 0, 0                 },
/*CD38*/  {""     , iInherent  , 0, 0                 },
/*CD39*/  {""     , iInherent  , 0, 0                 },
/*CD3A*/  {""     , iInherent  , 0, 0                 },
/*CD3B*/  {""     , iInherent  , 0, 0                 },
/*CD3C*/  {""     , iInherent  , 0, 0                 },
/*CD3D*/  {""     , iInherent  , 0, 0                 },
/*CD3E*/  {""     , iInherent  , 0, 0                 },
/*CD3F*/  {""     , iInherent  , 0, 0                 },

/*CD40*/  {""     , iInherent  , 0, 0                 },
/*CD41*/  {""     , iInherent  , 0, 0                 },
/*CD42*/  {""     , iInherent  , 0, 0                 },
/*CD43*/  {""     , iInherent  , 0, 0                 },
/*CD44*/  {""     , iInherent  , 0, 0                 },
/*CD45*/  {""     , iInherent  , 0, 0                 },
/*CD46*/  {""     , iInherent  , 0, 0                 },
/*CD47*/  {""     , iInherent  , 0, 0                 },
/*CD48*/  {""     , iInherent  , 0, 0                 },
/*CD49*/  {""     , iInherent  , 0, 0                 },
/*CD4A*/  {""     , iInherent  , 0, 0                 },
/*CD4B*/  {""     , iInherent  , 0, 0                 },
/*CD4C*/  {""     , iInherent  , 0, 0                 },
/*CD4D*/  {""     , iInherent  , 0, 0                 },
/*CD4E*/  {""     , iInherent  , 0, 0                 },
/*CD4F*/  {""     , iInherent  , 0, 0                 },

/*CD50*/  {""     , iInherent  , 0, 0                 },
/*CD51*/  {""     , iInherent  , 0, 0                 },
/*CD52*/  {""     , iInherent  , 0, 0                 },
/*CD53*/  {""     , iInherent  , 0, 0                 },
/*CD54*/  {""     , iInherent  , 0, 0                 },
/*CD55*/  {""     , iInherent  , 0, 0                 },
/*CD56*/  {""     , iInherent  , 0, 0                 },
/*CD57*/  {""     , iInherent  , 0, 0                 },
/*CD58*/  {""     , iInherent  , 0, 0                 },
/*CD59*/  {""     , iInherent  , 0, 0                 },
/*CD5A*/  {""     , iInherent  , 0, 0                 },
/*CD5B*/  {""     , iInherent  , 0, 0                 },
/*CD5C*/  {""     , iInherent  , 0, 0                 },
/*CD5D*/  {""     , iInherent  , 0, 0                 },
/*CD5E*/  {""     , iInherent  , 0, 0                 },
/*CD5F*/  {""     , iInherent  , 0, 0                 },

/*CD60*/  {""     , iInherent  , 0, 0                 },
/*CD61*/  {""     , iInherent  , 0, 0                 },
/*CD62*/  {""     , iInherent  , 0, 0                 },
/*CD63*/  {""     , iInherent  , 0, 0                 },
/*CD64*/  {""     , iInherent  , 0, 0                 },
/*CD65*/  {""     , iInherent  , 0, 0                 },
/*CD66*/  {""     , iInherent  , 0, 0                 },
/*CD67*/  {""     , iInherent  , 0, 0                 },
/*CD68*/  {""     , iInherent  , 0, 0                 },
/*CD69*/  {""     , iInherent  , 0, 0                 },
/*CD6A*/  {""     , iInherent  , 0, 0                 },
/*CD6B*/  {""     , iInherent  , 0, 0                 },
/*CD6C*/  {""     , iInherent  , 0, 0                 },
/*CD6D*/  {""     , iInherent  , 0, 0                 },
/*CD6E*/  {""     , iInherent  , 0, 0                 },
/*CD6F*/  {""     , iInherent  , 0, 0                 },

/*CD70*/  {""     , iInherent  , 0, 0                 },
/*CD71*/  {""     , iInherent  , 0, 0                 },
/*CD72*/  {""     , iInherent  , 0, 0                 },
/*CD73*/  {""     , iInherent  , 0, 0                 },
/*CD74*/  {""     , iInherent  , 0, 0                 },
/*CD75*/  {""     , iInherent  , 0, 0                 },
/*CD76*/  {""     , iInherent  , 0, 0                 },
/*CD77*/  {""     , iInherent  , 0, 0                 },
/*CD78*/  {""     , iInherent  , 0, 0                 },
/*CD79*/  {""     , iInherent  , 0, 0                 },
/*CD7A*/  {""     , iInherent  , 0, 0                 },
/*CD7B*/  {""     , iInherent  , 0, 0                 },
/*CD7C*/  {""     , iInherent  , 0, 0                 },
/*CD7D*/  {""     , iInherent  , 0, 0                 },
/*CD7E*/  {""     , iInherent  , 0, 0                 },
/*CD7F*/  {""     , iInherent  , 0, 0                 },

/*CD80*/  {""     , iInherent  , 0, 0                 },
/*CD81*/  {""     , iInherent  , 0, 0                 },
/*CD82*/  {""     , iInherent  , 0, 0                 },
/*CD83*/  {""     , iInherent  , 0, 0                 },
/*CD84*/  {""     , iInherent  , 0, 0                 },
/*CD85*/  {""     , iInherent  , 0, 0                 },
/*CD86*/  {""     , iInherent  , 0, 0                 },
/*CD87*/  {""     , iInherent  , 0, 0                 },
/*CD88*/  {""     , iInherent  , 0, 0                 },
/*CD89*/  {""     , iInherent  , 0, 0                 },
/*CD8A*/  {""     , iInherent  , 0, 0                 },
/*CD8B*/  {""     , iInherent  , 0, 0                 },
/*CD8C*/  {""     , iInherent  , 0, 0                 },
/*CD8D*/  {""     , iInherent  , 0, 0                 },
/*CD8E*/  {""     , iInherent  , 0, 0                 },
/*CD8F*/  {""     , iInherent  , 0, 0                 },

/*CD90*/  {""     , iInherent  , 0, 0                 },
/*CD91*/  {""     , iInherent  , 0, 0                 },
/*CD92*/  {""     , iInherent  , 0, 0                 },
/*CD93*/  {""     , iInherent  , 0, 0                 },
/*CD94*/  {""     , iInherent  , 0, 0                 },
/*CD95*/  {""     , iInherent  , 0, 0                 },
/*CD96*/  {""     , iInherent  , 0, 0                 },
/*CD97*/  {""     , iInherent  , 0, 0                 },
/*CD98*/  {""     , iInherent  , 0, 0                 },
/*CD99*/  {""     , iInherent  , 0, 0                 },
/*CD9A*/  {""     , iInherent  , 0, 0                 },
/*CD9B*/  {""     , iInherent  , 0, 0                 },
/*CD9C*/  {""     , iInherent  , 0, 0                 },
/*CD9D*/  {""     , iInherent  , 0, 0                 },
/*CD9E*/  {""     , iInherent  , 0, 0                 },
/*CD9F*/  {""     , iInherent  , 0, 0                 },

/*CDA0*/  {""     , iInherent  , 0, 0                 },
/*CDA1*/  {""     , iInherent  , 0, 0                 },
/*CDA2*/  {""     , iInherent  , 0, 0                 },
/*CDA3*/  {"CPD"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*CDA4*/  {""     , iInherent  , 0, 0                 },
/*CDA5*/  {""     , iInherent  , 0, 0                 },
/*CDA6*/  {""     , iInherent  , 0, 0                 },
/*CDA7*/  {""     , iInherent  , 0, 0                 },
/*CDA8*/  {""     , iInherent  , 0, 0                 },
/*CDA9*/  {""     , iInherent  , 0, 0                 },
/*CDAA*/  {""     , iInherent  , 0, 0                 },
/*CDAB*/  {""     , iInherent  , 0, 0                 },
/*CDAC*/  {"CPX"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*CDAD*/  {""     , iInherent  , 0, 0                 },
/*CDAE*/  {""     , iInherent  , 0, 0                 },
/*CDAF*/  {""     , iInherent  , 0, 0                 },

/*CDB0*/  {""     , iInherent  , 0, 0                 },
/*CDB1*/  {""     , iInherent  , 0, 0                 },
/*CDB2*/  {""     , iInherent  , 0, 0                 },
/*CDB3*/  {""     , iInherent  , 0, 0                 },
/*CDB4*/  {""     , iInherent  , 0, 0                 },
/*CDB5*/  {""     , iInherent  , 0, 0                 },
/*CDB6*/  {""     , iInherent  , 0, 0                 },
/*CDB7*/  {""     , iInherent  , 0, 0                 },
/*CDB8*/  {""     , iInherent  , 0, 0                 },
/*CDB9*/  {""     , iInherent  , 0, 0                 },
/*CDBA*/  {""     , iInherent  , 0, 0                 },
/*CDBB*/  {""     , iInherent  , 0, 0                 },
/*CDBC*/  {""     , iInherent  , 0, 0                 },
/*CDBD*/  {""     , iInherent  , 0, 0                 },
/*CDBE*/  {""     , iInherent  , 0, 0                 },
/*CDBF*/  {""     , iInherent  , 0, 0                 },

/*CDC0*/  {""     , iInherent  , 0, 0                 },
/*CDC1*/  {""     , iInherent  , 0, 0                 },
/*CDC2*/  {""     , iInherent  , 0, 0                 },
/*CDC3*/  {""     , iInherent  , 0, 0                 },
/*CDC4*/  {""     , iInherent  , 0, 0                 },
/*CDC5*/  {""     , iInherent  , 0, 0                 },
/*CDC6*/  {""     , iInherent  , 0, 0                 },
/*CDC7*/  {""     , iInherent  , 0, 0                 },
/*CDC8*/  {""     , iInherent  , 0, 0                 },
/*CDC9*/  {""     , iInherent  , 0, 0                 },
/*CDCA*/  {""     , iInherent  , 0, 0                 },
/*CDCB*/  {""     , iInherent  , 0, 0                 },
/*CDCC*/  {""     , iInherent  , 0, 0                 },
/*CDCD*/  {""     , iInherent  , 0, 0                 },
/*CDCE*/  {""     , iInherent  , 0, 0                 },
/*CDCF*/  {""     , iInherent  , 0, 0                 },

/*CDD0*/  {""     , iInherent  , 0, 0                 },
/*CDD1*/  {""     , iInherent  , 0, 0                 },
/*CDD2*/  {""     , iInherent  , 0, 0                 },
/*CDD3*/  {""     , iInherent  , 0, 0                 },
/*CDD4*/  {""     , iInherent  , 0, 0                 },
/*CDD5*/  {""     , iInherent  , 0, 0                 },
/*CDD6*/  {""     , iInherent  , 0, 0                 },
/*CDD7*/  {""     , iInherent  , 0, 0                 },
/*CDD8*/  {""     , iInherent  , 0, 0                 },
/*CDD9*/  {""     , iInherent  , 0, 0                 },
/*CDDA*/  {""     , iInherent  , 0, 0                 },
/*CDDB*/  {""     , iInherent  , 0, 0                 },
/*CDDC*/  {""     , iInherent  , 0, 0                 },
/*CDDD*/  {""     , iInherent  , 0, 0                 },
/*CDDE*/  {""     , iInherent  , 0, 0                 },
/*CDDF*/  {""     , iInherent  , 0, 0                 },

/*CDE0*/  {""     , iInherent  , 0, 0                 },
/*CDE1*/  {""     , iInherent  , 0, 0                 },
/*CDE2*/  {""     , iInherent  , 0, 0                 },
/*CDE3*/  {""     , iInherent  , 0, 0                 },
/*CDE4*/  {""     , iInherent  , 0, 0                 },
/*CDE5*/  {""     , iInherent  , 0, 0                 },
/*CDE6*/  {""     , iInherent  , 0, 0                 },
/*CDE7*/  {""     , iInherent  , 0, 0                 },
/*CDE8*/  {""     , iInherent  , 0, 0                 },
/*CDE9*/  {""     , iInherent  , 0, 0                 },
/*CDEA*/  {""     , iInherent  , 0, 0                 },
/*CDEB*/  {""     , iInherent  , 0, 0                 },
/*CDEC*/  {""     , iInherent  , 0, 0                 },
/*CDED*/  {""     , iInherent  , 0, 0                 },
/*CDEE*/  {"LDX"  , iIndexedY  , _HC11, 0             }, // 68HC11
/*CDEF*/  {"STX"  , iIndexedY  , _HC11, 0             }, // 68HC11

/*CDF0*/  {""     , iInherent  , 0, 0                 },
/*CDF1*/  {""     , iInherent  , 0, 0                 },
/*CDF2*/  {""     , iInherent  , 0, 0                 },
/*CDF3*/  {""     , iInherent  , 0, 0                 },
/*CDF4*/  {""     , iInherent  , 0, 0                 },
/*CDF5*/  {""     , iInherent  , 0, 0                 },
/*CDF6*/  {""     , iInherent  , 0, 0                 },
/*CDF7*/  {""     , iInherent  , 0, 0                 },
/*CDF8*/  {""     , iInherent  , 0, 0                 },
/*CDF9*/  {""     , iInherent  , 0, 0                 },
/*CDFA*/  {""     , iInherent  , 0, 0                 },
/*CDFB*/  {""     , iInherent  , 0, 0                 },
/*CDFC*/  {""     , iInherent  , 0, 0                 },
/*CDFD*/  {""     , iInherent  , 0, 0                 },
/*CDFE*/  {""     , iInherent  , 0, 0                 },
/*CDFF*/  {""     , iInherent  , 0, 0                 }
};


int Dis68HC11::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
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
    instr = &M6800_opcdTable[i];

    // is it a 68HC11 page opcode?
    if ((_subtype & CPU_68HC11) && (instr->cpu & _PAGE)) {
        switch (i) {
            default: // should not get here
                assert(false);
                break;

            case 0x18:
                i     = ReadByte(ad++);
                len   = 2;
                instr = &M68HC11_opcdTable18[i];
                break;

            case 0x1A:
                i     = ReadByte(ad++);
                len   = 2;
                instr = &M68HC11_opcdTable1A[i];
                break;

            case 0xCD:
                i     = ReadByte(ad++);
                len   = 2;
                instr = &M68HC11_opcdTableCD[i];
                break;

        }
    }

    strcpy(opcode, instr->op);
    lfref    = instr->lfref;
    parms[0] = 0;
    refaddr  = 0;

    // check if instruction is supported for this CPU
    if ((instr->cpu & _subtype) && opcode[0]) {
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

            case iIndexed: // nn,X
                i = ReadByte(ad++); // offset
                sprintf(parms, "$%.2X,X", i);
                len++;
                break;

            case iIndexedY: // nn,Y
                i = ReadByte(ad++); // offset
                sprintf(parms, "$%.2X,Y", i);
                len++;
                break;

            case iBDirect: // nn,mask
                i = ReadByte(ad++); // direct addr
                j = ReadByte(ad++); // mask
                sprintf(parms, "$%.2X,$%.2X", i, j);
                len += 2;
                break;

            case iBIndexed: // nn,X,mask
                i = ReadByte(ad++); // direct addr
                j = ReadByte(ad++); // mask
                sprintf(parms, "$%.2X,X,$%.2X", i, j);
                len += 2;
                break;

            case iBIndexedY: // nn,Y,mask
                i = ReadByte(ad++); // offset
                j = ReadByte(ad++); // mask
                sprintf(parms, "$%.2X,Y,$%.2X", i, j);
                len += 2;
                break;

            case iBDRelative: // nn,mask,addr
                i = ReadByte(ad++); // direct addr
                j = ReadByte(ad++); // mask
                k = ReadByte(ad++); // branch
                if (k == 0xFF) {
                    // offset FF is suspicious
                    lfref |= RIPSTOP;
                }
                len += 3;
                if (k >= 128) {
                    k = k - 256;
                }
                ra = (ad + k) & 0xFFFF;
                RefStr(ra, s, lfref, refaddr);
                sprintf(parms, "$%.2X,$%.2X,%s", i, j, s);
                break;

            case iBXRelative: // nn,X,mask,addr
                i = ReadByte(ad++); // direct addr
                j = ReadByte(ad++); // mask
                k = ReadByte(ad++); // branch
                if (k == 0xFF) {
                    // offset FF is suspicious
                    lfref |= RIPSTOP;
                }
                len += 3;
                if (k >= 128) {
                    k = k - 256;
                }
                ra = (ad + k) & 0xFFFF;
                RefStr(ra, s, lfref, refaddr);
                sprintf(parms, "$%.2X,X,$%.2X,%s", i, j, s);
                break;

            case iBYRelative: // nn,Y,mask,addr
                i = ReadByte(ad++); // direct addr
                j = ReadByte(ad++); // mask
                k = ReadByte(ad++); // branch
                if (k == 0xFF) {
                    // offset FF is suspicious
                    lfref |= RIPSTOP;
                }
                len += 3;
                if (k >= 128) {
                    k = k - 256;
                }
                ra = (ad + k) & 0xFFFF;
                RefStr(ra, s, lfref, refaddr);
                sprintf(parms, "$%.2X,Y,$%.2X,%s", i, j, s);
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

            case iMIndexed: // #bit,$ofs,X
                i = ReadByte(ad++); // bit
                j = ReadByte(ad++); // ofs
                sprintf(parms, "#%.2X,$%.2X,X", i, j);
                break;

            case iMExtended: // #bit,$dir
                i = ReadByte(ad++); // bit
                j = ReadByte(ad++); // direct addr
                sprintf(parms, "#%.2X,$%.2X", i, j);
                break;
        }
    } else {
        opcode[0] = 0;
    }

    // rip-stop checks
    if (opcode[0]) {
        switch (ReadByte(addr)) {
            case 0xFF: // STX $FFFF
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
