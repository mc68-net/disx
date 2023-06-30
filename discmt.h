// discmt.h
// storage for user-entered comments and labels

#ifndef _DISCMT_H_
#define _DISCMT_H_

#include "disx.h"


class SymDB {
private:
    // Comments/symbols are currently implemented as a linked list.
    // While not the most efficient thing, it works.
    // Since most requests will be in sequential order, simply
    // caching the last request should help a lot.

    struct comment {
        comment    *next;   // pointer to next list element
        addr_t      addr;   // address for this comment or label
        const char *text;   // comment or label text
    };

    comment *head;
    comment *cache; // pointer to last successful get_sym request
    comment *cache_prev; // previous link of cached comment

    comment *new_sym(addr_t addr, char const *s);
    void free_sym(comment *p);

    comment *find_sym(addr_t addr);

    void dump_syms();               // print all comments (for debugging)

public:
    SymDB() : head(0), cache(0) { }

    const char *get_sym(addr_t addr);   // returns a comment string or NULL
    void set_sym(addr_t addr, const char *s); // sets/changes/deletes a comment

    void load_syms(const char *path);   // load comments from a file
    void save_syms(const char *path);   // save comments to a file
    void free_syms();                   // delete all comments
};


// global scope comment storage object
extern SymDB cmt;
// global scope symbols storage object
extern SymDB sym;


#endif // _DISCMT_H_
