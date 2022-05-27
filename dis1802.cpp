// dis1802.cpp

static const char versionName[] = "RCA 1802 disassembler";

#include "discpu.h"

class Dis1802 : public CPU {
public:
    Dis1802(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


Dis1802 cpu_1802("1802", 0, BIG_END, ADDR_16, '$', 'H', "DB", "DW", "DL"); // LIST_24, M68K_opcdTab);


Dis1802::Dis1802(const char *name, int subtype, int endian, int addrwid,
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

static const struct InstrRec RCA1802_opcdTable[] =
{
            // op     line           lfref

/*00*/      {"IDL",  ""        , 0                },
/*01*/      {"LDN",  "R1"      , 0                },
/*02*/      {"LDN",  "R2"      , 0                },
/*03*/      {"LDN",  "R3"      , 0                },
/*04*/      {"LDN",  "R4"      , 0                },
/*05*/      {"LDN",  "R5"      , 0                },
/*06*/      {"LDN",  "R6"      , 0                },
/*07*/      {"LDN",  "R7"      , 0                },
/*08*/      {"LDN",  "R8"      , 0                },
/*09*/      {"LDN",  "R9"      , 0                },
/*0A*/      {"LDN",  "RA"      , 0                },
/*0B*/      {"LDN",  "RB"      , 0                },
/*0C*/      {"LDN",  "RC"      , 0                },
/*0D*/      {"LDN",  "RD"      , 0                },
/*0E*/      {"LDN",  "RE"      , 0                },
/*0F*/      {"LDN",  "RF"      , 0                },

/*10*/      {"INC",  "R0"      , 0                },
/*11*/      {"INC",  "R1"      , 0                },
/*12*/      {"INC",  "R2"      , 0                },
/*13*/      {"INC",  "R3"      , 0                },
/*14*/      {"INC",  "R4"      , 0                },
/*15*/      {"INC",  "R5"      , 0                },
/*16*/      {"INC",  "R6"      , 0                },
/*17*/      {"INC",  "R7"      , 0                },
/*18*/      {"INC",  "R8"      , 0                },
/*19*/      {"INC",  "R9"      , 0                },
/*1A*/      {"INC",  "RA"      , 0                },
/*1B*/      {"INC",  "RB"      , 0                },
/*1C*/      {"INC",  "RC"      , 0                },
/*1D*/      {"INC",  "RD"      , 0                },
/*1E*/      {"INC",  "RE"      , 0                },
/*1F*/      {"INC",  "RF"      , 0                },

/*20*/      {"DEC",  "R0"      , 0                },
/*21*/      {"DEC",  "R1"      , 0                },
/*22*/      {"DEC",  "R2"      , 0                },
/*23*/      {"DEC",  "R3"      , 0                },
/*24*/      {"DEC",  "R4"      , 0                },
/*25*/      {"DEC",  "R5"      , 0                },
/*26*/      {"DEC",  "R6"      , 0                },
/*27*/      {"DEC",  "R7"      , 0                },
/*28*/      {"DEC",  "R8"      , 0                },
/*29*/      {"DEC",  "R9"      , 0                },
/*2A*/      {"DEC",  "RA"      , 0                },
/*2B*/      {"DEC",  "RB"      , 0                },
/*2C*/      {"DEC",  "RC"      , 0                },
/*2D*/      {"DEC",  "RD"      , 0                },
/*2E*/      {"DEC",  "RE"      , 0                },
/*2F*/      {"DEC",  "RF"      , 0                },

/*30*/      {"BR",   "j"       , LFFLAG | REFFLAG | CODEREF},
/*31*/      {"BQ",   "j"       , REFFLAG | CODEREF},
/*32*/      {"BZ",   "j"       , REFFLAG | CODEREF},
/*33*/      {"BDF",  "j"       , REFFLAG | CODEREF},
/*34*/      {"B1",   "j"       , REFFLAG | CODEREF},
/*35*/      {"B2",   "j"       , REFFLAG | CODEREF},
/*36*/      {"B3",   "j"       , REFFLAG | CODEREF},
/*37*/      {"B4",   "j"       , REFFLAG | CODEREF},
/*38*/      {"SKP",  ""        , 0                }, // skips one byte (esentially a BRN)
/*39*/      {"BNQ",  "j"       , REFFLAG | CODEREF},
/*3A*/      {"BNZ",  "j"       , REFFLAG | CODEREF},
/*3B*/      {"BNF",  "j"       , REFFLAG | CODEREF},
/*3C*/      {"BN1",  "j"       , REFFLAG | CODEREF},
/*3D*/      {"BN2",  "j"       , REFFLAG | CODEREF},
/*3E*/      {"BN3",  "j"       , REFFLAG | CODEREF},
/*3F*/      {"BN4",  "j"       , REFFLAG | CODEREF},

/*40*/      {"LDA",  "R0"      , 0                },
/*41*/      {"LDA",  "R1"      , 0                },
/*42*/      {"LDA",  "R2"      , 0                },
/*43*/      {"LDA",  "R3"      , 0                },
/*44*/      {"LDA",  "R4"      , 0                },
/*45*/      {"LDA",  "R5"      , 0                },
/*46*/      {"LDA",  "R6"      , 0                },
/*47*/      {"LDA",  "R7"      , 0                },
/*48*/      {"LDA",  "R8"      , 0                },
/*49*/      {"LDA",  "R9"      , 0                },
/*4A*/      {"LDA",  "RA"      , 0                },
/*4B*/      {"LDA",  "RB"      , 0                },
/*4C*/      {"LDA",  "RC"      , 0                },
/*4D*/      {"LDA",  "RD"      , 0                },
/*4E*/      {"LDA",  "RE"      , 0                },
/*4F*/      {"LDA",  "RF"      , 0                },

/*50*/      {"STR",  "R0"      , 0                },
/*51*/      {"STR",  "R1"      , 0                },
/*52*/      {"STR",  "R2"      , 0                },
/*53*/      {"STR",  "R3"      , 0                },
/*54*/      {"STR",  "R4"      , 0                },
/*55*/      {"STR",  "R5"      , 0                },
/*56*/      {"STR",  "R6"      , 0                },
/*57*/      {"STR",  "R7"      , 0                },
/*58*/      {"STR",  "R8"      , 0                },
/*59*/      {"STR",  "R9"      , 0                },
/*5A*/      {"STR",  "RA"      , 0                },
/*5B*/      {"STR",  "RB"      , 0                },
/*5C*/      {"STR",  "RC"      , 0                },
/*5D*/      {"STR",  "RD"      , 0                },
/*5E*/      {"STR",  "RE"      , 0                },
/*5F*/      {"STR",  "RF"      , 0                },

/*60*/      {"IRX",  ""        , 0                },
/*61*/      {"OUT",  "1"       , 0                },
/*62*/      {"OUT",  "2"       , 0                },
/*63*/      {"OUT",  "3"       , 0                },
/*64*/      {"OUT",  "4"       , 0                },
/*65*/      {"OUT",  "5"       , 0                },
/*66*/      {"OUT",  "6"       , 0                },
/*67*/      {"OUT",  "7"       , 0                },
/*68*/      {"",     ""        , 0                },
/*69*/      {"INP",  "1"       , 0                },
/*6A*/      {"INP",  "2"       , 0                },
/*6B*/      {"INP",  "3"       , 0                },
/*6C*/      {"INP",  "4"       , 0                },
/*6D*/      {"INP",  "5"       , 0                },
/*6E*/      {"INP",  "6"       , 0                },
/*6F*/      {"INP",  "7"       , 0                },

/*70*/      {"RET",  ""        , LFFLAG           },
/*71*/      {"DIS",  ""        , LFFLAG           },
/*72*/      {"LDXA", ""        , 0                },
/*73*/      {"STXD", ""        , 0                },
/*74*/      {"ADC",  ""        , 0                },
/*75*/      {"SDB",  ""        , 0                },
/*76*/      {"SHRC", ""        , 0                },
/*77*/      {"SMB",  ""        , 0                },
/*78*/      {"SAV",  ""        , 0                },
/*79*/      {"MARK", ""        , 0                },
/*7A*/      {"REQ",  ""        , 0                },
/*7B*/      {"SEQ",  ""        , 0                },
/*7C*/      {"ADCI", "b"       , 0                },
/*7D*/      {"SDBI", "b"       , 0                },
/*7E*/      {"SHLC", ""        , 0                },
/*7F*/      {"SMBI", "b"       , 0                },

/*80*/      {"GLO",  "R0"      , 0                },
/*81*/      {"GLO",  "R1"      , 0                },
/*82*/      {"GLO",  "R2"      , 0                },
/*83*/      {"GLO",  "R3"      , 0                },
/*84*/      {"GLO",  "R4"      , 0                },
/*85*/      {"GLO",  "R5"      , 0                },
/*86*/      {"GLO",  "R6"      , 0                },
/*87*/      {"GLO",  "R7"      , 0                },
/*88*/      {"GLO",  "R8"      , 0                },
/*89*/      {"GLO",  "R9"      , 0                },
/*8A*/      {"GLO",  "RA"      , 0                },
/*8B*/      {"GLO",  "RB"      , 0                },
/*8C*/      {"GLO",  "RC"      , 0                },
/*8D*/      {"GLO",  "RD"      , 0                },
/*8E*/      {"GLO",  "RE"      , 0                },
/*8F*/      {"GLO",  "RF"      , 0                },

/*90*/      {"GHI",  "R0"      , 0                },
/*91*/      {"GHI",  "R1"      , 0                },
/*92*/      {"GHI",  "R2"      , 0                },
/*93*/      {"GHI",  "R3"      , 0                },
/*94*/      {"GHI",  "R4"      , 0                },
/*95*/      {"GHI",  "R5"      , 0                },
/*96*/      {"GHI",  "R6"      , 0                },
/*97*/      {"GHI",  "R7"      , 0                },
/*98*/      {"GHI",  "R8"      , 0                },
/*99*/      {"GHI",  "R9"      , 0                },
/*9A*/      {"GHI",  "RA"      , 0                },
/*9B*/      {"GHI",  "RB"      , 0                },
/*9C*/      {"GHI",  "RC"      , 0                },
/*9D*/      {"GHI",  "RD"      , 0                },
/*9E*/      {"GHI",  "RE"      , 0                },
/*9F*/      {"GHI",  "RF"      , 0                },

/*A0*/      {"PLO",  "R0"      , 0                },
/*A1*/      {"PLO",  "R1"      , 0                },
/*A2*/      {"PLO",  "R2"      , 0                },
/*A3*/      {"PLO",  "R3"      , 0                },
/*A4*/      {"PLO",  "R4"      , 0                },
/*A5*/      {"PLO",  "R5"      , 0                },
/*A6*/      {"PLO",  "R6"      , 0                },
/*A7*/      {"PLO",  "R7"      , 0                },
/*A8*/      {"PLO",  "R8"      , 0                },
/*A9*/      {"PLO",  "R9"      , 0                },
/*AA*/      {"PLO",  "RA"      , 0                },
/*AB*/      {"PLO",  "RB"      , 0                },
/*AC*/      {"PLO",  "RC"      , 0                },
/*AD*/      {"PLO",  "RD"      , 0                },
/*AE*/      {"PLO",  "RE"      , 0                },
/*AF*/      {"PLO",  "RF"      , 0                },

/*B0*/      {"PHI",  "R0"      , 0                },
/*B1*/      {"PHI",  "R1"      , 0                },
/*B2*/      {"PHI",  "R2"      , 0                },
/*B3*/      {"PHI",  "R3"      , 0                },
/*B4*/      {"PHI",  "R4"      , 0                },
/*B5*/      {"PHI",  "R5"      , 0                },
/*B6*/      {"PHI",  "R6"      , 0                },
/*B7*/      {"PHI",  "R7"      , 0                },
/*B8*/      {"PHI",  "R8"      , 0                },
/*B9*/      {"PHI",  "R9"      , 0                },
/*BA*/      {"PHI",  "RA"      , 0                },
/*BB*/      {"PHI",  "RB"      , 0                },
/*BC*/      {"PHI",  "RC"      , 0                },
/*BD*/      {"PHI",  "RD"      , 0                },
/*BE*/      {"PHI",  "RE"      , 0                },
/*BF*/      {"PHI",  "RF"      , 0                },

/*C0*/      {"LBR",  "w"       , LFFLAG | REFFLAG | CODEREF},
/*C1*/      {"LBQ",  "w"       , REFFLAG | CODEREF},
/*C2*/      {"LBZ",  "w"       , REFFLAG | CODEREF},
/*C3*/      {"LBDF", "w"       , REFFLAG | CODEREF},
/*C4*/      {"NOP",  ""        , 0                },
/*C5*/      {"LSNQ", ""        , 0                }, // lskip
/*C6*/      {"LSNZ", ""        , 0                }, // lskip
/*C7*/      {"LSNF", ""        , 0                }, // lskip
/*C8*/      {"LSKP", ""        , 0                }, // lskip
/*C9*/      {"LBNQ", "w"       , REFFLAG | CODEREF},
/*CA*/      {"LBNZ", "w"       , REFFLAG | CODEREF},
/*CB*/      {"LBNF", "w"       , REFFLAG | CODEREF},
/*CC*/      {"LSIE", ""        , 0                }, // lskip
/*CD*/      {"LSQ",  ""        , 0                }, // lskip
/*CE*/      {"LSZ",  ""        , 0                }, // lskip
/*CF*/      {"LSDF", ""        , 0                }, // lskip

/*D0*/      {"SEP",  "R0"      , LFFLAG           },
/*D1*/      {"SEP",  "R1"      , LFFLAG           },
/*D2*/      {"SEP",  "R2"      , LFFLAG           },
/*D3*/      {"SEP",  "R3"      , LFFLAG           },
/*D4*/      {"SEP",  "R4"      , LFFLAG           },
/*D5*/      {"SEP",  "R5"      , LFFLAG           },
/*D6*/      {"SEP",  "R6"      , LFFLAG           },
/*D7*/      {"SEP",  "R7"      , LFFLAG           },
/*D8*/      {"SEP",  "R8"      , LFFLAG           },
/*D9*/      {"SEP",  "R9"      , LFFLAG           },
/*DA*/      {"SEP",  "RA"      , LFFLAG           },
/*DB*/      {"SEP",  "RB"      , LFFLAG           },
/*DC*/      {"SEP",  "RC"      , LFFLAG           },
/*DD*/      {"SEP",  "RD"      , LFFLAG           },
/*DE*/      {"SEP",  "RE"      , LFFLAG           },
/*DF*/      {"SEP",  "RF"      , LFFLAG           },

/*E0*/      {"SEX",  "R0"      , 0                },
/*E1*/      {"SEX",  "R1"      , 0                },
/*E2*/      {"SEX",  "R2"      , 0                },
/*E3*/      {"SEX",  "R3"      , 0                },
/*E4*/      {"SEX",  "R4"      , 0                },
/*E5*/      {"SEX",  "R5"      , 0                },
/*E6*/      {"SEX",  "R6"      , 0                },
/*E7*/      {"SEX",  "R7"      , 0                },
/*E8*/      {"SEX",  "R8"      , 0                },
/*E9*/      {"SEX",  "R9"      , 0                },
/*EA*/      {"SEX",  "RA"      , 0                },
/*EB*/      {"SEX",  "RB"      , 0                },
/*EC*/      {"SEX",  "RC"      , 0                },
/*ED*/      {"SEX",  "RD"      , 0                },
/*EE*/      {"SEX",  "RE"      , 0                },
/*EF*/      {"SEX",  "RF"      , 0                },

/*F0*/      {"LDX", ""         , 0                },
/*F1*/      {"OR",  ""         , 0                },
/*F2*/      {"AND", ""         , 0                },
/*F3*/      {"XOR", ""         , 0                },
/*F4*/      {"ADD", ""         , 0                },
/*F5*/      {"SD",  ""         , 0                },
/*F6*/      {"SHR", ""         , 0                },
/*F7*/      {"SM",  ""         , 0                },
/*F8*/      {"LDI", "b"        , 0                },
/*F9*/      {"ORI", "b"        , 0                },
/*FA*/      {"ANI", "b"        , 0                },
/*FB*/      {"XRI", "b"        , 0                },
/*FC*/      {"ADI", "b"        , 0                },
/*FD*/      {"SDI", "b"        , 0                },
/*FE*/      {"SHL", ""         , 0                },
/*FF*/      {"SMI", "b"        , 0                },
};


int Dis1802::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    unsigned short  ad;
    int             i, opcd;
    InstrPtr        instr;
    char            *p, *l;
    char            s[256];
    addr_t          ra;

    ad    = addr;
    opcd = ReadByte(ad++);
    int len = 1;
    instr = &RCA1802_opcdTable[opcd];

    strcpy(opcode, instr->op);
    strcpy(parms, instr->parms);
    lfref   = instr->lfref;
    refaddr = 0;

    p = s;
    l = parms;
    while (*l) {
        switch (*l) {
            case 'b':   // immediate byte
                i = ReadByte(ad++);
                len++;

                H2Str(i, p);
                p += strlen(p);
                break;

            case 'j':   // short jump
                ra = ReadByte(ad++);
                len++;

                ra += ad & 0xFF00;
                RefStr(ra, p, lfref, refaddr);
                p += strlen(p);
                break;

            case 'w':   // immediate word
                ra = ReadWord(ad);
                ad += 2;
                len += 2;

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
