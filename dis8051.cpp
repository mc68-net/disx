// disz8051.cpp

static const char versionName[] = "Intel 8051 disassembler";

// enable 8051 register names (this needs to be a runtime config or a comment field)
#define USEREG 1 // 0 to disable register names, 1 to enable

#include "discpu.h"

class Dis8051 : public CPU {
public:
    Dis8051(const char *name, int subtype, int endian, int addrwid,
            char curAddrChr, char hexChr, const char *byteOp,
            const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


Dis8051 cpu_8051("8051", 0, BIG_END, ADDR_16, '$', 'H', "DB", "DW", "DL");
Dis8051 cpu_8032("8032", 0, BIG_END, ADDR_16, '$', 'H', "DB", "DW", "DL");


Dis8051::Dis8051(const char *name, int subtype, int endian, int addrwid,
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


#define maxOpcdLen 5    // length in bytes of longest opcode
typedef char OpcdStr[maxOpcdLen+1];
struct InstrRec {
    const char      *op;    // mnemonic
    const char      *parms; // parms
    uint8_t         lfref;  // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;


static const struct InstrRec I8051_opcdTable[] =
{
            // op     line         lf    ref
/*00*/      {"NOP",  ""        , 0                },
/*01*/      {"AJMP", "a"       , LFFLAG | REFFLAG | CODEREF},
/*02*/      {"LJMP", "w"       , LFFLAG | REFFLAG | CODEREF},
/*03*/      {"RR",   "A"       , 0                },
/*04*/      {"INC",  "A"       , 0                },
/*05*/      {"INC",  "b"       , 0                },
/*06*/      {"INC",  "@R0"     , 0                },
/*07*/      {"INC",  "@R1"     , 0                },

/*08*/      {"INC",  "R0"      , 0                },
/*09*/      {"INC",  "R1"      , 0                },
/*0A*/      {"INC",  "R2"      , 0                },
/*0B*/      {"INC",  "R3"      , 0                },
/*0C*/      {"INC",  "R4"      , 0                },
/*0D*/      {"INC",  "R5"      , 0                },
/*0E*/      {"INC",  "R6"      , 0                },
/*0F*/      {"INC",  "R7"      , 0                },

/*10*/      {"JBC",  "t,r"     , REFFLAG | CODEREF},
/*11*/      {"ACALL","a"       , REFFLAG | CODEREF},
/*12*/      {"LCALL","w"       , REFFLAG | CODEREF},
/*13*/      {"RRC",  "A"       , 0                },
/*14*/      {"DEC",  "A"       , 0                },
/*15*/      {"DEC",  "b"       , 0                },
/*16*/      {"DEC",  "@R0"     , 0                },
/*17*/      {"DEC",  "@R1"     , 0                },

/*18*/      {"DEC",  "R0"      , 0                },
/*19*/      {"DEC",  "R1"      , 0                },
/*1A*/      {"DEC",  "R2"      , 0                },
/*1B*/      {"DEC",  "R3"      , 0                },
/*1C*/      {"DEC",  "R4"      , 0                },
/*1D*/      {"DEC",  "R5"      , 0                },
/*1E*/      {"DEC",  "R6"      , 0                },
/*1F*/      {"DEC",  "R7"      , 0                },

/*20*/      {"JB",   "t,r"     , REFFLAG | CODEREF},
/*21*/      {"AJMP", "a"       , LFFLAG | REFFLAG | CODEREF},
/*22*/      {"RET",  ""        , LFFLAG           },
/*23*/      {"RL",   "A"       , 0                },
/*24*/      {"ADD",  "A,#i"    , 0                },
/*25*/      {"ADD",  "A,b"     , 0                },
/*26*/      {"ADD",  "A,@R0"   , 0                },
/*27*/      {"ADD",  "A,@R1"   , 0                },

/*28*/      {"ADD",  "A,R0"    , 0                },
/*29*/      {"ADD",  "A,R1"    , 0                },
/*2A*/      {"ADD",  "A,R2"    , 0                },
/*2B*/      {"ADD",  "A,R3"    , 0                },
/*2C*/      {"ADD",  "A,R4"    , 0                },
/*2D*/      {"ADD",  "A,R5"    , 0                },
/*2E*/      {"ADD",  "A,R6"    , 0                },
/*2F*/      {"ADD",  "A,R7"    , 0                },

/*30*/      {"JNB",  "t,r"     , REFFLAG | CODEREF},
/*31*/      {"ACALL","a"       , REFFLAG | CODEREF},
/*32*/      {"RETI",  ""       , LFFLAG           },
/*33*/      {"RLC",   "A"      , 0                },
/*34*/      {"ADDC", "A,#i"    , 0                },
/*35*/      {"ADDC", "A,b"     , 0                },
/*36*/      {"ADDC", "A,@R0"   , 0                },
/*37*/      {"ADDC", "A,@R1"   , 0                },

/*38*/      {"ADDC", "A,R0"    , 0                },
/*39*/      {"ADDC", "A,R1"    , 0                },
/*3A*/      {"ADDC", "A,R2"    , 0                },
/*3B*/      {"ADDC", "A,R3"    , 0                },
/*3C*/      {"ADDC", "A,R4"    , 0                },
/*3D*/      {"ADDC", "A,R5"    , 0                },
/*3E*/      {"ADDC", "A,R6"    , 0                },
/*3F*/      {"ADDC", "A,R7"    , 0                },

/*40*/      {"JC",   "r"       , REFFLAG | CODEREF},
/*41*/      {"AJMP", "a"       , LFFLAG | REFFLAG | CODEREF},
/*42*/      {"ORL",  "b,A"     , 0                },
/*43*/      {"ORL",  "b,#i"    , 0                },
/*44*/      {"ORL",  "A,#i"    , 0                },
/*45*/      {"ORL",  "A,b"     , 0                },
/*46*/      {"ORL",  "A,@R0"   , 0                },
/*47*/      {"ORL",  "A,@R1"   , 0                },

/*48*/      {"ORL",  "A,R0"    , 0                },
/*49*/      {"ORL",  "A,R1"    , 0                },
/*4A*/      {"ORL",  "A,R2"    , 0                },
/*4B*/      {"ORL",  "A,R3"    , 0                },
/*4C*/      {"ORL",  "A,R4"    , 0                },
/*4D*/      {"ORL",  "A,R5"    , 0                },
/*4E*/      {"ORL",  "A,R6"    , 0                },
/*4F*/      {"ORL",  "A,R7"    , 0                },

/*50*/      {"JNC",  "r"       , REFFLAG | CODEREF},
/*51*/      {"ACALL","a"       , REFFLAG | CODEREF},
/*52*/      {"ANL",  "b,A"     , 0                },
/*53*/      {"ANL",  "b,#i"    , 0                },
/*54*/      {"ANL",  "A,#i"    , 0                },
/*55*/      {"ANL",  "A,b"     , 0                },
/*56*/      {"ANL",  "A,@R0"   , 0                },
/*57*/      {"ANL",  "A,@R1"   , 0                },

/*58*/      {"ANL",  "A,R0"    , 0                },
/*59*/      {"ANL",  "A,R1"    , 0                },
/*5A*/      {"ANL",  "A,R2"    , 0                },
/*5B*/      {"ANL",  "A,R3"    , 0                },
/*5C*/      {"ANL",  "A,R4"    , 0                },
/*5D*/      {"ANL",  "A,R5"    , 0                },
/*5E*/      {"ANL",  "A,R6"    , 0                },
/*5F*/      {"ANL",  "A,R7"    , 0                },

/*60*/      {"JZ",   "r"       , REFFLAG | CODEREF},
/*61*/      {"AJMP", "a"       , LFFLAG | REFFLAG | CODEREF},
/*62*/      {"XRL",  "b,A"     , 0                },
/*63*/      {"XRL",  "b,#i"    , 0                },
/*64*/      {"XRL",  "A,#i"    , 0                },
/*65*/      {"XRL",  "A,b"     , 0                },
/*66*/      {"XRL",  "A,@R0"   , 0                },
/*67*/      {"XRL",  "A,@R1"   , 0                },

/*68*/      {"XRL",  "A,R0"    , 0                },
/*69*/      {"XRL",  "A,R1"    , 0                },
/*6A*/      {"XRL",  "A,R2"    , 0                },
/*6B*/      {"XRL",  "A,R3"    , 0                },
/*6C*/      {"XRL",  "A,R4"    , 0                },
/*6D*/      {"XRL",  "A,R5"    , 0                },
/*6E*/      {"XRL",  "A,R6"    , 0                },
/*6F*/      {"XRL",  "A,R7"    , 0                },

/*70*/      {"JNZ",  "r"       , REFFLAG | CODEREF},
/*71*/      {"ACALL","a"       , REFFLAG | CODEREF},
/*72*/      {"ORL",  "C,t"     , 0                },
/*73*/      {"JMP",  "@A+DPTR" , LFFLAG           },
/*74*/      {"MOV",  "A,#i"    , 0                },
/*75*/      {"MOV",  "b,#i"    , 0                },
/*76*/      {"MOV",  "@R0,#i"  , 0                },
/*77*/      {"MOV",  "@R1,#i"  , 0                },

/*78*/      {"MOV",  "R0,#i"   , 0                },
/*79*/      {"MOV",  "R1,#i"   , 0                },
/*7A*/      {"MOV",  "R2,#i"   , 0                },
/*7B*/      {"MOV",  "R3,#i"   , 0                },
/*7C*/      {"MOV",  "R4,#i"   , 0                },
/*7D*/      {"MOV",  "R5,#i"   , 0                },
/*7E*/      {"MOV",  "R6,#i"   , 0                },
/*7F*/      {"MOV",  "R7,#i"   , 0                },

/*80*/      {"SJMP", "r"       , LFFLAG | REFFLAG | CODEREF},
/*81*/      {"AJMP", "a"       , LFFLAG | REFFLAG | CODEREF},
/*82*/      {"ANL",  "C,t"     , 0                },
/*83*/      {"MOVC", "A,@A+PC" , 0                },
/*84*/      {"DIV",  "AB"      , 0                },
/*85*/      {"MOV",  "m"       , 0                },
/*86*/      {"MOV",  "b,@R0"   , 0                },
/*87*/      {"MOV",  "b,@R1"   , 0                },

/*88*/      {"MOV",  "b,R0"    , 0                },
/*89*/      {"MOV",  "b,R1"    , 0                },
/*8A*/      {"MOV",  "b,R2"    , 0                },
/*8B*/      {"MOV",  "b,R3"    , 0                },
/*8C*/      {"MOV",  "b,R4"    , 0                },
/*8D*/      {"MOV",  "b,R5"    , 0                },
/*8E*/      {"MOV",  "b,R6"    , 0                },
/*8F*/      {"MOV",  "b,R7"    , 0                },

/*90*/      {"MOV",  "DPTR,#w" , 0                },
/*91*/      {"ACALL","a"       , REFFLAG | CODEREF},
/*92*/      {"MOV",  "t,C"     , 0                },
/*93*/      {"MOVC", "A,@A+DPTR",0                },
/*94*/      {"SUBB", "A,#i"    , 0                },
/*95*/      {"SUBB", "A,b"     , 0                },
/*96*/      {"SUBB", "A,@R0"   , 0                },
/*97*/      {"SUBB", "A,@R1"   , 0                },

/*98*/      {"SUBB", "A,R0"    , 0                },
/*99*/      {"SUBB", "A,R1"    , 0                },
/*9A*/      {"SUBB", "A,R2"    , 0                },
/*9B*/      {"SUBB", "A,R3"    , 0                },
/*9C*/      {"SUBB", "A,R4"    , 0                },
/*9D*/      {"SUBB", "A,R5"    , 0                },
/*9E*/      {"SUBB", "A,R6"    , 0                },
/*9F*/      {"SUBB", "A,R7"    , 0                },

/*A0*/      {"ORL",  "C,/t"    , 0                },
/*A1*/      {"AJMP", "a"       , LFFLAG | REFFLAG | CODEREF},
/*A2*/      {"MOV",  "C,t"     , 0                },
/*A3*/      {"INC",  "DPTR"    , 0                },
/*A4*/      {"MUL",  "AB"      , 0                },
/*A5*/      {"",     ""        , 0                }, // MOV bit,bit or extended prefix
/*A6*/      {"MOV",  "@R0,b"   , 0                },
/*A7*/      {"MOV",  "@R1,b"   , 0                },

/*A8*/      {"MOV",  "R0,b"    , 0                },
/*A9*/      {"MOV",  "R1,b"    , 0                },
/*AA*/      {"MOV",  "R2,b"    , 0                },
/*AB*/      {"MOV",  "R3,b"    , 0                },
/*AC*/      {"MOV",  "R4,b"    , 0                },
/*AD*/      {"MOV",  "R5,b"    , 0                },
/*AE*/      {"MOV",  "R6,b"    , 0                },
/*AF*/      {"MOV",  "R7,b"    , 0                },

/*B0*/      {"ANL",  "C,/t"    , 0                },
/*B1*/      {"ACALL","a"       , REFFLAG | CODEREF},
/*B2*/      {"CPL",  "t"       , 0                },
/*B3*/      {"CPL",  "C"       , 0                },
/*B4*/      {"CJNE", "A,#i,r"  , REFFLAG | CODEREF},
/*B5*/      {"CJNE", "A,b,r"   , REFFLAG | CODEREF},
/*B6*/      {"CJNE", "@R0,#i,r", REFFLAG | CODEREF},
/*B7*/      {"CJNE", "@R1,#i,r", REFFLAG | CODEREF},

/*B8*/      {"CJNE", "R0,#i,r" , REFFLAG | CODEREF},
/*B9*/      {"CJNE", "R1,#i,r" , REFFLAG | CODEREF},
/*BA*/      {"CJNE", "R2,#i,r" , REFFLAG | CODEREF},
/*BB*/      {"CJNE", "R3,#i,r" , REFFLAG | CODEREF},
/*BC*/      {"CJNE", "R4,#i,r" , REFFLAG | CODEREF},
/*BD*/      {"CJNE", "R5,#i,r" , REFFLAG | CODEREF},
/*BE*/      {"CJNE", "R6,#i,r" , REFFLAG | CODEREF},
/*BF*/      {"CJNE", "R7,#i,r" , REFFLAG | CODEREF},

/*C0*/      {"PUSH", "b"       , 0                },
/*C1*/      {"AJMP", "a"       , LFFLAG | REFFLAG | CODEREF},
/*C2*/      {"CLR",  "t"       , 0                },
/*C3*/      {"CLR",  "C"       , 0                },
/*C4*/      {"SWAP", "A"       , 0                },
/*C5*/      {"XCH",  "A,b"     , 0                },
/*C6*/      {"XCH",  "A,@R0"   , 0                },
/*C7*/      {"XCH",  "A,@R1"   , 0                },

/*C8*/      {"XCH",  "A,R0"    , 0                },
/*C9*/      {"XCH",  "A,R1"    , 0                },
/*CA*/      {"XCH",  "A,R2"    , 0                },
/*CB*/      {"XCH",  "A,R3"    , 0                },
/*CC*/      {"XCH",  "A,R4"    , 0                },
/*CD*/      {"XCH",  "A,R5"    , 0                },
/*CE*/      {"XCH",  "A,R6"    , 0                },
/*CF*/      {"XCH",  "A,R7"    , 0                },

/*D0*/      {"POP",  "b"       , 0                },
/*D1*/      {"ACALL","a"       , REFFLAG | CODEREF},
/*D2*/      {"SETB", "t"       , 0                },
/*D3*/      {"SETB", "C"       , 0                },
/*D4*/      {"DA",   "A"       , 0                },
/*D5*/      {"DJNZ", "b,r"     , REFFLAG | CODEREF},
/*D6*/      {"XCHD", "@R0"     , 0                },
/*D7*/      {"XCHD", "@R1"     , 0                },

/*D8*/      {"DJNZ", "R0,r"    , REFFLAG | CODEREF},
/*D9*/      {"DJNZ", "R1,r"    , REFFLAG | CODEREF},
/*DA*/      {"DJNZ", "R2,r"    , REFFLAG | CODEREF},
/*DB*/      {"DJNZ", "R3,r"    , REFFLAG | CODEREF},
/*DC*/      {"DJNZ", "R4,r"    , REFFLAG | CODEREF},
/*DD*/      {"DJNZ", "R5,r"    , REFFLAG | CODEREF},
/*DE*/      {"DJNZ", "R6,r"    , REFFLAG | CODEREF},
/*DF*/      {"DJNZ", "R7,r"    , REFFLAG | CODEREF},

/*E0*/      {"MOVX", "A,@DPTR" , 0                },
/*E1*/      {"AJMP", "a"       , LFFLAG | REFFLAG | CODEREF},
/*E2*/      {"MOVX", "A,@R0"   , 0                },
/*E3*/      {"MOVX", "A,@R1"   , 0                },
/*E4*/      {"CLR",  "A"       , 0                },
/*E5*/      {"MOV",  "A,b"     , 0                },
/*E6*/      {"MOV",  "A,@R0"   , 0                },
/*E7*/      {"MOV",  "A,@R1"   , 0                },

/*E8*/      {"MOV",  "A,R0"    , 0                },
/*E9*/      {"MOV",  "A,R1"    , 0                },
/*EA*/      {"MOV",  "A,R2"    , 0                },
/*EB*/      {"MOV",  "A,R3"    , 0                },
/*EC*/      {"MOV",  "A,R4"    , 0                },
/*ED*/      {"MOV",  "A,R5"    , 0                },
/*EE*/      {"MOV",  "A,R6"    , 0                },
/*EF*/      {"MOV",  "A,R7"    , 0                },

/*F0*/      {"MOVX", "@DPTR,A" , 0                },
/*F1*/      {"ACALL","a"       , REFFLAG | CODEREF},
/*F2*/      {"MOVX", "@R0,A"   , 0                },
/*F3*/      {"MOVX", "@R1,A"   , 0                },
/*F4*/      {"CPL",  "A"       , 0                },
/*F5*/      {"MOV",  "b,A"     , 0                },
/*F6*/      {"MOV",  "@R0,A"   , 0                },
/*F7*/      {"MOV",  "@R1,A"   , 0                },

/*F8*/      {"MOV",  "R0,A"    , 0                },
/*F9*/      {"MOV",  "R1,A"    , 0                },
/*FA*/      {"MOV",  "R2,A"    , 0                },
/*FB*/      {"MOV",  "R3,A"    , 0                },
/*FC*/      {"MOV",  "R4,A"    , 0                },
/*FD*/      {"MOV",  "R5,A"    , 0                },
/*FE*/      {"MOV",  "R6,A"    , 0                },
/*FF*/      {"MOV",  "R7,A"    , 0                },
};


const char *sfr[128] = {
    /*80*/ "P0",    "SP",    "DPL",    "DPH",    "ADAT", "",      "",     "PCON",
    /*88*/ "TCON",  "TMOD",  "TL0",    "TL1",    "TH0",  "TH1",   "PWCM", "PWMP",
    /*90*/ "P1",    "",      "",       "",       "",     "",      "",      "",
    /*98*/ "SCON",  "SBUF",  "",       "",       "",     "",      "",      "",
    /*A0*/ "P2",    "",      "",       "",       "",     "",      "",      "",
    /*A8*/ "IE",    "CML0",  "CML1",   "CML2",   "CTL0", "CTL1",  "CTL2",  "CTL3",
    /*B0*/ "P3",    "",      "",       "",       "",     "",      "",      "",
    /*B8*/ "IP",    "",      "",       "",       "",     "",      "",      "",
    /*C0*/ "P4",    "",      "",       "",       "P5",   "ADCON", "ADCH",  "",
    /*C8*/ "T2CON", "CMH0",  "RCAP2L", "RCAP2H", "TL2",  "TH2",   "CTH2",  "CTH3",
    /*D0*/ "PSW",   "",      "",       "",       "",     "",      "",      "",
    /*D8*/ "I2CFG", "S1STA", "S1DAT",  "S1ADR",  "",     "",      "",      "",
    /*E0*/ "ACC",   "",      "",       "",       "",     "",      "",      "",
    /*E8*/ "CSR",   "",      "TN2CON", "CTCON",  "TML2", "TMH2",  "STE",   "RTE",
    /*F0*/ "B",     "",      "",       "",       "",     "",      "",      "",
    /*F8*/ "I2STA", "",      "",       "",       "PWM0", "PWM1",  "PWENA", "T3"
};


const char *sfrbit[128] = {
    /*80*/ "P0.0",  "P0.1",  "P0.2",    "P0.3",    "P0.4",  "P0.5",   "P0.6",   "P0.7",
    /*88*/ "IT0",   "IE0",   "IT1",     "IE1",     "TR0",   "TF0",    "TR1",    "TF1",
    /*90*/ "P1.0",  "P1.1",  "P1.2",    "P1.3",    "P1.4",  "P1.5",   "P1.6",   "P1.7",
    /*98*/ "RI",    "TI",    "RB8",     "TB8",     "REN",   "SM2",    "SM1",    "SM0",
    /*A0*/ "P2.0",  "P2.1",  "P2.2",    "P2.3",    "P2.4",  "P2.5",   "P2.6",   "P2.7",
    /*A8*/ "EX0",   "ET0",   "EX1",     "ET1",     "ES",    "IE.5",   "IE.6",   "EA",
    /*B0*/ "RXD",   "TXD",   "INT0",    "INT1",    "T0",    "T1",     "WR",     "RD",
    /*B8*/ "PX0",   "PT0",   "PX1",     "PT1",     "PS",    "IP.5",   "IP.6",   "IP.7",
    /*C0*/ "P4.0",  "P4.1",  "P4.2",    "P4.3",    "P4.4",  "P4.5",   "P4.6",   "P4.7",
    /*C8*/ "CPRL2", "CT2",   "TR2",     "EXEN2",   "TCLK",  "RCLK",   "EXF2",   "TF2",
    /*D0*/ "P",     "PSW.1", "OV",      "RS0",     "RS1",   "F0",     "AC",     "CY",
    /*D8*/ "CT0",   "CT1",   "I2CFG.2", "I2CFG.3", "TIRUN", "CLRTI",  "MASTRQ", "SLAVEN",
    /*E0*/ "ACC.0", "ACC.1", "ACC.2",   "ACC.3",   "ACC.4", "ACC.5",  "ACC.6",  "ACC.7",
    /*E8*/ "IBF",   "OBF",   "IDSM",    "OBFC",    "MA0",   "MA1",    "MB0",    "MB1",
    /*F0*/ "B.0",   "B.1",   "B.2",     "B.3",     "B.4",   "B.5",    "B.6",    "B.7",
    /*F8*/ "XSTP",  "SXTR",  "MAKSTP",  "MKSTR",   "XACTV", "XDATA",  "IDLE",   "I2STA.7"
};


int BitAddr(int addr)
{
    if (addr & 0x80) {
        // 80..FF -> 80..FF
        return addr & 0xF8;
    } else {
        // 00..7F -> 20..2F
        return 0x20 + (addr >> 3);
    }
}


int Dis8051::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    unsigned short  ad;
    int             i, opcd;
    int             r;
    InstrPtr        instr;
    char            *p, *l;
    char            s[256];
    addr_t          ra;

    ad    = addr;
    opcd = ReadByte(ad++);
    int len = 1;
    instr = &I8051_opcdTable[opcd];

    strcpy(opcode, instr->op);
    strcpy(parms, instr->parms);
    lfref  = instr->lfref;
    refaddr = 0;

    p = s;
    l = parms;
    while (*l) {
        switch (*l) {
            case 'i':   // immediate byte
                i = ReadByte(ad++);
                len++;

                H2Str(i, p);
                while (*p) p++;
                break;

            case 'b':   // register byte
                i = ReadByte(ad++);
                len++;

                if (USEREG && i >= 0x80 && sfr[i & 0x7F][0]) {
                    // if register name available
                    p = stpcpy(p, sfr[i & 0x7F]);
                } else {
                    // else use hex
                    H2Str(i, p);
                    while (*p) p++;
                }
                break;

            case 't':   // bit address
                i = ReadByte(ad++);
                len++;

#if 0
                // use the plain hex form
                H2Str(i, p);
                while (*p) p++;
#else
                // use the "addr.bit" form
                r = BitAddr(i);
                if (USEREG && r >= 0x80 && sfrbit[i & 0x7F][0]) {
                    // if register name available
                    p = stpcpy(p, sfrbit[i & 0x7F]);
                } else {
                    // else use hex
                    H2Str(r, p);
                    while (*p) p++;
                    *p++ = '.';
                    *p++ = '0'+ (i & 7);
                    *p   = 0;
                }
#endif

                break;

            case 'm':   // MOV b,b with parameters reversed
                i = ReadByte(ad++);
                len++;
                r = ReadByte(ad++);
                len++;

                if (USEREG && r >= 0x80 && sfr[r & 0x7F][0]) {
                    // if register name available
                    p = stpcpy(p, sfr[r & 0x7F]);
                } else {
                    H2Str(r, p);
                    while (*p) p++;
                }

                *p++ = ',';
                *p   = 0;

                if (USEREG && i >= 0x80 && sfr[i & 0x7F][0]) {
                    // if register name available
                    p = stpcpy(p, sfr[i & 0x7F]);
                } else {
                    H2Str(i, p);
                    while (*p) p++;
                }
                break;

            case 'r':   // 8051 short jump
                i = ReadByte(ad++);
                len++;
                if (i >= 128) {
                    i = i - 256;
                }
                ra = (ad + i) & 0xFFFF;

                RefStr(ra, p, lfref, refaddr);
                while (*p) p++;
                break;

            case 'a':   // 8051 page jump
                ra = ReadByte(ad++);
                // need to use high bits of ad after previous ad++
                ra |= (ad & 0xF800) | ((opcd & 0xE0) << 3);
                len++;

                RefStr(ra, p, lfref, refaddr);
                while (*p) p++;
                break;

            case 'w':   // immediate word
                ra = ReadWord(ad);
                ad += 2;
                len += 2;

                RefStr(ra, p, lfref, refaddr);
                while (*p) p++;
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
