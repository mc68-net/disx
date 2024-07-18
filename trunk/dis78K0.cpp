// dis78k0.c
// disassembler for NEC D78K0

static const char versionName[] = "NEC D78K0 disassembler";

#include "discpu.h"

class Dis78K0 : public CPU {
public:
    Dis78K0(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


Dis78K0 cpu_78K0("D78K0", 0, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DL");


Dis78K0::Dis78K0(const char *name, int subtype, int endian, int addrwid,
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


static const struct InstrRec opcdTable_78K0[] =
{

// NOTE: sub-pages must be handled before main page
//       because they overlay an invalid opcode

    // === page 31 ===
    //  op     mask    opcd    parms     lfref
    { 0x3101, 0xFF8F, "BTCLR", "s.x,w"   , 0 },
    { 0x3103, 0xFF8F, "BF"   , "s.x,w"   , 0 },
    { 0x3105, 0xFF8F, "BTCLR", "f.x,w"   , 0 },
    { 0x3106, 0xFF8F, "BT"   , "f.x,w"   , 0 },
    { 0x3107, 0xFF8F, "BF"   , "f.x,w"   , 0 },
    { 0x310B, 0xFF8F, "BTCLR", "A.x,w"   , 0 },
    { 0x310D, 0xFF8F, "BT"   , "A.x,w"   , 0 },
    { 0x310E, 0xFF8F, "BF"   , "A.x,w"   , 0 },

    { 0x310A, 0xFE00, "ADD"  , "[HL+r]"  , 0 },
    { 0x311A, 0xFE00, "SUB"  , "[HL+r]"  , 0 },
    { 0x312A, 0xFE00, "ADDC" , "[HL+r]"  , 0 },
    { 0x313A, 0xFE00, "SUBC" , "[HL+r]"  , 0 },
    { 0x314A, 0xFE00, "CMP"  , "[HL+r]"  , 0 },
    { 0x315A, 0xFE00, "AND"  , "[HL+r]"  , 0 },
    { 0x316A, 0xFE00, "OR"   , "[HL+r]"  , 0 },
    { 0x317A, 0xFE00, "XOR"  , "[HL+r]"  , 0 },

    { 0x3180, 0xFFFF, "ROL4" , "[HL]"    , 0 },
    { 0x3182, 0xFFFF, "DIVUW", "C"       , 0 },
    { 0x3188, 0xFFFF, "MULU" , "X"       , 0 },
    { 0x318A, 0xFFFE, "XCH"  , "A,[HL+r]", 0 },
    { 0x3190, 0xFFFF, "ROR4" , "[HL]"    , 0 },

    { 0x3100, 0xFF00, ""  , ""    , 0 }, // catch-all invalid page 31

    // === page 61 ===
    //  op     mask    opcd    parms     lfref
    { 0x6109, 0xFF8F, ""     , ""         , 0 }, // don't allow "61x9 op A,A"

    { 0x6100, 0xFFF8, "ADD"  , "r,A"     , 0 },
    { 0x6108, 0xFFF8, "ADD"  , "A,r"     , 0 },
    { 0x6110, 0xFFF8, "SUB"  , "r,A"     , 0 },
    { 0x6118, 0xFFF8, "SUB"  , "A,r"     , 0 },
    { 0x6120, 0xFFF8, "ADDC" , "r,A"     , 0 },
    { 0x6128, 0xFFF8, "ADDC" , "A,r"     , 0 },
    { 0x6130, 0xFFF8, "SUBC" , "r,A"     , 0 },
    { 0x6138, 0xFFF8, "SUBC" , "A,r"     , 0 },
    { 0x6140, 0xFFF8, "CMP"  , "r,A"     , 0 },
    { 0x6148, 0xFFF8, "CMP"  , "A,r"     , 0 },
    { 0x6150, 0xFFF8, "AND"  , "r,A"     , 0 },
    { 0x6158, 0xFFF8, "AND"  , "A,r"     , 0 },
    { 0x6160, 0xFFF8, "OR"   , "r,A"     , 0 },
    { 0x6168, 0xFFF8, "OR "  , "A,r"     , 0 },
    { 0x6170, 0xFFF8, "XOR"  , "r,A"     , 0 },
    { 0x6178, 0xFFF8, "XOR"  , "A,r"     , 0 },

    { 0x6180, 0xFFFF, "ADJBA", ""        , 0 },
    { 0x6190, 0xFFFF, "ADJBS", ""        , 0 },
    { 0x61D0, 0xFFFF, "SEL"  , "RB0"     , 0 },
    { 0x61D8, 0xFFFF, "SEL"  , "RB1"     , 0 },
    { 0x61F0, 0xFFFF, "SEL"  , "RB2"     , 0 },
    { 0x61F8, 0xFFFF, "SEL"  , "RB3"     , 0 },

    { 0x6189, 0xFF8F, "MOV1" , "A.x,CY"  , 0 },
    { 0x618A, 0xFF8F, "SET1" , "A.x"     , 0 },
    { 0x618B, 0xFF8F, "CLR1" , "A.x"     , 0 },
    { 0x618C, 0xFF8F, "MOV1" , "CY,A.x"  , 0 },
    { 0x618D, 0xFF8F, "AND1" , "CY,A.x"  , 0 },
    { 0x618E, 0xFF8F, "OR1"  , "CY,A.x"  , 0 },
    { 0x618F, 0xFF8F, "XOR1" , "CY,A.x"  , 0 },

    { 0x6100, 0xFF00, ""     , ""         , 0 }, // catch-all invalid page 61

    // === page 71 ===
    //  op     mask    opcd    parms     lfref
    { 0x7101, 0xFF8F, "MOV1" , "s.x,CY"   , 0 },
    { 0x7104, 0xFF8F, "MOV1" , "CY,s.x"   , 0 },
    { 0x7105, 0xFF8F, "AND1" , "CY,s.x"   , 0 },
    { 0x7106, 0xFF8F, "OR1"  , "CY,s.x"   , 0 },
    { 0x7107, 0xFF8F, "XOR1" , "CY,s.x"   , 0 },
    { 0x7109, 0xFF8F, "MOV1" , "f.x,CY"   , 0 },
    { 0x710A, 0xFF8F, "SET1" , "f.x"      , 0 },
    { 0x710B, 0xFF8F, "CLR1" , "f.x"      , 0 },
    { 0x710C, 0xFF8F, "MOV1" , "CY,f.x"   , 0 },
    { 0x710D, 0xFF8F, "AND1" , "CY,f.x"   , 0 },
    { 0x710E, 0xFF8F, "OR1"  , "CY,f.x"   , 0 },
    { 0x710F, 0xFF8F, "XOR1" , "CY,f.x"   , 0 },

    { 0x7181, 0xFF8F, "MOV1" , "[HL].s,CY", 0 },
    { 0x7182, 0xFF8F, "SET1" , "[HL].s"   , 0 },
    { 0x7183, 0xFF8F, "CLR1" , "[HL].s"   , 0 },
    { 0x7184, 0xFF8F, "MOV1" , "CY,[HL].x", 0 },
    { 0x7185, 0xFF8F, "AND1" , "CY,[HL].x", 0 },
    { 0x7186, 0xFF8F, "OR1"  , "CY,[HL].x", 0 },
    { 0x7187, 0xFF8F, "XOR1" , "CY,[HL].x", 0 },

    { 0x7100, 0xFF00, ""  , ""    , 0 }, // catch-all invalid page 71

    // === main page ==
    //  op     mask    opcd    parms     lfref
    { 0x0000, 0xFF00, "NOP"  , ""        , 0 },
    { 0x0100, 0xFF00, "NOT1" , "CY"      , 0 },
    { 0x0200, 0xFF00, "MOVW" , "AX,w"    , 0 },
    { 0x0300, 0xFF00, "MOVW" , "w,AX"    , 0 },
    { 0x0400, 0xFF00, "DBNZ" , "s,e"     , REFFLAG | CODEREF },
    { 0x0500, 0xFF00, "XCH"  , "A,[DE]"  , 0 },
//  { 0x0600, 0xFF00, ""     , ""        , 0 },
    { 0x0700, 0xFF00, "XCH"  , "A,[HL]"  , 0 },
    { 0x0800, 0xFF00, "ADD"  , "A,w"     , 0 },
    { 0x0900, 0xFF00, "ADD"  , "A,[HL+b]", 0 },
    { 0x0A00, 0x8F00, "SET1" , "s.x"     , 0 },
    { 0x0B00, 0x8F00, "CLR1" , "s.x"     , 0 },
    { 0x0C00, 0x8F00, "CALLF", "c"       , REFFLAG | CODEREF },
    { 0x0D00, 0xFF00, "ADD"  , "A,#b"    , 0 },
    { 0x0E00, 0xFF00, "ADD"  , "A,s"     , 0 },
    { 0x0F00, 0xFF00, "ADD"  , "A,[HL]"  , 0 },

    { 0x1000, 0xF900, "MOVW" , "p,#w"    , 0 },
    { 0x1100, 0xFF00, "MOV"  , "s,#b"    , 0 },
    { 0x1300, 0xFF00, "MOV"  , "f,#b"    , 0 },
//  { 0x1500, 0xFF00, ""     , ""        , 0 },
//  { 0x1700, 0xFF00, ""     , ""        , 0 },
    { 0x1800, 0xFF00, "SUB"  , "A,w"     , 0 },
    { 0x1900, 0xFF00, "SUB"  , "A,[HL+b]", 0 },
    { 0x1D00, 0xFF00, "SUB"  , "A,#b"    , 0 },
    { 0x1E00, 0xFF00, "SUB"  , "A,s"     , 0 },
    { 0x1F00, 0xFF00, "SUB"  , "A,[HL]"  , 0 },

    { 0x2000, 0xFF00, "SET1" , "CY"      , 0 },
    { 0x2100, 0xFF00, "CLR1" , "CY"      , 0 },
    { 0x2200, 0xFF00, "PUSH" , "PSW"     , 0 },
    { 0x2300, 0xFF00, "POP"  , "PSW"     , 0 },
    { 0x2400, 0xFF00, "ROR"  , "A,1"     , 0 },
    { 0x2500, 0xFF00, "ROL"  , "A,1"     , 0 },
    { 0x2600, 0xFF00, "RORC" , "A,1"     , 0 },
    { 0x2700, 0xFF00, "ROLC" , "A,1"     , 0 },
    { 0x2800, 0xFF00, "ADDC" , "A,w"     , 0 },
    { 0x2900, 0xFF00, "ADDC" , "A,[HL+b]", 0 },
    { 0x2D00, 0xFF00, "ADDC" , "A,#b"    , 0 },
    { 0x2E00, 0xFF00, "ADDC" , "A,s"     , 0 },
    { 0x2F00, 0xFF00, "ADDC" , "A,[HL]"  , 0 },

    { 0x3000, 0xF800, "XCH"  , "A,r"     , 0 },
    { 0x3800, 0xFF00, "SUBC" , "A,w"     , 0 },
    { 0x3900, 0xFF00, "SUBC" , "A,[HL+b]", 0 },
    { 0x3D00, 0xFF00, "SUBC" , "A,#b"    , 0 },
    { 0x3E00, 0xFF00, "SUBC" , "A,s"     , 0 },
    { 0x3F00, 0xFF00, "SUBC" , "A,[HL]"  , 0 },

    { 0x4000, 0xF800, "INC"  , "r"       , 0 },
    { 0x4800, 0xFF00, "CMP"  , "A,w"     , 0 },
    { 0x4900, 0xFF00, "CMP"  , "[HL+b]"  , 0 },
    { 0x4D00, 0xFF00, "CMP"  , "A,#b"    , 0 },
    { 0x4E00, 0xFF00, "CMP"  , "A,s"     , 0 },
    { 0x4F00, 0xFF00, "CMP"  , "A,[HL]"  , 0 },

    { 0x5000, 0xF800, "DEC"  , "r"       , 0 },
    { 0x5800, 0xFF00, "AND"  , "A,w"     , 0 },
    { 0x5900, 0xFF00, "AND"  , "[HL+b]"  , 0 },
    { 0x5D00, 0xFF00, "AND"  , "A,#b"    , 0 },
    { 0x5E00, 0xFF00, "AND"  , "A,s"     , 0 },
    { 0x5F00, 0xFF00, "AND"  , "A,[HL]"  , 0 },

    { 0x6000, 0xF800, "MOV"  , "A,r"     , 0 },
    { 0x6800, 0xFF00, "OR"   , "A,w"     , 0 },
    { 0x6900, 0xFF00, "OR"   , "[HL+b]"  , 0 },
    { 0x6D00, 0xFF00, "OR"   , "A,#b"    , 0 },
    { 0x6E00, 0xFF00, "OR"   , "A,s"     , 0 },
    { 0x6F00, 0xFF00, "OR"   , "A,[HL]"  , 0 },

    { 0x7000, 0xF800, "MOV"  , "r,A"     , 0 },
    { 0x7800, 0xFF00, "XOR"  , "A,w"     , 0 },
    { 0x7900, 0xFF00, "XOR"  , "[HL+b]"  , 0 },
    { 0x7D00, 0xFF00, "XOR"  , "A,#b"    , 0 },
    { 0x7E00, 0xFF00, "XOR"  , "A,s"     , 0 },
    { 0x7F00, 0xFF00, "XOR"  , "A,[HL]"  , 0 },

    { 0x8000, 0xF900, "INCW" , "p"       , 0 },
    { 0x8100, 0xFF00, "INC"  , "s"       , 0 },
    { 0x8300, 0xFF00, "XCH"  , "A,s"     , 0 },
    { 0x8500, 0xFD00, "MOV"  , "A,[p]"   , 0 },
    { 0x8800, 0xFF00, "ADD"  , "s,b"     , 0 },
    { 0x8900, 0xFF00, "MOV"  , "AX,s"    , 0 },
    { 0x8A00, 0xFE00, "DBNZ" , "r,e"     , REFFLAG | CODEREF },
    { 0x8C00, 0x8F00, "BT"   , "s.x,w"   , 0 },
    { 0x8D00, 0xFF00, "BC"   , "e"       , REFFLAG | CODEREF },
    { 0x8E00, 0xFF00, "MOV"  , "A,w"     , 0 },
    { 0x8F00, 0xFF00, "RETI" , ""        , LFFLAG },

    { 0x9000, 0xF900, "DECW" , "p"       , 0 },
    { 0x9100, 0xFF00, "DEC"  , "s"       , 0 },
    { 0x9300, 0xFF00, "XCH"  , "A,s"     , 0 },
    { 0x9500, 0xFD00, "MOV"  , "[p],A"   , 0 },
    { 0x9800, 0xFF00, "SUB"  , "s,b"     , 0 },
    { 0x9900, 0xFF00, "MOVW" , "s,AX"    , 0 },
    { 0x9A00, 0xFF00, "CALL" , "w"       , REFFLAG | CODEREF },
    { 0x9B00, 0xFF00, "BR"   , "w"       , LFFLAG | REFFLAG | CODEREF },
    { 0x9D00, 0xFF00, "BNC"  , "e"       , REFFLAG | CODEREF },
    { 0x9E00, 0xFF00, "MOV"  , "w,A"     , 0 },
    { 0x9F00, 0xFF00, "RETB" , ""        , LFFLAG },

    { 0xA000, 0xF800, "MOV"  , "r,#b"    , 0 },
    { 0xA800, 0xFF00, "ADDC" , "s,b"     , 0 },
    { 0xA900, 0xFF00, "MOVW" , "AX,f"    , 0 },
    { 0xAA00, 0xFF00, "MOV"  , "A,[HL+r]", 0 },
    { 0xAB00, 0xFF00, "MOV"  , "A,[HL+r]", 0 },
    { 0xAD00, 0xFF00, "BZ"   , "e"       , REFFLAG | CODEREF },
    { 0xAE00, 0xFF00, "MOV"  , "A,[HL+b]", 0 },
    { 0xAF00, 0xFF00, "RET"  , ""        , LFFLAG },

    { 0xB000, 0xF900, "POP"  , "p"       , 0 },
    { 0xB100, 0xF900, "PUSH" , "p"       , 0 },
    { 0xB800, 0xFF00, "SUBC" , "s,b"     , 0 },
    { 0xB900, 0xFF00, "MOVW" , "f,AX"    , 0 },
    { 0xBA00, 0xFF00, "MOV"  , "[HL+r],A", 0 },
    { 0xBB00, 0xFF00, "MOV"  , "[HL+r],A", 0 },
    { 0xBD00, 0xFF00, "BNZ"  , "e"       , REFFLAG | CODEREF },
    { 0xBE00, 0xFF00, "MOV"  , "[HL+b],A", 0 },
    { 0xBF00, 0xFF00, "BRK"  , ""        , 0 },

    { 0xC000, 0xFF00, ""     , ""        , 0 },
    { 0xC100, 0xC100, "CALLT", "t"       , REFFLAG },
    { 0xC000, 0xF900, "MOVW" , "AX,p"    , 0 },
    { 0xC800, 0xFF00, "CMP"  , "s,b"     , 0 },
    { 0xCA00, 0xFF00, "ADDW" , "AX,#w"   , 0 },
    { 0xCE00, 0xFF00, "XCH"  , "A,w"     , 0 },

    { 0xD000, 0xFF00, ""     , ""        , 0 },
    { 0xD000, 0xF900, "MOVW" , "p,AX"    , 0 },
    { 0xD800, 0xFF00, "AND"  , "s,b"     , 0 },
    { 0xDA00, 0xFF00, "SUBW" , "AX,#w"   , 0 },
    { 0xDE00, 0xFF00, "XCH"  , "A,[HL+b]", 0 },

    { 0xE000, 0xFF00, ""     , ""        , 0 },
    { 0xE000, 0xF900, "XCHW" , "AX,p"    , 0 },
    { 0xE800, 0xFF00, "OR"   , "s,b"     , 0 },
    { 0xEA00, 0xFF00, "CMPW" , "AX,#w"   , 0 },
    { 0xEE00, 0xFF00, "MOVW" , "s,#w"    , 0 },

    { 0xF000, 0xFF00, "MOV"  , "A,s"     , 0 },
    { 0xF200, 0xFF00, "MOV"  , "s,A"     , 0 },
    { 0xF400, 0xFF00, "MOV"  , "A,f"     , 0 },
    { 0xF600, 0xFF00, "MOV"  , "f,A"     , 0 },
    { 0xF800, 0xFF00, "XOR"  , "s,b"     , 0 },
    { 0xFA00, 0xFF00, "BR"   , "e"       , REFFLAG | CODEREF },
    { 0xFE00, 0xFF00, "MOVW" , "f,#w"    , 0 },

    {   0   ,   0   , ""     , ""        , 0 },
};


// =====================================================
int Dis78K0::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
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
    instr = FindInstr(opcdTable_78K0, op);

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

                // == CALLT ==
                case 't':
                    ra = (op & 0x3E) + 0x0040;
                    *p++ = '[';
                    *p = 0;
                    RefStr(ra, p, lfref, refaddr);
                    p += strlen(p);
                    *p++ = ']';
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
                break;

                // == CALLF ==
                case 'c': // 11 bit address
                    ra = 0x0800 | ((op & 0x70) << 4) | ReadByte(ad++);
                    len++;
                    RefStr(ra, p, lfref, refaddr);
                    p += strlen(p);
                    break;

                // === r ===
                case 'r':
                    i = op & 0x07;
                    *p++ = "XACBEDLH"[i];
                    break;

                // == saddr == refers to address range FE20-FF1F
                case 's':
                    i = ReadByte(ad++);
                    len++;
                    switch (i) {
                        // these are the ones that I have found documented
                        case 0x1C:
                            strcat(p,"SP");
                            break;

                        case 0x1E:
                            strcat(p,"PSW");
                            break;

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
                    p += strlen(p);
                    break;

                // == sfr == refers to address range FF00-FFFF
                case 'f':
                    i = ReadByte(ad++);
                    len++;
                    strcat(p, "sfr:");
                    if (i < 0x20) {
                        i += 256; // adjust for wraparound
                    }
                    ra = 0xFF00 + i;
                    RefStr(ra, p, lfref, refaddr);
                    p += strlen(p);
                    break;

                // == rp ==
                case 'p':
                    i = (op >> 1) & 0x03;
                    *p++ = "ABDH"[i]; // AX BC DE HL
                    *p++ = "XCEL"[i];
                    break;

                // == bit, low nibble ==
                case 'z':
                    i = op & 0x07;
                    *p++ = '0' + i;
                    break;

                // == bit, high nibble ==
                case 'x':
                    i = (op >> 4) & 0x07;
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


