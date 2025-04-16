// dis68HC12.cpp

static const char versionName[] = "Motorola 68HC12 disassembler";

// changes based on being in asm vs listing mode
#if 1
 const bool asmMode = true;
#else
 #include "disline.h"
 #define asmMode (!(disline.line_cols & disline.B_HEX))
#endif

#include "discpu.h"

class Dis68HC12 : public CPU {
public:
    Dis68HC12(const char *name, int subtype, int endian, int addrwid,
              char curAddrChr, char hexChr, const char *byteOp,
              const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);

private:
    void dis_idx(unsigned int &ad, int &len, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


Dis68HC12 cpu_68HC12("68HC12", 0, BIG_END, ADDR_16, '*', '$', "FCB", "FDB", "DL");


enum InstType {
    iInherent, iDirect, iImmediate, iLImmediate, iRelative, iLRelative,
    iIndexed, iExtended, iLExtended, iTfrExg, iLoop, iMove, iTrap,
    iBIndexed, iBDirect, iBExtended, iBDRelative, iBERelative, iBXRelative
};


Dis68HC12::Dis68HC12(const char *name, int subtype, int endian, int addrwid,
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

static const struct InstrRec M68HC12_opcdTable[] =
{
        // op       lf    ref     typ
/*00*/  {"BGND" , iInherent , 0                },
/*01*/  {"MEM"  , iInherent , 0                },
/*02*/  {"INY"  , iInherent , 0                },
/*03*/  {"DEY"  , iInherent , 0                },
/*04*/  {""     , iInherent , 0                }, // loop pre-byte
/*05*/  {"JMP"  , iIndexed  , LFFLAG | REFFLAG | CODEREF },
/*06*/  {"JMP"  , iExtended , LFFLAG | REFFLAG | CODEREF },
/*07*/  {"BSR"  , iRelative , REFFLAG | CODEREF},
/*08*/  {"INX"  , iInherent , 0                },
/*09*/  {"DEX"  , iInherent , 0                },
/*0A*/  {"RTC"  , iInherent , LFFLAG           },
/*0B*/  {"RTI"  , iInherent , LFFLAG           },
/*0C*/  {"BSET" , iBIndexed , 0                },
/*0D*/  {"BCLR" , iBIndexed , 0                },
/*0E*/  {"BRSET", iBXRelative, REFFLAG | CODEREF},
/*0F*/  {"BRCLR", iBXRelative, REFFLAG | CODEREF},

/*10*/  {"ANDCC", iImmediate, 0                },
/*11*/  {"EDIV" , iInherent , 0                },
/*12*/  {"MUL"  , iInherent , 0                },
/*13*/  {"EMUL" , iInherent , 0                },
/*14*/  {"ORCC" , iImmediate, 0                },
/*15*/  {"JSR"  , iIndexed  , REFFLAG | CODEREF},
/*16*/  {"JSR"  , iExtended , REFFLAG | CODEREF},
/*17*/  {"JSR"  , iDirect   , REFFLAG | CODEREF},
/*18*/  {""     , iInherent , 0                },    // page 2 pre-byte
/*19*/  {"LEAY" , iIndexed  , 0                },
/*1A*/  {"LEAX" , iIndexed  , 0                },
/*1B*/  {"LEAS" , iIndexed  , 0                },
/*1C*/  {"BSET" , iBExtended, 0                },
/*1D*/  {"BCLR" , iBExtended, 0                },
/*1E*/  {"BRSET", iBERelative, REFFLAG | CODEREF},
/*1F*/  {"BRCLR", iBERelative, REFFLAG | CODEREF},

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

/*30*/  {"PULX" , iInherent, 0                },
/*31*/  {"PULY" , iInherent, 0                },
/*32*/  {"PULA" , iInherent, 0                },
/*33*/  {"PULB" , iInherent, 0                },
/*34*/  {"PSHX" , iInherent, 0                },
/*35*/  {"PSHY" , iInherent, 0                },
/*36*/  {"PSHA" , iInherent, 0                },
/*37*/  {"PSHB" , iInherent, 0                },
/*38*/  {"PULC" , iInherent, 0                },
/*39*/  {"PSHC" , iInherent, 0                },
/*3A*/  {"PULD" , iInherent, 0                },
/*3B*/  {"PSHD" , iInherent, 0                },
/*3C*/  {""     , iInherent, 0                }, // WAV second byte
/*3D*/  {"RTS"  , iInherent, LFFLAG           },
/*3E*/  {"WAI"  , iInherent, 0                },
/*3F*/  {"SWI"  , iInherent, 0                },

/*40*/  {"NEGA" , iInherent, 0                },
/*41*/  {"COMA" , iInherent, 0                },
/*42*/  {"INCA" , iInherent, 0                },
/*43*/  {"DECA" , iInherent, 0                },
/*44*/  {"LSRA" , iInherent, 0                },
/*45*/  {"ROLA" , iInherent, 0                },
/*46*/  {"RORA" , iInherent, 0                },
/*47*/  {"ASRA" , iInherent, 0                },
/*48*/  {"ASLA" , iInherent, 0                },
/*49*/  {"LSRD" , iInherent, 0                },
/*4A*/  {"CALL" , iLExtended,0                },
/*4B*/  {"CALL" , iIndexed , 0                },
/*4C*/  {"BSET" , iBDirect,  0                },
/*4D*/  {"BCLR" , iBDirect,  0                },
/*4E*/  {"BRSET", iBDRelative, REFFLAG | CODEREF},
/*4F*/  {"BRCLR", iBDRelative, REFFLAG | CODEREF},

/*50*/  {"NEGB" , iInherent, 0                },
/*51*/  {"COMB" , iInherent, 0                },
/*52*/  {"INCB" , iInherent, 0                },
/*53*/  {"DECB" , iInherent, 0                },
/*54*/  {"LSRB" , iInherent, 0                },
/*55*/  {"ROLB" , iInherent, 0                },
/*56*/  {"RORB" , iInherent, 0                },
/*57*/  {"ASRB" , iInherent, 0                },
/*58*/  {"ASLB" , iInherent, 0                },
/*59*/  {"ASLD" , iInherent, 0                },
/*5A*/  {"STAA" , iDirect  , 0                },
/*5B*/  {"STAB" , iDirect  , 0                },
/*5C*/  {"STD"  , iDirect  , 0                },
/*5D*/  {"STY"  , iDirect  , 0                },
/*5E*/  {"STX"  , iDirect  , 0                },
/*5F*/  {"STS"  , iDirect  , 0                },

/*60*/  {"NEG"  , iIndexed , 0                },
/*61*/  {"COM"  , iIndexed , 0                },
/*62*/  {"INC"  , iIndexed , 0                },
/*63*/  {"DEC"  , iIndexed , 0                },
/*64*/  {"LSR"  , iIndexed , 0                },
/*65*/  {"ROL"  , iIndexed , 0                },
/*66*/  {"ROR"  , iIndexed , 0                },
/*67*/  {"ASR"  , iIndexed , 0                },
/*68*/  {"ASL"  , iIndexed , 0                },
/*69*/  {"CLR"  , iIndexed , 0                },
/*6A*/  {"STAA" , iIndexed , 0                },
/*6B*/  {"STAB" , iIndexed , 0                },
/*6C*/  {"STD"  , iIndexed , 0                },
/*6D*/  {"STY"  , iIndexed , 0                },
/*6E*/  {"STX"  , iIndexed , 0                },
/*6F*/  {"STS"  , iIndexed , 0                },

/*70*/  {"NEG"  , iExtended, 0                },
/*71*/  {"COM"  , iExtended, 0                },
/*72*/  {"INC"  , iExtended, 0                },
/*73*/  {"DEC"  , iExtended, 0                },
/*74*/  {"LSR"  , iExtended, 0                },
/*75*/  {"ROL"  , iExtended, 0                },
/*76*/  {"ROR"  , iExtended, 0                },
/*77*/  {"ASR"  , iExtended, 0                },
/*78*/  {"ASL"  , iExtended, 0                },
/*79*/  {"CLR"  , iExtended, 0                },
/*7A*/  {"STAA" , iExtended, 0                },
/*7B*/  {"STAB" , iExtended, 0                },
/*7C*/  {"STD"  , iExtended, 0                },
/*7D*/  {"STY"  , iExtended, 0                },
/*7E*/  {"STX"  , iExtended, 0                },
/*7F*/  {"STS"  , iExtended, 0                },

/*80*/  {"SUBA" , iImmediate , 0                },
/*81*/  {"CMPA" , iImmediate , 0                },
/*82*/  {"SBCA" , iImmediate , 0                },
/*83*/  {"SUBD" , iLImmediate, 0                },
/*84*/  {"ANDA" , iImmediate , 0                },
/*85*/  {"BITA" , iImmediate , 0                },
/*86*/  {"LDAA" , iImmediate , 0                },
/*87*/  {"CLRA" , iInherent  , 0                },
/*88*/  {"EORA" , iImmediate , 0                },
/*89*/  {"ADCA" , iImmediate , 0                },
/*8A*/  {"ORAA" , iImmediate , 0                },
/*8B*/  {"ADDA" , iImmediate , 0                },
/*8C*/  {"CPD"  , iLImmediate, 0                },
/*8D*/  {"CPY"  , iLImmediate, 0                },
/*8E*/  {"CPX"  , iLImmediate, 0                },
/*8F*/  {"CPS"  , iLImmediate, 0                },

/*90*/  {"SUBA" , iDirect, 0                },
/*91*/  {"CMPA" , iDirect, 0                },
/*92*/  {"SBCA" , iDirect, 0                },
/*93*/  {"SUBD" , iDirect, 0                },
/*94*/  {"ANDA" , iDirect, 0                },
/*95*/  {"BITA" , iDirect, 0                },
/*96*/  {"LDAA" , iDirect, 0                },
/*97*/  {"TSTA" , iDirect, 0                },
/*98*/  {"EORA" , iDirect, 0                },
/*99*/  {"ADCA" , iDirect, 0                },
/*9A*/  {"ORAA" , iDirect, 0                },
/*9B*/  {"ADDA" , iDirect, 0                },
/*9C*/  {"CPD"  , iDirect, 0                },
/*9D*/  {"CPY"  , iDirect, 0                },
/*9E*/  {"CPX"  , iDirect, 0                },
/*9F*/  {"CPS"  , iDirect, 0                },

/*A0*/  {"SUBA" , iIndexed , 0                },
/*A1*/  {"CMPA" , iIndexed , 0                },
/*A2*/  {"SBCA" , iIndexed , 0                },
/*A3*/  {"SUBD" , iIndexed , 0                },
/*A4*/  {"ANDA" , iIndexed , 0                },
/*A5*/  {"BITA" , iIndexed , 0                },
/*A6*/  {"LDAA" , iIndexed , 0                },
/*A7*/  {"NOP"  , iInherent, 0                },
/*A8*/  {"EORA" , iIndexed , 0                },
/*A9*/  {"ADCA" , iIndexed , 0                },
/*AA*/  {"ORAA" , iIndexed , 0                },
/*AB*/  {"ADDA" , iIndexed , 0                },
/*AC*/  {"CPD"  , iIndexed , 0                },
/*AD*/  {"CPY"  , iIndexed , 0                },
/*AE*/  {"CPX"  , iIndexed , 0                },
/*AF*/  {"CPS"  , iIndexed , 0                },

/*B0*/  {"SUBA" , iExtended, 0                },
/*B1*/  {"CMPA" , iExtended, 0                },
/*B2*/  {"SBCA" , iExtended, 0                },
/*B3*/  {"SUBD" , iExtended, 0                },
/*B4*/  {"ANDA" , iExtended, 0                },
/*B5*/  {"BITA" , iExtended, 0                },
/*B6*/  {"LDAA" , iExtended, 0                },
/*B7*/  {"TFR"  , iTfrExg  , 0                }, // TFR/EXG
/*B8*/  {"EORA" , iExtended, 0                },
/*B9*/  {"ADCA" , iExtended, 0                },
/*BA*/  {"ORAA" , iExtended, 0                },
/*BB*/  {"ADDA" , iExtended, 0                },
/*BC*/  {"CPD"  , iExtended, 0                },
/*BD*/  {"CPY"  , iExtended, 0                },
/*BE*/  {"CPX"  , iExtended, 0                },
/*BF*/  {"CPS"  , iExtended, 0                },

/*C0*/  {"SUBB" , iImmediate , 0                },
/*C1*/  {"CMPB" , iImmediate , 0                },
/*C2*/  {"SBCB" , iImmediate , 0                },
/*C3*/  {"ADDD" , iLImmediate, 0                },
/*C4*/  {"ANDB" , iImmediate , 0                },
/*C5*/  {"BITB" , iImmediate , 0                },
/*C6*/  {"LDAB" , iImmediate , 0                },
/*C7*/  {"CLRB" , iInherent  , 0                },
/*C8*/  {"EORB" , iImmediate , 0                },
/*C9*/  {"ADCB" , iImmediate , 0                },
/*CA*/  {"ORAB" , iImmediate , 0                },
/*CB*/  {"ADDB" , iImmediate , 0                },
/*CC*/  {"LDD"  , iLImmediate, 0                },
/*CD*/  {"LDY"  , iLImmediate, 0                },
/*CE*/  {"LDX"  , iLImmediate, 0                },
/*CF*/  {"LDS"  , iLImmediate, 0                },

/*D0*/  {"SUBB" , iDirect , 0                },
/*D1*/  {"CMPB" , iDirect , 0                },
/*D2*/  {"SBCB" , iDirect , 0                },
/*D3*/  {"ADDD" , iDirect , 0                },
/*D4*/  {"ANDB" , iDirect , 0                },
/*D5*/  {"BITB" , iDirect , 0                },
/*D6*/  {"LDAB" , iDirect , 0                },
/*D7*/  {"TSTB" , iInherent,0                },
/*D8*/  {"EORB" , iDirect , 0                },
/*D9*/  {"ADCB" , iDirect , 0                },
/*DA*/  {"ORAB" , iDirect , 0                },
/*DB*/  {"ADDB" , iDirect , 0                },
/*DC*/  {"LDD"  , iDirect , 0                },
/*DD*/  {"LDY"  , iDirect , 0                },
/*DE*/  {"LDX"  , iDirect , 0                },
/*DF*/  {"LDS"  , iDirect , 0                },

/*E0*/  {"SUBB" , iIndexed, 0                },
/*E1*/  {"CMPB" , iIndexed, 0                },
/*E2*/  {"SBCB" , iIndexed, 0                },
/*E3*/  {"ADDD" , iIndexed, 0                },
/*E4*/  {"ANDB" , iIndexed, 0                },
/*E5*/  {"BITB" , iIndexed, 0                },
/*E6*/  {"LDAB" , iIndexed, 0                },
/*E7*/  {"TST"  , iIndexed, 0                },
/*E8*/  {"EORB" , iIndexed, 0                },
/*E9*/  {"ADCB" , iIndexed, 0                },
/*EA*/  {"ORAB" , iIndexed, 0                },
/*EB*/  {"ADDB" , iIndexed, 0                },
/*EC*/  {"LDD"  , iIndexed, 0                },
/*ED*/  {"LDY"  , iIndexed, 0                },
/*EE*/  {"LDX"  , iIndexed, 0                },
/*EF*/  {"LDS"  , iIndexed, 0                },

/*F0*/  {"SUBB" , iExtended, 0                },
/*F1*/  {"CMPB" , iExtended, 0                },
/*F2*/  {"SBCB" , iExtended, 0                },
/*F3*/  {"ADDD" , iExtended, 0                },
/*F4*/  {"ANDB" , iExtended, 0                },
/*F5*/  {"BITB" , iExtended, 0                },
/*F6*/  {"LDAB" , iExtended, 0                },
/*F7*/  {"TST"  , iExtended, 0                },
/*F8*/  {"EORB" , iExtended, 0                },
/*F9*/  {"ADCB" , iExtended, 0                },
/*FA*/  {"ORAB" , iExtended, 0                },
/*FB*/  {"ADDB" , iExtended, 0                },
/*FC*/  {"LDD"  , iExtended, 0                },
/*FD*/  {"LDY"  , iExtended, 0                },
/*FE*/  {"LDX"  , iExtended, 0                },
/*FF*/  {"LDS"  , iExtended, 0                }
};

static const struct InstrRec M68HC12_opcdTable18[] =
{
            // op       lf    ref     typ
/*1800*/    {"MOVW" , iMove    , 0                }, // MOVW	IM-ID 5
/*1801*/    {"MOVW" , iMove    , 0                }, // MOVW	EX-ID 5
/*1802*/    {"MOVW" , iMove    , 0                }, // MOVW	ID-ID 4
/*1803*/    {"MOVW" , iMove    , 0                }, // MOVW	IM-EX 6
/*1804*/    {"MOVW" , iMove    , 0                }, // MOVW	EX-EX 6
/*1805*/    {"MOVW" , iMove    , 0                }, // MOVW	ID-EX 5
/*1806*/    {"ABA"  , iInherent, 0                },
/*1807*/    {"DAA"  , iInherent, 0                },
/*1808*/    {"MOVB" , iMove    , 0                }, // MOVB	IM-ID 4
/*1809*/    {"MOVB" , iMove    , 0                }, // MOVB	EX-ID 5
/*180A*/    {"MOVB" , iMove    , 0                }, // MOVB	ID-ID 4
/*180B*/    {"MOVB" , iMove    , 0                }, // MOVB	IM-EX 5
/*180C*/    {"MOVB" , iMove    , 0                }, // MOVB	EX-EX 6
/*180D*/    {"MOVB" , iMove    , 0                }, // MOVB	ID-EX 5
/*180E*/    {"TAB"  , iInherent, 0                },
/*180F*/    {"TBA"  , iInherent, 0                },

/*1810*/    {"IDIV" , iInherent , 0                },
/*1811*/    {"FDIV" , iInherent , 0                },
/*1812*/    {"EMACS", iLImmediate,0                },
/*1813*/    {"EMULS", iInherent , 0                },
/*1814*/    {"EDIVS", iInherent , 0                },
/*1815*/    {"IDIVS", iInherent , 0                },
/*1816*/    {"SBA"  , iInherent , 0                },
/*1817*/    {"CBA"  , iInherent , 0                },
/*1818*/    {"MAXA" , iIndexed  , 0                },
/*1819*/    {"MINA" , iIndexed  , 0                },
/*181A*/    {"EMAXD", iIndexed  , 0                },
/*181B*/    {"EMIND", iIndexed  , 0                },
/*181C*/    {"MINM" , iIndexed  , 0                },
/*181D*/    {"MAXM" , iIndexed  , 0                },
/*181E*/    {"EMAXM", iIndexed  , 0                },
/*181F*/    {"EMINM", iIndexed  , 0                },

/*1820*/    {"LBRA" , iLRelative, LFFLAG | REFFLAG | CODEREF},
/*1821*/    {"LBRN" , iLRelative, 0                },
/*1822*/    {"LBHI" , iLRelative, REFFLAG | CODEREF},
/*1823*/    {"LBLS" , iLRelative, REFFLAG | CODEREF},
/*1824*/    {"LBCC" , iLRelative, REFFLAG | CODEREF},    // also LBHS
/*1825*/    {"LBCS" , iLRelative, REFFLAG | CODEREF},    // also LBLO
/*1826*/    {"LBNE" , iLRelative, REFFLAG | CODEREF},
/*1827*/    {"LBEQ" , iLRelative, REFFLAG | CODEREF},
/*1828*/    {"LBVC" , iLRelative, REFFLAG | CODEREF},
/*1829*/    {"LBVS" , iLRelative, REFFLAG | CODEREF},
/*182A*/    {"LBPL" , iLRelative, REFFLAG | CODEREF},
/*182B*/    {"LBMI" , iLRelative, REFFLAG | CODEREF},
/*182C*/    {"LBGE" , iLRelative, REFFLAG | CODEREF},
/*182D*/    {"LBLT" , iLRelative, REFFLAG | CODEREF},
/*182E*/    {"LBGT" , iLRelative, REFFLAG | CODEREF},
/*182F*/    {"LBLE" , iLRelative, REFFLAG | CODEREF},

/*1830*/    {"TRAP" , iTrap    , 0                },
/*1831*/    {"TRAP" , iTrap    , 0                },
/*1832*/    {"TRAP" , iTrap    , 0                },
/*1833*/    {"TRAP" , iTrap    , 0                },
/*1834*/    {"TRAP" , iTrap    , 0                },
/*1835*/    {"TRAP" , iTrap    , 0                },
/*1836*/    {"TRAP" , iTrap    , 0                },
/*1837*/    {"TRAP" , iTrap    , 0                },
/*1838*/    {"TRAP" , iTrap    , 0                },
/*1839*/    {"TRAP" , iTrap    , 0                },
/*183A*/    {"REV"  , iInherent, 0                },
/*183B*/    {"REVW" , iInherent, 0                },
/*183C*/    {"WAV"  , iInherent, 0                },
/*183D*/    {"TBL"  , iIndexed , 0                },
/*183E*/    {"STOP" , iInherent, 0                },
/*183F*/    {"ETBL" , iIndexed , 0                },
// note: TBL and ETBL only allow single-byte indexed modes

/*1840*/    {"TRAP" , iTrap    , 0                },
/*1841*/    {"TRAP" , iTrap    , 0                },
/*1842*/    {"TRAP" , iTrap    , 0                },
/*1843*/    {"TRAP" , iTrap    , 0                },
/*1844*/    {"TRAP" , iTrap    , 0                },
/*1845*/    {"TRAP" , iTrap    , 0                },
/*1846*/    {"TRAP" , iTrap    , 0                },
/*1847*/    {"TRAP" , iTrap    , 0                },
/*1848*/    {"TRAP" , iTrap    , 0                },
/*1849*/    {"TRAP" , iTrap    , 0                },
/*184A*/    {"TRAP" , iTrap    , 0                },
/*184B*/    {"TRAP" , iTrap    , 0                },
/*184C*/    {"TRAP" , iTrap    , 0                },
/*184D*/    {"TRAP" , iTrap    , 0                },
/*184E*/    {"TRAP" , iTrap    , 0                },
/*184F*/    {"TRAP" , iTrap    , 0                },

/*1850*/    {"TRAP" , iTrap    , 0                },
/*1851*/    {"TRAP" , iTrap    , 0                },
/*1852*/    {"TRAP" , iTrap    , 0                },
/*1853*/    {"TRAP" , iTrap    , 0                },
/*1854*/    {"TRAP" , iTrap    , 0                },
/*1855*/    {"TRAP" , iTrap    , 0                },
/*1856*/    {"TRAP" , iTrap    , 0                },
/*1857*/    {"TRAP" , iTrap    , 0                },
/*1858*/    {"TRAP" , iTrap    , 0                },
/*1859*/    {"TRAP" , iTrap    , 0                },
/*185A*/    {"TRAP" , iTrap    , 0                },
/*185B*/    {"TRAP" , iTrap    , 0                },
/*185C*/    {"TRAP" , iTrap    , 0                },
/*185D*/    {"TRAP" , iTrap    , 0                },
/*185E*/    {"TRAP" , iTrap    , 0                },
/*185F*/    {"TRAP" , iTrap    , 0                },

/*1860*/    {"TRAP" , iTrap    , 0                },
/*1861*/    {"TRAP" , iTrap    , 0                },
/*1862*/    {"TRAP" , iTrap    , 0                },
/*1863*/    {"TRAP" , iTrap    , 0                },
/*1864*/    {"TRAP" , iTrap    , 0                },
/*1865*/    {"TRAP" , iTrap    , 0                },
/*1866*/    {"TRAP" , iTrap    , 0                },
/*1867*/    {"TRAP" , iTrap    , 0                },
/*1868*/    {"TRAP" , iTrap    , 0                },
/*1869*/    {"TRAP" , iTrap    , 0                },
/*186A*/    {"TRAP" , iTrap    , 0                },
/*186B*/    {"TRAP" , iTrap    , 0                },
/*186C*/    {"TRAP" , iTrap    , 0                },
/*186D*/    {"TRAP" , iTrap    , 0                },
/*186E*/    {"TRAP" , iTrap    , 0                },
/*186F*/    {"TRAP" , iTrap    , 0                },

/*1870*/    {"TRAP" , iTrap    , 0                },
/*1871*/    {"TRAP" , iTrap    , 0                },
/*1872*/    {"TRAP" , iTrap    , 0                },
/*1873*/    {"TRAP" , iTrap    , 0                },
/*1874*/    {"TRAP" , iTrap    , 0                },
/*1875*/    {"TRAP" , iTrap    , 0                },
/*1876*/    {"TRAP" , iTrap    , 0                },
/*1877*/    {"TRAP" , iTrap    , 0                },
/*1878*/    {"TRAP" , iTrap    , 0                },
/*1879*/    {"TRAP" , iTrap    , 0                },
/*187A*/    {"TRAP" , iTrap    , 0                },
/*187B*/    {"TRAP" , iTrap    , 0                },
/*187C*/    {"TRAP" , iTrap    , 0                },
/*187D*/    {"TRAP" , iTrap    , 0                },
/*187E*/    {"TRAP" , iTrap    , 0                },
/*187F*/    {"TRAP" , iTrap    , 0                },

/*1880*/    {"TRAP" , iTrap    , 0                },
/*1881*/    {"TRAP" , iTrap    , 0                },
/*1882*/    {"TRAP" , iTrap    , 0                },
/*1883*/    {"TRAP" , iTrap    , 0                },
/*1884*/    {"TRAP" , iTrap    , 0                },
/*1885*/    {"TRAP" , iTrap    , 0                },
/*1886*/    {"TRAP" , iTrap    , 0                },
/*1887*/    {"TRAP" , iTrap    , 0                },
/*1888*/    {"TRAP" , iTrap    , 0                },
/*1889*/    {"TRAP" , iTrap    , 0                },
/*188A*/    {"TRAP" , iTrap    , 0                },
/*188B*/    {"TRAP" , iTrap    , 0                },
/*188C*/    {"TRAP" , iTrap    , 0                },
/*188D*/    {"TRAP" , iTrap    , 0                },
/*188E*/    {"TRAP" , iTrap    , 0                },
/*188F*/    {"TRAP" , iTrap    , 0                },

/*1890*/    {"TRAP" , iTrap    , 0                },
/*1891*/    {"TRAP" , iTrap    , 0                },
/*1892*/    {"TRAP" , iTrap    , 0                },
/*1893*/    {"TRAP" , iTrap    , 0                },
/*1894*/    {"TRAP" , iTrap    , 0                },
/*1895*/    {"TRAP" , iTrap    , 0                },
/*1896*/    {"TRAP" , iTrap    , 0                },
/*1897*/    {"TRAP" , iTrap    , 0                },
/*1898*/    {"TRAP" , iTrap    , 0                },
/*1899*/    {"TRAP" , iTrap    , 0                },
/*189A*/    {"TRAP" , iTrap    , 0                },
/*189B*/    {"TRAP" , iTrap    , 0                },
/*189C*/    {"TRAP" , iTrap    , 0                },
/*189D*/    {"TRAP" , iTrap    , 0                },
/*189E*/    {"TRAP" , iTrap    , 0                },
/*189F*/    {"TRAP" , iTrap    , 0                },

/*18A0*/    {"TRAP" , iTrap    , 0                },
/*18A1*/    {"TRAP" , iTrap    , 0                },
/*18A2*/    {"TRAP" , iTrap    , 0                },
/*18A3*/    {"TRAP" , iTrap    , 0                },
/*18A4*/    {"TRAP" , iTrap    , 0                },
/*18A5*/    {"TRAP" , iTrap    , 0                },
/*18A6*/    {"TRAP" , iTrap    , 0                },
/*18A7*/    {"TRAP" , iTrap    , 0                },
/*18A8*/    {"TRAP" , iTrap    , 0                },
/*18A9*/    {"TRAP" , iTrap    , 0                },
/*18AA*/    {"TRAP" , iTrap    , 0                },
/*18AB*/    {"TRAP" , iTrap    , 0                },
/*18AC*/    {"TRAP" , iTrap    , 0                },
/*18AD*/    {"TRAP" , iTrap    , 0                },
/*18AE*/    {"TRAP" , iTrap    , 0                },
/*18AF*/    {"TRAP" , iTrap    , 0                },

/*18B0*/    {"TRAP" , iTrap    , 0                },
/*18B1*/    {"TRAP" , iTrap    , 0                },
/*18B2*/    {"TRAP" , iTrap    , 0                },
/*18B3*/    {"TRAP" , iTrap    , 0                },
/*18B4*/    {"TRAP" , iTrap    , 0                },
/*18B5*/    {"TRAP" , iTrap    , 0                },
/*18B6*/    {"TRAP" , iTrap    , 0                },
/*18B7*/    {"TRAP" , iTrap    , 0                },
/*18B8*/    {"TRAP" , iTrap    , 0                },
/*18B9*/    {"TRAP" , iTrap    , 0                },
/*18BA*/    {"TRAP" , iTrap    , 0                },
/*18BB*/    {"TRAP" , iTrap    , 0                },
/*18BC*/    {"TRAP" , iTrap    , 0                },
/*18BD*/    {"TRAP" , iTrap    , 0                },
/*18BE*/    {"TRAP" , iTrap    , 0                },
/*18BF*/    {"TRAP" , iTrap    , 0                },

/*18C0*/    {"TRAP" , iTrap    , 0                },
/*18C1*/    {"TRAP" , iTrap    , 0                },
/*18C2*/    {"TRAP" , iTrap    , 0                },
/*18C3*/    {"TRAP" , iTrap    , 0                },
/*18C4*/    {"TRAP" , iTrap    , 0                },
/*18C5*/    {"TRAP" , iTrap    , 0                },
/*18C6*/    {"TRAP" , iTrap    , 0                },
/*18C7*/    {"TRAP" , iTrap    , 0                },
/*18C8*/    {"TRAP" , iTrap    , 0                },
/*18C9*/    {"TRAP" , iTrap    , 0                },
/*18CA*/    {"TRAP" , iTrap    , 0                },
/*18CB*/    {"TRAP" , iTrap    , 0                },
/*18CC*/    {"TRAP" , iTrap    , 0                },
/*18CD*/    {"TRAP" , iTrap    , 0                },
/*18CE*/    {"TRAP" , iTrap    , 0                },
/*18CF*/    {"TRAP" , iTrap    , 0                },

/*18D0*/    {"TRAP" , iTrap    , 0                },
/*18D1*/    {"TRAP" , iTrap    , 0                },
/*18D2*/    {"TRAP" , iTrap    , 0                },
/*18D3*/    {"TRAP" , iTrap    , 0                },
/*18D4*/    {"TRAP" , iTrap    , 0                },
/*18D5*/    {"TRAP" , iTrap    , 0                },
/*18D6*/    {"TRAP" , iTrap    , 0                },
/*18D7*/    {"TRAP" , iTrap    , 0                },
/*18D8*/    {"TRAP" , iTrap    , 0                },
/*18D9*/    {"TRAP" , iTrap    , 0                },
/*18DA*/    {"TRAP" , iTrap    , 0                },
/*18DB*/    {"TRAP" , iTrap    , 0                },
/*18DC*/    {"TRAP" , iTrap    , 0                },
/*18DD*/    {"TRAP" , iTrap    , 0                },
/*18DE*/    {"TRAP" , iTrap    , 0                },
/*18DF*/    {"TRAP" , iTrap    , 0                },

/*18E0*/    {"TRAP" , iTrap    , 0                },
/*18E1*/    {"TRAP" , iTrap    , 0                },
/*18E2*/    {"TRAP" , iTrap    , 0                },
/*18E3*/    {"TRAP" , iTrap    , 0                },
/*18E4*/    {"TRAP" , iTrap    , 0                },
/*18E5*/    {"TRAP" , iTrap    , 0                },
/*18E6*/    {"TRAP" , iTrap    , 0                },
/*18E7*/    {"TRAP" , iTrap    , 0                },
/*18E8*/    {"TRAP" , iTrap    , 0                },
/*18E9*/    {"TRAP" , iTrap    , 0                },
/*18EA*/    {"TRAP" , iTrap    , 0                },
/*18EB*/    {"TRAP" , iTrap    , 0                },
/*18EC*/    {"TRAP" , iTrap    , 0                },
/*18ED*/    {"TRAP" , iTrap    , 0                },
/*18EE*/    {"TRAP" , iTrap    , 0                },
/*18EF*/    {"TRAP" , iTrap    , 0                },

/*18F0*/    {"TRAP" , iTrap    , 0                },
/*18F1*/    {"TRAP" , iTrap    , 0                },
/*18F2*/    {"TRAP" , iTrap    , 0                },
/*18F3*/    {"TRAP" , iTrap    , 0                },
/*18F4*/    {"TRAP" , iTrap    , 0                },
/*18F5*/    {"TRAP" , iTrap    , 0                },
/*18F6*/    {"TRAP" , iTrap    , 0                },
/*18F7*/    {"TRAP" , iTrap    , 0                },
/*18F8*/    {"TRAP" , iTrap    , 0                },
/*18F9*/    {"TRAP" , iTrap    , 0                },
/*18FA*/    {"TRAP" , iTrap    , 0                },
/*18FB*/    {"TRAP" , iTrap    , 0                },
/*18FC*/    {"TRAP" , iTrap    , 0                },
/*18FD*/    {"TRAP" , iTrap    , 0                },
/*18FE*/    {"TRAP" , iTrap    , 0                },
/*18FF*/    {"TRAP" , iTrap    , 0                },
};

static const struct InstrRec M68HC12_opcdTable04[] =
{
            // op       lf    ref     typ
/*0400*/    {"DBEQ" , iLoop    , 0                },
/*0401*/    {"DBEQ" , iLoop    , 0                },
/*0402*/    {""     , iInherent, 0                },
/*0403*/    {""     , iInherent, 0                },
/*0404*/    {"DBEQ" , iLoop    , 0                },
/*0405*/    {"DBEQ" , iLoop    , 0                },
/*0406*/    {"DBEQ" , iLoop    , 0                },
/*0407*/    {"DBEQ" , iLoop    , 0                },
/*0408*/    {""     , iInherent, 0                },
/*0409*/    {""     , iInherent, 0                },
/*040A*/    {""     , iInherent, 0                },
/*040B*/    {""     , iInherent, 0                },
/*040C*/    {""     , iInherent, 0                },
/*040D*/    {""     , iInherent, 0                },
/*040E*/    {""     , iInherent, 0                },
/*040F*/    {""     , iInherent, 0                },

/*0410*/    {"DBEQ" , iLoop    , 0                },
/*0411*/    {"DBEQ" , iLoop    , 0                },
/*0412*/    {""     , iInherent, 0                },
/*0413*/    {""     , iInherent, 0                },
/*0414*/    {"DBEQ" , iLoop    , 0                },
/*0415*/    {"DBEQ" , iLoop    , 0                },
/*0416*/    {"DBEQ" , iLoop    , 0                },
/*0417*/    {"DBEQ" , iLoop    , 0                },
/*0418*/    {""     , iInherent, 0                },
/*0419*/    {""     , iInherent, 0                },
/*041A*/    {""     , iInherent, 0                },
/*041B*/    {""     , iInherent, 0                },
/*041C*/    {""     , iInherent, 0                },
/*041D*/    {""     , iInherent, 0                },
/*041E*/    {""     , iInherent, 0                },
/*041F*/    {""     , iInherent, 0                },

/*0420*/    {"DBNE" , iLoop    , 0                },
/*0421*/    {"DBNE" , iLoop    , 0                },
/*0422*/    {""     , iInherent, 0                },
/*0423*/    {""     , iInherent, 0                },
/*0424*/    {"DBNE" , iLoop    , 0                },
/*0425*/    {"DBNE" , iLoop    , 0                },
/*0426*/    {"DBNE" , iLoop    , 0                },
/*0427*/    {"DBNE" , iLoop    , 0                },
/*0428*/    {""     , iInherent, 0                },
/*0429*/    {""     , iInherent, 0                },
/*042A*/    {""     , iInherent, 0                },
/*042B*/    {""     , iInherent, 0                },
/*042C*/    {""     , iInherent, 0                },
/*042D*/    {""     , iInherent, 0                },
/*042E*/    {""     , iInherent, 0                },
/*042F*/    {""     , iInherent, 0                },

/*0430*/    {"DBNE" , iLoop    , 0                },
/*0431*/    {"DBNE" , iLoop    , 0                },
/*0432*/    {""     , iInherent, 0                },
/*0433*/    {""     , iInherent, 0                },
/*0434*/    {"DBNE" , iLoop    , 0                },
/*0435*/    {"DBNE" , iLoop    , 0                },
/*0436*/    {"DBNE" , iLoop    , 0                },
/*0437*/    {"DBNE" , iLoop    , 0                },
/*0438*/    {""     , iInherent, 0                },
/*0439*/    {""     , iInherent, 0                },
/*043A*/    {""     , iInherent, 0                },
/*043B*/    {""     , iInherent, 0                },
/*043C*/    {""     , iInherent, 0                },
/*043D*/    {""     , iInherent, 0                },
/*043E*/    {""     , iInherent, 0                },
/*043F*/    {""     , iInherent, 0                },

/*0440*/    {"TBEQ" , iLoop    , 0                },
/*0441*/    {"TBEQ" , iLoop    , 0                },
/*0442*/    {""     , iInherent, 0                },
/*0443*/    {""     , iInherent, 0                },
/*0444*/    {"TBEQ" , iLoop    , 0                },
/*0445*/    {"TBEQ" , iLoop    , 0                },
/*0446*/    {"TBEQ" , iLoop    , 0                },
/*0447*/    {"TBEQ" , iLoop    , 0                },
/*0448*/    {""     , iInherent, 0                },
/*0449*/    {""     , iInherent, 0                },
/*044A*/    {""     , iInherent, 0                },
/*044B*/    {""     , iInherent, 0                },
/*044C*/    {""     , iInherent, 0                },
/*044D*/    {""     , iInherent, 0                },
/*044E*/    {""     , iInherent, 0                },
/*044F*/    {""     , iInherent, 0                },

/*0450*/    {"TBEQ" , iLoop    , 0                },
/*0451*/    {"TBEQ" , iLoop    , 0                },
/*0452*/    {""     , iInherent, 0                },
/*0453*/    {""     , iInherent, 0                },
/*0454*/    {"TBEQ" , iLoop    , 0                },
/*0455*/    {"TBEQ" , iLoop    , 0                },
/*0456*/    {"TBEQ" , iLoop    , 0                },
/*0457*/    {"TBEQ" , iLoop    , 0                },
/*0458*/    {""     , iInherent, 0                },
/*0459*/    {""     , iInherent, 0                },
/*045A*/    {""     , iInherent, 0                },
/*045B*/    {""     , iInherent, 0                },
/*045C*/    {""     , iInherent, 0                },
/*045D*/    {""     , iInherent, 0                },
/*045E*/    {""     , iInherent, 0                },
/*045F*/    {""     , iInherent, 0                },

/*0460*/    {"TBNE" , iLoop    , 0                },
/*0461*/    {"TBNE" , iLoop    , 0                },
/*0462*/    {""     , iInherent, 0                },
/*0463*/    {""     , iInherent, 0                },
/*0464*/    {"TBNE" , iLoop    , 0                },
/*0465*/    {"TBNE" , iLoop    , 0                },
/*0466*/    {"TBNE" , iLoop    , 0                },
/*0467*/    {"TBNE" , iLoop    , 0                },
/*0468*/    {""     , iInherent, 0                },
/*0469*/    {""     , iInherent, 0                },
/*046A*/    {""     , iInherent, 0                },
/*046B*/    {""     , iInherent, 0                },
/*046C*/    {""     , iInherent, 0                },
/*046D*/    {""     , iInherent, 0                },
/*046E*/    {""     , iInherent, 0                },
/*046F*/    {""     , iInherent, 0                },

/*0470*/    {"TBNE" , iLoop    , 0                },
/*0471*/    {"TBNE" , iLoop    , 0                },
/*0472*/    {""     , iInherent, 0                },
/*0473*/    {""     , iInherent, 0                },
/*0474*/    {"TBNE" , iLoop    , 0                },
/*0475*/    {"TBNE" , iLoop    , 0                },
/*0476*/    {"TBNE" , iLoop    , 0                },
/*0477*/    {"TBNE" , iLoop    , 0                },
/*0478*/    {""     , iInherent, 0                },
/*0479*/    {""     , iInherent, 0                },
/*047A*/    {""     , iInherent, 0                },
/*047B*/    {""     , iInherent, 0                },
/*047C*/    {""     , iInherent, 0                },
/*047D*/    {""     , iInherent, 0                },
/*047E*/    {""     , iInherent, 0                },
/*047F*/    {""     , iInherent, 0                },

/*0480*/    {"IBEQ" , iLoop    , 0                },
/*0481*/    {"IBEQ" , iLoop    , 0                },
/*0482*/    {""     , iInherent, 0                },
/*0483*/    {""     , iInherent, 0                },
/*0484*/    {"IBEQ" , iLoop    , 0                },
/*0485*/    {"IBEQ" , iLoop    , 0                },
/*0486*/    {"IBEQ" , iLoop    , 0                },
/*0487*/    {"IBEQ" , iLoop    , 0                },
/*0488*/    {""     , iInherent, 0                },
/*0489*/    {""     , iInherent, 0                },
/*048A*/    {""     , iInherent, 0                },
/*048B*/    {""     , iInherent, 0                },
/*048C*/    {""     , iInherent, 0                },
/*048D*/    {""     , iInherent, 0                },
/*048E*/    {""     , iInherent, 0                },
/*048F*/    {""     , iInherent, 0                },

/*0490*/    {"IBEQ" , iLoop    , 0                },
/*0491*/    {"IBEQ" , iLoop    , 0                },
/*0492*/    {""     , iInherent, 0                },
/*0493*/    {""     , iInherent, 0                },
/*0494*/    {"IBEQ" , iLoop    , 0                },
/*0495*/    {"IBEQ" , iLoop    , 0                },
/*0496*/    {"IBEQ" , iLoop    , 0                },
/*0497*/    {"IBEQ" , iLoop    , 0                },
/*0498*/    {""     , iInherent, 0                },
/*0499*/    {""     , iInherent, 0                },
/*049A*/    {""     , iInherent, 0                },
/*049B*/    {""     , iInherent, 0                },
/*049C*/    {""     , iInherent, 0                },
/*049D*/    {""     , iInherent, 0                },
/*049E*/    {""     , iInherent, 0                },
/*049F*/    {""     , iInherent, 0                },

/*04A0*/    {"IBNE" , iLoop    , 0                },
/*04A1*/    {"IBNE" , iLoop    , 0                },
/*04A2*/    {""     , iInherent, 0                },
/*04A3*/    {""     , iInherent, 0                },
/*04A4*/    {"IBNE" , iLoop    , 0                },
/*04A5*/    {"IBNE" , iLoop    , 0                },
/*04A6*/    {"IBNE" , iLoop    , 0                },
/*04A7*/    {"IBNE" , iLoop    , 0                },
/*04A8*/    {""     , iInherent, 0                },
/*04A9*/    {""     , iInherent, 0                },
/*04AA*/    {""     , iInherent, 0                },
/*04AB*/    {""     , iInherent, 0                },
/*04AC*/    {""     , iInherent, 0                },
/*04AD*/    {""     , iInherent, 0                },
/*04AE*/    {""     , iInherent, 0                },
/*04AF*/    {""     , iInherent, 0                },

/*04B0*/    {"IBNE" , iLoop    , 0                },
/*04B1*/    {"IBNE" , iLoop    , 0                },
/*04B2*/    {""     , iInherent, 0                },
/*04B3*/    {""     , iInherent, 0                },
/*04B4*/    {"IBNE" , iLoop    , 0                },
/*04B5*/    {"IBNE" , iLoop    , 0                },
/*04B6*/    {"IBNE" , iLoop    , 0                },
/*04B7*/    {"IBNE" , iLoop    , 0                },
/*04B8*/    {""     , iInherent, 0                },
/*04B9*/    {""     , iInherent, 0                },
/*04BA*/    {""     , iInherent, 0                },
/*04BB*/    {""     , iInherent, 0                },
/*04BC*/    {""     , iInherent, 0                },
/*04BD*/    {""     , iInherent, 0                },
/*04BE*/    {""     , iInherent, 0                },
/*04BF*/    {""     , iInherent, 0                },

/*04C0*/    {""     , iInherent, 0                },
/*04C1*/    {""     , iInherent, 0                },
/*04C2*/    {""     , iInherent, 0                },
/*04C3*/    {""     , iInherent, 0                },
/*04C4*/    {""     , iInherent, 0                },
/*04C5*/    {""     , iInherent, 0                },
/*04C6*/    {""     , iInherent, 0                },
/*04C7*/    {""     , iInherent, 0                },
/*04C8*/    {""     , iInherent, 0                },
/*04C9*/    {""     , iInherent, 0                },
/*04CA*/    {""     , iInherent, 0                },
/*04CB*/    {""     , iInherent, 0                },
/*04CC*/    {""     , iInherent, 0                },
/*04CD*/    {""     , iInherent, 0                },
/*04CE*/    {""     , iInherent, 0                },
/*04CF*/    {""     , iInherent, 0                },

/*04D0*/    {""     , iInherent, 0                },
/*04D1*/    {""     , iInherent, 0                },
/*04D2*/    {""     , iInherent, 0                },
/*04D3*/    {""     , iInherent, 0                },
/*04D4*/    {""     , iInherent, 0                },
/*04D5*/    {""     , iInherent, 0                },
/*04D6*/    {""     , iInherent, 0                },
/*04D7*/    {""     , iInherent, 0                },
/*04D8*/    {""     , iInherent, 0                },
/*04D9*/    {""     , iInherent, 0                },
/*04DA*/    {""     , iInherent, 0                },
/*04DB*/    {""     , iInherent, 0                },
/*04DC*/    {""     , iInherent, 0                },
/*04DD*/    {""     , iInherent, 0                },
/*04DE*/    {""     , iInherent, 0                },
/*04DF*/    {""     , iInherent, 0                },

/*04E0*/    {""     , iInherent, 0                },
/*04E1*/    {""     , iInherent, 0                },
/*04E2*/    {""     , iInherent, 0                },
/*04E3*/    {""     , iInherent, 0                },
/*04E4*/    {""     , iInherent, 0                },
/*04E5*/    {""     , iInherent, 0                },
/*04E6*/    {""     , iInherent, 0                },
/*04E7*/    {""     , iInherent, 0                },
/*04E8*/    {""     , iInherent, 0                },
/*04E9*/    {""     , iInherent, 0                },
/*04EA*/    {""     , iInherent, 0                },
/*04EB*/    {""     , iInherent, 0                },
/*04EC*/    {""     , iInherent, 0                },
/*04ED*/    {""     , iInherent, 0                },
/*04EE*/    {""     , iInherent, 0                },
/*04EF*/    {""     , iInherent, 0                },

/*04F0*/    {""     , iInherent, 0                },
/*04F1*/    {""     , iInherent, 0                },
/*04F2*/    {""     , iInherent, 0                },
/*04F3*/    {""     , iInherent, 0                },
/*04F4*/    {""     , iInherent, 0                },
/*04F5*/    {""     , iInherent, 0                },
/*04F6*/    {""     , iInherent, 0                },
/*04F7*/    {""     , iInherent, 0                },
/*04F8*/    {""     , iInherent, 0                },
/*04F9*/    {""     , iInherent, 0                },
/*04FA*/    {""     , iInherent, 0                },
/*04FB*/    {""     , iInherent, 0                },
/*04FC*/    {""     , iInherent, 0                },
/*04FD*/    {""     , iInherent, 0                },
/*04FE*/    {""     , iInherent, 0                },
/*04FF*/    {""     , iInherent, 0                },
};

static const struct InstrRec M68HC12_opcdTableX[] =
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

/*ix20*/    {"1,+X" , iInherent, 0                },
/*ix21*/    {"2,+X" , iInherent, 0                },
/*ix22*/    {"3,+X" , iInherent, 0                },
/*ix23*/    {"4,+X" , iInherent, 0                },
/*ix24*/    {"5,+X" , iInherent, 0                },
/*ix25*/    {"6,+X" , iInherent, 0                },
/*ix26*/    {"7,+X" , iInherent, 0                },
/*ix27*/    {"8,+X" , iInherent, 0                },
/*ix28*/    {"8,-X" , iInherent, 0                },
/*ix29*/    {"7,-X" , iInherent, 0                },
/*ix2A*/    {"6,-X" , iInherent, 0                },
/*ix2B*/    {"5,-X" , iInherent, 0                },
/*ix2C*/    {"4,-X" , iInherent, 0                },
/*ix2D*/    {"3,-X" , iInherent, 0                },
/*ix2E*/    {"2,-X" , iInherent, 0                },
/*ix2F*/    {"1,-X" , iInherent, 0                },

/*ix30*/    {"1,X+" , iInherent, 0                },
/*ix31*/    {"2,X+" , iInherent, 0                },
/*ix32*/    {"3,X+" , iInherent, 0                },
/*ix33*/    {"4,X+" , iInherent, 0                },
/*ix34*/    {"5,X+" , iInherent, 0                },
/*ix35*/    {"6,X+" , iInherent, 0                },
/*ix36*/    {"7,X+" , iInherent, 0                },
/*ix37*/    {"8,X+" , iInherent, 0                },
/*ix38*/    {"8,X-" , iInherent, 0                },
/*ix39*/    {"7,X-" , iInherent, 0                },
/*ix3A*/    {"6,X-" , iInherent, 0                },
/*ix3B*/    {"5,X-" , iInherent, 0                },
/*ix3C*/    {"4,X-" , iInherent, 0                },
/*ix3D*/    {"3,X-" , iInherent, 0                },
/*ix3E*/    {"2,X-" , iInherent, 0                },
/*ix3F*/    {"1,X-" , iInherent, 0                },

/*ix40*/    {"0,Y"  , iInherent, 0                },
/*ix41*/    {"1,Y"  , iInherent, 0                },
/*ix42*/    {"2,Y"  , iInherent, 0                },
/*ix43*/    {"3,Y"  , iInherent, 0                },
/*ix44*/    {"4,Y"  , iInherent, 0                },
/*ix45*/    {"5,Y"  , iInherent, 0                },
/*ix46*/    {"6,Y"  , iInherent, 0                },
/*ix47*/    {"7,Y"  , iInherent, 0                },
/*ix48*/    {"8,Y"  , iInherent, 0                },
/*ix49*/    {"9,Y"  , iInherent, 0                },
/*ix4A*/    {"10,Y" , iInherent, 0                },
/*ix4B*/    {"11,Y" , iInherent, 0                },
/*ix4C*/    {"12,Y" , iInherent, 0                },
/*ix4D*/    {"13,Y" , iInherent, 0                },
/*ix4E*/    {"14,Y" , iInherent, 0                },
/*ix4F*/    {"15,Y" , iInherent, 0                },

/*ix50*/    {"-16,Y", iInherent, 0                },
/*ix51*/    {"-15,Y", iInherent, 0                },
/*ix52*/    {"-14,Y", iInherent, 0                },
/*ix53*/    {"-13,Y", iInherent, 0                },
/*ix54*/    {"-12,Y", iInherent, 0                },
/*ix55*/    {"-11,Y", iInherent, 0                },
/*ix56*/    {"-10,Y", iInherent, 0                },
/*ix57*/    {"-9,Y" , iInherent, 0                },
/*ix58*/    {"-8,Y" , iInherent, 0                },
/*ix59*/    {"-7,Y" , iInherent, 0                },
/*ix5A*/    {"-6,Y" , iInherent, 0                },
/*ix5B*/    {"-5,Y" , iInherent, 0                },
/*ix5C*/    {"-4,Y" , iInherent, 0                },
/*ix5D*/    {"-3,Y" , iInherent, 0                },
/*ix5E*/    {"-2,Y" , iInherent, 0                },
/*ix5F*/    {"-1,Y" , iInherent, 0                },

/*ix60*/    {"1,+Y" , iInherent, 0                },
/*ix61*/    {"2,+Y" , iInherent, 0                },
/*ix62*/    {"3,+Y" , iInherent, 0                },
/*ix63*/    {"4,+Y" , iInherent, 0                },
/*ix64*/    {"5,+Y" , iInherent, 0                },
/*ix65*/    {"6,+Y" , iInherent, 0                },
/*ix66*/    {"7,+Y" , iInherent, 0                },
/*ix67*/    {"8,+Y" , iInherent, 0                },
/*ix68*/    {"8,-Y" , iInherent, 0                },
/*ix69*/    {"7,-Y" , iInherent, 0                },
/*ix6A*/    {"6,-Y" , iInherent, 0                },
/*ix6B*/    {"5,-Y" , iInherent, 0                },
/*ix6C*/    {"4,-Y" , iInherent, 0                },
/*ix6D*/    {"3,-Y" , iInherent, 0                },
/*ix6E*/    {"2,-Y" , iInherent, 0                },
/*ix6F*/    {"1,-Y" , iInherent, 0                },

/*ix70*/    {"1,Y+" , iInherent, 0                },
/*ix71*/    {"2,Y+" , iInherent, 0                },
/*ix72*/    {"3,Y+" , iInherent, 0                },
/*ix73*/    {"4,Y+" , iInherent, 0                },
/*ix74*/    {"5,Y+" , iInherent, 0                },
/*ix75*/    {"6,Y+" , iInherent, 0                },
/*ix76*/    {"7,Y+" , iInherent, 0                },
/*ix77*/    {"8,Y+" , iInherent, 0                },
/*ix78*/    {"8,Y-" , iInherent, 0                },
/*ix79*/    {"7,Y-" , iInherent, 0                },
/*ix7A*/    {"6,Y-" , iInherent, 0                },
/*ix7B*/    {"5,Y-" , iInherent, 0                },
/*ix7C*/    {"4,Y-" , iInherent, 0                },
/*ix7D*/    {"3,Y-" , iInherent, 0                },
/*ix7E*/    {"2,Y-" , iInherent, 0                },
/*ix7F*/    {"1,Y-" , iInherent, 0                },

/*ix80*/    {"0,SP" , iInherent, 0                },
/*ix81*/    {"1,SP" , iInherent, 0                },
/*ix82*/    {"2,SP" , iInherent, 0                },
/*ix83*/    {"3,SP" , iInherent, 0                },
/*ix84*/    {"4,SP" , iInherent, 0                },
/*ix85*/    {"5,SP" , iInherent, 0                },
/*ix86*/    {"6,SP" , iInherent, 0                },
/*ix87*/    {"7,SP" , iInherent, 0                },
/*ix88*/    {"8,SP" , iInherent, 0                },
/*ix89*/    {"9,SP" , iInherent, 0                },
/*ix8A*/    {"10,SP", iInherent, 0                },
/*ix8B*/    {"11,SP", iInherent, 0                },
/*ix8C*/    {"12,SP", iInherent, 0                },
/*ix8D*/    {"13,SP", iInherent, 0                },
/*ix8E*/    {"14,SP", iInherent, 0                },
/*ix8F*/    {"15,SP", iInherent, 0                },

/*ix90*/    {"-16,SP", iInherent, 0                },
/*ix91*/    {"-15,SP", iInherent, 0                },
/*ix92*/    {"-14,SP", iInherent, 0                },
/*ix93*/    {"-13,SP", iInherent, 0                },
/*ix94*/    {"-12,SP", iInherent, 0                },
/*ix95*/    {"-11,SP", iInherent, 0                },
/*ix96*/    {"-10,SP", iInherent, 0                },
/*ix97*/    {"-9,SP" , iInherent, 0                },
/*ix98*/    {"-8,SP" , iInherent, 0                },
/*ix99*/    {"-7,SP" , iInherent, 0                },
/*ix9A*/    {"-6,SP" , iInherent, 0                },
/*ix9B*/    {"-5,SP" , iInherent, 0                },
/*ix9C*/    {"-4,SP" , iInherent, 0                },
/*ix9D*/    {"-3,SP" , iInherent, 0                },
/*ix9E*/    {"-2,SP" , iInherent, 0                },
/*ix9F*/    {"-1,SP" , iInherent, 0                },

/*ixA0*/    {"1,+SP" , iInherent, 0                },
/*ixA1*/    {"2,+SP" , iInherent, 0                },
/*ixA2*/    {"3,+SP" , iInherent, 0                },
/*ixA3*/    {"4,+SP" , iInherent, 0                },
/*ixA4*/    {"5,+SP" , iInherent, 0                },
/*ixA5*/    {"6,+SP" , iInherent, 0                },
/*ixA6*/    {"7,+SP" , iInherent, 0                },
/*ixA7*/    {"8,+SP" , iInherent, 0                },
/*ixA8*/    {"8,-SP" , iInherent, 0                },
/*ixA9*/    {"7,-SP" , iInherent, 0                },
/*ixAA*/    {"6,-SP" , iInherent, 0                },
/*ixAB*/    {"5,-SP" , iInherent, 0                },
/*ixAC*/    {"4,-SP" , iInherent, 0                },
/*ixAD*/    {"3,-SP" , iInherent, 0                },
/*ixAE*/    {"2,-SP" , iInherent, 0                },
/*ixAF*/    {"1,-SP" , iInherent, 0                },

/*ixB0*/    {"1,SP+" , iInherent, 0                },
/*ixB1*/    {"2,SP+" , iInherent, 0                },
/*ixB2*/    {"3,SP+" , iInherent, 0                },
/*ixB3*/    {"4,SP+" , iInherent, 0                },
/*ixB4*/    {"5,SP+" , iInherent, 0                },
/*ixB5*/    {"6,SP+" , iInherent, 0                },
/*ixB6*/    {"7,SP+" , iInherent, 0                },
/*ixB7*/    {"8,SP+" , iInherent, 0                },
/*ixB8*/    {"8,SP-" , iInherent, 0                },
/*ixB9*/    {"7,SP-" , iInherent, 0                },
/*ixBA*/    {"6,SP-" , iInherent, 0                },
/*ixBB*/    {"5,SP-" , iInherent, 0                },
/*ixBC*/    {"4,SP-" , iInherent, 0                },
/*ixBD*/    {"3,SP-" , iInherent, 0                },
/*ixBE*/    {"2,SP-" , iInherent, 0                },
/*ixBF*/    {"1,SP-" , iInherent, 0                },

/*ixC0*/    {"0,PC"  , iInherent, 0                },
/*ixC1*/    {"1,PC"  , iInherent, 0                },
/*ixC2*/    {"2,PC"  , iInherent, 0                },
/*ixC3*/    {"3,PC"  , iInherent, 0                },
/*ixC4*/    {"4,PC"  , iInherent, 0                },
/*ixC5*/    {"5,PC"  , iInherent, 0                },
/*ixC6*/    {"6,PC"  , iInherent, 0                },
/*ixC7*/    {"7,PC"  , iInherent, 0                },
/*ixC8*/    {"8,PC"  , iInherent, 0                },
/*ixC9*/    {"9,PC"  , iInherent, 0                },
/*ixCA*/    {"10,PC" , iInherent, 0                },
/*ixCB*/    {"11,PC" , iInherent, 0                },
/*ixCC*/    {"12,PC" , iInherent, 0                },
/*ixCD*/    {"13,PC" , iInherent, 0                },
/*ixCE*/    {"14,PC" , iInherent, 0                },
/*ixCF*/    {"15,PC" , iInherent, 0                },

/*ixD0*/    {"-16,PC", iInherent, 0                },
/*ixD1*/    {"-15,PC", iInherent, 0                },
/*ixD2*/    {"-14,PC", iInherent, 0                },
/*ixD3*/    {"-13,PC", iInherent, 0                },
/*ixD4*/    {"-12,PC", iInherent, 0                },
/*ixD5*/    {"-11,PC", iInherent, 0                },
/*ixD6*/    {"-10,PC", iInherent, 0                },
/*ixD7*/    {"-9,PC" , iInherent, 0                },
/*ixD8*/    {"-8,PC" , iInherent, 0                },
/*ixD9*/    {"-7,PC" , iInherent, 0                },
/*ixDA*/    {"-6,PC" , iInherent, 0                },
/*ixDB*/    {"-5,PC" , iInherent, 0                },
/*ixDC*/    {"-4,PC" , iInherent, 0                },
/*ixDD*/    {"-3,PC" , iInherent, 0                },
/*ixDE*/    {"-2,PC" , iInherent, 0                },
/*ixDF*/    {"-1,PC" , iInherent, 0                },

/*ixE0*/    {",X"    , iImmediate, 0                }, // "n,X"
/*ixE1*/    {",X"    , iImmediate, 0                }, // "-n,X"
/*ixE2*/    {",X"    , iLImmediate,0                }, // "nn,X"
/*ixE3*/    {",X"    , iLImmediate,0                }, // "[nn,X]"
/*ixE4*/    {"A,X"   , iInherent , 0                }, // "A,X"  
/*ixE5*/    {"B,X"   , iInherent , 0                }, // "B,X"  
/*ixE6*/    {"D,X"   , iInherent , 0                }, // "D,X"  
/*ixE7*/    {"D,X"   , iInherent , 0                }, // "[D,X]"
/*ixE8*/    {",Y"    , iImmediate, 0                }, // "n,Y"
/*ixE9*/    {",Y"    , iImmediate, 0                }, // "-n,Y"
/*ixEA*/    {",Y"    , iLImmediate,0                }, // "nn,Y"
/*ixEB*/    {",Y"    , iLImmediate,0                }, // "[nn,Y]"
/*ixEC*/    {"A,Y"   , iInherent , 0                }, // "A,Y"  
/*ixED*/    {"B,Y"   , iInherent , 0                }, // "B,Y"  
/*ixEE*/    {"D,Y"   , iInherent , 0                }, // "D,Y"  
/*ixEF*/    {"D,Y"   , iInherent , 0                }, // "[D,Y]"

/*ixF0*/    {",SP"   , iImmediate, 0                }, // "n,SP"
/*ixF1*/    {",SP"   , iImmediate, 0                }, // "-n,SP"
/*ixF2*/    {",SP"   , iLImmediate,0                }, // "nn,SP"
/*ixF3*/    {",SP"   , iLImmediate,0                }, // "[nn,SP]"
/*ixF4*/    {"A,SP"  , iInherent , 0                }, // "A,SP"  
/*ixF5*/    {"B,SP"  , iInherent , 0                }, // "B,SP"  
/*ixF6*/    {"D,SP"  , iInherent , 0                }, // "D,SP"  
/*ixF7*/    {"D,SP"  , iInherent , 0                }, // "[D,SP]"
/*ixF8*/    {",PCR"  , iRelative , 0                }, // "n,PC"
/*ixF9*/    {",PCR"  , iRelative , 0                }, // "-n,PC"
/*ixFA*/    {",PCR"  , iLRelative, 0                }, // "nn,PC"
/*ixFB*/    {",PCR"  , iLRelative, 0                }, // "[nn,PC]"
/*ixFC*/    {"A,PC"  , iInherent , 0                }, // "A,PC"  
/*ixFD*/    {"B,PC"  , iInherent , 0                }, // "B,PC"  
/*ixFE*/    {"D,PC"  , iInherent , 0                }, // "D,PC"  
/*ixFF*/    {"D,PC"  , iInherent , 0                }  // "[D,PC]"
};

static const struct InstrRec tfrReg[] =
{
    {"A" , iImmediate , 0                },
    {"B" , iImmediate , 0                },
    {"CCR",iImmediate , 0                },
    {""  , iInherent  , 0                },
    {"D" , iLImmediate, 0                },
    {"X" , iLImmediate, 0                },
    {"Y" , iLImmediate, 0                },
    {"SP", iLImmediate, 0                },
};


void Dis68HC12::dis_idx(unsigned int &ad, int &len, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    InstrPtr        instr;
    int             i;
    int             r1;
    addr_t          ra;
    char            s[256];

    r1 = ReadByte(ad++);
    len++;
    instr = &M68HC12_opcdTableX[r1];
    lfref |= instr->lfref;
    if (instr->typ == iInherent && instr->op[0] == 0) {
        opcode[0] = 0;  // illegal
        len = 0;
    } else {
        switch (instr->typ & 0x7F) {
            default:
                opcode[0] = 0; // illegal
                break;

            case iInherent:
                if ((r1 & 0xE0) != 0xC0) {
                    strcpy(parms, instr->op);
                    break;
                }
                // 5-bit PC-relative
//                strcpy(parms, instr->op);
                i = r1 & 0x1F;
                if (i >= 16) {
                    i = i - 32;
                }
                ra = (ad + i) & 0xFFFF;
                RefStr(ra, s, lfref, refaddr);
                sprintf(parms, "<%s,PCR", s);
                break;

            case iImmediate:    // add $xx in front
                i = ReadByte(ad++);
                len++;
                if (r1 & 1) {
                     strcat(parms, "-");
                     i = 256 - i;
                }
                H2Str(i, s);
                strcat(parms, s);
                strcat(parms, instr->op);
                break;

            case iLImmediate:    // add $xx in front
                i = ReadWord(ad);
                ad += 2;
                len += 2;
                // note that this is a signed offset!
                if (i >= 32767) {
                     strcat(parms, "-");
                     i = 65536 - i;
                }
                H4Str(i, s);
                strcat(parms, s);
                strcat(parms, instr->op);
                break;

            case iExtended:     // add $xxxx in front
                ra = ReadWord(ad);
                ad += 2;
                len += 2;
                RefStr(ra, parms, lfref, refaddr);
                strcat(parms, instr->op);
                break;

            case iRelative:     // add PC+/-$xx
                i = ReadByte(ad++);
                len++;
                if (r1 & 1) {
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

        if ((r1 >= 0xE0) && ((r1 & 3) == 3)) {
            strcpy(s, parms);
            sprintf(parms, "[%s]", s);
        }
    }
}


int Dis68HC12::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
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
            instr = &M68HC12_opcdTable[i];
            break;

        case 0x18:
            i     = ReadByte(ad++);
            len   = 2;
            instr = &M68HC12_opcdTable18[i];
            break;

        case 0x04:
            i     = ReadByte(ad++);
            len   = 2;
            instr = &M68HC12_opcdTable04[i];
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
            if (asmMode) {
                *parms++ = '<';
                *parms = 0;
            }
            ra = ReadByte(ad++);
            RefStr2(ra, parms, lfref, refaddr);
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
            dis_idx(ad, len, opcode, parms, lfref, refaddr);
            break;

        case iExtended:
            ra = ReadWord(ad);
            ad += 2;
            len += 2;
            RefStr(ra, s, lfref, refaddr);
            strcpy(parms, s);
            break;

        case iLExtended:
            i = ReadByte(ad++);
            ra = (i << 16) + ReadWord(ad);
            ad += 2;
            len += 3;
            RefStr6(ra, s, lfref, refaddr);
            strcpy(parms, s);
            break;

        case iTfrExg:
            i = ReadByte(ad++);
            len++;
            if (i & 0x80) {
                strcpy(opcode, "EXG");
            }
            r1 = (i >> 4) & 7;
            r2 = (i & 7);
            if (
                !tfrReg[r1].op[0] || !tfrReg[r2].op[0] // if name is null
                || (i & 0x08)) {                        // if bit 3 is set
                opcode[0] = 0; // illegal
                len = 0;
            } else {
                sprintf(parms, "%s,%s", tfrReg[r1].op, tfrReg[r2].op);
            }
            break;

        case iBXRelative: // 0E/0F aaaa bb rr
        case iBIndexed:   // 0C/0D xx bb
            dis_idx(ad, len, opcode, parms, lfref, refaddr);
            parms += strlen(parms);

            i = ReadByte(ad++); // mask
            len++;
            sprintf(parms, ",#$%.2X", i);
            if (instr->typ == iBXRelative) {
do_rel:
                parms += strlen(parms);
                *parms++ = ',';
                *parms = 0;

                i = ReadByte(ad++);
                len++;
                if (i == 0xFF) {
                    // offset FF is suspicious
                    lfref |= RIPSTOP;
                }
                if (i >= 128) {
                    i = i - 256;
                }
                ra = (ad + i) & 0xFFFF;
                if (ra == addr) {
                    *parms++ = '*';
                    *parms = 0;
                } else {
                    RefStr(ra, parms, lfref, refaddr);
                }
            }
            break;

        case iBERelative: // 1E/1F aa bb rr
        case iBExtended:  // 1C/1D xx bb
            ra = ReadWord(ad); // direct addr
            ad += 2;
            i = ReadByte(ad++); // mask
            len += 3;
            RefStr(ra, parms, lfref, refaddr);
            parms += strlen(parms);
            sprintf(parms, ",#$%.2X", i);

            if (instr->typ == iBERelative) {
                goto do_rel;
            }
            break;

        case iBDRelative: // 4E/4F aa bb rr
        case iBDirect:    // 4C/4D aaaa bb
            ra = ReadByte(ad++); // direct addr
            i = ReadByte(ad++); // mask
            len += 2;
            RefStr2(ra, parms, lfref, refaddr);
            parms += strlen(parms);
            sprintf(parms, ",#$%.2X", i);

            if (instr->typ == iBDRelative) {
                goto do_rel;
            }
            break;

        case iLoop:       // 04 xx rr
            lfref |= REFFLAG | CODEREF; // simpler than setting each one
            {
                const char *reg = "";
                switch(i & 0x0F) {
                    case 0: reg = "A"; break;
                    case 1: reg = "B"; break;
                    case 4: reg = "D"; break;
                    case 5: reg = "X"; break;
                    case 6: reg = "Y"; break;
                    case 7: reg = "SP"; break;
                    default: opcode[0] = 0;  // illegal
                }
    
                if (opcode[0]) {
                    strcpy(parms, reg);
                }
            }
            parms += strlen(parms);
            *parms++ = ',';
            *parms = 0;

            r1 = ReadByte(ad++);
            len++;
            if (r1 == 0xFF) {
                // offset FF is suspicious
                lfref |= RIPSTOP;
            }
            if (i & 0x10) {
                 r1 += 256;
            }
            if (r1 >= 256) {
                r1 = r1 - 512;
            }
            ra = (ad + r1) & 0xFFFF;
            if (ra == addr) {
                *parms++ = '*';
                *parms = 0;
            } else {
                RefStr(ra, parms, lfref, refaddr);
            }

            break;

        case iMove:
            // i is second byte

            // note: immediate-to-indexed (1800 1808) has dest first!

            // i & 7 mod 3: src 0=imm 1=ext 2=idx
            switch ((i & 0x07) % 3) {
                case 0: // imm
                    if (i & 0x07) { // imm-to-ext
                        // i & 8: 0=movw 1=movb
                        if (i & 8) {
                            r1 = ReadByte(ad++);
                            len++;
                            sprintf(parms, "#$%.2X", r1);
                        } else {
                            r1 = ReadWord(ad);
                            ad += 2;
                            len += 2;
                            sprintf(parms, "#$%.4X", r1);
                        }
                    }
                    break;
                case 1: // ext
                    ra = ReadWord(ad);
                    ad += 2;
                    len += 2;
                    RefStr(ra, parms, lfref, refaddr);
                    break;
                case 2: // idx
                    dis_idx(ad, len, opcode, parms, lfref, refaddr);
                    break;
            }

            if (i & 0x07) { // not imm-to-idx
                parms += strlen(parms);
                *parms++ = ',';
                *parms   = 0;
            }

            // i & 7: dst 0-2=idx 3-6=ext
            if ((i & 0x07) < 3) {
                // idx
                dis_idx(ad, len, opcode, s, lfref, refaddr);
            } else {
                // ext
                ra = ReadWord(ad);
                ad += 2;
                len += 2;
                RefStr(ra, s, lfref, refaddr);
            }

            if (!(i & 0x07)) { // imm-to-idx
                parms += strlen(parms);
                // i & 8: 0=movw 1=movb
                if (i & 8) {
                    r1 = ReadByte(ad++);
                    len++;
                    sprintf(parms, "#$%.2X,", r1);
                } else {
                    r1 = ReadWord(ad);
                    ad += 2;
                    len += 2;
                    sprintf(parms, "#$%.4X,", r1);
                }
            }

            // now dst can go into the line
            parms += strlen(parms);
            strcat(parms, s);
            break;

        case iTrap:
            H2Str(i, parms);
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

