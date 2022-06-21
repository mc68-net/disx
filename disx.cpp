// disx.cpp

#include "disx.h"

#include "disscrn.h"
#include "dissave.h"
#include "discpu.h"

#ifndef VERSION
#define VERSION ""
#endif // VERSION
#ifndef DATE
#define DATE ""
#endif // DATE

// =====================================================
// command-line options

const char *progname;           // pointer to argv[0]
const char *cmdfname = NULL;    // pointer to filename in argv[]
addr_t base = 0;                // base address of code image
size_t size = 0;                // size of code image
size_t ofs  = 0;                // offset of code image in file
bool   do_asm = false;          // generate .asm listing
bool   do_lst = false;          // generate .lst listing
int    force = 0;               // flags to override base, size, and ofs


// =====================================================
void usage(void)
{
    printf("%s disassembler  version %s  %s\n", progname, VERSION, DATE);
    printf("\n");
    printf("Usage:\n");
    printf("    %s [options] [binfile]\n", progname);
    printf("\n");
    printf("Options:\n");
//  printf("    --             end of options\n");
    printf("    -c cpu         select default CPU type\n");
    printf("    -c ?           show list of supported CPU types\n");
    printf("    -b xxxx        hexidecimal base address\n");
    printf("    -s xxxx        hexidecimal size of binary data\n");
    printf("    -o xxxx        hexidecimal offset to start of data in file\n");
    printf("    -a             create binfile.asm and exit\n");
    printf("    -l             create binfile.lst and exit\n");
    printf("    -!             don't load binfile.ctl\n");
    exit(1);
}


// =====================================================
void getopts(int argc, char * const argv[])
{
    int ch;

    while ((ch = getopt(argc, argv, "c:b:s:o:!al?")) != -1) {
        switch (ch) {
            case 'c': // select CPU type
                if (optarg[0] == '?') {
                    CPU::show_list();
                    exit(1);
                    break;
                } else {
                    CPU *cpu = CPU::get_cpu(optarg);
                    if (cpu) {
                        cpu->set_cur_cpu();
                    } else {
                        printf("Invalid CPU type specified\n");
                        CPU::show_list();
                        exit(1);
                        break;
                    }
                }
                break;

            case 'b': // base address
                if (!HexValid(optarg)) {
                    usage();
                }
                base = HexVal(optarg);
                force |= DisSave::FORCE_BASE;
                break;

            case 's': // size of image
                if (!HexValid(optarg)) {
                    usage();
                }
                size = HexVal(optarg);
                force |= DisSave::FORCE_SIZE;
                break;

            case 'o': // file offset
                if (!HexValid(optarg)) {
                    usage();
                }
                ofs = HexVal(optarg);
                force |= DisSave::FORCE_OFS;
                break;

            case 'a': // create binfile.asm and exit
                do_asm = true;
                break;

            case 'l': // create binfile.lst and exit
                do_lst = true;
                break;

            case '!': // force binary parameters
                force |= DisSave::FORCE_FNAME;
                break;

            case '?':
            default:
                usage();
        }
    }
    argc -= optind;
    argv += optind;

    // now argc is the number of remaining arguments
    // and argv[0] is the first remaining argument

    // only allowed parameter at this point is file name
    if (argc > 1) {
        usage();
    }

    // if a file name was provided, try to load it
    if (argv[0]) {
        cmdfname = argv[0];
    }

    // if file name is "?", print help
    if (cmdfname[0] == '?' && cmdfname[1] == 0) {
        usage();
    }

    // file name must be provided to create .asm or .lst
    if (!cmdfname && (do_asm || do_lst)) {
        usage();
    }
}


// =====================================================
#include "disstore.h"
int main(int argc, char * const argv[])
{
    // start with no data
    // rom.unload();

    // process command line
    progname = argv[0];
    getopts(argc, argv);

    // ===== initialize your non-curses data structures here

    // initialize curses
    setup_screen();

    // ===== set up "document" here

    // set up ncurses screen
    scrn.init_scrn();

    if (cmdfname) {
        // load the binary, but use the .ctl file if present
        // ofs, base, and size are ignored if the .ctl file is used
        DisSave save;
        if (save.load_file(cmdfname, ofs, base, size, force) < 0) {
            end_screen();
            printf("%s: unable to load binary file '%s'\n", progname, cmdfname);
            exit(1);
        }

        scrn.init_scrn(false); // do not reset saved _top and _sel
    }

    if (do_asm) {
        disline.write_asm("asm");
    }
    if (do_lst) {
        disline.write_listing("lst");
    }

    if (!do_asm && !do_lst) {
        // main loop
        while (!scrn._quit) {
            // get a key from the keyboard and pass it along
            scrn.do_key(getch());
        }
    }

    // clean up and exit
    end_screen();

    return 0;
}
