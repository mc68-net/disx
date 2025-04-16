// dis2650.cpp

static const char versionName[] = "Signetics 2650 disassembler";

#include "discpu.h"

class Dis2650 : public CPU {
public:
    Dis2650(const char *name, int subtype, int endian, int addrwid,
          char curAddrChr, char hexChr, const char *byteOp,
          const char *wordOp, const char *longOp);


    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


Dis2650 cpu_2650("2650", 0, BIG_END, ADDR_16, '$', 'H', "DB", "DW", "DL");

Dis2650::Dis2650(const char *name, int subtype, int endian, int addrwid,
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

static const struct InstrRec S2650_opcdTable[] =
{
            // op       parms      lfref
/*00*/      {"LODZ,R0", ""       , 0                }, // 1 - reg
/*01*/      {"LODZ,R1", ""       , 0                },
/*02*/      {"LODZ,R2", ""       , 0                },
/*03*/      {"LODZ,R3", ""       , 0                },
/*04*/      {"LODI,R0", "i"      , 0                }, // 2 - imm
/*05*/      {"LODI,R1", "i"      , 0                },
/*06*/      {"LODI,R2", "i"      , 0                },
/*07*/      {"LODI,R3", "i"      , 0                },
/*08*/      {"LODR,R0", "s"      , 0                }, // 2 - rel
/*09*/      {"LODR,R1", "s"      , 0                },
/*0A*/      {"LODR,R2", "s"      , 0                },
/*0B*/      {"LODR,R3", "s"      , 0                },
/*0C*/      {"LODA,R0", "x"      , 0                }, // 3 - ext
/*0D*/      {"LODA,R1", "x"      , 0                },
/*0E*/      {"LODA,R2", "x"      , 0                },
/*0F*/      {"LODA,R3", "x"      , 0                },

/*10*/      {"",        ""       , 0                },
/*11*/      {"",        ""       , 0                },
/*12*/      {"SPSU",    ""       , 0                }, // 1 - 
/*13*/      {"SPSL",    ""       , 0                }, // 1 - 
/*14*/      {"RETC,EQ", ""       , 0                }, // 1 - 
/*15*/      {"RETC,GT", ""       , 0                },
/*16*/      {"RETC,LT", ""       , 0                },
/*17*/      {"RETC,UN", ""       , LFFLAG           },
/*18*/      {"BCTR,EQ", "r"      , REFFLAG | CODEREF}, // 2 - br8
/*19*/      {"BCTR,GT", "r"      , REFFLAG | CODEREF},
/*1A*/      {"BCTR,LT", "r"      , REFFLAG | CODEREF},
/*1B*/      {"BCTR,UN", "r"      , REFFLAG | CODEREF | LFFLAG},
/*1C*/      {"BCTA,EQ", "a"      , REFFLAG | CODEREF}, // 3 - br16
/*1D*/      {"BCTA,GT", "a"      , REFFLAG | CODEREF},
/*1E*/      {"BCTA,LT", "a"      , REFFLAG | CODEREF},
/*1F*/      {"BCTA,UN", "a"      , REFFLAG | CODEREF | LFFLAG},

/*20*/      {"EORZ,R0", ""       , 0                }, // 1 - reg
/*21*/      {"EORZ,R1", ""       , 0                },
/*22*/      {"EORZ,R2", ""       , 0                },
/*23*/      {"EORZ,R3", ""       , 0                },
/*24*/      {"EORI,R0", "i"      , 0                }, // 2 - imm
/*25*/      {"EORI,R1", "i"      , 0                },
/*26*/      {"EORI,R2", "i"      , 0                },
/*27*/      {"EORI,R3", "i"      , 0                },
/*28*/      {"EORR,R0", "s"      , 0                }, // 2 - rel
/*29*/      {"EORR,R1", "s"      , 0                },
/*2A*/      {"EORR,R2", "s"      , 0                },
/*2B*/      {"EORR,R3", "s"      , 0                },
/*2C*/      {"EORA,R0", "x"      , 0                }, // 3 - ext
/*2D*/      {"EORA,R1", "x"      , 0                },
/*2E*/      {"EORA,R2", "x"      , 0                },
/*2F*/      {"EORA,R3", "x"      , 0                },

/*30*/      {"REDC,R0", ""       , 0                }, // 1 - reg
/*31*/      {"REDC,R1", ""       , 0                },
/*32*/      {"REDC,R2", ""       , 0                },
/*33*/      {"REDC,R3", ""       , 0                },
/*34*/      {"RETE,EQ", ""       , 0                }, // 1 - 
/*35*/      {"RETE,GT", ""       , 0                },
/*36*/      {"RETE,LT", ""       , 0                },
/*37*/      {"RETE,UN", ""       , LFFLAG           },
/*38*/      {"BSTR,EQ", "r"      , REFFLAG | CODEREF}, // 2 - br8
/*39*/      {"BSTR,GT", "r"      , REFFLAG | CODEREF},
/*3A*/      {"BSTR,LT", "r"      , REFFLAG | CODEREF},
/*3B*/      {"BSTR,UN", "r"      , REFFLAG | CODEREF},
/*3C*/      {"BSTA,EQ", "a"      , REFFLAG | CODEREF}, // 2 - br16
/*3D*/      {"BSTA,GT", "a"      , REFFLAG | CODEREF},
/*3E*/      {"BSTA,LT", "a"      , REFFLAG | CODEREF},
/*3F*/      {"BSTA,UN", "a"      , REFFLAG | CODEREF},

/*40*/      {"HALT",    ""       , LFFLAG           }, // 1 - reg
/*41*/      {"ANDZ,R1", ""       , 0                },
/*42*/      {"ANDZ,R2", ""       , 0                },
/*43*/      {"ANDZ,R3", ""       , 0                },
/*44*/      {"ANDI,R0", "i"      , 0                }, // 2 - imm
/*45*/      {"ANDI,R1", "i"      , 0                },
/*46*/      {"ANDI,R2", "i"      , 0                },
/*47*/      {"ANDI,R3", "i"      , 0                },
/*48*/      {"ANDR,R0", "s"      , 0                }, // 2 - rel
/*49*/      {"ANDR,R1", "s"      , 0                },
/*4A*/      {"ANDR,R2", "s"      , 0                },
/*4B*/      {"ANDR,R3", "s"      , 0                },
/*4C*/      {"ANDA,R0", "x"      , 0                }, // 3 - ext
/*4D*/      {"ANDA,R1", "x"      , 0                },
/*4E*/      {"ANDA,R2", "x"      , 0                },
/*4F*/      {"ANDA,R3", "x"      , 0                },

/*50*/      {"RRR,R0",  ""       , 0                }, // 1 - reg
/*51*/      {"RRR,R1",  ""       , 0                },
/*52*/      {"RRR,R2",  ""       , 0                },
/*53*/      {"RRR,R3",  ""       , 0                },
/*54*/      {"REDE,R0", ""       , 0                },
/*55*/      {"REDE,R1", ""       , 0                },
/*56*/      {"REDE,R2", ""       , 0                },
/*57*/      {"REDE,R3", ""       , 0                },
/*58*/      {"BRNR,R0", "r"      , REFFLAG | CODEREF},
/*59*/      {"BRNR,R1", "r"      , REFFLAG | CODEREF},
/*5A*/      {"BRNR,R2", "r"      , REFFLAG | CODEREF},
/*5B*/      {"BRNR,R3", "r"      , REFFLAG | CODEREF},
/*5C*/      {"BRNA,R0", "a"      , REFFLAG | CODEREF},
/*5D*/      {"BRNA,R1", "a"      , REFFLAG | CODEREF},
/*5E*/      {"BRNA,R2", "a"      , REFFLAG | CODEREF},
/*5F*/      {"BRNA,R3", "a"      , REFFLAG | CODEREF},

/*60*/      {"IORZ,R0", ""       , 0                }, // 1 - reg
/*61*/      {"IORZ,R1", ""       , 0                },
/*62*/      {"IORZ,R2", ""       , 0                },
/*63*/      {"IORZ,R3", ""       , 0                },
/*64*/      {"IORI,R0", "i"      , 0                }, // 2 - imm
/*65*/      {"IORI,R1", "i"      , 0                },
/*66*/      {"IORI,R2", "i"      , 0                },
/*67*/      {"IORI,R3", "i"      , 0                },
/*68*/      {"IORR,R0", "s"      , 0                }, // 2 - rel
/*69*/      {"IORR,R1", "s"      , 0                },
/*6A*/      {"IORR,R2", "s"      , 0                },
/*6B*/      {"IORR,R3", "s"      , 0                },
/*6C*/      {"IORA,R0", "x"      , 0                }, // 3 - ext
/*6D*/      {"IORA,R1", "x"      , 0                },
/*6E*/      {"IORA,R2", "x"      , 0                },
/*6F*/      {"IORA,R3", "x"      , 0                },

/*70*/      {"REDD,R0", ""       , 0                }, // 1 - reg
/*71*/      {"REDD,R1", ""       , 0                },
/*72*/      {"REDD,R2", ""       , 0                },
/*73*/      {"REDD,R3", ""       , 0                },
/*74*/      {"CPSU",    "i"      , 0                }, // 2 - imm
/*75*/      {"CPSL",    "i"      , 0                },
/*76*/      {"PPSU",    "i"      , 0                },
/*77*/      {"PPSL",    "i"      , 0                },
/*78*/      {"BSNR,R0", "r"      , REFFLAG | CODEREF}, // br8
/*79*/      {"BSNR,R1", "r"      , REFFLAG | CODEREF},
/*7A*/      {"BSNR,R2", "r"      , REFFLAG | CODEREF},
/*7B*/      {"BSNR,R3", "r"      , REFFLAG | CODEREF},
/*7C*/      {"BSNA,R0", "a"      , REFFLAG | CODEREF}, // br16
/*7D*/      {"BSNA,R1", "a"      , REFFLAG | CODEREF},
/*7E*/      {"BSNA,R2", "a"      , REFFLAG | CODEREF},
/*7F*/      {"BSNA,R3", "a"      , REFFLAG | CODEREF},

/*80*/      {"ADDZ,R0", ""       , 0                }, // 1 - reg
/*81*/      {"ADDZ,R1", ""       , 0                },
/*82*/      {"ADDZ,R2", ""       , 0                },
/*83*/      {"ADDZ,R3", ""       , 0                },
/*84*/      {"ADDI,R0", "i"      , 0                }, // 2 - imm
/*85*/      {"ADDI,R1", "i"      , 0                },
/*86*/      {"ADDI,R2", "i"      , 0                },
/*87*/      {"ADDI,R3", "i"      , 0                },
/*88*/      {"ADDR,R0", "s"      , 0                }, // 2 - rel
/*89*/      {"ADDR,R1", "s"      , 0                },
/*8A*/      {"ADDR,R2", "s"      , 0                },
/*8B*/      {"ADDR,R3", "s"      , 0                },
/*8C*/      {"ADDA,R0", "x"      , 0                }, // 3 - ext
/*8D*/      {"ADDA,R1", "x"      , 0                },
/*8E*/      {"ADDA,R2", "x"      , 0                },
/*8F*/      {"ADDA,R3", "x"      , 0                },

/*90*/      {"",        ""       , 0                }, // 
/*91*/      {"",        ""       , 0                },
/*92*/      {"LPSU",    ""       , 0                }, // 1 - reg
/*93*/      {"LPSL",    ""       , 0                }, // 1 - reg
/*94*/      {"DAR,R0",  ""       , 0                }, // 1 - reg
/*95*/      {"DAR,R1",  ""       , 0                },
/*96*/      {"DAR,R2",  ""       , 0                },
/*97*/      {"DAR,R3",  ""       , 0                },
/*98*/      {"BCFR,EQ", "r"      , REFFLAG | CODEREF}, // 2 - br8
/*99*/      {"BCFR,GT", "r"      , REFFLAG | CODEREF},
/*9A*/      {"BCFR,O",  "r"      , REFFLAG | CODEREF},
/*9B*/      {"ZBRR",    "z"      , REFFLAG | CODEREF | LFFLAG}, // 2 - imm (zero page)
/*9C*/      {"BCFA,EQ", "a"      , REFFLAG | CODEREF}, // 3 - br16
/*9D*/      {"BCFA,GT", "a"      , REFFLAG | CODEREF},
/*9E*/      {"BCFA,O",  "a"      , REFFLAG | CODEREF},
/*9F*/      {"BXA,R3",  "a"      , REFFLAG | LFFLAG}, // 3 - br16

/*A0*/      {"SUBZ,R0", ""       , 0                }, // 1 - reg
/*A1*/      {"SUBZ,R1", ""       , 0                },
/*A2*/      {"SUBZ,R2", ""       , 0                },
/*A3*/      {"SUBZ,R3", ""       , 0                },
/*A4*/      {"SUBI,R0", "i"      , 0                }, // 2 - imm
/*A5*/      {"SUBI,R1", "i"      , 0                },
/*A6*/      {"SUBI,R2", "i"      , 0                },
/*A7*/      {"SUBI,R3", "i"      , 0                },
/*A8*/      {"SUBR,R0", "s"      , 0                }, // 3 - rel
/*A9*/      {"SUBR,R1", "s"      , 0                },
/*AA*/      {"SUBR,R2", "s"      , 0                },
/*AB*/      {"SUBR,R3", "s"      , 0                },
/*AC*/      {"SUBA,R0", "x"      , 0                }, // 4 - ext
/*AD*/      {"SUBA,R1", "x"      , 0                },
/*AE*/      {"SUBA,R2", "x"      , 0                },
/*AF*/      {"SUBA,R3", "x"      , 0                },

/*B0*/      {"WRTC,R0", ""       , 0                }, // 1 - reg
/*B1*/      {"WRTC,R1", ""       , 0                },
/*B2*/      {"WRTC,R2", ""       , 0                },
/*B3*/      {"WRTC,R3", ""       , 0                },
/*B4*/      {"TPSU",    "i"      , 0                }, // 2 - imm
/*B5*/      {"TPSL",    "i"      , 0                }, // 2 - imm
/*B6*/      {"",        ""       , 0                },
/*B7*/      {"",        ""       , 0                },
/*B8*/      {"BSFR,Z",  "r"      , REFFLAG | CODEREF}, // 2 - br8
/*B9*/      {"BSFR,GT", "r"      , REFFLAG | CODEREF},
/*BA*/      {"BSFR,LT", "r"      , REFFLAG | CODEREF},
/*BB*/      {"ZBSR",    "z"      , REFFLAG | CODEREF},
/*BC*/      {"BSFA,Z",  "a"      , REFFLAG | CODEREF}, // 3 - br16
/*BD*/      {"BSFA,GT", "a"      , REFFLAG | CODEREF},
/*BE*/      {"BSFA,LT", "a"      , REFFLAG | CODEREF},
/*BF*/      {"BSXA",    "a"      , REFFLAG | CODEREF},

/*C0*/      {"NOP",     ""       , 0                }, // AKA STRZ,R0"
/*C1*/      {"STRZ,R1", ""       , 0                }, // 1 - reg
/*C2*/      {"STRZ,R2", ""       , 0                },
/*C3*/      {"STRZ,R3", ""       , 0                },
/*C4*/      {"",        ""       , 0                }, // AKA STRI,Rn
/*C5*/      {"",        ""       , 0                },
/*C6*/      {"",        ""       , 0                },
/*C7*/      {"",        ""       , 0                },
/*C8*/      {"STRR,R0", "s"      , 0                }, // 2 - rel
/*C9*/      {"STRR,R1", "s"      , 0                },
/*CA*/      {"STRR,R2", "s"      , 0                },
/*CB*/      {"STRR,R3", "s"      , 0                },
/*CC*/      {"STRA,R0", "x"      , 0                }, // 3 - ext
/*CD*/      {"STRA,R1", "x"      , 0                },
/*CE*/      {"STRA,R2", "x"      , 0                },
/*CF*/      {"STRA,R3", "x"      , 0                },

/*D0*/      {"RRL,R0",  ""       , 0                }, // 1 - reg
/*D1*/      {"RRL,R1",  ""       , 0                },
/*D2*/      {"RRL,R2",  ""       , 0                },
/*D3*/      {"RRL,R3",  ""       , 0                },
/*D4*/      {"WRTE,R0", "i"      , 0                }, // 2 - imm
/*D5*/      {"WRTE,R1", "i"      , 0                },
/*D6*/      {"WRTE,R2", "i"      , 0                },
/*D7*/      {"WRTE,R3", "i"      , 0                },
/*D8*/      {"BIRR,R0", "r"      , REFFLAG | CODEREF}, // 2 - br8
/*D9*/      {"BIRR,R1", "r"      , REFFLAG | CODEREF},
/*DA*/      {"BIRR,R2", "r"      , REFFLAG | CODEREF},
/*DB*/      {"BIRR,R3", "r"      , REFFLAG | CODEREF},
/*DC*/      {"BIRA,R0", "a"      , REFFLAG | CODEREF}, // 3 - br16
/*DD*/      {"BIRA,R1", "a"      , REFFLAG | CODEREF},
/*DE*/      {"BIRA,R2", "a"      , REFFLAG | CODEREF},
/*DF*/      {"BIRA,R3", "a"      , REFFLAG | CODEREF},

/*E0*/      {"COMZ,R0", ""       , 0                }, // 1 - reg
/*E1*/      {"COMZ,R1", ""       , 0                },
/*E2*/      {"COMZ,R2", ""       , 0                },
/*E3*/      {"COMZ,R3", ""       , 0                },
/*E4*/      {"COMI,R0", "i"      , 0                }, // 2 - imm
/*E5*/      {"COMI,R1", "i"      , 0                },
/*E6*/      {"COMI,R2", "i"      , 0                },
/*E7*/      {"COMI,R3", "i"      , 0                },
/*E8*/      {"COMR,R0", "s"      , 0                }, // 2 - rel
/*E9*/      {"COMR,R1", "s"      , 0                },
/*EA*/      {"COMR,R2", "s"      , 0                },
/*EB*/      {"COMR,R3", "s"      , 0                },
/*EC*/      {"COMA,R0", "x"      , 0                }, // 3 - ext
/*ED*/      {"COMA,R1", "x"      , 0                },
/*EE*/      {"COMA,R2", "x"      , 0                },
/*EF*/      {"COMA,R3", "x"      , 0                },

/*F0*/      {"WRTD,R0", ""       , 0                }, // 1
/*F1*/      {"WRTD,R1", ""       , 0                },
/*F2*/      {"WRTD,R2", ""       , 0                },
/*F3*/      {"WRTD,R3", ""       , 0                },
/*F4*/      {"TMI,R0",  "i"      , 0                }, // 2 - imm
/*F5*/      {"TMI,R1",  "i"      , 0                },
/*F6*/      {"TMI,R2",  "i"      , 0                },
/*F7*/      {"TMI,R3",  "i"      , 0                },
/*F8*/      {"BDRR,R0", "r"      , REFFLAG | CODEREF}, // 2 - br8
/*F9*/      {"BDRR,R1", "r"      , REFFLAG | CODEREF},
/*FA*/      {"BDRR,R2", "r"      , REFFLAG | CODEREF},
/*FB*/      {"BDRR,R3", "r"      , REFFLAG | CODEREF},
/*FC*/      {"BDRA,R0", "a"      , REFFLAG | CODEREF}, // 3 - br16
/*FD*/      {"BDRA,R1", "a"      , REFFLAG | CODEREF},
/*FE*/      {"BDRA,R2", "a"      , REFFLAG | CODEREF},
/*FF*/      {"BDRA,R3", "a"      , REFFLAG | CODEREF},
};

int Dis2650::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
//  unsigned int    w;
    unsigned short  ad;
    addr_t          ra;
    int             i, opcd;
    InstrPtr        instr;
    char            *p, *l;
    char            s[256];
    bool            ind;
    int             mode;

    ad       = addr;
    opcd     = ReadByte(ad++);
    int len  = 1;
    instr    = &S2650_opcdTable[opcd];

    strcpy(opcode, instr->op);
    strcpy(parms, instr->parms);
    lfref   = instr->lfref;
    refaddr = 0;

    p = s;
    l = parms;
    while (*l) {
        switch (*l) {
            case 'z':   // zero page subroutine
            case 'i':   // immediate byte
                i = ReadByte(ad++);
                len++;

                if (*l == 'z') {
                RefStr2(i, p, lfref, refaddr);
                } else {
                    H2Str(i, p);
                }
                while (*p) {
                    p++;
                }
                break;

            case 'r':   // r = 8-bit rel branch
            case 's':   // s = 8-bit rel data
                i = ReadByte(ad++);
                len++;

                // bit 7 is indirect bit
                ind = !!(i & 0x80);
                if (ind) {
                    // indirect points to a big-endian word pointer
                    // what would be nice is to fetch this pointer
                    // and somehow make it a code reference

                    // make it a data reference
                    lfref &= ~CODEREF;
                    // '*' means it's indirect
                    *p = '*';
                    p++;
                }

                // offset is 7 bits
                i = (i & 0x7F);
                if (i >= 63) {
                    i = i - 128;
                }
                ra = (ad + i) & 0xFFFF;

                if ((*l == 'r') && (i == 0)) {
                    // handle special case of offset zero
                    *p++ = _curpc;
                    *p++ = '+';
                    *p++ = '0' + len;
                    lfref &= ~(REFFLAG | CODEREF);
                } else {
                    RefStr4(ra, p, lfref, refaddr);
                    while (*p) {
                        p++;
                    }
                }
                break;

            case 'a':   // 16-bit abs branch
            case 'x':   // 16-bit extended
                ra = ReadWord(ad);
                ad += 2;
                len += 2;

                // bit 15 is indirect bit
                ind = !!(ra & 0x8000);
                ra = ra & 0x7FFF;
                if (ind) {
                    // make it a data reference
                    lfref &= ~CODEREF;
                    // '*' means it's indirect
                    *p = '*';
                    p++;
                }

                // handle non-branch addressing
                mode = 0;
                if (*l == 'x') {
                    mode = (ra >> 13) & 0x03;
                    ra = ra & 0x1FFF;
                }

                RefStr4(ra, p, lfref, refaddr);
                while (*p) {
                    p++;
                }

                if (mode) {
                    // if indexed, move opcode reg to parm and set opcode reg to zero
                    int n = strlen(opcode);
                    if (opcode[n-3] == ',' &&
                        opcode[n-2] == 'R') {
                        *p++ = ',';
                        *p++ = 'R';
                        *p++ = opcode[n-1];
                        opcode[n-1] = '0';
                    }

                    switch (mode) {
                        default:
                        case 0: break; // not indexed
                        case 1: *p++ = ','; *p++ = '+'; break; // auto-increment
                        case 2: *p++ = ','; *p++ = '-'; break; // auto-decrement
                        case 3: break; // already completed
                    }
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
