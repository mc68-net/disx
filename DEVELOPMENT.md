disx Development Information
============================

(See also the [CHANGELOG].)


Disassembler Status
-------------------

This is a rough estimation of the quality of each disassembler. The quality
level is coded as follows:

- __5__: It has been used a lot, but some sub-types might be less than perfect.
- __4__: Disassembled code has been regularly re-assembled and compared.
- __3__: It generates code that has been re-assembled, but not a lot of it.
- __2__: Actual code has been disassembled and makes some sense, but no way
  to re-assemble.
- __1__: Early attempt, not even sure if all opcodes are correct.

#### Level 5

- `dis6502.cpp`:   6502 tested with many re-assemblies, 65816 works on a few samples
- `dis6809.cpp`:   Heavy usage and re-assmblies, no 6309 support.
- `dis68HC11.cpp`: Tested with many re-assemblies, but few 6303 examples.
- `dis68k.cpp`:    Heavy usage and re-assmblies.
- `disz80.cpp`:    Heavy usage and re-assemblies, but does not support 8080 mnemonics.

#### Level 4

- `dis8051.cpp`:   Tested with many re-assemblies.

#### Level 3

- `dis8008.cpp`:   Tested on a few re-assemblies.
- `dis8048.cpp`:   Tested with re-assemblies, but suffers from guessing bank selects.
- `disthumb.cpp`:  Needs work, and trouble dealing with odd code addresses (Thumb flag).
- `disz8.cpp`:     Tested on a few re-assemblies.

#### Level 2

- `dis1802.cpp`:   Sort of works, but few samples and it suffers from split references.
- `dis2650.cpp`:   Seems to work okay on a few samples.
- `dis4004.cpp`:   Tested on the Busicom calculator ROM.
- `dis68HC05.cpp`: Few examples to test it on.
- `dis68HC12.cpp`: Tested on an example and an "all opcodes" binary.
- `dis7810.cpp`:   Only tested on one code example.
- `dis78K3.cpp`:   Only tested on one code example.
- `dis8086.cpp`:   Very minimal, only useful for Small model code without segments.
- `disf8.cpp`:     Sort of works, but not enough examples.
- `dispdp11.cpp`:  Sort of works, but not enough examples, and it looks weird in hexadecimal.
- `dispic.cpp`:    Sort of works, but better testing needed.

#### Level 1

- `dis78K0.cpp`:   The code that I thought was 78K0 turned out to be 78K3, so no examples.
- `disarm.cpp`:    Still needs a lot of work, can suffer from split references.


Q and A
-------

- Q: Why is it version "4"?
  - A: Because I've had disassemblers for a very long time. Version "2" was
    when I added tracing and a configuration file. It was very detailed,
    and it could do a good job, but it was a real pain to use. Version 3
    was rewriting everything to use C++ inheritance. But it was still such
    a pain to use that for many years I had been wanting to do an
    interactive disassembler "someday". Once I finally started writing the
    code, it only took about two months for V4 to become functional. The V3
    disassembler cores were then ported to V4.

- Q: Why use ncurses and not a real windowing system?
  - A: Because it is more portable between Unix-like operating systems, and
    text UIs are underrated. I kept all the use of ncurses to a single .cpp
    file so that there should only be one layer to replace later. The key
    bindings were mostly inspired by vi and bash.

- Q: Why only one level of undo?
  - A: Because it's better than zero levels, and doing more would be a lot
    of work. Actually, I haven't really used it yet, so it's mostly there
    as a placeholder.

- Q: It crashed and now I can't see what I'm typing!
  - A: When an app using ncurses crashes, it can leave the terminal in an
    inconvienent state. The command `stty sane` should return things to
    normal. If it locks up, pressing control-C twice should force exit to
    the command line with ncurses properly cleaned up.

__REMEMBER, IF IT CRASHES YOU MAY NEED TO `stty sane` TO GET THE TERMINAL BACK.__

- Q: It crashed with an "assert"!
  - A: Try to reproduce the problem. Go back and do stuff up to right
    before the problem, then save. Try a step at a time until it crashes.
    What is needed to debug this is the binary file, the .ctl file from
    just before the crash, and what keys to press to cause the crash.



<!-------------------------------------------------------------------->
[CHANGELOG]: ./CHANGELOG.md
