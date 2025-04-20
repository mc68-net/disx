disx multi-CPU diassembler
==========================

- By [Bruce Tomlin] <bruce@xi6.com>.
- Documentation: [`disx4.txt`].

This is a multi-CPU disassembler for many major 8-bit and some 16-bit CPUs,
with a full-screen text user interface.

My first disassemblers were written in BASIC in the 80s, and were very
stupid. They would just read the next byte and try to disassemble it, until
the end of the input file. Any data would just become a complete mess.

Then I made a disassembler that would attempt to trace code references.
While I was at it, I made it work with multiple different CPUs. That worked
okay, but it had another problem. I had it use a control file to specify
what every non-code byte was, manually. And usually I needed to run the
disassembly again and again, to confirm that I got the last one right, and
then to see the next bytes that needed to be manually specified.

After many years of suffering through this, I wanted something that could
let me make those changes easily, with immediate response, and without
having to flip back and forth constantly between three windows. Eventually
I decided that a text-based interface would be easier to deal with than one
of the many windowing systems out there. So I made a simple screen kernel
using ncurses, while I thought about what kind of data structures would
most efficiently represent hints for disassembling code.

Once I started working on the real thing, it only took just over two months
to get it working. Then I ported my old CPU disassemblers to it, and went
looking for every bit of code that I could find to disassemble with it so
that I could kick its tires, over and over again. Bugs were found and
fixed, but most important, I found a good balance of keyboard commands to
make it efficient to use.

It supports a large set of CPUs, most of the common 8-bit ones, along with
a few obscure ones, in different levels of quality.

It compiles on Mac and Linux. Windows users can probably make it work with
Cygwin or WSL, or manually create a command-line project that links with
the ncurses library.

### Current CPUs supported

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


Further Information
-------------------

The project home page can be found at <http://xi6.com/projects/disx/>.
It includes ZIP files with several versions of the source code and links
to the SVN repository. (This README is based on the project home page.)

This Git repo was imported on 2025-04-20 from the original SVN repostory at
<http://svn.xi6.com/svn/disx4/trunk/>. It includes all tags and branches.



<!-------------------------------------------------------------------->
[Bruce Tomlin]: http://xi6.com/
[`disx4.txt`]: ./disx4.txt
