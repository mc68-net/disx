disx Tutorial
=============

[`USAGE.md`] is the primary documentation for commands; this file covers
part of that material in a way that may be more suited to initially
learning how the disassembler works.

Instructions for input below say "type" followed by the characters in
fixed-width format. Certain special symbols are used:

    ⏎       Enter or Return key.
    ←↑↓→    Arrows keys, though usually `hjkl` can be used.
    ^       A carat character.
    ˇX      Ctrl-X, i.e., hold down control while pressing the next character.


Command Summaries
-----------------

This is a summary of all comands learned in these tutorials, grouped
roughly by the type of function the command performs (movement, editing,
etc.)

    :wq⏎    write and quit

     z  M   move current line to middle line of window

    :#⏎     go to address _#_ (hex)
     j  k   move down/up one line
    ˇ]  @   go to refaddr (target of current instruction)
    ^T      return from 'go to refaddr
     [  ]   go to previous/next label

     /  ?   start search forward/backward (type search string)
    /x:⏎    search for symbol definition (does not work for auto-labels)
     n  N   search again in previous or opposite of previous direction

     c      disassemble current address as instruction
     C      disassemble remainder of routine as instructions
     t      disassemble tracing recursively through all calls/jumps/etc.

     ;      set/edit end-of-line comment for current line
     l      set label for current location
     L      set label for refaddr, the target of the current instruction
     '      toggle label type for current location (none → data → code)
     "      toggle label type refaddr (none → data → code)

### XXX TODO

- `U` for undo.


Sample Project: SORD M5 Boot ROM Disassembly
--------------------------------------------

A copy of the SORD M5 boot ROM can be found in the [`m5/rom/`] subdirectory
of <https://gitlab.com/retroabandon/sord-re>. For this tutorial, copy the
`int-jp.bin` file from that repo into an empty directory. You may wish to
put this directory under Git control, as you typically would on a real
reverse engineering project.

Note that you can do this tutorial with any ROM dump you like; though
obviously the screen displays won't match up and you'll have to change
things around a bit, you should be able to learn all the commands
effectively anyway.


Tutorial 1: Startup and Basic Instruction Disassembly
-----------------------------------------------------

The SORD is a Z80-based system, and you will have to inform `disx` of that
when you start it. Here we also provide the base address of $0000 to
demonstrate the option, but as that's the default it need not be specified
in this particular case. (Addresses are hex with no base markers, e.g. just
`C800`.)

    disx -c Z80 -b 0000 int-jp.bin

This leaves you at the start of the file disassembled only as byte data.

      ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
    » 0000:   F3                      DB      0F3H              «
      0001:   ED                      DB      0EDH
      0002:   5E                      DB      5EH
      0003:   31                      DB      31H
    ...

The top line is inverse and blank; this is where commands requiring
arguments or output will appear. The second line, $0000, will also be
inverted; this is the cursor indicating the current line on which you're
operating. In this file we cannot show the inverted cursor line, so we mark
it with guillemets at the left and right: `» … «`. In future screen
displays we will usually leave out the top command line.

Continuing with the tutorial:

* On line $0000, which is the entry point at reset for the Z80, and type
  `c` to disassemble the current instruction. This will change the line to
  a `DI` instruction and move the cursor forward to $0001.

* Type `c` again to disassemble the next instruction. This disassembles two
  bytes, so line $0002 is combined into line $0001, and your cursor is left
  on the new next line, $0003.

You now see:

      0000:   F3                      DI
      0001:   ED5E                    IM      2
    » 0003:   31                      DB      31H               «

`IM 2` being a slightly obscure instruction, let's add a comment to remind
us how this interrupt mode actually works.

* Type `k` to move up one line to the `IM 2` instruction.

* Type `;` to bring up the comment interface: the cursor will move to the
  top line after a `;` prompt. Continue with
  ` int vector: reg I*$100 + data bus⏎`. If you make an error, or want to
  rewrite a comment, you can use `ˇU` while typing it to clear the line.

We can now continue disassembly, but in a faster way:

* Type `j` to move down one line to the as-yet not disassembled `DB 31H`.

* Type `C` to continue to do the same disassembly until a stop point is
  reached. The `c` command will be repeatedly executed until the end of the
  routine, which may be a subroutine return, a jump point, or similar. In
  this case, it will be a `JR L0058` instruction.

This leaves you at:

      0000:   F3                      DI
      0001:   ED5E                    IM      2
      0003:   310073                  LD      SP,7300H
      0006:   1850                    JR      L0058

    » 0008:   21                      DB      21H               «
      0009:   94                      DB      94H

At $0006, the $18 $50 instruction is a relative jump $50 bytes forward,
taking us from location $0008 to $0058. Note that a label, `L0058`, has
been automatically created for that location because it's the target of a
jump. You could use `j` to move down to this location manually, but easier
is to:

* Type `kk` to move up two lines, back to $0008 line. Type `@` or `ˇ]`
  (Ctrl-]; see the guide at the start of the file), the "go to refaddr"
  command, to immediately jump to $0058.

The "refaddr" is the address referenced by the instruction or pseudo-op on
the currently highlighted line. `@` is mnemonic for "_at_ that location";
`ˇ]` is the same as the vi "jump to tag" command. Whichever you use, it
will take you to:

      0056:   18                      DB      18H
      0057:   D3                      DB      0D3H
    » 0058:   3E              L0058:  DB      3EH               «
      0059:   70                      DB      70H
      005A:   ED                      DB      0EDH

You could continue disassembling section by section with `C`, but an easier
way to do this is

* Type `t`, to do a _tracing_ disassembly which, instead of stopping at
  branch points, will follow recursively every branch point. This will
  leave you at $0085, the `JR L0033` that exits from this routine

You'll see that considerable amounts of code both above and below this
point have been disassembled (these are any routines that have been called
by this code), as well as further labels added (these are locations of data
accesses by all this disassembled code).

* Type `kkˇ]` (or `kk@`) to move up two lines to the exit point `JP L0033`
  and then move to the target of that JP.

This shows you a small routine whose purpose is pretty obvious:

      0032:   70                      DB      70H
    » 0033:   CD0D01          L0033:  CALL    L010D             «
      0036:   18FB                    JR      L0033

      0038:   C3                      DB      0C3H

* Type `l` to start the "label" command that will set (or in this case
  change) the label of this line. . `l` will put the cursor on the top line
  with an `l` at the start to indicate the command in progress: type
  `mainloop⏎` to give the label name followed by Enter to execute the
  command.

This changes the label for the current line and all references to it; you
will notice that the line after, that loops back to this one, has the
target ("referenced address") changed.

    » 0033:   CD0D01          MAINLOOP:CALL   L010D             «
      0036:   18FB                    JR      MAINLOOP

> [!NOTE]
> Labels may be entered in any case, but are always converted to upper
> case. There is currently no way around this.

* Search forward for this label by typing `/mainloop⏎`. This will take you
  to the next line; type `n` or `/⏎` to search for the same thing again,
  taking you back to $0083.

You can see that the label here has been changed as well.

    » 0083:   18AE                    JR      MAINLOOP          «

* Type `n` again to continue the search; there are no further matches in
  the file so it wraps around and takes you back to the first use, the
  definition.

* Note: you can type `?` to search backwards in the same way. `n` will
  search again in the same direction (`/` or `?`) as you previously used;
  `N` in the opposite direction.

* Type `Lmain⏎` to set the label of the _target_ of this instruction. As
  before, this will change throughout the file.

* Type `ˇ]` (or `@`) to follow the reference.

* Type `z` or `M` to move the current line to the centre of the window.
  (`z` is mnemonic for the `zz` command in Vim that does this; `M` is
  mnemonic for "middle".)

We're now ready to start getting into disassembly of more serious code.
But first let's save what we've done and take a break:

* Type `:wq⏎` to write out the metadata we've created (new labels, etc.)
  and exit the program.

#### Metadata Files

Looking in the current directory that has the `int-jp.bin` file we've been
disassembling we'll see three new files.

`int-jp.bin.ctl` is a binary file that contains all information excepting
the symbol addresses. This is essentialy a saved version of `disx`'s
internal data structures.

`int-jp.bin.sym` has the symbol addresses and is in a readable ASCII form:

    0033 MAINLOOP
    010D MAIN

`int-jp.bin.cmt` has the comments, and is also ASCII:

    0001  int vector: reg I*$100 + data bus


Tutorial 2: Assembler Output and Configuration
----------------------------------------------

`disx` can generate both a listing output that looks like what you've been
seeing on the screen and an assembler output suitable for feeding to most
assemblers. These can be done with the `:list⏎` and `:asm⏎` commands during
disassembly or from the command line with the `-l` and `-a` options:

    disx -a -l int-jp.bin

When you've examined these, restart disx in interactive mode. You no longer
need to specify the CPU etc. since that's been saved in the `.ctl` file:

    disx int-jp.bin

You will start right where you left off, at $010D. But for this tutorial we
want to look again at MAINLOOP, at $0033.

> [!NOTE]
> There is currently no command to jump to a label by name.

* Type `:33⏎` to jump to $0033 where `MAINLOOP` is. Remember to type `z` to
  centre this in the window if it's too close to the top or bottom.

You will then see this, which as noted above awkward because the label does
not fit into the 8-character field allocated for it, yet we have 16
characters for the hex data that we will never use.

    » 0033:   CD0D01          MAINLOOP:CALL   L010D             «
      0036:   18FB                    JR      MAINLOOP

* Type `:tabs` to see the current tab settings.

On the top line of the screen, in inverse text, it will print the current
tab settings:

    current tab stops are: ! 8 16 8 8 16

The exclamation point means that tabs will be used in the assembler output,
as we saw above, and the remainder of the numbers are the lengths of
the fields used for assembly and listing output, as well as listing display
on the screen.
- `8`: Address field. 4 digits (on an 8-bit CPU), colon, three spaces.
- `16`: Data field. up to 7 hex bytes followed by two or more spaces.
- `8`: Label field. Up to seven characters followed by a `:`.
- `8`: Mnemonic (instruction or psuedo-op).
- `16`: Operand.

Each field will push the field to the right over if it needs more space,
which is why we see the `CALL` mnemonic above pushed over by one character:
the 8-character label needs 9 characters including its trailing colon.

We'll reset the tabs to some better values and leave out the exclamation
mark to indicate we want our assembly output to use spaces instead of tabs.

* Type `:tabs 6 10 12 6 12⏎`.

This produces the more readable:

      0001: ED5E                  IM    2           ; int vector: reg I*$100 + data...
        … [many lines elided] …
    » 0033: CD0D01    MAINLOOP:   CALL  MAIN                    «
      0036: 18FB                  JR    MAINLOOP

* Type `:wq⏎` to save and quit.


Tutorial 3: RST Calls
---------------------

`disx` does not trace is RST calls: we need to disassemble those manually.
(This is basically a bug or set of bugs in the disassembler; these will be
fixed one day.)

* Type `:0⏎` to go to go to the start of the file, then `/rst⏎` to search
  for the first RST instruction. You will be brought to $03CA, an RST $10.

* Type `^]` or `@` to discover that that this doesn't jump to the target of
  RSTs. to the RST definition at $0010. So instead type `:10⏎` to get
  there.

* Type `t` to disassemble that routine. You'll notice it calls L14BD; that
  was disassembled earlier because other things call it.

Going through all the RSTs, you will find that only $08, $10 and $18 are
called. Disassemble all three and the remaining bytes between them (and the
$00 reset entrypoint) are probably data.


Tutorial 4: Label Types
-----------------------

* type `:0⏎` to go to the start of the file and look forward. We have a
  label `L0007` pointing to the second byte of the preceeding `JR`
  instruction:

        0006: 1850                  JR    L0058
        0007:           L0007:      EQU   $-1

Especially in very low areas of memory, this is typically a case of a
constant being given a label.

* Type `/l0007⏎` to search for the label, moving you down to the
  definition, and type `n` to search for the next instance, taking you to
  its use:

      » 0DA1: 110700                LD    DE,L0007              «
        0DA4: 01A040                LD    BC,40A0H
        0DA7: CD860B                CALL  L0B86

* Type `jj^]` to move down to it and follow it, so we can see what the call
  does.

It shows us a `LD (7227H),DE` instruction, so the DE argument above is
clearly just to be loaded into that particular RAM location and the
following one. Given the context, this is clearly just static data, nothing
to do with what's at memory location $0007 (the offset of the JR
instruction).

* Return to the caller with `^T`. Type `kk` to move up two lines back to
  the use of the `L0007` label.

* Type `"` once to toggle the label type for the refaddr. This switches it
  from 'code' to 'none,' removing the label and replacing it with a
  literal: `0007H`. Try it three more times to loop from 'none' to 'data'
  to 'code' back to 'none' again.

* Type `:7⏎` to return to the label definition point. It actually takes you
  to line 6; the label pointing into the second byte of that JR instruction
  is gone.

* Type `jj` to move down to the next label, `D0008:`.

`D0008` is labelling the RST08 vector, but `RST 08H` calls don't use labels
and the `D` at the front of this one means it's used only as a data
reference. This is clearly also just a small constant being used in one or
more locations. We can just directly remove this label:

* Type `'` to change this line's label type to `L0008`, and type `'` once
  more to change the label type to 'none', removing it.

The two labels below are similar, and can be dealt with in the same way.
But the third one has been disassembled to a data word: `D000E: DW 0E6C9H`.
This is potentially a reference (though it seems unlikely), so we need to
search forward for its uses. We find only one: `LD DE,D000E`, so we can
safely remove that label too using any of the techniques above.

The last label in the range of RSTs that is used is `D001C`. Searching for
that brings us to an `LD BC,D001C` just before an `LDIR`, clearly a count,
so we can remove that label as well.


Tutorial 5: Label Definitions Outside Disassembly Range
-------------------------------------------------------

At location $0008, HL is loaded with a constant: `LD HL,7094H`. If you
try to use the `L` command to set a label it will bring up the label
editor on the top line, but entering a label name will not change the
display, and if you exit and check the `.sym` file you'll notice that the
symbol was never added there.

To add symbols outside the disassembly range you need to create a `.equ`
file, which has the same format as the `.sym` file: 4-digit hex address,
space, label, then optional space and comment.

For this case, create the new `int-jp.bin.equ` file and add a single line
to it: `7094 ram_x0     loaded/read by RST 08`. When you start `disx` again
you'll see that this label is now used everywhere.


Tutorial X: Data Definitions
----------------------------

Let's start at the `MAIN` routine.

* Type `/main:` (don't forget the colon!) to search for it. This will find
  it immedately because only the definition will have the trailing colon.
  (Note that if you are already on the only matching line, further attempts
  to search will give a 'not found' error.)

You will see:

    » 010D:   119B04          MAIN:   LD      DE,D049B          «
      0110:   216272                  LD      HL,7262H
      0113:   CD8715                  CALL    L1587
      0116:   38F5                    JR      C,MAIN

Here we load DE with the value at `D049B` in memory; this automatically
generated label starts with a `D` because it's accessed only as data, never
executed (at least not directly in the way that the disassembler can see
it). Let's go to it and see if we can format the data there.

* type `xx`




`MAIN:` + 1,  $0110     The next register is loaded with a constant value,
`7262H`, because unlike $049B, the address $7262 is outside the range of
memory we're disassembling. These registers appear to be loaded as
parameters for the following `CALL`. so let's go look at that.

* Type `jj@` or `jjˇ]` to move down two lines to the CALL instruction and
  and jump to the start of the subroutine being called.i

* Type `M` or `z` to move the code up to the middle of the window.

This brings us to a relatively short routine that calls a couple of others:

    » 1587:   E5              L1587:  PUSH    HL                «
      1588:   D5                      PUSH    DE
      1589:   CD9815                  CALL    L1598
      158C:   D1                      POP     DE
      158D:   E1                      POP     HL
      158E:   D8                      RET     C
      158F:   23                      INC     HL
      1590:   CD6517                  CALL    L1765
      1593:   2B                      DEC     HL
      1594:   DC4200                  CALL    C,L0042
      1597:   C9                      RET





TODO: Commands to Introduce
---------------------------

- `[`,`]` to move to prev/next label.
- `:85` to move to location 85.
- Column settings (to give more space to label).
- Number format ($xx instead of 0xxh).
- Generating assembly output.
- Undo
- `<`/`>` prev/next addr from `@` and ???
- `;` comment



<!-------------------------------------------------------------------->
[`USAGE.md`]: ./USAGE.md
[`m5/rom/`]: https://gitlab.com/retroabandon/sord-re/-/tree/main/m5/rom?ref_type=heads
