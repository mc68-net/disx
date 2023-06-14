// dis8008.cpp

static const char versionName[] = "Intel 8008 disassembler";

#include "discpu.h"

class Dis8008 : public CPU {
public:
    Dis8008(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *revwordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


enum { // cur_CPU->subtype
    CPU_8008,
};

Dis8008 cpu_8008 ("8008",  CPU_8008, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DRW", "DL");



Dis8008::Dis8008(const char *name, int subtype, int endian, int addrwid,
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


// Intel 8008 opcodes
/*static*/ const struct InstrRec I8008_opcdTable[] =
{           // op     parms            lfref
/*00*/      {"",     ""        , 0                }, // no INR A, HLT alias
/*01*/      {"",     ""        , 0                }, // no DCR A, HLT alias
/*02*/      {"RLC",  ""        , 0                },
/*03*/      {"RNC",  ""        , 0                },
/*04*/      {"ADI",  "b"       , 0                },
/*05*/      {"RST",  "0"       , 0                },
/*06*/      {"MVI",  "A,b"     , 0                },
/*07*/      {"RET",  ""        , LFFLAG           },
/*08*/      {"INR",  "B"       , 0                },
/*09*/      {"DCR",  "B"       , 0                },
/*0A*/      {"RRC",  ""        , 0                },
/*0B*/      {"RNZ",  ""        , 0                },
/*0C*/      {"ACI",  "b"       , 0                },
/*0D*/      {"RST",  "1"       , 0                },
/*0E*/      {"MVI",  "B,b"     , 0                },
/*0F*/      {"",     ""        , 0                }, // RET alias

/*10*/      {"INR",  "C"       , 0                }, 
/*11*/      {"DCR",  "C"       , 0                },
/*12*/      {"RAL",  ""        , 0                },
/*13*/      {"RP",   ""        , 0                },
/*14*/      {"SUI",  "b"       , 0                },
/*15*/      {"RST",  "2"       , 0                },
/*16*/      {"MVI",  "C,b"     , 0                },
/*17*/      {"",     ""        , 0                }, // RET alias
/*18*/      {"INR",  "D"       , 0                }, 
/*19*/      {"DCR",  "D"       , 0                },
/*1A*/      {"RAR",  ""        , 0                },
/*1B*/      {"RPO",  ""        , 0                },
/*1C*/      {"SCI",  "b"       , 0                },
/*1D*/      {"RST",  "3"       , 0                },
/*1E*/      {"MVI",  "D,b"     , 0                },
/*1F*/      {"",     ""        , 0                }, // RET alias

/*20*/      {"INR",  "E"       , 0                }, 
/*21*/      {"DCR",  "E"       , 0                },
/*22*/      {"",     ""        , 0                },
/*23*/      {"RC",   ""        , 0                },
/*24*/      {"ANI",  "b"       , 0                },
/*25*/      {"RST",  "4"       , 0                },
/*26*/      {"MVI",  "E,b"     , 0                },
/*27*/      {"",     ""        , 0                }, // RET alias
/*28*/      {"INR",  "H"       , 0                }, 
/*29*/      {"DCR",  "H"       , 0                },
/*2A*/      {"",     ""        , 0                },
/*2B*/      {"RZ",   ""        , 0                },
/*2C*/      {"XRI",  "b"       , 0                },
/*2D*/      {"RST",  "5"       , 0                },
/*2E*/      {"MVI",  "H,b"     , 0                },
/*2F*/      {"",     ""        , 0                }, // RET alias

/*30*/      {"INR",  "L"       , 0                }, 
/*31*/      {"DCR",  "L"       , 0                },
/*32*/      {"",     ""        , 0                },
/*33*/      {"RM",   ""        , 0                },
/*34*/      {"ORI",  "b"       , 0                },
/*35*/      {"RST",  "6"       , 0                },
/*36*/      {"MVI",  "L,b"     , 0                },
/*37*/      {"",     ""        , 0                }, // RET alias
/*38*/      {"",     ""        , 0                }, // no INR M
/*39*/      {"",     ""        , 0                }, // no DCR M
/*3A*/      {"",     ""        , 0                },
/*3B*/      {"RPE",  ""        , 0                },
/*3C*/      {"CPI",  "b"       , 0                },
/*3D*/      {"RST",  "7"       , 0                },
/*3E*/      {"MVI",  "M,b"     , 0                },
/*3F*/      {"",     ""        , 0                }, // RET alias

/*40*/      {"JNC",  "w"       , REFFLAG | CODEREF},
/*41*/      {"IN",   "0"       , 0                },
/*42*/      {"CNC",  "w"       , REFFLAG | CODEREF},
/*43*/      {"IN",   "1"       , 0                },
/*44*/      {"JMP",  "w"       , LFFLAG | REFFLAG | CODEREF},
/*45*/      {"IN",   "2"       , 0                },
/*46*/      {"CALL", "w"       , REFFLAG | CODEREF}, // CALL alias
/*47*/      {"IN",   "3"       , 0                },
/*48*/      {"JNZ",  "w"       , REFFLAG | CODEREF},
/*49*/      {"IN",   "4"       , 0                },
/*4A*/      {"CNZ",  "w"       , REFFLAG | CODEREF},
/*4B*/      {"IN",   "5"       , 0                },
/*4C*/      {"",     ""        , 0                }, // JMP alias
/*4D*/      {"IN",   "6"       , 0                },
/*4E*/      {"",     ""        , 0                }, // CALL alias
/*4F*/      {"IN",   "7"       , 0                },

/*50*/      {"JP",   "w"       , REFFLAG | CODEREF},
/*51*/      {"OUT",  "8"       , 0                },
/*52*/      {"CP",   "w"       , REFFLAG | CODEREF},
/*53*/      {"OUT",  "9"       , 0                },
/*54*/      {"",     ""        , 0                }, // JMP alias
/*55*/      {"OUT",  "10"      , 0                },
/*56*/      {"",     ""        , 0                }, // CALL alias
/*57*/      {"OUT",  "11"      , 0                },
/*58*/      {"JPO",  "w"       , REFFLAG | CODEREF},
/*59*/      {"OUT",  "12"      , 0                },
/*5A*/      {"CPO",  "w"       , REFFLAG | CODEREF},
/*5B*/      {"OUT",  "13"      , 0                },
/*5C*/      {"",     ""        , 0                }, // JMP alias
/*5D*/      {"OUT",  "14"      , 0                },
/*5E*/      {"",     ""        , 0                }, // CALL alias
/*5F*/      {"OUT",  "15"      , 0                },

/*60*/      {"JC",   "w"       , REFFLAG | CODEREF},
/*61*/      {"OUT",  "16"      , 0                },
/*62*/      {"CC",   "w"       , REFFLAG | CODEREF},
/*63*/      {"OUT",  "17"      , 0                },
/*64*/      {"",     ""        , 0                }, // JMP alias
/*65*/      {"OUT",  "18"      , 0                },
/*66*/      {"",     ""        , 0                }, // CALL alias
/*67*/      {"OUT",  "19"      , 0                },
/*68*/      {"JZ",   "w"       , REFFLAG | CODEREF},
/*69*/      {"OUT",  "20"      , 0                },
/*6A*/      {"CZ",   "w"       , REFFLAG | CODEREF},
/*6B*/      {"OUT",  "21"      , 0                },
/*6C*/      {"",     ""        , 0                }, // JMP alias
/*6D*/      {"OUT",  "22"      , 0                },
/*6E*/      {"",     ""        , 0                }, // CALL alias
/*6F*/      {"OUT",  "23"      , 0                },

/*70*/      {"JM",   "w"       , REFFLAG | CODEREF},
/*71*/      {"OUT",  "24"      , 0                },
/*72*/      {"CM",   "w"       , REFFLAG | CODEREF},
/*73*/      {"OUT",  "25"      , 0                },
/*74*/      {"",     ""        , 0                }, // JMP alias
/*75*/      {"OUT",  "26"      , 0                },
/*76*/      {"",     ""        , 0                }, // CALL alias
/*77*/      {"OUT",  "27"      , 0                },
/*78*/      {"JPE",  "w"       , REFFLAG | CODEREF},
/*79*/      {"OUT",  "28"      , 0                },
/*7A*/      {"CPE",  "w"       , REFFLAG | CODEREF},
/*7B*/      {"OUT",  "29"      , 0                },
/*7C*/      {"",     ""        , 0                }, // JMP alias
/*7D*/      {"OUT",  "30"      , 0                },
/*7E*/      {"",     ""        , 0                }, // CALL alias
/*7F*/      {"OUT",  "31"      , 0                },

/*80*/      {"ADD",  "A"       , 0                },
/*81*/      {"ADD",  "B"       , 0                },
/*82*/      {"ADD",  "C"       , 0                },
/*83*/      {"ADD",  "D"       , 0                },
/*84*/      {"ADD",  "E"       , 0                },
/*85*/      {"ADD",  "H"       , 0                },
/*86*/      {"ADD",  "L"       , 0                },
/*87*/      {"ADD",  "M"       , 0                },
/*88*/      {"ADC",  "A"       , 0                },
/*89*/      {"ADC",  "B"       , 0                },
/*8A*/      {"ADC",  "C"       , 0                },
/*8B*/      {"ADC",  "D"       , 0                },
/*8C*/      {"ADC",  "E"       , 0                },
/*8D*/      {"ADC",  "H"       , 0                },
/*8E*/      {"ADC",  "L"       , 0                },
/*8F*/      {"ADC",  "M"       , 0                },

/*90*/      {"SUB",  "A"       , 0                },
/*91*/      {"SUB",  "B"       , 0                },
/*92*/      {"SUB",  "C"       , 0                },
/*93*/      {"SUB",  "D"       , 0                },
/*94*/      {"SUB",  "E"       , 0                },
/*95*/      {"SUB",  "H"       , 0                },
/*96*/      {"SUB",  "L"       , 0                },
/*97*/      {"SUB",  "M"       , 0                },
/*98*/      {"SBB",  "A"       , 0                },
/*99*/      {"SBB",  "B"       , 0                },
/*9A*/      {"SBB",  "C"       , 0                },
/*9B*/      {"SBB",  "D"       , 0                },
/*9C*/      {"SBB",  "E"       , 0                },
/*9D*/      {"SBB",  "H"       , 0                },
/*9E*/      {"SBB",  "L"       , 0                },
/*9F*/      {"SBB",  "M"       , 0                },

/*A0*/      {"ANA",  "A"       , 0                },
/*A1*/      {"ANA",  "B"       , 0                },
/*A2*/      {"ANA",  "C"       , 0                },
/*A3*/      {"ANA",  "D"       , 0                },
/*A4*/      {"ANA",  "E"       , 0                },
/*A5*/      {"ANA",  "H"       , 0                },
/*A6*/      {"ANA",  "L"       , 0                },
/*A7*/      {"ANA",  "M"       , 0                },
/*A8*/      {"XRA",  "A"       , 0                },
/*A9*/      {"XRA",  "B"       , 0                },
/*AA*/      {"XRA",  "C"       , 0                },
/*AB*/      {"XRA",  "D"       , 0                },
/*AC*/      {"XRA",  "E"       , 0                },
/*AD*/      {"XRA",  "H"       , 0                },
/*AE*/      {"XRA",  "L"       , 0                },
/*AF*/      {"XRA",  "M"       , 0                },

/*B0*/      {"ORA",  "A"       , 0                },
/*B1*/      {"ORA",  "B"       , 0                },
/*B2*/      {"ORA",  "C"       , 0                },
/*B3*/      {"ORA",  "D"       , 0                },
/*B4*/      {"ORA",  "E"       , 0                },
/*B5*/      {"ORA",  "H"       , 0                },
/*B6*/      {"ORA",  "L"       , 0                },
/*B7*/      {"ORA",  "M"       , 0                },
/*B8*/      {"CMP",  "A"       , 0                },
/*B9*/      {"CMP",  "B"       , 0                },
/*BA*/      {"CMP",  "C"       , 0                },
/*BB*/      {"CMP",  "D"       , 0                },
/*BC*/      {"CMP",  "E"       , 0                },
/*BD*/      {"CMP",  "H"       , 0                },
/*BE*/      {"CMP",  "L"       , 0                },
/*BF*/      {"CMP",  "M"       , 0                },

/*C0*/      {"NOP",  ""        , 0                },
/*C1*/      {"MOV",  "A,B"     , 0                },
/*C2*/      {"MOV",  "A,C"     , 0                },
/*C3*/      {"MOV",  "A,D"     , 0                },
/*C4*/      {"MOV",  "A,E"     , 0                },
/*C5*/      {"MOV",  "A,H"     , 0                },
/*C6*/      {"MOV",  "A,L"     , 0                },
/*C7*/      {"MOV",  "A,M"     , 0                },
/*C8*/      {"MOV",  "B,A"     , 0                },
/*C9*/      {"MOV",  "B,B"     , RIPSTOP          },
/*CA*/      {"MOV",  "B,C"     , 0                },
/*CB*/      {"MOV",  "B,D"     , 0                },
/*CC*/      {"MOV",  "B,E"     , 0                },
/*CD*/      {"MOV",  "B,H"     , 0                },
/*CE*/      {"MOV",  "B,L"     , 0                },
/*CF*/      {"MOV",  "B,M"     , 0                },

/*D0*/      {"MOV",  "C,A"     , 0                },
/*D1*/      {"MOV",  "C,B"     , 0                },
/*D2*/      {"MOV",  "C,C"     , RIPSTOP          },
/*D3*/      {"MOV",  "C,D"     , 0                },
/*D4*/      {"MOV",  "C,E"     , 0                },
/*D5*/      {"MOV",  "C,H"     , 0                },
/*D6*/      {"MOV",  "C,L"     , 0                },
/*D7*/      {"MOV",  "C,M"     , 0                },
/*D8*/      {"MOV",  "D,A"     , 0                },
/*D9*/      {"MOV",  "D,B"     , 0                },
/*DA*/      {"MOV",  "D,C"     , 0                },
/*DB*/      {"MOV",  "D,D"     , RIPSTOP          },
/*DC*/      {"MOV",  "D,E"     , 0                },
/*DD*/      {"MOV",  "D,H"     , 0                },
/*DE*/      {"MOV",  "D,L"     , 0                },
/*DF*/      {"MOV",  "D,M"     , 0                },

/*E0*/      {"MOV",  "E,A"     , 0                },
/*E1*/      {"MOV",  "E,B"     , 0                },
/*E2*/      {"MOV",  "E,C"     , 0                },
/*E3*/      {"MOV",  "E,D"     , 0                },
/*E4*/      {"MOV",  "E,E"     , RIPSTOP          },
/*E5*/      {"MOV",  "E,H"     , 0                },
/*E6*/      {"MOV",  "E,L"     , 0                },
/*E7*/      {"MOV",  "E,M"     , 0                },
/*E8*/      {"MOV",  "H,A"     , 0                },
/*E9*/      {"MOV",  "H,B"     , 0                },
/*EA*/      {"MOV",  "H,C"     , 0                },
/*EB*/      {"MOV",  "H,D"     , 0                },
/*EC*/      {"MOV",  "H,E"     , 0                },
/*ED*/      {"MOV",  "H,H"     , RIPSTOP          },
/*EE*/      {"MOV",  "H,L"     , 0                },
/*EF*/      {"MOV",  "H,M"     , 0                },

/*F0*/      {"MOV",  "L,A"     , 0                },
/*F1*/      {"MOV",  "L,B"     , 0                },
/*F2*/      {"MOV",  "L,C"     , 0                },
/*F3*/      {"MOV",  "L,D"     , 0                },
/*F4*/      {"MOV",  "L,E"     , 0                },
/*F5*/      {"MOV",  "L,H"     , 0                },
/*F6*/      {"MOV",  "L,L"     , RIPSTOP          },
/*F7*/      {"MOV",  "L,M"     , 0                },
/*F8*/      {"MOV",  "M,A"     , 0                },
/*F9*/      {"MOV",  "M,B"     , 0                },
/*FA*/      {"MOV",  "M,C"     , 0                },
/*FB*/      {"MOV",  "M,D"     , 0                },
/*FC*/      {"MOV",  "M,E"     , 0                },
/*FD*/      {"MOV",  "M,H"     , 0                },
/*FE*/      {"MOV",  "M,L"     , 0                },
/*FF*/      {"HLT",  ""        , 0                }
};


int Dis8008::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    addr_t      ad;
    addr_t      ra;
    int         i;
//  int         n;
//  int         d = 0;
    InstrPtr    instr; instr = NULL;
    char        *p, *l;
    char        s[256];

    // get first byte of instruction
    ad    = addr;
    i     = ReadByte(ad++);
    int len = 1;
    instr = &I8008_opcdTable[i];

    strcpy(opcode, instr->op);
    strcpy(parms,  instr->parms);
    lfref   = instr->lfref;
    refaddr = 0;

    // special peephole meta-op override for LXI
    // this is enabled by default but can be turned off with the hint flags
    if (!(rom.get_hint(addr) & 1) && ReadByte(ad+1) == i-8) {
        char reg = 0;
        switch (i) {
            case 0x16: // LXI B,bbcc  MVI C,cc / MVI B,bb  16 cc 0E bb
                reg = 'B';
                break;
            case 0x26: // LXI D,ddee  MVI E,ee / MVI D,dd  26 ee 1E dd
                reg = 'D';
                break;
            case 0x36: // LXI H,hhll  MVI L,ll / MVI H,hh  36 ll 2E hh
                reg = 'H';
                break;
            default:
                // meh
                break;
        }

        if (reg) {
            strcpy(opcode, "LXI");
            strcpy(parms,  "r,x");
            parms[0] = reg;
        }
    }

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

            // === 16-bit word which might be an address ===
            case 'w':
                ra = ReadWord(ad);
                ad += 2;
                len += 2;

                RefStr(ra, p, lfref, refaddr);
                p += strlen(p);
                break;

            // === LXI meta-op ===
            case 'x':
                ra = ReadByte(ad++); // get low byte
                ad++; // skip second MVI
                ra = ra | (ReadByte(ad++) << 8 ); // get high byte
                len += 3;

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
#if 0 // this is from the 8080 disassembler
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
#endif
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
