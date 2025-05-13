disx CHANGELOG
==============

#### 4.4.0 - 2025-??-??

New features:
- add support for NS405 CPU (8048 variant)
- add Signetics 2650 disassembler
- add Motorola 68HC12 disassembler

Bugs fixed:
- reset hint flags when changing instr type
- Z80: fix Intel 8080 opcode table
- fixed pointer use-after-free bug in comments/labels code

#### 4.3.0 - 2024-07-18

New features:
- added 6301 to 68HC11 disassembler
- added ARM Thumb diassembler
- added 4004 disassembler
- added equates file support

Bugs fixed:
- ORG was only being generated with a 16-bit address
- 68HC11: ASLD was incorrectly being recognized for 6800/6802/6808
- 68HC11: ABY instruction was not recognized
- 8048: hints now affect bank choice for long JMP/CALL

#### 4.2.0 - 2023-10-31

New features:
- added 68HC05 disassembler
- added ctrl-T command to trace at refaddr of current line
- added a minimal 8086 disassembler
- added a minimal PDP-11 disassembler
- added 8008 disassembler
- added Intel mnemonics for 8080 and 8085 disassembly
- added custom labels

Bugs fixed:
- 68HC11: fixed AIM/OIM/EIM/TIM instructions for 6303
- 6502: fixed some branch-always combinations
- F8: updated instruction definitions
- fixed a caching bug when deleting a label or comment

#### 4.1.0 - 2022-06-23

New features:
- Added support for user comments.
- 8051, 8048: added rip-stop for repeated 00H and FFH
- Command line input can now use left and right arrow keys.
- Tab stops are now saved to the .ctl file.

Bugs fixed:
- Fixed a possible crash when started when started with no filename.
- A7H in ASCII mode no longer produces " '''+80H "
- ORG line no longer shows a label
- 68HC11: fixed opcodes that were missed in a table update (18A6 etc.)
- The ')' command now clears the pre-LF attribute of added bytes.
- Fixed an infinite loop when search started from "END" line.
- Width of address/hex fields for search is now taken from tabs[] array.
- It is now possible to change tab stops.
- [ and ] commands will now skip past a pre-LF line.

#### 4.0.0 - 2022-05-27

Original version
