// disscrn.h

#ifndef _DISSCRN_H_
#define _DISSCRN_H_

#include <curses.h>

#include "disx.h"
#include "disline.h"


// =====================================================
// screen handler object

class DisScrn {
private:
    friend class DisSave;

    struct addrline_t _start;   // address of first line
    struct addrline_t _end;     // address of after last line
    struct addrline_t _top;     // address of first visible line on screen
    struct addrline_t _sel;     // address of currently selected line on screen
    int _sel_row;               // screen row of currently selected line
    int _count;                 // count for single-char commands
    WINDOW * _win;              // the window being used (default to stdscr)
    char _in_input;             // prompt char when inputting a command
    char _cmd[120];             // command buffer
    char _search[120];          // most recent search string
    char _err[120];             // error string
    bool _errbeep;              // true to make an error beep
    static bool _ctrl_c;        // control-C status flag

    static const int GOTO_SIZE = 8;
    addr_t _goto_stk[GOTO_SIZE];// goto stack
    int _goto_sp;               // goto stack pointer

public:
    bool _quit;                 // true to exit program
    char _rpt_key;              // key to repeat with '*' command
    int _rpt_count;             // count to use with '*' command
    int _cmd_col;               // first visible char of _cmd
    int _cmd_pos;               // current position in _cmd

public:
//  Note: no explicit constructor, to avoid problems with constructor order
    void init_scrn(bool reset=true); // initiailize screen position parameters
    void do_key(int key);       // handle a key from user input
    void error(const char *s);  // set/clear error message
    void ctrl_c();              // called when SIGING received
    void print_screen();        // print contents of screen

private:
    void print_line(struct addrline_t addr);
    void status_line();    
    void debug_info();          // print debug info on screen

    // returns last row of screen, minus status line
    int max_row() const { return getmaxy(_win)-2; }

    int screen_row(addrline_t ad) const; // return the screen row of ad
    void set_sel_line(int row); // put _sel on a specific line of the screen
    void recenter(bool strict = false); // make sure that _sel is on screen

    void down_next_instr();     // go down to next instr
    void key_up_arrow();        // handle up-arrow key
    void key_down_arrow();      // handle down-arrow key
    void top_of_page();         // go to first line of screen
    void bot_of_page();         // go to last line of screen
    void key_page_up();         // handle pgup key
    void key_page_down();       // handle pgdn key
    void key_home();            // handle home key
    void key_end();             // handle end key
    void set_cursor();          // set cursor to where it needs to be
    void input_key(int key);    // handle key input for command line
    void cmd_key(int key);      // handle single-key command
    void do_cmd_line();         // handle command in 'cmd'
    void do_label_sel();        // label current line
    void do_label_refaddr();    // label line referenced by this line
    void do_search(bool fwd);   // do search
    bool load_comment();        // load a comment for editing
    void do_comment();          // do comment
    addr_t get_refaddr();       // get refaddr from _sel, returns zero if none
    void do_trace(addr_t addr); // do trace, common code for shift-T and ctrl-T

    void push_addr(addr_t addr, addr_t next = 0);// push address to _goto_stk
    addr_t pop_addr();          // pop address from _goto_stk
    addr_t un_pop_addr();       // un-pop address from _goto_stk

public: // can't be private unless dispatch tables are moved into the class
    void do_cmd_quit(char *p);  // handle "quit" command
    void do_cmd_list(char *p);  // handle "asm"  command
    void do_cmd_asm (char *p);  // handle "list" command
    void do_cmd_save(char *p);  // handle "save" command
    void do_cmd_wq  (char *p);  // handle "wq"   command
    void do_cmd_load(char *p);  // handle "load" command
    void do_cmd_new (char *p);  // handle "new"  command
    void do_cmd_rst (char *p);  // handle "rst"  command
    void do_cmd_go  (char *p);  // handle "go"   command
    void do_cmd_end (char *p);  // handle '$'    command
    void do_cmd_cpu (char *p);  // handle "cpu"  command
    void do_cmd_defcpu(char *p);// handle "defcpu" command
    void do_cmd_tab (char *p);  // handle "tab"  command
    void do_cmd_label(char *p);	// hanble "label" command

    void do_cmd_a();            // handle 'a' command
    void do_cmd_b();            // handle 'b' command
    void do_cmd_c();            // handle 'c' command
    void do_cmd_C();            // handle 'C' command
    void do_cmd_d();            // handle 'd' command
    void do_cmd_D();            // handle 'D' command
    void do_cmd_h();            // handle 'h' command
    void do_cmd_lng();          // handle '|' command
    void do_cmd_I();            // handle 'I' command
    void do_cmd_X();            // handle 'X' command
    void do_cmd_O();            // handle 'O' command
    void do_cmd_o();            // handle 'o' command
    void do_cmd_ebc();          // handle '_' command
    void do_cmd_rl();           // handle ctrl-'\' command
    void do_cmd_lbltyp();
    void do_cmd_Open();         // handle 'O' command
    void do_cmd_cR();           // handle ctrl-'R' command
    void do_cmd_cU();           // handle ctrl-'U' command
    void do_cmd_T();            // handle 'T' command
    void do_cmd_cT();           // handle ctrl-'T' command
    void do_cmd_U();            // handle 'U' command
    void do_cmd_w();            // handle 'w' command
    void do_cmd_W();            // handle shift-W command
    void do_cmd_w1();           // handle ctrl-W command
    void do_cmd_x();            // handle 'x' command
    void do_cmd_rpt();          // handle '*' command
    void do_cmd_reflbltyp();    // handle '^' command
    void do_cmd_ref();          // handle '@' command
    void do_cmd_bak();          // handle '<' command
    void do_cmd_fwd();          // handle '>' command
    void do_cmd_cen();          // handle '`' command
    void do_cmd_top();          // handle '~' command
    void do_cmd_center();       // handle 'M' command
    void do_cmd_cln();          // handle '!' command
    void do_cmd_prv();          // handle ',' command
    void do_cmd_nxt();          // handle '.' command
    void do_cmd_les();          // handle '(' command
    void do_cmd_mor();          // handle ')' command
    void do_cmd_hint();         // handle '"' command
    void do_cmd_defhint();      // handle '$' command
};


extern void setup_screen();    
extern void end_screen();    
void finish(int UNUSED sig);


// global scope screen handling object
extern DisScrn scrn;


// now for some command line string processing functions
int GetWord(char *&s, char *word);     	// get a word from s, and make it upper-case
int GetString(char *&s, char *word);    // get a string optionally delimited by quotes
int ishex(char c);
int HexVal(const char *hexStr);         // get value of a hex string
int OctVal(const char *octStr);         // get value of an octal string
int HexOctVal(const char *hexStr);      // get value of a hex or octal string
uint32_t DecVal(const char *decStr);    // get value of a decimal string
bool HexValid(const char *hexStr);      // verify that a hex string is valid
bool OctValid(const char *octStr);      // verify that an octal string is valid
bool HexOctValid(const char *hexStr);   // verify that a hex or octal string is valid
const char *rem_blank(char *s);         // remove doubled blanks from s


#endif // _DISSCRN_H_
