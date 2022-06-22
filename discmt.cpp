// discmt.cpp
// comments storage
// this is not an efficient design, but it works (?)

#include "discmt.h"

#include "disscrn.h" // for HexVal
#include <errno.h>

CommentDB cmt;


// =====================================================
// if found, returns address of matching comment
// otherwise returns address of comment to insert after
// or NULL to insert before first comment
CommentDB::comment *CommentDB::find_comment(addr_t addr)
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
const char *CommentDB::get_comment(addr_t addr)
{
//printf("get_comment(%.8X) = ", (int) addr);
#if 1
    comment *p = find_comment(addr);

    if (p && addr == p->addr) {
//printf("'%s'\n", p->text);
        return p->text;
    }
#else
    for (comment *p = head; p; p = p->next) {
        if (addr == p->addr) {
            return p->text;
        }
        if (addr > p->addr) {
            break;
        }
    }
#endif
//printf("NULL\n");
    return NULL;
}


// =====================================================
void CommentDB::set_comment(addr_t addr, const char *s)
{
(void) addr;
(void) s;

//printf("set_comment(%.8X, ", (int) addr);
//if (s) printf("'%s')\n", s);
//  else printf("NULL)\n");

    // first try to find the comment
    comment *p = find_comment(addr);

    // is it the one we were looking for?
    if (!p || addr != p->addr) {
        // not the droid we were looking for
        if (!(s && *s)) {
             // trying to clear a non-existent comment
             return;
        }

        // create a new comment object
        comment *c = new_comment(addr, s);

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
            } else {
                // find previous comment
                comment *prev = NULL;
                for (comment *c = head; c && c != p; c = c->next) {
                    prev = c;
                }
                if (prev && prev->next == p) {
                    prev->next = p->next;
                    // cache the new comment
                    cache = p;
                    cache_prev = prev;
                } else {
                    // should not get here!
                    printf("  huh?\n"); fflush(stdout);
                }
            }
            // dispose of the old comment
            free_comment(p);
        }
    }
}


// =====================================================
void CommentDB::load_comments(const char *path)
{
    // start with empty comments list
    free_comments();

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
        }

        // look for blank separator
        char *str = strchr(p, ' ');

        // get addr and text and add the comment
        if (str) {
            *str++ = 0;
            addr_t addr = HexVal(p);
            set_comment(addr, str);
        }

        // get next line of comments file
        p = fgets(s, sizeof s - 1, f);
    }

    fclose(f);
}


// =====================================================
void CommentDB::save_comments(const char *path)
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
void CommentDB::free_comments()
{
    for (comment *p = head; p; ) {
        comment *q = p->next;
        free_comment(p);
        p = q;
    }

    head = NULL;
}


// =====================================================
CommentDB::comment *CommentDB::new_comment(addr_t addr, const char *s)
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
void CommentDB::free_comment(CommentDB::comment *p)
{
    if (p->text) {
        free((void *) p->text);
    }
    free(p);
}


// =====================================================
void CommentDB::dump_comments()
{
    printf("dump_comments()\n");

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

