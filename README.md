disx multi-CPU diassembler
==========================

(For those wanting to skip the introduction, you may want to go straight to
the [user manual][`USAGE.md`].)

[disx], by [Bruce Tomlin], is an interactive, curses-based ([TUI]) tracing
disassembler for Unix systems, including Linux and MacOS. (It may work on
Windows via Cygwin or other means.)

It supports almsot two dozen different (mostly 8-bit) CPUs and variants,
including Z80, 6800, 6502, 6809, and so on. It writes a small binary
control file that can be committed to version control, along with an ASCII
symbol definition file and optional "equate" (for symbols with values
outside of the disassembly range) file. It can generate both source code
and listing files.

The original version of this disassembler and its documentation are
available at [`xi6.com/projects/disx/`][disx] and [`disx4.txt`][disx4.txt]
on that site.

This branch contains modifications; see the README on the [`main`] branch
on this repo for information on the various branches and more on original
sources.


Introduction
------------

disx is a _tracing_ disassembler, meaning that when it starts it assumes
that all bytes are data and, when you ask it, it will simulate execution of
the opcodes from a given address (going down both the "not taken" and
"taken" paths of branches) to discover what parts of the binary are
executable code. It assigns default label names (`lxxxx` for code
referenced by branch instructions, `dxxxx` for data referenced by, e.g.,
loading a register with an address) as it goes; if the user changes any one
label name, it will be changed throughout the code.

The interface is much like any visual editor: you can use the arrow keys
and other commands to change the current line, and enter commands that act
on the current line (e.g., to change the label name for the current line,
or change the label name for the target of the current instruction).

disx is designed to make a decent disassembly that you can save as a `.asm`
file and further tweak in a regular editor; it's not designed to produce a
"perfect" disassembly that needs no further editing.

There are a few useful things that disx does not currently do, including:
- Tracking RAM references.
- Dealing with split references (high half in one instruction, low half in
  another).
- Combining separate binary files. (If your code comes from multiple ROMs,
  you will need to create a combined binary first.)
- Handling multiple code segments with different address ranges.

### Documentation

- [`README.md`]: This file.
- [`USAGE.md`]: The user manual.
- [`CHANGELOG.md`]: Notes on each release.
- [`DEVELOPMENT.md`]: Information for those looking at or modifying the code.

### Supported CPUs

disx is oriented mainly towards 8-bit CPUs, and these have the best
support. It does, however, have some support for a few 16- and 32-bit CPUs
as well.

- Intel/Zilog 8080 Z8080 8085 8085U Z8085 Z80 Z180 GB GBZ80
- MOS/WDC 6502 6502U 65C02 65SC02 65816 65C816
- Motorola 6800 6801 6802 6803 6808 6301 6303 68HC11
- Motorola 6805 68HC05
- Motorola 6809
- Motorola 68K 68000 68010
- Intel MCS-51 8051 8032
- Intel MCS-48 8021 8048 8049 8035 8041 8042
- Intel 8008
- Intel 4004
- Zilog Z8
- RCA 1802
- Fairchild F8
- PIC12 PIC14
- ARM (incomplete)
- NEC D78C10 D78K0 D78K3

For further information and the quality of support for each CPU, see
[`DEVELOPMENT.md`]


Building
--------

This uses a standard makefile, so use the "make" command to build it. It
should work in a standard Unix, Linux or MacOS environment with C++
developer tools installed. If this isn't sufficient information, then
google for "how to use make."

On some systems, if you have not previously used the compiler, "make" might
have trouble finding it. Consult the documentation of your OS distribution
for more information. (On Debian-based systems, including Ubuntu, `sudo apt
install build-essential xutils-dev` should do the trick.)

If you plan on modifying the code, you might want to use "make depend"
first. This generates a file named ".depend" with make dependencies for the
`.h` files. That way, when you change a common `.h` file, all files that
use it will be automatically rebuilt. The "make depend" command will
probably complain about missing standard includes; you should ignore these
warnings.

The developers have not tried to build this on Windows. It might work with
either Cygwin or the Linux subsystem. At least one person has tried with
Cygwin, but its system headers defined "addr_t" for some reason.



<!-------------------------------------------------------------------->
[`CHANGELOG.md`]: ./CHANGELOG.md
[`DEVELOPMENT.md`]: ./DEVELOPMENT.md
[`README.md`]: ./README.md
[`USAGE.md`]: ./USAGE.md
[`disx4.txt`]: ./disx4.txt

[Bruce Tomlin]: http://xi6.com/
[TUI]: https://en.wikipedia.org/wiki/Text-based_user_interface
[`main`]: https://github.com/mc68-net/disx/tree/main
[disx4.txt]: http://svn.xi6.com/svn/disx4/trunk/disx4.txt
[disx]: http://xi6.com/projects/disx
