// dis8086.cpp

static const char versionName[] = "Intel 8086 disassembler";

#include "discpu.h"

class Dis8086 : public CPU {
public:
    Dis8086(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    const struct InstrRec *find_opcode(int op, int &len);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
};


enum SubType {
    CPU_8086,
    CPU_80186,
    CPU_80286
};


Dis8086 cpu_8086 ("8086",  CPU_8086, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DL");
//Dis8086 cpu_8086 ("80186",  CPU_80186, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DL");
//Dis8086 cpu_8086 ("80286",  CPU_80286, LITTLE_END, ADDR_16, '$', 'H', "DB", "DW", "DL");


Dis8086::Dis8086(const char *name, int subtype, int endian, int addrwid,
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


enum InstType {
    o_Invalid,          // end of opcode list
    o_None,             // no parameters
    o_imm8,             // 8-bit immediate
    o_imm16,            // 16-bit immediate that can be a reference
    o_num16,            // 16-bit immediate that can not be a reference
    o_rm8_reg8,         // mode, 8-bit register
    o_rm16_reg16,       // mode, 16-bit register
    o_reg8_rm8,         // 8-bit register, mode
    o_reg16_rm16,       // 16-bit register, mode
    o_rm8,              // 8-bit mode
    o_rm16,             // 16-bit mode
    o_rmi8,             // r/m8, 8-bit immediate
    o_rmi16,            // r/m16, 16-bit immediate
    o_rmi8_16,          // r/m16, 8-bit immediate sign extended
    o_rimm8,            // reg, 8-bit immediate
    o_rimm16,           // reg, 16-bit immediate
    o_rot8,             // 8-bit rotate with immediate count
    o_rot16,            // 16-bit rotate with immediate count
    o_rm16_seg,         // mode, segment register
    o_seg_rm16,         // segment register, mode
    o_Far,              // far JMP and CALL
    o_rel8,             // 8-bit relative jump
    o_ESC,		// 80x87 instruction, 6 bits + rm16
    o_ENTER,            // ENTER instruction
};


// =====================================================


struct InstrRec {
    uint16_t        cmpWith;// opcode after andWith mask
    uint16_t        andWith;// bits of opcode to test
    SubType         cpu;    // minimum CPU version for this opcode
    enum InstType   typ;    // opcode type
    const char      *op;    // mnemonic
    const char      *parms; // parms
    uint8_t         lfref;  // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;


// selectors for register field in second byte
enum {
    r0 = 0<<3,
    r1 = 1<<3,
    r2 = 2<<3,
    r3 = 3<<3,
    r4 = 4<<3,
    r5 = 5<<3,
    r6 = 6<<3,
    r7 = 7<<3,
};


static const struct InstrRec I8086_opcdTable[] =
{
    { 0x0000, 0xFF00, CPU_8086 , o_rm8_reg8,   "ADD" , "",    0 },
    { 0x0100, 0xFF00, CPU_8086 , o_rm16_reg16, "ADD" , "",    0 },
    { 0x0200, 0xFF00, CPU_8086 , o_reg8_rm8,   "ADD" , "",    0 },
    { 0x0300, 0xFF00, CPU_8086 , o_reg16_rm16, "ADD" , "",    0 },
    { 0x0400, 0xFF00, CPU_8086 , o_imm8,       "ADD" , "AL,", 0 },
    { 0x0500, 0xFF00, CPU_8086 , o_imm16,      "ADD" , "AX,", 0 },
    { 0x0600, 0xFF00, CPU_8086 , o_None,       "PUSH", "ES",  0 },
    { 0x0700, 0xFF00, CPU_8086 , o_None,       "POP" , "ES",  0 },

    { 0x0800, 0xFF00, CPU_8086 , o_rm8_reg8,   "OR"  , "",    0 },
    { 0x0900, 0xFF00, CPU_8086 , o_rm16_reg16, "OR"  , "",    0 },
    { 0x0A00, 0xFF00, CPU_8086 , o_reg8_rm8,   "OR"  , "",    0 },
    { 0x0B00, 0xFF00, CPU_8086 , o_reg16_rm16, "OR"  , "",    0 },
    { 0x0C00, 0xFF00, CPU_8086 , o_imm8,       "OR"  , "AL,", 0 },
    { 0x0D00, 0xFF00, CPU_8086 , o_imm16,      "OR"  , "AX,", 0 },
    { 0x0E00, 0xFF00, CPU_8086 , o_None,       "PUSH", "CS",  0 },
// <-- put 0Fxx here right before POP CS
    { 0x0F00, 0xFF00, CPU_8086 , o_None,       "POP" , "CS",  0 }, // (later used as prefix)

    { 0x1000, 0xFF00, CPU_8086 , o_rm8_reg8,   "ADC" , "",    0 },
    { 0x1100, 0xFF00, CPU_8086 , o_rm16_reg16, "ADC" , "",    0 },
    { 0x1200, 0xFF00, CPU_8086 , o_reg8_rm8,   "ADC" , "",    0 },
    { 0x1300, 0xFF00, CPU_8086 , o_reg16_rm16, "ADC" , "",    0 },
    { 0x1400, 0xFF00, CPU_8086 , o_imm8,       "ADC" , "AL,", 0 },
    { 0x1500, 0xFF00, CPU_8086 , o_imm16,      "ADC" , "AX,", 0 },
    { 0x1600, 0xFF00, CPU_8086 , o_None,       "PUSH", "SS",  0 },
    { 0x1700, 0xFF00, CPU_8086 , o_None,       "POP" , "SS",  0 },

    { 0x1800, 0xFF00, CPU_8086 , o_rm8_reg8,   "SBC" , "",    0 },
    { 0x1900, 0xFF00, CPU_8086 , o_rm16_reg16, "SBC" , "",    0 },
    { 0x1A00, 0xFF00, CPU_8086 , o_reg8_rm8,   "SBC" , "",    0 },
    { 0x1B00, 0xFF00, CPU_8086 , o_reg16_rm16, "SBC" , "",    0 },
    { 0x1C00, 0xFF00, CPU_8086 , o_imm8,       "SBC" , "AL,", 0 },
    { 0x1D00, 0xFF00, CPU_8086 , o_imm16,      "SBC" , "AX,", 0 },
    { 0x1E00, 0xFF00, CPU_8086 , o_None,       "PUSH", "DS",  0 },
    { 0x1F00, 0xFF00, CPU_8086 , o_None,       "POP" , "DS",  0 },

    { 0x2000, 0xFF00, CPU_8086 , o_rm8_reg8,   "AND" , "",    0 },
    { 0x2100, 0xFF00, CPU_8086 , o_rm16_reg16, "AND" , "",    0 },
    { 0x2200, 0xFF00, CPU_8086 , o_reg8_rm8,   "AND" , "",    0 },
    { 0x2300, 0xFF00, CPU_8086 , o_reg16_rm16, "AND" , "",    0 },
    { 0x2400, 0xFF00, CPU_8086 , o_imm8,       "AND" , "AL,", 0 },
    { 0x2500, 0xFF00, CPU_8086 , o_imm16,      "AND" , "AX,", 0 },
    { 0x2600, 0xFF00, CPU_8086 , o_None,       "ES:" , "",    0 }, // prefix, branch hint
    { 0x2700, 0xFF00, CPU_8086 , o_None,       "DAA" , "",    0 },

    { 0x2800, 0xFF00, CPU_8086 , o_rm8_reg8,   "SUB" , "",    0 },
    { 0x2900, 0xFF00, CPU_8086 , o_rm16_reg16, "SUB" , "",    0 },
    { 0x2A00, 0xFF00, CPU_8086 , o_reg8_rm8,   "SUB" , "",    0 },
    { 0x2B00, 0xFF00, CPU_8086 , o_reg16_rm16, "SUB" , "",    0 },
    { 0x2C00, 0xFF00, CPU_8086 , o_imm8,       "SUB" , "AL,", 0 },
    { 0x2D00, 0xFF00, CPU_8086 , o_imm16,      "SUB" , "AX,", 0 },
    { 0x2E00, 0xFF00, CPU_8086 , o_None,       "CS:" , "",    0 }, // prefix, branch hint
    { 0x2F00, 0xFF00, CPU_8086 , o_None,       "DAS" , "",    0 },

    { 0x3000, 0xFF00, CPU_8086 , o_rm8_reg8,   "XOR" , "",    0 },
    { 0x3100, 0xFF00, CPU_8086 , o_rm16_reg16, "XOR" , "",    0 },
    { 0x3200, 0xFF00, CPU_8086 , o_reg8_rm8,   "XOR" , "",    0 },
    { 0x3300, 0xFF00, CPU_8086 , o_reg16_rm16, "XOR" , "",    0 },
    { 0x3400, 0xFF00, CPU_8086 , o_imm8,       "XOR" , "AL,", 0 },
    { 0x3500, 0xFF00, CPU_8086 , o_imm16,      "XOR" , "AX,", 0 },
    { 0x3600, 0xFF00, CPU_8086 , o_None,       "SS:" , "",    0 }, // prefix, branch hint
    { 0x3700, 0xFF00, CPU_8086 , o_None,       "AAA" , "",    0 },

    { 0x3800, 0xFF00, CPU_8086 , o_rm8_reg8,   "CMP" , "",    0 },
    { 0x3900, 0xFF00, CPU_8086 , o_rm16_reg16, "CMP" , "",    0 },
    { 0x3A00, 0xFF00, CPU_8086 , o_reg8_rm8,   "CMP" , "",    0 },
    { 0x3B00, 0xFF00, CPU_8086 , o_reg16_rm16, "CMP" , "",    0 },
    { 0x3C00, 0xFF00, CPU_8086 , o_imm8,       "CMP" , "AL,", 0 },
    { 0x3D00, 0xFF00, CPU_8086 , o_imm16,      "CMP" , "AX,", 0 },
    { 0x3E00, 0xFF00, CPU_8086 , o_None,       "DS:" , "",    0 }, // prefix, branch hint
    { 0x3F00, 0xFF00, CPU_8086 , o_None,       "AAS" , "",    0 },

    { 0x4000, 0xFF00, CPU_8086 , o_None, "INC" , "AX", 0 },
    { 0x4100, 0xFF00, CPU_8086 , o_None, "INC" , "CX", 0 },
    { 0x4200, 0xFF00, CPU_8086 , o_None, "INC" , "DX", 0 },
    { 0x4300, 0xFF00, CPU_8086 , o_None, "INC" , "BX", 0 },
    { 0x4400, 0xFF00, CPU_8086 , o_None, "INC" , "SP", 0 },
    { 0x4500, 0xFF00, CPU_8086 , o_None, "INC" , "BP", 0 },
    { 0x4600, 0xFF00, CPU_8086 , o_None, "INC" , "SI", 0 },
    { 0x4700, 0xFF00, CPU_8086 , o_None, "INC" , "DI", 0 },

    { 0x4800, 0xFF00, CPU_8086 , o_None, "DEC" , "AX", 0 },
    { 0x4900, 0xFF00, CPU_8086 , o_None, "DEC" , "CX", 0 },
    { 0x4A00, 0xFF00, CPU_8086 , o_None, "DEC" , "DX", 0 },
    { 0x4B00, 0xFF00, CPU_8086 , o_None, "DEC" , "BX", 0 },
    { 0x4C00, 0xFF00, CPU_8086 , o_None, "DEC" , "SP", 0 },
    { 0x4D00, 0xFF00, CPU_8086 , o_None, "DEC" , "BP", 0 },
    { 0x4E00, 0xFF00, CPU_8086 , o_None, "DEC" , "SI", 0 },
    { 0x4F00, 0xFF00, CPU_8086 , o_None, "DEC" , "DI", 0 },

    { 0x5000, 0xFF00, CPU_8086 , o_None, "PUSH", "AX", 0 },
    { 0x5100, 0xFF00, CPU_8086 , o_None, "PUSH", "CX", 0 },
    { 0x5200, 0xFF00, CPU_8086 , o_None, "PUSH", "DX", 0 },
    { 0x5300, 0xFF00, CPU_8086 , o_None, "PUSH", "BX", 0 },
    { 0x5400, 0xFF00, CPU_8086 , o_None, "PUSH", "SP", 0 },
    { 0x5500, 0xFF00, CPU_8086 , o_None, "PUSH", "BP", 0 },
    { 0x5600, 0xFF00, CPU_8086 , o_None, "PUSH", "SI", 0 },
    { 0x5700, 0xFF00, CPU_8086 , o_None, "PUSH", "DI", 0 },

    { 0x5800, 0xFF00, CPU_8086 , o_None, "POP" , "AX", 0 },
    { 0x5900, 0xFF00, CPU_8086 , o_None, "POP" , "CX", 0 },
    { 0x5A00, 0xFF00, CPU_8086 , o_None, "POP" , "DX", 0 },
    { 0x5B00, 0xFF00, CPU_8086 , o_None, "POP" , "BX", 0 },
    { 0x5C00, 0xFF00, CPU_8086 , o_None, "POP" , "SP", 0 },
    { 0x5D00, 0xFF00, CPU_8086 , o_None, "POP" , "BP", 0 },
    { 0x5E00, 0xFF00, CPU_8086 , o_None, "POP" , "SI", 0 },
    { 0x5F00, 0xFF00, CPU_8086 , o_None, "POP" , "DI", 0 },

    { 0x6000, 0xFF00, CPU_80186, o_None, "PUSHA", "", 0 },
    { 0x6100, 0xFF00, CPU_80186, o_None, "POPA" , "", 0 },
// 62 BOUND (80186)
// 63 ARPL  (80286)
// 64 FS: and branch hint
// 65 GS: and branch hint
// 66 operand size and precision size override prefix
// 67 address size override prefix

// 68 PUSH imm W (80186)
// 69 IMUL imm W (80186)
// 6A PUSH imm B (80186)
// 6B IMUL imm B (80186)
    { 0x6C00, 0xFF00, CPU_80186, o_None,  "INSB" , "", 0 },
    { 0x6D00, 0xFF00, CPU_80186, o_None,  "INSW" , "", 0 },
    { 0x6E00, 0xFF00, CPU_80186, o_None,  "OUTSB", "", 0 },
    { 0x6F00, 0xFF00, CPU_80186, o_None,  "OUTSW", "", 0 },

    { 0x7000, 0xFF00, CPU_8086 , o_rel8, "JO" , "", REFFLAG | CODEREF },
    { 0x7100, 0xFF00, CPU_8086 , o_rel8, "JNO", "", REFFLAG | CODEREF },
    { 0x7200, 0xFF00, CPU_8086 , o_rel8, "JC" , "", REFFLAG | CODEREF }, // JB/JC
    { 0x7300, 0xFF00, CPU_8086 , o_rel8, "JNC", "", REFFLAG | CODEREF }, // JAE/JNC
    { 0x7400, 0xFF00, CPU_8086 , o_rel8, "JE" , "", REFFLAG | CODEREF }, // JE/JZ
    { 0x7500, 0xFF00, CPU_8086 , o_rel8, "JNE", "", REFFLAG | CODEREF }, // JNE/JNZ
    { 0x7600, 0xFF00, CPU_8086 , o_rel8, "JBE", "", REFFLAG | CODEREF },
    { 0x7700, 0xFF00, CPU_8086 , o_rel8, "JA" , "", REFFLAG | CODEREF },

    { 0x7800, 0xFF00, CPU_8086 , o_rel8, "JS" , "", REFFLAG | CODEREF },
    { 0x7900, 0xFF00, CPU_8086 , o_rel8, "JNS", "", REFFLAG | CODEREF },
    { 0x7A00, 0xFF00, CPU_8086 , o_rel8, "JPE", "", REFFLAG | CODEREF },
    { 0x7B00, 0xFF00, CPU_8086 , o_rel8, "JPO", "", REFFLAG | CODEREF },
    { 0x7C00, 0xFF00, CPU_8086 , o_rel8, "JL" , "", REFFLAG | CODEREF },
    { 0x7D00, 0xFF00, CPU_8086 , o_rel8, "JGE", "", REFFLAG | CODEREF },
    { 0x7E00, 0xFF00, CPU_8086 , o_rel8, "JLE", "", REFFLAG | CODEREF },
    { 0x7F00, 0xFF00, CPU_8086 , o_rel8, "JG" , "", REFFLAG | CODEREF },

// 80 o_rmi8  r/m8, 8-bit immediate
    { 0x8000+r0, 0xFF38, CPU_8086 , o_rmi8,   "ADD", "", 0 },
    { 0x8000+r1, 0xFF38, CPU_8086 , o_rmi8,   "OR" , "", 0 },
    { 0x8000+r2, 0xFF38, CPU_8086 , o_rmi8,   "ADC", "", 0 },
    { 0x8000+r3, 0xFF38, CPU_8086 , o_rmi8,   "SBB", "", 0 },
    { 0x8000+r4, 0xFF38, CPU_8086 , o_rmi8,   "AND", "", 0 },
    { 0x8000+r5, 0xFF38, CPU_8086 , o_rmi8,   "SUB", "", 0 },
    { 0x8000+r6, 0xFF38, CPU_8086 , o_rmi8,   "XOR", "", 0 },
    { 0x8000+r7, 0xFF38, CPU_8086 , o_rmi8,   "CMP", "", 0 },

// 81 o_rmi16  r/m16, 16-bit immediate
    { 0x8100+r0, 0xFF38, CPU_8086 , o_rmi16,  "ADD", "", 0 },
    { 0x8100+r1, 0xFF38, CPU_8086 , o_rmi16,  "OR" , "", 0 },
    { 0x8100+r2, 0xFF38, CPU_8086 , o_rmi16,  "ADC", "", 0 },
    { 0x8100+r3, 0xFF38, CPU_8086 , o_rmi16,  "SBB", "", 0 },
    { 0x8100+r4, 0xFF38, CPU_8086 , o_rmi16,  "AND", "", 0 },
    { 0x8100+r5, 0xFF38, CPU_8086 , o_rmi16,  "SUB", "", 0 },
    { 0x8100+r6, 0xFF38, CPU_8086 , o_rmi16,  "XOR", "", 0 },
    { 0x8100+r7, 0xFF38, CPU_8086 , o_rmi16,  "CMP", "", 0 },

// 82 duplicate of 80 in 16/32-bit mode?
    { 0x8200+r0, 0xFF38, CPU_80186, o_rmi8,   "*ADD", "", 0 },
    { 0x8200+r1, 0xFF38, CPU_80186, o_rmi8,   "*OR" , "", 0 },
    { 0x8200+r2, 0xFF38, CPU_80186, o_rmi8,   "*ADC", "", 0 },
    { 0x8200+r3, 0xFF38, CPU_80186, o_rmi8,   "*SBB", "", 0 },
    { 0x8200+r4, 0xFF38, CPU_80186, o_rmi8,   "*AND", "", 0 },
    { 0x8200+r5, 0xFF38, CPU_80186, o_rmi8,   "*SUB", "", 0 },
    { 0x8200+r6, 0xFF38, CPU_80186, o_rmi8,   "*XOR", "", 0 },
    { 0x8200+r7, 0xFF38, CPU_80186, o_rmi8,   "*CMP", "", 0 },

// 83 o_rmi8_16  r/m16, 8-bit immediate sign extended
    { 0x8300+r0, 0xFF38, CPU_8086 , o_rmi8_16, "ADD", "", 0 },
    { 0x8300+r1, 0xFF38, CPU_8086 , o_rmi8_16, "OR" , "", 0 },
    { 0x8300+r2, 0xFF38, CPU_8086 , o_rmi8_16, "ADC", "", 0 },
    { 0x8300+r3, 0xFF38, CPU_8086 , o_rmi8_16, "SBB", "", 0 },
    { 0x8300+r4, 0xFF38, CPU_8086 , o_rmi8_16, "AND", "", 0 },
    { 0x8300+r5, 0xFF38, CPU_8086 , o_rmi8_16, "SUB", "", 0 },
    { 0x8300+r6, 0xFF38, CPU_8086 , o_rmi8_16, "XOR", "", 0 },
    { 0x8300+r7, 0xFF38, CPU_8086 , o_rmi8_16, "CMP", "", 0 },

    { 0x8400, 0xFF00, CPU_8086 , o_rm8,        "TEST", "", 0 },
    { 0x8500, 0xFF00, CPU_8086 , o_rm16,       "TEST", "", 0 },
    { 0x8600, 0xFF00, CPU_8086 , o_reg8_rm8,   "XCHG", "", 0 },
    { 0x8700, 0xFF00, CPU_8086 , o_reg16_rm16, "XCHG", "", 0 },

    { 0x8800, 0xFF00, CPU_8086 , o_rm8_reg8,   "MOV", "", 0 },
    { 0x8900, 0xFF00, CPU_8086 , o_rm16_reg16, "MOV", "", 0 },
    { 0x8A00, 0xFF00, CPU_8086 , o_reg8_rm8,   "MOV", "", 0 },
    { 0x8B00, 0xFF00, CPU_8086 , o_reg16_rm16, "MOV", "", 0 },
    { 0x8C00, 0xFF00, CPU_8086 , o_rm16_seg,   "MOV", "", 0 },
    { 0x8D00, 0xFF00, CPU_8086 , o_reg16_rm16, "LEA", "", 0 },
    { 0x8E00, 0xFF00, CPU_8086 , o_seg_rm16,   "MOV", "", 0 },
    { 0x8F00, 0xFF00, CPU_8086 , o_seg_rm16,   "POP", "", 0 }, // 8F/0

    { 0x9000, 0xFF00, CPU_8086 , o_None, "NOP" , ""     , 0 }, // (XCHG AX,AX)
    { 0x9100, 0xFF00, CPU_8086 , o_None, "XCHG", "AX,CX", 0 },
    { 0x9200, 0xFF00, CPU_8086 , o_None, "XCHG", "AX,DX", 0 },
    { 0x9300, 0xFF00, CPU_8086 , o_None, "XCHG", "AX,BX", 0 },
    { 0x9400, 0xFF00, CPU_8086 , o_None, "XCHG", "AX,SP", 0 },
    { 0x9500, 0xFF00, CPU_8086 , o_None, "XCHG", "AX,BP", 0 },
    { 0x9600, 0xFF00, CPU_8086 , o_None, "XCHG", "AX,SI", 0 },
    { 0x9700, 0xFF00, CPU_8086 , o_None, "XCHG", "AX,DI", 0 },

    { 0x9800, 0xFF00, CPU_8086 , o_None, "CBW"  , "",     0 },
    { 0x9900, 0xFF00, CPU_8086 , o_None, "CWD"  , "",     0 },
    { 0x9A00, 0xFF00, CPU_8086 , o_Far,  "CALL" , "FAR ", 0 }, // far
    { 0x9B00, 0xFF00, CPU_8086 , o_None, "WAIT" , "",     0 }, // WAIT/FWAIT
    { 0x9C00, 0xFF00, CPU_8086 , o_None, "PUSHF", "",     0 },
    { 0x9D00, 0xFF00, CPU_8086 , o_None, "POPF" , "",     0 },
    { 0x9E00, 0xFF00, CPU_8086 , o_None, "SAHF" , "",     0 },
    { 0x9F00, 0xFF00, CPU_8086 , o_None, "LAHF" , "",     0 },

    { 0xA000, 0xFF00, CPU_8086 , o_imm16, "MOV",   "AL,", 0 },
    { 0xA100, 0xFF00, CPU_8086 , o_imm16, "MOV",   "AX,", 0 },
    { 0xA200, 0xFF00, CPU_8086 , o_imm16, "MOV",   ",AL", 0 },
    { 0xA300, 0xFF00, CPU_8086 , o_imm16, "MOV",   ",AX", 0 },
    { 0xA400, 0xFF00, CPU_8086 , o_None,  "MOVSB", "",    0 },
    { 0xA500, 0xFF00, CPU_8086 , o_None,  "MOVSW", "",    0 },
    { 0xA600, 0xFF00, CPU_8086 , o_None,  "CMPSB", "",    0 },
    { 0xA700, 0xFF00, CPU_8086 , o_None,  "CMPSW", "",    0 },

    { 0xA800, 0xFF00, CPU_8086 , o_imm8,  "TEST" , "",   0 },
    { 0xA900, 0xFF00, CPU_8086 , o_imm16, "TEST" , "",   0 },
    { 0xAA00, 0xFF00, CPU_8086 , o_None,  "STOSB", "",   0 },
    { 0xAB00, 0xFF00, CPU_8086 , o_None,  "STOSW", "",   0 },
    { 0xAC00, 0xFF00, CPU_8086 , o_None,  "LODSB", "",   0 },
    { 0xAD00, 0xFF00, CPU_8086 , o_None,  "LODSW", "",   0 },
    { 0xAE00, 0xFF00, CPU_8086 , o_None,  "SCASB", "",   0 },
    { 0xAF00, 0xFF00, CPU_8086 , o_None,  "SCASW", "",   0 },

    { 0xB000, 0xFF00, CPU_8086 , o_imm8,  "MOV" , "AL,", 0 },
    { 0xB100, 0xFF00, CPU_8086 , o_imm8,  "MOV" , "CL,", 0 },
    { 0xB200, 0xFF00, CPU_8086 , o_imm8,  "MOV" , "DL,", 0 },
    { 0xB300, 0xFF00, CPU_8086 , o_imm8,  "MOV" , "BL,", 0 },
    { 0xB400, 0xFF00, CPU_8086 , o_imm8,  "MOV" , "AH,", 0 },
    { 0xB500, 0xFF00, CPU_8086 , o_imm8,  "MOV" , "CH,", 0 },
    { 0xB600, 0xFF00, CPU_8086 , o_imm8,  "MOV" , "DH,", 0 },
    { 0xB700, 0xFF00, CPU_8086 , o_imm8,  "MOV" , "BH,", 0 },

    { 0xB800, 0xFF00, CPU_8086 , o_imm16, "MOV" , "AX,", 0 },
    { 0xB900, 0xFF00, CPU_8086 , o_imm16, "MOV" , "CX,", 0 },
    { 0xBA00, 0xFF00, CPU_8086 , o_imm16, "MOV" , "DX,", 0 },
    { 0xBB00, 0xFF00, CPU_8086 , o_imm16, "MOV" , "BX,", 0 },
    { 0xBC00, 0xFF00, CPU_8086 , o_imm16, "MOV" , "SP,", 0 },
    { 0xBD00, 0xFF00, CPU_8086 , o_imm16, "MOV" , "BP,", 0 },
    { 0xBE00, 0xFF00, CPU_8086 , o_imm16, "MOV" , "SI,", 0 },
    { 0xBF00, 0xFF00, CPU_8086 , o_imm16, "MOV" , "DI,", 0 },

    { 0xC000+r0, 0xFF38, CPU_80186, o_rot8,  "ROL", "", 0 },
    { 0xC000+r1, 0xFF38, CPU_80186, o_rot8,  "ROR", "", 0 },
    { 0xC000+r2, 0xFF38, CPU_80186, o_rot8,  "RCL", "", 0 },
    { 0xC000+r3, 0xFF38, CPU_80186, o_rot8,  "RCR", "", 0 },
    { 0xC000+r4, 0xFF38, CPU_80186, o_rot8,  "SHL", "", 0 }, // SAL/SHL
    { 0xC000+r5, 0xFF38, CPU_80186, o_rot8,  "SHR", "", 0 },
    { 0xC000+r6, 0xFF38, CPU_80186, o_rot8,  "???", "", 0 },
    { 0xC000+r7, 0xFF38, CPU_80186, o_rot8,  "SAR", "", 0 },

    { 0xC100+r0, 0xFF38, CPU_80186, o_rot16, "ROL", "", 0 },
    { 0xC100+r1, 0xFF38, CPU_80186, o_rot16, "ROR", "", 0 },
    { 0xC100+r2, 0xFF38, CPU_80186, o_rot16, "RCL", "", 0 },
    { 0xC100+r3, 0xFF38, CPU_80186, o_rot16, "RCR", "", 0 },
    { 0xC100+r4, 0xFF38, CPU_80186, o_rot16, "SHL", "", 0 }, // SAL/SHL
    { 0xC100+r5, 0xFF38, CPU_80186, o_rot16, "SHR", "", 0 },
    { 0xC100+r6, 0xFF38, CPU_80186, o_rot16, "???", "", 0 },
    { 0xC100+r7, 0xFF38, CPU_80186, o_rot16, "SAR", "", 0 },

    { 0xC200, 0xFF00, CPU_8086 , o_num16,      "RETN", "", LFFLAG },
    { 0xC300, 0xFF00, CPU_8086 , o_None,       "RETN", "", LFFLAG },
    { 0xC400, 0xFF00, CPU_8086 , o_reg16_rm16, "LES" , "", 0 },
    { 0xC500, 0xFF00, CPU_8086 , o_reg16_rm16, "LDS" , "", 0 },
    { 0xC600+r0, 0xFF38, CPU_8086 , o_rmi8,    "MOV" , "", 0 }, // C6/0
    { 0xC700+r0, 0xFF38, CPU_8086 , o_rmi16,   "MOV" , "", 0 }, // C7/0
// C8 ENTER (80186)
    { 0xC800, 0xFF00, CPU_80186, o_ENTER, "ENTER", "", 0 },
// C9 LEAVE (80186)
    { 0xC900, 0xFF00, CPU_80186, o_None,  "LEAVE", "", 0 },
    { 0xCA00, 0xFF00, CPU_8086 , o_num16, "RETF",  "", LFFLAG },
    { 0xCB00, 0xFF00, CPU_8086 , o_None,  "RETF",  "", LFFLAG },
    { 0xCC00, 0xFF00, CPU_8086 , o_None,  "INT3",  "", 0 },
    { 0xCD00, 0xFF00, CPU_8086 , o_imm8,  "INT" ,  "", 0 },
    { 0xCE00, 0xFF00, CPU_8086 , o_None,  "INTO",  "", 0 },
    { 0xCF00, 0xFF00, CPU_8086 , o_None,  "IRET",  "", LFFLAG },

    { 0xD000+r0, 0xFF38, CPU_8086 , o_rm8,  "ROL", ",1",  0 },
    { 0xD000+r1, 0xFF38, CPU_8086 , o_rm8,  "ROR", ",1",  0 },
    { 0xD000+r2, 0xFF38, CPU_8086 , o_rm8,  "RCL", ",1",  0 },
    { 0xD000+r3, 0xFF38, CPU_8086 , o_rm8,  "RCR", ",1",  0 },
    { 0xD000+r4, 0xFF38, CPU_8086 , o_rm8,  "SHL", ",1",  0 }, // SHL/SAL
    { 0xD000+r5, 0xFF38, CPU_8086 , o_rm8,  "SHR", ",1",  0 },
//  { 0xD000+r6, 0xFF38, CPU_8086 , o_rm8,  "???", ",1",  0 },
    { 0xD000+r7, 0xFF38, CPU_8086 , o_rm8,  "SAR", ",1",  0 },

    { 0xD100+r0, 0xFF38, CPU_8086 , o_rm16, "ROL", ",1",  0 },
    { 0xD100+r1, 0xFF38, CPU_8086 , o_rm16, "ROR", ",1",  0 },
    { 0xD100+r2, 0xFF38, CPU_8086 , o_rm16, "RCL", ",1",  0 },
    { 0xD100+r3, 0xFF38, CPU_8086 , o_rm16, "RCR", ",1",  0 },
    { 0xD100+r4, 0xFF38, CPU_8086 , o_rm16, "SHL", ",1",  0 }, // SHL/SAL
    { 0xD100+r5, 0xFF38, CPU_8086 , o_rm16, "SHR", ",1",  0 },
//  { 0xD100+r6, 0xFF38, CPU_8086 , o_rm16, "???", ",1",  0 },
    { 0xD100+r7, 0xFF38, CPU_8086 , o_rm16, "SAR", ",1",  0 },

    { 0xD200+r0, 0xFF38, CPU_8086 , o_rm8,  "ROL", ",CL", 0 },
    { 0xD200+r1, 0xFF38, CPU_8086 , o_rm8,  "ROR", ",CL", 0 },
    { 0xD200+r2, 0xFF38, CPU_8086 , o_rm8,  "RCL", ",CL", 0 },
    { 0xD200+r3, 0xFF38, CPU_8086 , o_rm8,  "RCR", ",CL", 0 },
    { 0xD200+r4, 0xFF38, CPU_8086 , o_rm8,  "SHL", ",CL", 0 }, // SHL/SAL
    { 0xD200+r5, 0xFF38, CPU_8086 , o_rm8,  "SHR", ",CL", 0 },
//  { 0xD200+r6, 0xFF38, CPU_8086 , o_rm8,  "???", ",CL", 0 },
    { 0xD200+r7, 0xFF38, CPU_8086 , o_rm8,  "SAR", ",CL", 0 },

    { 0xD300+r0, 0xFF38, CPU_8086 , o_rm16, "ROL", ",CL", 0 },
    { 0xD300+r1, 0xFF38, CPU_8086 , o_rm16, "ROR", ",CL", 0 },
    { 0xD300+r2, 0xFF38, CPU_8086 , o_rm16, "RCL", ",CL", 0 },
    { 0xD300+r3, 0xFF38, CPU_8086 , o_rm16, "RCR", ",CL", 0 },
    { 0xD300+r4, 0xFF38, CPU_8086 , o_rm16, "SHL", ",CL", 0 }, // SHL/SAL
    { 0xD300+r5, 0xFF38, CPU_8086 , o_rm16, "SHR", ",CL", 0 },
//  { 0xD300+r6, 0xFF38, CPU_8086 , o_rm16, "???", ",CL", 0 },
    { 0xD300+r7, 0xFF38, CPU_8086 , o_rm16, "SAR", ",CL", 0 },

    { 0xD400, 0xFF00, CPU_8086 , o_None, "AAM" , "", 0 },
    { 0xD500, 0xFF00, CPU_8086 , o_None, "AAD" , "", 0 },
    { 0xD600, 0xFF00, CPU_8086 , o_None, "SALC", "", 0 },
    { 0xD700, 0xFF00, CPU_8086 , o_None, "XLAT", "", 0 },

    { 0xD800, 0xF800, CPU_8086 , o_ESC,  "ESC" , "", 0 }, // D8-DF ESC (floating point)

    { 0xE000, 0xFF00, CPU_8086 , o_rel8, "LOOPNZ", ""  , REFFLAG | CODEREF }, // LOOPNZ/LOPNE
    { 0xE100, 0xFF00, CPU_8086 , o_rel8, "LOOPZ", ""   , REFFLAG | CODEREF }, // LOOPZ/LOOPE
    { 0xE200, 0xFF00, CPU_8086 , o_rel8, "LOOP" , ""   , REFFLAG | CODEREF },
    { 0xE300, 0xFF00, CPU_8086 , o_rel8, "JCXZ" , ""   , REFFLAG | CODEREF },
    { 0xE400, 0xFF00, CPU_8086 , o_imm8, "IN"   , "AL,", 0 },
    { 0xE500, 0xFF00, CPU_8086 , o_imm8, "IN"   , "AX,", 0 },
    { 0xE600, 0xFF00, CPU_8086 , o_imm8, "OUT"  , ",AL", 0 },
    { 0xE700, 0xFF00, CPU_8086 , o_imm8, "OUT"  , ",AX", 0 },

    { 0xE800, 0xFF00, CPU_8086 , o_imm16, "CALL", "",        REFFLAG | CODEREF }, // near
    { 0xE900, 0xFF00, CPU_8086 , o_imm16, "JMP" , "",        LFFLAG | REFFLAG | CODEREF }, // near
    { 0xEA00, 0xFF00, CPU_8086 , o_Far,   "JMP" , "FAR "  ,  LFFLAG }, // far
    { 0xEB00, 0xFF00, CPU_8086 , o_rel8,  "JMP" , "SHORT ",  LFFLAG | REFFLAG | CODEREF }, // short
    { 0xEC00, 0xFF00, CPU_8086 , o_None,  "IN"  , "AL,[DX]", 0 },
    { 0xED00, 0xFF00, CPU_8086 , o_None,  "IN"  , "[DX],AL", 0 },
    { 0xEE00, 0xFF00, CPU_8086 , o_None,  "OUT" , "[DX],AL", 0 },
    { 0xEF00, 0xFF00, CPU_8086 , o_None,  "OUT" , "[DX],AX", 0 },

    { 0xF000, 0xFF00, CPU_8086 , o_None, "LOCK" , "", 0 }, // prefix
//  { 0xF100, 0xFF00, CPU_80386, o_None, "ICEBP", "", 0 }, // F1 ICEBP/INT1 (80386)
    { 0xF200, 0xFF00, CPU_8086 , o_None, "REPNE", "", 0 }, // prefix
    { 0xF300, 0xFF00, CPU_8086 , o_None, "REP"  , "", 0 }, // prefix
    { 0xF400, 0xFF00, CPU_8086 , o_None, "HLT"  , "", 0 },
    { 0xF500, 0xFF00, CPU_8086 , o_None, "CMC"  , "", 0 },

    { 0xF600+r0, 0xFF38, CPU_8086 , o_rimm8,  "TEST", "", 0 },
//  { 0xF600+r1, 0xFF38, CPU_8086 , o_rm8,    "*TEST","", 0 }, // duplicate of TEST
    { 0xF600+r2, 0xFF38, CPU_8086 , o_rm8,    "NOT" , "", 0 },
    { 0xF600+r3, 0xFF38, CPU_8086 , o_rm8,    "NEG" , "", 0 },
    { 0xF600+r4, 0xFF38, CPU_8086 , o_rm8,    "MUL" , "", 0 },
    { 0xF600+r5, 0xFF38, CPU_8086 , o_rm8,    "IMUL", "", 0 },
    { 0xF600+r6, 0xFF38, CPU_8086 , o_rm8,    "DIV" , "", 0 },
    { 0xF600+r7, 0xFF38, CPU_8086 , o_rm8,    "IDIV", "", 0 },

    { 0xF700+r0, 0xFF38, CPU_8086 , o_rimm16, "TEST", "", 0 },
//  { 0xF700+r1, 0xFF38, CPU_8086 , o_rm16,   "*TEST","", 0 }, // duplicate of TEST
    { 0xF700+r2, 0xFF38, CPU_8086 , o_rm16,   "NOT" , "", 0 },
    { 0xF700+r3, 0xFF38, CPU_8086 , o_rm16,   "NEG" , "", 0 },
    { 0xF700+r4, 0xFF38, CPU_8086 , o_rm16,   "MUL" , "", 0 },
    { 0xF700+r5, 0xFF38, CPU_8086 , o_rm16,   "IMUL", "", 0 },
    { 0xF700+r6, 0xFF38, CPU_8086 , o_rm16,   "DIV" , "", 0 },
    { 0xF700+r7, 0xFF38, CPU_8086 , o_rm16,   "IDIV", "", 0 },

    { 0xF800, 0xFF00, CPU_8086 , o_None, "CLC" , "", 0 },
    { 0xF900, 0xFF00, CPU_8086 , o_None, "STC" , "", 0 },
    { 0xFA00, 0xFF00, CPU_8086 , o_None, "CLI" , "", 0 },
    { 0xFB00, 0xFF00, CPU_8086 , o_None, "STI" , "", 0 },
    { 0xFC00, 0xFF00, CPU_8086 , o_None, "CLD" , "", 0 },
    { 0xFD00, 0xFF00, CPU_8086 , o_None, "STD" , "", 0 },

    { 0xFE00+r0, 0xFF38, CPU_8086 , o_rm8,      "INC" , "", 0 }, // FE/0
    { 0xFE00+r1, 0xFF38, CPU_8086 , o_rm8,      "DEC" , "", 0 }, // FE/1

    { 0xFF00+r0, 0xFF38, CPU_8086 , o_rm16,     "INC" , "", 0 }, // FF/0
    { 0xFF00+r1, 0xFF38, CPU_8086 , o_rm16,     "DEC" , "", 0 }, // FF/1
    { 0xFF00+r2, 0xFF38, CPU_8086 , o_rm16,     "CALL", "", 0 }, // FF/2 CALL indirect in segment
    { 0xFF00+r3, 0xFF38, CPU_8086 , o_rm16,     "CALL", "DWORD PTR ", 0 }, // FF/3 CALL indirect inter-segment
    { 0xFF00+r4, 0xFF38, CPU_8086 , o_rm16,     "JMP" , "", LFFLAG }, // FF/4 JMP indirect in segment
    { 0xFF00+r5, 0xFF38, CPU_8086 , o_rm16,     "JMP" , "DWORD PTR ", LFFLAG }, // FF/5 JMP indirect inter-segment
    { 0xFF00+r6, 0xFF38, CPU_8086 , o_seg_rm16, "POP" , "", 0 },
//  { 0xFF00+r7, 0xFF38, CPU_8086 , o_rm16,     "???" , "", 0 },

    { 0x0000, 0x0000, CPU_8086, o_Invalid, "", "", 0 }
};


// =====================================================


/*static*/ const char *reg8[] = {
    "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"
};

/*static*/ const char *reg16[] = {
    "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"
};

/*static*/ const char *sreg[] = {
    "ES", "CS", "SS", "DS",
    "S4", "S5", "S6", "S7" // <- in case of invalid values
};


// =====================================================


const struct InstrRec *Dis8086::find_opcode(int op, int &len)
{
    len = 1;

    const struct InstrRec *p;

    p = I8086_opcdTable;

    while (p->andWith && ((op & p->andWith) != p->cmpWith)) {
// p->cpu is not yet used, but will probably need something like this:
//      || (p->cpu > subtype)
        p++;
    }

    // if the second byte was being tested for more than reg, it's a 2-byte opcode
    if (p->andWith & 0xC7) {
        len = 2;
    }

    return p;
}




// =====================================================


int Dis8086::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    const struct InstrRec *instr = NULL;
    addr_t      ad;
    int         opcd;
    int         seg;
    addr_t      ra;
    char        s[256];
    int         i;
    int         mod,reg,rm;
#if 0
    int         n;
    int         d = 0;
    char        *p, *l;
#endif

    // get first byte of instruction
    ad    = addr;
    int len = 1;

    // find opcode
    opcd = ReadByte(ad)*256 + ReadByte(ad+1);
    instr = find_opcode(opcd, len);

#if 0
    if (instr) printf("\r\n %.2x %s ", ReadByte(ad)*256, instr->op);
#endif

    strcpy(opcode, instr->op);
    strcpy(parms,  instr->parms);
    lfref   = instr->lfref;
    refaddr = 0;
    ad += len;

    // pre-split next byte if it's a mode/reg
    // (note: this might read an out-of-range byte)
    i = ReadByte(ad);
    mod = (i >> 6) & 3; reg = (i >> 3) & 7; rm = i & 7;

    // handle opcode according to instruction type
    switch (instr->typ) {
	case o_None:
            break;

        case o_imm8:
        case o_rimm8:
            if (parms[0] == ',') {
                parms[0] = 0;
            }
            if (instr->typ == o_rimm8) {
                reg = (opcd >> 8) & 7;
                strcat(parms, reg8[reg]);
                strcat(parms, ",");
            }
            i = ReadByte(ad);
            len++;
            ad++;
            H2Str(i, s);
            strcat(parms, s);
            if (instr->parms[0] == ',') {
                strcat(parms, instr->parms);
            }
            break;

        case o_imm16:
        case o_num16:
        case o_rimm16:
            if (parms[0] == ',') {
                parms[0] = 0;
            }
            if (instr->typ == o_rimm16) {
                reg = (opcd >> 8) & 7;
                strcat(parms, reg16[reg]);
                strcat(parms, ",");
             }
            ra = ReadWord(ad);
            len += 2;
            ad += 2;
            if (instr->typ == o_num16) {
                H4Str(ra, s);
            } else {
                RefStr(ra, s, lfref, refaddr);
            }
            strcat(parms, s);
            if (instr->parms[0] == ',') {
                strcat(parms, instr->parms);
            }
            break;

	case o_Far:
            ra = ReadWord(ad);
            seg = ReadWord(ad+2);
            len += 4;
            ad += 4;
            H4Str(seg, s);
            strcat(parms, s);
            strcat(parms, ":");
            H4Str(ra, s);
//          RefStr(ra, s, lfref, refaddr);
            strcat(parms, s);
            break;

        case o_rm8:
        case o_rmi8:
        case o_rot8:
        case o_rm8_reg8:    // r/m8, reg8
        case o_reg8_rm8:    // reg8, r/m8
            len++;
            ad++;
            if (parms[0] == ',') {
                parms[0] = 0;
            }
            if (instr->typ == o_reg8_rm8) {
                strcat(parms, reg8[reg]);
                strcat(parms, ",");
            }
            if (mod == 3) { // register-to-register
                strcat(parms, reg8[rm]);
            } else if (mod == 0 && rm == 6) { // absolute
                ra = ReadWord(ad);
                len += 2;
                ad += 2;
                strcat(parms, "BYTE ");
                RefStr(ra, s, lfref, refaddr);
                strcat(parms, s);
            } else { // indexed
                switch(mod) {
                    default:
                    case 0: // disp=0
                        ra = 0;
                        break;
                    case 1: // disp=low,sext
                        ra = ReadByte(ad);
                        len += 1;
                        ad += 1;
                        if (ra & 0x80) {
                             ra |= 0xFF00;
                        }
                        break;
                    case 2: // disp=low,high
                        ra = ReadWord(ad);
                        len += 2;
                        ad += 2;
                        break;
                }

                strcat(parms, "BYTE ");
                if (ra) {
                    RefStr(ra, s, lfref, refaddr);
                    strcat(parms, s);
                } else {
                    strcat(parms, "0");
                }

                switch(rm) {
                    case 0: // BX+SI+ea
                        strcat(parms, "[BX+SI]");
                        break;
                    case 1: // BX+DI+ea
                        strcat(parms, "[BX+DI]");
                        break;
                    case 2: // BP+SI+ea
                        strcat(parms, "[BP+SI]");
                        break;
                    case 3: // BP+DI+ea
                        strcat(parms, "[BP+DI]");
                        break;
                    case 4: // SI+ea
                        strcat(parms, "[SI]");
                        break;
                    case 5: // DI+ea
                        strcat(parms, "[DI]");
                        break;
                    case 6: // BP+ea
                        strcat(parms, "[BP]");
                        break;
                    case 7: // BX+ea
                        strcat(parms, "[BX]");
                        break;
                }
            }
            switch (instr->typ) {
                case o_rm8_reg8:
                    strcat(parms, ",");
                    strcat(parms, reg8[reg]);
                    break;

                case o_rmi8:
                    strcat(parms, ",");
                    i = ReadByte(ad);
                    len++;
                    ad++;
                    H2Str(i, s);
                    strcat(parms, s);
                    break;

                case o_rot8:
                    strcat(parms, ",");
                    i = ReadByte(ad);
                    len++;
                    ad++;
                    sprintf(parms+strlen(parms), "%d", i);
                    break;

                default:
                    break;
            }
            if (instr->parms[0] == ',') {
                strcat(parms, instr->parms);
            }
            break;

        case o_rm16:
        case o_rmi16:
        case o_rot16:
        case o_rmi8_16:
        case o_rm16_reg16:  // r/m16, reg16
        case o_reg16_rm16:  // reg16, r/m16
        case o_rm16_seg:
        case o_seg_rm16:
        case o_ESC:
            len++;
            ad++;
            if (parms[0] == ',') {
                parms[0] = 0;
            }
            switch(instr->typ) {
                case o_reg16_rm16:
                    strcat(parms, reg16[reg]);
                    strcat(parms, ",");
                    break;

                case o_seg_rm16:
                    strcat(parms, sreg[reg]);
                    strcat(parms, ",");
                    break;

                case o_ESC:
                    sprintf(parms+strlen(parms), "%d/%d", (opcd >> 8) & 7, reg);
                    strcat(parms, ",");
                    break;

                default:
                    break;
            }

            if (mod == 3) { // register-to-register
                strcat(parms, reg16[rm]);
            } else if (mod == 0 && rm == 6) { // absolute
                ra = ReadWord(ad);
                len += 2;
                ad += 2;
                strcat(parms, "WORD ");
                RefStr(ra, s, lfref, refaddr);
                strcat(parms, s);
            } else { // indexed
                switch(mod) {
                    default:
                    case 0: // disp=0
                        ra = 0;
                        break;
                    case 1: // disp=low,sext
                        ra = ReadByte(ad);
                        len += 1;
                        ad += 1;
                        if (ra & 0x80) {
                             ra |= 0xFF00;
                        }
                        break;
                    case 2: // disp=low,high
                        ra = ReadWord(ad);
                        len += 2;
                        ad += 2;
                        break;
                }

                strcat(parms, "WORD ");
                if (ra) {
                    RefStr(ra, s, lfref, refaddr);
                    strcat(parms, s);
                } else {
                    strcat(parms, "0");
                }

                switch(rm) {
                    case 0: // BX+SI+ea
                        strcat(parms, "[BX+SI]");
                        break;
                    case 1: // BX+DI+ea
                        strcat(parms, "[BX+DI]");
                        break;
                    case 2: // BP+SI+ea
                        strcat(parms, "[BP+SI]");
                        break;
                    case 3: // BP+DI+ea
                        strcat(parms, "[BP+DI]");
                        break;
                    case 4: // SI+ea
                        strcat(parms, "[SI]");
                        break;
                    case 5: // DI+ea
                        strcat(parms, "[DI]");
                        break;
                    case 6: // BP+ea
                        strcat(parms, "[BP]");
                        break;
                    case 7: // BX+ea
                        strcat(parms, "[BX]");
                        break;
                }
            }

            switch (instr->typ) {
                case o_rm16_reg16:
                    strcat(parms, ",");
                    strcat(parms, reg16[reg]);
                    break;

                case o_rm16_seg:
                    strcat(parms, ",");
                    strcat(parms, sreg[reg]);
                    break;

                case o_rmi16:
                    strcat(parms, ",");
                    i = ReadWord(ad);
                    len += 2;
                    ad += 2;
//                  H4Str(i, s);
                    RefStr(i, s, lfref, refaddr);
                    strcat(parms, s);
                    break;

                case o_rmi8_16:
                    strcat(parms, ",");
                    i = ReadByte(ad);
                    len += 1;
                    ad += 1;
                    if (i & 0x80) {
                         i |= 0xFF00;
                    }
                    H4Str(i, s);
                    strcat(parms, s);
                    break;

                case o_rot16:
                    strcat(parms, ",");
                    i = ReadByte(ad);
                    len++;
                    ad++;
                    sprintf(parms+strlen(parms), "%d", i);
                    break;

                default:
                    break;
            }
            if (instr->parms[0] == ',') {
                strcat(parms, instr->parms);
            }
            break;

        case o_rel8:
            i = ReadByte(ad++);
            len++;
            if (i >= 128) {
                i = i - 256;
            }
            ra = ((ad + i) & 0xFFFF);
            RefStr(ra, s, lfref, refaddr);
            strcat(parms, s);
            break;

        case o_ENTER: // ENTER dsp16, imm8
            ra = ReadWord(ad);
            i = ReadByte(ad+2);
            len += 3;
            ad += 3;
            H4Str(ra, s);
            strcat(parms, s);
            strcat(parms, ",");
            H2Str(i, s);
            strcat(parms, s);
            break;

        default:
            break;
    }


#if 0
    // handle substitutions
    p = s;
    l = parms;
    while (*l) {
        switch (*l) {
            // === IX/IY indexed offset ===
            case 'd':
                if (d >= 128) {
                    p[-1] = '-';
                    d = 256 - d;
                }

                H2Str(d, p);
                p += strlen(p);
                break;

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

            // === 1-byte relative branch offset ===
            case 'r':
                i = ReadByte(ad++);
                len++;
                if (i >= 128) {
                    i = i - 256;
                }
                // compute offset but preserve bank address
                ra = ((ad + i) & 0xFFFF) | (ad & 0xFF0000);

                RefStr(ra, p, lfref, refaddr);
                p += strlen(p);

                // rip-stop check for suspicious branch offsets
                if (i == 0 || i == -1) {
                    lfref |= RIPSTOP;
                }
                // rip-stop check for DJNZ forward
                if (ReadByte(addr) == 0x10 && i > 0) {
                    lfref |= RIPSTOP;
                }

                break;

            // === RST nn extra bytes ===
            case 'x':
                n = (ReadByte(ad-1) >> 3) & 7;
                for (i=0; i < rom._rst_xtra[n]; i++) {
                    *p++ = ',';
                    H2Str(ReadByte(ad++), p);
                    p += strlen(p);
                    len++;
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
#endif

#if 1
    // rip-stop checks
    if (opcode[0]) {
        int op = ReadByte(addr);
        // three 00 or FF in a row
        if ((op == 0x00 || op == 0xFF) && op == ReadByte(addr-1) && op == ReadByte(addr-2)) {
            lfref |= RIPSTOP;
        }
    }
#endif

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


