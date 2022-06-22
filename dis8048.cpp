// dis8048.cpp

static const char versionName[] = "Intel 8048 disassembler";

#include "discpu.h"

class Dis8048 : public CPU {
public:
    Dis8048(const char *name, int subtype, int endian, int addrwid,
            char curAddrChr, char hexChr, const char *byteOp,
            const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);

private:
    int selmb(addr_t addr); // peek upward in code to look for SEL MBx instrs
};


enum {
    // CPU types
    CPU_8021 = 1, // 8021, reduced from MCS-8048
    CPU_8048 = 2, // 8x48/8x49/8x35, MCS-48
    CPU_8041 = 4, // 8x41/8x42, UPI-41/42, some changed instructions

    // CPU types for instructions
    _4821 = CPU_8048 | CPU_8021,
    _48   = CPU_8048,
    _41   = CPU_8041,
};

Dis8048 cpu_8021("8021", CPU_8021, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DL");
Dis8048 cpu_8048("8048", CPU_8048, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DL");
Dis8048 cpu_8049("8049", CPU_8048, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DL");
Dis8048 cpu_8035("8035", CPU_8048, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DL");
Dis8048 cpu_8041("8041", CPU_8041, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DL");
Dis8048 cpu_8042("8042", CPU_8041, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DL");


Dis8048::Dis8048(const char *name, int subtype, int endian, int addrwid,
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
    uint8_t         cpu;    // CPU subtype
    uint8_t         lfref;  // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;


static const struct InstrRec I8048_opcdTable[] =
{
            // op     line         lf    ref
/*00*/      {"NOP",  ""        , _4821, 0                },
/*01*/      {"",     ""        ,   0  , 0                },
/*02*/      {"OUTL", "BUS,A"   , _48  , 0                },
/*03*/      {"ADD",  "A,#b"    , _4821, 0                },
/*04*/      {"JMP",  "a"       , _4821, LFFLAG | REFFLAG | CODEREF},
/*05*/      {"EN",   "I"       , _48  , 0                },
/*06*/      {"",     ""        ,   0  , 0                },
/*07*/      {"DEC",  "A"       , _4821, 0                },

/*08*/      {"INS",  "A,BUS"   , _4821, 0                },
/*09*/      {"IN",   "A,P1"    , _4821, 0                },
/*0A*/      {"IN",   "A,P2"    , _4821, 0                },
/*0B*/      {"",     ""        ,   0  , 0                },
/*0C*/      {"MOVD", "A,P4"    , _4821, 0                },
/*0D*/      {"MOVD", "A,P5"    , _4821, 0                },
/*0E*/      {"MOVD", "A,P6"    , _4821, 0                },
/*0F*/      {"MOVD", "A,P7"    , _4821, 0                },

/*10*/      {"INC",  "@R0"     , _4821, 0                },
/*11*/      {"INC",  "@R1"     , _4821, 0                },
/*12*/      {"JB0",  "j"       , _48  , REFFLAG | CODEREF},
/*13*/      {"ADDC", "A,#b"    , _4821, 0                },
/*14*/      {"CALL", "a"       , _4821, REFFLAG | CODEREF},
/*15*/      {"DIS",  "I"       , _48  , 0                },
/*16*/      {"JTF",  "j"       , _4821, REFFLAG | CODEREF},
/*17*/      {"INC",  "A"       , _4821, 0                },

/*18*/      {"INC",  "R0"      , _4821, 0                },
/*19*/      {"INC",  "R1"      , _4821, 0                },
/*1A*/      {"INC",  "R2"      , _4821, 0                },
/*1B*/      {"INC",  "R3"      , _4821, 0                },
/*1C*/      {"INC",  "R4"      , _4821, 0                },
/*1D*/      {"INC",  "R5"      , _4821, 0                },
/*1E*/      {"INC",  "R6"      , _4821, 0                },
/*1F*/      {"INC",  "R7"      , _4821, 0                },

/*20*/      {"XCH",  "A,@R0"   , _4821, 0                },
/*21*/      {"XCH",  "A,@R1"   , _4821, 0                },
/*22*/      {"",     ""        ,   0  , 0                },
/*23*/      {"MOV",  "A,#b"    , _4821, 0                },
/*24*/      {"JMP",  "a"       , _4821, LFFLAG | REFFLAG | CODEREF},
/*25*/      {"EN",   "TCNTI"   , _48  , 0                },
/*26*/      {"JNT0", "j"       , _48  , REFFLAG | CODEREF},
/*27*/      {"CLR",  "A"       , _4821, 0                },

/*28*/      {"XCH",  "A,R0"    , _4821, 0                },
/*29*/      {"XCH",  "A,R1"    , _4821, 0                },
/*2A*/      {"XCH",  "A,R2"    , _4821, 0                },
/*2B*/      {"XCH",  "A,R3"    , _4821, 0                },
/*2C*/      {"XCH",  "A,R4"    , _4821, 0                },
/*2D*/      {"XCH",  "A,R5"    , _4821, 0                },
/*2E*/      {"XCH",  "A,R6"    , _4821, 0                },
/*2F*/      {"XCH",  "A,R7"    , _4821, 0                },

/*30*/      {"XCHD", "A,@R0"   , _4821, 0                },
/*31*/      {"XCHD", "A,@R1"   , _4821, 0                },
/*32*/      {"JB1",  "j"       , _48  , REFFLAG | CODEREF},
/*33*/      {"",     ""        ,   0  , 0                },
/*34*/      {"CALL", "a"       , _4821, REFFLAG | CODEREF},
/*35*/      {"DIS",  "TCNTI"   , _48  , 0                },
/*36*/      {"JT0",  "j"       , _48  , REFFLAG | CODEREF},
/*37*/      {"CPL",  "A"       , _4821, 0                },

/*38*/      {"",     ""        ,   0  , 0                },
/*39*/      {"OUTL", "P1,A"    , _4821, 0                },
/*3A*/      {"OUTL", "P2,A"    , _4821, 0                },
/*3B*/      {"",     ""        ,   0  , 0                },
/*3C*/      {"MOVD", "P4,A"    , _4821, 0                },
/*3D*/      {"MOVD", "P5,A"    , _4821, 0                },
/*3E*/      {"MOVD", "P6,A"    , _4821, 0                },
/*3F*/      {"MOVD", "P7,A"    , _4821, 0                },

/*40*/      {"ORL",  "A,@R0"   , _4821, 0                },
/*41*/      {"ORL",  "A,@R1"   , _4821, 0                },
/*42*/      {"MOV",  "A,T"     , _4821, 0                },
/*43*/      {"ORL",  "A,#b"    , _4821, 0                },
/*44*/      {"JMP",  "a"       , _4821, LFFLAG | REFFLAG | CODEREF},
/*45*/      {"STRT", "CNT"     , _4821, 0                },
/*46*/      {"JNT1", "j"       , _4821, REFFLAG | CODEREF},
/*47*/      {"SWAP", "A"       , _4821, 0                },

/*48*/      {"ORL",  "A,R0"    , _4821, 0                },
/*49*/      {"ORL",  "A,R1"    , _4821, 0                },
/*4A*/      {"ORL",  "A,R2"    , _4821, 0                },
/*4B*/      {"ORL",  "A,R3"    , _4821, 0                },
/*4C*/      {"ORL",  "A,R4"    , _4821, 0                },
/*4D*/      {"ORL",  "A,R5"    , _4821, 0                },
/*4E*/      {"ORL",  "A,R6"    , _4821, 0                },
/*4F*/      {"ORL",  "A,R7"    , _4821, 0                },

/*50*/      {"ANL",  "A,@R0"   , _4821, 0                },
/*51*/      {"ANL",  "A,@R1"   , _4821, 0                },
/*52*/      {"JB2",  "j"       , _48  , REFFLAG | CODEREF},
/*53*/      {"ANL",  "A,#b"    , _4821, 0                },
/*54*/      {"CALL", "a"       , _4821, REFFLAG | CODEREF},
/*55*/      {"STRT", "T"       , _4821, 0                },
/*56*/      {"JT1",  "j"       , _4821, REFFLAG | CODEREF},
/*57*/      {"DA",   "A"       , _4821, 0                },

/*58*/      {"ANL",  "A,R0"    , _4821, 0                },
/*59*/      {"ANL",  "A,R1"    , _4821, 0                },
/*5A*/      {"ANL",  "A,R2"    , _4821, 0                },
/*5B*/      {"ANL",  "A,R3"    , _4821, 0                },
/*5C*/      {"ANL",  "A,R4"    , _4821, 0                },
/*5D*/      {"ANL",  "A,R5"    , _4821, 0                },
/*5E*/      {"ANL",  "A,R6"    , _4821, 0                },
/*5F*/      {"ANL",  "A,R7"    , _4821, 0                },

/*60*/      {"ADD",  "A,@R0"   , _4821, 0                },
/*61*/      {"ADD",  "A,@R1"   , _4821, 0                },
/*62*/      {"MOV",  "T,A"     , _4821, 0                },
/*63*/      {"",     ""        ,   0  , 0                },
/*64*/      {"JMP",  "a"       , _4821, LFFLAG | REFFLAG | CODEREF},
/*65*/      {"STOP", "TCNT"    , _4821, 0                },
/*66*/      {"",     ""        ,   0  , 0                },
/*67*/      {"RRC",  "A"       , _4821, 0                },

/*68*/      {"ADD",  "A,R0"    , _4821, 0                },
/*69*/      {"ADD",  "A,R1"    , _4821, 0                },
/*6A*/      {"ADD",  "A,R2"    , _4821, 0                },
/*6B*/      {"ADD",  "A,R3"    , _4821, 0                },
/*6C*/      {"ADD",  "A,R4"    , _4821, 0                },
/*6D*/      {"ADD",  "A,R5"    , _4821, 0                },
/*6E*/      {"ADD",  "A,R6"    , _4821, 0                },
/*6F*/      {"ADD",  "A,R7"    , _4821, 0                },

/*70*/      {"ADDC", "A,@R0"   , _4821, 0                },
/*71*/      {"ADDC", "A,@R1"   , _4821, 0                },
/*72*/      {"JB3",  "j"       , _48  , REFFLAG | CODEREF},
/*73*/      {"",     ""        ,   0  , 0                },
/*74*/      {"CALL", "a"       , _4821, REFFLAG | CODEREF},
/*75*/      {"ENT0", "CLK"     , _48  , 0                },
/*76*/      {"JF1",  "j"       , _48  , REFFLAG | CODEREF},
/*77*/      {"RR",   "A"       , _4821, 0                },

/*78*/      {"ADDC", "A,R0"    , _4821, 0                },
/*79*/      {"ADDC", "A,R1"    , _4821, 0                },
/*7A*/      {"ADDC", "A,R2"    , _4821, 0                },
/*7B*/      {"ADDC", "A,R3"    , _4821, 0                },
/*7C*/      {"ADDC", "A,R4"    , _4821, 0                },
/*7D*/      {"ADDC", "A,R5"    , _4821, 0                },
/*7E*/      {"ADDC", "A,R6"    , _4821, 0                },
/*7F*/      {"ADDC", "A,R7"    , _4821, 0                },

/*80*/      {"MOVX", "A,@R0"   , _48  , 0                },
/*81*/      {"MOVX", "A,@R1"   , _48  , 0                },
/*82*/      {"",     ""        ,   0  , 0                },
/*83*/      {"RET",  ""        , _4821, LFFLAG           },
/*84*/      {"JMP",  "a"       , _4821, LFFLAG | REFFLAG | CODEREF},
/*85*/      {"CLR",  "F0"      , _4821, 0                },
/*86*/      {"JNI",  "j"       , _4821, REFFLAG | CODEREF},
/*87*/      {"",     ""        ,   0  , 0                },

/*88*/      {"ORL",  "BUS,#b"  , _48  , 0                },
/*89*/      {"ORL",  "P1,#b"   , _48  , 0                },
/*8A*/      {"ORL",  "P2,#b"   , _48  , 0                },
/*8B*/      {"",     ""        ,   0  , 0                },
/*8C*/      {"ORLD", "P4,A"    , _4821, 0                },
/*8D*/      {"ORLD", "P5,A"    , _4821, 0                },
/*8E*/      {"ORLD", "P6,A"    , _4821, 0                },
/*8F*/      {"ORLD", "P7,A"    , _4821, 0                },

/*90*/      {"MOVX", "@R0,A"   , _4821, 0                },
/*91*/      {"MOVX", "@R1,A"   , _4821, 0                },
/*92*/      {"JB4",  "j"       , _4821, REFFLAG | CODEREF},
/*93*/      {"RETR", ""        , _4821, LFFLAG           },
/*94*/      {"CALL", "a"       , _4821, REFFLAG | CODEREF},
/*95*/      {"CPL",  "F0"      , _4821, 0                },
/*96*/      {"JNZ",  "j"       , _4821, REFFLAG | CODEREF},
/*97*/      {"CLR",  "C"       , _4821, 0                },

/*98*/      {"ANL",  "BUS,#b"  , _4821, 0                },
/*99*/      {"ANL",  "P1,#b"   , _4821, 0                },
/*9A*/      {"ANL",  "P2,#b"   , _4821, 0                },
/*9B*/      {"",     ""        ,   0  , 0                },
/*9C*/      {"ANLD", "P4,A"    , _4821, 0                },
/*9D*/      {"ANLD", "P5,A"    , _4821, 0                },
/*9E*/      {"ANLD", "P6,A"    , _4821, 0                },
/*9F*/      {"ANLD", "P7,A"    , _4821, 0                },

/*A0*/      {"MOV",  "@R0,A"   , _4821, 0                },
/*A1*/      {"MOV",  "@R1,A"   , _4821, 0                },
/*A2*/      {"",     ""        ,   0  , 0                },
/*A3*/      {"MOVP", "A,@A"    , _4821, 0                },
/*A4*/      {"JMP",  "a"       , _4821, LFFLAG | REFFLAG | CODEREF},
/*A5*/      {"CLR",  "F1"      , _48  , 0                },
/*A6*/      {"",     ""        ,   0  , 0                },
/*A7*/      {"CPL",  "C"       , _4821, 0                },

/*A8*/      {"MOV",  "R0,A"    , _4821, 0                },
/*A9*/      {"MOV",  "R1,A"    , _4821, 0                },
/*AA*/      {"MOV",  "R2,A"    , _4821, 0                },
/*AB*/      {"MOV",  "R3,A"    , _4821, 0                },
/*AC*/      {"MOV",  "R4,A"    , _4821, 0                },
/*AD*/      {"MOV",  "R5,A"    , _4821, 0                },
/*AE*/      {"MOV",  "R6,A"    , _4821, 0                },
/*AF*/      {"MOV",  "R7,A"    , _4821, 0                },

/*B0*/      {"MOV",  "@R0,#b"  , _4821, 0                },
/*B1*/      {"MOV",  "@R1,#b"  , _4821, 0                },
/*B2*/      {"JB5",  "j"       , _48  , REFFLAG | CODEREF},
/*B3*/      {"JMPP", "@A"      , _4821, LFFLAG           },
/*B4*/      {"CALL", "a"       , _4821, REFFLAG | CODEREF},
/*B5*/      {"CPL",  "F1"      , _48  , 0                },
/*B6*/      {"JF0",  "j"       , _48  , REFFLAG | CODEREF},
/*B7*/      {"",     ""        ,   0  , 0                },

/*B8*/      {"MOV",  "R0,#b"   , _4821, 0                },
/*B9*/      {"MOV",  "R1,#b"   , _4821, 0                },
/*BA*/      {"MOV",  "R2,#b"   , _4821, 0                },
/*BB*/      {"MOV",  "R3,#b"   , _4821, 0                },
/*BC*/      {"MOV",  "R4,#b"   , _4821, 0                },
/*BD*/      {"MOV",  "R5,#b"   , _4821, 0                },
/*BE*/      {"MOV",  "R6,#b"   , _4821, 0                },
/*BF*/      {"MOV",  "R7,#b"   , _4821, 0                },

/*C0*/      {"",     ""        ,   0  , 0                },
/*C1*/      {"",     ""        ,   0  , 0                },
/*C2*/      {"",     ""        ,   0  , 0                },
/*C3*/      {"",     ""        ,   0  , 0                },
/*C4*/      {"JMP",  "a"       , _4821, LFFLAG | REFFLAG | CODEREF},
/*C5*/      {"SEL",  "RB0"     , _48  , 0                },
/*C6*/      {"JZ",   "j"       , _4821, REFFLAG | CODEREF},
/*C7*/      {"MOV",  "A,PSW"   , _48  , 0                },

/*C8*/      {"DEC",  "R0"      , _48  , 0                },
/*C9*/      {"DEC",  "R1"      , _48  , 0                },
/*CA*/      {"DEC",  "R2"      , _48  , 0                },
/*CB*/      {"DEC",  "R3"      , _48  , 0                },
/*CC*/      {"DEC",  "R4"      , _48  , 0                },
/*CD*/      {"DEC",  "R5"      , _48  , 0                },
/*CE*/      {"DEC",  "R6"      , _48  , 0                },
/*CF*/      {"DEC",  "R7"      , _48  , 0                },

/*D0*/      {"XRL",  "A,@R0"   , _4821, 0                },
/*D1*/      {"XRL",  "A,@R1"   , _4821, 0                },
/*D2*/      {"JB6",  "j"       , _48  , REFFLAG | CODEREF},
/*D3*/      {"XRL",  "A,#b"    , _4821, 0                },
/*D4*/      {"CALL", "a"       , _4821, REFFLAG | CODEREF},
/*D5*/      {"SEL",  "RB1"     , _48  , 0                },
/*D6*/      {"JNIBF","j"       , _48  , REFFLAG | CODEREF},
/*D7*/      {"MOV",  "PSW,A"   , _48  , 0                },

/*D8*/      {"XRL",  "A,R0"    , _4821, 0                },
/*D9*/      {"XRL",  "A,R1"    , _4821, 0                },
/*DA*/      {"XRL",  "A,R2"    , _4821, 0                },
/*DB*/      {"XRL",  "A,R3"    , _4821, 0                },
/*DC*/      {"XRL",  "A,R4"    , _4821, 0                },
/*DD*/      {"XRL",  "A,R5"    , _4821, 0                },
/*DE*/      {"XRL",  "A,R6"    , _4821, 0                },
/*DF*/      {"XRL",  "A,R7"    , _4821, 0                },

/*E0*/      {"",     ""        ,   0  , 0                },
/*E1*/      {"",     ""        ,   0  , 0                },
/*E2*/      {"",     ""        ,   0  , 0                },
/*E3*/      {"MOVP3","A,@A"    , _48  , 0                },
/*E4*/      {"JMP",  "a"       , _4821, LFFLAG | REFFLAG | CODEREF},
/*E5*/      {"SEL",  "MB0"     , _48  , 0                },
/*E6*/      {"JNC",  "j"       , _4821, REFFLAG | CODEREF},
/*E7*/      {"RL",   "A"       , _4821, 0                },

/*E8*/      {"DJNZ", "R0,j"    , _4821, REFFLAG | CODEREF},
/*E9*/      {"DJNZ", "R1,j"    , _4821, REFFLAG | CODEREF},
/*EA*/      {"DJNZ", "R2,j"    , _4821, REFFLAG | CODEREF},
/*EB*/      {"DJNZ", "R3,j"    , _4821, REFFLAG | CODEREF},
/*EC*/      {"DJNZ", "R4,j"    , _4821, REFFLAG | CODEREF},
/*ED*/      {"DJNZ", "R5,j"    , _4821, REFFLAG | CODEREF},
/*EE*/      {"DJNZ", "R6,j"    , _4821, REFFLAG | CODEREF},
/*EF*/      {"DJNZ", "R7,j"    , _4821, REFFLAG | CODEREF},

/*F0*/      {"MOV",  "A,@R0"   , _4821, 0                },
/*F1*/      {"MOV",  "A,@R1"   , _4821, 0                },
/*F2*/      {"JB7",  "j"       , _48  , REFFLAG | CODEREF},
/*F3*/      {"",     ""        ,   0  , 0                },
/*F4*/      {"CALL", "a"       , _4821, REFFLAG | CODEREF},
/*F5*/      {"SEL",  "MB1"     , _48  , 0                },
/*F6*/      {"JC",   "j"       , _4821, REFFLAG | CODEREF},
/*F7*/      {"RLC",  "A"       , _4821, 0                },

/*F8*/      {"MOV",  "A,R0"    , _4821, 0                },
/*F9*/      {"MOV",  "A,R1"    , _4821, 0                },
/*FA*/      {"MOV",  "A,R2"    , _4821, 0                },
/*FB*/      {"MOV",  "A,R3"    , _4821, 0                },
/*FC*/      {"MOV",  "A,R4"    , _4821, 0                },
/*FD*/      {"MOV",  "A,R5"    , _4821, 0                },
/*FE*/      {"MOV",  "A,R6"    , _4821, 0                },
/*FF*/      {"MOV",  "A,R7"    , _4821, 0                },
};

static const struct InstrRec I8041_opcdTable[] =
{
            // op     line         lf    ref
/*00*/      {"NOP",  ""        , _41, 0                },
/*01*/      {"",     ""        ,  0 , 0                },
/*02*/      {"OUT",  "DBB,A"   , _41, 0                },
/*03*/      {"ADD",  "A,#b"    , _41, 0                },
/*04*/      {"JMP",  "a"       , _41, LFFLAG | REFFLAG | CODEREF},
/*05*/      {"EN",   "I"       , _41, 0                },
/*06*/      {"",     ""        ,  0 , 0                },
/*07*/      {"DEC",  "A"       , _41, 0                },

/*08*/      {"",     ""        ,  0 , 0                },
/*09*/      {"IN",   "A,P1"    , _41, 0                },
/*0A*/      {"IN",   "A,P2"    , _41, 0                },
/*0B*/      {"",     ""        ,  0 , 0                },
/*0C*/      {"MOVD", "A,P4"    , _41, 0                },
/*0D*/      {"MOVD", "A,P5"    , _41, 0                },
/*0E*/      {"MOVD", "A,P6"    , _41, 0                },
/*0F*/      {"MOVD", "A,P7"    , _41, 0                },

/*10*/      {"INC",  "@R0"     , _41, 0                },
/*11*/      {"INC",  "@R1"     , _41, 0                },
/*12*/      {"JB0",  "j"       , _41, REFFLAG | CODEREF},
/*13*/      {"ADDC", "A,#b"    , _41, 0                },
/*14*/      {"CALL", "a"       , _41, REFFLAG | CODEREF},
/*15*/      {"DIS",  "I"       , _41, 0                },
/*16*/      {"JTF",  "j"       , _41, REFFLAG | CODEREF},
/*17*/      {"INC",  "A"       , _41, 0                },

/*18*/      {"INC",  "R0"      , _41, 0                },
/*19*/      {"INC",  "R1"      , _41, 0                },
/*1A*/      {"INC",  "R2"      , _41, 0                },
/*1B*/      {"INC",  "R3"      , _41, 0                },
/*1C*/      {"INC",  "R4"      , _41, 0                },
/*1D*/      {"INC",  "R5"      , _41, 0                },
/*1E*/      {"INC",  "R6"      , _41, 0                },
/*1F*/      {"INC",  "R7"      , _41, 0                },

/*20*/      {"XCH",  "A,@R0"   , _41, 0                },
/*21*/      {"XCH",  "A,@R1"   , _41, 0                },
/*22*/      {"IN",   "A,DBB"   , _41, 0                },
/*23*/      {"MOV",  "A,#b"    , _41, 0                },
/*24*/      {"JMP",  "a"       , _41, LFFLAG | REFFLAG | CODEREF},
/*25*/      {"EN",   "TCNTI"   , _41, 0                },
/*26*/      {"JNT0", "j"       , _41, REFFLAG | CODEREF},
/*27*/      {"CLR",  "A"       , _41, 0                },

/*28*/      {"XCH",  "A,R0"    , _41, 0                },
/*29*/      {"XCH",  "A,R1"    , _41, 0                },
/*2A*/      {"XCH",  "A,R2"    , _41, 0                },
/*2B*/      {"XCH",  "A,R3"    , _41, 0                },
/*2C*/      {"XCH",  "A,R4"    , _41, 0                },
/*2D*/      {"XCH",  "A,R5"    , _41, 0                },
/*2E*/      {"XCH",  "A,R6"    , _41, 0                },
/*2F*/      {"XCH",  "A,R7"    , _41, 0                },

/*30*/      {"XCHD", "A,@R0"   , _41, 0                },
/*31*/      {"XCHD", "A,@R1"   , _41, 0                },
/*32*/      {"JB1",  "j"       , _41, REFFLAG | CODEREF},
/*33*/      {"",     ""        ,  0 , 0                },
/*34*/      {"CALL", "a"       , _41, REFFLAG | CODEREF},
/*35*/      {"DIS",  "TCNTI"   , _41, 0                },
/*36*/      {"JT0",  "j"       , _41, REFFLAG | CODEREF},
/*37*/      {"CPL",  "A"       , _41, 0                },

/*38*/      {"",     ""        ,  0 , 0                },
/*39*/      {"OUTL", "P1,A"    , _41, 0                },
/*3A*/      {"OUTL", "P2,A"    , _41, 0                },
/*3B*/      {"",     ""        ,  0 , 0                },
/*3C*/      {"MOVD", "P4,A"    , _41, 0                },
/*3D*/      {"MOVD", "P5,A"    , _41, 0                },
/*3E*/      {"MOVD", "P6,A"    , _41, 0                },
/*3F*/      {"MOVD", "P7,A"    , _41, 0                },

/*40*/      {"ORL",  "A,@R0"   , _41, 0                },
/*41*/      {"ORL",  "A,@R1"   , _41, 0                },
/*42*/      {"MOV",  "A,T"     , _41, 0                },
/*43*/      {"ORL",  "A,#b"    , _41, 0                },
/*44*/      {"JMP",  "a"       , _41, LFFLAG | REFFLAG | CODEREF},
/*45*/      {"STRT", "CNT"     , _41, 0                },
/*46*/      {"JNT1", "j"       , _41, REFFLAG | CODEREF},
/*47*/      {"SWAP", "A"       , _41, 0                },

/*48*/      {"ORL",  "A,R0"    , _41, 0                },
/*49*/      {"ORL",  "A,R1"    , _41, 0                },
/*4A*/      {"ORL",  "A,R2"    , _41, 0                },
/*4B*/      {"ORL",  "A,R3"    , _41, 0                },
/*4C*/      {"ORL",  "A,R4"    , _41, 0                },
/*4D*/      {"ORL",  "A,R5"    , _41, 0                },
/*4E*/      {"ORL",  "A,R6"    , _41, 0                },
/*4F*/      {"ORL",  "A,R7"    , _41, 0                },

/*50*/      {"ANL",  "A,@R0"   , _41, 0                },
/*51*/      {"ANL",  "A,@R1"   , _41, 0                },
/*52*/      {"JB2",  "j"       , _41, REFFLAG | CODEREF},
/*53*/      {"ANL",  "A,#b"    , _41, 0                },
/*54*/      {"CALL", "a"       , _41, REFFLAG | CODEREF},
/*55*/      {"STRT", "T"       , _41, 0                },
/*56*/      {"JT1",  "j"       , _41, REFFLAG | CODEREF},
/*57*/      {"DA",   "A"       , _41, 0                },

/*58*/      {"ANL",  "A,R0"    , _41, 0                },
/*59*/      {"ANL",  "A,R1"    , _41, 0                },
/*5A*/      {"ANL",  "A,R2"    , _41, 0                },
/*5B*/      {"ANL",  "A,R3"    , _41, 0                },
/*5C*/      {"ANL",  "A,R4"    , _41, 0                },
/*5D*/      {"ANL",  "A,R5"    , _41, 0                },
/*5E*/      {"ANL",  "A,R6"    , _41, 0                },
/*5F*/      {"ANL",  "A,R7"    , _41, 0                },

/*60*/      {"ADD",  "A,@R0"   , _41, 0                },
/*61*/      {"ADD",  "A,@R1"   , _41, 0                },
/*62*/      {"MOV",  "T,A"     , _41, 0                },
/*63*/      {"",     ""        ,  0 , 0                },
/*64*/      {"JMP",  "a"       , _41, LFFLAG | REFFLAG | CODEREF},
/*65*/      {"STOP", "TCNT"    , _41, 0                },
/*66*/      {"",     ""        ,  0 , 0                },
/*67*/      {"RRC",  "A"       , _41, 0                },

/*68*/      {"ADD",  "A,R0"    , _41, 0                },
/*69*/      {"ADD",  "A,R1"    , _41, 0                },
/*6A*/      {"ADD",  "A,R2"    , _41, 0                },
/*6B*/      {"ADD",  "A,R3"    , _41, 0                },
/*6C*/      {"ADD",  "A,R4"    , _41, 0                },
/*6D*/      {"ADD",  "A,R5"    , _41, 0                },
/*6E*/      {"ADD",  "A,R6"    , _41, 0                },
/*6F*/      {"ADD",  "A,R7"    , _41, 0                },

/*70*/      {"ADDC", "A,@R0"   , _41, 0                },
/*71*/      {"ADDC", "A,@R1"   , _41, 0                },
/*72*/      {"JB3",  "j"       , _41, REFFLAG | CODEREF},
/*73*/      {"",     ""        ,  0 , 0                },
/*74*/      {"CALL", "a"       , _41, REFFLAG | CODEREF},
/*75*/      {"",     ""        ,  0 , 0                },
/*76*/      {"JF1",  "j"       , _41, REFFLAG | CODEREF},
/*77*/      {"RR",   "A"       , _41, 0                },

/*78*/      {"ADDC", "A,R0"    , _41, 0                },
/*79*/      {"ADDC", "A,R1"    , _41, 0                },
/*7A*/      {"ADDC", "A,R2"    , _41, 0                },
/*7B*/      {"ADDC", "A,R3"    , _41, 0                },
/*7C*/      {"ADDC", "A,R4"    , _41, 0                },
/*7D*/      {"ADDC", "A,R5"    , _41, 0                },
/*7E*/      {"ADDC", "A,R6"    , _41, 0                },
/*7F*/      {"ADDC", "A,R7"    , _41, 0                },

/*80*/      {"",     ""        ,  0 , 0                },
/*81*/      {"",     ""        ,  0 , 0                },
/*82*/      {"",     ""        ,  0 , 0                },
/*83*/      {"RET",  ""        , _41, LFFLAG           },
/*84*/      {"JMP",  "a"       , _41, LFFLAG | REFFLAG | CODEREF},
/*85*/      {"CLR",  "F0"      , _41, 0                },
/*86*/      {"JOBF", "j"       , _41, REFFLAG | CODEREF},
/*87*/      {"",     ""        ,  0 , 0                },

/*88*/      {"",     ""        ,  0 , 0                },
/*89*/      {"ORL",  "P1,#b"   , _41, 0                },
/*8A*/      {"ORL",  "P2,#b"   , _41, 0                },
/*8B*/      {"",     ""        ,  0 , 0                },
/*8C*/      {"ORLD", "P4,A"    , _41, 0                },
/*8D*/      {"ORLD", "P5,A"    , _41, 0                },
/*8E*/      {"ORLD", "P6,A"    , _41, 0                },
/*8F*/      {"ORLD", "P7,A"    , _41, 0                },

/*90*/      {"MOVX", "@R0,A"   , _41, 0                }, // MOVX @R0,A          MOV  STS,A
/*91*/      {"",     ""        , _41, 0                },
/*92*/      {"JB4",  "j"       , _41, REFFLAG | CODEREF},
/*93*/      {"RETR", ""        , _41, LFFLAG           },
/*94*/      {"CALL", "a"       , _41, REFFLAG | CODEREF},
/*95*/      {"CPL",  "F0"      , _41, 0                },
/*96*/      {"JNZ",  "j"       , _41, REFFLAG | CODEREF},
/*97*/      {"CLR",  "C"       , _41, 0                },

/*98*/      {"",     ""        ,  0 , 0                },
/*99*/      {"ANL",  "P1,#b"   , _41, 0                },
/*9A*/      {"ANL",  "P2,#b"   , _41, 0                },
/*9B*/      {"",     ""        ,  0 , 0                },
/*9C*/      {"ANLD", "P4,A"    , _41, 0                },
/*9D*/      {"ANLD", "P5,A"    , _41, 0                },
/*9E*/      {"ANLD", "P6,A"    , _41, 0                },
/*9F*/      {"ANLD", "P7,A"    , _41, 0                },

/*A0*/      {"MOV",  "@R0,A"   , _41, 0                },
/*A1*/      {"MOV",  "@R1,A"   , _41, 0                },
/*A2*/      {"",     ""        ,  0 , 0                },
/*A3*/      {"MOVP", "A,@A"    , _41, 0                },
/*A4*/      {"JMP",  "a"       , _41, LFFLAG | REFFLAG | CODEREF},
/*A5*/      {"CLR",  "F1"      , _41, 0                },
/*A6*/      {"",     ""        ,  0 , 0                },
/*A7*/      {"CPL",  "C"       , _41, 0                },

/*A8*/      {"MOV",  "R0,A"    , _41, 0                },
/*A9*/      {"MOV",  "R1,A"    , _41, 0                },
/*AA*/      {"MOV",  "R2,A"    , _41, 0                },
/*AB*/      {"MOV",  "R3,A"    , _41, 0                },
/*AC*/      {"MOV",  "R4,A"    , _41, 0                },
/*AD*/      {"MOV",  "R5,A"    , _41, 0                },
/*AE*/      {"MOV",  "R6,A"    , _41, 0                },
/*AF*/      {"MOV",  "R7,A"    , _41, 0                },

/*B0*/      {"MOV",  "@R0,#b"  , _41, 0                },
/*B1*/      {"MOV",  "@R1,#b"  , _41, 0                },
/*B2*/      {"JB5",  "j"       , _41, REFFLAG | CODEREF},
/*B3*/      {"JMPP", "@A"      , _41, LFFLAG           },
/*B4*/      {"CALL", "a"       , _41, REFFLAG | CODEREF},
/*B5*/      {"CPL",  "F1"      , _41, 0                },
/*B6*/      {"JF0",  "j"       , _41, REFFLAG | CODEREF},
/*B7*/      {"",     ""        ,  0 , 0                },

/*B8*/      {"MOV",  "R0,#b"   , _41, 0                },
/*B9*/      {"MOV",  "R1,#b"   , _41, 0                },
/*BA*/      {"MOV",  "R2,#b"   , _41, 0                },
/*BB*/      {"MOV",  "R3,#b"   , _41, 0                },
/*BC*/      {"MOV",  "R4,#b"   , _41, 0                },
/*BD*/      {"MOV",  "R5,#b"   , _41, 0                },
/*BE*/      {"MOV",  "R6,#b"   , _41, 0                },
/*BF*/      {"MOV",  "R7,#b"   , _41, 0                },

/*C0*/      {"",     ""        ,  0 , 0                },
/*C1*/      {"",     ""        ,  0 , 0                },
/*C2*/      {"",     ""        ,  0 , 0                },
/*C3*/      {"",     ""        ,  0 , 0                },
/*C4*/      {"JMP",  "a"       , _41, LFFLAG | REFFLAG | CODEREF},
/*C5*/      {"SEL",  "RB0"     , _41, 0                },
/*C6*/      {"JZ",   "j"       , _41, REFFLAG | CODEREF},
/*C7*/      {"MOV",  "A,PSW"   , _41, 0                },

/*C8*/      {"DEC",  "R0"      , _41, 0                },
/*C9*/      {"DEC",  "R1"      , _41, 0                },
/*CA*/      {"DEC",  "R2"      , _41, 0                },
/*CB*/      {"DEC",  "R3"      , _41, 0                },
/*CC*/      {"DEC",  "R4"      , _41, 0                },
/*CD*/      {"DEC",  "R5"      , _41, 0                },
/*CE*/      {"DEC",  "R6"      , _41, 0                },
/*CF*/      {"DEC",  "R7"      , _41, 0                },

/*D0*/      {"XRL",  "A,@R0"   , _41, 0                },
/*D1*/      {"XRL",  "A,@R1"   , _41, 0                },
/*D2*/      {"JB6",  "j"       , _41, REFFLAG | CODEREF},
/*D3*/      {"XRL",  "A,#b"    , _41, 0                },
/*D4*/      {"CALL", "a"       , _41, REFFLAG | CODEREF},
/*D5*/      {"SEL",  "RB1"     , _41, 0                },
/*D6*/      {"JNIBF","j"       , _41, REFFLAG | CODEREF},
/*D7*/      {"MOV",  "PSW,A"   , _41, 0                },

/*D8*/      {"XRL",  "A,R0"    , _41, 0                },
/*D9*/      {"XRL",  "A,R1"    , _41, 0                },
/*DA*/      {"XRL",  "A,R2"    , _41, 0                },
/*DB*/      {"XRL",  "A,R3"    , _41, 0                },
/*DC*/      {"XRL",  "A,R4"    , _41, 0                },
/*DD*/      {"XRL",  "A,R5"    , _41, 0                },
/*DE*/      {"XRL",  "A,R6"    , _41, 0                },
/*DF*/      {"XRL",  "A,R7"    , _41, 0                },

/*E0*/      {"",     ""        ,  0 , 0                },
/*E1*/      {"",     ""        ,  0 , 0                },
/*E2*/      {"",     ""        ,  0 , 0                },
/*E3*/      {"MOVP3","A,@A"    , _41, 0                },
/*E4*/      {"JMP",  "a"       , _41, LFFLAG | REFFLAG | CODEREF},
/*E5*/      {"EN",   "DMA"     , _41, 0                },
/*E6*/      {"JNC",  "j"       , _41, REFFLAG | CODEREF},
/*E7*/      {"RL",   "A"       , _41, 0                },

/*E8*/      {"DJNZ", "R0,j"    , _41, REFFLAG | CODEREF},
/*E9*/      {"DJNZ", "R1,j"    , _41, REFFLAG | CODEREF},
/*EA*/      {"DJNZ", "R2,j"    , _41, REFFLAG | CODEREF},
/*EB*/      {"DJNZ", "R3,j"    , _41, REFFLAG | CODEREF},
/*EC*/      {"DJNZ", "R4,j"    , _41, REFFLAG | CODEREF},
/*ED*/      {"DJNZ", "R5,j"    , _41, REFFLAG | CODEREF},
/*EE*/      {"DJNZ", "R6,j"    , _41, REFFLAG | CODEREF},
/*EF*/      {"DJNZ", "R7,j"    , _41, REFFLAG | CODEREF},

/*F0*/      {"MOV",  "A,@R0"   , _41, 0                },
/*F1*/      {"MOV",  "A,@R1"   , _41, 0                },
/*F2*/      {"JB7",  "j"       , _41, REFFLAG | CODEREF},
/*F3*/      {"",     ""        ,  0 , 0                },
/*F4*/      {"CALL", "a"       , _41, REFFLAG | CODEREF},
/*F5*/      {"EN",   "FLAGS"   , _41, 0                },
/*F6*/      {"JC",   "j"       , _41, REFFLAG | CODEREF},
/*F7*/      {"RLC",  "A"       , _41, 0                },

/*F8*/      {"MOV",  "A,R0"    , _41, 0                },
/*F9*/      {"MOV",  "A,R1"    , _41, 0                },
/*FA*/      {"MOV",  "A,R2"    , _41, 0                },
/*FB*/      {"MOV",  "A,R3"    , _41, 0                },
/*FC*/      {"MOV",  "A,R4"    , _41, 0                },
/*FD*/      {"MOV",  "A,R5"    , _41, 0                },
/*FE*/      {"MOV",  "A,R6"    , _41, 0                },
/*FF*/      {"MOV",  "A,R7"    , _41, 0                },
};


// search backwards to find SEL MB0/SEL MB1 to dentify JMP/CALL bank
// only search as long as unbroken code for the current CPU is found
// if none found, caller will assume it's already "correct" and use current bank
// returns 0 if SEL MB0 found, 1 if SEL MB1 found, or -1 if neither
int Dis8048::selmb(addr_t addr)
{
    while ((_subtype == CPU_8048) && !rom.AddrOutRange(addr)) {
        // search for previous instr start byte:
        addr--;
        while (!rom.AddrOutRange(addr) && rom.test_attr(addr, ATTR_CONT)) {
            // it's a continuation byte, keep going back
            addr--;
        }

        // check CPU type and return "unknown" if it's not for this CPU
        if (rom.AddrOutRange(addr) || rom.get_type(addr) != _id) {
            return -1;
        }

        // stop searching if instr has lfflag set
        if (I8048_opcdTable[rom.get_data(addr)].lfref & LFFLAG) {
            return -1;
        }

        // is it a SEL MBx instruction?
        if ((rom.get_data(addr) & 0xEF) == 0xE5) {
            // E5 = SEL MB0 -> 0 / F5 = SEL MB1 -> 1
            return rom.get_data(addr) == 0xF5;
        }
    }

    // ran past start of code image, return "unknown"
    return -1;
}


int Dis8048::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
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
    switch(_subtype) {
        default:       assert(false); FALLTHROUGH; // invalid subtype
        case CPU_8021:
        case CPU_8048: instr = &I8048_opcdTable[opcd]; break;
        case CPU_8041: instr = &I8041_opcdTable[opcd]; break;
    }

    strcpy(opcode, instr->op);
    strcpy(parms, instr->parms);
    lfref   = instr->lfref;
    refaddr = 0;

    // check that opcode is valid for the CPU type
    if (!(instr->cpu & _subtype)) {
        opcode[0] = 0;
    }

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

            case 'j':   // 8048 short jump
                ra = ReadByte(ad++);
                ra += ad & 0xFF00;
                len++;

                RefStr(ra, p, lfref, refaddr);
                p += strlen(p);
                break;

            case 'a':   // 8048 long jump
                // determine A11 from SEL MBx hints
                switch (selmb(addr)) {
                    case 0: // SEL MB0
                        ra = 0x0000;      break;
                    case 1: // SEL MB1
                        ra = 0x0800;      break;
                    default: // floating
                        ra = ad & 0x0800; break;
                }
                // get A0-A10 from opcode, keep A12-A15 unchanged
                ra |= (ad & 0xF000) | ((opcd << 3) & 0x0700);
                ra |= ReadByte(ad++);
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
            int po = ReadByte(prev);
            if (prev != addr - 1) {
                //first instruction was more than one byte
                po = (po << 8) | ReadByte(prev + 1);
            }
            int ops = (po << 8) | op;    // put the two opcodes together

            switch (ops) {
                case 0x0000: // two NOP in a row
                case 0xFFFF: // two MOV A,R7 in a row
                    lfref |= RIPSTOP;
                    break;
            }
        }
    }

    // invalid instruction handler, including the case where it ran out of bytes
    if (opcode[0]==0 || rom.AddrOutRange(addr)) {
        strcpy(opcode, "???");
        H2Str(ReadByte(addr), parms);
        len     = 0;
        lfref   = 0;
        refaddr = 0;
    }

    return len;
}

