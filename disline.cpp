// disline.cpp

#include "disline.h"

#include "discpu.h"
#include "discmt.h"
#include "ctype.h"


int DisLine::tabs[DisLine::T_NTABS-1] = {8, 16, 8, 8, 16};
int DisLine::line_cols = DisLine::SCRN_COLS;
bool DisLine::hard_tabs = true; // note: requires tabs[] to all be mults of 8


// =====================================================
// struct addrline_t
// =====================================================

// =====================================================
// advance to next addr no matter how many lines
void addrline_t::next_addr()
{
    if (addr < rom.get_end()) {
        addr++;
        while (addr < rom.get_end() && rom.test_attr(addr, ATTR_CONT)) {
            addr++;
        }
        line = 0;
    }
};


// =====================================================
// advance to previous addr no matter how many lines
void addrline_t::prev_addr()
{
    if (addr > rom._base) {
        addr--;
        while (addr > rom._base && rom.test_attr(addr, ATTR_CONT)) {
            addr--;
        }
        line = 0;
    }
};


// =====================================================
// advance to next line
void addrline_t::next_line()
{
    if (line >= num_lines() - 1) {
        addr++;
        while (addr < rom.get_end() && rom.test_attr(addr, ATTR_CONT)) {
            addr++;
        }
        line = 0;
    } else {
        line++;
    }
};


// =====================================================
// advance to previous line
void addrline_t::prev_line()
{
    if (line == 0) {
        // to handle multi-byte instructions, decrement until prev start
        addr--;
        while (addr >= rom._base && rom.test_attr(addr, ATTR_CONT)) {
            addr--;
        }
        line = num_lines() - 1;
    } else {
        line--;
    }
};


// =====================================================
// get number of lines at this address
// defaulting to 1 line for now
int addrline_t::num_lines() const
{
    // check through attributes to see if there are extra lines

    int n = 1;
    // blank line before the instruction (manually selected)
    if (rom.test_attr(addr, ATTR_LF0)) {
        n++;
    }

    // handle hidden labels (Lxxxx EQU $-1)
    for (int ofs = 1; ofs < rom.get_len(addr); ofs++) {
        if (rom.get_attr(addr + ofs) & ATTR_LMASK) {
            n++;
        }
    }

    // blank line after the instruction (from LF-flag in disassembly)
    if (rom.test_attr(addr, ATTR_LF1)) {
        n++;
    }

    return n;
}


// =====================================================
// get length of the "instruction" by checking CONT attributes
int addrline_t::get_len() const
{
    return rom.get_len(addr);
}


// =====================================================
//  int line_start(addrline_t &addrline);
// function to find the first valid addr that is <= the supplied addr
//  set addr to first instruction at address <= addr
void addrline_t::line_start(addr_t a)
{
    addr = rom.get_instr_start(a);
    line = 0;

    // if addr has ATTR_LF0, line = 1
    if (rom.test_attr(addr, ATTR_LF0)) {
        line = 1;
    }

    return;
}


// =====================================================
// class DisLine
// =====================================================

// declare the single instance
DisLine disline;


// =====================================================
// text for a combination of addr & subline
//  n is supposed to be the string size, but not yet used
//  addrline.line is also ignored
void DisLine::get_text(const addrline_t addr, char *s) const
{
    int ad = addr.addr;
    int ln = addr.line;

    if (ad >= rom.get_end()) {
        strcpy(s, "-- END --");
        return;
    }

    int lfref = 0;
    addr_t refaddr = 0;
    s[0] = 0; // default to blank line

    // check if LF-before
    if (rom.test_attr(ad, ATTR_LF0)) {
        // only on the first line
        if (ln == 0) {
            // if something other than blank, set it here
            return;
        }
        // count LF0 as having been checked
        ln--;
    }

    // check for main line of text
    if (ln-- == 0) {
        int len = get_dis_line(addr.addr, s, lfref, refaddr);
        // verify that next instruction is properly marked
        for (int i = 1; i < len; i++) {
            // set "continue" attribute for all successive bytes
            rom.set_attr_flag(addr.addr + i, ATTR_CONT);
        }
        if (len > 0) {
            // clear "continue" attribute for first byte of next instr
            rom.clear_attr_flag(addr.addr + len, ATTR_CONT);
        }
        return;
    }

    // check for label attribute lines
    int len = rom.get_len(ad);
    for (int ofs = 1; ofs < len; ofs++) {
        if (rom.get_attr(ad + ofs) & ATTR_LMASK) {
            if (ln-- == 0) {
                // found the right line
                char parms[16];
                sprintf(parms, "%c-%d", defCpu->_curpc, len - ofs);
                build_line(ad, s, "EQU", parms, ofs, /*flags=*/ BL_NOCOMMENT);
                return;
            }
        }
    }

    // check for LF-after
    if (/*ln == 0 &&*/ rom.test_attr(ad, ATTR_LF1)) {
        // if something other than blank, set it here
        return;
    }

    // unknown line number
    sprintf(s, "line=%d", addr.line);
}


// =====================================================
// get disassembly text line for the current CPU with address and labels
int DisLine::get_dis_line(const addr_t addr, char *s, int &lfref, addr_t &refaddr) const
{
    // storage for disassembly text
    // sizes are arbitrary and hopefully oversized
    char opcode[20] = {0};
    char parms[256] = {0};
    lfref = 0;
    refaddr = 0;

    // find the CPU for this addr, will return generic if not a CPU type
    CPU *cpu = CPU::get_cpu(rom.get_type(addr));

    // pass this line to the dissassembler
    int len = cpu->dis_line(addr, opcode, parms, lfref, refaddr);

    if (s) {
        build_line(addr, s, opcode, parms);
    }

    return len;
}


// =====================================================
// adds blanks or tab characters to a line
// returns pointer to the null at end of line
//
// the general effect should be that "too long" fields
// will attempt to compensate by removing blanks before later fields
//
// when hard tabs are used, all columns MUST be multiples of 8 characters!

char *DisLine::add_tab(char *s, int column, bool forceblank) const
{
    // sum all the enabled column widths including this one
    int n = 0;
    for (int i = 0; i <= column; i++) {
        if (line_cols & (1 << i)) {
            n += tabs[i];
        }
    }

    char *p;

    // do hard tabs mode if requested AND not in screen mode
    if (hard_tabs && !(line_cols & B_SCRN)) {
        // count current length of line including tabs
        int len = 0;
        for (p = s; *p; p++) {
            if (*p == '\t') {
                // determine number of chars to next tab stop
                len += (len % 8) ? 8 - (len % 8) : 8;
            } else {
                len++;
            }
        }

        // determine how many chars to end of field
        int x = n - len;
        if (x <= 0) {
            // if already past tabs, just add a single blank
            if (forceblank) {
                // force-append at least one blank
                *p++ = ' ';
                n++;
            }
        } else {
            // add tabs depending on needed chars / 8
            while (x > 0) {
                *p++ = '\t';
                x -= 8;
            }
        }
    } else {
        // get line length and point p to end of line
        int len = strlen(s);
        p = s + len;

        if (forceblank) {
            // force-append at least one blank
            *p++ = ' ';
            n++;
        }

        // append blanks to desired width
        for (int i = 0; i < n - len; i++) {
            *p++ = ' ';
        }
    }

    // add null terminator and return address of terminator
    *p = 0;
    return p;
}


// =====================================================
void DisLine::build_line(addr_t addr, char *s, const char *opcode,
                         const char* parms, int ofs, int flags) const
{
    char label[16] = {0};

    // get actual instruction length using attr flags
    int len = rom.get_len(addr);

    char *p = s;
    *p = 0;

// -----------------------------------------------------
// listing address

    if (line_cols & B_ADDR) {
        // add address
        if (defCpu->_addrwid == ADDR_16) {
            sprintf(p, "%.4X:", (unsigned) addr + ofs);
        } else {
            sprintf(p, "%.6X:", (unsigned) addr + ofs);
        }
        p += strlen(p);

        // append info character(s) if in screen mode
        if (line_cols & B_SCRN) {
             char c = 0;
             if (rom.test_attr(addr, ATTR_NOREF)) {
                 // ATTR_NOREF indicator
                 c = '-';
             }
             if (c) {
                 *p++ = c;
                 *p = 0;
             }
        }

        p = add_tab(s, T_ADDR, false);
    }

// -----------------------------------------------------
// object hex

    if (line_cols & B_HEX) {
        // add hex dump
        if (!ofs) { // only put hex on zero-offset lines
            // compute max hex bytes on line
            // note: this only works with even tab widths!
            int max = tabs[T_HEX] / 2 - 1;

            // put data bytes
            for (int i = 0; i < len; i++) {
                // if 7th out of 8 or more in a 16-char field
                if (i == max - 1 && len > max) {
                    strcat(p, "...");
                    break;
                }
                sprintf(p, "%.2X", rom.get_data(addr + i));
                p += strlen(p);
            }
        }

        p = add_tab(s, T_HEX, false);
    }

// -----------------------------------------------------
// label

    if (line_cols & (1 << T_LABEL)) {
        // add label of current address
        if (rom.get_attr(addr + ofs) & ATTR_LMASK) {
            generic.make_label(addr + ofs, label);
            strcat(label,":");
        }

        if (!(flags & BL_NOLABEL)) {
            strcat(p, label);
        }
        p = add_tab(s, T_LABEL, false);
    }

// -----------------------------------------------------
// opcode

    // put opcode
    if (line_cols & B_OPCODE) {
        strcat(p, opcode);
        p = add_tab(s, T_OPCODE, false);
    }

// -----------------------------------------------------
// parameters

// ***FIXME: need to handle lines too long after parms

    // put parms
    if (line_cols & B_PARMS) {
        strcat(p, parms);
        p = add_tab(s, T_PARMS, false);
    }

// -----------------------------------------------------
// comments

    if (!(flags & BL_NOCOMMENT)) {
        const char *comment = cmt.get_sym(addr);
        if (comment && comment[0]) {
            *p++ = ';';
            *p = 0;
            strcat(p, comment);
            p += strlen(p);
        }
    }

// -----------------------------------------------------
// trim any trailing whitespace (such as opcodes without parameters)

    while (p > s && isblank(p[-1])) {
        p--;
    }
//  *p++ = '|'; // to confirm that trailing whitespace really is gone
    *p = 0;
}


// =====================================================
// create an "ORG xxxxH" line for assembler output
void DisLine::dis_org(char *s) const
{
    // make operand parameter in proper format
    char parms[16];
    defCpu->H4Str(rom._base, parms);

    // build the ORG line
    build_line(rom._base, s, "ORG", parms, /*ofs=*/ 0, /*flags=*/ BL_NOLABEL | BL_NOCOMMENT);
}


// =====================================================
// make addr become code
// returns -1 if invalid code at addr
// returns 0 if nothing done
// returns 1 if valid opcode
// returns 2 if refaddr is a new code reference that should be traced
// returns 5 or 6 if lfflag (end of code stream)
// ***FIXME: most of the above is silly since lfref is being returned

int DisLine::make_code(addr_t addr, int &lfref, addr_t &refaddr)
{
    // first get the instruction length
    // (CPU type will need to be supplied in future versions)

    char opcode[20] = {0};
    char parms[256] = {0};
    lfref   = 0;
    refaddr = 0;

    // get instruction length for current CPU
    CPU *cpu = curCpu;
    int len = cpu->dis_line(addr, opcode, parms, lfref, refaddr);

    // is it an illegal instruction for the current CPU?
    if (len <= 0) {
        return -1;
    }

    // will it run past the end of the image?
    if (addr + len > rom.get_end()) {
        return -1;
    }

    // if it's already code for the current CPU, confirm instr len
    if (rom.get_type(addr) == cpu->_id) {
        // if instr len same, don't touch it, else it needs to be fixed
        if (len == rom.get_len(addr)) {
            return 0;
        }
    }

    // set the range of attribute bytes to type=cpu->_id
    // set_instr will handle setting all attr/type bytes
    // and smash a following partial instruction back to mData
    rom.set_instr(addr, len, cpu->_id, !!(lfref & LFFLAG));

    // now try to set the reference as a label
    if (refaddr && (lfref & REFFLAG)) {
        // skip if it's no-ref or already a label
        if (!rom.test_attr(refaddr, ATTR_NOREF) &&
           ((rom.get_attr(refaddr) & ATTR_LMASK)) == 0) {

            // if reference had no label but was already code for this CPU,
            // make it a code label
            if (!rom.test_attr(addr, ATTR_CONT) &&
                 rom.get_type(refaddr) == cpu->_id) {
                lfref |= CODEREF;
            }

            // set data or code reference
            if (lfref & CODEREF) {
                rom.clear_attr_flag(refaddr, ATTR_LDATA);
                rom.set_attr_flag(refaddr, ATTR_LCODE);
                // return code reference status for tracer
                return 2;
            } else {
                rom.set_attr_flag(refaddr, ATTR_LDATA);
                rom.clear_attr_flag(refaddr, ATTR_LCODE);
            }
        }
    }

    // return valid opcode status
    return 1;
}


// =====================================================
// create a text file with in asm or lst format
void DisLine::write_listing(const char *ext, bool do_asm)
{
    // save old listing format
    int old_cols = line_cols;

    if (do_asm) {
        // turn on assembler source format
        line_cols = ASM_COLS;
    } else {
        // set disassembly to listing format
        line_cols = LIST_COLS;
    }

    FILE *f;
    char newline[] = "\n";

    char s[256+21];
    strcpy(s, rom._fname);
    strcat(s, ".");
    if (ext) {
        strcat(s, ext);
    }

    // open the file for writing, text mode
    f = fopen(s, "w");

    if (do_asm) {
        // add "ORG xxxxH" and a blank line
        dis_org(s);
        fwrite(s, 1, strlen(s), f);
        fwrite(newline, 1, strlen(newline), f);
        fwrite(newline, 1, strlen(newline), f);
    }

    addrline_t addr(rom._base);
    addrline_t end(rom.get_end());
    while (addr != end) {
        get_text(addr, s);
        fwrite(s, 1, strlen(s), f);
        fwrite(newline, 1, strlen(newline), f);
        addr.next_line();
    }

    if (do_asm) {
        // add a blank line and an END opcode
        fwrite(newline, 1, strlen(newline), f);
        build_line(addr.addr, s, "END", "", /*ofs=*/ 0, /*flags=*/ BL_NOCOMMENT);
        fwrite(s, 1, strlen(s), f);
        fwrite(newline, 1, strlen(newline), f);
    }

    fclose(f);

    // restore old listing format
    line_cols = old_cols;
}

