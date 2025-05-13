// disscrn.cpp
// ncurses-based screen handling for disassembler

#include "disscrn.h"

#include "dissave.h"
#include "discpu.h"
#include "discmt.h"
#include <ctype.h>
#include <signal.h>

//#define DEBUG_POS // show debug info of cursor pos, etc.
#define CRLF "\r\n\e[0K"
#define BEL "\x07"

// global screen handler object
DisScrn scrn;

// control-C status flag
bool DisScrn::_ctrl_c = false;

// =====================================================
// dispatch table for single-char commands

struct cmd_char_t {
    char key;
    void (DisScrn::* func)(void);
};

struct cmd_char_t char_cmds[] =
{
//  { 'q',  &DisScrn::do_cmd_q   }, // exit program

    { 'a',  &DisScrn::do_cmd_a   }, // disasssemble as ascii
    { 'b',  &DisScrn::do_cmd_b   }, // disasssemble as bytes
    { 'h',  &DisScrn::do_cmd_h   }, // disassemble as hex
    { 'w',  &DisScrn::do_cmd_w   }, // disassemble as words
    { 0x17, &DisScrn::do_cmd_w1  }, // disassemble as word address minus one
    { 'W',  &DisScrn::do_cmd_W   }, // shft-W disassemble as reverse words
    { '\\', &DisScrn::do_cmd_lng }, // disassemble as longs
    { 'd',  &DisScrn::do_cmd_d   }, // disassemble as decimal word
    { 'D',  &DisScrn::do_cmd_D   }, // disassemble as decimal long
    { 'I',  &DisScrn::do_cmd_I   }, // disassemble as binary
 // { 'X',  &DisScrn::do_cmd_X   }, // disassemble as big-endian binary X
    { '_',  &DisScrn::do_cmd_ebc }, // disasssemble as ebcdic
 // { 'O',  &DisScrn::do_cmd_O   }, // disassemble as little-endian binary O
    { 'o',  &DisScrn::do_cmd_o   }, // disassemble as offset table entry
    { '|',  &DisScrn::do_cmd_rl  }, // shft-\ disassemble as reverse longs
    { '*',  &DisScrn::do_cmd_rpt }, // repeat previous b/w/|/a/h command
    { 'x',  &DisScrn::do_cmd_x   }, // disassemble as raw

    { 'c',  &DisScrn::do_cmd_c   }, // disassemble as code
    { 'C',  &DisScrn::do_cmd_C   }, // disassemble as code until lfflag or illegal
    { 't',  &DisScrn::do_cmd_T   }, // trace disassembly from _sel
    { 'T',  &DisScrn::do_cmd_cT  }, // trace disassembly from refaddr
    { '"', &DisScrn::do_cmd_reflbltyp }, // ctrl-L toggle label type at refaddr
    { '\'',  &DisScrn::do_cmd_lbltyp  }, // toggle this line's label type
    { 'O',  &DisScrn::do_cmd_Open   }, // O toggle pre-instruction blank line
    { 0x12, &DisScrn::do_cmd_cR  }, // ctrl-R toggle NOREF for this code address
    { 0x15, &DisScrn::do_cmd_cU  }, // ctrl-U save undo buffer
    { 'U',  &DisScrn::do_cmd_U   }, // shft-U swap undo buffer

    { '[',  &DisScrn::do_cmd_prv }, // go to previous label
    { ']',  &DisScrn::do_cmd_nxt }, // go to next label
    { '<',  &DisScrn::do_cmd_bak }, // go to previous address from @ or :addr
    { 0x14, &DisScrn::do_cmd_bak }, // ^T (v) "pop tag stack"
    { '>',  &DisScrn::do_cmd_fwd }, // go to next stacked address from @ or :addr
    { '@',  &DisScrn::do_cmd_ref }, // go to refaddr
    { 0x1D, &DisScrn::do_cmd_ref }, // ^] (vi) "go to tag definition"
    { '(',  &DisScrn::do_cmd_les }, // less data
    { ')',  &DisScrn::do_cmd_mor }, // more data

    { '`',  &DisScrn::do_cmd_cen }, // recenter screen
    { 'M',  &DisScrn::do_cmd_center }, // move selecton to center of screen
    { '~',  &DisScrn::do_cmd_top }, // move selecton to near top of screen
    { 'H',  &DisScrn::do_cmd_top }, // move selecton to near top of screen

    { '!',  &DisScrn::do_cmd_cln }, // clean up current label if not referenced

    { 'F',  &DisScrn::do_cmd_hint }, // toggle hint flags in current instr
    { '$',  &DisScrn::do_cmd_defhint }, // toggle default hint flags

    {  0,   NULL  }
};


// =====================================================
// dispatch table for command-line commands

struct cmd_line_t {
    const char *cmd;
    void (DisScrn::* func)(char *p);
};

struct cmd_line_t line_cmds[] =
{
    { "quit",   &DisScrn::do_cmd_quit   }, // "quit"   - exit program
    { "q",      &DisScrn::do_cmd_quit   }, // "q"      - exit program
    { "lst",    &DisScrn::do_cmd_list   }, // "lst"    - save disassembly listing
    { "list",   &DisScrn::do_cmd_list   }, // "list"   - save disassembly listing
    { "l",      &DisScrn::do_cmd_label  }, // "l"      - set/clear a label
    { "label",  &DisScrn::do_cmd_label  }, // "label"  - set/clear a label
    { "asm" ,   &DisScrn::do_cmd_asm    }, // "asm"    - save asm source listing
    { "save",   &DisScrn::do_cmd_save   }, // "save"   - save disassembly state
    { "wq",     &DisScrn::do_cmd_wq     }, // "wq"     - save state and quit
    { "w",      &DisScrn::do_cmd_save   }, // "w"      - save disassembly state
    { "load",   &DisScrn::do_cmd_load   }, // "load"   - load binary and disassembly state
//  { "l",      &DisScrn::do_cmd_load   }, // "l"      - load binary and disassembly state
//  { "new",    &DisScrn::do_cmd_new    }, // "new"    - unload current file
    { "rst",    &DisScrn::do_cmd_rst    }, // "rst"    - set lengths after Z80 RST instrs
//  { "go",     &DisScrn::do_cmd_go     }, // "go"     - go to address
//  { "g",      &DisScrn::do_cmd_go     }, // "g"      - go to address
    { "$",      &DisScrn::do_cmd_end    }, // "$"      - go to end
    { "cpu",    &DisScrn::do_cmd_cpu    }, // "cpu"    - set default CPU type
    { "defcpu", &DisScrn::do_cmd_defcpu }, // "defcpu" - set default CPU type
    { "tab",    &DisScrn::do_cmd_tab    }, // "tab"    - set/show tab stops
    { "tabs",   &DisScrn::do_cmd_tab    }, // "tabs"   - set/show tab stops
    {  NULL,    NULL  }
};


// =====================================================
// handle SIGINT

// Control-C signal handler
void finish(int sig)
{
    // let the application handle control-C
    if (sig == SIGINT) {
        scrn.ctrl_c();
        return;
    }

    end_screen();

    // ===== do your non-curses wrapup here

    exit(0);
}


// =====================================================
// SIGWINCH signal handler

static bool sigwinched = false;

void sigwinch(int UNUSED sig)
{
    // use a flag instead of trying to do ncurses calls
    // in what could be an interrupt context
    sigwinched = true;
}


// =====================================================
extern "C" void my_assert_func(const char *expr, const char *file, int line)
{
    // clear the screen and exit curses
    end_screen();

    printf("%s:%d assertion failed: '%s'\n", file, line, expr);

#if 0
    printf("_start = %.4X:%d\n", (int) scrn._start.addr, scrn._start.line);
    printf("_top   = %.4X:%d\n", (int) scrn._top.addr, scrn._top.line);
    printf("_sel   = %.4X:%d\n", (int) scrn._sel.addr, scrn._sel.line);
    printf("_end   = %.4X:%d\n", (int) scrn._end.addr, scrn._end.line);
    printf("_sel_row  = %3d\n", scrn._sel_row);
//  printf("curs_row  = %3d\n", screen_row(_sel));
//  printf("end_row   = %3d\n", screen_row(_end));
    printf("max_row() = %3d\n", scrn.max_row());
    printf("get_len() = %3d\n", scrn._sel.get_len());
    printf("attr = %.2X\n", rom.get_attr(scrn._sel.addr));
#endif

    exit(0);
}

// =====================================================
void setup_screen()
{
    // set up curses
    initscr();              // initialize the curses library
    keypad(stdscr, TRUE);   // enable extended function key mapping
    set_escdelay(25);       // set escape delay of 25ms to allow ESC key
    nonl();                 // tell curses not to do NL->CR/NL on output
    cbreak();               // take input chars one at a time, no wait for \n
    noecho();               // don't echo input

#if 0 // disable colors for now
    if (has_colors()) {
        start_color();

        // Simple color assignment, often all we need.  Color pair 0 cannot
        // be redefined.  This example uses the same value for the color
        // pair as for the foreground color, though of course that is not
        // necessary:
        init_pair(1, COLOR_RED,     COLOR_BLACK);
        init_pair(2, COLOR_GREEN,   COLOR_BLACK);
        init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
        init_pair(4, COLOR_BLUE,    COLOR_BLACK);
        init_pair(5, COLOR_CYAN,    COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_WHITE,   COLOR_BLACK);
    }
#endif

    // set up signal handlers
    signal(SIGINT,  finish);  // ctrl-C
    signal(SIGTSTP, SIG_IGN); // disable ctrl-Z
    signal(SIGQUIT, SIG_IGN); // disable ctrl-backslash

    // set up (proper) SIGWINCH handler
    // apparently it needs to be registered without the SA_RESTART flag
    // whatever that means: https://stackoverflow.com/questions/53822353/
    struct sigaction old_action;
    struct sigaction new_action;
    new_action.sa_handler = sigwinch;
    new_action.sa_flags = 0;        // no flags!
    sigemptyset(&new_action.sa_mask);
    sigaction(SIGWINCH, &new_action, &old_action);

    // set up non-blocking keyboard input to allow eating ctrl-C's
    timeout(500);
}


// =====================================================
void end_screen()
{
    // clean up after curses
    endwin();
}


// =====================================================
void DisScrn::ctrl_c()
{
    // flag was not cleared, something probably got wedged
    // this means that if something hangs, you can still
    // press ctrl-C twice to get out
    if (_ctrl_c) {
        // try to clean up the terminal before exiting
        finish(0);
    }

    // set the flag
    _ctrl_c = true;
}


// =====================================================
//
// displays debug information on the screen
// called at the end of print_screen()

void DisScrn::debug_info()
{
    int row = 1;        // first row for debug info
    int col = 80;       // column for debug info

    wmove(_win, row++, col);
    wprintw(_win, " _start = %.4X:%d ", (int) _start.addr, (int) _start.line);

    wmove(_win, row++, col);
    wprintw(_win, " _top   = %.4X:%d ", (int) _top.addr, (int) _top.line);

    wmove(_win, row++, col);
    wprintw(_win, " _sel   = %.4X:%d ", (int) _sel.addr, (int) _sel.line);

    wmove(_win, row++, col);
    wprintw(_win, " _end   = %.4X:%d ", (int) _end.addr, (int) _end.line);

    wmove(_win, row++, col);
    wprintw(_win, " _sel_row  = %3d ", _sel_row);

//  wmove(_win, row++, col);
//  wprintw(_win, " curs_row  = %3d ", screen_row(_sel));

//  wmove(_win, row++, col);
//  wprintw(_win, " end_row   = %3d ", screen_row(_end));

    wmove(_win, row++, col);
    wprintw(_win, " max_row() = %3d ", max_row());

    wmove(_win, row++, col);
    wprintw(_win, " get_len() = %3d ", _sel.get_len());

    wmove(_win, row++, col);
    wprintw(_win, " attr = %.2X ", rom.get_attr(_sel.addr));

    // spit it out
    wrefresh(_win);
}


// =====================================================
// void init_scrn()
//
// This initializes the screen handler object.
//
// Note that an explicit constructor is not used, to avoid potential
// problems with it calling other objects. The screen handler object
// is declared at global scope and there is not a definited order
// for constructors.

void DisScrn::init_scrn(bool reset)
{
    disline.get_start_addr(_start);     // start address
    disline.get_end_addr(_end);         // line *after* last line
    _sel_row   = 0;
    _count     = 0;
    _win       = stdscr;
    _in_input  = 0;
    _cmd[0]    = 0;
    _err[0]    = 0;
    _errbeep   = false;
    _search[0] = 0;
    _quit      = false;
    _rpt_key   = 0;
    _rpt_count = 1;
    memset(_goto_stk, 0, sizeof _goto_stk);
    _goto_sp   = 0;

if (reset) {
    _top       = _start;
    _sel       = _start;
} else {
    recenter();
}

    print_screen();
}

// =====================================================
// push address to _goto_stk
void DisScrn::push_addr(addr_t addr, addr_t next)
{
    // don't push a null
    if (!addr) {
        return;
    }

    if (_goto_sp == GOTO_SIZE) {
        // stack is full, just drop the oldest one
        for (int i = 0; i < GOTO_SIZE - 1; i++) {
            _goto_stk[i] = _goto_stk[i+1];
        }
        _goto_sp--;
    }

    _goto_stk[_goto_sp++] = addr;

    // try to push forward the "next" address
    if (next) {
        push_addr(next);
        _goto_sp--;
    }
}


// =====================================================
// pop address from _goto_stk
addr_t DisScrn::pop_addr()
{
    // stack is empty, return fail
    if (_goto_sp == 0) {
        return 0;
    }

    return _goto_stk[--_goto_sp];
}


// =====================================================
// un-pop address from _goto_stk
addr_t DisScrn::un_pop_addr()
{
    // already at end
    if (_goto_sp == GOTO_SIZE) {
        return 0;
    }
    if (_goto_stk[_goto_sp + 1]) {
        return _goto_stk[++_goto_sp];
    }
    return 0;
}

// =====================================================
// returns the screen row which contains ad (starting at _top)
// returns -1 if before _top
// returns -2 if beyond max_row()
int DisScrn::screen_row(addrline_t ad) const
{
    // check if sel is before top of screen
    if (ad.addr < _top.addr) {
        return -1;
    }

    // count lines from _top until ad
    addrline_t addr = _top;
    int row = 0;
    while (row <= max_row()) {
        if (addr == ad) {
            // selection found
            return row;
        }
        if (addr == _end) {
            // ad is after _end, (but it shouldn't be)
            break;
        }
        addr.next_line();
        row++;
    }

    // past end of screen
    return -2;
}


// =====================================================
// attempts to make sure that _sel is on the screen
void DisScrn::set_sel_line(int row)
{
    // set _top to 'row' lines before _sel, stopping at _start
    addrline_t addr = _sel;
    for (int i = 0; i < row && addr != _start; i++) {
        addr.prev_line();
    }
    _top = addr;

    // if _end is on screen, try to keep it at bottom of screen

    // find the screen row of the _end line
    int end_row = screen_row(_end);
    if (end_row >= 0 && end_row < max_row()) {
        // need to move screen back down by "max_row() - end_row" lines
        // but if _top ever reaches _start then stop
        // (because the screen is bigger than entire file)
        for (int i = max_row() - end_row; (i > 0) && (_top != _start); i--) {
            _top.prev_line();
        }
    }
}


// =====================================================
// attempts to make sure that _sel is on the screen
// strict == true ensures that the selection is not
// too close to the bottom or top of the screen
void DisScrn::recenter(bool strict)
{
    // thresholds are numlines/5 and numlines*4/5
    int threshold = max_row() / 5;
    if (threshold > 10) {
        threshold = 10;
    }

    // find the screen row of _sel
    int row = screen_row(_sel);

    if (row == -1) {
        // if _sel is before _top, count back threshold from _sel and set _top
        set_sel_line(threshold);
        return;
    }

    if (row == -2) {
        // if _sel is after _top + maxrows,
        // try to place _top at 3/4 threshold back from _sel
        set_sel_line(max_row() - threshold);
        return;
    }

    // positive row number is on screen, may not need to re-center

    if (strict) {
        // if in first 1/4 of screen, move _top back
        if (row < threshold) {
            set_sel_line(threshold);
            return;
        }

        // if in last 1/4 of screen, move _top forward
        if (row > max_row() - threshold) {
            set_sel_line(max_row() - threshold);
            return;
        }
    }
}


// =====================================================
void DisScrn::error(const char *s)
{
    if (s && s[0] == 0x07 /* BEL */) {
        _errbeep = true;
        s++;
    }

    if (s && s[0]) {
        // if error message, save it and clear count
        strcpy(_err, s);
        _count = 0;
    } else {
        // no error message, just clear it
        _err[0] = 0;
    }

    status_line();
    wrefresh(_win);
}


// =====================================================
// void print_line(int addr, int UNUSED sub_line)
//
// This print one line of the screen.
// - addr = the address of the line being printed
// - sub_line = which line of the address to print
//
void DisScrn::print_line(struct addrline_t addr)
{
    // use DisLine
    char s[256]; // storage for line text
    disline.get_text(addr, s);

    // check remaining width of screen
    int w = getmaxx(_win) - getcurx(_win);
    if (strlen(s) >= w) {
        strcpy(s + w - 3, "...");
    }
    wprintw(_win, "%s", s);

    // return number of lines to advance?
    // (in future, possibly return negative if more sub_lines?)
}


// =====================================================
//  void set_cursor(void);
//
void DisScrn::set_cursor()
{
    if (_in_input) {
        wmove(_win, /*row=*/ 0, /*col=*/ _cmd_pos - _cmd_col + 1);
    } else {
        // put cursor on right edge of screen to indicate scroll position
        // note that this is based on address, not lines, so "long" lines
        // will scroll faster!
        float pos = (float) (_sel.addr - _start.addr) / (float) rom._size
                  * (float) (max_row());
        wmove(_win, /*row=*/ 1 + (int) (pos + 0.5), /*col=*/ getmaxx(_win) - 1);
    }
}


// =====================================================
//  void status_line(void);
//
void DisScrn::status_line()
{
    wmove(_win, /*row=*/ 0, /*col=*/ 0);

    if (_in_input) {
        // display current command

        // default to start with prompt character
        char c = _in_input;

        // determine max width of command line in the window
        int maxwid = getmaxx(_win) - 2;

        // compute X position of cursor
        int xpos = _cmd_pos - _cmd_col;

        // check if beyond either edge of the command line space
        if (xpos < 0) {
            // before left edge, add xpos to _cmd_col, max 0
            _cmd_col += xpos;
            if (_cmd_col < 0) {
                _cmd_col = 0;
            }
        } else if (xpos > maxwid) {
             // after right edge, add xpos - maxwid to _cmd_col
            _cmd_col += xpos - maxwid;
        }

        // determine how much of _cmd is after _cmd_col
        int n = strlen(_cmd + _cmd_col);
        if (n > maxwid) {
            n = maxwid;
        }
        n += _cmd_col;

        // if not at first char, start the line with '<' character
        if (_cmd_col) {
            // start with a '<' to show that something is off to the left
            c = '<';
        }

        // see if end of string is outside the window range
        char cc = _cmd[n];
        if (cc) {
            // temporarily lop off the end of _cmd
            _cmd[n] = 0;
        }

        // print what can be shown
        wprintw(_win, "%c%s", c, _cmd + _cmd_col);
        if (cc) {
            // start with a '>' to show that something is off to the right
            wprintw(_win, ">");
            _cmd[n] = cc;
        }

        wclrtoeol(_win);
    } else {
        // display status bar, in inverse video
        wattron(_win, A_REVERSE);

#if 0
        // what to display? I dunno, how about the current address?
        if (defCpu->_addrwid == ADDR_16) {
            wprintw(_win, "  ADDR = %.4X", _sel.addr);
        } else {
            wprintw(_win, "  ADDR = %.6X", _sel.addr);
        }
#endif

        if (_count) {
            wprintw(_win, "  COUNT = %d", _count);
        }

        if (_err[0]) {
            wprintw(_win, "  %s", _err);
        }

        // fill from cursor column to width-1 with inverse blanks
        for (int i = getcurx(_win); i < getmaxx(_win)-1; i++) {
            waddch(_win, ' ');
        }

        wattroff(_win, A_REVERSE);
    }

    set_cursor();
}


// =====================================================
// void print_screen()
//
// This refreshes the entire disasembly display.
// It seems rather wasteful to redraw the entire screen after
// every key, but ncurses will (should) optimize redraw for us.

void DisScrn::print_screen()
{
    // recovery attempt in case _sel_row has fallen off the screen
    // don't want to do this recursively because it's much more difficult
    // to do it exactly once that way
    bool tried = false;
TryAgain:

    // set disassembly to screen format
    disline.line_cols = disline.SCRN_COLS;

    // ensure that we haven't gone off the rails
    // how can this happen?
    // - be on a raw byte (mData)
    // - do a trace disassembly
    // - the byte you are sitting on becomes an instruction
    // - but not the first byte of the instruction

    // check that _sel line is not ATTR_CONT
    while (rom.get_attr(_sel.addr) & ATTR_CONT) {
        _sel.addr--;
        _sel.line = 0;
    }

    // check that _top line is not ATTR_CONT
    while (rom.get_attr(_top.addr) & ATTR_CONT) {
        _top.addr--;
        _top.line = 0;
    }

    // check that _sel sub-line is in range
    int n = _sel.num_lines();
    if (_sel.line >= n) {
        _sel.line = n - 1;
    }

    // check that _top sub-line is in range
    n = _top.num_lines();
    if (_top.line >= n) {
        _top.line = n - 1;
    }

#if 0
    // now ensure that _sel hasn't sneaked above _top
    if ((_sel.addr < _top.addr) ||
        ((_sel.addr == _top.addr) && (_sel.line < _top.line))) {
        _top = _sel;
    }
#else
    // try to make sure that _sel is on screen
    recenter();
#endif

    struct addrline_t addr = _top; // get top line of screen

    _sel_row = -1;      // default to selected row not yet found

    // print status line
    status_line();

    // loop to print visible rows
    // note that row=0, col=0 is top left of screen
    for (int row = 0; row <= max_row(); row++) {
        // go to line of display
        wmove(_win, row+1, /*col=*/ 0);

        // if this is the selected address and line, highlight it
        bool inverse = (addr == _sel);
        if (inverse) {
            _sel_row = row;     // _sel_row has been found
            wattron(_win, A_REVERSE);
        }

        // print the line for that row, then advance to next addr/line
        print_line(addr);

        // unfortunately wclrtoeol() doesn't propagate the reverse attribute
        if (inverse) {
            // fill from cursor column to width-1 with inverse blanks
            for (int i = getcurx(_win); i < getmaxx(_win)-1; i++) {
                waddch(_win, ' ');
            }
        }

        wclrtoeol(_win);

        // clear attributes for next line
        wattroff(_win, A_REVERSE);

        // stop if at END line, otherwise advance to next line
        if (addr == _end) {
            break;
        }
        addr.next_line();
    }

    // clear rest of screen just in case
    wclrtobot(_win);

#ifdef DEBUG_POS
    debug_info();
#endif

    // put cursor on highlighted line
    set_cursor();

    // mark screen for refresh
    wrefresh(_win);

    // problem if _sel_row not found
    // what to do if this happens? probably could set _top = _sel and try again
    // might also want to check if it's > max_row()

    if (_sel_row < 0 && !tried) {
        _top = _sel;
        tried = true;
        goto TryAgain;
    } else {
        assert(_sel_row >= 0);
    }
}


// =====================================================
void DisScrn::do_cmd_quit(char *p)
{
    char word[256];
    int token = GetWord(p, word);

    if (!rom._changed || token == '!') {
        _quit = true;
    }

    error("file changed, use :wq or :q! to exit");
}


// =====================================================
void DisScrn::do_cmd_list(char UNUSED *p)
{
    disline.write_listing("lst");

    char s[256+21];
    sprintf(s, "listing saved to %s.lst", rom._fname);
    error(s);
}


// =====================================================
void DisScrn::do_cmd_asm(char UNUSED *p)
{
    disline.write_asm("asm");

    char s[256+21];
    sprintf(s, "listing saved to %s.asm", (char *) rom._fname);
    error(s);
}


// =====================================================
void DisScrn::do_cmd_save(char UNUSED *p)
{
    DisSave save;

    save.save_file();

    char s[256+10];
    sprintf(s, "saved %s.ctl", (char *) rom._fname);
    error(s);
}


// =====================================================
void DisScrn::do_cmd_wq(char UNUSED *p)
{
    *p = 0;
    do_cmd_save(p);
    do_cmd_quit(p);
}


// =====================================================
// :load "filename"
void DisScrn::do_cmd_load(char *p)
{
    DisSave save;

    // attempt to get filename
    char fname[256] = {0};
    GetString(p, fname);

    // reload same file if fname is "-"
    if (fname[0] == '-' && fname[1] == 0) {
        strcpy(fname, rom._fname);
    }

    if (!fname[0]) {
        error("no binary file name specified");
        return;
    }

    // get ofs, base, size if entered
    char word[256];
    size_t ofs  = 0;
    addr_t base = 0;
    size_t size = 0;
    int force = 0;
    int scale = defCpu->word_size();

    int token = GetWord(p, word);
    while (token && !_err[0]) {
        if (token == '!') {
            force |= save.FORCE_FNAME;
        } else {
            // make sure parameter is hexadecimal
            if (!HexOctValid(word+1)) {
                error("invalid hex parameter");
            } else
            switch (tolower(word[0])) {
                case 'b':
                    base = HexOctVal(word+1) * scale;
                    force |= save.FORCE_BASE;
                    break;
                case 's':
                    size = HexOctVal(word+1) * scale;
                    force |= save.FORCE_SIZE;
                    break;
                case 'o':
                    ofs  = HexOctVal(word+1) * scale;
                    force |= save.FORCE_OFS;
                    break;
                default:
                    error("invalid parameter");
                    break;
            }
        }
        token = GetWord(p, word);
    }

    if (!_err[0]) {
        // now try to load the file
        if (save.load_file(fname, ofs, base, size, force)) {
            // failed to load, don't reset screen pointers
            // binary data has been cleared if failure during load
            return;
        }
        init_scrn(false); // do not reset saved _top and _sel
    }
}


// =====================================================
void DisScrn::do_cmd_new(char UNUSED *p)
{
    rom.unload();
    init_scrn();
}


// =====================================================
void DisScrn::do_cmd_rst(char UNUSED *p)
{
    char word[256];
    if (GetWord(p, word)) {
        // confirm eight digits
        if (strlen(word) == sizeof rom._rst_xtra) {
            p = word;
            for (int i = 0; i < sizeof rom._rst_xtra; i++) {
                if (!isdigit(*p)) {
                    goto err;
                }
                rom._rst_xtra[i] = *p++ - '0';
            }
        } else {
err:
            error("RST lengths must be 8 digits");
            return;
        }
    }

    // show current rst_xtra
    p = stpcpy(word, "RST lengths = ");
    for (int i = 0; i < sizeof rom._rst_xtra; i++) {
        *p++ = rom._rst_xtra[i] + '0';
    }
    *p = 0;

    // force screen update to re-scan any visible RST instrs
    print_screen();

    error(word);
}


// =====================================================
void DisScrn::do_cmd_go(char *p)
{
    char word[256];
    if (GetWord(p, word)) {
        int addr = HexOctVal(word) * defCpu->word_size();
        // if before start of binary image, use start address
        if (addr < rom._base) {
            addr = rom._base;
        }

        // if not beyond end of binary image, go to address
        if (addr <= rom.get_end()) {
            // save current _sel.addr on stack
            push_addr(_sel.addr, addr);
            // select line and redraw screen
            _sel.line_start(addr);
            recenter(true);
            print_screen();
            return;
        }
    }

    error("invalid address");
}


// =====================================================
void DisScrn::do_cmd_end(char UNUSED *p)
{
    _sel.line_start(_end.addr);
    recenter(true);
    print_screen();
}


// =====================================================
void DisScrn::do_cmd_cpu(char *p)
{
    char word[256];
    if (GetWord(p, word)) {
        class CPU *cpu = CPU::get_cpu(word);
        if (cpu) {
            cpu->set_cur_cpu();
            sprintf(word, "current CPU type is now '%s'", cpu->_name);
            error(word);
            print_screen();
            return;
        }
    } else {
        sprintf(word, "current CPU type = '%s'", curCpu->_name);
        error(word);
        return;
    }

    error("invalid CPU type");
}


// =====================================================
void DisScrn::do_cmd_defcpu(char *p)
{
    char word[256];
    if (GetWord(p, word)) {
        class CPU *cpu = CPU::get_cpu(word);
        if (cpu) {
            cpu->set_def_cpu();
            sprintf(word, "default CPU type is now '%s'", cpu->_name);
            error(word);
            print_screen();
#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " endian = %d ", generic._endian);
#endif
            return;
        }
    } else {
        sprintf(word, "default CPU type = '%s'", defCpu->_name);
        error(word);
        return;
    }

    error("invalid CPU type");
}


// =====================================================
void DisScrn::do_cmd_tab(char *p)
{
    char s[256];

    if (*p) {
         int tab = 0;
         DisLine::hard_tabs = false;
         while (*p && tab < disline.T_NTABS - 1) {
             int token = GetWord(p, s);
             if (token == '!') {
                 //***FIXME: this should only be allowed as the first parm
                 DisLine::hard_tabs = true;
             } else {
                 int n = DecVal(s);
                 if (!n) {
                     error("invalid parameter");
                     return;
                 }
                 //***FIXME: should complain if hard tabs and not a multiple of 8
                 disline.tabs[tab++] = n;
             }
         }
         print_screen();
    }

    sprintf(s, "current tab stops are: %c %d %d %d %d %d",
            disline.hard_tabs ? '!' : ' ',
            disline.tabs[0], disline.tabs[1],
            disline.tabs[2], disline.tabs[3], disline.tabs[4]);
    error(s);
}


// =====================================================
void DisScrn::do_cmd_label(char *p)
{
    char word[256];
    char str[256];
    int token = GetString(p, word);
    addr_t addr = _sel.addr;
    str[0] = 0;

    if (token) {
        //***TODO: check here for hexadecimal word, if found, change addr

        // convert label to uppercase
        for (char *p = word; *p; p++) {
            *p = toupper(*p);
        }
        strcpy(str, word);
    } else {
        // if no parameter, delete label at current address
    }

    // see if label name has changed
    const char *old = sym.get_sym(addr);
    if (!old || strcmp(old, str)) {
        sym.set_sym(addr, str);
        rom._changed = true;
        print_screen();
    }
}

void DisScrn::do_label_sel()
{
    do_cmd_label(_cmd);
}

void DisScrn::do_label_refaddr()
{
    //  XXX copy/paste/hack from: do_cmd_cT(), do_cmd_ref(), do_cmd_label()
    addrline_t save = _sel;         // save current selection
    addr_t refaddr = get_refaddr(); // get refaddr for current line
    if (!refaddr)  return;          // exit if no refaddr
    _sel.line_start(refaddr);       // set current line to refaddr line
    do_cmd_label(_cmd);             // set label for that line
    _sel = save;                    // restore current line
    //  We need to reprint the screen because do_cmd_label() updated it,
    //  but unfortunately this moves the cursor line to "near the top."
    print_screen();
}


// =====================================================
// int do_cmd_line()
//
// This handles a command in '_cmd'
// Returns non-zero to exit program.

void DisScrn::do_cmd_line()
{
    // get first word as command
    char word[256];
    char *line = _cmd;
    GetWord(line, word);

    // search for command in line_cmds table
    for (cmd_line_t *p = line_cmds; p->cmd; p++) {
        if (strcasecmp(word, p->cmd)==0) {
            error(NULL);
            // call command with pointer to parameters
            (scrn.*(p->func))(line);
            return;
        }
    }

    // check if it's a valid hex address
    if (HexOctValid(word)) {
//        // note: don't check for less than _start
//        if (HexOctVal(word) <= _start.addr) {
            do_cmd_go(word);
            return;
//        }
    }

    error("invalid command");
    print_screen();
}


// =====================================================
// remove doubled blanks from s
const char *rem_blank(char *s)
{
    char last = 0;

    for (char *p = s; *p; p++) {
        if (last != ' ' || *p != ' ') {
            *s++ = *p;
        }
        last = *p;
    }

    *s = 0;
    return s;
}


// =====================================================
// find the next/previous line matching _search
void DisScrn::do_search(bool UNUSED fwd)
{
    // remove excess blanks from search string
    char search[sizeof _search];
    strcpy(search, _search);
    rem_blank(search);

    // can't search for an empty string
    if (!search[0]) {
        return;
    }

    struct addrline_t addr = _sel;

    // width of the address and hex data fields
    const int skiphex = abs(DisLine::tabs[DisLine::T_ADDR]) +
                        abs(DisLine::tabs[DisLine::T_HEX]);

    bool wrapped = false; // needed to avoid infinite loop!
    while (true) {
        // move forward/back one line
        if (fwd) {
            // forward search
            // if current line is not _end, move to next line
            if (addr != _end) {
                addr.next_line();
            }
            // if next line is _end, wrap around
            if (addr == _end) {
                addr = _start;
                error("wrapped around...");
                _errbeep = true;
                // special case if search started at _end
                if (wrapped && _sel == _end) {
                    break;
                }
                wrapped = true;
            }
        } else {
            // reverse search
            // if current line is _start, wrap around
            if (addr == _start) {
                addr = _end;
                error("wrapped around...");
                _errbeep = true;
                // special case if search started at _end
                if (_sel == _end) {
                    break;
                }
            }
            addr.prev_line();
        }

        // check if finished wrapping around
        if (addr == _sel) {
            break;
        }

        // disassemble current line
        char s[256]; // storage for line text
        disline.get_text(addr, s);
        if (strlen(s) >= skiphex) { // blank lines happen!
            memcpy(s, s+skiphex, strlen(s)-skiphex+1); // remove addr/data
            rem_blank(s); // remove excess blanks from s
            // if found, move screen to show line and set _sel
            if (strcasestr(s, search)) {
                _sel = addr;
                recenter(true);
                print_screen();
                return;
            }
        }
    }

    char s[256];
    sprintf(s, "not found: %s", search);
    error(s);
    print_screen();
}


// =====================================================
// attempt to load a comment for the current line
// returns false if comments are not allowed for the line
bool DisScrn::load_comment()
{
    // must be "line 0" or "line 1 && ATTR_LF0"
    if (rom._base <= _sel.addr && _sel.addr < rom.get_end() &&
        _sel.line - !!(rom.get_attr(_sel.addr) & ATTR_LF0) == 0) {

        // default to no comment
        _cmd[0] = 0;

        // attempt to get comment for the address
        const char *s = cmt.get_sym(_sel.addr);
        if (s) {
            strcpy(_cmd, s);
        }

        _cmd_col = 0;
        _cmd_pos = strlen(_cmd); // start at end of _cmd

        return true;
    }

    return false;
}


// =====================================================
void DisScrn::do_comment()
{
    // updated comment string is in _cmd
    // comment address is in _sel.addr

    // see if comment has changed
    const char *old = cmt.get_sym(_sel.addr);
    if (!old || strcmp(old, _cmd)) {
        cmt.set_sym(_sel.addr, _cmd);
        rom._changed = true;
        print_screen();
    }
}


// =====================================================
// change this line to type = mAscii
void DisScrn::do_cmd_a()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 60) {
        len = 60; // cap the length
    }

    _rpt_key = 'a';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len, mAscii);

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mAscii
void DisScrn::do_cmd_ebc()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 60) {
        len = 60; // cap the length
    }

    _rpt_key = '_';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len, mEbcdic);

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mByte
void DisScrn::do_cmd_b()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 32) {
        len = 32; // cap the length
    }

    _rpt_key = 'b';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len, mByte);

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mHex
void DisScrn::do_cmd_h()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 32) {
        len = 32; // cap the length
    }

    _rpt_key = 'h';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len, mHex);

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mWord
void DisScrn::do_cmd_w()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 20) {
        len = 20; // cap the length
    }

    _rpt_key = 'w';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len * 2, mWord);

    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mWord1
// Note that the maximum count is 1. The purpose of mWord1
// is to allow "word+1" to be an address reference, for 6502
// jump tables, and only one reference is supported per line.
void DisScrn::do_cmd_w1()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 1) {
        len = 1; // cap the length
    }

    _rpt_key = 0x17;
    _rpt_count = len;

    rom.set_instr(_sel.addr, len * 2, mWord1);

    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mRWord
void DisScrn::do_cmd_W()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 20) {
        len = 20; // cap the length
    }

    _rpt_key = 'W';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len * 2, mRWord);

    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mDecW
void DisScrn::do_cmd_d()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 20) {
        len = 20; // cap the length
    }

    _rpt_key = 'd';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len * 2, mDecW);

    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mDecL
void DisScrn::do_cmd_D()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 16) {
        len = 16; // cap the length
    }

    _rpt_key = 'D';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len * 4, mDecL);

    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mBin
void DisScrn::do_cmd_I()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 16) {
        len = 16; // cap the length
    }

    _rpt_key = 'I';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len, mBin);

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mBinX
void DisScrn::do_cmd_X()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 16) {
        len = 16; // cap the length
    }

    _rpt_key = 'X';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len, mBinX);

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mBinO
void DisScrn::do_cmd_O()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 16) {
        len = 16; // cap the length
    }

    _rpt_key = 'O';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len, mBinO);

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mOfs
void DisScrn::do_cmd_o()
{
    // set up for repeat
    _rpt_key = 'O';
    _rpt_count = 0;

    // the hard part is getting the refaddr for the label!
addr_t addr = _sel.addr;
(void) addr;
    char opcode[20] = {0};
    char parms[256] = {0};
    int  lfref      =  0;
    addr_t refaddr  =  0;

    // set the instruction type
    rom.set_instr(_sel.addr, 2, mOfs);

#if 0
printf(" *** addr=%.8X len=%d attr=%.2X type=%.2X mOfs=%.2X ***\r\n",
 (int) _sel.addr, rom.get_len(_sel.addr), rom.get_attr(_sel.addr), rom.get_type(_sel.addr), mOfs); fflush(stdout);
#endif

    // disassemble the instruction to get refaddr
    generic.dis_line(_sel.addr, opcode, parms, lfref, refaddr);

    // REFFLAG should be true, but might not because of
    // stuff like odd addresses or outside of rom area
    if ((lfref & REFFLAG)) {
        // make a label if the refaddr doesn't already have one
        if (!(rom.get_attr(refaddr) & ATTR_LMASK)) {
            int type = ATTR_LDATA;
            // is it already disassembled as the start of an opcode?
            if (!(rom.get_attr(refaddr) & ATTR_CONT) && rom.get_type(refaddr) >= mCode) {
                type = ATTR_LCODE;
            }
            rom.set_attr_flag(refaddr, type);
        }
}

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();
}


// =====================================================
// change this line to type = mLong
void DisScrn::do_cmd_lng()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 16) {
        len = 16; // cap the length, note that 20 longs = 259 bytes
    }

    _rpt_key = '\\';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len * 4, mLong);

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to type = mRLong
void DisScrn::do_cmd_rl()
{
    // first get the line length
    int len = _count;
    _count = 0;

    if (len == 0) {
        len = 1;
    }
    if (len > 16) {
        len = 16; // cap the length, note that 20 longs = 259 bytes
    }

    _rpt_key = '|';
    _rpt_count = len;

    rom.set_instr(_sel.addr, len * 4, mRLong);

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// change this line to code
void DisScrn::do_cmd_c()
{
    int lfref = 0;
    addr_t refaddr = 0;

    // if after code line, use next address
    if (_sel.line - rom.test_attr(_sel.addr, ATTR_LF0) > 0) {
        down_next_instr();
    }

    if (disline.make_code(_sel.addr, lfref, refaddr) < 0) {
        error("Illegal instruction");
    }

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d lf = %d ", len, lfref);
#endif
}


// =====================================================
// change lines to code until illegal or lfflag
void DisScrn::do_cmd_C()
{
    int lfref = 0;
    addr_t refaddr = 0;

    rom.save_undo();

    // if after code line, use next address
    if (_sel.line - rom.test_attr(_sel.addr, ATTR_LF0) > 0) {
        down_next_instr();
    }

    while (true) {
        int ret = disline.make_code(_sel.addr, lfref, refaddr);

        // stop if illegal instruction
        if (ret < 0) {
            //error("Illegal instruction");
            break;
        }

        // stop if reached already disassembled code
        if (ret == 0) {
            break;
        }

//***FIXME: probably need to set ATTR_LCODE on refaddr here

        // move down a line to take advantage of key repeat
        down_next_instr();//key_down_arrow();

        // stop if lfflag on this instruction
        if (lfref & LFFLAG) {
            break;
        }

        // stop if rip-stop activated
        if (lfref & RIPSTOP) {
            error(BEL "rip-stop activated");
            break;
        }
    }

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d lf = %d ", len, lfref);
#endif
}


// =====================================================
#define MAX_TRACE_STACK 1024 // maximum trace stack size
addr_t trace_stack[MAX_TRACE_STACK] = {0}; // addresses to trace
int sp = 0; // next unused location in stack[]

void reset_stack()
{
    sp = 0;
}


// =====================================================
void push_stack(addr_t value)
{
    if (sp > MAX_TRACE_STACK - 1) {
        return; // yeah, whatever
    }
    if (value) { // zero is the special end value
        trace_stack[sp++] = value;
    }
}


// =====================================================
addr_t pop_stack()
{
    if (sp) {
        return trace_stack[--sp];
    } else {
        return 0;
    }
}


// =====================================================
// trace disassemble from addr
void DisScrn::do_trace(addr_t addr)
{
    int lfref = 0;
    addr_t refaddr = 0;
    addr_t nextaddr = 0;

    rom.save_undo();

    reset_stack();
    bool firsttime = true; // to allow tracing to start at 0000
    // loop for every address to trace
    while (firsttime || addr) {
        // for every valid instruction
        while (true) {
            // stop if end of binary
            if (addr >= _end.addr) {
                break;
            }

            // disassemble the next instruction and define label if necessary
            int ret = disline.make_code(addr, lfref, refaddr);

            // stop if illegal instruction
            if (ret < 0) {
                break;
            }

            // stop if reached already disassembled code
            if (ret == 0) {
                break;
            }

            // if CODEREF and target address not already >= mCode, add to stack
            if ((lfref & CODEREF) && !rom.AddrOutRange(addr)
                                  && !(lfref & RIPSTOP)
                                  && rom.get_type(refaddr) < mCode) {
                // check for indirect reference
                switch (lfref & INDMASK) {
                    case INDNONE:
                        // it's okay
                        break;

                    case INDWBE:
                        // read indirect 16-bit big-endian
                        refaddr = (rom.get_data(refaddr) * 256 +
                                   rom.get_data(refaddr + 1))
                                  * defCpu->word_size();
                        break;

                    case INDWLE: // |
                    case INDLBE: //  >-- to be implemented later as needed
                    case INDLLE: // |
                    default:
                        // no idea what it is, ignore it
                        refaddr = 0;
                        break;
                }

                if (refaddr) {
                    push_stack(refaddr);
                }
            }

            // advance to next instruction
            addr += rom.get_len(addr);

            // add skip reference
            if (lfref & SKPMASK) {
                push_stack(addr + ((lfref & SKPMASK) >> SKPSHFT) - 1);
            }

            // stop if lfflag on this instruction
            if (lfref & LFFLAG) {
                break;
            }

            // stop if rip-stop activated
            if (lfref & RIPSTOP) {
                error(BEL "rip-stop activated");
                break;
            }
        }

        // if this is the end of the first block, save the addr
        if (firsttime && addr != _sel.addr) {
            nextaddr = addr;
        }
        firsttime = false;

        // get next address to disassemble
        addr = pop_stack();
    }

    // set the addr to the end of the first group
    if (nextaddr) {
        _sel.line_start(nextaddr);
    }
}


// =====================================================
// trace disassemble from _sel
void DisScrn::do_cmd_T()
{
    // start at the current _sel address
    addr_t addr = _sel.addr;
    if (_sel.line - rom.test_attr(addr, ATTR_LF0) > 0) {
        // if after code line, use next address
        addr += _sel.get_len();
    }

    do_trace(addr);

    // recalculate the screen
    print_screen();
}


// =====================================================
// trace disassemble from refaddr
void DisScrn::do_cmd_cT()
{
    // save current selection
    addrline_t save = _sel;

    // make current address a word
    if (rom.get_type(_sel.addr) != mWord) {
        rom.set_instr(_sel.addr, 2, mWord);
    }

    // get refaddr for current line
    addr_t addr = get_refaddr();
    // exit if no refaddr
    if (!addr) {
        return;
    }

    // don't trace if refaddr is already code for the current CPU
    if (rom.get_type(addr) != curCpu->_id) {
        do_trace(addr);

        // confirm that something happened
        char s[256];
        sprintf(s, "%.4X traced", (int) addr);
        error(s);
    }

    // if addr has no label but now it's code, make a code label
    if ((rom.get_attr(addr) & ATTR_LMASK) == ATTR_LNONE) {
        if (rom.get_type(addr) == curCpu->_id) {
            rom.set_attr(addr, rom.get_attr(addr) | ATTR_LCODE);
        }
    }

    // return cursor to previous selection
    _sel = save;

    // move to next sel addr
    _sel.next_addr();

    // recalculate the screen
    print_screen();
}


// =====================================================
// clean label if not referenced from elsewhere
void DisScrn::do_cmd_cln()
{
    char s[256];

    // get appropriate addr for current line, including $-n labels
    int line = _sel.line;
    addr_t label = _sel.addr;
    if (rom.test_attr(label, ATTR_LF0)) {
        line--; // remove pre-LF from line count
    }
    if (line <= 0) {
        // on code line or pre-LF line, use selection address
        // do nothing if line does not have a label
        if (!rom.test_attr(_sel.addr, ATTR_LMASK)) {
            label = 0;
        }
    } else {
        // on EQU $-xx line, use the matching offset label
        while (line--) {
            // go to next address
            label++;
            // find next offset label
            while (label < rom.get_end() &&
                   !(rom.test_attr(label, ATTR_LMASK))) {
                label++;
            }
            // test if end of rom (this really should not happen!)
            if (label >= rom.get_end()) {
                return;
            }
        }
        // label should now be the address of the selected line
    }

#if 0
    // display label address for debugging
    if (label) {
        sprintf(s, "label addr = %.4X", (int) label);
        error(s);
    }
#endif

    if (label) {
        int scale = defCpu->word_size();

        // start at beginning of rom
        addr_t ad = rom._base;

        // disassemble every line that is not mData
        bool found = false;
        while (ad < rom.get_end()) {
            int len = 0;
            if (rom.get_type(ad) != mData && !rom.test_attr(ad, ATTR_CONT)) {
                // disassemble the address
                int lfref = 0;
                addr_t refaddr = 0;
                len = disline.get_dis_line(ad, s, lfref, refaddr);
                // if reference was found, stop looking
                if ((lfref & (REFFLAG|CODEREF)) && (refaddr == label)) {
                    if (defCpu->_radix & RAD_OCT) {
                        sprintf(s, "Found label %.6o at %.6o", (int) label / scale,
                                                               (int) ad / scale);
                    } else {
                        sprintf(s, "Found label %.4X at %.4X", (int) label / scale,
                                                               (int) ad / scale);
                    }
                    error(s);
                    found = true;
                    break;
                }
            }
            // advance to next instruction
            if (len) {
                ad += len;
            } else {
                ad++;
            }
        }
        if (!found) {
            // label was not referenced, so delete it here
            if (defCpu->_radix & RAD_OCT) {
                sprintf(s, "Deleted label at %.6o", (int) label / scale);
            } else {
                sprintf(s, "Deleted label at %.4X", (int) label / scale);
            }
            error(s);
            rom.clear_attr_flag(label, ATTR_LMASK);
        }
    }

//  recenter(true);
    print_screen();

}


// =====================================================
// toggle label type
void DisScrn::do_cmd_lbltyp()
{
    // ignore if on LF0 pre-blank line
    int line = _sel.line;
    if (rom.test_attr(_sel.addr, ATTR_LF0)) {
        if (line == 0) {
            return;
        }
        line--;
    }

    // check if it might be an EQU $-n line
    if (line) {
        addr_t addr = _sel.addr + 1;
        while (line) {
            int attr = rom.get_attr(addr);
            if (!(attr & ATTR_CONT)) {
                // past end of instruction group
                return;
            }
            // found a label, count down
            if (attr & ATTR_LMASK) {
                line--;
            }
            // if not desired label, move to next addr
            if (line) {
                addr++;
            }
        }

        // don't cycle it, just turn it off
        rom.clear_attr_flag(addr, ATTR_LMASK);

        // recalculate the screen
        print_screen();

        return;
    }

    // get current label status
    int lbl = rom.get_attr(_sel.addr) & ATTR_LMASK;

    // increment label type (skipping ATTR_LXXXX for now)
    lbl++;
    if (lbl > ATTR_LCODE) {
        lbl = ATTR_LNONE;
    }

    // update label type
    rom.clear_attr_flag(_sel.addr, ATTR_LMASK);
    rom.set_attr_flag(_sel.addr, lbl);

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// toggle label type at refadr
void DisScrn::do_cmd_reflbltyp()
{
    char s[256] = {0};
    int lfref = 0;
    addr_t refaddr = 0;

    // try to get refadr for current line
    int len = disline.get_dis_line(_sel.addr, s, lfref, refaddr);

    wrefresh(_win);

    if (len > 0 && (lfref & REFFLAG) && (refaddr != 0)) {
        // get current label status
        int lbl = rom.get_attr(refaddr) & ATTR_LMASK;

        // increment label type (skipping ATTR_LXXXX for now)
        lbl++;
        if (lbl > ATTR_LCODE) {
            lbl = ATTR_LNONE;
        }

        // update label type
        rom.clear_attr_flag(refaddr, ATTR_LMASK);
        rom.set_attr_flag(refaddr, lbl);

        // recalculate the screen
        print_screen();
    }
}


// =====================================================
// get refadr for _sel, returns zero if none
addr_t DisScrn::get_refaddr()
{
    char s[256] = {0};
    int lfref = 0;
    addr_t refaddr = 0;

    // try to get refadr for current line
    int len = disline.get_dis_line(_sel.addr, s, lfref, refaddr);

    // check that instruction is valid has a refaddr
    if (len > 0 && (lfref & REFFLAG) && (refaddr != 0)) {
        // if in image range, return the refaddr
        if (rom._base <= refaddr && refaddr <= rom.get_end()) {
            return refaddr;
        }
    }

    return 0;
}


// =====================================================
// goto refaddr
void DisScrn::do_cmd_ref()
{
    addr_t refaddr = get_refaddr();

    if (refaddr) {
        // save current _sel.addr on stack
        push_addr(_sel.addr, refaddr);
        // select line and redraw screen
        _sel.line_start(refaddr);
        recenter(true);
        print_screen();
    }
}


// =====================================================
// goto previous label
void DisScrn::do_cmd_prv()
{
    if (_sel.addr > rom._base) {
        _sel.prev_addr();
        while (_sel.addr > rom._base && !rom.test_attr(_sel.addr, ATTR_LMASK)) {
            // find previous line
            _sel.prev_addr();
        }
        // check for pre-LF line
        if (rom.test_attr(_sel.addr, ATTR_LF0)) {
            _sel.next_line();
        }
        recenter(true);
        print_screen();
    }
}


// =====================================================
// goto next label
void DisScrn::do_cmd_nxt()
{
    if (_sel.addr < rom.get_end()) {
        _sel.next_addr();
        while (_sel.addr < rom.get_end() && !rom.test_attr(_sel.addr, ATTR_LMASK)) {
            // find next line
            _sel.next_addr();
        }
        // check for pre-LF line
        if (rom.test_attr(_sel.addr, ATTR_LF0)) {
            _sel.next_line();
        }
        recenter(true);
        print_screen();
    }
}


// =====================================================
// goto stacked addr
void DisScrn::do_cmd_bak()
{
    addr_t addr = pop_addr();
    if (addr) {
        // select line and redraw screen
        _sel.line_start(addr);
        recenter(true);
        print_screen();
    }
}


// =====================================================
// goto next stacked addr
void DisScrn::do_cmd_fwd()
{
    addr_t addr = un_pop_addr();
    if (addr) {
        // select line and redraw screen
        _sel.line_start(addr);
        recenter(true);
        print_screen();
    }
}


// =====================================================
// max length and element size for each data type

struct types_info_t {
    uint8_t typ; // type code
    uint8_t max; // maximum number of elements on a line
    uint8_t siz; // element size
} types_info[] = {
    { mByte,   32, 1 },
    { mWord,   20, 2 },
    { mLong,   16, 4 },
    { mAscii,  60, 1 },
    { mRWord,  20, 2 },
    { mRLong,  16, 4 },
    { mHex,    32, 1 },
    { mBin,    16, 1 },
    { mBinX,   16, 1 },
    { mBinO,   16, 1 },
    { mEbcdic, 60, 1 },
    { mDecW,   20, 2 },
    { mDecL,   16, 4 },
    { mData,    0, 1 } // end of list
};


// get max length for a data type
int get_type_max(uint8_t typ) {
    for (types_info_t *p = types_info; p->max; p++) {
        if (p->typ == typ) {
            return p->max * p->siz;
        }
    }
    return 0; // type not found
}


// get element size for a data type
int get_type_size(uint8_t typ) {
    for (types_info_t *p = types_info; p->max; p++) {
        if (p->typ == typ) {
            return p->siz;
        }
    }
    return 1; // return size = 1 for invalid types
}


// =====================================================
// less data
void DisScrn::do_cmd_les()
{
    // verify that this is a command with a length
    int typ = rom.get_type(_sel.addr);
    int max = get_type_max(typ);

    // if this is an undefined byte, try extending the previous line
    if (typ == mData) {
        addrline_t a = _sel;
        a.prev_line();
        typ = rom.get_type(a.addr);
        max = get_type_max(typ);
        if (max) {
            _sel = a;
        }
    }

    if (!max) {
        return;
    }

    addr_t a = _sel.addr + 1;

    // find end of run and determine length
    int len = 1;
    while (a < rom.get_end() && rom.test_attr(a, ATTR_CONT)) {
        a++;
        len++;
    }

    int siz = get_type_size(typ);

    // verify length is at least 2x siz
    if (len < siz * 2) {
        return;
    }

    // set all abandoned bytes to mData
    --a;
    while (a > rom._base && rom.test_attr(a, ATTR_CONT) && siz--) {
        rom.set_type(a, mData);
        rom.clear_attr_flag(a, ATTR_CONT | ATTR_LF1);
        --a;
    }

    print_screen();
}


// =====================================================
// more data
void DisScrn::do_cmd_mor()
{
    // verify that this is a command with a length
    int typ = rom.get_type(_sel.addr);
    int max = get_type_max(typ);

    // if this is an undefined byte, try extending the previous line
    if (typ == mData) {
        addrline_t a = _sel;
        a.prev_line();
        typ = rom.get_type(a.addr);
        max = get_type_max(typ);
        if (max) {
            _sel = a;
        }
    }

    if (!max) {
        return;
    }

    addr_t a = _sel.addr + 1;

    // find end of run and determine length
    int len = 1;
    while (a < rom.get_end() && rom.test_attr(a, ATTR_CONT)) {
        a++;
        len++;
    }

    // verify length is < max
    if (len >= max) {
        return;
    }

    // add new bytes equal to the size of one element
    int siz = get_type_size(typ);
    while (a < rom.get_end() && siz--) {
        // add new byte(s) to current run
        rom.set_type(a, typ);
        rom.set_attr_flag(a, ATTR_CONT);
        rom.clear_attr_flag(a, ATTR_LF0 | ATTR_LF1);
        a++;
    }

    // update upcoming ATTR_CONT bytes with type mData
    while (a < rom.get_end() && rom.test_attr(a, ATTR_CONT)) {
        rom.set_type(a, mData);
        rom.clear_attr_flag(a, ATTR_CONT | ATTR_LF1);
        a++;
    }

    print_screen();
}


// =====================================================
// handle '"' command to change hint flags of current instr

void DisScrn::do_cmd_hint()
{
    // toggle hint flags in current instr
    int hint = rom.get_hint(_sel.addr);
    hint = (hint + 1) & 3;
    rom.set_hint(_sel.addr, hint);

    // print message with new hint value
    char s[256];
    sprintf(s, "instr hint set to %d", hint);
    error(s);

    // re-disassemble current instruction and get its new length
    CPU *cpu = CPU::get_cpu(rom.get_type(_sel.addr));
    char opcode[20] = {0};
    char parms[256] = {0};
    int lfref = 0;
    addr_t refaddr = 0;
    int len = cpu->dis_line(_sel.addr, opcode, parms, lfref, refaddr);

    // update length and clean up broken instructions that follow
    rom.set_instr(_sel.addr, len, rom.get_type(_sel.addr), !!(lfref & LFFLAG));

    // redraw everything
    print_screen();
}


// =====================================================
// handle '$' command to change default hint flags

void DisScrn::do_cmd_defhint()
{
    // increment default hint value
    rom._defhint = (rom._defhint + 1) & 3;

    // print message with new defhint value
    char s[256];
    sprintf(s, "defhint set to %d", rom._defhint);
    error(s);
}


// =====================================================
// recenter screen
void DisScrn::do_cmd_cen()
{
    recenter(true);
    print_screen();
}


// =====================================================
// recenter screen
void DisScrn::do_cmd_top()
{
    // thresholds are numlines/5 and numlines*4/5
    int threshold = max_row() / 5;
    if (threshold > 10) {
        threshold = 10;
    }

    set_sel_line(threshold);
    print_screen();
}

void DisScrn::do_cmd_center()
{
    set_sel_line(max_row() / 2);
    print_screen();
}



// =====================================================
// toggle the ATTR_LF0 flag indicating a blank line above this one
void DisScrn::do_cmd_Open()
{
    // toggle the ATTR_LF0 flag
    rom.toggle_attr_flag(_sel.addr, ATTR_LF0);

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// toggle outbound refs
void DisScrn::do_cmd_cR()
{
    // ignore unless on the "main" line of the instruction
    // this prevents hidden label lines from changing the "main" line
    if (rom.test_attr(_sel.addr, ATTR_LF0)) {
        if (_sel.line != 1) return;
    } else {
        if (_sel.line != 0) return;
    }

    // toggle the ATTR_NOREF flag
    rom.toggle_attr_flag(_sel.addr, ATTR_NOREF);

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// save undo buffer
void DisScrn::do_cmd_cU()
{
    rom.save_undo();
    print_screen();
}


// =====================================================
// swap undo buffer
void DisScrn::do_cmd_U()
{
    rom.swap_undo();
    print_screen();
}


// =====================================================
// change this line to type = mData
void DisScrn::do_cmd_x()
{
    // first get the instruction length
    int len = 1;

    // get the range of attribute bytes to type=mCode
    // set_instr will handle setting all attr/type bytes
    // and smash a following partial instruction back to mData
    rom.set_instr(_sel.addr, len, mData);

    // move down a line to take advantage of key repeat
    down_next_instr();//key_down_arrow();

    // recalculate the screen
    print_screen();

#if 0
    wmove(_win, 1, 40);
    wprintw(_win, " len = %d ", len);
#endif
}


// =====================================================
// void cmd_key(int key)
//
// This handles a single-key command
// Returns non-zero to exit program.

void DisScrn::cmd_key(int key)
{
    // search for key in char_cmds table
    for (cmd_char_t *p = char_cmds; p->key; p++) {
        if (key == p->key) {
            error(NULL);
            (scrn.*(p->func))();
            return;
        }
    }

    switch (key) {
        // digits give a count for single-char commands
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            error(NULL);
            // keep track of the last two digits in _count
            _count = (_count % 10) * 10 + key - '0';
            status_line();
            wrefresh(_win);
            break;

        // backspace key lops off the last count digit entered
        case 0x08: // BS
        case 0x7F: // DEL
        case KEY_BACKSPACE: // curses backspace key code
            error(NULL);
            if (_count) {
                _count = _count / 10;
                status_line();
                wrefresh(_win);
            }
            break;

        case 0x1B: // ESC key clears count
            _count = 0;
            status_line();
            wrefresh(_win);
            break;
    }
}


// =====================================================
// void do_key_rpt()
//
// This repeats the most recent data change command.

void DisScrn::do_cmd_rpt()
{
    // search for key in char_cmds table
    for (cmd_char_t *p = char_cmds; p->key; p++) {
        if (_rpt_key == p->key) {
            error(NULL);
            _count = _rpt_count;
            (scrn.*(p->func))();
            return;
        }
    }

    // either no _rpt_key has been saved or it was wrong
    // just silently fail?
}


// =====================================================
// void input_key(int key)
//
// This handles a key event from the event loop.
// Returns non-zero to exit program.

void DisScrn::input_key(int key)
{
    int len = strlen(_cmd);

    switch (key) {
        case -1: // no key
            break;

        // Some keys that shouldn't work in this mode, to let
        // you know "hey dummy, look at the top of the screen!"
        case KEY_DOWN:
        case KEY_UP:
            beep();
            break;

        case 0x08: // BS
        case 0x7F: // DEL
        case KEY_BACKSPACE: // curses backspace key code
            if (_cmd_pos) {
                _cmd_pos--;
                memcpy(_cmd + _cmd_pos, _cmd + _cmd_pos + 1, len - _cmd_pos + 1);
                print_screen();
            } else if (!len) {
                if (_in_input == ';') {
                    // require ESC or ENTER to end a comment
                    break;
                }
                // backspace past beginning exits command input
                _in_input = 0;
                print_screen();
            }
            break;

        case KEY_DC: // delete-character key (forward delete)
            if (_cmd_pos < len) {
                memcpy(_cmd + _cmd_pos, _cmd + _cmd_pos + 1, len - _cmd_pos + 1);
                print_screen();
            }
            break;

        case 0x0D: // CR
        {
            key = _in_input;    // get input prompt char
            _in_input = 0;      // disable input
            error("");          // clear error message
            print_screen();     // refresh screen
            // do something with _cmd here
            switch (key) {
                case ':':
                    do_cmd_line();
                    break;

                case '/':
                case '?':
                    // if search string entered, use it
                    if (_cmd[0]) {
                        strcpy(_search, _cmd);
                    }
                    do_search(key == '/');
                    break;

                case ';':
                    do_comment();
                    break;

                case 'l':
                    do_label_sel();
                    break;

                case 'L':
                    do_label_refaddr();
                    break;

                default:
                    // shouldn't get here
                    break;
            }
            break;
        }

        case 0x01: // ctrl-A
            // move to beginning of line
            if (_cmd_pos) {
                _cmd_pos = 0;
                print_screen();
            }
            break;

        case 0x05: // ctrl-E
            // move to end of line
            if (_cmd_pos < len) {
                _cmd_pos = len;
                print_screen();
            }
            break;

        case 0x1B: // ESC
            _in_input = false;
            _cmd[0] = 0;
            print_screen();
            break;

        case KEY_LEFT:
            if (_cmd_pos) {
                _cmd_pos--;
                print_screen();
            }
            break;

        case KEY_RIGHT:
            if (_cmd_pos < len) {
                _cmd_pos++;
                print_screen();
            }
            break;

        case 0x15: // ctrl-U
        case 0x18: // ctrl-X
            // delete from cursor to beginning of line
            if (_cmd_pos) {
                memcpy(_cmd, _cmd + _cmd_pos, len - _cmd_pos + 1);
                _cmd_pos = 0;
                print_screen();
            }
            break;

        default:
            // non-text characters not allowed
            if ((key < ' ') || (key > '~')) {
#if 0
                // debug strange keys
                wmove(_win, 1, 20);
                wprintw(_win, " >0x%.2X< ", key);
                wrefresh(_win);
#endif
                break;
            }

            // check for cmd buffer full or end of screen
            if (len >= sizeof _cmd - 1) {
                break;
            }

            // make an insert space
            memcpy(_cmd + _cmd_pos + 1, _cmd + _cmd_pos, len - _cmd_pos + 1);

            // add character to buffer
            _cmd[_cmd_pos] = key;
            _cmd_pos++;

            print_screen();
            break;
    }
}


// =====================================================
// void do_key(int c)
//
// This handles a key event from the event loop.
// Returns non-zero to exit program.

void DisScrn::do_key(int key)
{
    // make a beep ONCE, if requested
    if (_errbeep) {
        beep();
        _errbeep = false;
    }

    // acknowledge if a ctrl-C came in
    if (_ctrl_c) {
        _in_input = false; // can't display error message over input line!
        error("use ':q' to quit");
        _ctrl_c = false; // we've handled it, not stuck!
    }

    // if SIGWINCH happened, and no key to send, pass a KEY_RESIZE down
    if (key <= 0 && sigwinched) {
         key = KEY_RESIZE;
         sigwinched = false;
    }

    // handle command-line input
    if (_in_input && key != KEY_RESIZE) {
        input_key(key);
        return;
    }

    switch (key) {
        case 0x02: // ctrl-B, top-of-page/pgup
            if (_sel_row == 0) {
                key_page_up();
            } else {
                top_of_page();
            }
            print_screen();
            break;

        case 0x04: // ctrl-D, bottom-of-page/pgdn
            if (_sel_row == max_row()) {
                key_page_down();
            } else {
                bot_of_page();
            }
            print_screen();
            break;

        case KEY_UP: // up arrow
        case 'k': // (vi) move up a line
            key_up_arrow();
            print_screen();
            break;

        case KEY_DOWN: // down arrow
        case 'j':      // (vi)
            key_down_arrow();
            print_screen();
            break;

        case KEY_HOME:  // Home
            key_home();
            print_screen();
            break;

        case KEY_END:   // End
        case 'G':       // XXX should also take _count to move to a given line
            key_end();
            print_screen();
            break;

        case KEY_PPAGE: // PgUp
            key_page_up();
            print_screen();
            break;

        case ' ': // space bar
        case KEY_NPAGE: // PgDn
        case 0x06: // ctrl-F: page forward (vi)
            key_page_down();
            print_screen();
            break;

        case 0x0C:       // Ctrl-L (vi) refresh scren
        case KEY_RESIZE: // display size change (SIGWINCH)
            // note: these first two lines are normally done
            // in the ncurses SIGWINCH handler
            endwin();
            wrefresh(stdscr);
            // now redraw the screen in the new display size
            print_screen();
            break;

        case ';': // start a comment
            // make sure line can have a comment, and get existing comment
            if (load_comment()) {
                _in_input = key;
                print_screen();
            }
            break;

        case ':': // start command line
        case '/': // start search forward
        case '?': // start search backward
        case 'L': // label current line
        case 'l': // label target of current line
            error(NULL);
            _cmd[0] = 0;
            _cmd_col = 0;
            _cmd_pos = 0;
            _in_input = key;
            print_screen();
            break;

        default: // unknown key
            cmd_key(key);
            break;
    }
}


// =====================================================
//    void key_up_arrow()

void DisScrn::key_up_arrow()
{
    // can't go up from start address
    if (_sel == _start) {
        return;
    }

    // at top screen row?
    if (_sel_row <= 0) {
        // if at top line, scroll screen down one line
        _top.prev_line();
        _sel.prev_line();
    } else {
        // decrement selected line
        _sel_row--;
        _sel.prev_line();
    }
}


// =====================================================
void DisScrn::down_next_instr()
{
    // can't go down from end address
    if (_sel == _end) {
        return;
    }

    // keep going down until _sel.addr changes
    addr_t oldaddr = _sel.addr;
    while (_sel.addr == oldaddr) {
        // at bottom screen row? (leave one blank line just in case)
        if (_sel_row >= max_row()-1) {
            // if at actual bottom line, move top down one extra
            // there are cases when the number of lines for a full
            // screen changes, and print_screen() fails
            if (_sel_row >= max_row()) {
                _top.next_line();
            }
            // if at bottom line, scroll screen up one line
            _top.next_line();
            _sel.next_line();
        } else {
            // increment selected line
            _sel_row++;
        _sel.next_line();
        }
    }
}


// =====================================================
//    void key_down_arrow()

void DisScrn::key_down_arrow()
{
    // can't go down from end address
    if (_sel == _end) {
        return;
    }

    // at bottom screen row?
    if (_sel_row >= max_row()) {
        // if at bottom line, scroll screen up one line
        _top.next_line();
        _sel.next_line();
    } else {
        // increment selected line
        _sel_row++;
        _sel.next_line();
    }
}


// =====================================================
//    void key_page_up()

void DisScrn::key_page_up()
{
    // roll screen up by one page, attempting to keep same
    // line on screen selected, then start moving selection
    // up until moved full page or hit top of document

    int n = max_row() + 1; // maximum number of lines to move

    // attempt to roll while not at top of document
    while (n && _top != _start) {

        // scroll screen up and move selection up by one line
        _top.prev_line();
        _sel.prev_line();

        n--;
    }

    // docuemnt fully scrolled to top, now finish with up-arrows

    while (n--) {
        key_up_arrow();
    }
}


// =====================================================
//    void key_page_down()

void DisScrn::key_page_down()
{
    // roll screen down by one page, attempting to keep same
    // line on screen selected, then start moving selection
    // down until moved full page or hit end of document

    int maxy = max_row() + 1;

    // get address of bottom line on screen for compare
    addrline_t bot = _top;
    for (int i = 0; i <= maxy; i++) {
        bot.next_line();
    }

    int n = maxy; // maximum number of lines to move
    // attempt to roll while not at bottom of document
    while (n) {
        // are there still lines left beyond the screen?
        int i = maxy;
        addrline_t addr = _top;
        while (i && addr != _end) {
            addr.next_line();
            i--;
        }
        if (i) {
            break; // nope, the end line is at the bottom of the screen
        }

        // scroll screen down and move selection down by one line
        _top.next_line();
        _sel.next_line();

        n--;
    }

    // docuemnt fully scrolled to bottom, now finish with down-arrows

    while (n--) {
        key_down_arrow();
    }
}


// =====================================================
//    void top_of_page()

void DisScrn::top_of_page()
{
    // move selection up to first visible line
    while (_sel_row > 0 && _sel != _top) {
        key_up_arrow();
    }
}


// =====================================================
//    void bot_of_page()

void DisScrn::bot_of_page()
{
    // move selection down to last visible line
    while (_sel_row < max_row() && _sel != _end) {
        key_down_arrow();
    }
}


// =====================================================
//    void key_home()

void DisScrn::key_home()
{

    // move to first line of document
    _sel = _start;
    _top = _sel;
    _sel_row = 0;
}


// =====================================================
//    void key_end()

void DisScrn::key_end()
{

    // move to last line of document (this would just show the END line)
    _sel = _end;
    _top = _sel;
    _sel_row = 0;

    // try to move up by max_row lines
    int i;
    for (i = 1; i <= max_row() && _top != _start; i++) {
        key_up_arrow();
    }

    // now go back down by the same number of lnes
    // yes this is cheesy, but it works well enough for now
    while (--i) {
        key_down_arrow();
    }
}


// =====================================================
// string utility functions
// =====================================================

// =====================================================
int ishex(char c)
{
    c = toupper(c);
    return isdigit(c) || ('A' <= c && c <= 'F');
}


// =====================================================
int HexVal(const char *hexStr)
{
    addr_t  hexVal;
    int     evalErr;
    int     c;

// need to handle errors here better, if only in returning the value of the first valid digits!

    evalErr = false;
    hexVal  = 0;

    while ((c = toupper(*hexStr++))) {
        if (!ishex(c)) {
            evalErr = true;
        } else {
            if (c > '9') {
                c = c - 'A' + 10;
            } else {
                c = c - '0';
            }
            hexVal = hexVal * 16 + c;
        }
    }

    if (evalErr) {
        hexVal = 0;
//      Error("Invalid hexadecimal number");
    }

    return hexVal;
}


// =====================================================
bool HexValid(const char *hexStr)
{
    // empty string is not valid
    if (!hexStr[0]) {
        return false;
    }

    // any non-hex character is not valid
    while (*hexStr) {
        if (!ishex(*hexStr++)) {
            return false;
        }
    }

    return true;
}


// =====================================================
int OctVal(const char *octStr)
{
    addr_t  octVal;
    int     evalErr;
    int     c;

    evalErr = false;
    octVal  = 0;

    while ((c = toupper(*octStr++))) {
        if ((c < '0') || (c > '7')) {
            evalErr = true;
        } else {
            c = c - '0';
            octVal = octVal * 8 + c;
        }
    }

    if (evalErr) {
        octVal = 0;
//      Error("Invalid octal number");
    }

    return octVal;
}


// =====================================================
bool OctValid(const char *octStr)
{
    // empty string is not valid
    if (!octStr[0]) {
        return false;
    }

    while (*octStr) {
        char c = *octStr++;
        if ((c < '0') || (c > '7')) {
            return false;
        }
    }

    return true;
}


// =====================================================
int HexOctVal(const char *hexStr)
{
    switch (toupper(*hexStr)) {
        case '$':
        case 'H':
            return HexVal(hexStr+1);
        case 'O':
            return OctVal(hexStr+1);
        default:
            if (defCpu->_radix & RAD_OCT) {
                return OctVal(hexStr);
            } else {
                return HexVal(hexStr);
            }
    }
}


// =====================================================
bool HexOctValid(const char *hexStr)
{
    switch (toupper(*hexStr)) {
        case '$':
        case 'H':
            return HexValid(hexStr+1);
        case 'O':
            return OctValid(hexStr+1);
        default:
            if (defCpu->_radix & RAD_OCT) {
                return OctValid(hexStr);
            } else {
                return HexValid(hexStr);
            }
    }
}


// =====================================================
uint32_t DecVal(const char *decStr)
{
    uint32_t decVal;
    int      evalErr;
    int      c;
    bool     negative;

    negative = false;
    evalErr = false;
    decVal  = 0;

    if (*decStr == '-') {
        negative = true;
        decStr++;
    }

    while ((c = *decStr++)) {
        if (!isdigit(c)) {
            evalErr = true;
        } else {
            decVal = decVal * 10 + c - '0';
        }
    }

    if (evalErr) {
        decVal = 0;
//      Error("Invalid decimal number");
    }

    if (negative) {
        decVal = -decVal;
    }

    return decVal;
}


// =====================================================
// get next word or symbol from s, and make it upper-case
// returns 0 for end-of-line; if a single char, returns that char; else -1
int GetWord(char *&s, char *word)
{
    *word = 0;
    char *p = word;

    // skip initial whitespace
    while (*s == ' ') {
        s++;
    }

    // test for end of line
    uint8_t c = *s;
    if (c) {
        while (c != ' ' && c != 0) {
            // if not first character, only allow alphanumeric strings
            if (p != word) {
                if (!isalnum(word[0])) {
                    break;
                }
            }
            // add character to word
            *p++ = toupper(c);
            c = *++s;
            // only allow single character if not alphanumeric
            if (!isalnum(c)) {
                break;
            }
        }
        *p = 0;

        // skip trailing whitespace
        while (c == ' ') {
            c = *++s;
        }

        // check for single-character token
        if (word[0] && !word[1]) {
            return word[0];
        }

        // found a multi-char word
        return -1;
    }

    // found end of line
    return 0;
}


// =====================================================
int GetString(char *&s, char *word)
{
    // skip initial whitespace
    while (*s == ' ') {
        s++;
    }

    // check for leading quote
    uint8_t c = *s;
    if (c != '"') {
        c = ' ';
    }

    // copy until the next quote or blank, and set s after the quote
    while (*s && *s != c) {
        if (*s == '\\' && *(s+1)) {
            s++;
        }
        *word++ = *s++;
    }
    *word = 0;

    // skip the second quote or first blank
    if (*s) {
        s++;
    }

    // return the run as a word
    return -1;
}


