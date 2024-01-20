// discpu.h


#ifndef _DISCPU_H_
#define _DISCPU_H_

#include "disx.h"
#include "disstore.h"


// =====================================================
// base class with basic functionality for disassembly

enum {
    UNKNOWN_END = -1,
    LITTLE_END  = 0, // for isBigEnd
    BIG_END     = 1, // for isBigEnd

    ADDR_16     = 4, // for addrwid
    ADDR_24     = 6,
    ADDR_32     = 8,
};

class CPU {

public:
    const char *_file;    // file name of this disassembler
    const char *_version; // text name of this disassembler
    const char *_name;    // name of this CPU type
    uint8_t    _id;       // assigned ID

protected:
    uint8_t    _subtype;  // subtype for multiple-name disassemblers
    CPU        *_next;    // pointer to next disassembler in the list

    const char *_dbopcd;  // text for BYTE or DB or FCB etc.
    const char *_dwopcd;  // text for WORD or DW or FDB etc.
    const char *_dlopcd;  // text for LONG or DL etc.
    // more of these as needed...
    const char *_drwopcd; // text for reverse-endian DW
//  const char *_drlopcd; // text for reverse-endian DL

public:
    char    _curpc;       // character for . or * or $
    uint8_t _endian;      // =1 if big-endian (6800/68000), =0 if little endian
    char    _hexchr;      // $ for motorola style $FFFF, H for Intel style 0FFFFH
    uint8_t _addrwid;     // preferred address width, =4 for most 8-bit, =6 for 68000/10
    bool    _usefcc;      // use Motorola FCC pseudo-op

public:
    // disassemble an instruction and return the length in bytes, returns len or <=0 if invalid
    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr) = 0;

    void make_label(addr_t addr, char *s) const;

    static void show_list();

public:
    // find a disassembler in the list by name
    static CPU *get_cpu(const char *s);
    // find a disassembler in the list by id
    static CPU *get_cpu(int id);
    // find next dissasembler in the list
    static CPU *next_cpu(CPU *cpu);

    static void set_cur_cpu(CPU *cpu);
    void set_cur_cpu();

    static void set_def_cpu(CPU *cpu);
    void set_def_cpu();

// -----------------------------------------------------

protected:
    // register this disassembler in the list
    void add_cpu();

    // get memory data functions
    int ReadByte(addr_t addr) const;

    int ReadWord(addr_t addr) const; // in preferred endian for CPU
    int ReadRWord(addr_t addr) const; // in reverse endian for CPU
    int ReadWordBE(addr_t addr) const;
    int ReadWordLE(addr_t addr) const;

    int ReadLong(addr_t addr) const; // in preferred endian for CPU
    int ReadRLong(addr_t addr) const; // in reverse endian for CPU
    int ReadLongBE(addr_t addr) const;
    int ReadLongLE(addr_t addr) const;

    // print hex functions, returns byte after the string
    char *H2Str(uint8_t  b, char *s) const;
    char *H4Str(uint16_t w, char *s) const;
    char *H6Str(uint32_t l, char *s) const;
    char *H8Str(uint32_t l, char *s) const;

    // print reference to a label (if label exists) or just hex
    char *RefStr2(addr_t addr, char *s, int &lfref, addr_t &refaddr) const;
    char *RefStr4(addr_t addr, char *s, int &lfref, addr_t &refaddr) const;
    char *RefStr6(addr_t addr, char *s, int &lfref, addr_t &refaddr) const;
    char *RefStr8(addr_t addr, char *s, int &lfref, addr_t &refaddr) const;
    // this one looks at the preferred address width for the CPU
    char *RefStr (addr_t addr, char *s, int &lfref, addr_t &refaddr) const;

    addr_t find_prev_label(addr_t addr) const;
    addr_t find_prev_instr(addr_t addr) const;

    friend class DisSave; // for save_file() / load_file()
    friend class DisLine; // for dis_org()

    // returns true if this CPU can have 16-bit code addresses be odd (Thumb)
    virtual bool has_odd_code() { return false; }

    // returns true if the address is an odd_code address
    static bool is_odd_code(addr_t addr);
};

extern CPU *curCpu;             // pointer to current CPU
extern CPU *defCpu;             // pointer to default CPU
//extern CPU *cpu_list;         // pointer to CPU list (private)
extern uint8_t next_cpu_id;     // next CPU ID to assign


// =====================================================
// handles generic disassembly stuff like DB and DW
// also determines defaults based on curCpu

class DisDefault : public CPU {

public:
    DisDefault();
    virtual int dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr);
//    virtual int dis_len(addr_t addr, int &lfref);

private:
    void byte_dis_line(addr_t addr, char *opcode, char *parms) const;
    void word_dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr) const;
    void long_dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr) const;
    void rword_dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr) const;
    void rlong_dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr) const;
    void hex_dis_line(addr_t addr, char *opcode, char *parms) const;
    void bin_dis_line(addr_t addr, char *opcode, char *parms) const;
    void asc_dis_line(addr_t addr, char *opcode, char *parms) const;
    void ebcdic_dis_line(addr_t addr, char *opcode, char *parms) const;
    void decw_dis_line(addr_t addr, char *opcode, char *parms) const;
    void decl_dis_line(addr_t addr, char *opcode, char *parms) const;
    void ofs_dis_line(addr_t addr, char *opcode, char *parms, int &lfref, addr_t &refaddr) const;
    void raw_dis_line(addr_t addr, char *opcode, char *parms) const;

};


extern DisDefault generic;



#endif // _DISCPU_H_
