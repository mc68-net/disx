// disstore.cpp

#include "disstore.h"


// global storage handler object
DisStore rom;


// =====================================================
// destructor, free up everything
//  ~DisStore();
DisStore::~DisStore()
{
    unload();
}


// =====================================================
// unload everything to prepare for loading a new image
void DisStore::unload()
{
    _fname[0] = 0;
    _changed  = false;
    _ofs    = 0;
    _base   = 0;
    _size   = 0;
    _wordsz = 0;

    if (_data) {
        free(_data);
        _data = 0;
    }
    if (_attr) {
        free(_attr);
        _attr = 0;
    }
    if (_type) {
        free(_type);
        _type = 0;
    }
    if (_undo_attr) {
        free(_undo_attr);
        _undo_attr = 0;
    }
    if (_undo_type) {
        free(_undo_type);
        _undo_type = 0;
    }

    memset(_rst_xtra, 0, sizeof _rst_xtra);
}


// =====================================================
//  uint8_t get_data(addr_t addr);
int DisStore::get_data(addr_t addr)
{
    if (_data && addr >= _base && addr < get_end()) {
        if (_wordsz) {
            // ***FIXME: need to know endian here
            // this is big endian
            return ((_data[(addr - _base)*2    ] << 8) & 0xFF00) |
                    (_data[(addr - _base)*2 + 1]       & 0x00FF);
        } else {
            return _data[addr - _base];
        }
    }

    return 0;
}


// =====================================================
//  uint8_t get_attr(addr_t addr);
int DisStore::get_attr(addr_t addr)
{
    if (_attr && addr >= _base && addr < get_end()) {
        return _attr[addr - _base];
    }

    return 0;
}


// =====================================================
//  uint8_t get_type(addr_t addr);
int DisStore::get_type(addr_t addr)
{
    if (_type && addr >= _base && addr < get_end()) {
        return _type[addr - _base];
    }

    return 0;
}


// =====================================================
//  void set_attr(addr_t addr, int attr);
void DisStore::set_attr(addr_t addr, int attr)
{
    if (_attr && addr >= _base && addr < get_end()) {
        uint8_t *p = _attr + addr - _base;
        if (*p != attr) {
             *p = attr;
            _changed = true;
        }
    }
}


// =====================================================
//  void set_type(addr_t addr, int type);
void DisStore::set_type(addr_t addr, int type)
{
    if (_type && addr >= _base && addr < get_end()) {
        uint8_t *p = _type + addr - _base;
        if (*p != type) {
            *p = type;
            _changed = true;
        }
    }
}


// =====================================================
//  void set_attr_flag(addr_t addr, int attr);
void DisStore::set_attr_flag(addr_t addr, int attr)
{
    if (_attr && addr >= _base && addr < get_end()) {
        uint8_t *p = _attr + addr - _base;
        uint8_t a = *p | attr;
        if (*p != a) {
            *p = a;
            _changed = true;
        }
    }
}


// =====================================================
//  void clear_attr_flag(addr_t addr, int attr);
void DisStore::clear_attr_flag(addr_t addr, int attr)
{
    if (_attr && addr < get_end()) {
        uint8_t *p = _attr + addr - _base;
        uint8_t a = *p & ~attr;
        if (*p != a) {
            *p = a;
            _changed = true;
        }
    }
}


// =====================================================
//  void toggle_attr_flag(addr_t addr, int attr);
void DisStore::toggle_attr_flag(addr_t addr, int attr)
{
    if (_attr && addr < get_end()) {
        _attr[addr - _base] ^= attr;
        _changed = true;
    }
}


// =====================================================
//  void test_attr(addr_t addr, int attr);
//  returns true if any flags in attr are set
bool DisStore::test_attr(addr_t addr, int attr)
{
    if (_attr && addr >= _base && addr < get_end()) {
        return (_attr[addr - _base] & attr) != 0;
    }

    return false;
}


// =====================================================
// copy the current attr and type into the undo buffers
void DisStore::save_undo()
{
    if (_attr && _undo_attr) {
        memcpy(_undo_attr, _attr, _size);
    }

    if (_type && _undo_type) {
        memcpy(_undo_type, _type, _size);
    }
}


// =====================================================
// swap undo buffer
// the lazy way is just swap the pointers
void DisStore::swap_undo()
{
    uint8_t *t;

    // ***FIXME: add a second _changed flag and swap it too?
    _changed = true;

    t = _attr;
    _attr = _undo_attr;
    _undo_attr = t;

    t = _type;
    _type = _undo_type;
    _undo_type = t;
}


// =====================================================
void DisStore::set_instr(addr_t addr, int len, int type, bool lfflag)
// set_instr will handle setting all attr/type bytes
// and smash a following partial instruction back to mData
{
    // set first byte to type and lfflag
    clear_attr_flag(addr, ATTR_LF1);
    if (lfflag) {
        set_attr_flag(addr, ATTR_LF1);
    }
    set_type(addr++, type);

    // set following bytes (if any) to type/ATTR_CONT
    while (--len) {
        set_attr_flag(addr, ATTR_CONT);
        set_type(addr++, type);
    }

    // smash any partial instruction afterward
    while (addr < rom.get_end() && test_attr(addr, ATTR_CONT)) {
        clear_attr_flag(addr, ~(ATTR_LMASK|ATTR_NOREF)); // don't smash label flags
        set_type(addr++, mData);
    }
}


// =====================================================
int DisStore::get_len(addr_t addr)
{
    int len = 0; // note: this will return len == 0 if at end address

    // check for end of rom data
    for (addr_t a = addr; a != rom._base + rom._size; a++) {
        // check if first byte or ATTR_CONT set
        if (a == addr || test_attr(a, ATTR_CONT)) {
            len++;
        } else {
            break;
        }
    }

    return len;
}

// =====================================================
addr_t DisStore::get_instr_start(addr_t addr)
{
    // first check to see if it is in image range
    if (addr < _base) {
        addr = _base;
    } else
    if (addr > get_end()) {
        addr = get_end();
    } else {
        // if ATTR_CONT is set, keep going back
        while (addr >= _base && test_attr(addr, ATTR_CONT)) {
            addr--;
        }
    }

    return addr;
}


// =====================================================
//  int load_file(char *fname, addr_t ofs, addr_t size, addr_t base);
// returns -1 if error, else file size
int DisStore::load_bin(const char *fname, addr_t ofs, addr_t size, addr_t base)
{
    size_t n;

    // remove any previous image data
    unload();

    // open the file
    FILE *f = fopen(fname, "rb");
    if (!f) {
        goto error;
    }

    // save the file path
    strcpy(_fname, fname);

    // if image size not specified, use either file size or a specific maximum
    if (size == 0) {
#if 0
        size = 0x10000;
#else
        n = fseek(f, 0, SEEK_END);
        size = ftell(f);
        n = fseek(f, 0, SEEK_SET);
#endif
    }

    // Note: adding an extra safety byte to mallocs, just in case

    // allocate _data
    _data = (uint8_t *) malloc(size + 1);
    if (!_data) {
        goto error;
    }
    memset(_data, 0x00, size);

    // allocate _attr
    _attr = (uint8_t *) malloc(size + 1);
    if (! _attr) {
        goto error;
    }
    memset(_attr, 0x00, size);

    // allocate _type
    _type = (uint8_t *) malloc(size + 1);
    if (! _type) {
        goto error;
    }
    memset(_type, mData, size);

    // allocate _undo_attr and _undo_type
    _undo_attr = (uint8_t *) malloc(size + 1);
    if (! _undo_attr) {
        goto error;
    }
    _undo_type = (uint8_t *) malloc(size + 1);
    if (! _undo_type) {
        goto error;
    }
    save_undo();

    // skip ofs bytes
    if (ofs) {
        n = fseek(f, ofs, SEEK_SET);
    }

    // read data, error if zero bytes or more than requested
    // remaining data in file is ignored
    n = fread(_data, 1, size, f);
    if ((n == 0) || (n > size)) {
        goto error;
    }

    // close file
    fclose(f);
    f = NULL;

    // set base address, and image size to number of bytes read
    _ofs  = ofs;
    _base = base;
    _size = n;

    // Data loaded, return size of data loaded
    return (int) size;

    // -----------------------------------------------------
    // something went wrong, dispose of everything and return an error
error:
    // remove data storage
    unload();

    return -1;
}


// =====================================================
bool DisStore::AddrOutRange(addr_t addr)
{
    return addr - _base > _size;
//  return addr < _base || addr - _base > _size;
}


