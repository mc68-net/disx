// dissave.h

#ifndef _DISSAVE_H_
#define _DISSAVE_H_

#include "disx.h"


// =====================================================
class DisSave {

public:
    enum {
        FORCE_FNAME = 1, // override file name from .ctl file
        FORCE_BASE  = 2, // override base address from .ctl file
        FORCE_OFS   = 4, // override start offset from .ctl file
        FORCE_SIZE  = 8  // override image size from .ctl file
    };

    int save_file();
    int load_file(const char *fname, addr_t ofs, addr_t base, addr_t size,
                  int force = 0);

    int save_block(FILE *f, const char *type, const void *data, int len);
    void *load_block(FILE *f, char *type, int &len);

    void write_long(FILE *f, int data) const;
    int read_long(FILE *f) const;
};


#endif // _DISSAVE_H_
