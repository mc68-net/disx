// dis6502.cpp

static const char versionName[] = "MOS Technology 6502 disassembler";

#include "discpu.h"

class Dis6502 : public CPU {
public:
    Dis6502(const char *name, int subtype, int endian, int addrwid,
            char curAddrChr, char hexChr, const char *byteOp,
            const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);

private:
    char *RefStrZP(addr_t addr, char *s, int &lfref, addr_t &refaddr) const;

};


enum {
    CPU_6502,   // classic 6502 (with ROR instruction)
    CPU_6502U,  // 6502 with commonly accepted mnemonics for undocumented opcodes
    CPU_65C02,  // 65C02 with bit instructions
    CPU_65SC02, // 65C02 without bit instructions
    CPU_65C816, // 65HC16
};


// CPU type objects
Dis6502 cpu_6502  ("6502",   CPU_6502,   LITTLE_END, ADDR_16, '*', '$', "FCB",  "FDB",  "DC.L");
Dis6502 cpu_6502U ("6502U",  CPU_6502U,  LITTLE_END, ADDR_16, '*', '$', "FCB",  "FDB",  "DC.L");
Dis6502 cpu_65C02 ("65C02",  CPU_65C02,  LITTLE_END, ADDR_16, '*', '$', "FCB",  "FDB",  "DC.L");
Dis6502 cpu_65SC02("65SC02", CPU_65SC02, LITTLE_END, ADDR_16, '*', '$', "FCB",  "FDB",  "DC.L");
Dis6502 cpu_65816 ("65816",  CPU_65C816, LITTLE_END, ADDR_16, '*', '$', "DC.B", "DC.W", "DC.L");
Dis6502 cpu_65C816("65C816", CPU_65C816, LITTLE_END, ADDR_16, '*', '$', "DC.B", "DC.W", "DC.L");


Dis6502::Dis6502(const char *name, int subtype, int endian, int addrwid,
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


int mode;

enum InstType {
    iImplied,
    iAccum,     // A
    iImmediate, // #i
    iRelative,  // r
    iZeroPage,  // d
    iZeroPageX, // d,X
    iZeroPageY, // d,Y
    iAbsolute,  // a
    iAbsoluteX, // a,X
    iAbsoluteY, // a,Y
    iIndirect,  // (a)
    iIndirectX, // (d,X)
    iIndirectY, // (d),Y

    // 65C02 modes
    iDirIndir,  // (d)
    iAbsIndX,   // (a,X) - for JMP only
    iBBRBBS,    // BBRn $zp,ofs / BBSn $zp,ofs

    // 65C816 modes
    iAbsLong,   // al
    iAbsLongX,  // al,X
    iDirIndirL, // [d]
    iDirIndirY, // [d],Y
    iStackRel,  // d,S
    iStackIndY, // (d,S),Y
    iRelativeL, // rl
    iBlockMove, // MVN,MVP
    // Because of the M and X flags, some 65816 instructions can have variable
    // immediate operand lengths determined by code that was executed elsewhere.
    // These need manually-set hint flags to be disassembled properly.
    iImmediateA,// immediate for A/C (.LONGA ON/OFF) 16-bit when M flag clear
    iImmediateI,// immediate for X/Y (.LONGI ON/OFF) 16-bit when X flag clear
};

enum {
    // meaning of hint flags for 65816
    HINT_LONGI = 1, // hint bit for .LONGI (X/Y immediate 8/16 bits)
    HINT_LONGA = 2, // hint bit for .LONGA (A/C immediate 8/16 bits)
};

struct InstrRec {
    const char      *op;    // mnemonic
    enum InstType   typ;    // typ
    uint8_t         lfref;  // lfFlag/refFlag/codeRef
};

static const struct InstrRec opcdTable6502[] = {
    // op      typ            lfref
    {"BRK", iImplied  , 0                },  // 00
    {"ORA", iIndirectX, 0                },  // 01
    {""   , iImplied  , 0                },  // 02
    {""   , iImplied  , 0                },  // 03
    {""   , iImplied  , 0                },  // 04
    {"ORA", iZeroPage , 0                },  // 05
    {"ASL", iZeroPage , 0                },  // 06
    {""   , iImplied  , 0                },  // 07
    {"PHP", iImplied  , 0                },  // 08
    {"ORA", iImmediate, 0                },  // 09
    {"ASL", iAccum    , 0                },  // 0A
    {""   , iImplied  , 0                },  // 0B
    {""   , iImplied  , 0                },  // 0C
    {"ORA", iAbsolute , 0                },  // 0D
    {"ASL", iAbsolute , 0                },  // 0E
    {""   , iImplied  , 0                },  // 0F

    {"BPL", iRelative , REFFLAG | CODEREF},  // 10
    {"ORA", iIndirectY, 0                },  // 11
    {""   , iImplied  , 0                },  // 12
    {""   , iImplied  , 0                },  // 13
    {""   , iImplied  , 0                },  // 14
    {"ORA", iZeroPageX, 0                },  // 15
    {"ASL", iZeroPageX, 0                },  // 16
    {""   , iImplied  , 0                },  // 17
    {"CLC", iImplied  , 0                },  // 18
    {"ORA", iAbsoluteY, 0                },  // 19
    {""   , iImplied  , 0                },  // 1A
    {""   , iImplied  , 0                },  // 1B
    {""   , iImplied  , 0                },  // 1C
    {"ORA", iAbsoluteX, 0                },  // 1D
    {"ASL", iAbsoluteX, 0                },  // 1E
    {""   , iImplied  , 0                },  // 1F

    {"JSR", iAbsolute , REFFLAG | CODEREF},  // 20
    {"AND", iIndirectX, 0                },  // 21
    {""   , iImplied  , 0                },  // 22
    {""   , iImplied  , 0                },  // 23
    {"BIT", iZeroPage , 0                },  // 24
    {"AND", iZeroPage , 0                },  // 25
    {"ROL", iZeroPage , 0                },  // 26
    {""   , iImplied  , 0                },  // 27
    {"PLP", iImplied  , 0                },  // 28
    {"AND", iImmediate, 0                },  // 29
    {"ROL", iAccum    , 0                },  // 2A
    {""   , iImplied  , 0                },  // 2B
    {"BIT", iAbsolute , 0                },  // 2C
    {"AND", iAbsolute , 0                },  // 2D
    {"ROL", iAbsolute , 0                },  // 2E
    {""   , iImplied  , 0                },  // 2F

    {"BMI", iRelative , REFFLAG | CODEREF},  // 30
    {"AND", iIndirectY, 0                },  // 31
    {""   , iImplied  , 0                },  // 32
    {""   , iImplied  , 0                },  // 33
    {""   , iImplied  , 0                },  // 34
    {"AND", iZeroPageX, 0                },  // 35
    {"ROL", iZeroPageX, 0                },  // 36
    {""   , iImplied  , 0                },  // 37
    {"SEC", iImplied  , 0                },  // 38
    {"AND", iAbsoluteY, 0                },  // 39
    {""   , iImplied  , 0                },  // 3A
    {""   , iImplied  , 0                },  // 3B
    {""   , iImplied  , 0                },  // 3C
    {"AND", iAbsoluteX, 0                },  // 3D
    {"ROL", iAbsoluteX, 0                },  // 3E
    {""   , iImplied  , 0                },  // 3F

    {"RTI", iImplied  , LFFLAG           },  // 40
    {"EOR", iIndirectX, 0                },  // 41
    {""   , iImplied  , 0                },  // 42
    {""   , iImplied  , 0                },  // 43
    {""   , iImplied  , 0                },  // 44
    {"EOR", iZeroPage , 0                },  // 45
    {"LSR", iZeroPage , 0                },  // 46
    {""   , iImplied  , 0                },  // 47
    {"PHA", iImplied  , 0                },  // 48
    {"EOR", iImmediate, 0                },  // 49
    {"LSR", iAccum    , 0                },  // 4A
    {""   , iImplied  , 0                },  // 4B
    {"JMP", iAbsolute , LFFLAG | REFFLAG | CODEREF},  // 4C
    {"EOR", iAbsolute , 0                },  // 4D
    {"LSR", iAbsolute , 0                },  // 4E
    {""   , iImplied  , 0                },  // 4F

    {"BVC", iRelative , REFFLAG | CODEREF},  // 50
    {"EOR", iIndirectY, 0                },  // 51
    {""   , iImplied  , 0                },  // 52
    {""   , iImplied  , 0                },  // 53
    {""   , iImplied  , 0                },  // 54
    {"EOR", iZeroPageX, 0                },  // 55
    {"LSR", iZeroPageX, 0                },  // 56
    {""   , iImplied  , 0                },  // 57
    {"CLI", iImplied  , 0                },  // 58
    {"EOR", iAbsoluteY, 0                },  // 59
    {""   , iImplied  , 0                },  // 5A
    {""   , iImplied  , 0                },  // 5B
    {""   , iImplied  , 0                },  // 5C
    {"EOR", iAbsoluteX, 0                },  // 5D
    {"LSR", iAbsoluteX, 0                },  // 5E
    {""   , iImplied  , 0                },  // 5F

    {"RTS", iImplied  , LFFLAG           },  // 60
    {"ADC", iIndirectX, 0                },  // 61
    {""   , iImplied  , 0                },  // 62
    {""   , iImplied  , 0                },  // 63
    {""   , iImplied  , 0                },  // 64
    {"ADC", iZeroPage , 0                },  // 65
    {"ROR", iZeroPage , 0                },  // 66
    {""   , iImplied  , 0                },  // 67
    {"PLA", iImplied  , 0                },  // 68
    {"ADC", iImmediate, 0                },  // 69
    {"ROR", iAccum    , 0                },  // 6A
    {""   , iImplied  , 0                },  // 6B
    {"JMP", iIndirect , LFFLAG           },  // 6C
    {"ADC", iAbsolute , 0                },  // 6D
    {"ROR", iAbsolute , 0                },  // 6E
    {""   , iImplied  , 0                },  // 6F

    {"BVS", iRelative , REFFLAG | CODEREF},  // 70
    {"ADC", iIndirectY, 0                },  // 71
    {""   , iImplied  , 0                },  // 72
    {""   , iImplied  , 0                },  // 73
    {""   , iImplied  , 0                },  // 74
    {"ADC", iZeroPageX, 0                },  // 75
    {"ROR", iZeroPageX, 0                },  // 76
    {""   , iImplied  , 0                },  // 77
    {"SEI", iImplied  , 0                },  // 78
    {"ADC", iAbsoluteY, 0                },  // 79
    {""   , iImplied  , 0                },  // 7A
    {""   , iImplied  , 0                },  // 7B
    {""   , iImplied  , 0                },  // 7C
    {"ADC", iAbsoluteX, 0                },  // 7D
    {"ROR", iAbsoluteX, 0                },  // 7E
    {""   , iImplied  , 0                },  // 7F

    {""   , iImplied  , 0                },  // 80
    {"STA", iIndirectX, 0                },  // 81
    {""   , iImplied  , 0                },  // 82
    {""   , iImplied  , 0                },  // 83
    {"STY", iZeroPage , 0                },  // 84
    {"STA", iZeroPage , 0                },  // 85
    {"STX", iZeroPage , 0                },  // 86
    {""   , iImplied  , 0                },  // 87
    {"DEY", iImplied  , 0                },  // 88
    {""   , iImplied  , 0                },  // 89
    {"TXA", iImplied  , 0                },  // 8A
    {""   , iImplied  , 0                },  // 8B
    {"STY", iAbsolute , 0                },  // 8C
    {"STA", iAbsolute , 0                },  // 8D
    {"STX", iAbsolute , 0                },  // 8E
    {""   , iImplied  , 0                },  // 8F

    {"BCC", iRelative , REFFLAG | CODEREF},  // 90
    {"STA", iIndirectY, 0                },  // 91
    {""   , iImplied  , 0                },  // 92
    {""   , iImplied  , 0                },  // 93
    {"STY", iZeroPageX, 0                },  // 94
    {"STA", iZeroPageX, 0                },  // 95
    {"STX", iZeroPageY, 0                },  // 96
    {""   , iImplied  , 0                },  // 97
    {"TYA", iImplied  , 0                },  // 98
    {"STA", iAbsoluteY, 0                },  // 99
    {"TXS", iImplied  , 0                },  // 9A
    {""   , iImplied  , 0                },  // 9B
    {""   , iImplied  , 0                },  // 9C
    {"STA", iAbsoluteX, 0                },  // 9D
    {""   , iImplied  , 0                },  // 9E
    {""   , iImplied  , 0                },  // 9F

    {"LDY", iImmediate, 0                },  // A0
    {"LDA", iIndirectX, 0                },  // A1
    {"LDX", iImmediate, 0                },  // A2
    {""   , iImplied  , 0                },  // A3
    {"LDY", iZeroPage , 0                },  // A4
    {"LDA", iZeroPage , 0                },  // A5
    {"LDX", iZeroPage , 0                },  // A6
    {""   , iImplied  , 0                },  // A7
    {"TAY", iImplied  , 0                },  // A8
    {"LDA", iImmediate, 0                },  // A9
    {"TAX", iImplied  , 0                },  // AA
    {""   , iImplied  , 0                },  // AB
    {"LDY", iAbsolute , 0                },  // AC
    {"LDA", iAbsolute , 0                },  // AD
    {"LDX", iAbsolute , 0                },  // AE
    {""   , iImplied  , 0                },  // AF

    {"BCS", iRelative , REFFLAG | CODEREF},  // B0
    {"LDA", iIndirectY, 0                },  // B1
    {""   , iImplied  , 0                },  // B2
    {""   , iImplied  , 0                },  // B3
    {"LDY", iZeroPageX, 0                },  // B4
    {"LDA", iZeroPageX, 0                },  // B5
    {"LDX", iZeroPageY, 0                },  // B6
    {""   , iImplied  , 0                },  // B7
    {"CLV", iImplied  , 0                },  // B8
    {"LDA", iAbsoluteY, 0                },  // B9
    {"TSX", iImplied  , 0                },  // BA
    {""   , iImplied  , 0                },  // BB
    {"LDY", iAbsoluteX, 0                },  // BC
    {"LDA", iAbsoluteX, 0                },  // BD
    {"LDX", iAbsoluteY, 0                },  // BE
    {""   , iImplied  , 0                },  // BF

    {"CPY", iImmediate, 0                },  // C0
    {"CMP", iIndirectX, 0                },  // C1
    {""   , iImplied  , 0                },  // C2
    {""   , iImplied  , 0                },  // C3
    {"CPY", iZeroPage , 0                },  // C4
    {"CMP", iZeroPage , 0                },  // C5
    {"DEC", iZeroPage , 0                },  // C6
    {""   , iImplied  , 0                },  // C7
    {"INY", iImplied  , 0                },  // C8
    {"CMP", iImmediate, 0                },  // C9
    {"DEX", iImplied  , 0                },  // CA
    {""   , iImplied  , 0                },  // CB
    {"CPY", iAbsolute , 0                },  // CC
    {"CMP", iAbsolute , 0                },  // CD
    {"DEC", iAbsolute , 0                },  // CE
    {""   , iImplied  , 0                },  // CF

    {"BNE", iRelative , REFFLAG | CODEREF},  // D0
    {"CMP", iIndirectY, 0                },  // D1
    {""   , iImplied  , 0                },  // D2
    {""   , iImplied  , 0                },  // D3
    {""   , iImplied  , 0                },  // D4
    {"CMP", iZeroPageX, 0                },  // D5
    {"DEC", iZeroPageX, 0                },  // D6
    {""   , iImplied  , 0                },  // D7
    {"CLD", iImplied  , 0                },  // D8
    {"CMP", iAbsoluteY, 0                },  // D9
    {""   , iImplied  , 0                },  // DA
    {""   , iImplied  , 0                },  // DB
    {""   , iImplied  , 0                },  // DC
    {"CMP", iAbsoluteX, 0                },  // DD
    {"DEC", iAbsoluteX, 0                },  // DE
    {""   , iImplied  , 0                },  // DF

    {"CPX", iImmediate, 0                },  // E0
    {"SBC", iIndirectX, 0                },  // E1
    {""   , iImplied  , 0                },  // E2
    {""   , iImplied  , 0                },  // E3
    {"CPX", iZeroPage , 0                },  // E4
    {"SBC", iZeroPage , 0                },  // E5
    {"INC", iZeroPage , 0                },  // E6
    {""   , iImplied  , 0                },  // E7
    {"INX", iImplied  , 0                },  // E8
    {"SBC", iImmediate, 0                },  // E9
    {"NOP", iImplied  , 0                },  // EA
    {""   , iImplied  , 0                },  // EB
    {"CPX", iAbsolute , 0                },  // EC
    {"SBC", iAbsolute , 0                },  // ED
    {"INC", iAbsolute , 0                },  // EE
    {""   , iImplied  , 0                },  // EF

    {"BEQ", iRelative , REFFLAG | CODEREF},  // F0
    {"SBC", iIndirectY, 0                },  // F1
    {""   , iImplied  , 0                },  // F2
    {""   , iImplied  , 0                },  // F3
    {""   , iImplied  , 0                },  // F4
    {"SBC", iZeroPageX, 0                },  // F5
    {"INC", iZeroPageX, 0                },  // F6
    {""   , iImplied  , 0                },  // F7
    {"SED", iImplied  , 0                },  // F8
    {"SBC", iAbsoluteY, 0                },  // F9
    {""   , iImplied  , 0                },  // FA
    {""   , iImplied  , 0                },  // FB
    {""   , iImplied  , 0                },  // FC
    {"SBC", iAbsoluteX, 0                },  // FD
    {"INC", iAbsoluteX, 0                },  // FE
    {""   , iImplied  , 0                }   // FF
};

static const struct InstrRec opcdTable6502U[] = {
    // op      typ            lfref
    {"BRK", iImplied  , 0                },  // 00
    {"ORA", iIndirectX, 0                },  // 01
    {""   , iImplied  , 0                },  // 02
    {"SLO", iIndirectX, 0                },  // 03
    {""   , iImplied  , 0                },  // 04 - 3-cycle NOP
    {"ORA", iZeroPage , 0                },  // 05
    {"ASL", iZeroPage , 0                },  // 06
    {"SLO", iZeroPage , 0                },  // 07
    {"PHP", iImplied  , 0                },  // 08
    {"ORA", iImmediate, 0                },  // 09
    {"ASL", iAccum    , 0                },  // 0A
    {""   , iImplied  , 0                },  // 0B
    {""   , iImplied  , 0                },  // 0C
    {"ORA", iAbsolute , 0                },  // 0D
    {"ASL", iAbsolute , 0                },  // 0E
    {"SLO", iAbsolute , 0                },  // 0F

    {"BPL", iRelative , REFFLAG | CODEREF},  // 10
    {"ORA", iIndirectY, 0                },  // 11
    {""   , iImplied  , 0                },  // 12
    {"SLO", iIndirectY, 0                },  // 13
    {""   , iImplied  , 0                },  // 14
    {"ORA", iZeroPageX, 0                },  // 15
    {"ASL", iZeroPageX, 0                },  // 16
    {"SLO", iZeroPageX, 0                },  // 17
    {"CLC", iImplied  , 0                },  // 18
    {"ORA", iAbsoluteY, 0                },  // 19
    {""   , iImplied  , 0                },  // 1A
    {"SLO", iAbsoluteY, 0                },  // 1B
    {""   , iImplied  , 0                },  // 1C
    {"ORA", iAbsoluteX, 0                },  // 1D
    {"ASL", iAbsoluteX, 0                },  // 1E
    {"SLO", iAbsoluteX, 0                },  // 1F

    {"JSR", iAbsolute , REFFLAG | CODEREF},  // 20
    {"AND", iIndirectX, 0                },  // 21
    {""   , iImplied  , 0                },  // 22
    {"RLA", iIndirectX, 0                },  // 23
    {"BIT", iZeroPage , 0                },  // 24
    {"AND", iZeroPage , 0                },  // 25
    {"ROL", iZeroPage , 0                },  // 26
    {"RLA", iZeroPage , 0                },  // 27
    {"PLP", iImplied  , 0                },  // 28
    {"AND", iImmediate, 0                },  // 29
    {"ROL", iAccum    , 0                },  // 2A
    {""   , iImplied  , 0                },  // 2B
    {"BIT", iAbsolute , 0                },  // 2C
    {"AND", iAbsolute , 0                },  // 2D
    {"ROL", iAbsolute , 0                },  // 2E
    {"RLA", iAbsolute , 0                },  // 2F

    {"BMI", iRelative , REFFLAG | CODEREF},  // 30
    {"AND", iIndirectY, 0                },  // 31
    {""   , iImplied  , 0                },  // 32
    {"RLA", iIndirectY, 0                },  // 33
    {""   , iImplied  , 0                },  // 34
    {"AND", iZeroPageX, 0                },  // 35
    {"ROL", iZeroPageX, 0                },  // 36
    {"RLA", iZeroPageX, 0                },  // 37
    {"SEC", iImplied  , 0                },  // 38
    {"AND", iAbsoluteY, 0                },  // 39
    {""   , iImplied  , 0                },  // 3A
    {"RLA", iAbsoluteY, 0                },  // 3B
    {""   , iImplied  , 0                },  // 3C
    {"AND", iAbsoluteX, 0                },  // 3D
    {"ROL", iAbsoluteX, 0                },  // 3E
    {"RLA", iAbsoluteX, 0                },  // 3F

    {"RTI", iImplied  , LFFLAG           },  // 40
    {"EOR", iIndirectX, 0                },  // 41
    {""   , iImplied  , 0                },  // 42
    {"SRE", iIndirectX, 0                },  // 43
    {""   , iImplied  , 0                },  // 44
    {"EOR", iZeroPage , 0                },  // 45
    {"LSR", iZeroPage , 0                },  // 46
    {"SRE", iZeroPage , 0                },  // 47
    {"PHA", iImplied  , 0                },  // 48
    {"EOR", iImmediate, 0                },  // 49
    {"LSR", iAccum    , 0                },  // 4A
    {""   , iImplied  , 0                },  // 4B
    {"JMP", iAbsolute , LFFLAG | REFFLAG | CODEREF},  // 4C
    {"EOR", iAbsolute , 0                },  // 4D
    {"LSR", iAbsolute , 0                },  // 4E
    {"SRE", iAbsolute , 0                },  // 4F

    {"BVC", iRelative , REFFLAG | CODEREF},  // 50
    {"EOR", iIndirectY, 0                },  // 51
    {""   , iImplied  , 0                },  // 52
    {"SRE", iIndirectY, 0                },  // 53
    {""   , iImplied  , 0                },  // 54
    {"EOR", iZeroPageX, 0                },  // 55
    {"LSR", iZeroPageX, 0                },  // 56
    {"SRE", iZeroPageX, 0                },  // 57
    {"CLI", iImplied  , 0                },  // 58
    {"EOR", iAbsoluteY, 0                },  // 59
    {""   , iImplied  , 0                },  // 5A
    {"SRE", iAbsoluteY, 0                },  // 5B
    {""   , iImplied  , 0                },  // 5C
    {"EOR", iAbsoluteX, 0                },  // 5D
    {"LSR", iAbsoluteX, 0                },  // 5E
    {"SRE", iAbsoluteX, 0                },  // 5F

    {"RTS", iImplied  , LFFLAG           },  // 60
    {"ADC", iIndirectX, 0                },  // 61
    {""   , iImplied  , 0                },  // 62
    {"RRA", iIndirectX, 0                },  // 63
    {""   , iImplied  , 0                },  // 64
    {"ADC", iZeroPage , 0                },  // 65
    {"ROR", iZeroPage , 0                },  // 66
    {"RRA", iZeroPage , 0                },  // 67
    {"PLA", iImplied  , 0                },  // 68
    {"ADC", iImmediate, 0                },  // 69
    {"ROR", iAccum    , 0                },  // 6A
    {""   , iImplied  , 0                },  // 6B
    {"JMP", iIndirect , LFFLAG           },  // 6C
    {"ADC", iAbsolute , 0                },  // 6D
    {"ROR", iAbsolute , 0                },  // 6E
    {"RRA", iAbsolute , 0                },  // 6F

    {"BVS", iRelative , REFFLAG | CODEREF},  // 70
    {"ADC", iIndirectY, 0                },  // 71
    {""   , iImplied  , 0                },  // 72
    {"RRA", iIndirectY, 0                },  // 73
    {""   , iImplied  , 0                },  // 74
    {"ADC", iZeroPageX, 0                },  // 75
    {"ROR", iZeroPageX, 0                },  // 76
    {"RRA", iZeroPageX, 0                },  // 77
    {"SEI", iImplied  , 0                },  // 78
    {"ADC", iAbsoluteY, 0                },  // 79
    {""   , iImplied  , 0                },  // 7A
    {"RRA", iAbsoluteY, 0                },  // 7B
    {""   , iImplied  , 0                },  // 7C
    {"ADC", iAbsoluteX, 0                },  // 7D
    {"ROR", iAbsoluteX, 0                },  // 7E
    {"RRA", iAbsoluteX, 0                },  // 7F

    {""   , iImplied  , 0                },  // 80
    {"STA", iIndirectX, 0                },  // 81
    {""   , iImplied  , 0                },  // 82
    {"SAX", iIndirectX, 0                },  // 83
    {"STY", iZeroPage , 0                },  // 84
    {"STA", iZeroPage , 0                },  // 85
    {"STX", iZeroPage , 0                },  // 86
    {"SAX", iZeroPage , 0                },  // 87
    {"DEY", iImplied  , 0                },  // 88
    {""   , iImplied  , 0                },  // 89
    {"TXA", iImplied  , 0                },  // 8A
    {""   , iImplied  , 0                },  // 8B
    {"STY", iAbsolute , 0                },  // 8C
    {"STA", iAbsolute , 0                },  // 8D
    {"STX", iAbsolute , 0                },  // 8E
    {"SAX", iAbsolute , 0                },  // 8F

    {"BCC", iRelative , REFFLAG | CODEREF},  // 90
    {"STA", iIndirectY, 0                },  // 91
    {""   , iImplied  , 0                },  // 92
    {""   , iImplied  , 0                },  // 93
    {"STY", iZeroPageX, 0                },  // 94
    {"STA", iZeroPageX, 0                },  // 95
    {"STX", iZeroPageY, 0                },  // 96
    {"SAX", iZeroPageY, 0                },  // 97
    {"TYA", iImplied  , 0                },  // 98
    {"STA", iAbsoluteY, 0                },  // 99
    {"TXS", iImplied  , 0                },  // 9A
    {""   , iImplied  , 0                },  // 9B
    {""   , iImplied  , 0                },  // 9C
    {"STA", iAbsoluteX, 0                },  // 9D
    {""   , iImplied  , 0                },  // 9E
    {""   , iImplied  , 0                },  // 9F

    {"LDY", iImmediate, 0                },  // A0
    {"LDA", iIndirectX, 0                },  // A1
    {"LDX", iImmediate, 0                },  // A2
    {"LAX", iIndirectX, 0                },  // A3
    {"LDY", iZeroPage , 0                },  // A4
    {"LDA", iZeroPage , 0                },  // A5
    {"LDX", iZeroPage , 0                },  // A6
    {"LAX", iZeroPage , 0                },  // A7
    {"TAY", iImplied  , 0                },  // A8
    {"LDA", iImmediate, 0                },  // A9
    {"TAX", iImplied  , 0                },  // AA
    {""   , iImplied  , 0                },  // AB
    {"LDY", iAbsolute , 0                },  // AC
    {"LDA", iAbsolute , 0                },  // AD
    {"LDX", iAbsolute , 0                },  // AE
    {"LAX", iAbsolute , 0                },  // AF

    {"BCS", iRelative , REFFLAG | CODEREF},  // B0
    {"LDA", iIndirectY, 0                },  // B1
    {""   , iImplied  , 0                },  // B2
    {"LAX", iIndirectY, 0                },  // B3
    {"LDY", iZeroPageX, 0                },  // B4
    {"LDA", iZeroPageX, 0                },  // B5
    {"LDX", iZeroPageY, 0                },  // B6
    {"LAX", iZeroPageY, 0                },  // B7
    {"CLV", iImplied  , 0                },  // B8
    {"LDA", iAbsoluteY, 0                },  // B9
    {"TSX", iImplied  , 0                },  // BA
    {""   , iImplied  , 0                },  // BB
    {"LDY", iAbsoluteX, 0                },  // BC
    {"LDA", iAbsoluteX, 0                },  // BD
    {"LDX", iAbsoluteY, 0                },  // BE
    {"LAX", iAbsoluteY, 0                },  // BF

    {"CPY", iImmediate, 0                },  // C0
    {"CMP", iIndirectX, 0                },  // C1
    {""   , iImplied  , 0                },  // C2
    {"DCP", iIndirectX, 0                },  // C3
    {"CPY", iZeroPage , 0                },  // C4
    {"CMP", iZeroPage , 0                },  // C5
    {"DEC", iZeroPage , 0                },  // C6
    {"DCP", iZeroPage , 0                },  // C7
    {"INY", iImplied  , 0                },  // C8
    {"CMP", iImmediate, 0                },  // C9
    {"DEX", iImplied  , 0                },  // CA
    {""   , iImplied  , 0                },  // CB
    {"CPY", iAbsolute , 0                },  // CC
    {"CMP", iAbsolute , 0                },  // CD
    {"DEC", iAbsolute , 0                },  // CE
    {"DCP", iAbsolute , 0                },  // CF

    {"BNE", iRelative , REFFLAG | CODEREF},  // D0
    {"CMP", iIndirectY, 0                },  // D1
    {""   , iImplied  , 0                },  // D2
    {"DCP", iIndirectY, 0                },  // D3
    {""   , iImplied  , 0                },  // D4
    {"CMP", iZeroPageX, 0                },  // D5
    {"DEC", iZeroPageX, 0                },  // D6
    {"DCP", iZeroPageX, 0                },  // D7
    {"CLD", iImplied  , 0                },  // D8
    {"CMP", iAbsoluteY, 0                },  // D9
    {""   , iImplied  , 0                },  // DA
    {"DCP", iAbsoluteY, 0                },  // DB
    {""   , iImplied  , 0                },  // DC
    {"CMP", iAbsoluteX, 0                },  // DD
    {"DEC", iAbsoluteX, 0                },  // DE
    {"DCP", iAbsoluteX, 0                },  // DF

    {"CPX", iImmediate, 0                },  // E0
    {"SBC", iIndirectX, 0                },  // E1
    {""   , iImplied  , 0                },  // E2
    {"ISB", iIndirectX, 0                },  // E3
    {"CPX", iZeroPage , 0                },  // E4
    {"SBC", iZeroPage , 0                },  // E5
    {"INC", iZeroPage , 0                },  // E6
    {"ISB", iZeroPage , 0                },  // E7
    {"INX", iImplied  , 0                },  // E8
    {"SBC", iImmediate, 0                },  // E9
    {"NOP", iImplied  , 0                },  // EA
    {""   , iImplied  , 0                },  // EB
    {"CPX", iAbsolute , 0                },  // EC
    {"SBC", iAbsolute , 0                },  // ED
    {"INC", iAbsolute , 0                },  // EE
    {"ISB", iAbsolute , 0                },  // EF

    {"BEQ", iRelative , REFFLAG | CODEREF},  // F0
    {"SBC", iIndirectY, 0                },  // F1
    {""   , iImplied  , 0                },  // F2
    {"ISB", iIndirectY, 0                },  // F3
    {""   , iImplied  , 0                },  // F4
    {"SBC", iZeroPageX, 0                },  // F5
    {"INC", iZeroPageX, 0                },  // F6
    {"ISB", iZeroPageX, 0                },  // F7
    {"SED", iImplied  , 0                },  // F8
    {"SBC", iAbsoluteY, 0                },  // F9
    {""   , iImplied  , 0                },  // FA
    {"ISB", iAbsoluteY, 0                },  // FB
    {""   , iImplied  , 0                },  // FC
    {"SBC", iAbsoluteX, 0                },  // FD
    {"INC", iAbsoluteX, 0                },  // FE
    {"ISB", iAbsoluteX, 0                }   // FF
};

static const struct InstrRec opcdTable65C02[] = {
    // op      typ            lfref
    {"BRK" , iImplied  , 0                },  // 00
    {"ORA" , iIndirectX, 0                },  // 01
    {""    , iImplied  , 0                },  // 02
    {""    , iImplied  , 0                },  // 03
    {"TSB" , iZeroPage , 0                },  // 04
    {"ORA" , iZeroPage , 0                },  // 05
    {"ASL" , iZeroPage , 0                },  // 06
    {"RMB0", iZeroPage , 0                },  // 07
    {"PHP" , iImplied  , 0                },  // 08
    {"ORA" , iImmediate, 0                },  // 09
    {"ASL" , iAccum    , 0                },  // 0A
    {""    , iImplied  , 0                },  // 0B
    {"TSB" , iAbsolute , 0                },  // 0C
    {"ORA" , iAbsolute , 0                },  // 0D
    {"ASL" , iAbsolute , 0                },  // 0E
    {"BBR0", iBBRBBS   , REFFLAG | CODEREF},  // 0F

    {"BPL" , iRelative , REFFLAG | CODEREF},  // 10
    {"ORA" , iIndirectY, 0                },  // 11
    {"ORA" , iDirIndir , 0                },  // 12
    {""    , iImplied  , 0                },  // 13
    {"TRB" , iZeroPage , 0                },  // 14
    {"ORA" , iZeroPageX, 0                },  // 15
    {"ASL" , iZeroPageX, 0                },  // 16
    {"RMB1", iZeroPage , 0                },  // 17
    {"CLC" , iImplied  , 0                },  // 18
    {"ORA" , iAbsoluteY, 0                },  // 19
    {"INC" , iAccum    , 0                },  // 1A
    {""    , iImplied  , 0                },  // 1B
    {"TRB" , iAbsolute , 0                },  // 1C
    {"ORA" , iAbsoluteX, 0                },  // 1D
    {"ASL" , iAbsoluteX, 0                },  // 1E
    {"BBR1", iBBRBBS   , REFFLAG | CODEREF},  // 1F

    {"JSR" , iAbsolute , REFFLAG | CODEREF},  // 20 
    {"AND" , iIndirectX, 0                },  // 21
    {""    , iImplied  , 0                },  // 22
    {""    , iImplied  , 0                },  // 23
    {"BIT" , iZeroPage , 0                },  // 24
    {"AND" , iZeroPage , 0                },  // 25
    {"ROL" , iZeroPage , 0                },  // 26
    {"RMB2", iZeroPage , 0                },  // 27
    {"PLP" , iImplied  , 0                },  // 28
    {"AND" , iImmediate, 0                },  // 29
    {"ROL" , iAccum    , 0                },  // 2A
    {""    , iImplied  , 0                },  // 2B
    {"BIT" , iAbsolute , 0                },  // 2C
    {"AND" , iAbsolute , 0                },  // 2D
    {"ROL" , iAbsolute , 0                },  // 2E
    {"BBR2", iBBRBBS   , REFFLAG | CODEREF},  // 2F

    {"BMI" , iRelative , REFFLAG | CODEREF},  // 30
    {"AND" , iIndirectY, 0                },  // 31
    {"AND" , iDirIndir , 0                },  // 32
    {""    , iImplied  , 0                },  // 33
    {"BIT" , iZeroPageX, 0                },  // 34
    {"AND" , iZeroPageX, 0                },  // 35
    {"ROL" , iZeroPageX, 0                },  // 36
    {"RMB3", iZeroPage , 0                },  // 37
    {"SEC" , iImplied  , 0                },  // 38
    {"AND" , iAbsoluteY, 0                },  // 39
    {"DEC" , iAccum    , 0                },  // 3A
    {""    , iImplied  , 0                },  // 3B
    {"BIT" , iAbsoluteX, 0                },  // 3C
    {"AND" , iAbsoluteX, 0                },  // 3D
    {"ROL" , iAbsoluteX, 0                },  // 3E
    {"BBR3", iBBRBBS   , REFFLAG | CODEREF},  // 3F

    {"RTI" , iImplied  , LFFLAG           },  // 40
    {"EOR" , iIndirectX, 0                },  // 41
    {""    , iImplied  , 0                },  // 42
    {""    , iImplied  , 0                },  // 43
    {""    , iImplied  , 0                },  // 44
    {"EOR" , iZeroPage , 0                },  // 45
    {"LSR" , iZeroPage , 0                },  // 46
    {"RMB4", iZeroPage , 0                },  // 47
    {"PHA" , iImplied  , 0                },  // 48
    {"EOR" , iImmediate, 0                },  // 49
    {"LSR" , iAccum    , 0                },  // 4A
    {""    , iImplied  , 0                },  // 4B
    {"JMP" , iAbsolute , LFFLAG | REFFLAG | CODEREF},  // 4C
    {"EOR" , iAbsolute , 0                },  // 4D
    {"LSR" , iAbsolute , 0                },  // 4E
    {"BBR4", iBBRBBS   , REFFLAG | CODEREF},  // 4F

    {"BVC" , iRelative , REFFLAG | CODEREF},  // 50
    {"EOR" , iIndirectY, 0                },  // 51
    {"EOR" , iDirIndir , 0                },  // 52
    {""    , iImplied  , 0                },  // 53
    {""    , iImplied  , 0                },  // 54
    {"EOR" , iZeroPageX, 0                },  // 55
    {"LSR" , iZeroPageX, 0                },  // 56
    {"RMB5", iZeroPage , 0                },  // 57
    {"CLI" , iImplied  , 0                },  // 58
    {"EOR" , iAbsoluteY, 0                },  // 59
    {"PHY" , iImplied  , 0                },  // 5A
    {""    , iImplied  , 0                },  // 5B
    {""    , iImplied  , 0                },  // 5C
    {"EOR" , iAbsoluteX, 0                },  // 5D
    {"LSR" , iAbsoluteX, 0                },  // 5E
    {"BBR5", iBBRBBS   , REFFLAG | CODEREF},  // 5F

    {"RTS" , iImplied  , LFFLAG           },  // 60
    {"ADC" , iIndirectX, 0                },  // 61
    {""    , iImplied  , 0                },  // 62
    {""    , iImplied  , 0                },  // 63
    {"STZ" , iZeroPage , 0                },  // 64
    {"ADC" , iZeroPage , 0                },  // 65
    {"ROR" , iZeroPage , 0                },  // 66
    {"RMB6", iZeroPage , 0                },  // 67
    {"PLA" , iImplied  , 0                },  // 68
    {"ADC" , iImmediate, 0                },  // 69
    {"ROR" , iAccum    , 0                },  // 6A
    {""    , iImplied  , 0                },  // 6B
    {"JMP" , iIndirect , LFFLAG           },  // 6C
    {"ADC" , iAbsolute , 0                },  // 6D
    {"ROR" , iAbsolute , 0                },  // 6E
    {"BBR6", iBBRBBS   , REFFLAG | CODEREF},  // 6F

    {"BVS" , iRelative , REFFLAG | CODEREF},  // 70
    {"ADC" , iIndirectY, 0                },  // 71
    {"ADC" , iDirIndir , 0                },  // 72
    {""    , iImplied  , 0                },  // 73
    {"STZ" , iZeroPageX, 0                },  // 74
    {"ADC" , iZeroPageX, 0                },  // 75
    {"ROR" , iZeroPageX, 0                },  // 76
    {"RMB7", iZeroPage , 0                },  // 77
    {"SEI" , iImplied  , 0                },  // 78
    {"ADC" , iAbsoluteY, 0                },  // 79
    {"PLY" , iImplied  , 0                },  // 7A
    {""    , iImplied  , 0                },  // 7B
    {"JMP" , iAbsIndX  , LFFLAG | REFFLAG },  // 7C
    {"ADC" , iAbsoluteX, 0                },  // 7D
    {"ROR" , iAbsoluteX, 0                },  // 7E
    {"BBR7", iBBRBBS   , REFFLAG | CODEREF},  // 7F

    {"BRA" , iRelative , LFFLAG | REFFLAG | CODEREF},  // 80
    {"STA" , iIndirectX, 0                },  // 81
    {""    , iImplied  , 0                },  // 82
    {""    , iImplied  , 0                },  // 83
    {"STY" , iZeroPage , 0                },  // 84
    {"STA" , iZeroPage , 0                },  // 85
    {"STX" , iZeroPage , 0                },  // 86
    {"SMB0", iZeroPage , 0                },  // 87
    {"DEY" , iImplied  , 0                },  // 88
    {"BIT" , iImmediate, 0                },  // 89
    {"TXA" , iImplied  , 0                },  // 8A
    {""    , iImplied  , 0                },  // 8B
    {"STY" , iAbsolute , 0                },  // 8C
    {"STA" , iAbsolute , 0                },  // 8D
    {"STX" , iAbsolute , 0                },  // 8E
    {"BBS0", iBBRBBS   , REFFLAG | CODEREF},  // 8F

    {"BCC" , iRelative , REFFLAG | CODEREF},  // 90
    {"STA" , iIndirectY, 0                },  // 91
    {"STA" , iDirIndir , 0                },  // 92
    {""    , iImplied  , 0                },  // 93
    {"STY" , iZeroPageX, 0                },  // 94
    {"STA" , iZeroPageX, 0                },  // 95
    {"STX" , iZeroPageY, 0                },  // 96
    {"SMB1", iZeroPage , 0                },  // 97
    {"TYA" , iImplied  , 0                },  // 98
    {"STA" , iAbsoluteY, 0                },  // 99
    {"TXS" , iImplied  , 0                },  // 9A
    {""    , iImplied  , 0                },  // 9B
    {"STZ" , iAbsolute , 0                },  // 9C
    {"STA" , iAbsoluteX, 0                },  // 9D
    {"STZ" , iAbsoluteX, 0                },  // 9E
    {"BBS1", iBBRBBS   , REFFLAG | CODEREF},  // 9F

    {"LDY" , iImmediate, 0                },  // A0
    {"LDA" , iIndirectX, 0                },  // A1
    {"LDX" , iImmediate, 0                },  // A2
    {""    , iImplied  , 0                },  // A3
    {"LDY" , iZeroPage , 0                },  // A4
    {"LDA" , iZeroPage , 0                },  // A5
    {"LDX" , iZeroPage , 0                },  // A6
    {"SMB2", iZeroPage , 0                },  // A7
    {"TAY" , iImplied  , 0                },  // A8
    {"LDA" , iImmediate, 0                },  // A9
    {"TAX" , iImplied  , 0                },  // AA
    {""    , iImplied  , 0                },  // AB
    {"LDY" , iAbsolute , 0                },  // AC
    {"LDA" , iAbsolute , 0                },  // AD
    {"LDX" , iAbsolute , 0                },  // AE
    {"BBS2", iBBRBBS   , REFFLAG | CODEREF},  // AF

    {"BCS" , iRelative , REFFLAG | CODEREF},  // B0
    {"LDA" , iIndirectY, 0                },  // B1
    {"LDA" , iDirIndir , 0                },  // B2
    {""    , iImplied  , 0                },  // B3
    {"LDY" , iZeroPageX, 0                },  // B4
    {"LDA" , iZeroPageX, 0                },  // B5
    {"LDX" , iZeroPageY, 0                },  // B6
    {"SMB3", iZeroPage , 0                },  // B7
    {"CLV" , iImplied  , 0                },  // B8
    {"LDA" , iAbsoluteY, 0                },  // B9
    {"TSX" , iImplied  , 0                },  // BA
    {""    , iImplied  , 0                },  // BB
    {"LDY" , iAbsoluteX, 0                },  // BC
    {"LDA" , iAbsoluteX, 0                },  // BD
    {"LDX" , iAbsoluteY, 0                },  // BE
    {"BBS3", iBBRBBS   , REFFLAG | CODEREF},  // BF

    {"CPY" , iImmediate, 0                },  // C0
    {"CMP" , iIndirectX, 0                },  // C1
    {""    , iImplied  , 0                },  // C2
    {""    , iImplied  , 0                },  // C3
    {"CPY" , iZeroPage , 0                },  // C4
    {"CMP" , iZeroPage , 0                },  // C5
    {"DEC" , iZeroPage , 0                },  // C6
    {"SMB4", iZeroPage , 0                },  // C7
    {"INY" , iImplied  , 0                },  // C8
    {"CMP" , iImmediate, 0                },  // C9
    {"DEX" , iImplied  , 0                },  // CA
    {""    , iImplied  , 0                },  // CB
    {"CPY" , iAbsolute , 0                },  // CC
    {"CMP" , iAbsolute , 0                },  // CD
    {"DEC" , iAbsolute , 0                },  // CE
    {"BBS4", iBBRBBS   , REFFLAG | CODEREF},  // CF

    {"BNE" , iRelative , REFFLAG | CODEREF},  // D0
    {"CMP" , iIndirectY, 0                },  // D1
    {"CMP" , iDirIndir , 0                },  // D2
    {""    , iImplied  , 0                },  // D3
    {""    , iImplied  , 0                },  // D4
    {"CMP" , iZeroPageX, 0                },  // D5
    {"DEC" , iZeroPageX, 0                },  // D6
    {"SMB5", iZeroPage , 0                },  // D7
    {"CLD" , iImplied  , 0                },  // D8
    {"CMP" , iAbsoluteY, 0                },  // D9
    {"PHX" , iImplied  , 0                },  // DA
    {""    , iImplied  , 0                },  // DB
    {""    , iImplied  , 0                },  // DC
    {"CMP" , iAbsoluteX, 0                },  // DD
    {"DEC" , iAbsoluteX, 0                },  // DE
    {"BBS5", iBBRBBS   , REFFLAG | CODEREF},  // DF

    {"CPX" , iImmediate, 0                },  // E0
    {"SBC" , iIndirectX, 0                },  // E1
    {""    , iImplied  , 0                },  // E2
    {""    , iImplied  , 0                },  // E3
    {"CPX" , iZeroPage , 0                },  // E4
    {"SBC" , iZeroPage , 0                },  // E5
    {"INC" , iZeroPage , 0                },  // E6
    {"SMB6", iZeroPage , 0                },  // E7
    {"INX" , iImplied  , 0                },  // E8
    {"SBC" , iImmediate, 0                },  // E9
    {"NOP" , iImplied  , 0                },  // EA
    {""    , iImplied  , 0                },  // EB
    {"CPX" , iAbsolute , 0                },  // EC
    {"SBC" , iAbsolute , 0                },  // ED
    {"INC" , iAbsolute , 0                },  // EE
    {"BBS6", iBBRBBS   , REFFLAG | CODEREF},  // EF

    {"BEQ" , iRelative , REFFLAG | CODEREF},  // F0
    {"SBC" , iIndirectY, 0                },  // F1
    {"SBC" , iDirIndir , 0                },  // F2
    {""    , iImplied  , 0                },  // F3
    {""    , iImplied  , 0                },  // F4
    {"SBC" , iZeroPageX, 0                },  // F5
    {"INC" , iZeroPageX, 0                },  // F6
    {"SMB7", iZeroPage , 0                },  // F7
    {"SED" , iImplied  , 0                },  // F8
    {"SBC" , iAbsoluteY, 0                },  // F9
    {"PLX" , iImplied  , 0                },  // FA
    {""    , iImplied  , 0                },  // FB
    {""    , iImplied  , 0                },  // FC
    {"SBC" , iAbsoluteX, 0                },  // FD
    {"INC" , iAbsoluteX, 0                },  // FE
    {"BBS7", iBBRBBS   , REFFLAG | CODEREF},  // FF
};

static const struct InstrRec opcdTable65C816[] = {
    // op      typ            lfref
    {"BRK", iImplied  , 0                },  // 00
    {"ORA", iIndirectX, 0                },  // 01
    {"COP", iImmediate, 0                },  // 02 (might need new type without '#')
    {"ORA", iStackRel , 0                },  // 03
    {"TSB", iZeroPage , 0                },  // 04
    {"ORA", iZeroPage , 0                },  // 05
    {"ASL", iZeroPage , 0                },  // 06
    {"ORA", iDirIndirL, 0                },  // 07
    {"PHP", iImplied  , 0                },  // 08
    {"ORA", iImmediateA,0                },  // 09
    {"ASL", iAccum    , 0                },  // 0A
    {"PHD", iImplied  , 0                },  // 0B
    {"TSB", iAbsolute , 0                },  // 0C
    {"ORA", iAbsolute , 0                },  // 0D
    {"ASL", iAbsolute , 0                },  // 0E
    {"ORA", iAbsLong  , 0                },  // 0F

    {"BPL", iRelative , REFFLAG | CODEREF},  // 10
    {"ORA", iIndirectY, 0                },  // 11
    {"ORA", iDirIndir , 0                },  // 12
    {"ORA", iStackIndY, 0                },  // 13
    {"TRB", iZeroPage , 0                },  // 14
    {"ORA", iZeroPageX, 0                },  // 15
    {"ASL", iZeroPageX, 0                },  // 16
    {"ORA", iDirIndirY, 0                },  // 17
    {"CLC", iImplied  , 0                },  // 18
    {"ORA", iAbsoluteY, 0                },  // 19
    {"INC", iAccum    , 0                },  // 1A
    {"TCS", iImplied  , 0                },  // 1B
    {"TRB", iAbsolute , 0                },  // 1C
    {"ORA", iAbsoluteX, 0                },  // 1D
    {"ASL", iAbsoluteX, 0                },  // 1E
    {"ORA", iAbsLongX , 0                },  // 1F

    {"JSR", iAbsolute , REFFLAG | CODEREF},  // 20 
    {"AND", iIndirectX, 0                },  // 21
    {"JSL", iAbsLong  , 0                },  // 22
    {"AND", iStackRel , 0                },  // 23
    {"BIT", iZeroPage , 0                },  // 24
    {"AND", iZeroPage , 0                },  // 25
    {"ROL", iZeroPage , 0                },  // 26
    {"AND", iDirIndirL, 0                },  // 27
    {"PLP", iImplied  , 0                },  // 28
    {"AND", iImmediateA,0                },  // 29
    {"ROL", iAccum    , 0                },  // 2A
    {"PLD", iImplied  , 0                },  // 2B
    {"BIT", iAbsolute , 0                },  // 2C
    {"AND", iAbsolute , 0                },  // 2D
    {"ROL", iAbsolute , 0                },  // 2E
    {"AND", iAbsLong  , 0                },  // 2F

    {"BMI", iRelative , REFFLAG | CODEREF},  // 30
    {"AND", iIndirectY, 0                },  // 31
    {"AND", iDirIndir , 0                },  // 32
    {"AND", iStackIndY, 0                },  // 33
    {"BIT", iZeroPageX, 0                },  // 34
    {"AND", iZeroPageX, 0                },  // 35
    {"ROL", iZeroPageX, 0                },  // 36
    {"AND", iDirIndirY, 0                },  // 37
    {"SEC", iImplied  , 0                },  // 38
    {"AND", iAbsoluteY, 0                },  // 39
    {"DEC", iAccum    , 0                },  // 3A
    {"TSC", iImplied  , 0                },  // 3B
    {"BIT", iAbsoluteX, 0                },  // 3C
    {"AND", iAbsoluteX, 0                },  // 3D
    {"ROL", iAbsoluteX, 0                },  // 3E
    {"AND", iAbsLongX , 0                },  // 3F

    {"RTI", iImplied  , LFFLAG           },  // 40
    {"EOR", iIndirectX, 0                },  // 41
    {"WDM", iImplied  , 0                },  // 42
    {"EOR", iStackRel , 0                },  // 43
    {"MVP", iBlockMove, 0                },  // 44
    {"EOR", iZeroPage , 0                },  // 45
    {"LSR", iZeroPage , 0                },  // 46
    {"EOR", iDirIndirL, 0                },  // 47
    {"PHA", iImplied  , 0                },  // 48
    {"EOR", iImmediateA,0                },  // 49
    {"LSR", iAccum    , 0                },  // 4A
    {"PHK", iImplied  , 0                },  // 4B
    {"JMP", iAbsolute , LFFLAG | REFFLAG | CODEREF},  // 4C
    {"EOR", iAbsolute , 0                },  // 4D
    {"LSR", iAbsolute , 0                },  // 4E
    {"EOR", iAbsLong  , 0                },  // 4F

    {"BVC", iRelative , REFFLAG | CODEREF},  // 50
    {"EOR", iIndirectY, 0                },  // 51
    {"EOR", iDirIndir , 0                },  // 52
    {"EOR", iStackIndY, 0                },  // 53
    {"MVN", iBlockMove, 0                },  // 54
    {"EOR", iZeroPageX, 0                },  // 55
    {"LSR", iZeroPageX, 0                },  // 56
    {"EOR", iDirIndirY, 0                },  // 57
    {"CLI", iImplied  , 0                },  // 58
    {"EOR", iAbsoluteY, 0                },  // 59
    {"PHY", iImplied  , 0                },  // 5A
    {"TCD", iImplied  , 0                },  // 5B
    {"JMP", iAbsLong  , 0                },  // 5C
    {"EOR", iAbsoluteX, 0                },  // 5D
    {"LSR", iAbsoluteX, 0                },  // 5E
    {"EOR", iAbsLongX , 0                },  // 5F

    {"RTS", iImplied  , LFFLAG           },  // 60
    {"ADC", iIndirectX, 0                },  // 61
    {"PER", iRelativeL, 0                },  // 62
    {"ADC", iStackRel , 0                },  // 63
    {"STZ", iZeroPage , 0                },  // 64
    {"ADC", iZeroPage , 0                },  // 65
    {"ROR", iZeroPage , 0                },  // 66
    {"ADC", iDirIndirL, 0                },  // 67
    {"PLA", iImplied  , 0                },  // 68
    {"ADC", iImmediateA,0                },  // 69
    {"ROR", iAccum    , 0                },  // 6A
    {"RTL", iImplied  , 0                },  // 6B
    {"JMP", iIndirect , LFFLAG           },  // 6C
    {"ADC", iAbsolute , 0                },  // 6D
    {"ROR", iAbsolute , 0                },  // 6E
    {"ADC", iAbsLong  , 0                },  // 6F

    {"BVS", iRelative , REFFLAG | CODEREF},  // 70
    {"ADC", iIndirectY, 0                },  // 71
    {"ADC", iDirIndir , 0                },  // 72
    {"ADC", iStackIndY, 0                },  // 73
    {"STZ", iZeroPageX, 0                },  // 74
    {"ADC", iZeroPageX, 0                },  // 75
    {"ROR", iZeroPageX, 0                },  // 76
    {"ADC", iDirIndirY, 0                },  // 77
    {"SEI", iImplied  , 0                },  // 78
    {"ADC", iAbsoluteY, 0                },  // 79
    {"PLY", iImplied  , 0                },  // 7A
    {"TDC", iImplied  , 0                },  // 7B
    {"JMP", iAbsIndX  , LFFLAG | REFFLAG },  // 7C
    {"ADC", iAbsoluteX, 0                },  // 7D
    {"ROR", iAbsoluteX, 0                },  // 7E
    {"ADC", iAbsLongX , 0                },  // 7F

    {"BRA", iRelative , LFFLAG | REFFLAG | CODEREF},  // 80
    {"STA", iIndirectX, 0                },  // 81
    {"BRL", iRelativeL, LFFLAG | REFFLAG | CODEREF},  // 82
    {"STA", iStackRel , 0                },  // 83
    {"STY", iZeroPage , 0                },  // 84
    {"STA", iZeroPage , 0                },  // 85
    {"STX", iZeroPage , 0                },  // 86
    {"STA", iDirIndirL, 0                },  // 87
    {"DEY", iImplied  , 0                },  // 88
    {"BIT", iImmediateA,0                },  // 89
    {"TXA", iImplied  , 0                },  // 8A
    {"PHB", iImplied  , 0                },  // 8B
    {"STY", iAbsolute , 0                },  // 8C
    {"STA", iAbsolute , 0                },  // 8D
    {"STX", iAbsolute , 0                },  // 8E
    {"STA", iAbsLong  , 0                },  // 8F

    {"BCC", iRelative , REFFLAG | CODEREF},  // 90
    {"STA", iIndirectY, 0                },  // 91
    {"STA", iDirIndir , 0                },  // 92
    {"STA", iStackIndY, 0                },  // 93
    {"STY", iZeroPageX, 0                },  // 94
    {"STA", iZeroPageX, 0                },  // 95
    {"STX", iZeroPageY, 0                },  // 96
    {"STA", iDirIndirY, 0                },  // 97
    {"TYA", iImplied  , 0                },  // 98
    {"STA", iAbsoluteY, 0                },  // 99
    {"TXS", iImplied  , 0                },  // 9A
    {"TXY", iImplied  , 0                },  // 9B
    {"STZ", iAbsolute , 0                },  // 9C
    {"STA", iAbsoluteX, 0                },  // 9D
    {"STZ", iAbsoluteX, 0                },  // 9E
    {"STA", iAbsLongX , 0                },  // 9F

    {"LDY", iImmediateI,0                },  // A0 // with X flag clear
    {"LDA", iIndirectX, 0                },  // A1
    {"LDX", iImmediateI,0                },  // A2 // with X flag clear
    {"LDA", iStackRel , 0                },  // A3
    {"LDY", iZeroPage , 0                },  // A4
    {"LDA", iZeroPage , 0                },  // A5
    {"LDX", iZeroPage , 0                },  // A6
    {"LDA", iDirIndirL, 0                },  // A7
    {"TAY", iImplied  , 0                },  // A8
    {"LDA", iImmediateA,0                },  // A9
    {"TAX", iImplied  , 0                },  // AA
    {"PLB", iImplied  , 0                },  // AB
    {"LDY", iAbsolute , 0                },  // AC
    {"LDA", iAbsolute , 0                },  // AD
    {"LDX", iAbsolute , 0                },  // AE
    {"LDA", iAbsLong  , 0                },  // AF

    {"BCS", iRelative , REFFLAG | CODEREF},  // B0
    {"LDA", iIndirectY, 0                },  // B1
    {"LDA", iDirIndir , 0                },  // B2
    {"LDA", iStackIndY, 0                },  // B3
    {"LDY", iZeroPageX, 0                },  // B4
    {"LDA", iZeroPageX, 0                },  // B5
    {"LDX", iZeroPageY, 0                },  // B6
    {"LDA", iDirIndirY, 0                },  // B7
    {"CLV", iImplied  , 0                },  // B8
    {"LDA", iAbsoluteY, 0                },  // B9
    {"TSX", iImplied  , 0                },  // BA
    {"TYX", iImplied  , 0                },  // BB
    {"LDY", iAbsoluteX, 0                },  // BC
    {"LDA", iAbsoluteX, 0                },  // BD
    {"LDX", iAbsoluteY, 0                },  // BE
    {"LDA", iAbsLongX , 0                },  // BF

    {"CPY", iImmediateI,0                },  // C0 // with X flag clear
    {"CMP", iIndirectX, 0                },  // C1
    {"REP", iImmediate, 0                },  // C2
    {"CMP", iStackRel , 0                },  // C3
    {"CPY", iZeroPage , 0                },  // C4
    {"CMP", iZeroPage , 0                },  // C5
    {"DEC", iZeroPage , 0                },  // C6
    {"CMP", iDirIndirL, 0                },  // C7
    {"INY", iImplied  , 0                },  // C8
    {"CMP", iImmediateA,0                },  // C9
    {"DEX", iImplied  , 0                },  // CA
    {"WAI", iImplied  , 0                },  // CB
    {"CPY", iAbsolute , 0                },  // CC
    {"CMP", iAbsolute , 0                },  // CD
    {"DEC", iAbsolute , 0                },  // CE
    {"CMP", iAbsLong  , 0                },  // CF

    {"BNE", iRelative , REFFLAG | CODEREF},  // D0
    {"CMP", iIndirectY, 0                },  // D1
    {"CMP", iDirIndir , 0                },  // D2
    {"CMP", iStackIndY, 0                },  // D3
    {"PEI", iZeroPage , 0                },  // D4 (might be iDirIndir)
    {"CMP", iZeroPageX, 0                },  // D5
    {"DEC", iZeroPageX, 0                },  // D6
    {"CMP", iDirIndirY, 0                },  // D7
    {"CLD", iImplied  , 0                },  // D8
    {"CMP", iAbsoluteY, 0                },  // D9
    {"PHX", iImplied  , 0                },  // DA
    {"STP", iImplied  , 0                },  // DB
    {"JML", iIndirect , 0                },  // DC
    {"CMP", iAbsoluteX, 0                },  // DD
    {"DEC", iAbsoluteX, 0                },  // DE
    {"CMP", iAbsLongX , 0                },  // DF

    {"CPX", iImmediateI,0                },  // E0 // with X flag clear
    {"SBC", iIndirectX, 0                },  // E1
    {"SEP", iImmediate, 0                },  // E2
    {"SBC", iStackRel , 0                },  // E3
    {"CPX", iZeroPage , 0                },  // E4
    {"SBC", iZeroPage , 0                },  // E5
    {"INC", iZeroPage , 0                },  // E6
    {"SBC", iDirIndirL, 0                },  // E7
    {"INX", iImplied  , 0                },  // E8
    {"SBC", iImmediateA,0                },  // E9
    {"NOP", iImplied  , 0                },  // EA
    {"XBA", iImplied  , 0                },  // EB
    {"CPX", iAbsolute , 0                },  // EC
    {"SBC", iAbsolute , 0                },  // ED
    {"INC", iAbsolute , 0                },  // EE
    {"SBC", iAbsLong  , 0                },  // EF

    {"BEQ", iRelative , REFFLAG | CODEREF},  // F0
    {"SBC", iIndirectY, 0                },  // F1
    {"SBC", iDirIndir , 0                },  // F2
    {"SBC", iStackIndY, 0                },  // F3
    {"PEA", iAbsolute , 0                },  // F4
    {"SBC", iZeroPageX, 0                },  // F5
    {"INC", iZeroPageX, 0                },  // F6
    {"SBC", iDirIndirY, 0                },  // F7
    {"SED", iImplied  , 0                },  // F8
    {"SBC", iAbsoluteY, 0                },  // F9
    {"PLX", iImplied  , 0                },  // FA
    {"XCE", iImplied  , 0                },  // FB
    {"JSR", iAbsIndX  , 0                },  // FC
    {"SBC", iAbsoluteX, 0                },  // FD
    {"INC", iAbsoluteX, 0                },  // FE
    {"SBC", iAbsLongX , 0                }   // FF
};


char *Dis6502::RefStrZP(addr_t addr, char *s, int &lfref, addr_t &refaddr) const
{
#if 1
    return RefStr2(addr, s, lfref, refaddr);
#else
(void) lfref;   // unused
(void) refaddr; // unused
    return H2Str(addr, s);
#endif
}


int Dis6502::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    addr_t      ad;
    int         i;
    char        s[256];
    int         len;
    int         typ;
    const struct InstrRec *M6502_opcdTable;

    ad   = addr;
    i    = ReadByte(ad++);
    len  = 1;
    addr_t ra = 0; // reference address

    // select the opcode table for this subtype
    switch (_subtype) {
        default:         assert(false); FALLTHROUGH; // invalid subtype
        case CPU_6502:   M6502_opcdTable = opcdTable6502;   break;
        case CPU_6502U:  M6502_opcdTable = opcdTable6502U;  break;
        case CPU_65C02:
        case CPU_65SC02: M6502_opcdTable = opcdTable65C02;  break;
        case CPU_65C816: M6502_opcdTable = opcdTable65C816; break;
    }

    strcpy(opcode, M6502_opcdTable[i].op);
    lfref    = M6502_opcdTable[i].lfref;
    parms[0] = 0;
    refaddr  = 0;
    typ      = M6502_opcdTable[i].typ;

    // disable x7 and xF for 65SC02
    if (_subtype == CPU_65SC02 && ((i & 7) == 7)) {
        opcode[0] = 0;
        typ = iImplied;
    }

    // 65816: handle 8 vs 16 bit immediate by hint flags
    if (_subtype == CPU_65C816) {
        int hint = rom.get_hint(addr);
        switch (typ) {
            // if the hint flag isn't set, make it an 8-bit immediate
            case iImmediateA: // for A/C (.LONGA ON/OFF)
                if (!(hint & HINT_LONGA)) {
                    typ = iImmediate;
                }
                break;

            case iImmediateI: // for X/Y (.LONGI ON/OFF)
                if (!(hint & HINT_LONGI)) {
                    typ = iImmediate;
                }
                break;
        }
    }

    switch (typ) {
        case iImplied:
            /* nothing */;
            break;

        case iAccum:
            strcat(opcode, "A");
            break;

        case iImmediate:
            sprintf(parms, "#$%.2X", ReadByte(ad++));
            len++;
            break;

        case iImmediateA: // for A/C (.LONGA ON/OFF)
#if 0 // sometimes an address is used with ADC #xxxx etc.
            sprintf(parms, "#$%.4X", ReadWord(ad)); // don't make it a ref addr
            ad += 2;
            len += 2;
            break;
#endif
        case iImmediateI: // for X/Y (.LONGI ON/OFF)
            ra = ReadWord(ad);
            ad += 2;
            len += 2;
            *parms++ = '#';
            *parms = 0;
            RefStr(ra, parms, lfref, refaddr);
            break;

        case iRelative:
            i = ReadByte(ad++);
            if (i == 0xFF) {
                // offset FF is suspicious
                lfref |= RIPSTOP;
            }
            len++;
            if (i >= 128) {
                i = i - 256;
            }
            ra = (ad + i) & 0xFFFF;
            RefStr(ra, parms, lfref, refaddr);
            break;

        case iZeroPage:
            ra = ReadByte(ad++);
            len++;
            RefStrZP(ra, parms, lfref, refaddr);
            break;

        case iZeroPageX:
            ra = ReadByte(ad++);
            len++;
            RefStrZP(ra, s, lfref, refaddr);
            parms += strlen(parms);
            sprintf(parms, "%s,X", s);
            break;

        case iZeroPageY:
            ra = ReadByte(ad++);
            len++;
            RefStrZP(ra, s, lfref, refaddr);
            sprintf(parms, "%s,Y", s);
            break;

        case iAbsolute:
            ra = ReadWord(ad);
            ad += 2;
            len += 2;
            if (ra < 0x100) {
                // force 16-bit mode
                *parms++ = '>';
                *parms = 0;
            }
            RefStr(ra, parms, lfref, refaddr);
            break;

        case iAbsoluteX:
            ra = ReadWord(ad);
            ad += 2;
            len += 2;
            if (ra < 0x100) {
                // force 16-bit mode
                *parms++ = '>';
                *parms = 0;
            }
//            if (ra < 0x10000) {
//                // force 24-bit mode
//                *parms++ = '>';
//                *parms++ = '>';
//                *parms = 0;
//            }
            RefStr(ra, s, lfref, refaddr);
            sprintf(parms, "%s,X", s);
            break;

        case iAbsoluteY:
            ra = ReadWord(ad);
            ad += 2;
            len += 2;
            RefStr(ra, s, lfref, refaddr);
            sprintf(parms, "%s,Y", s);
            break;

        case iIndirect:
            ra = ReadWord(ad);
            ad += 2;
            len += 2;
            RefStr(ra, s, lfref, refaddr);
            sprintf(parms, "(%s)", s);
            break;

        case iIndirectX:
            ra = ReadByte(ad++);
            len++;
            RefStrZP(ra, s, lfref, refaddr);
            sprintf(parms, "(%s,X)", s);
            break;

        case iIndirectY:
            ra = ReadByte(ad++);
            len++;
            RefStrZP(ra, s, lfref, refaddr);
            sprintf(parms, "(%s),Y", s);
            break;

        case iDirIndir:
            ra = ReadByte(ad++);
            len++;
            RefStrZP(ra, s, lfref, refaddr);
            sprintf(parms, "(%s)", s);
            break;

        case iAbsIndX:
            ra = ReadWord(ad);
            ad += 2;
            len += 2;
            RefStr(ra, s, lfref, refaddr);
            sprintf(parms, "(%s,X)", s);
            break;

        case iAbsLong:   // low,high,bank
            ra = ReadWord(ad) + ReadByte(ad+2) * 65536;
            ad += 3;
            len += 3;
            if (ra < 0x10000) {
                // force 24-bit mode
                *parms++ = '>';
                *parms++ = '>';
                *parms = 0;
            }
            RefStr6(ra, s, lfref, refaddr);
            sprintf(parms, "%s", s);
            break;

        case iAbsLongX:  // al,X
            ra = ReadWord(ad) + ReadByte(ad+2) * 65536;
            ad += 3;
            len += 3;
            if (ra < 0x10000) {
                // force 24-bit mode
                *parms++ = '>';
                *parms++ = '>';
                *parms = 0;
            }
            RefStr6(ra, s, lfref, refaddr);
            sprintf(parms, "%s,X", s);
            break;

        case iDirIndirL: // [d]
            ra = ReadByte(ad++);
            len++;
            RefStrZP(ra, s, lfref, refaddr);
            sprintf(parms, "[%s]", s);
            break;

        case iDirIndirY: // [d],Y
            ra = ReadByte(ad++);
            len++;
            RefStrZP(ra, s, lfref, refaddr);
            sprintf(parms, "[%s],Y", s);
            break;

        case iStackRel:  // d,S
            sprintf(parms, "$%.2X,S", ReadByte(ad++));
            len++;
            break;

        case iStackIndY: // (d,S),Y
            sprintf(parms, "($%.2X,S),Y", ReadByte(ad++));
            len++;
            break;

        case iRelativeL:
            i = ReadWord(ad);
            ad += 2;
            len += 2;
            if (i >= 32767) {
                i = i - 65536;
            }
            ra = (ad + i) & 0xFFFFFF;
            RefStr6(ra, parms, lfref, refaddr);
            break;

        case iBlockMove: // MVN/MVP dstbnk,strbnk
            i = ReadByte(ad++);
            len += 2;
            sprintf(parms, "$%.2X,$%.2X", ReadByte(ad++),i);
            break;

        case iBBRBBS: // opcode, zp, ofs
            // iZeroPage
            ra = ReadByte(ad++);
            len++;
            RefStrZP(ra, parms, lfref, refaddr);

            strcat(parms + strlen(parms), ",");

            // iRelative
            i = ReadByte(ad++);
            if (i == 0xFF) {
                // offset FF is suspicious
                lfref |= RIPSTOP;
            }
            len++;
            if (i >= 128) {
                i = i - 256;
            }
            ra = (ad + i) & 0xFFFF;
            RefStr(ra, parms + strlen(parms), lfref, refaddr);

            break;
    }

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
                case 0x0000: // two BRK in a row
                    lfref |= RIPSTOP;
                    break;

                // implied branch-always combinations
                case 0x1890: // CLC:BCS
                case 0x38B0: // SEC:BCS
                case 0xB850: // CLV:BVC
                case 0xA000F0: // LDY #0:BEQ
                case 0xA200F0: // LDX #0:BEQ
                case 0xA900F0: // LDA #0:BEQ
                    // ignore if current address has a label
                    if (rom.test_attr(addr, ATTR_LMASK)) {
                        break;
                    }
                    // force end of trace
                    lfref |= LFFLAG;
                    break;
            }

            // other conditions
            switch (ops & 0xFF00FF) {
                case 0x100030: // BPL:BMI
                case 0x300010: // BMI:BPL
                case 0x500070: // BVC:BVS
                case 0x700050: // BVS:BVC
                case 0x9000B0: // BCC:BCS
                case 0xB00090: // BCS:BCC
                case 0xD000F0: // BNE:BEQ
                case 0xF000D0: // BEQ:BNE
                case 0xA000D0: // LDY #i:BNE
                case 0xA200D0: // LDX #i:BNE
                case 0xA900D0: // LDA #i:BNE
                    // ignore if current address has a label
                    if (rom.test_attr(addr, ATTR_LMASK)) {
                        break;
                    }
                    if (ops & 0xFF00) {
                        // loading a non-zero value
                        lfref |= LFFLAG;
                        break;
                    }
                    break;
            }
        }
    }

    // check for invalid instruction
    if (opcode[0]==0 || rom.AddrOutRange(addr)) {
        strcpy(opcode, "???");
        sprintf(parms, "$%.2X", ReadByte(addr));
        len     = 0;
    }

    return len;
}


