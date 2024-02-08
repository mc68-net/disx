// DisThumb.cpp

static const char versionName[] = "ARM Thumb disassembler";
#include "disx.h"
#include "discpu.h"
#include "string.h"

//const int maxInstLen = 8;  // length in bytes of longest instruction

struct InstrRec
{
    uint32_t        andWith;
    uint32_t        cmpWith;
    int             typ;        // typ
// need to add ARM CPU version too?
    const char      *op;        // mnemonic
    uint8_t         lfref;  // lfFlag/refFlag/codeRef
};
typedef const struct InstrRec *InstrPtr;


class DisThumb : public CPU {
public:
    DisThumb(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
    virtual bool has_odd_code() { return true; } // enable "odd code" mode for Thumb

private:
    int FetchWord(int addr, int &len);
    InstrPtr Thumb_FindInstr(uint32_t opcd);
    void SPR_Name(char *s, int reg);
    void shifter_operand_1(char *parms, int opcd);
    void addr_mode_2(char *parms, long addr, int &lfref, addr_t &refaddr, int opcd);
    void addr_mode_3(char *parms/*, long addr*/, int opcd);
    void ldm_stm_regs_4(char *op, char *parms, int opcd);
};


enum {
    CPU_THUMB = 0,  // don't use Thumb2 instructions
    CPU_THUMB2,     // do use Thumb2 instructions
};


DisThumb cpu_Thumb    ("THUMB",     CPU_THUMB,  LITTLE_END, ADDR_32, '.', '$', ".byte", ".word", ".long");
DisThumb cpu_Thumb_LE ("THUMB-LE",  CPU_THUMB,  LITTLE_END, ADDR_32, '.', '$', ".byte", ".word", ".long");
DisThumb cpu_Thumb_BE ("THUMB-BE",  CPU_THUMB,  BIG_END,    ADDR_32, '.', '$', ".byte", ".word", ".long");
DisThumb cpu_Thumb2   ("THUMB2",    CPU_THUMB2, LITTLE_END, ADDR_32, '.', '$', ".byte", ".word", ".long");
DisThumb cpu_Thumb2_LE("THUMB2-LE", CPU_THUMB2, LITTLE_END, ADDR_32, '.', '$', ".byte", ".word", ".long");
DisThumb cpu_Thumb2_BE("THUMB2-BE", CPU_THUMB2, BIG_END,    ADDR_32, '.', '$', ".byte", ".word", ".long");


DisThumb::DisThumb(const char *name, int subtype, int endian, int addrwid,
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


enum {
    o_Invalid,
    o_Implied,
    o_TwoRegs,

    o_MOVS,         // special case with shift = 0
    o_MoveShift,    // 0xxx/1xxx move shifted register
    o_ADD_SUB,      // 1xxx add/subtract
    o_Imm8,         // 2xxx 8-bit immediate
    o_ALU,          // 4xxx ALU operations
    o_HiReg,        // 4xxx hi-register ADD/CMP/MOV
    o_LDR_PC,       // 4xxx LDR Rd,[PC, #imm]
    o_BX_BLX,       // 47xx BX/BLX
    o_LDR_STR_ofs,  // 5xxx LDR/STR/LDRB/STRB with offset
                    // 5xxx LDRH/STRH/LDSB/LDSH
    o_STR_LDR,      // 6xxx STR/LDR
    o_STRB_LDRB,    // 7xxx STRB/LDRB
    o_STRH_LDRH,    // 8xxx STRH/LDRH
    o_STR_LDR_SP,   // 9xxx STR/LDR Rd,[sp, #imm]
    o_LoadAddr,     // Axxx ADD to SP or PC
    o_PUSH_POP,     // Bxxx PUSH/POP
    o_ImmS7,        // Bxxx 7-bit add/sub to SP
    o_STM_LDM,      // Cxxx stmia/ldmia
    o_B8,           // Dxxx b<cc>.n with 8-bit offset
    o_B11,          // Exxx b.n with 11-bit offset
    o_BL23,         // Fxxx bl 23-bit relative
    o_Op8,          //      8-bit immediate operand

    o_MRS,
    o_MSR,
};

static const struct InstrRec Thumb_opcdTable[] =
{
    // and     cmp      typ        op   lf ref

    //  1 -- 0  0  0  o  o  n  n  n  n  n  rs rs rs rd rd rd -- move shifted register
    // 0xxx
    { 0xFFC0, 0x0000, o_MOVS,      "movs", 0 }, // special case of LSL with zero shift
    { 0xF800, 0x0000, o_MoveShift, "lsls", 0 },
    { 0xF800, 0x0800, o_MoveShift, "lsrs", 0 },
    { 0xF800, 0x1000, o_MoveShift, "asrs", 0 },

    //  2 -- 0  0  0  1  1  I  op r  r  r  rs rs rs rd rd rd -- add/subtract
    // 1xxx 
    { 0xFA00, 0x1800, o_ADD_SUB, "adds", 0 }, // 1800 - add, reg 1C00 - add, imm
    { 0xFA00, 0x1A00, o_ADD_SUB, "subs", 0 }, // 1A00 - sub, reg 1E00 - sub, imm

    //  3 -- 0  0  1  op op r  r  r  n  n  n  n  n  n  n  n -- move/compare/add/subtract immediate
    // 2xxx/3xxx
    // note: the "s" of this group appears to be in
    // the gnu opcodes, but not the official ARM Thumb syntax
    { 0xF800, 0x2000, o_Imm8, "movs", 0 },
    { 0xF800, 0x2800, o_Imm8, "cmp",  0 },
    { 0xF800, 0x3000, o_Imm8, "adds", 0 },
    { 0xF800, 0x3800, o_Imm8, "subs", 0 },

    //  4 -- 0  1  0  0  0  0  o  o  o  o  rs rs rs rd rd rd -- ALU operations
    // 4xxx
    // note: the "s" of this group appears to be in
    // the gnu opcodes, but not the official ARM Thumb syntax
    { 0xFFC0, 0x4000, o_ALU, "ands", 0 },
    { 0xFFC0, 0x4040, o_ALU, "eors", 0 },
    { 0xFFC0, 0x4080, o_ALU, "lsls", 0 },
    { 0xFFC0, 0x40C0, o_ALU, "lsrs", 0 },

    { 0xFFC0, 0x4100, o_ALU, "asrs", 0 },
    { 0xFFC0, 0x4140, o_ALU, "adcs", 0 },
    { 0xFFC0, 0x4180, o_ALU, "sbcs", 0 },
    { 0xFFC0, 0x41C0, o_ALU, "rors", 0 },

    { 0xFFC0, 0x4200, o_ALU, "tst",  0 },
    { 0xFFC0, 0x4240, o_ALU, "negs", 0 },
    { 0xFFC0, 0x4280, o_ALU, "cmp",  0 },
    { 0xFFC0, 0x42C0, o_ALU, "cmn",  0 },

    { 0xFFC0, 0x4300, o_ALU, "orrs", 0 },
    { 0xFFC0, 0x4340, o_ALU, "muls", 0 },
    { 0xFFC0, 0x4380, o_ALU, "bics", 0 },
    { 0xFFC0, 0x43C0, o_ALU, "mvns", 0 },

    //  5 -- 0  1  0  0  0  1  op op H1 H2 rs rs rs rd rd rd -- hi register operations/branch exchange
    // 4xxx
    { 0xFF00, 0x4400, o_HiReg,    "add",  0 },
    { 0xFF00, 0x4500, o_HiReg,    "cmp",  0 },
    { 0xFF00, 0x4600, o_HiReg,    "mov",  0 },
    { 0xFF87, 0x4700, o_BX_BLX,   "bx",   LFFLAG },
    { 0xFF87, 0x4780, o_BX_BLX,   "blx",  0 },

    //  6 -- 0  1  0  0  1  r  r  r  n  n  n  n  n  n  n  n -- PC-relative load
    // 4xxx
    { 0xF800, 0x4800, o_LDR_PC,   "ldr", REFFLAG },

    //  7 -- 0  1  0  1  L  B  0  ro ro ro rb rb rb rd rd rd -- load/store with register offset
    // 5xxx 
    { 0xFE00, 0x5000, o_LDR_STR_ofs, "str",  0 },
    { 0xFE00, 0x5400, o_LDR_STR_ofs, "strb", 0 },
    { 0xFE00, 0x5800, o_LDR_STR_ofs, "ldr",  0 },
    { 0xFE00, 0x5C00, o_LDR_STR_ofs, "ldrb", 0 },

    //  8 -- 0  1  0  1  H  S  1  ro ro ro rb rb rb rd rd rd -- load/store sign-extended byte/halfword
    // 5xxx (not tested)
    { 0xFE00, 0x5200, o_LDR_STR_ofs, "strh", 0 },
    { 0xFE00, 0x5600, o_LDR_STR_ofs, "ldrh", 0 },
    { 0xFE00, 0x5A00, o_LDR_STR_ofs, "ldsb", 0 },
    { 0xFE00, 0x5E00, o_LDR_STR_ofs, "ldsh", 0 },

    //  9 -- 0  1  1  B  L  o  o  o  o  o  rb rb rb rd rd rd -- load/store with immediate offset
    // 6xxx/7xxx 
    { 0xF800, 0x6000, o_STR_LDR,   "str",  0 },
    { 0xF800, 0x6800, o_STR_LDR,   "ldr",  0 },
    { 0xF800, 0x7000, o_STRB_LDRB, "strb", 0 },
    { 0xF800, 0x7800, o_STRB_LDRB, "ldrb", 0 },

    // 10 -- 1  0  0  0  L  o  o  o  o  o  rb rb rb rd rd rd -- load/store halfword
    // 8xxx
    { 0xF800, 0x8000, o_STRH_LDRH, "strh", 0 },
    { 0xF800, 0x8800, o_STRH_LDRH, "ldrh", 0 },

    // 11 -- 1  0  0  1  L  r  r  r  n  n  n  n  n  n  n  n -- SP-relative load/store
    // 9xxx
    { 0xF800, 0x9000, o_STR_LDR_SP, "str", 0 },
    { 0xF800, 0x9800, o_STR_LDR_SP, "ldr", 0 },

    // 12 -- 1  0  1  0  SP r  r  r  n  n  n  n  n  n  n  n -- load address
    // Axxx 
    { 0xF800, 0xA000, o_LoadAddr, "ldr",   REFFLAG }, // pc  (add Rd, pc, #imm)
    { 0xF800, 0xA800, o_LoadAddr, "add",   0 },       // sp  {add Rd, sp, #imm)

    // 13 -- 1  0  1  1  0  0  0  0  S  n  n  n  n  n  n  n -- add offset to stack pointer
    { 0xFF80, 0xB000, o_ImmS7,    "add",   0 },
    { 0xFF80, 0xB080, o_ImmS7,    "sub",   0 },

    // 14 -- 1  0  1  1  L  1  0  R  R  R  r  r  r  r  r  r -- push/pop registers

    // Bxxx misc 16-bit instructions
//  { 0xFF80, 0xB000, }, // add immediate to SP
//  { 0xFF80, 0xB800, }, // subtract immediate from SP
//  { 0xFFC0, 0xB200, o_TwoRegs,  "sxth",  0 }, // not tested
//  { 0xFFC0, 0xB240, o_TwoRegs,  "sxtb",  0 }, // not tested
    { 0xFFC0, 0xB280, o_TwoRegs,  "uxth",  0 },
    { 0xFFC0, 0xB2C0, o_TwoRegs,  "uxtb",  0 },
    { 0xFE00, 0xB400, o_PUSH_POP, "push",  0 },
//  { 0xFFE0, 0xB660, }, // CPS change processor state
//  { 0xFFC0, 0xBA00, }, // REV byte-reverse word
//  { 0xFFC0, 0xBA40, }, // REV16 byte-reverse packed halfword
//  { 0xFFC0, 0xBA80, }, // not defined?
//  { 0xFFC0, 0xBAC0, }, // REVSH byte-reverse signed halfword
    { 0xFE00, 0xBC00, o_PUSH_POP, "pop" ,  0 },
    { 0xFF00, 0xBE00, o_Op8,      "bkpt",  0 }, // breakpoint
    { 0xFFFF, 0xBF00, o_Implied,  "nop",   0 },
    { 0xFFFF, 0xBF30, o_Implied,  "wfi",   0 },
//  { 0xFFFF, 0xBF  , }, // hints

    // 15 -- 1  1  0  0  L  r  r  r  r  r  r  r  r  r  r  r -- multiple load/store
    { 0xF800, 0xC000, o_STM_LDM,  "stmia", 0 },
    { 0xF800, 0xC800, o_STM_LDM,  "ldmia", 0 },

    // 16 -- 1  1  0  1  c  c  c  c  o  o  o  o  o  o  o  o -- conditional branch
    { 0xFF00, 0xD000, o_B8,       "beq.n", REFFLAG | CODEREF },
    { 0xFF00, 0xD100, o_B8,       "bne.n", REFFLAG | CODEREF },
    { 0xFF00, 0xD200, o_B8,       "bcs.n", REFFLAG | CODEREF }, // also bhs
    { 0xFF00, 0xD300, o_B8,       "bcc.n", REFFLAG | CODEREF }, // also blo
    { 0xFF00, 0xD400, o_B8,       "bmi.n", REFFLAG | CODEREF },
    { 0xFF00, 0xD500, o_B8,       "bpl.n", REFFLAG | CODEREF },
    { 0xFF00, 0xD600, o_B8,       "bvs.n", REFFLAG | CODEREF },
    { 0xFF00, 0xD700, o_B8,       "bvc.n", REFFLAG | CODEREF },
    { 0xFF00, 0xD800, o_B8,       "bhi.n", REFFLAG | CODEREF },
    { 0xFF00, 0xD900, o_B8,       "bls.n", REFFLAG | CODEREF },
    { 0xFF00, 0xDA00, o_B8,       "bge.n", REFFLAG | CODEREF },
    { 0xFF00, 0xDB00, o_B8,       "blt.n", REFFLAG | CODEREF },
    { 0xFF00, 0xDC00, o_B8,       "bgt.n", REFFLAG | CODEREF },
    { 0xFF00, 0xDD00, o_B8,       "ble.n", REFFLAG | CODEREF },
//  { 0xFF00, 0xDE00, o_Invalid,  "",      0 }, // undefined

    // 17 -- 1  1  0  1  1  1  1  1  n  n  n  n  n  n  n  n -- software interrupt
//  { 0xFF00, 0xDF00, o_Op8,      "swi",   0 },

    // 18 -- 1  1  1  0  0  o  o  o  o  o  o  o  o  o  o  o -- unconditional branch
    { 0xF800, 0xE000, o_B11,      "b.n",   LFFLAG | REFFLAG | CODEREF },
//  { 0xF800, 0xE800, o_    ,     "",      0 },

    { 0xFFE0, 0xF3E0, o_MRS, "mrs", 0 },
    { 0xFFE0, 0xF380, o_MSR, "msr", 0 },

    // 19 -- 1  1  1  1  H  o  o  o  o  o  o  o  o  o  o  o -- long branch with link
//  { 0xF800F800, 0xF000F800, o_BL23, "bl", REFFLAG | CODEREF }, // 32-bit match version
    { 0xF800, 0xF000, o_BL23,     "bl",    REFFLAG | CODEREF },
//  { 0xF800, 0xF800, o_BL23,     "bl",    REFFLAG | CODEREF }, // low half of o_BL23

    {0x00000000, 0x00000000, o_Invalid,   ""     , 0                }
};

static const char *regs[] =
{
    "r0",  "r1",  "r2",  "r3",
    "r4",  "r5",  "r6",  "r7",
    "r8",  "r9",  "r10", "r11",
    "r12", "sp",  "lr",  "pc"
};


// register names for MRS and MSR
static const char *sprs[] =
{
    "APSR",    "IAPSR",   "EAPSR",       "XPSR",      //  0-3
    "4",       "IPSR",    "EPSR",        "IEPSR",     //  4-7
    "MSP",     "PSP",     "10",          "11",        //  8-11
    "12",      "13",      "14",          "15",        // 12-15
    "PRIMASK", "BASEPRI", "BASEPRI_MAX", "FAULTMASK", // 16-19
    "CONTROL",                                        // 20-
};


int DisThumb::FetchWord(int addr, int &len)
{
    addr_t w;

    w = ReadWord(addr + len);
    len += 2;
    return w;
}


InstrPtr DisThumb::Thumb_FindInstr(uint32_t opcd)
{
    InstrPtr p;

    p = Thumb_opcdTable;

    while (p->andWith && ((opcd & p->andWith) != p->cmpWith)) p++;

    return p;
}


void DisThumb::SPR_Name(char *s, int reg)
{
    if (reg < ARRAY_SIZE(sprs)) {
        strcpy(s, sprs[reg]);
    } else {
        sprintf(s, "%d", reg);
    }
}


int DisThumb::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
//  addr_t      ad;
    uint16_t    opcd;
    int         i;
    InstrPtr    instr;
    char        s[256];
    bool        invalid;
    bool        addcond;
    bool        addflags;
    addr_t      ra;

    *opcode  = 0;
    *parms   = 0;
    int len  = 0;
    lfref    = 0;
    refaddr  = 0;
    invalid  = true;
    addcond  = false;
    addflags = false;

//  ad = addr;
    opcd = FetchWord(addr, len);
//  32-bit instruction if [15:11] is 11101, 11110, or 11111
    instr = Thumb_FindInstr(opcd);

    if (!(addr & 1) && instr && instr->typ && *(instr->op)) {
        invalid  = false;
        addcond  = true;
        len     = 2;
        strcpy(opcode, instr->op);
        lfref  = instr->lfref;

        switch (instr->typ) {
            case o_Implied:
                break;

            case o_MOVS:
                sprintf(parms, "%s, %s", regs[opcd & 7], regs[(opcd >> 3) & 7]);
                break;

            case o_MoveShift:
                // ofs (opcd >> 6) & 31
                // Rs  regs[(opcd >> 3) & 7]
                // Rd  regs[opcd & 7]
                sprintf(parms, "%s, %s, #%d", regs[opcd & 7], regs[(opcd >> 3) & 7], (opcd >> 6) & 31);
                break;

            case o_ADD_SUB:
                // Rd regs[opcd & 7]
                // Rs regs[(opcd >> 3) & 7]
                // Rn regs[(opcd >> 6) & 7]
                // ofs = '#' + str((opcd >> 6) & 7)
                if (opcd & 0x0400) {
                    // op Rd, Rs, #ofs
                    sprintf(parms, "%s, %s, #%d", regs[opcd & 7], regs[(opcd >> 3) & 7], (opcd >> 6) & 7);
                } else {
                    // op Rd, Rs, Rn
                    sprintf(parms, "%s, %s, %s", regs[opcd & 7], regs[(opcd >> 3) & 7], regs[(opcd >> 6) & 7]);
                }
                break;

            case o_Imm8:
                i = opcd & 0x00FF;
                sprintf(parms, "%s, #%d", regs[(opcd >> 8) & 7], opcd & 0xFF);
                break;

            case o_ALU:
                sprintf(parms, "%s, %s", regs[opcd & 7], regs[(opcd >> 3) & 7]);
                break;

            case o_HiReg:
                    sprintf(parms, "%s, %s", regs[((opcd >> 4) & 8) | (opcd & 7)],
                                             regs[(opcd >> 3) & 15]);
                break;

            case o_LDR_PC:
                ra = (addr & 0xFFFFFFFC) + 4 + (opcd & 0xFF) * 4;
//              sprintf(parms, "r%d, [pc, #%d]", (opcd >> 8) & 7, i);
                sprintf(parms, "r%d, ", (opcd >> 8) & 7);
                RefStr(ra, parms + strlen(parms), lfref, refaddr);
                break;

            case o_BX_BLX:
                sprintf(parms, "%s", regs[(opcd >> 3) & 15]);
                break;

            case o_LDR_STR_ofs:
                // Rd regs[opcd & 7]
                // Rb regs[(opcd >> 3) & 7]
                // Ro regs[(opcd >> 6) & 7]
                sprintf(parms, "%s, [%s, %s]", regs[opcd & 7], regs[(opcd >> 3) & 7], regs[(opcd >> 6) & 7]);
                break;

            case o_STR_LDR:
                sprintf(parms, "r%d, [r%d, #%d]", opcd & 7, (opcd >> 3) & 7, ((opcd >> 6) & 0x1F) * 4);
                break;

            case o_STRB_LDRB:
                sprintf(parms, "r%d, [r%d, #%d]", opcd & 7, (opcd >> 3) & 7, (opcd >> 6) & 0x1F);
                break;

            case o_STRH_LDRH:
                sprintf(parms, "r%d, [r%d, #%d]", opcd & 7, (opcd >> 3) & 7, ((opcd >> 6) & 0x1F) * 2);
                break;

            case o_STR_LDR_SP:
                sprintf(parms, "r%d, [sp, #%d]", (opcd >> 8) & 7, (opcd & 0x00FF) * 4);
                break;

            case o_LoadAddr:
                if (opcd & 0x0800) { // add sp
                    sprintf(parms, "%s, sp, #%d", regs[(opcd >> 8) & 7], (opcd & 0xFF) * 4);
                } else { // add pc
                    ra = (addr & 0xFFFFFFFC) + 4 + (opcd & 0xFF) * 4;
                    sprintf(parms, "%s, ", regs[(opcd >> 8) & 7]);

                    RefStr(ra, parms + strlen(parms), lfref, refaddr);
                }
                break;

            case o_ImmS7:
                i = opcd & 0x007F;
                sprintf(parms, "sp, #%d", i * 4);
                break;

            case o_PUSH_POP:
            {
                bool comma = false;
                strcat(parms, "{");
                //if ((opcd & 0x1FF) == 0) strcat(parms,"--"); // no regs
                for (i = 0; i <= 8; i++) {
                    if (opcd & (1 << i)) {
                        if (!comma) {
                            comma = true;
                        } else {
                            strcat(parms, ", ");
                        }

                        switch (i) {
                            case 0: case 1: case 2: case 3:
                            case 4: case 5: case 6: case 7:
                                strcat(parms, regs[i]);
                                break;

                            case 8:
                                // bit 11: push=0, pop=1
                                if (opcd & 0x0800) {
                                    // pop register #8 = pc
                                    strcat(parms, "pc");
                                    lfref |= LFFLAG;
                                } else {
                                    // push register #8 = lr
                                    strcat(parms, "lr");
                                }

                            default: // should not get here!
//                              strcat(parms, "?");
                                break;
                        }
                    }
                }
                strcat(parms, "}");
                break;
            }

            case o_STM_LDM:
            {
                bool comma = false;
                sprintf(parms, "r%d!, {", (opcd >> 8) & 7);
                //if ((opcd & 0x7FF) == 0) strcat(parms,"--"); // no regs
                for (i = 0; i <= 7; i++) {
                    if (opcd & (1 << i)) {
                        if (!comma) {
                            comma = true;
                        } else {
                            strcat(parms, ", ");
                        }

                        strcat(parms, regs[i]);
                        break;
                    }
                }
                strcat(parms, "}");
                break;
            }

            case o_B8:
                i = opcd & 0x00FF;
                // sign extend
                if (i & 0x0080) {
                    i |= 0xFFFFFF00; // -0x80 - n or something?
                }
                ra = addr + i*2 + 4;
                RefStr(ra, parms, lfref, refaddr);
                break;

            case o_B11:
                i = opcd & 0x07FF;
                // sign extend
                if (i & 0x0400) {
                    i |= 0xFFFFF800; // -0x400 - n or something?
                }
                ra = addr + i*2 + 4;
                RefStr(ra, parms, lfref, refaddr);
                break;

            case o_BL23:
            {
                uint16_t opcd2 = FetchWord(addr, len);
                // ensure that second word is valid
                if ((opcd2 & 0xF800) != 0xF800) {
                    invalid = true;
                    break;
                }
                // get high 11 bits
                i = (opcd & 0x07FF) << 12;
                // sign-extend a negative value
                if (opcd & 0x400) {
                    i |= 0xFF800000;
                }
                // get low 11 bits
                i |= (opcd2 & 0x07FF) << 1;
//              strcat(parms, regs[(opcd >> 8) & 7]);
//              sprintf(parms + strlen(parms), ", #%d", opcd & 0xFF);
                ra = addr + len + i;
                RefStr(ra, parms, lfref, refaddr);
                break;
            }

            case o_TwoRegs:
                sprintf(parms, "%s, %s", regs[opcd & 7], regs[(opcd >> 3) & 7]);
                break;

            case o_MRS:
            {
                uint16_t opcd2 = FetchWord(addr, len);
                // ensure that second word is valid
                if ((opcd2 & 0xF000) != 0x8000) {
                    invalid = true;
                    break;
                }
                SPR_Name(s, opcd2 & 0xFF);
                sprintf(parms, "%s, %s", regs[(opcd2 >> 8) & 3], s);
                break;
            }

            case o_MSR:
            {
                uint16_t opcd2 = FetchWord(addr, len);
                // ensure that second word is valid
                if ((opcd2 & 0xF800) != 0x8800) {
                    invalid = true;
                    break;
                }
                SPR_Name(s, opcd2 & 0xFF);
                sprintf(parms, "%s, %s", s, regs[opcd & 3]);
                break;
            }

            case o_Op8:
                i = opcd & 0x00FF;
                sprintf(parms, "#0x%.2x", opcd & 0xFF);
                break;

            default:
                strcpy(parms, "???");
                break;
        }
    }

    if (invalid || rom.AddrOutRange(addr)) {
        strcpy(opcode, _dlopcd);
        sprintf(parms, "???");
        len     = 0;
        lfref   = 0;
        refaddr = 0;
        addcond  = false;
    }

    return len;
}
