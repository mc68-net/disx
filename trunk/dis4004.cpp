// dis4004.cpp

static const char versionName[] = "Intel 4004 disassembler";

#include "discpu.h"

class Dis4004 : public CPU {
public:
    Dis4004(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *revwordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


enum { // cur_CPU->subtype
    CPU_4004,
};

Dis4004 cpu_4004 ("4004",  CPU_4004, BIG_END, ADDR_16, '$', 'H', "DB", "DW", "DRW", "DL");



Dis4004::Dis4004(const char *name, int subtype, int endian, int addrwid,
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


// =====================================================


// Intel 4004 opcodes
/*static*/ const struct InstrRec I4004_opcdTable[] =
{           // op     parms            lfref
/*00*/      {"NOP", ""        , 0                },
/*01*/      {"",    ""        , 0                },
/*02*/      {"",    ""        , 0                },
/*03*/      {"",    ""        , 0                },
/*04*/      {"",    ""        , 0                },
/*05*/      {"",    ""        , 0                },
/*06*/      {"",    ""        , 0                },
/*07*/      {"",    ""        , 0                },
/*08*/      {"",    ""        , 0                },
/*09*/      {"",    ""        , 0                },
/*0A*/      {"",    ""        , 0                },
/*0B*/      {"",    ""        , 0                },
/*0C*/      {"",    ""        , 0                },
/*0D*/      {"",    ""        , 0                },
/*0E*/      {"",    ""        , 0                },
/*0F*/      {"",    ""        , 0                },

/*10*/      {"JCN",  "0,x"    , REFFLAG | CODEREF}, 
/*11*/      {"JNT",  "x"      , REFFLAG | CODEREF},
/*12*/      {"JC",   "x"      , REFFLAG | CODEREF},
/*13*/      {"JCN",  "3,x"    , REFFLAG | CODEREF},
/*14*/      {"JZ",   "x"      , REFFLAG | CODEREF},
/*15*/      {"JCN",  "5,x"    , REFFLAG | CODEREF},
/*16*/      {"JCN",  "6,x"    , REFFLAG | CODEREF},
/*17*/      {"JCN",  "7,x"    , REFFLAG | CODEREF},
/*18*/      {"JCN",  "8,x"    , REFFLAG | CODEREF},
/*19*/      {"JT",   "x"      , REFFLAG | CODEREF},
/*1A*/      {"JNC",  "x"      , REFFLAG | CODEREF},
/*1B*/      {"JCN",  "11,x"   , REFFLAG | CODEREF},
/*1C*/      {"JNZ",  "x"      , REFFLAG | CODEREF},
/*1D*/      {"JCN",  "13,x"   , REFFLAG | CODEREF},
/*1E*/      {"JCN",  "14,x"   , REFFLAG | CODEREF},
/*1F*/      {"JCN",  "15,x"   , REFFLAG | CODEREF},

/*20*/      {"FIM",  "p,b"    , 0                },
/*21*/      {"SRC",  "p"      , 0                },
/*22*/      {"FIM",  "p,b"    , 0                },
/*23*/      {"SRC",  "p"      , 0                },
/*24*/      {"FIM",  "p,b"    , 0                },
/*25*/      {"SRC",  "p"      , 0                },
/*26*/      {"FIM",  "p,b"    , 0                },
/*27*/      {"SRC",  "p"      , 0                },
/*28*/      {"FIM",  "p,b"    , 0                },
/*29*/      {"SRC",  "p"      , 0                },
/*2A*/      {"FIM",  "p,b"    , 0                },
/*2B*/      {"SRC",  "p"      , 0                },
/*2C*/      {"FIM",  "p,b"    , 0                },
/*2D*/      {"SRC",  "p"      , 0                },
/*2E*/      {"FIM",  "p,b"    , 0                },
/*2F*/      {"SRC",  "p"      , 0                },

/*30*/      {"FIN",  "p"      , 0                },
/*31*/      {"JIN",  "p"      , 0                },
/*32*/      {"FIN",  "p"      , 0                },
/*33*/      {"JIN",  "p"      , 0                },
/*34*/      {"FIN",  "p"      , 0                },
/*35*/      {"JIN",  "p"      , 0                },
/*36*/      {"FIN",  "p"      , 0                },
/*37*/      {"JIN",  "p"      , 0                },
/*38*/      {"FIN",  "p"      , 0                },
/*39*/      {"JIN",  "p"      , 0                },
/*3A*/      {"FIN",  "p"      , 0                },
/*3B*/      {"JIN",  "p"      , 0                },
/*3C*/      {"FIN",  "p"      , 0                },
/*3D*/      {"JIN",  "p"      , 0                },
/*3E*/      {"FIN",  "p"      , 0                },
/*3F*/      {"JIN",  "p"      , 0                },

/*40*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*41*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*42*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*43*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*44*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*45*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*46*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*47*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*48*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*49*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*4A*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*4B*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*4C*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*4D*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*4E*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},
/*4F*/      {"JUN",  "y"      , LFFLAG | REFFLAG | CODEREF},

/*50*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*51*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*52*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*53*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*54*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*55*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*56*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*57*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*58*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*59*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*5A*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*5B*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*5C*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*5D*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*5E*/      {"JMS",  "y"      , REFFLAG | CODEREF},
/*5F*/      {"JMS",  "y"      , REFFLAG | CODEREF},

/*60*/      {"INC",  "r"      , 0                },
/*61*/      {"INC",  "r"      , 0                },
/*62*/      {"INC",  "r"      , 0                },
/*63*/      {"INC",  "r"      , 0                },
/*64*/      {"INC",  "r"      , 0                },
/*65*/      {"INC",  "r"      , 0                },
/*66*/      {"INC",  "r"      , 0                },
/*67*/      {"INC",  "r"      , 0                },
/*68*/      {"INC",  "r"      , 0                },
/*69*/      {"INC",  "r"      , 0                },
/*6A*/      {"INC",  "r"      , 0                },
/*6B*/      {"INC",  "r"      , 0                },
/*6C*/      {"INC",  "r"      , 0                },
/*6D*/      {"INC",  "r"      , 0                },
/*6E*/      {"INC",  "r"      , 0                },
/*6F*/      {"INC",  "r"      , 0                },

/*70*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*71*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*72*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*73*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*74*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*75*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*76*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*77*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*78*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*79*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*7A*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*7B*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*7C*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*7D*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*7E*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},
/*7F*/      {"ISZ",  "r,x"    , REFFLAG | CODEREF},

/*80*/      {"ADD",  "r"      , 0                },
/*81*/      {"ADD",  "r"      , 0                },
/*82*/      {"ADD",  "r"      , 0                },
/*83*/      {"ADD",  "r"      , 0                },
/*84*/      {"ADD",  "r"      , 0                },
/*85*/      {"ADD",  "r"      , 0                },
/*86*/      {"ADD",  "r"      , 0                },
/*87*/      {"ADD",  "r"      , 0                },
/*88*/      {"ADD",  "r"      , 0                },
/*89*/      {"ADD",  "r"      , 0                },
/*8A*/      {"ADD",  "r"      , 0                },
/*8B*/      {"ADD",  "r"      , 0                },
/*8C*/      {"ADD",  "r"      , 0                },
/*8D*/      {"ADD",  "r"      , 0                },
/*8E*/      {"ADD",  "r"      , 0                },
/*8F*/      {"ADD",  "r"      , 0                },

/*90*/      {"SUB",  "r"      , 0                },
/*91*/      {"SUB",  "r"      , 0                },
/*92*/      {"SUB",  "r"      , 0                },
/*93*/      {"SUB",  "r"      , 0                },
/*94*/      {"SUB",  "r"      , 0                },
/*95*/      {"SUB",  "r"      , 0                },
/*96*/      {"SUB",  "r"      , 0                },
/*97*/      {"SUB",  "r"      , 0                },
/*98*/      {"SUB",  "r"      , 0                },
/*99*/      {"SUB",  "r"      , 0                },
/*9A*/      {"SUB",  "r"      , 0                },
/*9B*/      {"SUB",  "r"      , 0                },
/*9C*/      {"SUB",  "r"      , 0                },
/*9D*/      {"SUB",  "r"      , 0                },
/*9E*/      {"SUB",  "r"      , 0                },
/*9F*/      {"SUB",  "r"      , 0                },

/*A0*/      {"LD",   "r"      , 0                },
/*A1*/      {"LD",   "r"      , 0                },
/*A2*/      {"LD",   "r"      , 0                },
/*A3*/      {"LD",   "r"      , 0                },
/*A4*/      {"LD",   "r"      , 0                },
/*A5*/      {"LD",   "r"      , 0                },
/*A6*/      {"LD",   "r"      , 0                },
/*A7*/      {"LD",   "r"      , 0                },
/*A8*/      {"LD",   "r"      , 0                },
/*A9*/      {"LD",   "r"      , 0                },
/*AA*/      {"LD",   "r"      , 0                },
/*AB*/      {"LD",   "r"      , 0                },
/*AC*/      {"LD",   "r"      , 0                },
/*AD*/      {"LD",   "r"      , 0                },
/*AE*/      {"LD",   "r"      , 0                },
/*AF*/      {"LD",   "r"      , 0                },

/*B0*/      {"XCH",  "r"      , 0                },
/*B1*/      {"XCH",  "r"      , 0                },
/*B2*/      {"XCH",  "r"      , 0                },
/*B3*/      {"XCH",  "r"      , 0                },
/*B4*/      {"XCH",  "r"      , 0                },
/*B5*/      {"XCH",  "r"      , 0                },
/*B6*/      {"XCH",  "r"      , 0                },
/*B7*/      {"XCH",  "r"      , 0                },
/*B8*/      {"XCH",  "r"      , 0                },
/*B9*/      {"XCH",  "r"      , 0                },
/*BA*/      {"XCH",  "r"      , 0                },
/*BB*/      {"XCH",  "r"      , 0                },
/*BC*/      {"XCH",  "r"      , 0                },
/*BD*/      {"XCH",  "r"      , 0                },
/*BE*/      {"XCH",  "r"      , 0                },
/*BF*/      {"XCH",  "r"      , 0                },

/*C0*/      {"BBL",  "0"      , LFFLAG           },
/*C1*/      {"BBL",  "1"      , LFFLAG           },
/*C2*/      {"BBL",  "2"      , LFFLAG           },
/*C3*/      {"BBL",  "3"      , LFFLAG           },
/*C4*/      {"BBL",  "4"      , LFFLAG           },
/*C5*/      {"BBL",  "5"      , LFFLAG           },
/*C6*/      {"BBL",  "6"      , LFFLAG           },
/*C7*/      {"BBL",  "7"      , LFFLAG           },
/*C8*/      {"BBL",  "8"      , LFFLAG           },
/*C9*/      {"BBL",  "9"      , LFFLAG           },
/*CA*/      {"BBL",  "10"     , LFFLAG           },
/*CB*/      {"BBL",  "11"     , LFFLAG           },
/*CC*/      {"BBL",  "12"     , LFFLAG           },
/*CD*/      {"BBL",  "13"     , LFFLAG           },
/*CE*/      {"BBL",  "14"     , LFFLAG           },
/*CF*/      {"BBL",  "15"     , LFFLAG           },

/*D0*/      {"LDM",  "0"      , 0                },
/*D1*/      {"LDM",  "1"      , 0                },
/*D2*/      {"LDM",  "2"      , 0                },
/*D3*/      {"LDM",  "3"      , 0                },
/*D4*/      {"LDM",  "4"      , 0                },
/*D5*/      {"LDM",  "5"      , 0                },
/*D6*/      {"LDM",  "6"      , 0                },
/*D7*/      {"LDM",  "7"      , 0                },
/*D8*/      {"LDM",  "8"      , 0                },
/*D9*/      {"LDM",  "9"      , 0                },
/*DA*/      {"LDM",  "10"     , 0                },
/*DB*/      {"LDM",  "11"     , 0                },
/*DC*/      {"LDM",  "12"     , 0                },
/*DD*/      {"LDM",  "13"     , 0                },
/*DE*/      {"LDM",  "14"     , 0                },
/*DF*/      {"LDM",  "15"     , 0                },

/*E0*/      {"WRM",  ""       , 0                },
/*E1*/      {"WMP",  ""       , 0                },
/*E2*/      {"WRR",  ""       , 0                },
/*E3*/      {"WPM",  ""       , 0                },
/*E4*/      {"WR0",  ""       , 0                },
/*E5*/      {"WR1",  ""       , 0                },
/*E6*/      {"WR2",  ""       , 0                },
/*E7*/      {"WR3",  ""       , 0                },
/*E8*/      {"SBM",  ""       , 0                },
/*E9*/      {"RDM",  ""       , 0                },
/*EA*/      {"RDR",  ""       , 0                },
/*EB*/      {"ADM",  ""       , 0                },
/*EC*/      {"RD0",  ""       , 0                },
/*ED*/      {"RD1",  ""       , 0                },
/*EE*/      {"RD2",  ""       , 0                },
/*EF*/      {"RD3",  ""       , 0                },

/*F0*/      {"CLB",  ""       , 0                },
/*F1*/      {"CLC",  ""       , 0                },
/*F2*/      {"IAC",  ""       , 0                },
/*F3*/      {"CMC",  ""       , 0                },
/*F4*/      {"CMA",  ""       , 0                },
/*F5*/      {"RAL",  ""       , 0                },
/*F6*/      {"RAR",  ""       , 0                },
/*F7*/      {"TCC",  ""       , 0                },
/*F8*/      {"DAC",  ""       , 0                },
/*F9*/      {"TCS",  ""       , 0                },
/*FA*/      {"STC",  ""       , 0                },
/*FB*/      {"DAA",  ""       , 0                },
/*FC*/      {"KBP",  ""       , 0                },
/*FD*/      {"DCL",  ""       , 0                },
/*FE*/      {"",     ""       , 0                },
/*FF*/      {"",     ""       , 0                }
};


int Dis4004::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    addr_t      ad;
    addr_t      ra;
    int         i;
    int         op;
//  int         n;
//  int         d = 0;
    InstrPtr    instr; instr = NULL;
    char        *p, *l;
    char        s[256];

    // get first byte of instruction
    ad    = addr;
    op    = ReadByte(ad++);
    int len = 1;
    instr = &I4004_opcdTable[op];

    strcpy(opcode, instr->op);
    strcpy(parms,  instr->parms);
    lfref   = instr->lfref;
    refaddr = 0;

    // handle substitutions
    p = s;
    l = parms;
    while (*l) {
        switch (*l) {
            // === 8-bit byte ===
            case 'b':
                i = ReadByte(ad++);
                len++;

                H2Str(i, p);
                p += strlen(p);
                break;

            // register
            case 'r':
#if 1
                sprintf(p, "%d", op & 15);
#else
                sprintf(p, "R%d", op & 15);
#endif
                p += strlen(p);
                break;

            // register pair
            case 'p':
#if 1
                sprintf(p, "%d", (op & 0x0E) >> 1);
#else
                sprintf(p, "P%d", (op & 0x0E) >> 1);
#endif
                p += strlen(p);
                break;

            // 8-bit address (ISZ)
            case 'x':
                ra = ReadByte(ad++); // get low byte
                ra = ra | (ad & 0xFF00); // merge in PC high byte
                len++;

                RefStr(ra, p, lfref, refaddr);
                p += strlen(p);
                break;

            // 12-bit address
            case 'y':
                ra = (op & 15) << 8; // get high byte from opcode
                ra = ra | ReadByte(ad++); // get low byte
                len++;

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

    // rip-stop checks
    if (opcode[0]) {
        // find the previous instruction for this CPU
        addr_t prev = find_prev_instr(addr);
        if (prev) {
            int op = ReadByte(addr);
            switch (op) {
                case 0x00: // three NOP in a row
                    if (ReadByte(prev) == op) {
                        prev = find_prev_instr(prev);
                        if (ReadByte(prev) == op) {
                            lfref |= RIPSTOP;
                        }
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
