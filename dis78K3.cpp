// dis78K3.c
// disassembler for NEC D78K3

static const char versionName[] = "NEC D78K3 disassembler";

#include "discpu.h"

class Dis78K3 : public CPU {
public:
    Dis78K3(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


Dis78K3 cpu_78K3("D78K3", 0, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DL");


Dis78K3::Dis78K3(const char *name, int subtype, int endian, int addrwid,
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


static const struct InstrRec opcdTable_78K3[] =
{
// *** remember to move page instrs to top?

    //  op     mask    opcd    parms     lfref

// 1
// MOV
    { 0x2BFE, 0xFFFF, "MOV"  , "PSWL,#b", 0 }, // special cases of sfr
    { 0x2BFF, 0xFFFF, "MOV"  , "PSWH,#b", 0 },
    { 0x12FE, 0xFFFF, "MOV"  , "PSWL,A" , 0 },
    { 0x12FF, 0xFFFF, "MOV"  , "PSWH,A" , 0 },
    { 0x10FE, 0xFFFF, "MOV"  , "A,PSWL" , 0 },
    { 0x10FF, 0xFFFF, "MOV"  , "A,PSWH" , 0 },

    { 0xB800, 0xF800, "MOV"  , "r1,#b"  , 0 },
    { 0x3A00, 0xFF00, "MOV"  , "s,#b"   , 0 },
    { 0x2B00, 0xFF00, "MOV"  , "f,#b"   , 0 },
    { 0x2400, 0xFF08, "MOV"  , "r,r1"   , 0 },
    { 0xD000, 0xF800, "MOV"  , "A,r1"   , 0 },
    { 0x2000, 0xFF00, "MOV"  , "A,s"    , 0 },
    { 0x2200, 0xFF00, "MOV"  , "s,A"    , 0 },
    { 0x3800, 0xFF00, "MOV"  , "s,s"    , 0 },
    { 0x1000, 0xFF00, "MOV"  , "A,f"    , 0 },
    { 0x1200, 0xFF00, "MOV"  , "f,A"    , 0 },
//  { 0x5800, 0xF800, "MOV"  , "A,m1"   , 0 }, // deferred to avoid 5E/5F
//  { 0x0000, 0xE08F, "MOV"  , "A,m"    , 0 },
//  { 0x5000, 0xF800, "MOV"  , "m1,A"   , 0 }, // deferred to avoid 56/57
//  { 0x0080, 0xE08F, "MOV"  , "m,A"    , 0 },
    { 0x1800, 0xFF00, "MOV"  , "A,[s]"  , 0 },
    { 0x1900, 0xFF00, "MOV"  , "[s],A"  , 0 },
    { 0x09F0, 0xFFFF, "MOV"  , "A,w"    , 0 },
    { 0x09F1, 0xFFFF, "MOV"  , "w,A"    , 0 },

// XCH
    { 0xD800, 0xF800, "XCH"  , "A,r1"   , 0 },
    { 0x2500, 0xFF08, "XCH"  , "r,r1"   , 0 },
//  { 0x0004, 0xE08F, "XCH"  , "A,m"    , 0 },
    { 0x2100, 0xFF00, "XCH"  , "A,s"    , 0 },
    { 0x0121, 0xFFFF, "XCH"  , "A,f"    , 0 },
    { 0x2300, 0xFF00, "XCH"  , "A,[s]"  , 0 },
    { 0x3900, 0xFF00, "XCH"  , "s,s"    , 0 },

// 2
// MOVW
    { 0x6000, 0xF800, "MOVW" , "p1,#w" , 0 },
    { 0x0C00, 0xFF00, "MOVW" , "s,#w"  , 0 },
    { 0x0B00, 0xFF00, "MOVW" , "f,#w"  , 0 },
    { 0x2408, 0xFF18, "MOVW" , "p,p1"  , 0 },
    { 0x1C00, 0xFF00, "MOVW" , "AX,s"  , 0 },
    { 0x1A00, 0xFF00, "MOVW" , "s,AX"  , 0 },
    { 0x3C00, 0xFF00, "MOVW" , "s,s"   , 0 },
    { 0x1100, 0xFF00, "MOVW" , "AX,f"  , 0 },
    { 0x1300, 0xFF00, "MOVW" , "f,AX"  , 0 },
    { 0x0980, 0xFFF8, "MOVW" , "r1,w"  , 0 },
    { 0x0990, 0xFFF8, "MOVW" , "w,r1"  , 0 },
//  { 0x0001, 0xE08F, "MOVW" , "AX,m"  , 0 }, //*** mem = special
//  { 0x0081, 0xE08F, "MOVW" , "m,AX"  , 0 }, //*** mem = special
// XCHW
    { 0x1B00, 0xFF00, "XCHW" , "AX,s"  , 0 },
    { 0x011B, 0xFFFF, "XCHW" , "AX,f"  , 0 },
    { 0x2A00, 0xFF00, "XCHW" , "s,s"   , 0 },
    { 0x2508, 0xFF18, "XCHW" , "p,p1"  , 0 },
//  { 0x0005, 0xE08F, "XCHW" , "AX,m"  , 0 }, //*** mem = special

// 3
// ADD  8
    { 0xA800, 0xFF00, "ADD"  , "A,#b" , 0 },
    { 0x6800, 0xFF00, "ADD"  , "s,#b" , 0 },
    { 0x0168, 0xFFFF, "ADD"  , "f,#b" , 0 },
    { 0x8800, 0xFF08, "ADD"  , "r,r1" , 0 },
    { 0x9800, 0xFF00, "ADD"  , "A,s"  , 0 },
    { 0x0198, 0xFFFF, "ADD"  , "A,f"  , 0 },
    { 0x7800, 0xFF00, "ADD"  , "s,s"  , 0 },
//  { 0x0008, 0xE08F, "ADD"  , "A,m"  , 0 },
//  { 0x0088, 0xE08F, "ADD"  , "m,A"  , 0 },
// ADDC 9
    { 0xA900, 0xFF00, "ADDC" , "A,#b" , 0 },
    { 0x6900, 0xFF00, "ADDC" , "s,#b" , 0 },
    { 0x0169, 0xFFFF, "ADDC" , "f,#b" , 0 },
    { 0x8900, 0xFF08, "ADDC" , "r,r1" , 0 },
    { 0x9900, 0xFF00, "ADDC" , "A,s"  , 0 },
    { 0x0199, 0xFFFF, "ADDC" , "A,f"  , 0 },
    { 0x7900, 0xFF00, "ADDC" , "s,s"  , 0 },
//  { 0x0009, 0xE08F, "ADDC" , "A,m"  , 0 },
//  { 0x0089, 0xE08F, "ADDC" , "m,A"  , 0 },
// SUB  A
    { 0xAA00, 0xFF00, "SUB"  , "A,#b" , 0 },
    { 0x6A00, 0xFF00, "SUB"  , "s,#b" , 0 },
    { 0x016A, 0xFFFF, "SUB"  , "f,#b" , 0 },
    { 0x8A00, 0xFF08, "SUB"  , "r,r1" , 0 },
    { 0x9A00, 0xFF00, "SUB"  , "A,s"  , 0 },
    { 0x019A, 0xFFFF, "SUB"  , "A,f"  , 0 },
    { 0x7A00, 0xFF00, "SUB"  , "s,s"  , 0 },
//  { 0x000A, 0xE08F, "SUB"  , "A,m"  , 0 },
//  { 0x008A, 0xE08F, "SUB"  , "m,A"  , 0 },
// SUBC B
    { 0xAB00, 0xFF00, "SUBC" , "A,#b" , 0 },
    { 0x6B00, 0xFF00, "SUBC" , "s,#b" , 0 },
    { 0x016B, 0xFFFF, "SUBC" , "f,#b" , 0 },
    { 0x8B00, 0xFF08, "SUBC" , "r,r1" , 0 },
    { 0x9B00, 0xFF00, "SUBC" , "A,s"  , 0 },
    { 0x019B, 0xFFFF, "SUBC" , "A,f"  , 0 },
    { 0x7B00, 0xFF00, "SUBC" , "s,s"  , 0 },
//  { 0x000B, 0xE08F, "SUBC" , "A,m"  , 0 },
//  { 0x008B, 0xE08F, "SUBC" , "m,A"  , 0 },
// AND  C
    { 0xAC00, 0xFF00, "AND"  , "A,#b" , 0 },
    { 0x6C00, 0xFF00, "AND"  , "s,#b" , 0 },
    { 0x016C, 0xFFFF, "AND"  , "f,#b" , 0 },
    { 0x8C00, 0xFF08, "AND"  , "r,r1" , 0 },
    { 0x9C00, 0xFF00, "AND"  , "A,s"  , 0 },
    { 0x019C, 0xFFFF, "AND"  , "A,f"  , 0 },
    { 0x7C00, 0xFF00, "AND"  , "s,s"  , 0 },
//  { 0x000C, 0xE08F, "AND"  , "A,m"  , 0 },
//  { 0x008C, 0xE08F, "AND"  , "m,A"  , 0 },
// OR   E
    { 0xAE00, 0xFF00, "OR"   , "A,#b" , 0 },
    { 0x6E00, 0xFF00, "OR"   , "s,#b" , 0 },
    { 0x016E, 0xFFFF, "OR"   , "f,#b" , 0 },
    { 0x8E00, 0xFF08, "OR"   , "r,r1" , 0 },
    { 0x9E00, 0xFF00, "OR"   , "A,s"  , 0 },
    { 0x019E, 0xFFFF, "OR"   , "A,f"  , 0 },
    { 0x7E00, 0xFF00, "OR"   , "s,s"  , 0 },
//  { 0x000E, 0xE08F, "OR"   , "A,m"  , 0 },
//  { 0x008E, 0xE08F, "OR"   , "m,A"  , 0 },
// XOR  D
    { 0xAD00, 0xFF00, "XOR"  , "A,#b" , 0 },
    { 0x6D00, 0xFF00, "XOR"  , "s,#b" , 0 },
    { 0x016D, 0xFFFF, "XOR"  , "f,#b" , 0 },
    { 0x8D00, 0xFF08, "XOR"  , "r,r1" , 0 },
    { 0x9D00, 0xFF00, "XOR"  , "A,s"  , 0 },
    { 0x019D, 0xFFFF, "XOR"  , "A,f"  , 0 },
    { 0x7D00, 0xFF00, "XOR"  , "s,s"  , 0 },
//  { 0x000D, 0xE08F, "XOR"  , "A,m"  , 0 },
//  { 0x008D, 0xE08F, "XOR"  , "m,A"  , 0 },
// CMP  F
    { 0xAF00, 0xFF00, "CMP"  , "A,#b" , 0 },
    { 0x6F00, 0xFF00, "CMP"  , "s,#b" , 0 },
    { 0x016F, 0xFFFF, "CMP"  , "f,#b" , 0 },
    { 0x8F00, 0xFF08, "CMP"  , "r,r1" , 0 },
    { 0x9F00, 0xFF00, "CMP"  , "A,s"  , 0 },
    { 0x019F, 0xFFFF, "CMP"  , "A,f"  , 0 },
    { 0x7F00, 0xFF00, "CMP"  , "s,s"  , 0 },
//  { 0x000F, 0xE08F, "CMP"  , "A,m"  , 0 },
//  { 0x008F, 0xE08F, "CMP"  , "m,A"  , 0 },

// 4
    { 0x2D00, 0xFF00, "ADDW"  , "AX,#w" , 0 },
    { 0x0D00, 0xFF00, "ADDW"  , "s,#w"  , 0 },
    { 0x010D, 0xFFFF, "ADDW"  , "f,#w"  , 0 },
    { 0x8808, 0xFF18, "ADDW"  , "p,p1"  , 0 },
    { 0x1D00, 0xFF00, "ADDW"  , "AX,s"  , 0 },
    { 0x011D, 0xFFFF, "ADDW"  , "AX,f"  , 0 },
    { 0x3D00, 0xFF00, "ADDW"  , "s,s"   , 0 },
    { 0x2E00, 0xFF00, "SUBW"  , "AX,#w" , 0 },
    { 0x0E00, 0xFF00, "SUBW"  , "s,#w"  , 0 },
    { 0x010E, 0xFFFF, "SUBW"  , "f,#w"  , 0 },
    { 0x8A08, 0xFF18, "SUBW"  , "p,p1"  , 0 },
    { 0x1E00, 0xFF00, "SUBW"  , "AX,s"  , 0 },
    { 0x011E, 0xFFFF, "SUBW"  , "AX,f"  , 0 },
    { 0x3E00, 0xFF00, "SUBW"  , "s,s"   , 0 },
    { 0x2F00, 0xFF00, "CMPW"  , "AX,w"  , 0 },
    { 0x0F00, 0xFF00, "CMPW"  , "s,#w"  , 0 },
    { 0x010F, 0xFFFF, "CMPW"  , "f,#w"  , 0 },
    { 0x8F08, 0xFF18, "CMPW"  , "p,p1"  , 0 },
    { 0x1F00, 0xFF00, "CMPW"  , "AX,s"  , 0 },
    { 0x011F, 0xFFFF, "CMPW"  , "AX,f"  , 0 },
    { 0x3F00, 0xFF00, "CMPW"  , "s,s"   , 0 },

// 5
    { 0x0508, 0xFFF8, "MULU"  , "r1"    , 0 },
    { 0x0518, 0xFFF8, "DIVUW" , "r1"    , 0 },
    { 0x0528, 0xFFF8, "MULUW" , "p1"    , 0 },
    { 0x05E8, 0xFFF8, "DIVUX" , "p1"    , 0 },

// 6
    { 0x0538, 0xFFF8, "MULW"  , "p1"    , 0 },

// 7
    { 0x0785, 0xFFFF, "MACW"  , "b"     , 0 }, //*** b -> special

// 8
    { 0x0705, 0xFFFF, "MACSW" , "b"     , 0 }, //*** b -> special

// 9
//  { 0x09B0, 0xFFFF, "SACW"  , "[DE+],[HL+]||", 0 }, //*** 09 B0 41 46

// 10
    { 0x09A0, 0xFFFF, "MOVTBLW", "w"   , 0 }, //*** w -> special

// 11
    { 0xC000, 0xF800, "INC"  , "r1"    , 0 },
    { 0x2600, 0xFF00, "INC"  , "s"     , 0 },
    { 0xC800, 0xF800, "DEC"  , "r1"    , 0 },
    { 0x2700, 0xFF00, "DEC"  , "s"     , 0 },
    { 0x4400, 0xFC00, "INCW" , "p2"    , 0 },
    { 0x07E8, 0xFFFF, "INCW" , "s"     , 0 },
    { 0x4C00, 0xFC00, "DECW" , "p2"    , 0 },
    { 0x07E9, 0xFFFF, "DECW" , "s"     , 0 },

// 12
    { 0x3040, 0xFFC0, "ROR"  , "r1,h" , 0 }, //*** r1 <- B2, h <- B2 shr 3
    { 0x3140, 0xFFC0, "ROL"  , "r1,h" , 0 }, //*** r1 <- B2, h <- B2 shr 3
    { 0x3000, 0xFFC0, "RORC" , "r1,h" , 0 }, //*** r1 <- B2, h <- B2 shr 3
    { 0x3100, 0xFFC0, "ROLC" , "r1,h" , 0 }, //*** r1 <- B2, h <- B2 shr 3
    { 0x3080, 0xFFC0, "SHR"  , "r1,h" , 0 }, //*** r1 <- B2, h <- B2 shr 3
    { 0x3180, 0xFFC0, "SHL"  , "r1,h" , 0 }, //*** r1 <- B2, h <- B2 shr 3
    { 0x30C0, 0xFFC0, "SHRW" , "p1,h",  0 }, //*** rp1 <- B2, h <- B2 shr 3
    { 0x31C0, 0xFFC0, "SHLW" , "p1,h",  0 }, //*** rp1 <- B2, h <- B2 shr 3
    { 0x0588, 0xFFF8, "ROR4" , "[p1]",  0 }, //*** n <-b2
    { 0x0598, 0xFFF8, "ROL4" , "[p1]",  0 },

// 13
    { 0x05FE, 0xFFFF, "ADJBA"  , ""     , 0 },
    { 0x05FF, 0xFFFF, "ADJBS"  , ""     , 0 },

// 14
    { 0x0400, 0xFF00, "CVTBW"  , ""     , 0 },

// 15
// MOV1 0/1
    { 0x0800, 0xFFF8, "MOV1"  , "CY,s.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0808, 0xFFF8, "MOV1"  , "CY,f.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0308, 0xFFF8, "MOV1"  , "CY,A.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0300, 0xFFF8, "MOV1"  , "CY,X.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0208, 0xFFF8, "MOV1"  , "CY,PSWH.n"  , 0 }, // *** n -> B1,b2..b0
    { 0x0200, 0xFFF8, "MOV1"  , "CY,PSWL.n"  , 0 }, // *** n -> B1,b2..b0
    { 0x0810, 0xFFF8, "MOV1"  , "s.n,CY"     , 0 }, // *** n -> B1,b2..b0
    { 0x0818, 0xFFF8, "MOV1"  , "f.n,CY"     , 0 }, // *** n -> B1,b2..b0
    { 0x0318, 0xFFF8, "MOV1"  , "A.n,CY"     , 0 }, // *** n -> B1,b2..b0
    { 0x0310, 0xFFF8, "MOV1"  , "X.n,CY"     , 0 }, // *** n -> B1,b2..b0
    { 0x0218, 0xFFF8, "MOV1"  , "PSWH.n,CY"  , 0 }, // *** n -> B1,b2..b0
    { 0x0210, 0xFFF8, "MOV1"  , "PSWL.n,CY"  , 0 }, // *** n -> B1,b2..b0
// AND1 2/3
    { 0x0820, 0xFFF8, "AND1"  , "CY,s.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0830, 0xFFF8, "AND1"  , "CY,/s.n"    , 0 }, // *** n -> B1,b2..b0
    { 0x0828, 0xFFF8, "AND1"  , "CY,f.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0838, 0xFFF8, "AND1"  , "CY,/f.n"    , 0 }, // *** n -> B1,b2..b0
    { 0x0328, 0xFFF8, "AND1"  , "CY,A.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0338, 0xFFF8, "AND1"  , "CY,/A.n"    , 0 }, // *** n -> B1,b2..b0
    { 0x0320, 0xFFF8, "AND1"  , "CY,X.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0330, 0xFFF8, "AND1"  , "CY,/X.n"    , 0 }, // *** n -> B1,b2..b0
    { 0x0228, 0xFFF8, "AND1"  , "CY,PSWH.n"  , 0 }, // *** n -> B1,b2..b0
    { 0x0238, 0xFFF8, "AND1"  , "CY,/PSWH.n" , 0 }, // *** n -> B1,b2..b0
    { 0x0220, 0xFFF8, "AND1"  , "CY,PSWL.n"  , 0 }, // *** n -> B1,b2..b0
    { 0x0230, 0xFFF8, "AND1"  , "CY,/PSWL.n" , 0 }, // *** n -> B1,b2..b0
// OR1  4/5
    { 0x0840, 0xFFF8, "OR1"  , "CY,s.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0850, 0xFFF8, "OR1"  , "CY,/s.n"    , 0 }, // *** n -> B1,b2..b0
    { 0x0848, 0xFFF8, "OR1"  , "CY,f.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0858, 0xFFF8, "OR1"  , "CY,/f.n"    , 0 }, // *** n -> B1,b2..b0
    { 0x0348, 0xFFF8, "OR1"  , "CY,A.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0358, 0xFFF8, "OR1"  , "CY,/A.n"    , 0 }, // *** n -> B1,b2..b0
    { 0x0340, 0xFFF8, "OR1"  , "CY,X.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0350, 0xFFF8, "OR1"  , "CY,/X.n"    , 0 }, // *** n -> B1,b2..b0
    { 0x0248, 0xFFF8, "OR1"  , "CY,PSWH.n"  , 0 }, // *** n -> B1,b2..b0
    { 0x0258, 0xFFF8, "OR1"  , "CY,/PSWH.n" , 0 }, // *** n -> B1,b2..b0
    { 0x0240, 0xFFF8, "OR1"  , "CY,PSWL.n"  , 0 }, // *** n -> B1,b2..b0
    { 0x0250, 0xFFF8, "OR1"  , "CY,/PSWL.n" , 0 }, // *** n -> B1,b2..b0
// XOR1 6
    { 0x0860, 0xFFF8, "XOR1"  , "CY,s.n"   , 0 }, // *** n -> B1,b2..b0
    { 0x0868, 0xFFF8, "XOR1"  , "CY,f.n"   , 0 }, // *** n -> B1,b2..b0
    { 0x0368, 0xFFF8, "XOR1"  , "CY,A.n"   , 0 }, // *** n -> B1,b2..b0
    { 0x0360, 0xFFF8, "XOR1"  , "CY,X.n"   , 0 }, // *** n -> B1,b2..b0
    { 0x0268, 0xFFF8, "XOR1"  , "CY,PSWH.n", 0 }, // *** n -> B1,b2..b0
    { 0x0260, 0xFFF8, "XOR1"  , "CY,PSWL.n", 0 }, // *** n -> B1,b2..b0
// SET1 8
    { 0xB000, 0xF800, "SET1"  , "s.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0888, 0xFFF8, "SET1"  , "f.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0388, 0xFFF8, "SET1"  , "A.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0380, 0xFFF8, "SET1"  , "X.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0288, 0xFFF8, "SET1"  , "PSWH.n"  , 0 }, // *** n -> B1,b2..b0
    { 0x0280, 0xFFF8, "SET1"  , "PSWH.n"  , 0 }, // *** n -> B1,b2..b0
    { 0x4100, 0xF800, "SET1"  , "CY"      , 0 },
// CLR1 9
    { 0xA000, 0xF800, "CLR1"  , "s.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0898, 0xFFF8, "CLR1"  , "f.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0398, 0xFFF8, "CLR1"  , "A.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0390, 0xFFF8, "CLR1"  , "X.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0298, 0xFFF8, "CLR1"  , "PSWH.n"  , 0 }, // *** n -> B1,b2..b0
    { 0x0290, 0xFFF8, "CLR1"  , "PSWH.n"  , 0 }, // *** n -> B1,b2..b0
    { 0x4000, 0xF800, "CLR1"  , "CY"      , 0 },
// NOT1 7
    { 0x0870, 0xFFF8, "NOT1"  , "s.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0878, 0xFFF8, "NOT1"  , "f.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0378, 0xFFF8, "NOT1"  , "A.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0370, 0xFFF8, "NOT1"  , "X.n"     , 0 }, // *** n -> B1,b2..b0
    { 0x0278, 0xFFF8, "NOT1"  , "PSWH.n"  , 0 }, // *** n -> B1,b2..b0
    { 0x0270, 0xFFF8, "NOT1"  , "PSWH.n"  , 0 }, // *** n -> B1,b2..b0
    { 0x4200, 0xF800, "NOT1"  , "CY"      , 0 },

// 16
    { 0x2800, 0xFF00, "CALL" , "w"    , REFFLAG | CODEREF },
    { 0x0558, 0xFFF8, "CALL" , "p1"   , 0 },
    { 0x0578, 0xFFF8, "CALL" , "[p1]" , 0 },
    { 0x9000, 0xF800, "CALLF", "c"    , 0 }, //*** f = 0800 + 11 bit addr
    { 0xE000, 0xE000, "CALLT", "[t]"  , 0 }, //*** t = B1,b4..b0 -> 0080..00FE
    { 0x5E00, 0xFF00, "BRK"  , ""     , 0 },
    { 0x5600, 0xFF00, "RET"  , ""     , LFFLAG },
    { 0x5F00, 0xFF00, "RETB" , ""     , LFFLAG },
    { 0x5700, 0xFF00, "RETI" , ""     , LFFLAG },

// 17
    { 0x07D9, 0xFFFF, "PUSH" , "s"    , 0 },
    { 0x3500, 0xFF00, "PUSH" , "q"    , 0 }, //*** q = post = reg list w/RP5
    { 0x4900, 0xFF00, "PUSH" , "PSW"  , 0 },
    { 0x3700, 0xFF00, "PUSHU", "q"    , 0 }, //*** q = post = reg list w/PSW
    { 0x07D8, 0xFFFF, "POP"  , "s"    , 0 },
    { 0x3400, 0xFF00, "POP"  , "q"    , 0 }, //*** q = post = reg list w/PSW
    { 0x4800, 0xFF00, "POP"  , "PSW"  , 0 },
    { 0x3600, 0xFF00, "POPU" , "q"    , 0 }, //*** q = post = reg list w/PSW
    { 0x0BFC, 0xFFFF, "MOVW" , "SP,#w", 0 },
    { 0x13FC, 0xFFFF, "MOVW" , "SP,AX", 0 },
    { 0x11FC, 0xFFFF, "MOVW" , "AX,SP", 0 },
    { 0x05C8, 0xFFFF, "INCW" , "SP"   , 0 },
    { 0x05C9, 0xFFFF, "DECW" , "SP"   , 0 },

// 18
    { 0x07C8, 0xFFFF, "CHKL" , "f"    , 0 },
    { 0x07C9, 0xFFFF, "CHKLA", "f"    , 0 },

// 19
    { 0x2C00, 0xFF00, "BR"   , "w"    , LFFLAG | REFFLAG | CODEREF },
    { 0x0548, 0xFFF8, "BR"   , "p1"   , LFFLAG | REFFLAG | CODEREF }, //*** p1 -> rp1 in B2,b2..b0
    { 0x0568, 0xFFF8, "BR"   , "[p1]" , LFFLAG | REFFLAG | CODEREF }, //*** p1 -> rp1 in B2,b2..b0
    { 0x1400, 0xFF00, "BR"   , "e"    , LFFLAG | REFFLAG | CODEREF },

// 20
    { 0x8300, 0xFF00, "BC"   , "e"    , 0 }, // BC  / BL
    { 0x8200, 0xFF00, "BNC"  , "e"    , 0 }, // BNC / BNL
    { 0x8100, 0xFF00, "BZ"   , "e"    , 0 }, // BZ  / BE
    { 0x8000, 0xFF00, "BNZ"  , "e"    , 0 }, // BNZ / BNE
    { 0x8500, 0xFF00, "BV"   , "e"    , 0 }, // BV  / BPE
    { 0x8400, 0xFF00, "BNV"  , "e"    , 0 }, // BNV / BPO
    { 0x8700, 0xFF00, "BN"   , "e"    , 0 },
    { 0x8600, 0xFF00, "BP"   , "e"    , 0 },
    { 0x07FB, 0xFFFF, "BGT"  , "e"    , 0 },
    { 0x07F9, 0xFFFF, "BGE"  , "e"    , 0 },
    { 0x07F8, 0xFFFF, "BLT"  , "e"    , 0 },
    { 0x07FA, 0xFFFF, "BLE"  , "e"    , 0 },
    { 0x07FD, 0xFFFF, "BH"   , "e"    , 0 },
    { 0x07FC, 0xFFFF, "BNH"  , "e"    , 0 },
// BT
    { 0x7000, 0xF800, "BT", "s.n,e"   , 0 }, //*** n -> B1,b2..b0
    { 0x08B8, 0xFFF8, "BT", "f.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x03B8, 0xFFF8, "BT", "A.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x03B0, 0xFFF8, "BT", "X.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x02B8, 0xFFF8, "BT", "PSWH.n,e", 0 }, //*** n -> B2,b2..b0
    { 0x02B0, 0xFFF8, "BT", "PSWL.n,e", 0 }, //*** n -> B2,b2..b0
// BF
    { 0x08A0, 0xFFF8, "BF", "s.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x08A8, 0xFFF8, "BF", "f.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x03A8, 0xFFF8, "BF", "A.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x03A0, 0xFFF8, "BF", "X.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x02A8, 0xFFF8, "BF", "PSWH.n,e", 0 }, //*** n -> B2,b2..b0
    { 0x02A0, 0xFFF8, "BF", "PSWL.n,e", 0 }, //*** n -> B2,b2..b0
// BTCLR
    { 0x08D0, 0xFFF8, "BTCLR", "s.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x08D8, 0xFFF8, "BTCLR", "f.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x03D8, 0xFFF8, "BTCLR", "A.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x03D0, 0xFFF8, "BTCLR", "X.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x02D8, 0xFFF8, "BTCLR", "PSWH.n,e", 0 }, //*** n -> B2,b2..b0
    { 0x02D0, 0xFFF8, "BTCLR", "PSWL.n,e", 0 }, //*** n -> B2,b2..b0
// BFSET
    { 0x08C0, 0xFFF8, "BFSET", "s.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x08C8, 0xFFF8, "BFSET", "f.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x03C8, 0xFFF8, "BFSET", "A.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x03C0, 0xFFF8, "BFSET", "X.n,e"   , 0 }, //*** n -> B2,b2..b0
    { 0x02C8, 0xFFF8, "BFSET", "PSWH.n,e", 0 }, //*** n -> B2,b2..b0
    { 0x02C0, 0xFFF8, "BFSET", "PSWL.n,e", 0 }, //*** n -> B2,b2..b0
// DBNZ
    { 0x3200, 0xFE00, "DBNZ" , "r2,e"    , 0 },
    { 0x3B00, 0xFF00, "DBNZ" , "s,e"     , 0 },

// 21
    { 0x05D8, 0xFFF8, "BRKCS" , "RBn" , 0 }, //*** n -> B2,b2..b0
    { 0x2900, 0xFF00, "RETCS" , "w"   , 0 },
    { 0x09E0, 0xFFFF, "RETCSB", "w"   , 0 },

// 22
    { 0x1500, 0xFFFF, "MOVM"   , "[DE+],A"    , 0 },
    { 0x1510, 0xFFFF, "MOVM"   , "[DE-],A"    , 0 },
    { 0x1520, 0xFFFF, "MOVBK"  , "[DE+],[HL+]", 0 },
    { 0x1530, 0xFFFF, "MOVBK"  , "[DE-],[HL-]", 0 },
    { 0x1501, 0xFFFF, "XCHM"   , "[DE+],A"    , 0 },
    { 0x1511, 0xFFFF, "XCHM"   , "[DE-],A"    , 0 },
    { 0x1521, 0xFFFF, "XCHBK"  , "[DE+],[HL+]", 0 },
    { 0x1531, 0xFFFF, "XCHBK"  , "[DE-],[HL-]", 0 },
    { 0x1504, 0xFFFF, "CMPME"  , "[DE+],A"    , 0 },
    { 0x1514, 0xFFFF, "CMPME"  , "[DE-],A"    , 0 },
    { 0x1524, 0xFFFF, "CMPBKE" , "[DE+],[HL+]", 0 },
    { 0x1534, 0xFFFF, "CMPBKE" , "[DE-],[HL-]", 0 },
    { 0x1505, 0xFFFF, "CMPMNE" , "[DE+],A"    , 0 },
    { 0x1515, 0xFFFF, "CMPMNE" , "[DE-],A"    , 0 },
    { 0x1525, 0xFFFF, "CMPBKNE", "[DE+],[HL+]", 0 },
    { 0x1535, 0xFFFF, "CMPBKNE", "[DE-],[HL-]", 0 },
    { 0x1507, 0xFFFF, "CMPMC"  , "[DE+],A"    , 0 },
    { 0x1517, 0xFFFF, "CMPMC"  , "[DE-],A"    , 0 },
    { 0x1527, 0xFFFF, "CMPBKC" , "[DE+],[HL+]", 0 },
    { 0x1537, 0xFFFF, "CMPBKC" , "[DE-],[HL-]", 0 },
    { 0x1506, 0xFFFF, "CMPMNC" , "[DE+],A"    , 0 },
    { 0x1516, 0xFFFF, "CMPMNC" , "[DE-],A"    , 0 },
    { 0x1526, 0xFFFF, "CMPBKNC", "[DE+],[HL+]", 0 },
    { 0x1536, 0xFFFF, "CMPBKNC", "[DE-],[HL-]", 0 },

// 23
    { 0x09C0, 0xFFFF, "MOV"  , "STBC,#w", 0 }, //*** w -> b, !b
    { 0x09C2, 0xFFFF, "MOV"  , "WDM,#w" , 0 },
    { 0x4300, 0xFF00, "SWRS" , ""       , 0 },
    { 0x05A8, 0xFFF8, "SEL"  , "RBn"    , 0 }, //*** n -> B2,b2..b0
    { 0x05B8, 0xFFF8, "SEL"  , "RBn,ALT", 0 },
    { 0x0000, 0xFF00, "NOP"  , ""       , 0 },
    { 0x4B00, 0xFF00, "EI"   , ""       , 0 },
    { 0x4A00, 0xFF00, "DI"   , ""       , 0 },

// all "mem" instructions need to go at the end
// because they don't use all possible matching opcodes

    { 0x5800, 0xF800, "MOV"  , "A,m1" , 0 }, // deferred to avoid 5E/5F
    { 0x5000, 0xF800, "MOV"  , "m1,A" , 0 }, // deferred to avoid 56/57
    { 0x0000, 0xE08F, "MOV"  , "A,m"  , 0 },
    { 0x0080, 0xE08F, "MOV"  , "m,A"  , 0 },
    { 0x0004, 0xE08F, "XCH"  , "A,m"  , 0 },
    { 0x0001, 0xE08F, "MOVW" , "AX,m" , 0 },
    { 0x0081, 0xE08F, "MOVW" , "m,AX" , 0 },
    { 0x0005, 0xE08F, "XCHW" , "AX,m" , 0 },
    { 0x0008, 0xE08F, "ADD"  , "A,m"  , 0 },
    { 0x0088, 0xE08F, "ADD"  , "m,A"  , 0 },
    { 0x0009, 0xE08F, "ADDC" , "A,m"  , 0 },
    { 0x0089, 0xE08F, "ADDC" , "m,A"  , 0 },
    { 0x000A, 0xE08F, "SUB"  , "A,m"  , 0 },
    { 0x008A, 0xE08F, "SUB"  , "m,A"  , 0 },
    { 0x000B, 0xE08F, "SUBC" , "A,m"  , 0 },
    { 0x008B, 0xE08F, "SUBC" , "m,A"  , 0 },
    { 0x000C, 0xE08F, "AND"  , "A,m"  , 0 },
    { 0x008C, 0xE08F, "AND"  , "m,A"  , 0 },
    { 0x000E, 0xE08F, "OR"   , "A,m"  , 0 },
    { 0x008E, 0xE08F, "OR"   , "m,A"  , 0 },
    { 0x000D, 0xE08F, "XOR"  , "A,m"  , 0 },
    { 0x008D, 0xE08F, "XOR"  , "m,A"  , 0 },
    { 0x000F, 0xE08F, "CMP"  , "A,m"  , 0 },
    { 0x008F, 0xE08F, "CMP"  , "m,A"  , 0 },

    {   0   ,   0   , ""     , ""       , 0 },
};


static const char *m16[] = {
  "[DE+]", "[HL+]", "[DE-]", "[HL-]", "[DE]", "[HL]", "[VP]", "[UP]"
};


static const char *m17[] = {
  "[DE+A]", "[HL+A]", "[DE+B]", "[HL+B]", "[VP+DE]", "[VP+HL]", "?", "?"
};


static const char *m06[] = {
  "[DE+", "[SP+", "[HL+", "[UP+", "[VP+", "?", "?", "?"
};


static const char *m0A[] = {
  "[DE]", "[A]", "[HL]", "[B]", "?", "?", "?", "?"
};


// =====================================================
int Dis78K3::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
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
    instr = FindInstr(opcdTable_78K3, op);

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

                // == relative branch ==
                case 'e': // 8-bit relative
                    i = ReadByte(ad++);
                    len++;
                    if (i >= 128) { // sign-extend
                        i = i - 256;
                    }
                    ra = (ad + i) & 0xFFFF;
                    RefStr(ra, p, lfref, refaddr);
                    p += strlen(p);
                    lfref |= REFFLAG | CODEREF;
                break;

                // == CALLT ==
                case 't':
                    ra = (op & 0x3E) + 0x0080;
                    RefStr(ra, p, lfref, refaddr);
                    p += strlen(p);
                    lfref |= REFFLAG | CODEREF;
                    break;

                // == CALLF ==
                case 'c': // 11 bit address
                    ra = 0x0800 | ((op & 0x70) << 4) | ReadByte(ad++);
                    len++;
                    RefStr(ra, p, lfref, refaddr);
                    p += strlen(p);
                    lfref |= REFFLAG | CODEREF;
                    break;

                // === r ===
                case 'r':
                    switch (*(l+1)) {
                        default: // r: R0..R15
                            i = (op >> 4) & 0x07;
                            sprintf(p, "R%d", i);
                            p += strlen(p);
                            break;
                        case '1': // r1
                            l++;
                            i = op & 0x07;
                            sprintf(p, "R%d", i);
                            p += strlen(p);
                            break;
                        case '2': // r2
                            l++;
                            *p++ = "CB"[op & 1];
                            break;
                    }
                    break;

                // == saddr == refers to address range FE20-FF1F
                case 's':
                    i = ReadByte(ad++);
                    len++;
#if 0
                    switch (i) {
#if 0
                        // these are the ones that I have found documented
                        case 0x1C:
                            strcat(p,"SP");
                            break;

                        case 0x1E:
                            strcat(p,"PSW");
                            break;
#endif
                        default:
                            strcat(p, "saddr:");
                            p += strlen(p);
                            if (i < 0x20) {
                                i += 256; // adjust for wraparound
                            }
                            ra = 0xFE20 + i;
                            RefStr(ra, p, lfref, refaddr);
                        break;
                    }
#else
                    sprintf(p, "s%.2X", i);
#endif
                    p += strlen(p);
                    break;

                // == sfr == refers to address range FF00-FFFF
                case 'f':
                    i = ReadByte(ad++);
                    len++;
#if 0
                    strcat(p, "sfr:");
                    if (i < 0x20) {
                        i += 256; // adjust for wraparound
                    }
                    ra = 0xFF00 + i;
                    RefStr(ra, p, lfref, refaddr);
#else
                    sprintf(p, "f%.2X", i);
#endif
                    p += strlen(p);
                    break;

                // == rp ==
                case 'p':
                    switch (*(l+1)) {
                        default: // rp: R0..R15
                            i = (op >> 5) & 0x07;
                            sprintf(p, "RP%c", i + '0');
                            p += strlen(p);
                            break;
                        case '1': // rp1
                            l++;
                            i = op & 0x07;
                            sprintf(p, "RP%c", "04152637"[i]);
                            p += strlen(p);
                            break;
                        case '2': // rp2
                            l++;
                            i = op & 0x03;
                            *p++ = "VUDH"[i]; // VP UP DE HL
                            *p++ = "PPEL"[i];
                            break;
                    }
                    break;

                // == shift/rotate count ==
                case 'h':
                    i = (op >> 3) & 0x07;
                    *p++ = '0' + i;
                    break;

                // == bit number from instr ==
                case 'n':
                    i = op & 0x07;
                    *p++ = '0' + i;
                    break;

                // == push/pop regs ==
                // op bit 0x02 = PUSHU/POPU
                // op bit 0x01 = PUSH/PUSHU
                // pop low to high, push high to low
                case 'q':
                    i = ReadByte(ad++);
                    len++;
                    if (!i) {
                        strcat(p, "(no regs)");
                    } else {
                        bool comma = false;
                        for (int bit = 0; bit < 8; bit++) {
                            if (i & (1 << bit)) {
                                 if (comma) {
                                     *p++ = ',';
                                 }
                                 if ((i == 5) && (op & 0x02)) {
                                     sprintf(p, "PSW");
                                 } else {
                                     sprintf(p, "R%d", bit);
                                 }
                                 comma = true;
                            }
                            p += strlen(p);
                        }
                    }
                    break;

                // == mem modes ==
                case 'm':
                    switch (*(l+1)) {
                        const char *q;

                        default: // general 'mem' case
                            // mode = B1 bits 4..0, 16 17 06 0A only
                            // mem = B2 bits 6..4
                            q = 0;
                            switch ((op >> 8) & 0x1F) {
                                case 0x16: // register indirect
                                    q = m16[(op >> 4) & 0x07];
                                    strcat(p, q);
                                    break;

                                case 0x17: // base indexed
                                    q = m17[(op >> 4) & 0x07];
                                    strcat(p, q);
                                    break;

                                case 0x06: // based (plus a byte)
                                    q = m06[(op >> 4) & 0x07];
                                    i = ReadByte(ad++);
                                    len++;
                                    strcat(p, q);
                                    p += strlen(p);
                                    H2Str(i, p); // add byte index
                                    strcat(p, "]");
                                    break;

                                case 0x0A: // indexed (with a word)
                                    q = m0A[(op >> 4) & 0x07];
                                    i = ReadWord(ad);
                                    i += 2;
                                    len++;
                                    H4Str(i, p); // add word index
                                    strcat(p, m0A[(op >> 4) & 0x07]);
                                    break;

                                default:
                                    break;
                            }
                            p += strlen(p);
                            if (!q) {
                                opcode[0] = 0; // invalid
                            }
                            break;
                        case '1': // short 'mem' case
                            l++;
                            strcat(p, m16[op & 0x07]);
                            if (!*p) {
                                opcode[0] = 0; // invalid
                            }
                            break;
                    }
                    p += strlen(p);
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


