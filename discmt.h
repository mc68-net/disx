// discmt.h
// storage for user-entered comments

#ifndef _DISCMT_H_
#define _DISCMT_H_

#include "disx.h"


class CommentDB {
private:
    struct comment {
        comment *next;
        addr_t addr;
        const char * text;
    };

    comment *head;
    comment *cache; // pointer to last successful get_comments request
    comment *cache_prev; // previous link of cached comment

    comment *new_comment(addr_t addr, char const *s);
    void free_comment(comment *p);

    comment *find_comment(addr_t addr);

public:
    CommentDB() : head(0), cache(0) { }

    const char *get_comment(addr_t addr);     // returns a comment string or NULL
    void set_comment(addr_t addr, const char *s); // sets/changes/deletes a comment

    void load_comments(const char *path);     // load comments from a file
    void save_comments(const char *path);     // save comments to a file
    void free_comments();               // delete all comments

    void dump_comments();               // print all comments (for debugging)
};


// global scope comment handling object
extern CommentDB cmt;


#endif // _DISCMT_H_
