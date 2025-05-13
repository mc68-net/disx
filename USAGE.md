disx - Full-Screeen Interactive Disassembler
=============================================

Invocation
----------

Usage is `disx [options] [binfile]`; if no _binfile_ is supplied, disx will
show an empty file with no selected CPU type.

#### Annotation Files

Information (annotations) for _binfile_ are found in several different files
which are all read at startup.

_binfile_`.ctl` is a binary file containing all information outside of the
other more specific files below. This is created automatically by the `:w`
(write) command and is not user-editable.

The following files are ASCII with one item per line, consisting of a
hexadecimal address, a space, and the annotation. (For label files, a space
after the label name followed by text is a comment for that file.) Lines
not starting with a hex number are ignored.

These files may be edited when disx is not running; disx will load and
re-sort these at startup, writing them when the `:w` command is executed.

- _binfile_`.sym`: Symbol names and addresses for symbols within the
  disassembly range.
- _binfile_`.equ`: Symbol names and addresses for symbols outside the
  disassembly range. This is never created by disx, but is read. These
  symbols are not displayed in generated assembly source and listing files.
- _binfile_`.cmt`: Comments.

Caveats:
- There are no checks for duplicate addresses.
- Zero-page addressing uses labels on only some CPUs.
- Labels can not be attached to `EQU $-1` lines.

#### Command-line Options

disx options are as follows. All _xxxx_ addresses are given in hex.
* `-h`:     Show command-line usage.
* `-c cpu`: Use CPU type _cpu_. 
* `-c ?`:   Show available CPU types.
* `-b xxxx` Base address, i.e. load address of first byte of binary file.
* `-s xxxx` Maximum size of code image to be loaded.
* `-o xxxx` Offset of data in binary file to disassemble.
            (This allows skipping file headers.)
* `-a`      Create _binfile_`.asm` and exit.
* `-l`      Create _binfile_`.lst` and exit.
* `-!`      Don't load _binfile_`.ctl`.


disx Commands
-------------

Commands are case-sensitive; for some of them using a shifted (upper case)
version of the command will do the command in the reverse direction. Typing
an invalid command will generate a bell indicating an error and command
input will be reset to the initial state. Capital letters prefixed by a
carat (`^`) indicate control characters.

Single character commands are executed immediately. Commands followed by
`…` (indicating one or more parameters typed after the command) are
_command line_ commands that open up a command line at the top of the
window for further entry, and are edited and executed using the following
keys:

    Backspace   Delete character before cursor, or exit input mode if line
      or DEL    is empty.
    Enter       Finish line and start executing it.
    Escape      Abort current line and exit input mode.
    ← / →       Move the cursor left or right.
    ^U or ^X    Erase from the cursor position to the beginning of the line.
    ^A          Move cursor to start of line.
    ^E          Move cursor to end of line.

Some immediate commands, marked as 'ⁿnn',  can take a count parameter where
_nn_ is the maximum count. Typing one or more digits `0`–`9` before the
command sets the count (an indication of the current count will be be
displayed). Pressing `Esc` will clear the count.  Some commands have a
maximum count.

Each line may have one _reference_ to code or data: the argument given to a
command that branches to a location or loads/saves data to a location, or
the first word on a data line. Labels will usually be created for
references and updated throughout the source; these may be changed  in type
(execution address or data) or removed with various commands. Specific
addresses can be prevented from ever having a label created for them using
the no-reference attribute (`^R` command to toggle for the current line's
address); 0x0000 is specifically prevented from automatically generating a
reference, because it is almost never used as such.

#### Movement Commands

disx has a current line that is displayed in inverse text; this is the
equivalent of a cursor in a text editor. The following commands change
the current line.

    ↑       Move up one line
    ↓       Move down one line
    [, ]    Go backward and forward to the next non-EQU label.
    @       Go to the address referenced by the current instruction.
    <, >    Go backward and forward along `@` or `:xxxx` usages.

    PgUp    Scroll down a page at a time, while staying on the same screen line
    PgDn,   Scroll up a page at a time, while staying on the same screen line
    Space
    Home    Move to first line of the file
    End     Move to last line of the file

    ^B     Move to top of screen, then move up a page at a time
    ^D     Move to bottom of screen, then move down a page at a time
    ~      Re-center the selected line towards the middle of the screen.
           This is useful when at the very top or bottom of the screen
           and you need to see a few more lines.

    :xxxx  Go to address `xxxx` (hexadecimal).

    !      Search the entire code for any reference to the label at the
           current line, and delete the label if not found. The address
           of the first reference is reported.
    /…     Search forward for …. Searches are case insensitive and
           collapse all spaces together. Searches also ignore the hex
           address and data fields.
    ?…     Search backward for …, as above.

#### Data Disassembly Commands

    x       Disasssemble as raw data (the default state)
    a       (ⁿ40) Disassemble as ASCII text
    b       (ⁿ32) Disassemble as single bytes
    h       (ⁿ32) Disassemble as hex bytes with no formatting

    I       (ⁿ16) Disassemble as binary, 0x55 = 01010101B
    X       (ⁿ16) Disassemble as visible binary, 0x55 = _X_X_X_X
    O       (ⁿ16) Disassemble as visible binary, 0x55 = O_O_O_O_
            See "xobits.h"/"xobits.s" for the "visible binary" equates.

    w       (ⁿ20) Disassemble as 2-byte words in CPU's endianness
    W       (ⁿ20) Disassemble as 2-byte words in reverse endianness
    ^W      (ⁿ1)  Disassemble as word address - 1 for 6502 jump tables
    \       (ⁿ16) Disassemble as 4-byte longwords in CPU's endianness
    |       (ⁿ16) Disassemble as 4-byte longwords in reverse endianness

    d       (ⁿ20) Disassemble as decimal words (unsigned)
    D       (ⁿ16) Disassemble as decimal longwords (signed)
    _       (ⁿ40) Disassemble as EBCDIC text

    *       Repeat previous data format command with same count
    ( and ) Expand or shrink the size of a line of data.

    o       Dissasemble current word as a position-independent offset in a
            table starting at the most recent label. This is commonly found
            on certain architectures such as 68k. If the target address has
            no label, a data label will be created. If the target is
            already disassembled as code, a code label will be created.

#### Code Disassembly and Tracing Commands

    c       Disassemble current address as code. Ignored if already
            disassembled as current CPU, or if an illegal instruction.

    C       Disassemble as code until unconditional branch, illegal
            instruction, rip-stop (see below), or already disassembled code.

    T       Trace disassemble from current instruction. Same rules as `C`
            except that all code references are followed and disassembled
            as well.

    ^T      Disassemble as a data word and trace from the referenced
            address. This is intended for jump tables. The reference
            address will become a code label when appropriate.

Tracing through code references can quickly disassemble large ranges of
code, but it can also be misled by "tricks" like subroutines that pop the
return address, read data from that address, and then put it back before
returning.  Note that the Z-80 series is particularly vulnerable to space
(0x20) characters causing bogus code references. The "rip-stop"
functionality (see below) attempts to detect the most obvious situations.

Also see "Rip-Stop" below.

#### Label-related Commands

Labels are automatically assigned to the targets of branch instructions
(`Lxxxx`) and data loads/stores (`Dxxxx`). These labels may be changed and
new labels assigned to lines without a label. Supplying no name to a label
command will revert the label to its automatically generated default, if
there was one, or otherwise remove the label from a line.

    L, ^L  Toggle label type at this address between none, data, and code.
            Note that if the current line is a `EQU $-n` line, this will
            only remove the label. You must use `X` to break up the
            previous instruction, change the label state, then fix it with
            `c` afterward.
    ^       Toggle label type at address referenced by the current instruction.
    ^R      Toggle the no-reference attribute for selected address, to
            prevent labels from being automatically created. This is useful
            for certain "magic number" addresses like 0x1000, and almost
            anything at address 0000-00FF.

    :l …    Set label for current line. Entering an empty label will remove
            any current custom label.
    :label  Alias for `:l`.

See the "Invocation" section above for information on the `.sym` and `.equ`
files in which labels are stored.

#### Comment and Miscellaneous Commands

    l      Toggle a pre-instruction blank line before this code/data line.
    ;       Attach comment to line

The comment attach command will bring up the current comment (if any) for
editing using the standard command line editing keys. Clearing the comment
line and entering it will delete the comment. See "Invocation" above for
the comment file format.

If a comment "falls off" because its address is now in the middle of a
line, it is not immediately deleted, but can not be seen until the address
starts a line again. However, when saving, such hidden labels will not be
saved.

#### Hint Commands

    "       Change the hint flags for the current address. There are two
            bits of hint flags that rotate through all four combinations.
    $       Change the default hint flags for newly disassembled instructions.

#### Undo-related Commands

There is minimal undo support, in the form of one saved state. It is
automatically saved by the multiple-instruction disassembly commands (`C`
and `T`), and manually by the `^U` command. But it's really not in a useful
state right now. Comments are not affected by the undo command.

    ^U      Save current state for the next undo.
    U       Undo to last saved state.

#### Disassembly Configuration Commands

    :defcpu …   Set "default" CPU type used for number syntax and endian
                order for DW directives, etc.
    :cpu …      Set CPU type for disassembly. If `defcpu` has not yet been
                set, it will also be set to this CPU type.

    :tabs …     With no parameters, displays current tab settings. With `!`
                as the first parameter, all column widths must be multiples
                of 8 and listing files will have hard tabs. The remaining
                five parameters (trailing parameters optional) are the
                field widths (defaults in parens) for the address (8), hex
                object code (16), label (8), opcode (8), and operand (16);
                comments start after these.

    :rst …      With no parameters, displays current settings. With eight
                single-digit parameters, sets the number of additional
                bytes to be disassembled for each of the eight 8080/Z80 RST
                instructions. (E.g., `21000000` will disassemble RST0 as
                three bytes, RST1 as two bytes, and all others as a single
                byte.) Changing this will not automatically re-scan the
                code; you must go to each RST instruction and
                re-disassemble it with the `c` command.

#### File Management Commands

    :q          Exit the program. If the disassembly status has been changed,
                you must use `:q!` to force quit or save first with `:w`.
    :w          Save the current disassembly state to the files mentioned in
                "Invocation" above.
    :save       Alias for `:w`.
    :asm        Write a source file suitable for assembly to `binfile.asm`.
    :list       Write an assembly listing-style file to `binfile.lst`
    :load …     Load the binary file to disassemble. Parameters are
                `<file> [!] [Bxxxx] [Sxxxx] [Oxxxx]`. Use `-` as the filename
                to reload the current file. Other parameters are as per
                "Invocation" above.


Rip-Stop
--------

An auto-tracing disassembler can be very powerful, but can also overrun
code areas into data, generating various problems including many bogus
labels that can be a pain to clean up. The rip-stop functionality stops
automatic disassembly when suspicious code is reached. (What is
"suspicious" varies based on the instruction set.) This applies only to the
tracing disassembly commands (`C`, `T` and `^T`), and not to the single
instruction disassembly command (`c`), which can be used to manually
disassemble an instruction that caused a rip-stop.

Currently rip-stop is implemented mainly for the Z80 and 6502
disassemblers. Here are some of the various trigger conditions:

Z-80:
- Three NOP instructions in a row (repeated 0x00).
- Two `RST 38H` instructions in a row (repeated 0xFF).
- `LD r,r` where both _r_ values are the same register.
- Repeated `LD r1,r2` with the same pair of registers.
- Branches with an offset of 0x00 or 0xFF.
- `DJNZ` with a forward offset (common on 8051, but rarely used on Z80).

6502:
- Two `BRK` instructions in a row (repeated 0x00).
- Branches with an offset of 0xFF.
- The rip-stop code also tries to detect "always branch" conditions such as
  `BEQ` followed by `BNE`.

68HC11:
- Two `NOP` or `SWI` instructions in a row (repeated 0x01 or 0x3F).
- `STX $FFFF` (repeated 0xFF).
- Branches with an offset of 0xFF.

8008:
- Repeated `MOV r,r` with the same pair of registers.
- Two `NOP` instructions in a row.
- Two `HLT` instructions in a row (repeated 0xFF).


Hint Flags
----------

Some disassemblers can use a few extra hints on what to do. There are two
flag bits for each address, giving four combinations. The meaning of these
is different for each disassembler.

#### 65816

Some instructions change length depending on the run-time value of two
processor status flags. These have been assigned to set the `LONGA`
(immediate loads for the accumulator are 16 bits) and `LONGI` (immediate
loads for the index registers are 16 bits) flags.

    0   none
    1   LONGI
    2   LONGA
    3   LONGI + LONGA

#### 8008

Loads of register pairs are by default detected and combined into `LXI`
instructions. In some cases (such as a branch into the second instruction)
you will want to override this.

    0,2 combine LXI
    1,3 don't combine LXI

#### 8048

The bank for long jumps is by default determined by tracing back in the
code for `SEL MB0/MB1` instructions, but sometimes this is wrong. You can
use hints to force which bank to use.

    0   automatic
    1   current bank
    2   SEL MB0 bank
    3   SEL MB1 bank


Miscellaneous Stuff
-------------------

* If the .bin file has been moved to a different directory, the path saved
  in the .ctl file will not match. There will still be an attempt to load
  the .ctl file from the same directory as the .bin file.

* The 8048 disassembler has to guess the `SEL MB0/MB1` flag for the `JMP`
  and `CALL` instruction bank address. This is done by searching backwards
  as long as there's contiguous 8048 code. If none is found, it defaults to
  the bank at the current address. It is possible for a called subroutine
  to change the bank, and further calls/jumps after the call will have to
  be manually overridden with hints.

* Not all of the disassembler cores have been thoroughly tested. Some (ARM
  and PIC) were never quite finished.

* The PIC disassembler creates label names using the byte address rather
  than the word address.

* The PIC disassembler requires the binary be in little-endian format (low
  instruction byte first).

* The 78K0 disassembler is mostly working, but the code that I had hoped
  used it was actually for a different CPU, so it hasn't really been
  tested.

* Because Thumb often uses the low bit of an address to indicate Thumb
  code, longword references to Thumb code will be displayed as the even
  address "+1". You can not use the "^" command to put a label at the even
  address, but will have to manually add a label at the even address.


Hints and Tips
--------------

* Sometimes you have a binary of an unknown CPU type. It is very helpful to
  identify hex opcodes before disassembling so that you know which CPU type
  to select, and also to know where the hardware-defined entry points are.
  Sometimes even determining the base address of code can be difficult.

* Common endianness, opcodes, and vectors for various CPUs:
  - LE 8080
    - CD xx xx / C3 xx xx / C9 / F5 E5 D5 ... D1 E1 F1 / 11 xx xx / 21 xx xx
    - reset at 0000, vector entry points at 0008 0010 0018 0020 0028 0030
      0038 0066
  - LE 6502
    - A9 xx / 4C xx xx / 20 xx xx / 60
    - vector address words at FFFx
  - BE Z8
    - D6 xx xx / 8D xx xx / AF
    - reset at 000C, vector address words at 0000-000B
  - LE 8048
    - 02 xx xx / 12 xx xx / 83
    - reset at 0000, interrupt entry points at 0003 0007
  - BE 8051
    - 02 xx xx / 12 xx xx / 20
    - reset at 0000, interrupt entry points at 0003 000B 0013 001B 0023
  - BE 6800
    - BD xx xx / 7E xx xx / 39
    - vector address words at FFFx
  - BE 6809
    - BD xx xx /
    - vector address words at FFFx
  - BE 1802
    - Dx / Cx xx xx / 70 / 3x xx / F8 xx By F8 xx Ay
  - BE 68000
    - 4E56 0000 / 4E5E 4E75
    - vector address longwords at 0000-00FF
  - LE 8086
    - return instructions C2 C3 CA CB / branch instructions 7xrr with rr F0-10

* CPUs without 16-bit load instructions are harder to work with because
  there are no register loads of full addresses. You can still put a label
  at the address, but the references will have to be manually replaced in
  the resulting `.asm` file. It is a good idea to put the reference in a
  comment.

* When you have subroutine calls followed by inline ASCII text or other
  non-code data that can't disassemble properly, one workaround is:
  - use the search function to find one of the bytes of the call
    instruction (`/ db xxH`).
  - Use the single instruction disassembly command (`c`) to convert the
    subroutine call to code.
  - Repeat for all such calls.
  - Code tracing will now automatically stop at the already disassembled
    subroutine call when tracing.
  - This is of course not as simple as it sounds.
