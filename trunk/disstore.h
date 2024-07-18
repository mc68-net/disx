// disstore.h
// storage for ROM image and attributes

#ifndef _DISSTORE_H_
#define _DISSTORE_H_

#include "disx.h"


// data types for _type
// note that embedded lengths are likely to be deprecated
// because ATTR_CONT flags can represent the line length
enum MemType {
    mData       = 0,    // raw data
//  mCode       = 1,    // deprecated in favor of multiple CPU types
    mByte       = 2,
    mWord       = 3,
    mLong       = 4,
    mAscii      = 5,
    mRWord      = 6,    // reverse-order word
    mRLong      = 7,    // reverse-order longword (not yet implemented)
    mHex        = 8,    // HEX pseudo-op
    mBin        = 9,    // binary bytes
    mBinX       = 10,   // big-endian X binary bytes
    mBinO       = 11,   // little-endian O binary bytes
    mEbcdic     = 15,   // EBCDIC text
    mOfs        = 16,   // word offset position-independent table entry
    mDecW       = 17,   // decimal word
    mDecL       = 18,   // decimal long
    mWord1      = 19,   // word address minus one (for 6502 jump tables)
    mCode       = 65,   // start of CPU types (alpha will look nice in the .ctl file)
};


enum LfRef {
    LFFLAG  = 0x01,     // this instruction does not continue to next byte
    REFFLAG = 0x02,     // this instruction has an address reference
    CODEREF = 0x04,     // the reference is also a code reference
    RIPSTOP = 0x08,     // this instruction is probably bogus
};


// attributes for _attr
enum MemAttr {
    ATTR_LNONE = 0x00,  // no label on this line
    ATTR_LDATA = 0x01,  // data label at this line
    ATTR_LCODE = 0x02,  // code label at this line
    ATTR_LXXXX = 0x03,  // X label at this line
    ATTR_LMASK = 0x03,  // mask for label bits
    ATTR_HMASK = 0x60,  // mask for hint bits
    ATTR_HSHFT = 5,     // shift for hint bits

    ATTR_LF0   = 0x04,  // LF before line (manually selected blank line)
    ATTR_LF1   = 0x08,  // LF after line (from disassembler's lfFlag)
    ATTR_NOREF = 0x10,  // do not turn references to this address into a label
    ATTR_HINT1 = 0x20,  // hint flags for individual disassemblers (these are used
    ATTR_HINT2 = 0x40,  // for things like 8042 bank select and 65C816 operand sizes)
    ATTR_CONT  = 0x80,  // continuation from previous address
};


class DisStore {
public:                 // attr bits
    size_t  _ofs;       // file offset to start of data (only used for save file)
    addr_t  _base;      // base address of ROM data (data array addr offset)
    size_t  _size;      // size of ROM data
    char    _fname[256];// file name from load_file
    bool    _changed;   // true if attr or type has been changed since last save

    uint8_t _defhint;   // default value for ATTR_HINT1 and ATTR_HINT2 (0..3)

    // Z-80 specific settings
    // placed here to not need to know if the disassembler even exists
    uint8_t _rst_xtra[8]; // extra bytes for each RST

private:
    uint8_t *_data;     // pointer to data storage
    uint8_t *_attr;     // pointer to attribute storage
    uint8_t *_type;     // pointer to type storage
    uint8_t *_undo_attr;// pointer to attribute storage
    uint8_t *_undo_type;// pointer to type storage
    uint8_t _wordsz;    // 0=8-bit, 1=16-bit word? (data array address scaling)

public:
    DisStore() : _base(0), _size(0), _data(NULL), _attr(NULL), _type(0), _wordsz(0) { };
    ~DisStore();
    int get_end() { return (int) _base + (int) _size; }
    int get_data(addr_t addr);
    int get_attr(addr_t addr);
    int get_hint(addr_t addr);
    int get_type(addr_t addr);

    void set_attr(addr_t addr, int attr);
    void set_hint(addr_t addr, int hint);
    void set_type(addr_t addr, int type);

    void set_attr_flag(addr_t addr, int attr);
    void clear_attr_flag(addr_t addr, int attr);
    void toggle_attr_flag(addr_t addr, int attr);
    bool test_attr(addr_t addr, int attr);

    int get_len(addr_t addr);
    addr_t get_instr_start(addr_t addr);
    int load_bin(const char *fname, addr_t ofs, addr_t size, addr_t base);
    void unload();

    // put an instruction of type type and length len at addr
    void set_instr(addr_t addr, int len, int type, bool lfflag = false);

    void save_undo();
    void swap_undo();

    bool AddrOutRange(addr_t addr);


    friend class DisSave;

private:
};


// global scope storage handling object
extern DisStore rom;


#endif // _DISSTORE_H_
