// discmt.h
// storage for user-entered comments and labels

#ifndef _DISCMT_H_
#define _DISCMT_H_

#include "disx.h"


class SymDB {
private:
    struct comment {
        comment *next;
        addr_t addr;
        const char * text;
    };

    comment *head;
    comment *cache; // pointer to last successful get_sym request
    comment *cache_prev; // previous link of cached comment

    comment *new_sym(addr_t addr, char const *s);
    void free_sym(comment *p);

    comment *find_sym(addr_t addr);

public:
    SymDB() : head(0), cache(0) { }

    const char *get_sym(addr_t addr);     // returns a comment string or NULL
    void set_sym(addr_t addr, const char *s); // sets/changes/deletes a comment

    void load_syms(const char *path);     // load comments from a file
    void save_syms(const char *path);     // save comments to a file
    void free_syms();               // delete all comments

    void dump_syms();               // print all comments (for debugging)
};


// global scope comment storage object
extern SymDB cmt;
// global scope comment storage object
extern SymDB cmt;


#endif // _DISCMT_H_
