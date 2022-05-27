// disf8.cpp

static const char versionName[] = "Fairchild F8 disassembler";

#include "discpu.h"

class DisF8 : public CPU {
public:
    DisF8(const char *name, int subtype, int endian, int addrwid,
          char curAddrChr, char hexChr, const char *byteOp,
          const char *wordOp, const char *longOp);


    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


DisF8 cpu_F8("F8", 0, BIG_END, ADDR_16, '$', 'H', "DB", "DW", "DL");

DisF8::DisF8(const char *name, int subtype, int endian, int addrwid,
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

static const struct InstrRec F8_opcdTable[] =
{
            // op     line         lf    ref

/*00*/      {"LR",   "A,KU"    , 0                },
/*01*/      {"LR",   "A,KL"    , 0                },
/*02*/      {"LR",   "A,QU"    , 0                },
/*03*/      {"LR",   "A,QL"    , 0                },
/*04*/      {"LR",   "KU,A"    , 0                },
/*05*/      {"LR",   "KL,A"    , 0                },
/*06*/      {"LR",   "QU,A"    , 0                },
/*07*/      {"LR",   "QL,A"    , 0                },
/*08*/      {"LR",   "K,PC1"   , 0                },
/*09*/      {"LR",   "PC1,K"   , 0                },
/*0A*/      {"LR",   "A,IS"    , 0                },
/*0B*/      {"LR",   "IS,A"    , 0                },
/*0C*/      {"PK",   ""        , 0                },
/*0D*/      {"LR",   "PC0,Q"   , LFFLAG | REFFLAG },
/*0E*/      {"LR",   "Q,DC0"   , 0                },
/*0F*/      {"LR",   "DC0,Q"   , 0                },

/*10*/      {"LR",   "DC0,H"   , 0                },
/*11*/      {"LR",   "H,DC0"   , 0                },
/*12*/      {"SR",   "1"       , 0                },
/*13*/      {"SL",   "1"       , 0                },
/*14*/      {"SR",   "4"       , 0                },
/*15*/      {"SL",   "4"       , 0                },
/*16*/      {"LM",   ""        , 0                },
/*17*/      {"ST",   ""        , 0                },
/*18*/      {"COM",  ""        , 0                },
/*19*/      {"LNK",  ""        , 0                },
/*1A*/      {"DI",   ""        , 0                },
/*1B*/      {"EI",   ""        , 0                },
/*1C*/      {"POP",  ""        , LFFLAG           },
/*1D*/      {"LR",   "W,J"     , 0                },
/*1E*/      {"LR",   "J,W"     , 0                },
/*1F*/      {"INC",  ""        , 0                },

/*20*/      {"LI",   "b"       , 0                },
/*21*/      {"NI",   "b"       , 0                },
/*22*/      {"OI",   "b"       , 0                },
/*23*/      {"XI",   "b"       , 0                },
/*24*/      {"AI",   "b"       , 0                },
/*25*/      {"CI",   "b"       , 0                },
/*26*/      {"IN",   "b"       , 0                },
/*27*/      {"OUT",  "b"       , 0                },
/*28*/      {"PI",   "w"       , REFFLAG | CODEREF},
/*29*/      {"JMP",  "w"       , LFFLAG | REFFLAG | CODEREF},
/*2A*/      {"DCI",  "w"       , 0                },
/*2B*/      {"NOP",  ""        , 0                },
/*2C*/      {"XDC",  ""        , 0                },
/*2D*/      {"",     ""        , 0                },
/*2E*/      {"",     ""        , 0                },
/*2F*/      {"",     ""        , 0                },

/*30*/      {"DS",   "0"       , 0                },
/*31*/      {"DS",   "1"       , 0                },
/*32*/      {"DS",   "2"       , 0                },
/*33*/      {"DS",   "3"       , 0                },
/*34*/      {"DS",   "4"       , 0                },
/*35*/      {"DS",   "5"       , 0                },
/*36*/      {"DS",   "6"       , 0                },
/*37*/      {"DS",   "7"       , 0                },
/*38*/      {"DS",   "8"       , 0                },
/*39*/      {"DS",   "9"       , 0                },
/*3A*/      {"DS",   "10"      , 0                },
/*3B*/      {"DS",   "11"      , 0                },
/*3C*/      {"DS",   "S"       , 0                },
/*3D*/      {"DS",   "I"       , 0                },
/*3E*/      {"DS",   "D"       , 0                },
/*3F*/      {"",     ""        , 0                },

/*40*/      {"LR",   "A,0"     , 0                },
/*41*/      {"LR",   "A,1"     , 0                },
/*42*/      {"LR",   "A,2"     , 0                },
/*43*/      {"LR",   "A,3"     , 0                },
/*44*/      {"LR",   "A,4"     , 0                },
/*45*/      {"LR",   "A,5"     , 0                },
/*46*/      {"LR",   "A,6"     , 0                },
/*47*/      {"LR",   "A,7"     , 0                },
/*48*/      {"LR",   "A,8"     , 0                },
/*49*/      {"LR",   "A,9"     , 0                },
/*4A*/      {"LR",   "A,10"    , 0                },
/*4B*/      {"LR",   "A,11"    , 0                },
/*4C*/      {"LR",   "A,S"     , 0                },
/*4D*/      {"LR",   "A,I"     , 0                },
/*4E*/      {"LR",   "A,D"     , 0                },
/*4F*/      {"",     ""        , 0                },

/*50*/      {"LR",   "0,A"     , 0                },
/*51*/      {"LR",   "1,A"     , 0                },
/*52*/      {"LR",   "2,A"     , 0                },
/*53*/      {"LR",   "3,A"     , 0                },
/*54*/      {"LR",   "4,A"     , 0                },
/*55*/      {"LR",   "5,A"     , 0                },
/*56*/      {"LR",   "6,A"     , 0                },
/*57*/      {"LR",   "7,A"     , 0                },
/*58*/      {"LR",   "8,A"     , 0                },
/*59*/      {"LR",   "9,A"     , 0                },
/*5A*/      {"LR",   "10,A"    , 0                },
/*5B*/      {"LR",   "11,A"    , 0                },
/*5C*/      {"LR",   "S,A"     , 0                },
/*5D*/      {"LR",   "I,A"     , 0                },
/*5E*/      {"LR",   "D,A"     , 0                },
/*5F*/      {"",     ""        , 0                },

/*60*/      {"LISU", "0"       , 0                },
/*61*/      {"LISU", "1"       , 0                },
/*62*/      {"LISU", "2"       , 0                },
/*63*/      {"LISU", "3"       , 0                },
/*64*/      {"LISU", "4"       , 0                },
/*65*/      {"LISU", "5"       , 0                },
/*66*/      {"LISU", "6"       , 0                },
/*67*/      {"LISU", "7"       , 0                },
/*68*/      {"LISL", "0"       , 0                },
/*69*/      {"LISL", "1"       , 0                },
/*6A*/      {"LISL", "2"       , 0                },
/*6B*/      {"LISL", "3"       , 0                },
/*6C*/      {"LISL", "4"       , 0                },
/*6D*/      {"LISL", "5"       , 0                },
/*6E*/      {"LISL", "6"       , 0                },
/*6F*/      {"LISL", "7"       , 0                },

/*70*/      {"CLR",  ""        , 0                },
///*70*/    {"LIS",  "0"       , 0                },
/*71*/      {"LIS",  "1"       , 0                },
/*72*/      {"LIS",  "2"       , 0                },
/*73*/      {"LIS",  "3"       , 0                },
/*74*/      {"LIS",  "4"       , 0                },
/*75*/      {"LIS",  "5"       , 0                },
/*76*/      {"LIS",  "6"       , 0                },
/*77*/      {"LIS",  "7"       , 0                },
/*78*/      {"LIS",  "8"       , 0                },
/*79*/      {"LIS",  "9"       , 0                },
/*7A*/      {"LIS",  "10"      , 0                },
/*7B*/      {"LIS",  "11"      , 0                },
/*7C*/      {"LIS",  "12"      , 0                },
/*7D*/      {"LIS",  "13"      , 0                },
/*7E*/      {"LIS",  "14"      , 0                },
/*7F*/      {"LIS",  "15"      , 0                },

/*80*/      {"BT",   "0,r"     , REFFLAG | CODEREF},
/*81*/      {"BP",   "r"       , REFFLAG | CODEREF},
/*82*/      {"BC",   "r"       , REFFLAG | CODEREF},
/*83*/      {"BT",   "3,r"     , REFFLAG | CODEREF},
/*84*/      {"BZ",   "r"       , REFFLAG | CODEREF},
/*85*/      {"BT",   "5,r"     , REFFLAG | CODEREF},
/*86*/      {"BT",   "6,r"     , REFFLAG | CODEREF},
/*87*/      {"BT",   "7,r"     , REFFLAG | CODEREF},
/*88*/      {"AM",   ""        , 0                },
/*89*/      {"AMD",  ""        , 0                },
/*8A*/      {"NM",   ""        , 0                },
/*8B*/      {"OM",   ""        , 0                },
/*8C*/      {"XM",   ""        , 0                },
/*8D*/      {"CM",   ""        , 0                },
/*8E*/      {"ADC",  ""        , 0                },
/*8F*/      {"BR7",  "r"       , REFFLAG | CODEREF},

/*90*/      {"BR",   "r"       , LFFLAG | REFFLAG | CODEREF},
/*91*/      {"BM",   "r"       , REFFLAG | CODEREF},
/*92*/      {"BNC",  "r"       , REFFLAG | CODEREF},
/*93*/      {"BF",   "3,r"     , REFFLAG | CODEREF},
/*94*/      {"BNZ",  "r"       , REFFLAG | CODEREF},
/*95*/      {"BF",   "5,r"     , REFFLAG | CODEREF},
/*96*/      {"BF",   "6,r"     , REFFLAG | CODEREF},
/*97*/      {"BF",   "7,r"     , REFFLAG | CODEREF},
/*98*/      {"BNO",  "r"       , REFFLAG | CODEREF},
/*99*/      {"BF",   "9,r"     , REFFLAG | CODEREF},
/*9A*/      {"BF",   "10,r"    , REFFLAG | CODEREF},
/*9B*/      {"BF",   "11,r"    , REFFLAG | CODEREF},
/*9C*/      {"BF",   "12,r"    , REFFLAG | CODEREF},
/*9D*/      {"BF",   "13,r"    , REFFLAG | CODEREF},
/*9E*/      {"BF",   "14,r"    , REFFLAG | CODEREF},
/*9F*/      {"BF",   "15,r"    , REFFLAG | CODEREF},

/*A0*/      {"INS",  "0"       , 0                },
/*A1*/      {"INS",  "1"       , 0                },
/*A2*/      {"INS",  "2"       , 0                }, // illegal?
/*A3*/      {"INS",  "3"       , 0                }, // illegal?
/*A4*/      {"INS",  "4"       , 0                },
/*A5*/      {"INS",  "5"       , 0                },
/*A6*/      {"INS",  "6"       , 0                },
/*A7*/      {"INS",  "7"       , 0                },
/*A8*/      {"INS",  "8"       , 0                },
/*A9*/      {"INS",  "9"       , 0                },
/*AA*/      {"INS",  "10"      , 0                },
/*AB*/      {"INS",  "11"      , 0                },
/*AC*/      {"INS",  "12"      , 0                },
/*AD*/      {"INS",  "13"      , 0                },
/*AE*/      {"INS",  "14"      , 0                },
/*AF*/      {"INS",  "15"      , 0                },

/*B0*/      {"OUTS", "0"       , 0                },
/*B1*/      {"OUTS", "1"       , 0                },
/*B2*/      {"OUTS", "2"       , 0                }, // illegal?
/*B3*/      {"OUTS", "3"       , 0                }, // illegal?
/*B4*/      {"OUTS", "4"       , 0                },
/*B5*/      {"OUTS", "5"       , 0                },
/*B6*/      {"OUTS", "6"       , 0                },
/*B7*/      {"OUTS", "7"       , 0                },
/*B8*/      {"OUTS", "8"       , 0                },
/*B9*/      {"OUTS", "9"       , 0                },
/*BA*/      {"OUTS", "10"      , 0                },
/*BB*/      {"OUTS", "11"      , 0                },
/*BC*/      {"OUTS", "12"      , 0                },
/*BD*/      {"OUTS", "13"      , 0                },
/*BE*/      {"OUTS", "14"      , 0                },
/*BF*/      {"OUTS", "15"      , 0                },

/*C0*/      {"AS",   "0"       , 0                },
/*C1*/      {"AS",   "1"       , 0                },
/*C2*/      {"AS",   "2"       , 0                },
/*C3*/      {"AS",   "3"       , 0                },
/*C4*/      {"AS",   "4"       , 0                },
/*C5*/      {"AS",   "5"       , 0                },
/*C6*/      {"AS",   "6"       , 0                },
/*C7*/      {"AS",   "7"       , 0                },
/*C8*/      {"AS",   "8"       , 0                },
/*C9*/      {"AS",   "9"       , 0                },
/*CA*/      {"AS",   "10"      , 0                },
/*CB*/      {"AS",   "11"      , 0                },
/*CC*/      {"AS",   "S"       , 0                },
/*CD*/      {"AS",   "I"       , 0                },
/*CE*/      {"AS",   "D"       , 0                },
/*CF*/      {"",     ""        , 0                },

/*D0*/      {"ASD",  "0"       , 0                },
/*D1*/      {"ASD",  "1"       , 0                },
/*D2*/      {"ASD",  "2"       , 0                },
/*D3*/      {"ASD",  "3"       , 0                },
/*D4*/      {"ASD",  "4"       , 0                },
/*D5*/      {"ASD",  "5"       , 0                },
/*D6*/      {"ASD",  "6"       , 0                },
/*D7*/      {"ASD",  "7"       , 0                },
/*D8*/      {"ASD",  "8"       , 0                },
/*D9*/      {"ASD",  "9"       , 0                },
/*DA*/      {"ASD",  "10"      , 0                },
/*DB*/      {"ASD",  "11"      , 0                },
/*DC*/      {"ASD",  "S"       , 0                },
/*DD*/      {"ASD",  "I"       , 0                },
/*DE*/      {"ASD",  "D"       , 0                },
/*DF*/      {"",     ""        , 0                },

/*E0*/      {"XS",   "0"       , 0                },
/*E1*/      {"XS",   "1"       , 0                },
/*E2*/      {"XS",   "2"       , 0                },
/*E3*/      {"XS",   "3"       , 0                },
/*E4*/      {"XS",   "4"       , 0                },
/*E5*/      {"XS",   "5"       , 0                },
/*E6*/      {"XS",   "6"       , 0                },
/*E7*/      {"XS",   "7"       , 0                },
/*E8*/      {"XS",   "8"       , 0                },
/*E9*/      {"XS",   "9"       , 0                },
/*EA*/      {"XS",   "10"      , 0                },
/*EB*/      {"XS",   "11"      , 0                },
/*EC*/      {"XS",   "S"       , 0                },
/*ED*/      {"XS",   "I"       , 0                },
/*EE*/      {"XS",   "D"       , 0                },
/*EF*/      {"",     ""        , 0                },

/*F0*/      {"NS",   "0"       , 0                },
/*F1*/      {"NS",   "1"       , 0                },
/*F2*/      {"NS",   "2"       , 0                },
/*F3*/      {"NS",   "3"       , 0                },
/*F4*/      {"NS",   "4"       , 0                },
/*F5*/      {"NS",   "5"       , 0                },
/*F6*/      {"NS",   "6"       , 0                },
/*F7*/      {"NS",   "7"       , 0                },
/*F8*/      {"NS",   "8"       , 0                },
/*F9*/      {"NS",   "9"       , 0                },
/*FA*/      {"NS",   "10"      , 0                },
/*FB*/      {"NS",   "11"      , 0                },
/*FC*/      {"NS",   "S"       , 0                },
/*FD*/      {"NS",   "I"       , 0                },
/*FE*/      {"NS",   "D"       , 0                },
/*FF*/      {"",     ""        , 0                },
};


int DisF8::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
//  unsigned int    w;
    unsigned short  ad;
    addr_t          ra;
    int             i, opcd;
    InstrPtr        instr;
    char            *p, *l;
    char            s[256];

    ad       = addr;
    opcd     = ReadByte(ad++);
    int len  = 1;
    instr    = &F8_opcdTable[opcd];

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
                while (*p) {
                    p++;
                }
                break;

            case 'r':
                i = ReadByte(ad++);
                len++;
                if (i >= 128) {
                    i = i - 256;
                }
                ra = ((ad-1) + i) & 0xFFFF;

                RefStr4(ra, p, lfref, refaddr);
                while (*p) {
                    p++;
                }
                break;

            case 'w':   // immediate word
                ra = ReadWord(ad);
                ad += 2;
                len += 2;

                RefStr4(ra, p, lfref, refaddr);
                while (*p) {
                    p++;
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

    if (opcode[0]==0 || rom.AddrOutRange(addr)) {
        strcpy(opcode, "???");
        H2Str(ReadByte(addr), parms);
        len      = 0;
        lfref    = 0;
        refaddr  = 0;
    }

    return len;
}
