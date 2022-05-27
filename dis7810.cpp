// dis7810.c
// disassembler for NEC D78C10/C13/C14

static const char versionName[] = "NEC D78C10 disassembler";

#include "discpu.h"

class Dis7810 : public CPU {
public:
    Dis7810(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


Dis7810 cpu_7810("D78C10", 0, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DL");


Dis7810::Dis7810(const char *name, int subtype, int endian, int addrwid,
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
    uint16_t    opcode; // opcode: xx00=8-bit instr, xxyy=16-bit instr
    uint16_t    mask;   //   mask: 0=end, low byte=00 for 8-bit opcode
    const char  *op;    // mnemonic
    const char  *parms; // parms
    uint8_t     lfref;  // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;



// =====================================================
// first byte in bits 15-8, second byte in bits 7-0
static const struct InstrRec *FindInstr(const struct InstrRec *table, uint16_t opcode)
{
    for ( ; table->mask; table++) {
        if (table->opcode == (opcode & table->mask)) {
            return table;
        }
    }

    return NULL;
}



static const struct InstrRec opcdTable_7810[] =
{
    //  op     mask    opcd    parms   lfref
    { 0x0000, 0xFF00, "NOP"  , ""    , 0 },
    { 0x0100, 0xFF00, "LDAW" , "b"   , 0 },
    { 0x0200, 0xFF00, "INX"  , "SP"  , 0 },
    { 0x0300, 0xFF00, "DCX"  , "SP"  , 0 },
    { 0x0400, 0xFF00, "LXI"  , "SP,w", 0 },
    { 0x0500, 0xFF00, "ANIW" , "b,#b", 0 },
//  { 0x0600, 0xFF00, "__I"  , "A,#b", 0 },
    { 0x0700, 0xFF00, "ANI"  , "A,#b", 0 },
    { 0x0800, 0xF800, "MOV"  , "A,r" , 0 }, // 08-0F

    { 0x1000, 0xFF00, "EXA"  , ""    , 0 },
    { 0x1100, 0xFF00, "EXX"  , ""    , 0 },
    { 0x1200, 0xFF00, "INX"  , "B"   , 0 },
    { 0x1300, 0xFF00, "DCX"  , "B"   , 0 },
    { 0x1400, 0xFF00, "LXI"  , "B,w" , 0 },
    { 0x1500, 0xFF00, "ORIW" , "b,#b", 0 },
    { 0x1600, 0xFF00, "XRI"  , "A,#b", 0 },
    { 0x1700, 0xFF00, "ORI"  , "A,#b", 0 },
    { 0x1800, 0xF800, "MOV"  , "r1,A", 0 }, // 18-1F

    { 0x2000, 0xFF00, "INRW" , "b"   , 0 },
    { 0x2100, 0xFF00, "JB"   , ""    , 0 },
    { 0x2200, 0xFF00, "INX"  , "D"   , 0 },
    { 0x2300, 0xFF00, "DCX"  , "D"   , 0 },
    { 0x2400, 0xFF00, "LXI"  , "D,w" , 0 },
    { 0x2500, 0xFF00, "GTIW" , "b,#b", 0 },
    { 0x2600, 0xFF00, "ADINC", "A,#b", 0 },
    { 0x2700, 0xFF00, "GTI"  , "A,#b", 0 },
    // 29-2F / AB-AF STAX rpa2

    { 0x3000, 0xFF00, "DCRW" , "b"   , 0 },
    { 0x3100, 0xFF00, "BLOCK", ""    , 0 },
    { 0x3200, 0xFF00, "INX"  , "H"   , 0 },
    { 0x3300, 0xFF00, "DCX"  , "H"   , 0 },
    { 0x3400, 0xFF00, "LXI"  , "H,w" , 0 },
    { 0x3500, 0xFF00, "LTIW" , "b,#b", 0 },
    { 0x3600, 0xFF00, "SUINB", "A,#b", 0 },
    { 0x3700, 0xFF00, "LTI"  , "A,#b", 0 },
    // 39-3F / BB-BF STAX rpa2

    { 0x4000, 0xFF00, "CALL" , "w"   , REFFLAG | CODEREF },
    { 0x4100, 0xFF00, "INR"  , "A"   , 0 },
    { 0x4200, 0xFF00, "INR"  , "B"   , 0 },
    { 0x4300, 0xFF00, "INR"  , "C"   , 0 },
    { 0x4400, 0xFF00, "LXI"  , "EA,w", 0 },
    { 0x4500, 0xFF00, "ONIW" , "b,#b", 0 },
    { 0x4600, 0xFF00, "ADI"  , "A,#b", 0 },
    { 0x4700, 0xFF00, "ONI"  , "A,#b", 0 },
    // 48 - 2-byte
    { 0x4900, 0xFF00, "MVIX" , "B"   , 0 },
    { 0x4A00, 0xFF00, "MVIX" , "D"   , 0 },
    { 0x4B00, 0xFF00, "MVIX" , "H"   , 0 },
    { 0x4CC0, 0xFFC0, "MOV"  , "A,s" , 0 }, // 4CC0 MOV A,sr
    { 0x4DC0, 0xFFC0, "MOV"  , "s,A" , 0 }, // 4DC0 MOV sr,A
    { 0x4E00, 0xFE00, "JRE"  , "e"   , LFFLAG | REFFLAG | CODEREF }, // 4E-4F

    { 0x5000, 0xFF00, "EXH"  , ""    , 0 },
    { 0x5100, 0xFF00, "DCR"  , "A"   , 0 },
    { 0x5200, 0xFF00, "DCR"  , "B"   , 0 },
    { 0x5300, 0xFF00, "DCR"  , "C"   , 0 },
    { 0x5400, 0xFF00, "JMP"  , "w"   , LFFLAG | REFFLAG | CODEREF },
    { 0x5500, 0xFF00, "OFFIW", "b,#b", 0 },
    { 0x5600, 0xFF00, "ACI"  , "A,#b", 0 },
    { 0x5700, 0xFF00, "OFFI" , "A,#b", 0 },
    { 0x5800, 0xF800, "BIT"  , "z,b" , 0 }, // 58-5F

    // 60 - 2-byte
    { 0x6100, 0xFF00, "DAA"  , ""    , 0 },
    { 0x6200, 0xFF00, "RETI" , ""    , LFFLAG },
    { 0x6300, 0xFF00, "STAW" , "b"   , 0 },
    // 64 - 2-byte
    { 0x6500, 0xFF00, "NEIW" , "b,#b", 0 },
    { 0x6600, 0xFF00, "SUI"  , "A,#b", 0 },
    { 0x6700, 0xFF00, "NEI"  , "A,#b", 0 },
    { 0x6800, 0xF800, "MVI"  , "r,#b", 0 }, // 68-6F MVI r,b

    // 70 - 2-byte
    { 0x7100, 0xFF00, "MVIW" , "b,#b", 0 },
    { 0x7200, 0xFF00, "SOFTI", ""    , 0 },
// 73 ???
    // 74 - 2-byte
    { 0x7500, 0xFF00, "EQIW" , "b,#b", 0 },
    { 0x7600, 0xFF00, "SBI"  , "A,#b", 0 },
    { 0x7700, 0xFF00, "EQI"  , "A,#b", 0 },
    { 0X7800, 0xF800, "CALF" , "c"   , REFFLAG | CODEREF }, // 78-7F

    { 0x8000, 0xE000, "CALT" , "t"   , REFFLAG }, // 80-9F

    { 0xA000, 0xFF00, "POP"  , "V"   , 0 },
    { 0xA100, 0xFF00, "POP"  , "B"   , 0 },
    { 0xA200, 0xFF00, "POP"  , "D"   , 0 },
    { 0xA300, 0xFF00, "POP"  , "H"   , 0 },
    { 0xA400, 0xFF00, "POP"  , "EA"  , 0 },
    { 0xA500, 0xFF00, "DMOV" , "EA,B", 0 },
    { 0xA600, 0xFF00, "DMOV" , "EA,D", 0 },
    { 0xA700, 0xFF00, "DMOV" , "EA,H", 0 },
    { 0xA800, 0xFF00, "INX"  , "EA"  , 0 },
    { 0xA900, 0xFF00, "DCX"  , "EA"  , 0 },
    { 0xAA00, 0xFF00, "EI"   , ""    , 0 },
    // 29-2F / AB-AF STAX rpa2
    { 0x2800, 0x7800, "LDAX" , "a2"  , 0 }, // 29-2F / AB-AF

    { 0xB000, 0xFF00, "PUSH" , "V"   , 0 },
    { 0xB100, 0xFF00, "PUSH" , "B"   , 0 },
    { 0xB200, 0xFF00, "PUSH" , "D"   , 0 },
    { 0xB300, 0xFF00, "PUSH" , "H"   , 0 },
    { 0xB400, 0xFF00, "PUSH" , "EA"  , 0 },
    { 0xB500, 0xFF00, "DMOV" , "B,EA", 0 },
    { 0xB600, 0xFF00, "DMOV" , "D,EA", 0 },
    { 0xB700, 0xFF00, "DMOV" , "H,EA", 0 },
    { 0xB800, 0xFF00, "RET"  , ""    , LFFLAG },
    { 0xB900, 0xFF00, "RETS" , ""    , LFFLAG },
    { 0xBA00, 0xFF00, "DI"   , ""    , 0 },
    // 39-3F / BB-BF STAX rpa2
    { 0x3800, 0x7800, "STAX" , "a2"  , 0 }, // 39-3F / BB-BF

    { 0xC000, 0xC000, "JR"   , "j"   , LFFLAG | REFFLAG | CODEREF }, // C0-FF

//  === 48 xx ===

    { 0x4800, 0xFFFC, "SLRC" , "r2" , 0 }, // 4801-4803
    { 0x4804, 0xFFFC, "SLLC" , "r2" , 0 }, // 4805-4808

    { 0x4808, 0xFFF8, "SK"   , "f"  , 0 }, // 480A/480B/480C
    { 0x4818, 0xFFF8, "SKN"  , "f"  , 0 }, // 481A/481B/481C

    { 0x4820, 0xFFFC, "SLR"  , "r2" , 0 }, // 4821-4823
    { 0x4824, 0xFFFC, "SLL"  , "r2" , 0 }, // 4825-4827
    { 0x4828, 0xFFFF, "JEA"  , ""   , 0 },
    { 0x4829, 0xFFFF, "CALB" , ""   , 0 },
    { 0x482A, 0xFFFF, "CLC"  , ""   , 0 },
    { 0x482B, 0xFFFF, "STC"  , ""   , 0 },
    { 0x482C, 0xFFFC, "MUL"  , "r2" , 0 }, // 482D-482F

    { 0x4830, 0xFFFC, "RLR"  , "r2" , 0 }, // 4831-4833
    { 0x4834, 0xFFFC, "RLL"  , "r2" , 0 }, // 4835-4837
    { 0x4838, 0xFFFF, "RLD"  , ""   , 0 },
    { 0x4839, 0xFFFF, "RRD"  , ""   , 0 },
    { 0x483A, 0xFFFF, "NEGA" , ""   , 0 },
    { 0x483B, 0xFFFF, "HALT" , ""   , 0 },
    { 0x483C, 0xFFFC, "DIV"  , "r2" , 0 }, // 483D-483F

    { 0x48A0, 0xFFFF, "DSLR" , "EA" , 0 },
    { 0x48A4, 0xFFFF, "DSLL" , "EA" , 0 },
    { 0x48B0, 0xFFFF, "DRLR" , "EA" , 0 },
    { 0x48B4, 0xFFFF, "DRLL" , "EA" , 0 },
    { 0x48BB, 0xFFFF, "STOP" , ""   , 0 },

    { 0x4840, 0xFFE0, "SKIT" , "i"  , 0 }, // 4840-485F
    { 0x4840, 0xFFE0, "SKNIT", "i"  , 0 }, // 4860-487F

    { 0x4880, 0xFFF0, "LDEAX", "a3" , 0 },
    { 0x4890, 0xFFF0, "STEAX", "a3" , 0 },

    { 0x48A0, 0xFFFF, "DSLR" , "EA" , 0 },
    { 0x48A4, 0xFFFF, "DSLL" , "EA" , 0 },

    { 0x48B0, 0xFFFF, "DRLR" , "EA" , 0 },
    { 0x48B4, 0xFFFF, "DRLL" , "EA" , 0 },
    { 0x48B8, 0xFFFF, "STOP" , ""   , 0 },

    //48 C0+sr4	DMOV EA,sr4
    { 0x48C0, 0xFFFF, "DMOV", "EA,ECNT", 0 },
    { 0x48C1, 0xFFFF, "DMOV", "EA,ECPT", 0 },

    //48 D2+sr3	DMOV sr3,EA
    { 0x48D2, 0xFFFF, "DMOV", "ETM0,EA", 0 },
    { 0x48D3, 0xFFFF, "DMOV", "ETM1,EA", 0 },

//  === 60 xx ===

//  { 0x6000, 0xFFF8, "__A"  , "r,A" , 0 },
    { 0x6008, 0xFFF8, "ANA"  , "r,A" , 0 },
    { 0x6010, 0xFFF8, "XRA"  , "r,A" , 0 },
    { 0x6018, 0xFFF8, "ORA"  , "r,A" , 0 },
    { 0x6020, 0xFFF8, "ADDNC", "r,A" , 0 },
    { 0x6028, 0xFFF8, "GTA"  , "r,A" , 0 },
    { 0x6030, 0xFFF8, "SUBNB", "r,A" , 0 },
    { 0x6038, 0xFFF8, "LTA"  , "r,A" , 0 },

    { 0x6040, 0xFFF8, "ADD"  , "r,A" , 0 },
//  { 0x6048, 0xFFF8, "__A"  , "r,A" , 0 },
    { 0x6050, 0xFFF8, "ADC"  , "r,A" , 0 },
//  { 0x6058, 0xFFF8, "__A"  , "r,A" , 0 },
    { 0x6060, 0xFFF8, "SUB"  , "r,A" , 0 },
    { 0x6068, 0xFFF8, "NEA"  , "r,A" , 0 },
    { 0x6070, 0xFFF8, "SUBB" , "r,A" , 0 },
    { 0x6078, 0xFFF8, "EQA"  , "r,A" , 0 },

//  { 0x6080, 0xFFF8, "__A"  , "A,r" , 0 },
    { 0x6088, 0xFFF8, "ANA"  , "A,r" , 0 },
    { 0x6090, 0xFFF8, "XRA"  , "A,r" , 0 },
    { 0x6098, 0xFFF8, "ORA"  , "A,r" , 0 },
    { 0x60A0, 0xFFF8, "ADDNC", "A,r" , 0 },
    { 0x60A8, 0xFFF8, "GTA"  , "A,r" , 0 },
    { 0x60B0, 0xFFF8, "SUBNB", "A,r" , 0 },
    { 0x60B8, 0xFFF8, "LTA"  , "A,r" , 0 },

    { 0x60C0, 0xFFF8, "ADD"  , "A,r" , 0 },
//  { 0x60C8, 0xFFF8, "__A"  , "A,r" , 0 },
    { 0x60D0, 0xFFF8, "ADC"  , "A,r" , 0 },
//  { 0x60D8, 0xFFF8, "__A"  , "A,r" , 0 },
    { 0x60E0, 0xFFF8, "SUB"  , "A,r" , 0 },
    { 0x60E8, 0xFFF8, "NEA"  , "A,r" , 0 },
    { 0x60F0, 0xFFF8, "SUBB" , "A,r" , 0 },
    { 0x60F8, 0xFFF8, "EQA"  , "A,r" , 0 },

//  === 64 xx ===

    { 0x6400, 0xFF78, "MVI"  , "s2,#b", 0 }, // 640x / 648x
    { 0x6408, 0xFF78, "ANI"  , "s2,#b", 0 },
    { 0x6410, 0xFF78, "XRI"  , "s2,#b", 0 }, // 641x / 649x
    { 0x6418, 0xFF78, "ORI"  , "s2,#b", 0 },
    { 0x6420, 0xFF78, "ADINC", "s2,#b", 0 }, // 642x / 64Ax
    { 0x6428, 0xFF78, "GTI"  , "s2,#b", 0 },
    { 0x6430, 0xFF78, "SUINB", "s2,#b", 0 }, // 643x / 64Bx
    { 0x6438, 0xFF78, "LTI"  , "s2,#b", 0 },

    { 0x6440, 0xFF78, "ADI"  , "s2,#b", 0 }, // 644x / 64Cx
    { 0x6448, 0xFF78, "ONI"  , "s2,#b", 0 },
    { 0x6450, 0xFF78, "ACI"  , "s2,#b", 0 }, // 645x / 64Dx
    { 0x6458, 0xFF78, "OFFI" , "s2,#b", 0 },
    { 0x6460, 0xFF78, "SUI"  , "s2,#b", 0 }, // 646x / 64Ex
    { 0x6468, 0xFF78, "NEI"  , "s2,#b", 0 },
    { 0x6470, 0xFF78, "SBI"  , "s2,#b", 0 }, // 647x / 64Fx
    { 0x6478, 0xFF78, "EQI"  , "s2,#b", 0 },

//  === 70 xx ===

    { 0x700E, 0xFFFF, "SSPD" , "w"    , 0 },
    { 0x700F, 0xFFFF, "LSPD" , "w"    , 0 },
    { 0x701E, 0xFFFF, "SBCD" , "w"    , 0 },
    { 0x701F, 0xFFFF, "LBCD" , "w"    , 0 },
    { 0x702E, 0xFFFF, "SDED" , "w"    , 0 },
    { 0x702F, 0xFFFF, "LDED" , "w"    , 0 },
    { 0x703E, 0xFFFF, "SHLD" , "w"    , 0 },
    { 0x703F, 0xFFFF, "LHLD" , "w"    , 0 },

    { 0x7040, 0xFFFC, "EADD" , "EA,r2", 0 },
    { 0x7060, 0xFFFC, "ESUB" , "EA,r2", 0 },
    { 0x7068, 0xFFF8, "MOV"  , "r,w"  , 0 },
    { 0x7078, 0xFFF8, "MOV"  , "w,r"  , 0 },

//  { 0x7080, 0xFFF8, "___X"  , "a"   , 0 },
    { 0x7088, 0xFFF8, "ANAX"  , "a"   , 0 },
    { 0x7090, 0xFFF8, "XRAX"  , "a"   , 0 },
    { 0x7098, 0xFFF8, "ORAX"  , "a"   , 0 },
    { 0x70A0, 0xFFF8, "ADDNCX", "a"   , 0 },
    { 0x70A8, 0xFFF8, "GTAX"  , "a"   , 0 },
    { 0x70B0, 0xFFF8, "SUBNBX", "a"   , 0 },
    { 0x70B8, 0xFFF8, "LTAX"  , "a"   , 0 },
    { 0x70C0, 0xFFF8, "ADDX"  , "a"   , 0 },
    { 0x70C8, 0xFFF8, "ONAX"  , "a"   , 0 },
    { 0x70D0, 0xFFF8, "ADDCX" , "a"   , 0 },
    { 0x70D8, 0xFFF8, "OFFAX" , "a"   , 0 },
    { 0x70E0, 0xFFF8, "SUBX"  , "a"   , 0 },
    { 0x70E8, 0xFFF8, "SBBX"  , "a"   , 0 },
    { 0x70F0, 0xFFF8, "NEAX"  , "a"   , 0 },
    { 0x70F8, 0xFFF8, "EQAX"  , "a"   , 0 },

//  === 74 xx ===

    { 0x7400, 0xFFF8, "ANI"  , "r,#b" , 0 },
//  { 0x7408, 0xFFF8, "__I"  , "r,#b" , 0 },
    { 0x7410, 0xFFF8, "ANI"  , "r,#b" , 0 },
    { 0x7418, 0xFFF8, "ORI"  , "r,#b" , 0 },
    { 0x7420, 0xFFF8, "ADINC", "r,#b" , 0 },
    { 0x7428, 0xFFF8, "GTI"  , "r,#b" , 0 },
    { 0x7430, 0xFFF8, "SUINB", "r,#b" , 0 },
    { 0x7438, 0xFFF8, "LTI"  , "r,#b" , 0 },
    { 0x7440, 0xFFF8, "ADI"  , "r,#b" , 0 },
    { 0x7448, 0xFFF8, "ONI"  , "r,#b" , 0 },
    { 0x7450, 0xFFF8, "ACI"  , "r,#b" , 0 },
    { 0x7458, 0xFFF8, "OFFI" , "r,#b" , 0 },
    { 0x7460, 0xFFF8, "SUI"  , "r,#b" , 0 },
    { 0x7468, 0xFFF8, "NEI"  , "r,#b" , 0 },
    { 0x7470, 0xFFF8, "SBI"  , "r,#b" , 0 },
    { 0x7478, 0xFFF8, "EQI"  , "r,#b" , 0 },

//  { 0x7480, 0xFFFF, "___W"  , "b"   , 0 },
//  { 0x7484, 0xFFFC, "D___"  , "p"   , 0 },
    { 0x7488, 0xFFFF, "ANAW"  , "b"   , 0 },
    { 0x748C, 0xFFFC, "DAN"   , "EA,p", 0 },
    { 0x7490, 0xFFFF, "XRAW"  , "b"   , 0 },
    { 0x7494, 0xFFFC, "DXR"   , "EA,p", 0 },
    { 0x7498, 0xFFFF, "ORAW"  , "b"   , 0 },
    { 0x749C, 0xFFFC, "DOR"   , "EA,p", 0 },
    { 0x74A0, 0xFFFF, "ADDNCW", "b"   , 0 },
    { 0x74A4, 0xFFFC, "DADDNC", "EA,p", 0 },
    { 0x74A8, 0xFFFF, "GTAW"  , "b"   , 0 },
    { 0x74AC, 0xFFFC, "DGT"   , "EA,p", 0 },
    { 0x74B0, 0xFFFF, "SUBNBW", "b"   , 0 },
    { 0x74B4, 0xFFFC, "DSUBNB", "EA,p", 0 },
    { 0x74B8, 0xFFFF, "LTAW"  , "b"   , 0 },
    { 0x74BC, 0xFFFC, "DLT"   , "EA,p", 0 },
    { 0x74C0, 0xFFFF, "ADDW"  , "b"   , 0 },
    { 0x74C4, 0xFFFC, "DADD"  , "EA,p", 0 },
    { 0x74C8, 0xFFFF, "ONAW"  , "b"   , 0 },
    { 0x74CC, 0xFFFC, "DON"   , "EA,p", 0 },
    { 0x74D0, 0xFFFF, "ADCW"  , "b"   , 0 },
    { 0x74D4, 0xFFFC, "DADC"  , "EA,p", 0 },
    { 0x74D8, 0xFFFF, "OFFAW" , "b"   , 0 },
    { 0x74DC, 0xFFFC, "DOFF"  , "EA,p", 0 },
    { 0x74E0, 0xFFFF, "SUBW"  , "b"   , 0 },
    { 0x74E4, 0xFFFC, "DSUB"  , "EA,p", 0 },
    { 0x74E8, 0xFFFF, "NEAW"  , "b"   , 0 },
    { 0x74EC, 0xFFFC, "DNE"   , "EA,p", 0 },
    { 0x74F0, 0xFFFF, "SBBW"  , "b"   , 0 },
    { 0x74F4, 0xFFFC, "DSBB"  , "EA,p", 0 },
    { 0x74F8, 0xFFFF, "EQAW"  , "b"   , 0 },
    { 0x74FC, 0xFFFC, "DEQ"   , "EA,p", 0 },

    {   0   ,   0   , ""      , ""    , 0 },
};


// r1 replaces V/A with EAH/EAL
//static const char *r[8] = { "V", "A", "B", "C", "D", "E", "H", "L"  };

// rp1 repaces SP with VA
static const char *rp[8] = { "SP", "BC", "DE", "HL", "EA", "", "", "" };

static const char *rpa[16] =
{
    "", "(BC)", "(DE)", "(HL)"  , "(DE)+" , "(HL)+" , "(DE)-"  , "(HL)-" ,
    "", ""    , ""    , "(DE+b)", "(HL+A)", "(HL+B)", "(HL+EA)", "(HL+b)"
};

static const char *rpa3[16] =
{
    "", "", "(DE)", "(HL)"  , "(DE)++", "(HL)++", "",        "",
    "", "", ""    , "(DE+b)", "(HL+A)", "(HL+B)", "(HL+EA)", "(HL+b)"
};

// register names for sr, sr1, and sr2
static const char *sr[64] =
{
/*00*/ "PA"  , "PB"  , "PC"  , "PD"  , ""    , "PF"  , "MKH" , "MKL" ,
/*08*/ "ANM" , "SMH" , "SML" , "EOM" , "ETMM", "TMM" , ""    , ""    ,
/*10*/ "MM"  , "MCC" , "MA"  , "MB"  , "MC"  , ""    , ""    , "MF"  , 
/*18*/ "TXB" , "RXB" , "TM0" , "TM1" , ""    , ""    , ""    , ""    , 
/*20*/ "CR0" , "CR1" , "CR2" , "CR3" , ""    , ""    , ""    , ""    , 
/*28*/ "ZCM" , ""    , ""    , ""    , ""    , ""    , ""    , ""    ,
/*30*/ ""    , ""    , ""    , ""    , ""    , ""    , ""    , ""    ,
/*38*/ ""    , ""    , ""    , ""    , ""    , ""    , ""    , ""
};

static const char *f[8] = { "", "", "CY", "HC", "Z", "", "", "" };

static const char *irf[32] =
{
/*00*/ "NMI", "FT0", "FT1", "F1"  , "F2"  , "FE0", "FE1", "FEIN",
/*08*/ "FAD", "FSR", "FS" , "ER"  , "OV"  , ""   , ""   , ""    ,
/*10*/ "AN4", "AN5", "AN6", "AN7" , "SB"  , ""   , ""   , ""    ,
/*18*/ ""   , ""   , ""    , ""   , ""    , ""    , ""   , ""
};


// =====================================================
int Dis7810::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
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
    int op = (ReadByte(ad) << 8) | ReadByte(ad+1);
    int len = 1;
    ad++;

    // get instruction information
    instr = FindInstr(opcdTable_7810, op);

    if (instr && (instr->mask & 0x00FF)) {
        // eat second byte if two-byte opcode
        ad++;
        len++;
    } else {
        // shift down register bits if one-byte opcode
        op >>= 8;
    }

    if (instr) {
        // handle opcode according to instruction type
        strcpy(opcode, instr->op);
        strcpy(parms,  instr->parms);
        lfref   = instr->lfref;
        refaddr = 0;

        // handle substitutions
        p = s;
        *p = 0;
        l = parms;
        while (*l && len) {
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

                // == CALT ==
                case 't':
                    ra = 0x80 | (op*2 & 0x1F);
                    RefStr(ra, p, lfref, refaddr);
                    p += strlen(p);
                    break;

                // == JRE ==
                case 'e': // 9-bit relative
                    i = ((op & 0x01) << 8) | ReadByte(ad++);
                    len++;
                    if (i >= 256) { // sign-extend
                        i = i - 512;
                    }
                    ra = (ad + i) & 0xFFFF;
                    RefStr(ra, p, lfref, refaddr);
                    p += strlen(p);
                    break;

                // == CALF ==
                case 'c': // indirect through fast call table
                    ra = ((op & 0x0F) << 8) | ReadByte(ad++);
                    len++;
                    RefStr(ra, p, lfref, refaddr);
                    p += strlen(p);
                    break;

                // == JR ==
                case 'j': // 6-bit relative
                    i = op & 0x3F;
                    if (i >= 32) { // sign-extend
                         i = i - 64;
                    }
                    ra = (ad + i) & 0xFFFF;
                    RefStr(ra, p, lfref, refaddr);
                    p += strlen(p);
                    break;

                // === r ===
                case 'r':
                    switch (l[1]) {
                        default: // r
                            i = op & 0x07;
                            *p++ = "VABCDEHL"[i];
                            break;
                        case '1': // r1
                            l++;
                            i = op & 0x07;
                            switch (i) {
                                case 0: // EAH
                                case 1: // EAL
                                    strcat(p, "EA");
                                    p += strlen(p);
                                break;
                                default:
                                    break;
                            }
                            *p++ = "HLBCDEHL"[i];
                            break;
                        case '2': // r2
                            l++;
                            i = op & 0x03;
                            if (i == 0) {
                                // 0 is invalid, return illegal instr
                            len = 0;
                                break;
                            }
                            *p++ = "ABC"[i-1];
                            break;
                    }
                    break;

                // == sr ==
                case 's':
                    if (l[1] == '2') { // sr2
                        l++;
                        i = ((op & 0x80) >> 4) | (op & 0x07);
                    } else { // sr / sr1
                        i = op & 0x3F;
                    }
                    if (sr[i] && sr[i][0]) {
                        strcat(p, sr[i]);
                    p += strlen(p);
                    } else {
                        // register name not found, return illegal instr
                        len = 0;
                    }
                    break;

                // == rp ==
                case 'p':
                    i = op & 0x03;
                    if (rp[i] && rp[i][0]) {
                        strcat(p, rp[i]);
                        p += strlen(p);
                    } else {
                        // register name not found, return illegal instr
                        len = 0;
                    }
                    break;

                // == rpa ==
                case 'a':
                    if (l[1] == '3') {
                        l++;
                        i = op & 0x0F;

                        if (rpa3[i] && rpa3[i][0]) {
                            strcat(p, rp[i]);
                            p += strlen(p);
                        } else {
                            // register name not found, return illegal instr
                            len = 0;
                            break;
                        }
                    } else {
                         switch (l[1]) {
                             default: // rpa
                                 i = op & 0x07;
                                 break;
                             case '1': // rpa1
                                 l++;
                             i = op & 0x03;
                             break;
                             case '2': // rpa2
                                 l++;
                                 i = ((op & 0x80) >> 4) | (op & 0x07);
                                 break;
                        }

                        if (rpa[i] && rpa[i][0]) {
                            strcat(p, rpa[i]);
                            p += strlen(p);
                        } else {
                            // register name not found, return illegal instr
                            len = 0;
                            break;
                        }
                    }
                    *p = 0;

                    // check for (HL+b) etc.
                    if (p[-2] == 'b') {
                        H2Str(ReadByte(ad++), p-2);
                        len++;
                        p += strlen(p);
                        *p++ = ')';
                    }
                    break;

                // == f ==
                case 'f':
                    i = op & 0x07;
                    if (f[i] && f[i][0]) {
                        strcat(p, f[i]);
                        p += strlen(p);
                    } else {
                        // flag name not found, return illegal instr
                        len = 0;
                    }
                    break;

                // == irf ==
                case 'i':
                    i = op & 0x07;
                    if (irf[i] && irf[i][0]) {
                        strcat(p, irf[i]);
                        p += strlen(p);
                    } else {
                        // flag name not found, return illegal instr
                        len = 0;
                    }
                    break;

                // == bit ==
                case 'z':
                    i = op & 0x07;
                    *p++ = '0' + i;
                    break;

                default:
                    *p++ = *l;
                    break;
            }
            *p = 0;
            l++;
        }

        strcpy(parms, s);
    }

    // invalid instruction handler, including the case where it ran out of bytes
    if (opcode==NULL || opcode[0]==0 || rom.AddrOutRange(addr)) {
        strcpy(opcode, "???");
        H2Str(ReadByte(addr), parms);
        len      = 0; // illegal instruction
        lfref    = 0;
        refaddr  = 0;
    }

    return len;
}


