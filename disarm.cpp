// disarm.cpp

static const char versionName[] = "ARM disassembler";
#include "disx.h"
#include "discpu.h"

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


class DisARM : public CPU {
public:
    DisARM(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);

private:
    addr_t ARM_FetchLong(int addr, int &len);
    InstrPtr ARM_FindInstr(uint32_t opcd);
    void shifter_operand_1(char *parms, int opcd);
    void addr_mode_2(char *parms, long addr, int &lfref, addr_t &refaddr, int opcd);
    void addr_mode_3(char *parms/*, long addr*/, int opcd);
    void ldm_stm_regs_4(char *op, char *parms, int opcd);
};


DisARM cpu_ARM   ("ARM",    0, LITTLE_END, ADDR_32, '.', '$', ".byte", ".word", ".long");
DisARM cpu_ARM_LE("ARM-LE", 0, LITTLE_END, ADDR_32, '.', '$', ".byte", ".word", ".long");
DisARM cpu_ARM_BE("ARM-BE", 0, BIG_END,    ADDR_32, '.', '$', ".byte", ".word", ".long");


DisARM::DisARM(const char *name, int subtype, int endian, int addrwid,
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
    o_1_Shift,
    o_2_Shift,
    o_MOV,
    o_LDR_STR,
    o_Branch,
    o_LDM_STM,
    o_MCR_MRC,
    o_MRS,
    o_MSR,
    o_LDRH_STRH,
    o_MUL,
    o_BX,
};

static const struct InstrRec ARM_opcdTable[] =
{
    //   and         cmp       typ          lf ref op
    {0xFFFFFFFF, 0xE1A00000, o_Implied,   "nop"  , 0                },
    {0x0E1000F0, 0x000000B0, o_LDRH_STRH, "strh" , 0                },
    {0x0E1000F0, 0x001000B0, o_LDRH_STRH, "ldrh" , 0                },
    {0x0E1000F0, 0x001000D0, o_LDRH_STRH, "ldrsb", 0                },
    {0x0E1000F0, 0x001000F0, o_LDRH_STRH, "ldrsh", 0                },
    {0x0FE000F0, 0x00000090, o_MUL,       "mul"  , 0                },
    {0x0FE000F0, 0x00200090, o_MUL,       "mla"  , 0                },
    {0x0FE000F0, 0x00800090, o_MUL,       "umull", 0                },
    {0x0FE000F0, 0x00900090, o_MUL,       "umlal", 0                },
    {0x0FE000F0, 0x00C00090, o_MUL,       "smull", 0                },
    {0x0FE000F0, 0x00D00090, o_MUL,       "smlal", 0                },

    {0x0DE00000, 0x00A00000, o_2_Shift,   "adc"  , 0                },
    {0x0DE00000, 0x00800000, o_2_Shift,   "add"  , 0                },
    {0x0DE00000, 0x00000000, o_2_Shift,   "and"  , 0                },
    {0x0F000000, 0x0A000000, o_Branch,    "b"    , LFFLAG | REFFLAG | CODEREF},
    {0x0F000000, 0x0B000000, o_Branch,    "bl"   , REFFLAG | CODEREF},
    {0x0DE00000, 0x01C00000, o_2_Shift,   "bic"  , 0                },
    {0x0FF000F0, 0x01200010, o_BX,        "bx"   , LFFLAG           },
    {0x0DF00000, 0x01700000, o_1_Shift,   "cmn"  , 0                },
    {0x0DF00000, 0x01500000, o_1_Shift,   "cmp"  , 0                },
    {0x0DE00000, 0x00200000, o_2_Shift,   "eor"  , 0                },
    {0x0E500000, 0x08100000, o_LDM_STM,   "ldm"  , 0                },
    {0x0C500000, 0x04100000, o_LDR_STR,   "ldr"  , 0                },
    {0x0C500000, 0x04500000, o_LDR_STR,   "ldrb" , 0                },
    {0x0DE00000, 0x01A00000, o_MOV,       "mov"  , 0                },
    {0x0F100010, 0x0E000010, o_MCR_MRC,   "mcr"  , 0                },
    {0x0F100010, 0x0E100010, o_MCR_MRC,   "mrc"  , 0                },
    {0x0FB00000, 0x01000000, o_MRS,       "mrs"  , 0                },
    {0x0FB00000, 0x03200000, o_MSR,       "msr"  , 0                },
    {0x0FB000F0, 0x01200000, o_MSR,       "msr"  , 0                },
    {0x0DE00000, 0x01E00000, o_MOV,       "mvn"  , 0                },
    {0x0DE00000, 0x01800000, o_2_Shift,   "orr"  , 0                },
    {0x0DE00000, 0x00600000, o_2_Shift,   "rsb"  , 0                },
    {0x0DE00000, 0x00E00000, o_2_Shift,   "rsc"  , 0                },
    {0x0DE00000, 0x00C00000, o_2_Shift,   "sbc"  , 0                },
    {0x0E500000, 0x08000000, o_LDM_STM,   "stm"  , 0                },
    {0x0C500000, 0x04000000, o_LDR_STR,   "str"  , 0                },
    {0x0C500000, 0x04400000, o_LDR_STR,   "strb" , 0                },
    {0x0DE00000, 0x00400000, o_2_Shift,   "sub"  , 0                },
    {0x0DF00000, 0x01300000, o_1_Shift,   "teq"  , 0                },
    {0x0DF00000, 0x01100000, o_1_Shift,   "tst"  , 0                },
    {0x00000000, 0x00000000, o_Invalid,   ""     , 0                }
};

static const char *conds[] =
{
    "eq", "ne", "cs" /*hs*/, "cc" /*lo*/,
    "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt",
    "gt", "le", "al", "nv"
};

static const char *shifts[] =
{
    "lsl", "lsr", "asr", "ror"
};

static const char *regs[] =
{
    "r0",  "r1",  "r2",  "r3",
    "r4",  "r5",  "r6",  "r7",
    "r8",  "r9",  "r10", "r11",
    "r12", "sp",  "lr",  "pc"
};


addr_t DisARM::ARM_FetchLong(int addr, int &len)
{
    addr_t l;

    l = ReadLong(addr + len);
    len += 4;
    return l;
}


InstrPtr DisARM::ARM_FindInstr(uint32_t opcd)
{
    InstrPtr p;

    p = ARM_opcdTable;

    while (p->andWith && ((opcd & p->andWith) != p->cmpWith)) p++;

    return p;
}


void DisARM::shifter_operand_1(char *parms, int opcd)
{
    unsigned int val;
    int i;
    char s[256];

    if (opcd & (1 << 25)) {
        val = opcd & 0xFF;
        i = ((opcd >> 8) & 15) * 2;
        val = (val >> i) | (val << (32-i));
        sprintf(s, "#0x%X", val);
        strcat(parms, s);
#if 0
        sprintf(s, "%u", val);
        strcat(comnt, s);
#endif
        return;
    }

    if (opcd & (1 << 4)) {
        // register shift
        sprintf(s, "%s, %s %s", regs[opcd & 15], shifts[(opcd >> 5) & 3],
                                regs[(opcd >> 8) & 15]);
        strcat(parms, s);
        return;
    }

    // immediate shift
    switch (opcd & 0x0FF0) {
        case 0x0060:
            sprintf(s, "%d, RRX", opcd & 15);
            break;
        case 0x0000:
            sprintf(s, "%s", regs[opcd & 15]);
            break;
        default:
            sprintf(s, "%s, %s #%d", regs[opcd & 15], shifts[(opcd >> 5) & 3], (opcd >> 7) & 31);
            break;
    }
    strcat(parms, s);
    return;
}


void DisARM::addr_mode_2(char *parms, long addr, int &lfref, addr_t &refaddr, int opcd)
{
    char sign;
    int ofs;
    char s[256];
    addr_t ra;

    if (opcd & (1 << 23)) {
        sign = '+';
    } else {
        sign = '-';
    }

    if (((opcd >> 24) & 15) == 5) {
        // immediate offset/index
        if (((opcd >> 16) & 15) == 15) {
            ofs = opcd & 0xFFF;
            if (sign == '-') {
                ofs = -ofs;
            }
            ra = addr + 8 + ofs;
            RefStr(ra, s, lfref, refaddr);
//          strcat(parms, "[PC, #");
            strcat(parms, s);
//          strcat(parms, "-*-8]");
            return;
        }

        if ((opcd & 0xFFF) == 0) {
            sprintf(s, "[%s]", regs[(opcd >> 16) & 15]);
        } else if (opcd & (1 << 23)) {
            sprintf(s, "[%s, #0x%X]", regs[(opcd >> 16) & 15], opcd & 0xFFF);
        } else {
            sprintf(s, "[%s, #-0x%X]", regs[(opcd >> 16) & 15], opcd & 0xFFF);
        }
        strcat(parms, s);
#if 0
        if (opcd & 0xFFF) {
            if (sign == '-') {
                sprintf(comnt, "%d", -(opcd & 0xFFF));
            } else {
                sprintf(comnt, "%d",   opcd & 0xFFF);
            }
        }
#endif
        if (opcd & (1 << 21)) {
            strcat(parms, "!");
        }
        return;
    }

    if (((opcd >> 24) & 15) == 4) {
        // immediate post-indexed
        if ((opcd & 0xFFF) == 0) {
            sprintf(s, "[%s]", regs[(opcd >> 16) & 15]);
        } else if (opcd & (1 << 23)) {
            sprintf(s, "[%s], #0x%X",  regs[(opcd >> 16) & 15], opcd & 0xFFF);
        } else {
            sprintf(s, "[%s], #-0x%X", regs[(opcd >> 16) & 15], opcd & 0xFFF);
        }
        strcat(parms, s);
#if 0
        if (opcd & 0xFFF) {
            if (sign == '-') {
                sprintf(comnt, "%d", -(opcd & 0xFFF));
            } else {
                sprintf(comnt, "%d",   opcd & 0xFFF);
            }
        }
#endif
        return;
    }

    if ((opcd & 0xFF0) == 0) {
        // register offset/index
        if (opcd & (1 << 24)) {
            if (sign == '-')
                sprintf(s, "[%s, -%s]", regs[(opcd >> 16) & 15], regs[opcd & 15]);
            else
                sprintf(s, "[%s, %s]", regs[(opcd >> 16) & 15], regs[opcd & 15]);
            if (opcd & (1 << 21)) strcat(parms, "!");
        } else {
            if (sign == '-') {
                sprintf(s, "[%s], -%s", regs[(opcd >> 16) & 15], regs[opcd & 15]);
            } else {
                sprintf(s, "[%s], %s",  regs[(opcd >> 16) & 15], regs[opcd & 15]);
            }
        }
        strcat(parms, s);
        return;
    }

    if ((opcd & 0xFF0) == 0x060) {
        // RRX
        if (opcd & (1 << 24)) {
            if (sign == '-') {
                sprintf(s, "[%s, -%s, RRX]", regs[(opcd >> 16) & 15], regs[opcd & 15]);
            } else {
                sprintf(s, "[%s, %s, RRX]",  regs[(opcd >> 16) & 15], regs[opcd & 15]);
            }
            if (opcd & (9 << 21)) {
                strcat(parms, "!");
            }
        } else {
            if (sign == '-') {
                sprintf(s, "[%s], -%s, RRX", regs[(opcd >> 16) & 15], regs[opcd & 15]);
            } else {
                sprintf(s, "[%s], %s, RRX",  regs[(opcd >> 16) & 15], regs[opcd & 15]);
            }
        }
        strcat(parms, s);
        return;
    }

    // scaled register offset/index
    ofs = (opcd >> 7) & 31;
    if (ofs == 0) {
        ofs = 32;
    }
    if (opcd & (1 << 24)) {
        if (sign == '-') {
            sprintf(s, "[%s, -%s, %s #%d]", regs[(opcd >> 16) & 15], regs[opcd & 15],
                    shifts[(opcd >> 5) & 3], ofs);
        } else {
            sprintf(s, "[%s, %s, %s #%d]",  regs[(opcd >> 16) & 15], regs[opcd & 15],
                    shifts[(opcd >> 5) & 3], ofs);
        }
        if (opcd & (1 << 21)) {
            strcat(s, "!");
        }
    } else {
        if (sign == '-') {
            sprintf(s, "[%s], -%s, %s #%d", regs[(opcd >> 16) & 15], regs[opcd & 15],
                    shifts[(opcd >> 5) & 3], ofs);
        } else {
            sprintf(s, "[%s], %s, %s #%d",  regs[(opcd >> 16) & 15], regs[opcd & 15],
                    shifts[(opcd >> 5) & 3], ofs);
        }
    }
    strcat(parms, s);
}


void DisARM::addr_mode_3(char *parms/*, long addr*/, int opcd)
{
    char sign;
    int ofs;
    char s[256];

    if (opcd & (1 << 23)) {
        sign = '+';
    } else {
        sign = '-';
    }

    if (opcd & (1 << 22)) {
        // immediate offset/index
        ofs = ((opcd >> 4) & 0xF0) + (opcd & 0x0F);

        if (opcd & (1 << 24)) {
            if (sign == '-') {
                sprintf(s, "[%s, #-0x%X] ; -%d", regs[(opcd >> 16) & 15], ofs, ofs);
            } else {
                sprintf(s, "[%s, #0x%X] ; %d",   regs[(opcd >> 16) & 15], ofs, ofs);
            }
            strcat(parms, s);
            if (opcd & (1 << 21)) {
                strcat(parms, "!");
            }
            return;
        }
        if (sign == '-') {
            sprintf(s, "[%s], #-0x%X ; -%d", regs[(opcd >> 16) & 15], ofs, ofs);
        } else {
            sprintf(s, "[%s], #0x%X ; %d",   regs[(opcd >> 16) & 15], ofs, ofs);
        }
        strcat(parms, s);
        return;
    }

    // register offset/index
    if (opcd & (1 << 24)) {
        if (sign == '-') {
            sprintf(s, "[%s, -%s]", regs[(opcd >> 16) & 15], regs[opcd & 15]);
        } else {
            sprintf(s, "[%s, %s]",  regs[(opcd >> 16) & 15], regs[opcd & 15]);
        }
        if (opcd & (1 << 21)) {
            strcat(parms, "!");
        }
        strcat(parms, s);
        return;
    }
    if (sign == '-') {
        sprintf(s, "[%s], -%s", regs[(opcd >> 16) & 15], regs[opcd & 15]);
    } else {
        sprintf(s, "[%s], %s",  regs[(opcd >> 16) & 15], regs[opcd & 15]);
    }
    strcat(parms, s);
}


void DisARM::ldm_stm_regs_4(char *opcode, char *parms, int opcd)
{
    int i, j, n;

    if (opcd & (1 << 23)) {
        strcat(opcode, "i");
    } else {
        strcat(opcode, "d");
    }
    if (opcd & (1 << 24)) {
        strcat(opcode, "b");
    } else {
        strcat(opcode, "a");
    }

    strcat(parms, "{");

    n = 0;
    i = 0;
    while (i <= 15) {
        j = i+1;
        if (opcd & (1 << i)) {
            if (n++) {
                strcat(parms, ", ");
            }
            while (j <= 12 && (opcd & (1 << j))) {
                j++;
            }
            strcat(parms, regs[i]);
            if (j != i+1) {
                strcat(parms, "-");
                strcat(parms, regs[j-1]);
            }
        }
        i = j;
    }

    strcat(parms, "}");
}


int DisARM::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
//  addr_t      ad;
    uint32_t    opcd;
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
    opcd = ARM_FetchLong(addr, len);
    instr = ARM_FindInstr(opcd);

    if (instr && instr->typ && *(instr->op)) {
        invalid  = false;
        addcond  = true;
        len     = 4;
        strcpy(opcode, instr->op);
        lfref  = instr->lfref;

        switch (instr->typ) {
            case o_Implied:
                break;

            case o_Branch:
                i = opcd & 0x00FFFFFF;
                if (i & 0x00800000) {
                    i |= 0xFF000000;
                }
                ra = addr + i*4 + 8;
                RefStr(ra, parms, lfref, refaddr);
                if ((opcd >> 28) != 14) {
                    lfref &= ~LFFLAG;
                }
                break;

            case o_1_Shift:
                sprintf(parms, "%s, ", regs[(opcd >> 16) & 15]);
                shifter_operand_1(parms, opcd);
                if (((opcd >> 16) & 15) == 15 && (opcd >> 28) == 14) {
                    lfref |= LFFLAG;
                }
                break;

            case o_2_Shift:
                sprintf(parms, "%s, %s, ", regs[(opcd >> 12) & 15], regs[(opcd >> 16) & 15]);
                shifter_operand_1(parms, opcd);
                addflags = !!(opcd & 0x00100000);
                if (((opcd >> 12) & 15) == 15 && (opcd >> 28) == 14) {
                    lfref |= LFFLAG;
                }
                break;

            case o_MOV:
                sprintf(parms, "%s, ", regs[(opcd >> 12) & 15]);
                shifter_operand_1(parms, opcd);
                addflags = !!(opcd & 0x00100000);
                if (((opcd >> 12) & 15) == 15 && (opcd >> 28) == 14) {
                    lfref |= LFFLAG;
                }
                break;

            case o_LDR_STR:
                sprintf(parms, "%s, ", regs[(opcd >> 12) & 15]);
                addr_mode_2(parms, addr, lfref, refaddr, opcd);
                if (((opcd >> 12) & 15) == 15 && (opcd >> 28) == 14) {
                    lfref |= LFFLAG;
                }
                break;

            case o_LDRH_STRH:
                sprintf(parms, "%s, ", regs[(opcd >> 12) & 15]);
                addr_mode_3(parms/*, addr*/, opcd);
                if (((opcd >> 12) & 15) == 15 && (opcd >> 28) == 14) {
                    lfref |= LFFLAG;
                }
                break;

            case o_LDM_STM:
                if (opcd & (1 << 21)) {
                    sprintf(parms, "%s!, ", regs[(opcd >> 16) & 15]);
                } else {
                    sprintf(parms, "%s, " , regs[(opcd >> 16) & 15]);
                }
                ldm_stm_regs_4(opcode, parms, opcd);
                if (opcd & (1 << 22)) {
                    strcat(parms, "^");
                }
                if ((opcd & (1 << 15)) && ((opcd >> 28) == 14) &&
                    (opcd & (1 << 20)) ) {
                    lfref |= LFFLAG;
                }
                break;

            case o_MCR_MRC:
                if ((opcd >> 28) == 15) {
                    strcat(opcode, "2");
                    addcond = false;
                }
                if ((opcd >> 5) & 7) {
//                    sprintf(parms, "p%d, %d, %s, cr%d, cr%d, {%d}",
                    sprintf(parms, "p%d, %d, %s, cr%d, cr%d, %d",
                            (opcd >> 8) & 15, (opcd >> 21) & 7, regs[(opcd >> 12) & 15],
                            (opcd >> 16) & 15, opcd & 15, (opcd >> 5) & 7);
                } else {
                    sprintf(parms, "p%d, %d, %s, cr%d, cr%d",
                            (opcd >> 8) & 15, (opcd >> 21) & 7, regs[(opcd >> 12) & 15],
                            (opcd >> 16) & 15, opcd & 15);
                }
                break;

            case o_MRS:
                if (opcd & (1 << 22)) {
                    sprintf(parms, "%s, spsr", regs[(opcd >> 12) & 15]);
                } else {
                    sprintf(parms, "%s, cpsr", regs[(opcd >> 12) & 15]);
                }
                break;

            case o_MSR:
                if (opcd & (1 << 22)) {
                    strcpy(s, "spsr_");
                } else {
                    strcpy(s, "cpsr_");
                }
                if (opcd & (1 << 16)) {
                    strcat(s, "c");
                }
                if (opcd & (1 << 17)) {
                    strcat(s, "x");
                }
                if (opcd & (1 << 18)) {
                    strcat(s, "s");
                }
                if (opcd & (1 << 19)) {
                    strcat(s, "f");
                }
                if (opcd & (1 << 25)) {
                    sprintf(parms, "%s, #0x%X", s, opcd & 0xFF);
                } else {
                    sprintf(parms, "%s, %s", s, regs[opcd & 15]);
                }
                break;

            case o_MUL:
                sprintf(parms, "%s, %s, %s", regs[(opcd >> 16) & 15],
                            regs[opcd & 15], regs[(opcd >> 8) & 15]);
                addflags = !!(opcd & 0x00100000);
                break;

            case o_BX:
                sprintf(parms, "%s", regs[opcd & 15]);
                if ((opcd >> 28) != 14) {
                    lfref |= LFFLAG;
                }
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

    if (len) {
        i = opcd >> 28;
//if (i != 0x0E)
//printf("COND %d %s\n", opcd >> 28, conds[opcd >> 28]);
        if (addcond && i != 14) {
            strcat(opcode, conds[opcd >> 28]);
        }
        if (addflags) {
            strcat(opcode, "s");
        }
    }

    return len;
}
