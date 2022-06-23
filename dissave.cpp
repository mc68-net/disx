// dissave.cpp

#include "dissave.h"

#include "disstore.h"
#include "disscrn.h"
#include "discpu.h"
#include "discmt.h"
#include "rle.h"

#define MAGIC "dsx4"
#define CRLF "\r\n\e[0K"
//#define DBG


// ***FIXME: Yes, I know that I'm leaving a lot of potential bugs
//    with unchecked file and memory errors. I'll get around to fixing
//    them someday, but it's tedium that I don't want right now.


// =====================================================
// writes a longword in big-endian order
void DisSave::write_long(FILE *f, int data) const
{
    uint8_t buf[4];
    buf[0] = (data >> 24);
    buf[1] = (data >> 16);
    buf[2] = (data >> 8);
    buf[3] = data;
    fwrite(buf, 1, 4, f);
}


// =====================================================
int DisSave::read_long(FILE *f) const
// reads a longword in big-endian order
{
    int n; (void) n; // disabling "fread return value ignored" warnings for now

    uint8_t buf[4] = {0};
    n = fread(buf, 1, 4, f);
#ifdef DBG
printf("  read_long: %.2X %.2X %.2X %.2X  " CRLF, buf[0], buf[1], buf[2], buf[3]);
#endif
    return (buf[0] << 24) | (buf[1] << 16) |
           (buf[2] << 8)  |  buf[3];
}


// =====================================================
int DisSave::save_block(FILE *f, const char *type, const void *data, int len)
{
    // write block type, 4 bytes
    fwrite(type, 1, 4, f);

    // write block data size, 4 bytes
    write_long(f, len);

    // write data, len bytes
    if (len) {
        fwrite(data, 1, len, f);
    }

    // mark data as saved
    rom._changed = false;

    return 0;
}


// =====================================================
void *DisSave::load_block(FILE *f, char *type, int &len)
{
    int n; (void) n; // disabling "fread return value ignored" warnings for now

    // read block type, 4 bytes
    type[4] = 0;
    n = fread(type, 1, 4, f);

    // read block size, 4 bytes
    len = read_long(f);

    // create pointer
    void *p = (uint8_t *) malloc(len);

    // read data, len bytes
    if (p) {
        n = fread(p, 1, len, f);
    }

    return p;
}


// =====================================================
int DisSave::save_file()
{
#ifdef DBG
//printf("  DisSave::save_file  " CRLF); fflush(stdout);
#endif

#if 0
    printf("  curCpu = '%s'  " CRLF, curCpu->_name);
    for (class CPU *cpu = CPU::next_cpu(NULL); cpu; cpu = CPU::next_cpu(cpu)) {
        printf("  CPU #%d = '%s'  " CRLF, cpu->_id, cpu->_name);
    }
    fflush(stdout);
    sleep(5);
#endif

    char path[256] = {0};

    strcpy(path, rom._fname);
    strcat(path, ".ctl");

    // open file for binary write
    FILE *f = fopen(path, "w");
    if (!f) {
        return -1;
    }

    // write magic number
    fwrite(MAGIC, 1, 4, f);

    // write "file" block   
    //   xx xx xx xx ofs
    //   xx xx xx xx base
    //   xx xx xx xx size
    //   .. .. .. 00 fname 

    fwrite("file", 1, 4, f);
    write_long(f, strlen(rom._fname) + 13); // 13 = 3x4 + fname zero terminator
    write_long(f, rom._ofs);
    write_long(f, rom._base);
    write_long(f, rom._size);
    fwrite(rom._fname, 1, strlen(rom._fname) + 1, f);

    // write "attr" and "type" blocks
    // note: only write one type or another, not both!
#if 0 // must save either raw or RLE, not both!
    // uncompressed attr/type blocks
    save_block(f, "attr", rom._attr, rom._size);
    save_block(f, "type", rom._type, rom._size);
#else
    // RLE commpressed "attR" block
    int n = rle_enc(rom._attr, rom._size, NULL);// determine size of encoded buffer
    char *buf = (char *) malloc(n);             // allocate encoded buffer
    n = rle_enc(rom._attr, rom._size, buf);     // encode buffer
    save_block(f, "attR", buf, n);              // write buffer
    free(buf);                                  // free buffer

    // RLE commpressed "typR" block
    n = rle_enc(rom._type, rom._size, NULL);    // determine size of encoded buffer
    buf = (char *) malloc(n);                   // allocate encoded buffer
    n = rle_enc(rom._type, rom._size, buf);     // encode buffer
    save_block(f, "typR", buf, n);              // write buffer
    free(buf);                                  // free buffer
#endif

    // write "rst " block
    save_block(f, "rst ", rom._rst_xtra, sizeof rom._rst_xtra);

    // write 'cpu ' block
    // first determine block size
    size_t len = strlen(defCpu->_name) + 2;
    len += strlen(curCpu->_name) + 2;
    for (class CPU *cpu = CPU::next_cpu(NULL); cpu; cpu = CPU::next_cpu(cpu)) {
        len += strlen(cpu->_name) + 2;
    }

    // create storage for CPU name block, and add curCpu
    char *block = (char *) malloc(len);
    char *p = block;

    // save defCpu as ID = 0
    *p++ = 0; // ID = 0
    if (defCpu != &generic) {
        strcpy(p, defCpu->_name);
    } else {
        strcpy(p, curCpu->_name);
    }
    p += strlen(p) + 1;

    // save curCpu as ID = 1
    *p++ = 1; // ID = 1
    strcpy(p, curCpu->_name);
    p += strlen(p) + 1;

    // add CPU list
    for (class CPU *cpu = CPU::next_cpu(NULL); cpu; cpu = CPU::next_cpu(cpu)) {
        *p++ = cpu->_id;
        strcpy(p, cpu->_name);
        p += strlen(p) + 1;
    }

    if (p - block != len) {
        printf("  expected len=%ld, got %ld  " CRLF, len, p - block);
        fflush(stdout);
        sleep(5);
    }

    // write the block
    save_block(f, "cpu ", block, len);

    free(block);

    // write "tabs" block
    // one byte per tab stop, currently five tab stops
    fwrite("tabs", 1, 4, f);
    write_long(f, DisLine::T_NTABS-1);
    for (int i = 0; i < DisLine::T_NTABS-1; i++) {
        char c = DisLine::tabs[i];
        fwrite(&c, 1, 1, f);
    }

    // write "view" block
    //   xx xx xx xx sel addr
    //   00 00 00 xx sel line
    //   xx xx xx xx top addr
    //   00 00 00 xx top line
    fwrite("view", 1, 4, f);
    write_long(f, 16);
    write_long(f, scrn._sel.addr);
    write_long(f, scrn._sel.line);
    write_long(f, scrn._top.addr);
    write_long(f, scrn._top.line);

    // write "end" block
    save_block(f, "end ", NULL, 0);

    // close file
    fclose(f);

    // write comments
    strcpy(path, rom._fname);
    strcat(path, ".cmt");
    cmt.save_comments(path);

    return 0;
}


// =====================================================
// note that parameters are ignored if a .ctl file is found
int DisSave::load_file(const char *fname, addr_t ofs, addr_t base, addr_t size, int force)
{
    int n;

#ifdef DBG
//printf("  DisSave::load_file  " CRLF); fflush(stdout);
#endif

//printf("  ofs = %.4X  base = %.4X  size = %.4X  " CRLF, (unsigned) ofs, (unsigned) size, (unsigned) base); fflush(stdout); sleep(2);

#if 0
    printf("  curCpu = '%s'  " CRLF, curCpu->_name);
    for (class CPU *cpu = CPU::next_cpu(NULL); cpu; cpu = CPU::next_cpu(cpu)) {
        printf("  CPU #%d = '%s'  " CRLF, cpu->_id, cpu->_name);
    }
    fflush(stdout);
    //sleep(5);
#endif

    char path[256] = {0};
    char bin_name[256] = {0};
    char err[256] = {0};

    // try to open "fname.ctl"
    strcpy(path, fname);
    strcat(path, ".ctl");

#ifdef DBG
//printf("  opening file %s  " CRLF, path); fflush(stdout);
#endif

    FILE *f = NULL;
    if (!(force & FORCE_FNAME)) {
        // open file for binary read
        f = fopen(path, "r");
    }
    if (!f) { // unable to open file
        // if .ctl file not found, try to open image using specified parameters
        if (fname[0] && rom.load_bin(fname, ofs, size, base) < 0) {
            sprintf(err, "error loading binary file %s", fname);
            scrn.error(err);
            return -1;
        }
        // successful load without .ctl file
        disline.get_start_addr(scrn._sel);
        scrn._top = scrn._sel;
        return 0;
    }

    // .ctl file found and opened as 'f'
    // note that file name in the .ctl info will be used instead

    // read and confirm magic number
    // read magic, 4 bytes
    // ---------------------

    char magic[4];
    n = fread(magic, 1, 4, f);

#ifdef DBG
//printf("  magic number expected '%4s' found '%4s'  " CRLF, MAGIC, magic); fflush(stdout);
#endif

    if (strncmp(MAGIC, magic, 4)) {
        scrn.error("incorrect magic number");
        goto ERROR;
    }

    // note that errors after this point need to unload all data on failure

    // start reading blocks
    while (1) {
#ifdef DBG
//int pos = ftell(f);
#endif

        // read next block header
        // (4 byte tag and 4 byte length)
        // ---------------------

        char tag[4];
        if (fread(tag, 1, 4, f) < 4) {
            scrn.error("unexpected end of file");
            break;
        }
        size_t len = read_long(f);

#ifdef DBG
//printf("  at %.4X got tag '%4s' len %.8X  " CRLF, pos, tag, (unsigned) len); fflush(stdout);
#endif

        // process the block
        // ---------------------

        if (strncmp("end ", tag, 4) == 0) {     // 'end ' block
            // end of data, success!
            break;
        } else

        // ---------------------
        // file info block

        if (strncmp("file", tag, 4) == 0) {     // 'file' block
            // get saved parameters if not overridden
            n = read_long(f);
            if (!(force & FORCE_OFS)) {
                ofs = n;
            }
            n = read_long(f);
            if (!(force & FORCE_BASE)) {
                base = n;
            }
            n = read_long(f);
            if (!(force & FORCE_SIZE)) {
                size = n;
            }
            n = fread(bin_name, 1, len - 12, f);
#ifdef DBG
//printf("  bin_name = '%s'  " CRLF, bin_name);
#endif
            // try to open binary with rom.load_bin
            if (rom.load_bin(fname, ofs, size, base) < 0) {

                // unable to load binary, check if filename in .ctl was not expected

                // report problem if bin_name != fname
                if (strcmp(fname, bin_name) != 0) {
                    // report that file didn't load but name was wrong too
                    scrn.error(".ctl file contains different binary filename!");
                    break;
                }

                scrn.error("error loading binary file");
                break;
            }

            // error if incorrect size
            if (size && size != rom._size) {
                scrn.error("incorrect binary file size");
                break;
            }

            // set default values for _top and _sel
            disline.get_start_addr(scrn._sel);
            scrn._top = scrn._sel;
        } else

        // ---------------------
        // old uncompressed 'attr' block

        if (strncmp("attr", tag, 4) == 0) {     // 'attr' block (must be after 'file')
            // error if file block not yet seen
            if (!bin_name[0]) {
                // ERROR
            }

// *** FIXME: (note: 'attr' tag is deprecated)
// if len <= rom.size, just read len bytes, the rest should be zero
// if len > rom.size, read len bytes, then read (rom._size - len) more bytes

            // error if incorrect size
            if (len > rom._size) {
                // ERROR
            }

            // read into rom._attr
            n = fread(rom._attr, 1, len, f);
        } else

        // ---------------------
        // old uncompressed 'type' block

        if (strncmp("type", tag, 4) == 0) {     // 'type' block (must be after 'file')
            // error if file block not yet seen
            if (!bin_name[0]) {
                // ERROR
            }

// *** FIXME: (note: 'type' tag is deprecated)
// if len <= rom.size, just read len bytes, the rest should be zero
// if len > rom.size, read len bytes, then read (rom._size - len) more bytes

            // error if incorrect size
            if (len > rom._size) {
                // ERROR
            }

            // read into rom._type
            n = fread(rom._type, 1, len, f);
        } else

        // ---------------------
        // RLE commpressed "attR" block

        if (strncmp("attR", tag, 4) == 0) {     // 'ATTR' block (must be after 'file')
            // error if file block not yet seen
            if (!bin_name[0]) {
                scrn.error("'ATTR' block found before 'file' block");
                break;
            }

            // allocate encoded buffer
            char *enc = (char *) malloc(len);
            // read encoded buffer
            n = fread(enc, 1, len, f);
            // determine decoded size
            int n = rle_dec(enc, len, NULL);
            // special case if decoded size is larger than rom._size
            if (n > rom._size) {
                char *tmp = (char *) malloc(n);
                // decode into temp buffer
                rle_dec(enc, len, tmp);
                // copy temp to rom._attr
                memcpy(rom._attr, tmp, rom._size);
                // free temporary buffer
                free(tmp);
            } else {
                // decode buffer into rom._attr
                rle_dec(enc, len, rom._attr);
            }
            // free encoded buffer
            free(enc);
        } else

        // ---------------------
        // RLE commpressed "typR" block

        if (strncmp("typR", tag, 4) == 0) {     // 'TYPE' block (must be after 'file')
            // error if file block not yet seen
            if (!bin_name[0]) {
                scrn.error("'TYPE' block found before 'file' block");
            }

            // allocate encoded buffer
            char *enc = (char *) malloc(len);
            // read encoded buffer
            n = fread(enc, 1, len, f);
            // determine decoded size
            int n = rle_dec(enc, len, NULL);
            // special case if decoded size is larger than rom._size
            if (n > rom._size) {
                char *tmp = (char *) malloc(n);
                // decode into temp buffer
                rle_dec(enc, len, tmp);
                // copy temp to rom._type
                memcpy(rom._type, tmp, rom._size);
                // free temporary buffer
                free(tmp);
            } else {
                // decode buffer into rom._type
                rle_dec(enc, len, rom._type);
            }
            // free encoded buffer
            free(enc);
        } else

        // ---------------------
        // Z-80 RST excess bytes block

        if (strncmp("rst ", tag, 4) == 0) {     // 'rst ' block
            if (len != sizeof rom._rst_xtra) {
                // ERROR
            }
            n = fread(rom._rst_xtra, 1, len, f);
        } else

        // ---------------------
        // cpu types block

        if (strncmp("cpu ", tag, 4) == 0) {     // 'cpu ' block (must be after 'type')
            char *block = (char *) malloc(len);
            uint8_t map[256] = {0};
            n = fread(block, 1, len, f);
            for (char *p = block; p < block + len; p += strlen(p + 1) + 2) {
                // get the CPU ID for this name
                // special values 0 and 1 set the default/current CPU type
                class CPU *cpu = CPU::get_cpu(p + 1);
                if (cpu) {
                    switch(*p) {
                        case 0: // defCpu
                            cpu->set_def_cpu();
                            break;
                        case 1: // curCpu
                            cpu->set_cur_cpu();
                            break;
                        default:
                            // add to map
                            map[p[0] & 0xFF] = cpu->_id;
                            break;
                    }
                }
            }

            // if no curCpu, use defCpu
            if (curCpu == &generic) {
                curCpu = defCpu;
#if 1 // uncomment if you don't want defCpu to be generic
            } else
            // if no defCpu, use curCpu
            if (defCpu == &generic) {
                defCpu = curCpu;
#endif
            }

            // now apply map to rom._type[]
            for (size_t i = 0; i < rom._size; i++) {
                if (map[rom._type[i]]) {
#if 0
if (i == 0) {
    printf("  mapping type=%d -> %d  " CRLF, rom._type[i], map[rom._type[i]]);
    fflush(stdout);
    sleep(5);
}
#endif
                    rom._type[i] = map[rom._type[i]];
                }
            }

            free(block);
        } else

        // ---------------------
        // tabs block

        if (strncmp("tabs", tag, 4) == 0) {     // 'tabs' block
            unsigned char buf[DisLine::T_NTABS-1];

            n = fread(buf, 1, DisLine::T_NTABS-1, f);
            for (int i = 0; i < DisLine::T_NTABS-1; i++) {
                DisLine::tabs[i] = buf[i];
            }
        } else

        // ---------------------
        // view info block

        if (strncmp("view", tag, 4) == 0) {     // 'view' block
            addr_t sel_addr = read_long(f);
            int    sel_line = read_long(f);
            addr_t top_addr = read_long(f);
            int    top_line = read_long(f);
            // ERROR if unable to read any of those

            // compensate if overrides have put the view outside the data
            if (top_addr < rom._base || top_addr > rom.get_end()) {
                top_addr = rom._base;
                top_line = 0;
            }
            if (sel_addr < rom._base || sel_addr > rom.get_end()) {
                sel_addr = rom._base;
                sel_line = 0;
            }

            // update scrn._sel and scrn._top
            // note: caller needs to smash everything but _sel and _top
            scrn._sel.addr = sel_addr;
            scrn._sel.line = sel_line;
            scrn._top.addr = top_addr;
            scrn._top.line = top_line;

            scrn._end.addr = rom.get_end();
            scrn._end.line = 0;
        } else {

        // ---------------------
        // unknown block type
#if 0
            // skip the block data
            fseek(f, len, SEEK_CUR);
#else
            // unknown block encountered
            sprintf(err, "unknown block type %4s", tag);
            scrn.error(err);
            break;
#endif
        }

        // loops until error or "end " block found
    }

ERROR:
    // close the file
    if (f) {
        fclose(f);
    }

    // nuke the load if there was a problem
    if (scrn._err[0]) {
#ifdef DBG
//printf("  error: '%s'  " CRLF, scrn._err); fflush(stdout);
#endif
        rom.unload();
        scrn.key_home();
#ifdef DBG
//sleep(2);
#endif
        return -1;
    }

    // read comments
    strcpy(path, rom._fname);
    strcat(path, ".cmt");
    cmt.load_comments(path);
    
    rom.save_undo();
    rom._changed = false;

#ifdef DBG
//sleep(2);
#endif

    return 0;
}



