// discmt.cpp
// comments storage
// this is not an efficient design, but it works (?)

#include "discmt.h"

#include "disscrn.h" // for HexVal
#include <errno.h>

// global scope comment storage object
SymDB cmt;
// global scope symbols storage object
SymDB sym;
// global scope equates storage object
SymDB equ;


// =====================================================
// if found, returns address of matching comment
// otherwise returns address of comment to insert after
// or NULL to insert before first comment
SymDB::comment *SymDB::find_sym(addr_t addr)
{
    comment *q = NULL; // previous comment
    comment *p = head;

    // if addr is past the cache, skip to the middle of the list
    if (cache && addr >= cache->addr) {
        p = cache;
        q = cache_prev;
    }

    // search for addr
    while (p) {
        // is it an exact match?
        if (addr == p->addr) {
            // update cache
            cache = p;
            cache_prev = q;

            return p;
        }

        // we've gone past where it should be, so stop now
        if (addr < p->addr) {
            break;
        }

        // go to the next comment
        q = p;
        p = p->next;
    }

    // return comment to insert after
    return q;
}


// =====================================================
const char *SymDB::get_sym(addr_t addr)
{
    comment *p = find_sym(addr);

    if (p && addr == p->addr) {
        return p->text;
    }
    return NULL;
}


// =====================================================
void SymDB::set_sym(addr_t addr, const char *s)
{
    // first try to find the comment
    comment *p = find_sym(addr);

    // is it the one we were looking for?
    if (!p || addr != p->addr) {
        // not the droid we were looking for
        if (!(s && *s)) {
             // trying to clear a non-existent comment
             return;
        }

        // create a new comment object
        comment *c = new_sym(addr, s);

        // link it into the list after p
        if (p) {
            c->next = p->next;
            p->next = c;
        } else {
            c->next = head;
            head = c;
        }
    } else {
        // comment found
        if (s && s[0]) {
            // change comment
            free((void *) p->text);
            p->text = (char *) malloc(strlen(s)+1);
            strcpy((char *) p->text, s);
        } else {
            // delete comment
            // need to find prev comment first
            if (p == head) {
                // remove from head of list
                head = p->next;
                // invalidate the cache pointer
                cache = 0;
            } else {
                // find previous comment
                comment *prev = NULL;
                for (comment *c = head; c && c != p; c = c->next) {
                    prev = c;
                }
                if (prev && prev->next == p) {
                    // unlink the deleted comment
                    prev->next = p->next;
                    // cache the next comment
                    cache = p->next;
                    cache_prev = prev;
                } else {
                    // should not get here!
                    printf("  huh?  \n"); fflush(stdout);
                }
            }
            // dispose of the old comment
            assert(p != cache_prev);
            assert(p != cache);
            free_sym(p);
        }
    }
}


// =====================================================
void SymDB::load_syms(const char *path, bool isComment)
{
    // start with empty comments list
    free_syms();

    FILE *f = fopen(path, "rt");
    if (!f) {
        // assume no comments if .cmt file not found
        if (errno == ENOENT) return;

        return; // ***FIXME: need to report error
    }

    char s[256];
    
    // get first line of comments file
    char *p = fgets(s, sizeof s - 1, f);
    while (p) {
        // remove trailing newline
        int n = strlen(p);
        if (n && p[n-1] == '\n') {
            p[n-1] = 0;
            n--;
        }
        if (n && p[n-1] == '\r') {
            p[n-1] = 0;
            n--;
        }

        // look for blank separator
        char *str = strchr(p, ' ');

        // get addr and text and add the comment
        if (str) {
            // remove everything after whitespace if not comment
            if (!isComment) {
                strtok(str, " \x09");
            }
            *str++ = 0;
            if (ishex(s[0])) {
                addr_t addr = HexVal(p);
                set_sym(addr, str);
            }
        }

        // get next line of comments file
        p = fgets(s, sizeof s - 1, f);
    }

    fclose(f);
}


// =====================================================
void SymDB::save_syms(const char *path)
{
    if (head) {
        // create the .cmt file
        FILE *f = fopen(path, "wt");
        if (!f) return; // ***FIXME: need to report error

        for (comment *p = head; p; p = p->next) {
            // check that the comment is on a line
            if (!(rom.get_attr(p->addr) & ATTR_CONT)) {
                fprintf(f, "%.4X %s\n", (int) p->addr, p->text);
            }
        }

        fclose(f);
    } else {
        // no comments to save, delete the .cmt file
        unlink(path);
    }
}


// =====================================================
// free up the entire list of comments
void SymDB::free_syms()
{
    for (comment *p = head; p; ) {
        comment *q = p->next;
        free_sym(p);
        p = q;
    }

    head = NULL;
}


// =====================================================
SymDB::comment *SymDB::new_sym(addr_t addr, const char *s)
{
    comment *p = (comment *) malloc(sizeof *p);

    char *str = (char *) malloc(strlen(s)+1);
    strcpy(str, s);

    p->next = NULL;
    p->addr = addr;
    p->text = str;

    return p;
}


// =====================================================
void SymDB::free_sym(SymDB::comment *p)
{
    if (p->text) {
        free((void *) p->text);
    }
    free(p);
}


// =====================================================
void SymDB::dump_syms()
{
    printf("dump_syms()\n");

    if (!head) {
        printf("  head = NULL\n");
    }

    for (comment *p = head; p; p = p->next) {
        printf("  %.8X ", (int) p->addr);
        if (p->text) {
            printf("'%s'", p->text);
        } else {
            printf("NULL");
        }
        printf("\n");
    }
}

