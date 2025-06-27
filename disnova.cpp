// disnova.cpp

static const char versionName[] = "DG Nova disassembler";

#include "discpu.h"

struct InstrRec {
    uint16_t        andWith;
    uint16_t        cmpWith;
    int             typ;        // typ
    const char      *op;        // mnemonic
    uint16_t        lfref;      // lfFlag/refFlag/codeRef/etc.
};
typedef const struct InstrRec *InstrPtr;


class DisNova : public CPU {
public:
    DisNova(const char *name, int subtype, int endian, int addrwid,
           char curAddrChr, char hexChr, const char *byteOp,
           const char *wordOp, const char *longOp, const int radix);

    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);

private:
    uint16_t FetchWord(int addr, int &len);
    InstrPtr FindInstr(uint16_t opcd);
};


enum {
    CPU_Nova,
//    CPU_Nova3, // adds stack
};


DisNova cpu_Nova("Nova",    CPU_Nova, BIG_END, ADDR_16, '*', 'O', "DB", "DW", "DL", RAD_OCT16BE);
// occasionally it used hexadecimal
//DisNova cpu_NovaHex("NovaHex", CPU_Nova, BIG_END, ADDR_16, '*', 'O', "DB", "DW", "DL", RAD_HEX16BE);

DisNova::DisNova(const char *name, int subtype, int endian, int addrwid,
               char curAddrChr, char hexChr, const char *byteOp,
               const char *wordOp, const char *longOp, const int radix)
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
    _radix   = radix;
    _wordsize = WS_16BE;

    add_cpu();
}


// =====================================================


enum InstType {
    o_Invalid,      // 
    o_None,         // no operands      . . . .|. . . .|. . . .|. . . .
    o_OneAcc,       //                  x x x AC- ---op-- 0 0 0|0 0 0 1
    o_NoAccEA,	    // No-acc-EA        0 0 0 OP- @ IDX|[displacement-]
    o_OneAccEA,	    // One-acc-EA       0 0 0 AC- @ IDX|[displacement-]
    o_IOFormat,	    // IO-format        0 1 1 AC- OP- -F- [device-code]
    o_AriLog,	    // Arith/Logical    1 ACS ACD OP- SH- -C- #|[SKIP-]
    o_MAP,          //                  0 1 1 n|n 0 0 0|1 1 1 1|1 1 1 1
    o_IOTFormat,    //                  0 1 1 0|0 1 1 1|t t 1 1|1 1 1 1
    o_TRAP,         //                  1 s s d|d q q q|q q q q|1 0 0 0
};


static const struct InstrRec Nova_opcdTable[] =
{
    // and     cmp     typ            op       lfref
    { 0xF800, 0x0000, o_NoAccEA,  "JMP",     REFFLAG | CODEREF | LFFLAG}, // 0000 0ixx bbbb bbbb JMP (@)DISP(,IX)
    { 0xF800, 0x0800, o_NoAccEA,  "JSR",     REFFLAG | CODEREF}, // 0000 1ixx bbbb bbbb JSR (@)DISP(,IX)
    { 0xF800, 0x1000, o_NoAccEA,  "ISZ",     SKPBYTES(2) }, // 0001 0ixx bbbb bbbb ISZ	(@)DISP(,IX)
    { 0xF800, 0x1800, o_NoAccEA,  "DSZ",     SKPBYTES(2) }, // 0001 1ixx bbbb bbbb DSZ	(@)DISP(,IX)
    { 0xE000, 0x2000, o_OneAccEA, "LDA",     0 }, // 001a aixx bbbb bbbb LDA	AC,(@)DISP(,IX)
    { 0xE000, 0x4000, o_OneAccEA, "STA",     0 }, // 010a aixx bbbb bbbb STA	AC,(@)DISP(,IX)

    { 0xFFFF, 0x607F, o_None,     "INTEN",   0 }, // 0110 0000 0111 1111 INTEN	
    { 0xE7FF, 0x6001, o_OneAcc,   "MTFP",    0 }, // 011a a000 0000 0001 MTFP	AC
    { 0xE7FF, 0x6081, o_OneAcc,   "MFFP",    0 }, // 011a a000 1000 0001 MFFP	AC
    { 0xFFFF, 0x60BF, o_None,     "INTDS",   0 }, // 0110 0000 1011 1111 INTDS	
    { 0xE7FF, 0x60FF, o_MAP,      "MAP",     0 }, // 011n n000 1111 1111 MAP
    { 0xE7FF, 0x6201, o_OneAcc,   "MTSP",    0 }, // 011a a010 0000 0001 MTSP	AC
    { 0xE7FF, 0x6281, o_OneAcc,   "MFSP",    0 }, // 011a a010 1000 0001 MFSP	AC
//  { 0xE73F, 0x623F, o_IOFormat, "IORST",   0 }, // 011a a010 ff11 1111 IORST	
    { 0xFFFF, 0x6501, o_None,     "SAV",     0 }, // 0110 0101 0000 0001 SAV	
    { 0xFFFF, 0x6581, o_None,     "RET", LFFLAG}, // 0110 0101 1000 0001 RET	
    { 0xFFFF, 0x7641, o_None,     "DIV",     0 }, // 0111 0110 0100 0001 DIV	
    { 0xFFFF, 0x76C1, o_None,     "MUL",     0 }, // 0111 0110 1100 0001 MUL	

    { 0xFF00, 0x6000, o_IOFormat, "NIO",     0 }, // 0110 0000 ffpp pppp NIOf	DEV
    { 0xFF00, 0x6700, o_IOTFormat,"SKP",     SKPBYTES(2) }, // 0110 0111 tt11 1111 SKPT	DEV
    { 0xFF00, 0x6A00, o_IOFormat, "RTCDS",   0 }, // 0110 1010 ff11 1111 RTCDSf	
    { 0xFF00, 0x7200, o_IOFormat, "RTCEN",   0 }, // 0111 0010 ff11 1111 RTCENf	

    { 0xE700, 0x6100, o_IOFormat, "DIA",     0 }, // 011a a001 ffpp pppp DIAf	AC,DEV
    { 0xE700, 0x6200, o_IOFormat, "DOA",     0 }, // 011a a010 ffpp pppp DOAf	AC,DEV
    { 0xE7FF, 0x6301, o_OneAcc,   "PSHA",    0 }, // 011a a011 0000 0001 PSHA	AC
    { 0xE7FF, 0x6381, o_OneAcc,   "POPA",    0 }, // 011a a011 1000 0001 POPA	AC
    { 0xE73F, 0x633F, o_IOFormat, "INTA",    0 }, // 011a a011 ff11 1111 INTAf	AC
    { 0xE700, 0x6300, o_IOFormat, "DIB",     0 }, // 011a a011 ffpp pppp DIBf	AC,DEV
    { 0xE73F, 0x643F, o_IOFormat, "MSKO",    0 }, // 011a a100 ff11 1111 MSKOf	AC
    { 0xE700, 0x6400, o_IOFormat, "DOB",     0 }, // 011a a100 ffpp pppp DOAf	AC,DEV
    { 0xE700, 0x6500, o_IOFormat, "DIC",     0 }, // 011a a101 ffpp pppp DICf	AC,DEV
    { 0xE73F, 0x663F, o_IOFormat, "HALT",    0 }, // 011a a110 ff11 1111 HALTf	
    { 0xE700, 0x6600, o_IOFormat, "DOA",     0 }, // 011a a110 ffpp pppp DOAf	AC,DEV

    { 0x800F, 0x8008, o_TRAP,     "TRAP",    0 }, // 1ssd dqqq qqqq 1000 TRAP	AC,(@)DISP(,IX)

    { 0x8700, 0x8000, o_AriLog,   "COM",     0 }, // 1ssd d000 rrcc nwww COM(CS#) S,D(,SKCND)
    { 0x8700, 0x8100, o_AriLog,   "NEG",     0 }, // 1ssd d001 rrcc nwww NEG(CS#) S,D(,SKCND)
    { 0x8700, 0x8200, o_AriLog,   "MOV",     0 }, // 1ssd d010 rrcc nwww MOV(CS#) S,D(,SKCND)
    { 0x8700, 0x8300, o_AriLog,   "INC",     0 }, // 1ssd d011 rrcc nwww INC(CS#) S,D(,SKCND)
    { 0x8700, 0x8400, o_AriLog,   "ADC",     0 }, // 1ssd d100 rrcc nwww ADC(CS#) S,D(,SKCND)
    { 0x8700, 0x8500, o_AriLog,   "SUB",     0 }, // 1ssd d101 rrcc nwww SUB(CS#) S,D(,SKCND)
    { 0x8700, 0x8600, o_AriLog,   "ADD",     0 }, // 1ssd d110 rrcc nwww ADD(CS#) S,D(,SKCND)
    { 0x8700, 0x8700, o_AriLog,   "AND",     0 }, // 1ssd d111 rrcc nwww AND(CS#) S,D(,SKCND)

    { 0x0000, 0x0000, o_Invalid,  "",        0 }
};


static const char *devices[64] = {
    //   x0      x1      x2      x3      x4     x5     x6      x7
/*00*/ "",     "MDV",  "MMU",  "MMU1", "PAR", "",    "MCAT", "MCAR", // 02,03 also MAP,MAP1
/*10*/ "TTI",  "TTO",  "PTR",  "PTP",  "RTC", "PLT", "CDR",  "LPT",
/*20*/ "DSK",  "ADCV", "MTA",  "DACV", "DCM", "",    "",     "",
/*30*/ "QTY",  "IBM1", "IBM2", "DKP",  "CAS", "CRC", "IBP",  "IVT",  // 30 also SLA, 34 also MUX
/*40*/ "DPI",  "DPO",  "DIO",  "DIOT", "MXM", "",    "MCAT1","MCAR1",// 40,41 also SCR,SCT
/*50*/ "TTI1", "TTO1", "PTR1", "PTP1", "RTC1","PLT1","CDR1", "LPT1",
/*60*/ "DSK1", "ADCV1","MTA1", "DACV1","",    "",    "",     "",     // 64-66 alternate FPU location
/*70*/ "QTY1", "",     "",     "DKP1", "FPU1","FPU2","FPU",  "CPU"   // 70 also SLA1, 71-72 second IBM
};                                                                   // 74 also CAS1


uint16_t DisNova::FetchWord(int addr, int &len)
{
    uint16_t w;

    // read a word in big-endian order
    w = (ReadByte(addr + len) << 8) + ReadByte(addr + len + 1);
    len += 2;
    return w;
}


InstrPtr DisNova::FindInstr(uint16_t opcd)
{
    InstrPtr p;

    p = Nova_opcdTable;

    while (p->andWith && ((opcd & p->andWith) != p->cmpWith)) {
        p++;
    }

    return p;
}


int DisNova::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    int         n;
    char        s[256];
    addr_t      ra;
    const char  *p;

    opcode[0] = 0;
    parms[0] = 0;
    int len = 0;
    lfref   = 0;
    refaddr = 0;

    bool invalid = true;

    int opcd = FetchWord(addr, len);
    InstrPtr instr = FindInstr(opcd);
//    int hint = rom.get_hint(addr);

    if (!(addr & 0x01) &&       // require even alignment!
        instr && instr->typ && *(instr->op)) {
        invalid = false;
        strcpy(opcode, instr->op);
        lfref = instr->lfref;

        switch (instr->typ) {
            case o_None:        // - no operands    .... .... .... ....
                break;

            case o_OneAcc:      //                      x x x AC- ---op-- 0 0 0 0 0 0 1
                sprintf(parms, "%d", (opcd >> 11) & 3);
                break;

            case o_OneAccEA:	// One-acc-EA		0 0 0 AC- @ IDX [displacement-]
                sprintf(parms, "%d,", (opcd >> 11) & 3);
                // fallthrough

            case o_NoAccEA:	// No-acc-EA		0 0 0 OP- @ IDX [displacement-]
                if (opcd & 0x0400) {
                    strcat(parms, "@");
                    lfref |= INDWBE;
                }

                n = opcd & 0xFF;
                if (n > 127) {
                    n = n - 256;
                }
                parms += strlen(parms);

                switch ((opcd >> 8) & 3) {
                    default:
                    case 0: // zero page
                        RefStr2(opcd & 0xFF, s, lfref, refaddr);
                        sprintf(parms, "%s", s);
                        break;

                    case 2: // AC2+disp
                    case 3: // AC3+disp
                        if (n < 0) {
                            *parms++ = '-';
                            n = -n;
                        }
                        H2Str(n, s);
                        sprintf(parms, "%s,%d", s, (opcd >> 8) & 3);
                        break;

                    case 1: // PC+disp
                        ra = addr/2 + n;
                        RefStr(ra, parms, lfref, refaddr);
                }

                //strcat(parms, "...");
                break;

            case o_IOFormat:	// IO-format		0 1 1 AC- OP- -F- [device-code]
                switch ((opcd >> 6) & 3) {
                    default:
                    case 0: break;
                    case 1: strcat(opcode, "S"); break;
                    case 2: strcat(opcode, "C"); break;
                    case 3: strcat(opcode, "P"); break;
                }

                // note: don't show device parameter if it is zero
                sprintf(parms, "%d", (opcd >> 11) & 3);
                if (opcd & 0x3F) {
                    p = devices[opcd & 0x3F];
                    if (!p || !*p) {
                        H2Str(opcd & 0x3F, s);
                        p = s;
                    }
                    parms += strlen(parms);
                    sprintf(parms, ",%s", p);
                }
                break;

            case o_IOTFormat:
                switch ((opcd >> 6) & 3) {
                    default:
                    case 0: strcat(opcode, "BN"); break;
                    case 1: strcat(opcode, "BZ"); break;
                    case 2: strcat(opcode, "DN"); break;
                    case 3: strcat(opcode, "DZ"); break;
                }
                p = devices[opcd & 0x3F];
                if (!p || !*p) {
                    H2Str(opcd & 0x3F, s);
                    p = s;
                }
                sprintf(parms, "%s", p);
                break;

            case o_AriLog:	// Arith/Logical	1 ACS ACD OP- SH- -C- # [SKIP-]
                switch ((opcd >> 4) & 3) {
                    default:
                    case 0: break;
                    case 1: strcat(opcode, "Z"); break;
                    case 2: strcat(opcode, "O"); break;
                    case 3: strcat(opcode, "C"); break;
                }

                switch ((opcd >> 6) & 3) {
                    default:
                    case 0: break;
                    case 1: strcat(opcode, "L"); break;
                    case 2: strcat(opcode, "R"); break;
                    case 3: strcat(opcode, "S"); break;
                }

                if (opcd & 8) {
                    strcat(opcode, "#");
                }

                sprintf(parms, "%d,%d", (opcd >> 13) & 3, (opcd >> 11) & 3);

                if (opcd & 7) {
                    // add skip reference
                    lfref |= SKPBYTES(2);
                }
                switch (opcd & 7) {
                    default:
                    case 0: break;
                    case 1: strcat(parms, ",SKP"); lfref |= LFFLAG; break;
                    case 2: strcat(parms, ",SZC"); break;
                    case 3: strcat(parms, ",SNC"); break;
                    case 4: strcat(parms, ",SZR"); break;
                    case 5: strcat(parms, ",SNR"); break;
                    case 6: strcat(parms, ",SEZ"); break;
                    case 7: strcat(parms, ",SBN"); break;
                }
                break;

            case o_MAP:
                sprintf(parms, "%d", (opcd >> 11) & 3);
                break;

            case o_TRAP:
                H2Str((opcd >> 4) & 0x7F, s);
                sprintf(parms, "%d,%d,%s", (opcd >> 13) & 3, (opcd >> 11) & 3, s);
                break;

            default:
                invalid = true;
                break;
        }
    }

    if (invalid || rom.AddrOutRange(addr)) {
        strcpy(opcode, "???");
        sprintf(parms, "$%.4X", ReadWord(addr));
        len      = 0;
        lfref    = 0;
        refaddr  = 0;
    }

    // rip-stop checks
    if (opcode[0]) {
        int op = ReadWord(addr);
        switch (op) {
            case 0x0000: // repeated all zeros or all ones
            case 0xFFFF:
                if (ReadWord(addr+2) == op) {
                    lfref |= RIPSTOP;
                }
                break;
        }
    }

    return len;
}
