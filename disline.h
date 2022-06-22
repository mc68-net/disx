// disline.h

#ifndef _DISLINE_H_
#define _DISLINE_H_

#include "disx.h"
#include "disstore.h"


// combination of addr and a sub-line to reference a dissassembly line
struct addrline_t
{
    addr_t      addr;   // address of this instruction
    uint8_t     line;   // line number within this address, starting at zero

    // constructors
    addrline_t() : addr(0), line(0) { }
    addrline_t(addr_t _addr, uint8_t _line = 0) : addr(_addr), line(_line) { }

    // compare two addrline_t for equality
    inline bool operator==(const addrline_t &a)
        { return addr == a.addr && line == a.line; }
    inline bool operator!=(const addrline_t &a)
        { return addr != a.addr || line != a.line; }

    void next_addr();   // advance to next addr
    void prev_addr();   // advance to prev addr
    void next_line();   // advance to next line
    void prev_line();   // advance to prev line

    // get number of display lines at this address
    int num_lines() const;

    // return data length of this line
    int get_len() const;

    // set to first instruction at <= addr
    void line_start(addr_t a);
};


class DisLine
{
public:
    enum {
        // tab columns in tabs[]
        T_ADDR   = 0, // listing address
        T_HEX    = 1, // object hex
        T_LABEL  = 2, // label
        T_OPCODE = 3, // opcode
        T_PARMS  = 4, // parameters
        T_CMT    = 5, // comments
        T_NTABS  = 6,
        T_SCRN   = 7, // flag if in screen mode

        // bits in line_cols
        B_ADDR   = 1 << T_ADDR,
        B_HEX    = 1 << T_HEX,
        B_LABEL  = 1 << T_LABEL,
        B_OPCODE = 1 << T_OPCODE,
        B_PARMS  = 1 << T_PARMS,
        B_SCRN   = 1 << T_SCRN,

        // column sets for various display modes
        SCRN_COLS = B_ADDR | B_HEX | B_LABEL | B_OPCODE | B_PARMS  | B_SCRN,
        LIST_COLS = B_ADDR | B_HEX | B_LABEL | B_OPCODE | B_PARMS,
        ASM_COLS  =                  B_LABEL | B_OPCODE | B_PARMS,

        // build_line flags
        BL_NOLABEL   = 1 << 0,  // do not show label
        BL_NOCOMMENT = 1 << 1,  // do not show comment
    };

    static int tabs[T_NTABS];   // values of tab positions
    static int line_cols;       // false to show hex bytes, true for asm source format
    static bool hard_tabs;      // true to put hard tabs in listing

public:
    void get_text(const addrline_t addr, char *s) const;

    // used to initialize disscrn parameters
    void get_start_addr(addrline_t &a) { a.addr = rom._base;     a.line = 0; }
    void get_end_addr  (addrline_t &a) { a.addr = rom.get_end(); a.line = 0; }

    // get the length of the instr at addr

    // make addr become code
    int make_code(addr_t addr, int &lfref, addr_t &refaddr);
    void build_line(addr_t addr, char *s, const char *opcode,
                    const char* parms, int ofs = 0, int flags = 0) const;

    int get_dis_line(addr_t addr, char *s, int &lfref, addr_t &refaddr) const;
    void dis_org(char *s) const;

    // create a text file with in asm or lst format
    void write_listing(const char *ext, bool do_asm = false);
    void write_asm(const char *ext) { write_listing(ext, true); }

private:
    char *add_tab(char *s, int column, bool forceblank = true) const;
};


extern DisLine disline;


#endif // _DISLINE_H_
