// discpu.cpp

#include "discpu.h"
#include "discmt.h"

// =====================================================
// class CPU
// =====================================================

CPU *curCpu = &generic;         // pointer to current CPU
CPU *defCpu = &generic;         // pointer to default CPU
CPU *cpu_list = NULL;           // pointer to CPU list
uint8_t next_cpu_id = mCode;    // next CPU ID to assign


// =====================================================
// register this disassembler in the list of disassemblers
void CPU::add_cpu()
{
//  curCpu = this;

    _id      = next_cpu_id++;

    // add new disassembler to the end of the linked list
    // so they will appear in registration order
    // (registration order will depend on link order of C++ constructors)
    if (!cpu_list) {
        // empty list
        cpu_list = this;
    } else {
        // find last item in list
        CPU *p = cpu_list;
        while (p->_next) {
            p = p->_next;
        }
        // add after last item
        p->_next = this;
    }
}


// =====================================================
void CPU::set_cur_cpu(CPU *cpu)
{
    cpu->set_cur_cpu();
}


// =====================================================
void CPU::set_cur_cpu()
{
    curCpu = this;

    if (defCpu == &generic) {
        set_def_cpu();
    }
}


// =====================================================
void CPU::set_def_cpu(CPU *cpu)
{
    cpu->set_def_cpu();
}


// =====================================================
void CPU::set_def_cpu()
{
    defCpu = this;

    // copy settings into generic
    generic._dbopcd  = _dbopcd;
    generic._dwopcd  = _dwopcd;
    generic._dlopcd  = _dlopcd;
    generic._drwopcd = _drwopcd;
    generic._endian  = _endian;
    generic._curpc   = _curpc;
    generic._hexchr  = _hexchr;
    generic._addrwid = _addrwid;
    generic._usefcc  = _usefcc;
}


// =====================================================
// find a disassembler in the list by name
class CPU *CPU::get_cpu(const char *s)
{
#if 0
    if (strcasecmp(s, curCpu->_name)) {
        return NULL;
    } else {
        return curCpu;
    }
#else
    for (class CPU *cpu = cpu_list; cpu; cpu = cpu->_next) {
        if (strcasecmp(s, cpu->_name) == 0) {
            return cpu;
        }
    }

    return NULL;
#endif
}


// =====================================================
// find a disassembler in the list by id
class CPU *CPU::get_cpu(int id)
{
    // if id is not for a disasssembler, use the generic disassembler
    if (id < mCode) {
        return &generic;
    }

    // find the CPU from the list
    // could probably cache the last used one before searching the list
#if 0
    if (id != curCpu->_id) {
        return NULL;
    } else {
        return curCpu;
    }
#else
    for (class CPU *cpu = cpu_list; cpu; cpu = cpu->_next) {
        if (id == cpu->_id) {
            return cpu;
        }
    }

    return NULL;
#endif
}


// =====================================================
// find a dissasembler in the list in order
// start with cpu = NULL, returns NULL at end of list
class CPU *CPU::next_cpu(class CPU *cpu)
{
#if 0
    // currently only supporting curCpu
    if (cpu) {
        return NULL;
    } else {
        return curCpu;
    }
#else
    if (cpu) {
        // return next CPU in list
        return cpu->_next;
    } else {
        // return start of list
        return cpu_list;
    }
#endif
}


// =====================================================
void CPU::show_list()
{
    const char *file = "";

    printf("Supported CPU types:");
    for (CPU *cpu = CPU::next_cpu(NULL); cpu; cpu = CPU::next_cpu(cpu)) {
        if (strcmp(file, cpu->_file)) {
            printf("\n");
            // print new file name
//          printf("%s: \"%s\"", cpu->_file, cpu->_version);
            printf("  %s:", cpu->_version);
            file = cpu->_file;
        }
//      printf("  %-8s - %s\n", cpu->_name, cpu->_version);
        printf(" %s", cpu->_name);
    }

    printf("\n");
}


// =====================================================
int CPU::ReadByte(addr_t addr) const
{
    return rom.get_data(addr++);
}


// =====================================================
int CPU::ReadWord(addr_t addr) const
{
    if (_endian) {
        return ReadWordBE(addr);
    } else {
        return ReadWordLE(addr);
    }
}


// =====================================================
int CPU::ReadRWord(addr_t addr) const
{
    if (_endian) {
        return ReadWordLE(addr);
    } else {
        return ReadWordBE(addr);
    }
}


// =====================================================
int CPU::ReadWordBE(addr_t addr) const
{
    return (rom.get_data(addr) << 8) | rom.get_data(addr+1);
}


// =====================================================
int CPU::ReadWordLE(addr_t addr) const
{
    return rom.get_data(addr) | (rom.get_data(addr+1) << 8);
}


// =====================================================
int CPU::ReadLong(addr_t addr) const
{
    if (_endian) {
        return ReadLongBE(addr);
    } else {
        return ReadLongLE(addr);
    }
}


// =====================================================
int CPU::ReadRLong(addr_t addr) const
{
    if (_endian) {
        return ReadLongLE(addr);
    } else {
        return ReadLongBE(addr);
    }
}


// =====================================================
int CPU::ReadLongBE(addr_t addr) const
{
    return (rom.get_data(addr)  << 24) | (rom.get_data(addr+1) << 16) |
           (rom.get_data(addr+2) << 8) |  rom.get_data(addr+3);
}


// =====================================================
int CPU::ReadLongLE(addr_t addr) const
{
    return rom.get_data(addr)          | (rom.get_data(addr+1) << 8) |
          (rom.get_data(addr+2) << 16) | (rom.get_data(addr+3) << 24);
}


// =====================================================
char *CPU::H2Str(uint8_t b, char *s) const
{
    if (_hexchr == '$') {
        sprintf(s, "$%.2X", b);
    } else {
        if (b > 0x9F) {
            sprintf(s, "0%.2XH", b);
        } else {
            sprintf(s, "%.2XH", b);
        }
    }
    return s;
}


// =====================================================
char *CPU::H4Str(uint16_t w, char *s) const
{
    if (_hexchr == '$') {
        sprintf(s, "$%.4X", w);
    } else {
        if (w > 0x9FFF) {
            sprintf(s, "0%.4XH", w);
        } else {
            sprintf(s, "%.4XH", w);
        }
    }
    return s;
}


// =====================================================
char *CPU::H6Str(uint32_t l, char *s) const
{
    if (_hexchr == '$') {
        sprintf(s, "$%.6X", l);
    } else {
        if (l > 0x9FFFFF) {
            sprintf(s, "0%.6XH", l);
        } else {
            sprintf(s, "%.6XH", l);
        }
    }
    return s;
}


// =====================================================
char *CPU::H8Str(uint32_t l, char *s) const
{
    if (_hexchr == '$') {
        sprintf(s, "$%.8X", l);
    } else {
        if (l > 0x9FFFFFFF) {
            sprintf(s, "0%.8XH", l);
        } else {
            sprintf(s, "%.8XH", l);
        }
    }
    return s;
}


// =====================================================
char *CPU::HxStr(uint32_t l, char *s) const
{
    if (l > 0x00FFFFFF) {
        return H8Str(l, s);
    } else if (rom._base > 0x0000FFFF) {
        return H6Str(l, s);
    } else {
        return H4Str(l, s);
    }
}


// =====================================================
bool CPU::ref_label(addr_t addr, char *s, int &lfref, addr_t &refaddr) const
{
    // common code for the various RefStr functions

    s[0] = 0;

    if (rom._base <= addr && addr <= rom.get_end() && addr != 0) {
        lfref |= REFFLAG;
        make_label(addr, s);
        refaddr = addr;
        // note that if the label wasn't actually flagged as a label,
        // make_label will leave the string empty, but refaddr will still be set
    }

    // check if equate defined at this address
    const char *str;
    if ((str = equ.get_sym(addr))) {
        strcpy(s, str);
    }

    return !s[0];
}

// =====================================================
char *CPU::RefStr2(addr_t addr, char *s, int &lfref, addr_t &refaddr) const
{
    s[0] = 0;

    if (ref_label(addr, s, lfref, refaddr)) {
        H2Str(addr, s);
    }

    return s;
}


// =====================================================
char *CPU::RefStr4(addr_t addr, char *s, int &lfref, addr_t &refaddr) const
{
    if (ref_label(addr, s, lfref, refaddr)) {
        H4Str(addr, s);
    }

    return s;
}


// =====================================================
char *CPU::RefStr6(addr_t addr, char *s, int &lfref, addr_t &refaddr) const
{
    if (ref_label(addr, s, lfref, refaddr)) {
        H6Str(addr, s);
    }

    return s;
}


// =====================================================
char *CPU::RefStr8(addr_t addr, char *s, int &lfref, addr_t &refaddr) const
{
    // check for an odd_code situation (odd addresses of Thumb code)
    // This is only done in RefStr8 simply because that's
    // the only place where it is needed.
    bool odd = is_odd_code(addr);
    if (odd) {
        addr--;
    }

    if (ref_label(addr, s, lfref, refaddr)) {
        H8Str(addr, s);
    }

    // append a trailing "+1" for the odd_code situation
    if (odd) {
        strcat(s, "+1");
    }

    return s;
}


// =====================================================
char *CPU::RefStr(addr_t addr, char *s, int &lfref, addr_t &refaddr) const
{
    switch(_addrwid) {
        default:
        case ADDR_16:
            return RefStr4(addr, s, lfref, refaddr);
        case ADDR_24:
            return RefStr6(addr, s, lfref, refaddr);
        case ADDR_32:
            return RefStr8(addr, s, lfref, refaddr);
    }

    return s;
}


// =====================================================
void CPU::make_label(addr_t addr, char *s) const
{
    s[0] = 0;
    char c = 0;

    // check if external symbol defined at this address
    const char *str;
    if ((str = sym.get_sym(addr))) {
        strcpy(s, str);
        return;
    }

    // get label type for this address
    switch(rom.get_attr(addr) & ATTR_LMASK) {
        default: // no label
        case ATTR_LNONE:
            return;

        case ATTR_LDATA:
            c = 'D';
            break;

        case ATTR_LCODE:
            c = 'L';
            break;

        case ATTR_LXXXX:
            c = 'X';
            break;
    }

    // concatenate address to label type
    switch (defCpu->_addrwid) {
        default:
        case ADDR_16:
            sprintf(s, "%c%.4X", c, (unsigned) addr);
            break;
        case ADDR_24:
        case ADDR_32:
            sprintf(s, "%c%.6X", c, (unsigned) addr);
            break;
    }
}


// =====================================================
// returns address of previous instruction, or zero if invalid
// (invalid means either before start of ROM or not from this CPU)
addr_t CPU::find_prev_instr(addr_t addr) const
{
    // have to go back at least one byte
    addr--;

    while (addr >= rom._base) {
        // look for previous instruction start
        if (!rom.test_attr(addr, ATTR_CONT)) {
            // ignore if wrong CPU
            if (rom.get_type(addr) != _id) {
                // it's not from this CPU
                return 0;
            }

            // found the previous instruction from this CPU!
            return addr;
        }
        // try next byte back
        addr--;
    }

    return 0; // was at beginning of rom area
}


// =====================================================
// returns address of previous label, or zero if none found
addr_t CPU::find_prev_label(addr_t addr) const
{
    while (addr >= rom._base) {
        if (rom.test_attr(addr, ATTR_LMASK)) {
            return addr;
        }
        addr--;
    }

    return 0; // previous label not found
}


// =====================================================
// class DisDefault
// =====================================================

DisDefault generic;


// =====================================================
DisDefault::DisDefault()
{
    _file    = __FILE__;
    _name    = "(none)";
    _version = "";
    _subtype = 0;
    _next    = NULL;
    _id      = 0;
    _dbopcd  = "DB";
    _dwopcd  = "DW";
    _dlopcd  = "DL";
    _curpc   = '$';
    _endian  = LITTLE_END;
    _hexchr  = 'H';
    _addrwid = ADDR_16; // should be using defCpu!
    _usefcc  = false;
}


// =====================================================
void DisDefault::byte_dis_line(addr_t addr, char *opcode, char *parms) const
{
    int len = rom.get_len(addr);

    char *p = parms;
    strcpy(opcode, _dbopcd);

    for (int i = 0; i < len; i++) {
        if (i) {
            p = stpcpy(p, ",");
        }
        H2Str(rom.get_data(addr++),p);
        p += strlen(p);

        // avoid buffer overflow the lazy way
        if (strlen(parms) > 200) {
            strcpy(p, "...");
            break;
        }
    }
}


// =====================================================
void DisDefault::word_dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr) const
{
    int len = rom.get_len(addr) / 2;
    bool w1 = rom.get_type(addr) == mWord1;

    char *p = parms;
    strcpy(opcode, _dwopcd);
    int w = ReadWord(addr);
    if (w1) {
        w++; // mWord1 reference address is w + 1
    }
    addr += 2;
    RefStr4(w, p, lfref, refaddr);
    p += strlen(p);
    if (w1) {
        strcat(p, "-1");
        p += 2;
    }

    for (int i = 0; i < len - 1; i++) {
        int w = ReadWord(addr);
        addr += 2;
        p = stpcpy(p, ",");
        H4Str(w, p);
        p += strlen(p);

        // avoid buffer overflow the lazy way
        if (strlen(parms) > 200) {
            strcpy(p, "...");
            break;
        }
    }
}


// =====================================================
void DisDefault::rword_dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr) const
{
    int len = rom.get_len(addr) / 2;

    char *p = parms;
    // use DW reverse opcode if specified
    if (_drwopcd) {
        strcpy(opcode, _drwopcd);
    } else {
        strcpy(opcode, _dwopcd);
        strcat(opcode, "*");
    }
    int w = ReadRWord(addr);
    addr += 2;
    RefStr4(w, p, lfref, refaddr);
    p += strlen(p);

    for (int i = 0; i < len - 1; i++) {
        int w = ReadRWord(addr);
        addr += 2;
        p = stpcpy(p, ",");
        H4Str(w, p);
        p += strlen(p);

        // avoid buffer overflow the lazy way
        if (strlen(parms) > 200) {
            strcpy(p, "...");
            break;
        }
    }
}


// =====================================================
void DisDefault::long_dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr) const
{
    int len = rom.get_len(addr) / 4;

    char *p = parms;
    strcpy(opcode, _dlopcd);
    uint32_t w = ReadLong(addr);
    addr += 4;
    RefStr8(w, p, lfref, refaddr);
    p += strlen(p);

    for (int i = 0; i < len - 1; i++) {
        uint32_t l = ReadLong(addr);
        addr += 4;
        p = stpcpy(p, ",");
        H8Str(l, p);
        p += strlen(p);

        // avoid buffer overflow the lazy way
        if (strlen(parms) > 200) {
            strcpy(p, "...");
            break;
        }
    }
}


// =====================================================
void DisDefault::rlong_dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr) const
{
    int len = rom.get_len(addr) / 4;

    char *p = parms;
    strcpy(opcode, _dlopcd);
    strcat(opcode, "*");
    uint32_t w = ReadRLong(addr);
    addr += 4;
    RefStr8(w, p, lfref, refaddr);
    p += strlen(p);

    for (int i = 0; i < len - 1; i++) {
        uint32_t l = ReadRLong(addr);
        addr += 4;
        p = stpcpy(p, ",");
        H8Str(l, p);
        p += strlen(p);

        // avoid buffer overflow the lazy way
        if (strlen(parms) > 200) {
            strcpy(p, "...");
            break;
        }
    }
}


// =====================================================
void DisDefault::decw_dis_line(addr_t addr, char *opcode, char *parms) const
{
    int len = rom.get_len(addr) / 2;

    char *p = parms;
    strcpy(opcode, _dwopcd);

    for (int i = 0; i < len; i++) {
        int w = ReadWord(addr);
        addr += 2;
        if (i) {
            p = stpcpy(p, ",");
        }
        sprintf(p, "%d", w);
        p += strlen(p);

        // avoid buffer overflow the lazy way
        if (strlen(parms) > 200) {
            strcpy(p, "...");
            break;
        }
    }
}


// =====================================================
void DisDefault::decl_dis_line(addr_t addr, char *opcode, char *parms) const
{
    int len = rom.get_len(addr) / 4;

    char *p = parms;
    strcpy(opcode, _dlopcd);

    for (int i = 0; i < len; i++) {
        uint32_t l = ReadLong(addr);
        addr += 4;
        if (i) {
            p = stpcpy(p, ",");
        }
        sprintf(p, "%d", l);
        p += strlen(p);

        // avoid buffer overflow the lazy way
        if (strlen(parms) > 200) {
            strcpy(p, "...");
            break;
        }
    }
}


// =====================================================
void DisDefault::ofs_dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr) const
{
    // if not exactly 2 bytes, disassemble as bytes and exit
    int len = rom.get_len(addr);
    if (len != 2) {
        byte_dis_line(addr, opcode, parms);
        return;
//printf(" *** addr=%.8X len=%d label not found ***\r\n", (int) addr, len); fflush(stdout); sleep(3);
    }

    // get previous label
    addr_t label = find_prev_label(addr);
    // no label found, disassemble as word and exit
    // note: this will also fail if the label is at address zero
    if (!addr) {
        word_dis_line(addr, opcode, parms, lfref, refaddr);
        return;
//printf(" *** addr=%.8X label=%.8X label not found ***\r\n", (int) addr, (int) label); fflush(stdout); sleep(3);
    }

    // get word in default endian
    int w = ReadWord(addr);
    addr += 2;

    // sign-extend word
    w = (int16_t) w;

    // create disassembly line "DW <label+w> - <label>"
    strcpy(opcode, _dwopcd);                    // DW
    char *p = parms;
    RefStr(label + w, p, lfref, refaddr);       // label+w
    p += strlen(p);
    *p++ = '-';                                 // -
    *p = 0;
    int nolfref;        // dummy to ignore ref of label
    addr_t norefaddr;
    RefStr(label, p, nolfref, norefaddr);       // label
//printf(" *** ! addr=%.8X '%s %s' ***\r\n", (int) addr, opcode, parms);
}


// =====================================================
void DisDefault::hex_dis_line(addr_t addr, char *opcode, char *parms) const
{
    int len = rom.get_len(addr);

    char *p = parms;
    strcpy(opcode, "HEX");
    for (int i = 0; i < len; i++) {
        if (i) {
            p = stpcpy(p, " ");
        }

        sprintf(p, "%.2X", ReadByte(addr++));
        p += strlen(p);

        // avoid buffer overflow the lazy way
        if (strlen(parms) > 200) {
            strcpy(p, "...");
            break;
        }
    }
}


// =====================================================
void DisDefault::bin_dis_line(addr_t addr, char *opcode, char *parms) const
{
    int len = rom.get_len(addr);

    char *p = parms;
    strcpy(opcode, _dbopcd);
    int typ = rom.get_type(addr);

    for (int i = 0; i < len; i++) {
        if (i) {
            p = stpcpy(p, ",");
        }

        int b = rom.get_data(addr++);
        switch (typ) {
           default:
           case mBin: // binary
                for (int bit = 0x80; bit; bit >>= 1) {
                            *p++ = b & bit ? '1' : '0';
                }
                *p++ = 'B';
                break;

           case mBinX: // big-endian X
                for (int bit = 0x80; bit; bit >>= 1) {
                    *p++ = b & bit ? 'X' : '_';
                }
                break;

           case mBinO: // little-endian O
                for (int bit = 0x01; bit <= 0x80; bit <<= 1) {
                    *p++ = b & bit ? 'O' : '_';
                }
                break;
        }
        *p = 0;

        // avoid buffer overflow the lazy way
        if (strlen(parms) > 200) {
            strcpy(p, "...");
            break;
        }
    }
}


// =====================================================
void DisDefault::asc_dis_line(addr_t addr, char *opcode, char *parms) const
{
    int len = rom.get_len(addr);

    enum {
        none,
        chr,
        byte
    };
    int last_typ = none;
    char quote = '"';

    char *p = parms;
    strcpy(opcode, _dbopcd);

    if (_usefcc) {
        // for Motorola style, use opcode FCC and change quote to '/'
        strcpy(opcode, "FCC");
        quote = '/';
    }

    for (int i = 0; i < len; i++) {
        int c = rom.get_data(addr + i);
        int typ = byte;
        if (' ' <= c && c <= '~' && c != quote && c != '\\') {
            typ = chr;
        }

        // put comma between changes and between bytes
        if ((last_typ != none && typ != last_typ) ||
            (last_typ == byte && typ == byte)) {
            // close the quote from the previous string
            if (last_typ == chr) {
                *p++ = quote;
                *p = 0;
            }

            // put comma between elements
            *p++ = ',';
            *p = 0;
        }

        if (last_typ != chr && typ == chr) {
            // open the quote for the new string
            if (typ == chr) {
                *p++ = quote;
                *p = 0;
            }
        }

        if (typ == chr) {
            *p++ = c;
            *p = 0;
        } else {
           // FCC must start with a delimiter character!
           if (_usefcc && i == 0) {
               *p++ = quote;
               *p++ = quote;
               *p++ = ',';
               *p = 0;
           }
#if 1 // display ASCII characters with high bit set as 'c'+80H
           if (' '+0x80 <= c && c <= '~'+0x80 && c != '\''+0x80
               && c != '\'' && c != '\\'+0x80) {
               // 'c'+
               sprintf(p, "'%c'+", c & 0x7F);
               c = 0x80;
               p += strlen(p);
           }
#endif
           H2Str(c, p);
        }
        p += strlen(p);

        // avoid buffer overflow the lazy way
        if (strlen(parms) > 200) {
            strcpy(p, "...");
            break;
        }

        last_typ = typ;
    }

    if (last_typ == chr) {
        *p++ = quote;
        *p = 0;
    }
}


// =====================================================
/* Index is EBCDIC 1047 code point; value is ASCII platform equivalent */
const uint8_t ebcdic_table[] = {
/*_0   _1   _2   _3   _4   _5   _6   _7   _8   _9   _A   _B   _C   _D   _E  _F*/
0x00,0x01,0x02,0x03,0x9C,0x09,0x86,0x7F,0x97,0x8D,0x8E,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x9D,0x0A,0x08,0x87,0x18,0x19,0x92,0x8F,0x1C,0x1D,0x1E,0x1F,
0x80,0x81,0x82,0x83,0x84,0x85,0x17,0x1B,0x88,0x89,0x8A,0x8B,0x8C,0x05,0x06,0x07,
0x90,0x91,0x16,0x93,0x94,0x95,0x96,0x04,0x98,0x99,0x9A,0x9B,0x14,0x15,0x9E,0x1A,
0x20,0xA0,0xE2,0xE4,0xE0,0xE1,0xE3,0xE5,0xE7,0xF1,0xA2,0x2E,0x3C,0x28,0x2B,0x7C,
0x26,0xE9,0xEA,0xEB,0xE8,0xED,0xEE,0xEF,0xEC,0xDF,0x21,0x24,0x2A,0x29,0x3B,0x5E,
0x2D,0x2F,0xC2,0xC4,0xC0,0xC1,0xC3,0xC5,0xC7,0xD1,0xA6,0x2C,0x25,0x5F,0x3E,0x3F,
0xF8,0xC9,0xCA,0xCB,0xC8,0xCD,0xCE,0xCF,0xCC,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
0xD8,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0xAB,0xBB,0xF0,0xFD,0xFE,0xB1,
0xB0,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0xAA,0xBA,0xE6,0xB8,0xC6,0xA4,
0xB5,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0xA1,0xBF,0xD0,0x5B,0xDE,0xAE,
0xAC,0xA3,0xA5,0xB7,0xA9,0xA7,0xB6,0xBC,0xBD,0xBE,0xDD,0xA8,0xAF,0x5D,0xB4,0xD7,
0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0xAD,0xF4,0xF6,0xF2,0xF3,0xF5,
0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0xB9,0xFB,0xFC,0xF9,0xFA,0xFF,
0x5C,0xF7,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0xB2,0xD4,0xD6,0xD2,0xD3,0xD5,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xB3,0xDB,0xDC,0xD9,0xDA,0x9F
/*_0   _1   _2   _3   _4   _5   _6   _7   _8   _9   _A   _B   _C   _D   _E  _F*/
};

void DisDefault::ebcdic_dis_line(addr_t addr, char *opcode, char *parms) const
{
    int len = rom.get_len(addr);

    enum {
        none,
        chr,
        byte
    };
    int last_typ = none;
    char quote = '"';

    char *p = parms;
    strcpy(opcode, "EBCDIC");

    for (int i = 0; i < len; i++) {
        int c = rom.get_data(addr + i);
        int asc = ebcdic_table[c];
        int typ = byte;
        // - if asc is in printable range use the ascii
        // - if not, use ebcdic hex byte
        if (' ' <= asc && asc <= '~' && asc != quote && asc != '\\') {
            typ = chr;
        }

        // put comma between changes and between bytes
        if ((last_typ != none && typ != last_typ) ||
            (last_typ == byte && typ == byte)) {
            // close the quote from the previous string
            if (last_typ == chr) {
                *p++ = quote;
                *p = 0;
            }

            // put comma between elements
            *p++ = ',';
            *p = 0;
        }

        if (last_typ != chr && typ == chr) {
            // open the quote for the new string
            if (typ == chr) {
                *p++ = quote;
                *p = 0;
            }
        }

        if (typ == chr) {
            *p++ = asc;
            *p = 0;
        } else {
           H2Str(c, p);
        }
        p += strlen(p);

        // avoid buffer overflow the lazy way
        if (strlen(parms) > 200) {
            strcpy(p, "...");
            break;
        }

        last_typ = typ;
    }

    if (last_typ == chr) {
        *p++ = quote;
        *p = 0;
    }
}


// =====================================================
void DisDefault::raw_dis_line(addr_t addr, char *opcode, char *parms) const
{
    strcpy(opcode, _dbopcd);
    H2Str(rom.get_data(addr), parms);

    // check if the multiple bytes are present
    if (rom.get_len(addr) > 1) {
        strcat(parms, ",...");
    }
}


// =====================================================
int DisDefault::dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr)
{
    lfref     = 0;
    refaddr   = 0;
    opcode[0] = 0;
    parms[0]  = 0;

    switch(rom.get_type(addr)) {
        case mByte:
            // bytes
            byte_dis_line(addr, opcode, parms);
            break;

        case mWord:
        case mWord1:
            // words
            word_dis_line(addr, opcode, parms, lfref, refaddr);
            break;

        case mLong:
            // longs
            long_dis_line(addr, opcode, parms, lfref, refaddr);
            break;

        case mRWord:
            // words
            rword_dis_line(addr, opcode, parms, lfref, refaddr);
            break;

        case mRLong:
            // longs
            rlong_dis_line(addr, opcode, parms, lfref, refaddr);
            break;

        case mDecW:
            // hex
            decw_dis_line(addr, opcode, parms);
            break;

        case mDecL:
            // hex
            decl_dis_line(addr, opcode, parms);
            break;

        case mOfs:
            // word offset position-independent table entry
            ofs_dis_line(addr, opcode, parms, lfref, refaddr);
            break;

        case mHex:
            // hex
            hex_dis_line(addr, opcode, parms);
            break;

        case mBin:
        case mBinX:
        case mBinO:
            // binary
            bin_dis_line(addr, opcode, parms);
            break;

        case mAscii:
            // hex
            asc_dis_line(addr, opcode, parms);
            break;

        case mEbcdic:
            // hex
            ebcdic_dis_line(addr, opcode, parms);
            break;

        case mData:
        default: // anything else (mCode should already go to a CPU)
            // unknown, use default
            raw_dis_line(addr, opcode, parms);
            break;
    }

    return rom.get_len(addr);
}


// =====================================================
// Determine if the address is an "odd_code" address.
// This is used for Thumb, where an odd address can refer
// to code at the even address, and the low bit is a flag
// indicating that it is Thumb code.
bool CPU::is_odd_code(addr_t addr)
{
    // if addr is even, return false
    if (!(addr & 1)) {
        return false;
    }

    // get instr type of addr
    int type = rom.get_type(addr);
    // if < mCode, return false
    if (type < mCode) {
        return false;
    }

    // addr or addr-1 must be a code label address
    if (!( ((rom.get_attr(addr-1) & ATTR_LMASK) == ATTR_LCODE) ||
           ((rom.get_attr(addr  ) & ATTR_LMASK) == ATTR_LCODE) )) {
        return false;
    }

    // look up the CPU from the type
    CPU *cpu = get_cpu(type);
    // ask the cpu if it supports oddcode
    return (cpu && cpu->has_odd_code());
}
