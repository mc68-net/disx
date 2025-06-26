; lots of constants for disassembled classic Mac OS code

 if 0 ; copy this to the top of the .asm file

	opt nolist
	include	"mac.h"
	opt list

 endif

; 68000 CCR flags
FLAG_X		equ 0x10
FLAG_N		equ 0x08
FLAG_Z		equ 0x04
FLAG_V		equ 0x02
FLAG_C		equ 0x01

; system globals
SysCom		EQU $0100 ; -     start of System communication area
MonkeyLives	EQU $0100 ; word  monkey lives if nonzero
ScrVRes		EQU $0102 ; word  screen vertical dots/inch
ScrHRes		EQU $0104 ; word  screen horizontal dots/inch
ScreenRow	EQU $0106 ; word  rowBytes of screen
MemTop		EQU $0108 ; long  ptr to end of RAM
BufPtr		EQU $010C ; long  ptr to end of jump table
StkLowPt	EQU $0110 ; long  lowest stack pointer value as measured in VBL task
HeapEnd		EQU $0114 ; long  ptr to end of application heap
TheZone		EQU $0118 ; long  ptr to current heap zone
UTableBase	EQU $011C ; long  ptr to unit I/O table
MacJmp		EQU $0120 ; long  ptr to jump vector table used by MacsBug
DskRtnAdr	EQU $0124 ; long  temporary pointer used by Disk Driver
*TwiggyVars	EQU $0128 ; long  ptr to 'other' driver variables (Lisa 5.25" drive)
PollRtnAddr	EQU $0128 ; long  ptr to 'other' driver variables (Lisa 5.25" drive)
DskVerify	EQU $012C ; byte  used by Mac 3.5" Disk Driver for read/verify
LoadTrap	EQU $012D ; byte  trap before launch?
MmInOK		EQU $012E ; byte  Initial Memory Manager checks ok?
*DskWr11	EQU $012F ; byte  try 1-1 disk writes?
CPUFlag		EQU $012F ; byte  code for installed CPU: 0=68000, 1=68010, 2=68020, 3=68030
ApplLimit	EQU $0130 ; long  address of application heap limit
SonyVars	EQU $0134 ; long  ptr to Mac 3.5" Disk Driver variables
PWMValue	EQU $0138 ; word  current PWM value
PollStack	EQU $013A ; long  address of SCC poll data start stack location
PollProc	EQU $013E ; long  ptr to SCC poll data procedure
DskErr		EQU $0142 ; word  disk routine result code
SysEvtMask	EQU $0144 ; word  system event mask
SysEvtBuf	EQU $0146 ; long  ptr to system event queue element buffer
EventQueue	EQU $014A ; 10    event queue header
EvtBufCnt	EQU $0154 ; word  maximum #of events in SysEvtBuf minus 1
RndSeed		EQU $0156 ; long  random number seed
SysVersion	EQU $015A ; word  System file version number (e.g. System 4.1=$0410)
SEvtEnb		EQU $015C ; byte  0 = SysEvent always returns FALSE
DSWndUpdate	EQU $015D ; byte  GetNextEvent not to paint behind System error dialog?
FontFlag	EQU $015E ; byte  font manager loop flag
Filler3		EQU $015F ; byte  1 byte of filler
VBLQueue	EQU $0160 ; 10    VBL queue header
Ticks		EQU $016A ; long  Tick count: time since system startup (tick=1/60 sec)
MBTicks		EQU $016E ; long  tick count when mouse button was last pressed
MBState		EQU $0172 ; byte  current mouse button state
Tocks		EQU $0173 ; byte  Lisa sub-tick count
KeyMap		EQU $0174 ; 8     bitmap of the keyboard
KeypadMap	EQU $017C ; long  bitmap for numeric keypad (uses 18 bits)
;[????]		EQU $0180 ; long  
KeyLast		EQU $0184 ; word  ASCII code for last valid keycode
KeyTime		EQU $0186 ; long  tickcount when KEYLAST was received
KeyRepTime	EQU $018A ; long  tick count when key was last repeated
KeyThresh	EQU $018E ; word  threshold for key repeat
KeyRepThresh	EQU $0190 ; word  key repeat speed
Lv11DT		EQU $0192 ; 32    Level-1 secondary interrupt vector table
Lv12DT		EQU $01B2 ; 32    Level-2 secondary interrupt vector table
UnitNtryCnt	EQU $01D2 ; word  count of entries in unit table
VIA		EQU $01D4 ; long  base address of 6522 VIA chip 
SCCRd		EQU $01D8 ; long  addr of Z8530 SCC chip (used when reading the chip)
SCCWr		EQU $01DC ; long  address of Z8530 SCC chip (used when writing the chip)
IWM		EQU $01E0 ; long  base address of IWM chip (floppy drive controller)
scratch20	EQU $01E4 ; 20    general scratch area
SysParam	EQU $01F8 ; -     System parameter RAM vars (PRAM info)
SPValid		EQU $01F8 ; byte  validation: $A8 if last write to clock chip was good
SPATalkA	EQU $01F9 ; byte  AppleTalk node ID for modem port
SPATalkB	EQU $01FA ; byte  AppleTalk node ID for printer port
SPConfig	EQU $01FB ; byte  serial-port-in-use flags for both ports
SPPortA		EQU $01FC ; word  modem port configuration (baud, parity, bits)
SPPortB		EQU $01FE ; word  printer port configuration (baud, parity, bits)
SPAlarm		EQU $0200 ; long  alarm clock setting
SPFont		EQU $0204 ; word  font number of application font minus 1
*SPKbdPrint	EQU $0206 ; word  auto-key threshold/rate and printer connection
SPKbd		EQU $0206 ; byte  auto-key threshold and rate
SPPrint		EQU $0207 ; byte  printer connection
*SPVolClik	EQU $0208 ; word  speaker volume; double click and caret flash times
SPVolCtl	EQU $0208 ; byte  speaker volume
SPClikCaret	EQU $0209 ; byte  double click and caret flash times
SPMisc		EQU $020A ; byte  reserved for future use
SPMisc2		EQU $020B ; byte  mouse tracking, startup floppy drive, menu blink
Time		EQU $020C ; long  current date/time (seconds since midnight 1 JAN 1904)
*BootDrive	EQU $0210 ; word  drive number of boot drive
BootDrive	EQU $0210 ; word  working directory reference number of boot disk
JShell		EQU $0212 ; word  journaling shell state
*Filler3A	EQU $0214 ; word  negative of vRefNum last seen by Standard File Package
SFSaveDisk	EQU $0214 ; word  negative of vRefNum last seen by Standard File Package
KbdVars		EQU $0216 ; -     Keyboard manager variables
;[????]		EQU $0216 ; word  
KbdLast		EQU $0218 ; byte  ADB address of keyboard last used
;[????]		EQU $0219 ; byte  
JKybdTask	EQU $021A ; long  ptr to keyboard VBL task hook
KbdType		EQU $021E ; byte  keyboard model number
AlarmState	EQU $021F ; byte  alarm clock: Bit7=parity, Bit6=beeped, Bit0=enable
*CurIOTrap	EQU $0220 ; word  current I/O trap being executed
MemErr		EQU $0220 ; word  Memory Manager error code
DiskVars	EQU $0222 ; -     Disk driver variables (60 bytes)
;[????]		EQU $0222 ; 60     [Editor note: ignore this line]
FlEvtMask	EQU $025E ; word  mask of flushable events (FlushEvents)
SdVolume	EQU $0260 ; byte  Current speaker volume (bits 0 through 2 only)
SdEnable	EQU $0261 ; byte  Sound enabled?
SoundVars	EQU $0262 ; -     Sound driver variables (32 bytes)
SoundPtr	EQU $0262 ; long  pointer to 4-voice sound definition (SynthRec)
SoundBase	EQU $0266 ; long  ptr to free-form sound definition (SynthRec)
SoundVBL	EQU $026A ; 16    vertical retrace control element
SoundDCE	EQU $027A ; long  pointer to Sound Driver's device control entry
SoundActive	EQU $027E ; byte  sound is active?
SoundLevel	EQU $027F ; byte  current amplitude in 740-byte sound buffer
CurPitch	EQU $0280 ; word  current value of COUNT in square-wave SynthRec
SoundLast	EQU $0282 ; long  address past last sound variable
;[????]		EQU $0286 ; long  
;[????]		EQU $028A ; long  
ROM85		EQU $028E ; word  holds a positive value if 128K or later ROM in Mac
PortAUse	EQU $0290 ; byte  Port A usage: if zero, port available
PortBUse	EQU $0291 ; byte  Port B usage: if zero, port available
ScreenVars	EQU $0292 ; -     Screen driver variables (8 bytes)
;[????]		EQU $0292 ; long  
;[????]		EQU $0296 ; long  
JGNEFilter	EQU $029A ; long  ptr to GetNextEvent filter procedure
Key1Trans	EQU $029E ; long  ptr to keyboard translator procedure
Key2Trans	EQU $02A2 ; long  ptr to numeric keypad translator procedure
SysZone		EQU $02A6 ; long  starting address of system heap zone
ApplZone	EQU $02AA ; long  starting address of application heap zone
ROMBase		EQU $02AE ; long  base address of ROM (Trap Dispatcher)
RAMBase		EQU $02B2 ; long  base address of RAM (Trap Dispatcher)
BasicGlob	EQU $02B6 ; long  ptr to BASIC globals
DSAlertTab	EQU $02BA ; long  ptr to system error alert table in use
ExtStsDT	EQU $02BE ; 16    External/status interrupt vector table
SCCASts		EQU $02CE ; byte  SCC read register 0 last external/status interrupt - A
SCCBSts		EQU $02CF ; byte  SCC read register 0 last external/status interrupt - B
SerialVars	EQU $02D0 ; -     async driver variables (16 bytes) 
;[????]		EQU $02D0 ; long  
;[????]		EQU $02D4 ; long  
ABusVars	EQU $02D8 ; long  ptr to AppleTalk variables
;[????]		EQU $02DC ; long  
FinderName	EQU $02E0 ; 16    name of the shell, usually "Finder" (STRING[15])
DoubleTime	EQU $02F0 ; long  double click interval in ticks
CaretTime	EQU $02F4 ; long  caret blink interval in ticks
ScrDmpEnb	EQU $02F8 ; byte  screen dump enable - zero disables FKEY processing
ScrDmpType	EQU $02F9 ; byte  $FF dumps screen, $FE dumps front window (FKEY 4)
TagData		EQU $02FA ; -     sector tag info for disk drivers (14 bytes)
;[????]		EQU $02FA ; word  
BufTgFNum	EQU $02FC ; long  File tags buffer: file number
BufTgFFlg	EQU $0300 ; word  File tags buffer: flags (bit1=1 if resource fork)
BufTgFBkNum	EQU $0302 ; word  File tags buffer: logical block number
BufTgDate	EQU $0304 ; long  File tags buffer: last modification date/time
DrvQHdr		EQU $0308 ; 10    queue header of drives in system
PWMBuf2		EQU $0312 ; long  ptr to PWM buffer 1 (or 2 if sound)
HpChk		EQU $0316 ; long  heap check RAM code
*MaskBC		EQU $031A ; long  Memory Manager byte count mask
*MaskHandle	EQU $031A ; long  Memory Manager handle mask
*¥MaskPtr	EQU $031A ; long  Memory Manager pointer mask
Lo3Bytes	EQU $031A ; long  holds the constant $00FFFFFF
MinStack	EQU $031E ; long  minimum stack size used in InitApplZone
DefltStack	EQU $0322 ; long  default size of stack
MMDefFlags	EQU $0326 ; word  default zone flags
GZRootHnd	EQU $0328 ; long  root handle for GrowZone
GZRootPtr	EQU $032C ; long  root pointer for GrowZone
GZMoveHnd	EQU $0330 ; long  moving handle for GrowZone
DSDrawProc	EQU $0334 ; long  ptr to alternate system error draw procedure
EjectNotify	EQU $0338 ; long  ptr to eject notify procedure
IAZNotify	EQU $033C ; long  ptr to world swaps notify procedure
FileVars	EQU $0340 ; -     file system vars (184 bytes)
CurDB		EQU $0340 ; word  current dir block/used for searches
CkdDB		EQU $0340 ; word  current dir block/used for searches
*FSCallAsync	EQU $0342 ; word  "One byte free"
NxtDB		EQU $0342 ; word  
MaxDB		EQU $0344 ; word  
FlushOnly	EQU $0346 ; byte  flag used by UnMountVol and FlushVol
RegRsrc		EQU $0347 ; byte  flag used by OpenRF and FileOpen
FLckUnlck	EQU $0348 ; byte  flag used by SetFilLock and RstFilLock
FrcSync		EQU $0349 ; byte  when set, all file system calls are synchronized
NewMount	EQU $034A ; byte  used by MountVol to flag new mounts
NoEject		EQU $034B ; byte  used by Eject and Offline
DrMstrBlk	EQU $034C ; word  master directory block in a volume
HFSGlobals	EQU $034E ; -     HFS global variables (to 03F6)
FCBSPtr		EQU $034E ; long  ptr to file control block buffer
DefVCBPtr	EQU $0352 ; long  ptr to default volume control block
VCBQHdr		EQU $0356 ; 10    volume control block queue header
FSQHdr		EQU $0360 ; 10    file I/O queue header
HFSVars		EQU $036A ; -     Start of TFS variables (RAM version)
HFSStkTop	EQU $036A ; long  Temp location of stack ptr during async calls
HFSStkPtr	EQU $036E ; long  Temporary location of HFS stack ptr
WDCBsPtr	EQU $0372 ; long  Working Directory queue header
HFSFlags	EQU $0376 ; byte  Internal HFS flags
*SysCRefCnt	EQU $0377 ; byte  system cache usage count (#of vols)
CacheFlag	EQU $0377 ; byte  system cache usage count now used as cache flag
SysBMCPtr	EQU $0378 ; long  System-wide bitmap cache pointer
SysVolCPtr	EQU $037C ; long  System-wide volume cache pointer
SysCtlCPtr	EQU $0380 ; long  System-wide control cache pointer
DefVRefNum	EQU $0384 ; word  Default volume's VRefNum/WDRefNum
PMSPPtr		EQU $0386 ; long  ptr to list of directories on PMSP
HFSDSErr	EQU $0392 ; word  Final gasp - error that caused IOErr
HFSVarEnd	EQU $0394 ; -     End of HFS variable area
CacheVars	EQU $0394 ; 8     
CurDirStore	EQU $0398 ; word  ID of last directory opened
;[????]		EQU $039A ; word  
CacheCom	EQU $039C ; long  
;[????]		EQU $03A0 ; word  
ErCode		EQU $03A2 ; word  report errors here during async routines
Params		EQU $03A4 ; -     File Mgr I/O ParamBlock (50 bytes)
;[????]		EQU $03A4 ; 50     
FSTemp8		EQU $03D6 ; 8     used by Rename
*FSTemp4	EQU $03DE ; word  used by Rename and CkFilMod
FSIOErr		EQU $03DE ; word  last I/O error
;[????]		EQU $03E0 ; word  
FSQueueHook	EQU $03E2 ; long  ptr to hook to capture all FS calls
ExtFSHook	EQU $03E6 ; long  ptr to command done hook
DskSwtchHook	EQU $03EA ; long  ptr to hook for disk-switch dialog
ReqstVol	EQU $03EE ; long  ptr to offline or external file system volume VCB
ToExtFS		EQU $03F2 ; long  ptr to external file system
FSVarEnd	EQU $03F6 ; -     end of file system variables
FSFCBLen	EQU $03F6 ; word  size of file control block; holds -1 on 64K ROM Macs
DSAlertRect	EQU $03F8 ; 8     rectangle for system error and disk-switch alerts
*DispatchTab	EQU $0400 ; 1024  OS & Toolbox trap dispatch table (64K ROM) (512 words)
OSDispTable	EQU $0400 ; 1024  OS trap dispatch table(128K and later ROM) (256 longs)
;GRAFBEGIN	EQU $0800 ; -     graf (QuickDraw) global area
JHideCursor	EQU $0800 ; long  
JShowCursor	EQU $0804 ; long  
JShieldCursor	EQU $0808 ; long  
JScrnAddr	EQU $080C ; long  
JScrnSize	EQU $0810 ; long  
JInitCrsr	EQU $0814 ; long  
JSetCrsr	EQU $0818 ; long  
JCrsrObscure	EQU $081C ; long  
JUpdateProc	EQU $0820 ; long  
LGrafJump	EQU $0824 ; long  
GrafVar		EQU $0824 ; -     QuickDraw variables
ScrnBase	EQU $0824 ; long  base address of main screen
MTemp		EQU $0828 ; long  low-level interrupt mouse location
RawMouse	EQU $082C ; long  un-jerked mouse coordinates
NMouse		EQU $0830 ; long  processed mouse coordinate
CrsrPin		EQU $0834 ; 8     cursor pinning rectangle
CrsrRect	EQU $083C ; 8     cursor hit rectangle
TheCrsr		EQU $0844 ; 68    cursor data, mask & hotspot
CrsrAddr	EQU $0888 ; long  address of data under cursor
*CrsrSave	EQU $088C ; 64    data under the cursor [Editor's note: 64K ROM only]
CrsrSave	EQU $088C ; long  ptr to data under the cursor
;[????]		EQU $0890 ; 20    
MainDevice	EQU $08A4 ; long  handle to current main device
DeviceList	EQU $08A8 ; long  handle to first element in device list
;[????]		EQU $08AC ; long  
QDColors	EQU $08B0 ; 28    default QuickDraw colors
CrsrVis		EQU $08CC ; byte  cursor visible?
CrsrBusy	EQU $08CD ; byte  cursor locked out?
CrsrNew		EQU $08CE ; byte  cursor changed?
CrsrCouple	EQU $08CF ; byte  cursor coupled to mouse?
CrsrState	EQU $08D0 ; word  cursor nesting level
CrsrObscure	EQU $08D2 ; byte  Cursor obscure semaphore
CrsrScale	EQU $08D3 ; byte  cursor scaled?
;[????]		EQU $08D4 ; word  
MouseMask	EQU $08D6 ; long  V-H mask for ANDing with mouse
MouseOffset	EQU $08DA ; long  V-H offset for adding after ANDing
JournalFlag	EQU $08DE ; word  journaling state
JSwapFont	EQU $08E0 ; long  jump entry for FMSwapFont
*JFontInfo	EQU $08E4 ; long  jump entry for FMFontMetrics
WidthListHand	EQU $08E4 ; long  handle to a list of handles of recently-used width tables
JournalRef	EQU $08E8 ; word  Journalling driver's refnum
;[????]		EQU $08EA ; word  
CrsrThresh	EQU $08EC ; word  delta threshold for mouse scaling
JCrsrTask	EQU $08EE ; long  address of CrsrVBLTask
GRAFEND		EQU $08F2 ; -     End of graphics globals
WWExist		EQU $08F2 ; byte  window manager initialized?
DExist		EQU $08F3 ; byte  QuickDraw is initialized
JFetch		EQU $08F4 ; long  ptr to fetch-a-byte routine for drivers
JStash		EQU $08F8 ; long  ptr to stash-a-byte routine for drivers
JIODone		EQU $08FC ; long  ptr to IODone routine for drivers
LoadVars	EQU $0900 ; -     Segment Loader variables (68 bytes)
CurApRefNum	EQU $0900 ; word  refNum of current application's resFile
LaunchFlag	EQU $0902 ; byte  Tells whether Launch or Chain was last called
;[????]		EQU $0903 ; byte  
CurrentA5	EQU $0904 ; long  current value of register A5
CurStackBase	EQU $0908 ; long  ptr to the base (beginning) of the stack
;[????]		EQU $090C ; long  
CurApName	EQU $0910 ; 32    name of current application (STRING[31])
SaveSegHandle	EQU $0930 ; long  handle to segment 0 (CODE 0)
CurJTOffset	EQU $0934 ; word  current jump table offset from register A5
CurPageOption	EQU $0936 ; word  current page 2 configuration (screen/sound buffers)
HiliteMode	EQU $0938 ; word  set to -1 if hilighting mode is on, 0 otherwise
LoaderPBlock	EQU $093A ; 10    param block for ExitToShell
PrintVars	EQU $0944 ; 16    print code variables
*LastLGlobal	EQU $0944 ; long  address past last loader global
PrintErr	EQU $0944 ; word  Print Manager error code
;[????]		EQU $0946 ; 14    
*CoreEditVars	EQU $0954 ; 12    core edit variables
LastPGlobal	EQU $0954 ; long  address of last printer global
;[????]		EQU $0958 ; long  
;[????]		EQU $095C ; long  
scrapVars	EQU $0960 ; -     Scrap Manager variables (32 bytes)
*scrapInfo	EQU $0960 ; long  scrap length
scrapSize	EQU $0960 ; long  scrap length
scrapHandle	EQU $0964 ; long  handle to RAM scrap
scrapCount	EQU $0968 ; word  count changed by ZeroScrap
scrapState	EQU $096A ; word  scrap state: tells if scrap exists in RAM or on disk
scrapName	EQU $096C ; long  pointer to scrap file name (normally "Clipboard File")
scrapTag	EQU $0970 ; 16    scrap file name (STRING[15])
scrapEnd	EQU $0980 ; -     End of scrap vars
ToolGBase	EQU $0980 ; -     base address of toolbox globals
ToolVars	EQU $0980 ; -     toolbox variables
RomFont0	EQU $0980 ; long  handle to system font
*ApFontID	EQU $0984 ; word  resource ID of application font
ApFontID	EQU $0984 ; word  font number of application font
GotStrike	EQU $0986 ; byte  Do we have the strike?
FMDefaultSize	EQU $0987 ; byte  default size
*CurFMInput	EQU $0988 ; long  ptr to QuickDraw FMInput record
CurFMFamily	EQU $0988 ; word  current font family
CurFMSize	EQU $098A ; word  current font size
CurFMFace	EQU $098C ; byte  current font face
CurFMNeedBits	EQU $098D ; byte  boolean telling whether it needs strike
CurFMDevice	EQU $098E ; word  current font device
CurFMNumer	EQU $0990 ; long  current numerator of scale factor
CurFMDenom	EQU $0994 ; long  current denominator of scale factor
FMgrOutRec	EQU $0998 ; long  ptr to QuickDraw FontOutput record
FOutError	EQU $0998 ; word  Font Manager error code
TFOutFontHandle	EQU $099A ;long handle to font bits
FOutBold	EQU $099E ; byte  bolding factor
FOutItalic	EQU $099F ; byte  italic factor
FOutULOffset	EQU $09A0 ; byte  underline offset
FOutULShadow	EQU $09A1 ; byte  underline halo
FOutULThick	EQU $09A2 ; byte  underline thickness
FOutShadow	EQU $09A3 ; byte  shadow factor
FOutExtra	EQU $09A4 ; byte  extra horizontal width
FOutAscent	EQU $09A5 ; byte  height above baseline
FOutDescent	EQU $09A6 ; byte  height below baseline
FOutWidMax	EQU $09A7 ; byte  maximum width of character
FOutLeading	EQU $09A8 ; byte  space between lines
FOutUnused	EQU $09A9 ; byte  unused (padding) byte -must have even number
FOutNumer	EQU $09AA ; long  point for numerators of scale factor
FOutDenom	EQU $09AE ; long  point for denominators of scale factor
FMDotsPerInch	EQU $09B2 ; long  h,v dotsPerInch (resolution) of current device
FMStyleTab	EQU $09B6 ; 24    style heuristic table given by device
ToolScratch	EQU $09CE ; 8     scratch area
WindowList	EQU $09D6 ; long  ptr to Z-ordered linked list of windows
SaveUpdate	EQU $09DA ; word  Enable update events?
PaintWhite	EQU $09DC ; word  erase windows before update event?
WMgrPort	EQU $09DE ; long  ptr to window manager's grafport
DeskPort	EQU $09E2 ; long  ptr to Desk grafPort (Whole screen)
OldStructure	EQU $09E6 ; long  handle to saved structure region
OldContent	EQU $09EA ; long  handle to saved content region
GrayRgn		EQU $09EE ; long  handle to rounded-corner region drawn as the desktop
SaveVisRgn	EQU $09F2 ; long  handle to temporarily saved visRegion
DragHook	EQU $09F6 ; long  ptr to user hook called during dragging
scratch8	EQU $09FA ; 8     general scratch area
TempRect	EQU $09FA ; 8     scratch rectangle
OneOne		EQU $0A02 ; long  holds the constant $00010001
MinusOne	EQU $0A06 ; long  holds the constant $FFFFFFFF
TopMenuItem	EQU $0A0A ; word  pixel value of top of scrollable menu
AtMenuBottom	EQU $0A0C ; word  flag for menu scrolling
IconBitmap	EQU $0A0E ; 14    scratch bitmap used for plotting things
MenuList	EQU $0A1C ; long  handle to current menuBar list structure
MBarEnable	EQU $0A20 ; word  menuBar enable for desk acc's that own the menu bar
CurDeKind	EQU $0A22 ; word  window kind of deactivated window
MenuFlash	EQU $0A24 ; word  flash feedback count
TheMenu		EQU $0A26 ; word  resource ID of hilited menu
SavedHandle	EQU $0A28 ; long  handle to data under a menu
*MrMacHook	EQU $0A2C ; long  Mr. Macintosh hook
MBarHook	EQU $0A2C ; long  ptr to MenuSelect hook called before menu is drawn
MenuHook	EQU $0A30 ; long  ptr to user hook called during MenuSelect
DragPattern	EQU $0A34 ; 8     pattern used to draw outlines of dragged regions
DeskPattern	EQU $0A3C ; 8     pattern used for the desktop
DragFlag	EQU $0A44 ; word  implicit parameter to DragControl
CurDragAction	EQU $0A46 ; long  ptr to implicit actionProc for dragControl
FPState		EQU $0A4A ; 6     floating point state
TopMapHndl	EQU $0A50 ; long  handle to map of most recently opened resource file
SysMapHndl	EQU $0A54 ; long  handle to map of System resourc file
SysMap		EQU $0A58 ; word  reference number of System resource file
CurMap		EQU $0A5A ; word  reference number of current resource file
ResReadOnly	EQU $0A5C ; word  Read-only flag
ResLoad		EQU $0A5E ; word  Auto-load feature
ResErr		EQU $0A60 ; word  Resource Manager error code
TaskLock	EQU $0A62 ; byte  re-entering SystemTask
FScaleDisable	EQU $0A63 ; byte  disable font scaling?
CurActivate	EQU $0A64 ; long  ptr to window slated for activate event
CurDeactive	EQU $0A68 ; long  ptr to window slated for deactivate event
DeskHook	EQU $0A6C ; long  ptr to hook for painting the desk
TEDoText	EQU $0A70 ; long  ptr to textEdit doText proc hook
TERecal		EQU $0A74 ; long  ptr to textEdit recalText proc hook
*MicroSoft	EQU $0A78 ; 12    ApplScratch - for Seattle font
ApplScratch	EQU $0A78 ; 12    application scratch area
GhostWindow	EQU $0A84 ; long  ptr to window never to be considered frontmost
CloseOrnHook	EQU $0A88 ; long  ptr to hook for closing desk ornaments
ResumeProc	EQU $0A8C ; long  ptr to Resume procedure (System error dialog)
SaveProc	EQU $0A90 ; long  address of Save failsafe procedure
SaveSP		EQU $0A94 ; long  Safe stack ptr for restart or save
ANumber		EQU $0A98 ; word  resID of last alert
ACount		EQU $0A9A ; word  number of times last alert was called (0 through 3)
DABeeper	EQU $0A9C ; long  ptr to current beep routine
DAStrings	EQU $0AA0 ; 16    paramText substitution strings (4 handles)
TEScrpLengt	EQU $0AB0 ; long  textEdit Scrap Length
TEScrpHandl	EQU $0AB4 ; long  handle to textEdit Scrap
AppPacks	EQU $0AB8 ; 32    Handles to PACK resources (ID's from 0 to 7)
SysResName	EQU $0AD8 ; 20    name of system resource file (STRING[19])
AppParmHandle	EQU $0AEC ; long  handle to hold application parameters
DSErrCode	EQU $0AF0 ; word  last (or current) system error alert ID
ResErrProc	EQU $0AF2 ; long  ptr to Resource Manager error procedure
TEWdBreak	EQU $0AF6 ; long  ptr to default word break routine
DlgFont		EQU $0AFA ; word  current font number for dialogs and alerts
LastTGLobal	EQU $0AFC ; long  address of last global
; 128K ROM globals
TrapAgain	EQU $0B00 ; long  use 4 bytes here for another trap
;[????]		EQU $0B04 ; word  
ROMMapHndl	EQU $0B06 ; long  handle to ROM resource map
PWMBuf1		EQU $0B0A ; long  ptr to PWM buffer
BootMask	EQU $0B0E ; word  needed during boot
WidthPtr	EQU $0B10 ; long  ptr to global width table
AtalkHk1	EQU $0B14 ; long  ptr to Appletalk hook 1
AtalkHk2	EQU $0B18 ; long  ptr to Appletalk hook 2
;[????]		EQU $0B1C ; long  
;[????]		EQU $0B20 ; word  
HWCfgFlags	EQU $0B22 ; word  hardware configuration flags (two names for this global)
SCSIFlag	EQU $0B22 ; word  SCSI configuration word (bit 15=1 if SCSI installed)
;[????]		EQU $0B24 ; 6     
WidthTabHandle	EQU $0B2A ; long  handle to global width table
;[????]		EQU $0B2E ; 6     
BtDskRfn	EQU $0B34 ; word  boot drive driver reference number
BootTmp8	EQU $0B36 ; 8   temporary space needed by StartBoot
;[????]		EQU $0B3E ; byte  
T1Arbitrate	EQU $0B3F ; byte  holds $FF if Timer T1 up for grabs
;[????]		EQU $0B40 ; 20    
MenuDisable	EQU $0B54 ; long  resID and menuItem of last chosen menu item
;[????]		EQU $0B58 ; 40    
;		EQU $0B80 ; -   switched variables (128 bytes)
RMGRHiVars	EQU $0B80 ; -     RMGR variables (32 bytes)
;[????]		EQU $0B80 ; 14    
RomMapInsert	EQU $0B9E ; byte  flag: insert [resource] map to the ROM resources
TmpResLoad	EQU $0B9F ; byte  temp SetResLoad state for calls using ROMMapInsert
IntlSpec	EQU $0BA0 ; long  international software installed if not -1
;[????]		EQU $0BA4 ; word  
SysFontFam	EQU $0BA6 ; word  if nonzero, the font # for system font
SysFontSize	EQU $0BA8 ; word  if nonzero, the system font size 
MBarHeight	EQU $0BAA ; word  pixel height of menu bar 
;[????]		EQU $0BAC ; long  
NewUnused	EQU $0BC0 ; word  formerly FlEvtMask
LastFOND	EQU $0BC2 ; long  handle to last family record used
;[????]		EQU $0BC4 ; 48    
FractEnable	EQU $0BF4 ; byte  enables fractional widths if not zero
;[????]		EQU $0BF5 ; byte  
;[????]		EQU $0BF6 ; 10    

; A5 offset of qd globals:
;  $0010	finder information handle
; -$00C6  FF3A	fmOutput fontData;	,26	EQU $FFFFFF36 ; FMOutput record
; -$00AC  FF54	fmOutPtr fontPtr;	,4	EQU $FFFFFF50 ; ptr to fontData
; -$00A8  FF58	fixed fontAdj;		,4	EQU $FFFFFF54 ; Fixed Point
; -$00A4  FF5C	point patAlign;		,4	EQU $FFFFFF58 ; Point
; -$00A0  FF60	int polyMax;		,2	EQU $FFFFFF5C ; INTEGER
; -$009E  FF62	polyHandle thePoly;	,4	EQU $FFFFFF5E ; POLYHANDLE
; -$009A  FF66	int playIndex;		,2	EQU $FFFFFF62 ; INTEGER
; -$0098  FF68	picHandle playPic;	,4	EQU $FFFFFF64 ; Long
; -$0094  FF6C	int rgnMax;		,2	EQU $FFFFFF68 ; INTEGER
; -$0092  FF6E	int rgnIndex;		,2	EQU $FFFFFF6A ; INTEGER
; -$0090  FF70	qdHandle rgnBuf;	,4	EQU $FFFFFF6C ; PointsHandle
; -$008C  FF74	region wideData;	,10	EQU $FFFFFF70 ; Fake Region
; -$0082  FF7E	rgnPtr wideMaster;	,4	EQU $FFFFFF7A ; RgnPtr
; -$007E  FF82	rgnHandle wideOpen;	,4	EQU $FFFFFF7E ; RgnHandle
; -$007A  FF86	long randSeed;		,4	EQU $FFFFFF82 ; [long]
; -$0068  FF98	bitMap screenBits;	,14	EQU $FFFFFF86 ; [BitMap]
; -$0068  FF98	cursor arrow;		,68	EQU $FFFFFF94 ; [Cursor]
; -$002C  FFD4	pattern dkGray;		,8	EQU $FFFFFFD8 ; [Pattern]
; -$0024  FFDC	pattern ltGray;		,8	EQU $FFFFFFE0 ; [Pattern]
; -$001C  FFE4	pattern gray;		,8	EQU $FFFFFFE8 ; [Pattern]
; -$0014  FFEC	pattern black;		,8	EQU $FFFFFFF0 ; [Pattern]
; -$000C  FFF4	pattern white;		,8	EQU $FFFFFFF8 ; [Pattern]
; -$0004  FFFC	grafptr thePort;	,4	EQU $0 ; [GrafPtr]
; -$0000  0000	pointer to qd globals, set to -4(A5) by InitGraf(qd.thePort)
; GrafPort structure
; 00      ; device:     Integer;    {device-specific information}
; 02      ; portBits:   BitMap;     {bitmap}
; 10      ; portBounds: Rect;
; 18      ; portRect:   Rect;       {port rectangle}
; 20      ; visRgn:     RgnHandle;  {visible region}
; 24      ; clipRgn:    RgnHandle;  {clipping region}
; 28      ; bkPat:      Pattern;    {background pattern}
; 30      ; fillPat:    Pattern;    {fill pattern}
; 38      ; pnLoc:      Point;      {pen location}
; 3C      ; pnSize:     Point;      {pen size}
; 40      ; pnMode:     Integer;    {pattern mode}
; 42      ; pnPat:      Pattern;    {pen pattern}
; 4A      ; pnVis:      Integer;    {pen visibility}
; 4C      ; txFont:     Integer;    {font number for text}
; 4E      ; txFace:     Style;      {text's font style}
; 50      ; txMode:     Integer;    {source mode for text}
; 52      ; txSize:     Integer;    {font size for text}
; 54      ; spExtra:    Fixed;      {extra space}
; 58      ; fgColor:    LongInt;    {foreground color}
; 5C      ; bkColor:    LongInt;    {background color}
; 60      ; colrBit:    Integer;    {color bit}
; 62      ; patStretch: Integer;    {used internally}
; 64      ; picSave:    Handle;     {picture being saved, used internally}
; 68      ; rgnSave:    Handle;     {region being saved, used internally}
; 6C      ; polySave:   Handle;     {polygon being saved, used internally}
; 70      ; grafProcs:  QDProcsPtr; {low-level drawing routines}

;Ax0x ==========
_Open		macro
		dc.w 0xA000
		endm
_Close		macro
		dc.w 0xA001
		endm
_Read		macro
		dc.w 0xA002
		endm
_Write		macro
		dc.w 0xA003
		endm
_Write_A	macro
		dc.w 0xA403
		endm
_Control	macro
		dc.w 0xA004
		endm
_Status		macro
		dc.w 0xA005
		endm
; 0xA006 "_KillIO"
_GetVolInfo	macro
		dc.w 0xA007
		endm
_Create		macro
		dc.w 0xA008
		endm
_Delete		macro
		dc.w 0xA009
		endm
; 0xA00A "_OpenRF"
_Rename		macro
		dc.w 0xA00B
		endm
_GetFileInfo	macro
		dc.w 0xA00C
		endm
_SetFileInfo	macro
		dc.w 0xA00D
		endm
; 0xA00E "_UnmountVol"
; 0xA00F "_MountVol"
;Ax1x ==========
; 0xA010 "_Allocate"
_GetEOF		macro
		dc.w 0xA011
		endm
_SetEOF		macro
		dc.w 0xA012
		endm
_FlushVol	macro
		dc.w 0xA013
		endm
; 0xA014 "_GetVol"
; 0xA015 "_SetVol"
; 0xA016 "_InitQueue"
; 0xA017 "_Eject"
; 0xA018 "_GetFPos"
; 0xA019 "_InitZone"
; 0xA11A "_GetZone"
; 0xA01B "_SetZone"
_FreeMem	macro
		dc.w 0xA01C
		endm
_MaxMem		macro
		dc.w 0xA11D
		endm
_NewPtr		macro
		dc.w 0xA11E
		endm
_DisposPtr	macro
		dc.w 0xA01F
		endm
;Ax2x ==========
; 0xA020 "_SetPtrSize"
; 0xA021 "_GetPtrSize"
_NewHandle	macro
		dc.w 0xA122
		endm
_DisposHandle	macro
		dc.w 0xA023
		endm
_SetHandleSize	macro
		dc.w 0xA024
		endm
_GetHandleSize	macro
		dc.w 0xA025
		endm
; 0xA126 "_HandleZone"
_ReallocHandle	macro
		dc.w 0xA027
		endm
; 0xA128 "_RecoverHandle"
_HLock		macro
		dc.w 0xA029
		endm
_HUnlock	macro
		dc.w 0xA02A
		endm
; 0xA02B "_EmptyHandle"
; 0xA02C "_InitApplZone"
_SetApplLimit	macro
		dc.w 0xA02D
		endm
; 0xA02E "_BlockMove"
; 0xA02F "_PostEvent"
;Ax3x ==========
; 0xA030 "_OSEventAvail"
; 0xA031 "_GetOSEvent"
_FlushEvents	macro
		dc.w 0xA032
		endm
_VInstall	macro
		dc.w 0xA033
		endm
_VRemove	macro
		dc.w 0xA034
		endm
; 0xA035 "_Offline"
_MoreMasters	macro
		dc.w 0xA036
		endm
; 0xA037 "_ReadParam"
; 0xA038 "_WriteParam"
_ReadDateTime	macro
		dc.w 0xA039
		endm
_SetDateTime	macro
		dc.w 0xA03A
		endm
_Delay		macro
		dc.w 0xA03B
		endm
; 0xA03C "_CmpString"
; 0xA03D "_DrvrInstall"
; 0xA03E "_DrvrRemove"
; 0xA03F "_InitUtil"
;Ax4x ==========
_ResrvMem	macro
		dc.w 0xA040
		endm
; 0xA041 "_SetFilLock"
; 0xA042 "_RstFilLock"
; 0xA043 "_SetFilType"
_SetFPos	macro
		dc.w 0xA044
		endm
_FlushFile	macro
		dc.w 0xA045
		endm
; 0xA146 "_GetTrapAddress"
; 0xA047 "_SetTrapAddress"
; 0xA148 "_PtrZone"
; 0xA049 "_HPurge"
; 0xA04A "_HNoPurge"
_SetGrowZone	macro
		dc.w 0xA04B
		endm
_CompactMem	macro
		dc.w 0xA14C
		endm
_PurgeMem	macro
		dc.w 0xA14D
		endm
; 0xA04E "_AddDrive"
; 0xA04F "_RDrvrInstall"
;A85x ==========
_InitCursor	macro
		dc.w 0xA850
		endm
_SetCursor	macro
		dc.w 0xA851
		endm
; 0xA852 "_HideCursor"
_ShowCursor	macro
		dc.w 0xA853
		endm
; 0xA054 "_UprString"
; 0xA855 "_ShieldCursor"
; 0xA856 "_ObscureCursor"
; 0xA057 "_SetAppBase"
; 0xA858 "_BitAnd"
; 0xA859 "_BitXor"
; 0xA85A "_BitNot"
; 0xA85B "_BitOr"
; 0xA85C "_BitShift"
; 0xA85D "_BitTst"
; 0xA85E "_BitSet"
; 0xA85F "_BitClr"
;A86x ==========
; 0xA860 "_WaitNextEvent"
; 0xA861 "_Random"
; 0xA862 "_ForeColor"
; 0xA863 "_BackColor"
; 0xA864 "_ColorBit"
_GetPixel	macro
		dc.w 0xA865
		endm
; 0xA866 "_StuffHex"
; 0xA867 "_LongMul"
; 0xA868 "_FixMul"
; 0xA869 "_FixRatio"
; 0xA86A "_HiWord"
; 0xA86B "_LoWord"
; 0xA86C "_FixRound"
; 0xA86D "_InitPort"
_InitGraf	macro
		dc.w 0xA86E
		endm
; 0xA86F "_OpenPort"
;A87x ==========
; 0xA870 "_LocalToGlobal"
_GlobalToLocal	macro
		dc.w 0xA871
		endm
; 0xA872 "_GrafDevice"
_SetPort	macro
		dc.w 0xA873
		endm
; 0xA874 "_GetPort"
; 0xA875 "_SetPBits"
; 0xA876 "_PortSize"
_MovePortTo	macro
		dc.w 0xA877
		endm
_SetOrigin	macro
		dc.w 0xA878
		endm
; 0xA879 "_SetClip"
; 0xA87A "_GetClip"
_ClipRect	macro
		dc.w 0xA87B
		endm
_BackPat	macro
		dc.w 0xA87C
		endm
; 0xA87D "_ClosePort"
; 0xA87E "_AddPt"
; 0xA87F "_SubPt"
;A88x ==========
; 0xA880 "_SetPt"
; 0xA881 "_EqualPt"
; 0xA882 "_StdText"
_DrawChar	macro
		dc.w 0xA883
		endm
; 0xA884 "_DrawString"
; 0xA885 "_DrawText"
; 0xA886 "_TextWidth"
_TextFont	macro
		dc.w 0xA887
		endm
_TextFace	macro
		dc.w 0xA888
		endm
_TextMode	macro
		dc.w 0xA889
		endm
_TextSize	macro
		dc.w 0xA88A
		endm
_GetFontInfo	macro
		dc.w 0xA88B
		endm
; 0xA88C "_StringWidth"
_CharWidth	macro
		dc.w 0xA88D
		endm
; 0xA88E "_SpaceExtra"
; 0xA88F "_JugglerDispatch"
;A89x ==========
; 0xA890 "_StdLine"
_LineTo		macro
		dc.w 0xA891
		endm
; 0xA892 "_Line"
_MoveTo		macro
		dc.w 0xA893
		endm
; 0xA894 "_Move"
; 0xA895 "_ShutDown"
; 0xA896 "_HidePen"
; 0xA897 "_ShowPen"
_GetPenState	macro
		dc.w 0xA898
		endm
_SetPenState	macro
		dc.w 0xA899
		endm
; 0xA89A "_GetPen"
; 0xA89B "_PenSize"
_PenMode	macro
		dc.w 0xA89C
		endm
_PenPat		macro
		dc.w 0xA89D
		endm
_PenNormal	macro
		dc.w 0xA89E
		endm
; 0xA89F "_UnimplTrap"
;A8Ax ==========
; 0xA8A0 "_StdRect"
_FrameRect	macro
		dc.w 0xA8A1
		endm
_PaintRect	macro
		dc.w 0xA8A2
		endm
_EraseRect	macro
		dc.w 0xA8A3
		endm
_InverRect	macro
		dc.w 0xA8A4
		endm
; 0xA8A5 "_FillRect"
; 0xA8A6 "_EqualRect"
; 0xA8A7 "_SetRect"
_OffsetRect	macro
		dc.w 0xA8A8
		endm
_InsetRect	macro
		dc.w 0xA8A9
		endm
_SectRect	macro
		dc.w 0xA8AA
		endm
; 0xA8AB "_UnionRect"
; 0xA8AC "_Pt2Rect"
_PtInRect	macro
		dc.w 0xA8AD
		endm
; 0xA8AE "_EmptyRect"
; 0xA8AF "_StdRRect"
;A8Bx ==========
; 0xA8B0 "_FrameRoundRect"
; 0xA8B1 "_PaintRoundRect"
; 0xA8B2 "_EraseRoundRect"
; 0xA8B3 "_InverRoundRect"
; 0xA8B4 "_FillRoundRect"
; 0xA8B6 "_StdOval"
; 0xA8B7 "_FrameOval"
; 0xA8B8 "_PaintOval"
; 0xA8B9 "_EraseOval"
; 0xA8BA "_InvertOval"
; 0xA8BB "_FillOval"
; 0xA8BC "_SlopeFromAngle"
; 0xA8BD "_StdArc"
_FrameArc	macro
		dc.w 0xA8BE
		endm
; 0xA8BF "_PaintArc"
;A8Cx ==========
; 0xA8C0 "_EraseArc"
; 0xA8C1 "_InvertArc"
; 0xA8C2 "_FillArc"
; 0xA8C3 "_PtToAngle"
; 0xA8C4 "_AngleFromSlope"
; 0xA8C5 "_StdPoly"
; 0xA8C6 "_FramePoly"
; 0xA8C7 "_PaintPoly"
; 0xA8C8 "_ErasePoly"
; 0xA8C9 "_InvertPoly"
; 0xA8CA "_FillPoly"
; 0xA8CB "_OpenPoly"
; 0xA8CC "_ClosePgon"
; 0xA8CD "_KillPoly"
; 0xA8CE "_OffsetPoly"
; 0xA8CF "_PackBits"
;A8Dx ==========
; 0xA8D0 "_UnpackBits"
; 0xA8D1 "_StdRgn"
; 0xA8D2 "_FrameRgn"
; 0xA8D3 "_PaintRgn"
; 0xA8D4 "_EraseRgn"
; 0xA8D5 "_InverRgn"
; 0xA8D6 "_FillRgn"
; A8D7 not used?
_NewRgn		macro
		dc.w 0xA8D8
		endm
_DisposRgn	macro
		dc.w 0xA8D9
		endm
; 0xA8DA "_OpenRgn"
; 0xA8DB "_CloseRgn"
; 0xA8DC "_CopyRgn"
; 0xA8DD "_SetEmptyRgn"
; 0xA8DE "_SetRecRgn"
; 0xA8DF "_RectRgn"
;A8Ex ==========
_OfsetRgn	macro
		dc.w 0xA8E0
		endm
; 0xA8E1 "_InsetRgn"
; 0xA8E2 "_EmptyRgn"
; 0xA8E3 "_EqualRgn"
; 0xA8E4 "_SectRgn"
; 0xA8E5 "_UnionRgn"
; 0xA8E6 "_DiffRgn"
; 0xA8E7 "_XorRgn"
; 0xA8E8 "_PtInRgn"
; 0xA8E9 "_RectInRgn"
; 0xA8EA "_SetStdProcs"
; 0xA8EB "_StdBits"
_CopyBits	macro
		dc.w 0xA8EC
		endm
; 0xA8ED "_StdTxMeas"
; 0xA8EE "_StdGetPic"
_ScrollRect	macro
		dc.w 0xA8EF
		endm
;A8Fx ==========
; 0xA8F0 "_StdPutPic"
; 0xA8F1 "_StdComment"
; 0xA8F2 "_PicComment"
_OpenPicture	macro
		dc.w 0xA8F3
		endm
_ClosePicture	macro
		dc.w 0xA8F4
		endm
_KillPicture	macro
		dc.w 0xA8F5
		endm
_DrawPicture	macro
		dc.w 0xA8F6
		endm
; AF87 not used?
; 0xA8F8 "_ScalePt"
; 0xA8F9 "_MapPt"
; 0xA8FA "_MapRect"
; 0xA8FB "_MapRgn"
; 0xA8FC "_MapPoly"
; 0xA8FD "_PrGlue"
_InitFonts	macro
		dc.w 0xA8FE
		endm
; 0xA8FF "_GetFName"
;A90x ==========
; 0xA900 "_GetFNum"
; 0xA901 "_FMSwapFont"
; 0xA902 "_RealFont"
; 0xA903 "_SetFontLock"
; 0xA904 "_DrawGrowIcon"
; 0xA905 "_DragGrayRgn"
; 0xA906 "_NewString"
; 0xA907 "_SetString"
; 0xA908 "_ShowHide"
; 0xA909 "_CalcVis"
; 0xA90A "_CalcVBehind"
; 0xA90B "_ClipAbove"
; 0xA90C "_PaintOne"
; 0xA90D "_PaintBehind"
; 0xA90E "_SaveOld"
; 0xA90F "_DrawNew"
;A91x ==========
; 0xA910 "_GetWMgrPort"
; 0xA911 "_CheckUpdate"
_InitWindows	macro
		dc.w 0xA912
		endm
_NewWindow	macro
		dc.w 0xA913
		endm
; 0xA914 "_DisposWindow"
_ShowWindow	macro
		dc.w 0xA915
		endm
_HideWindow	macro
		dc.w 0xA916
		endm
; 0xA917 "_GetWRefCon"
; 0xA918 "_SetWRefCon"
; 0xA919 "_GetWTitle"
; 0xA91A "_SetWTitle"
_MoveWindow	macro
		dc.w 0xA91B
		endm
; 0xA91C "_HiliteWindow"
_SizeWindow	macro
		dc.w 0xA91D
		endm
_TrackGoAway	macro
		dc.w 0xA91E
		endm
_SelectWindow	macro
		dc.w 0xA91F
		endm
;A92x ==========
; 0xA920 "_BringToFront"
; 0xA921 "_SendBehind"
_BeginUpdate	macro
		dc.w 0xA922
		endm
_EndUpdate	macro
		dc.w 0xA923
		endm
; 0xA924 "_FrontWindow"
_DragWindow	macro
		dc.w 0xA925
		endm
; 0xA926 "_DragTheRgn"
_InvalRgn	macro
		dc.w 0xA927
		endm
_InvalRect	macro
		dc.w 0xA928
		endm
_ValidRgn	macro
		dc.w 0xA929
		endm
_ValidRect	macro
		dc.w 0xA92A
		endm
_GrowWindow	macro
		dc.w 0xA92B
		endm
_FindWindow	macro
		dc.w 0xA92C
		endm
_CloseWindow	macro
		dc.w 0xA92D
		endm
; 0xA92E "_SetWindowPic"
; 0xA92F "_GetWindowPic"
;A93x ==========
_InitMenus	macro
		dc.w 0xA930
		endm
_NewMenu	macro
		dc.w 0xA931
		endm
_DisposMenu	macro
		dc.w 0xA932
		endm
_AppendMenu	macro
		dc.w 0xA933
		endm
_ClearMenuBar	macro
		dc.w 0xA934
		endm
_InsertMenu	macro
		dc.w 0xA935
		endm
_DeleteMenu	macro
		dc.w 0xA936
		endm
_DrawMenuBar	macro
		dc.w 0xA937
		endm
_HiliteMenu	macro
		dc.w 0xA938
		endm
; 0xA939 "_EnableItem"
; 0xA93A "_DisableItem"
_GetMenuBar	macro
		dc.w 0xA93B
		endm
_SetMenuBar	macro
		dc.w 0xA93C
		endm
_MenuSelect	macro
		dc.w 0xA93D
		endm
_MenuKey	macro
		dc.w 0xA93E
		endm
; 0xA93F "_GetItmIcon"
;A94x ==========
; 0xA940 "_SetItmIcon"
; 0xA941 "_GetItmStyle"
; 0xA942 "_SetItmStyle"
; 0xA943 "_GetItmMark"
; 0xA944 "_SetItmMark"
_CheckItem	macro
		dc.w 0xA945
		endm
_GetItem	macro
		dc.w 0xA946
		endm
_SetItem	macro
		dc.w 0xA947
		endm
; 0xA948 "_CalcMenuSize"
_GetMHandle	macro
		dc.w 0xA949
		endm
; 0xA94A "_SetMFlash"
; 0xA94B "_PlotIcon"
; 0xA94C "_FlashMenuBar"
_AddResMenu	macro
		dc.w 0xA94D
		endm
; 0xA94E "_PinRect"
; 0xA94F "_DeltaPoint"
;A95x ==========
; 0xA950 "_CountMItems"
; 0xA951 "_InsertResMenu"
; 0xA952 "_DelMenuItem"
; 0xA953 "_UpdtControls"
_NewControl	macro
		dc.w 0xA954
		endm
_DisposControl	macro
		dc.w 0xA955
		endm
; 0xA956 "_KillControls"
_ShowControl	macro
		dc.w 0xA957
		endm
_HideControl	macro
		dc.w 0xA958
		endm
_MoveControl	macro
		dc.w 0xA959
		endm
; 0xA95A "_GetCRefCon"
; 0xA95B "_SetCRefCon"
_SizeControl	macro
		dc.w 0xA95C
		endm
_HiliteControl	macro
		dc.w 0xA95D
		endm
; 0xA95E "_GetCTitle"
; 0xA95F "_SetCTitle"
;A96x ==========
_GetCtlValue	macro
		dc.w 0xA960
		endm
; 0xA961 "_GetMinCtl"
; 0xA962 "_GetMaxCtl"
_SetCtlValue	macro
		dc.w 0xA963
		endm
_SetMinCtl	macro
		dc.w 0xA964
		endm
_SetMaxCtl	macro
		dc.w 0xA965
		endm
; 0xA966 "_TestControl"
; 0xA967 "_DragControl"
_TrackControl	macro
		dc.w 0xA968
		endm
_DrawControls	macro
		dc.w 0xA969
		endm
; 0xA96A "_GetCtlAction"
; 0xA96B "_SetCtlAction"
; 0xA96C "_FindControl"
;A96D not used?
; 0xA96E "_Dequeue"
; 0xA96F "_Enqueue"
_FindControl	macro
		dc.w 0xA96C
		endm
;A97x ==========
_GetNextEvent	macro
		dc.w 0xA970
		endm
_EventAvail	macro
		dc.w 0xA971
		endm
_GetMouse	macro
		dc.w 0xA972
		endm
_StillDown	macro
		dc.w 0xA973
		endm
; 0xA974 "_Button"
; 0xA975 "_TickCount"
_GetKeys	macro
		dc.w 0xA976
		endm
; 0xA977 "_WaitMouseUp"
; 0xA979 "_CouldDialog"
; 0xA97A "_FreeDialog"
_InitDialogs	macro
		dc.w 0xA97B
		endm
_GetNewDialog	macro
		dc.w 0xA97C
		endm
; 0xA97D "_NewDialog"
_SelIText	macro
		dc.w 0xA97E
		endm
; 0xA97F "_IsDialogEvent"
;A98x ==========
; 0xA980 "_DialogSelect"
_DrawDialog	macro
		dc.w 0xA981
		endm
; 0xA982 "_CloseDialog"
_DisposDialog	macro
		dc.w 0xA983
		endm
; 0xA984 "_FindDItem"
; 0xA985 "_Alert"
; 0xA986 "_StopAlert"
; 0xA987 "_NoteAlert"
; 0xA988 "_CautionAlert"
; 0xA989 "_CouldAlert"
; 0xA98A "_FreeAlert"
; 0xA98B "_ParamText"
; 0xA98C "_ErrorSound"
_GetDItem	macro
		dc.w 0xA98D
		endm
; 0xA98E "_SetDItem"
_SetIText	macro
		dc.w 0xA98F
		endm
;A99x ==========
_GetIText	macro
		dc.w 0xA990
		endm
_ModalDialog	macro
		dc.w 0xA991
		endm
; 0xA992 "_DetachResource"
; 0xA993 "_SetResPurge"
; 0xA994 "_CurResFile"
; 0xA995 "_InitResources"
; 0xA996 "_RsrcZoneInit"
_OpenResFile	macro
		dc.w 0xA997
		endm
_UseResFile	macro
		dc.w 0xA998
		endm
; 0xA999 "_UpdateResFile"
; 0xA99B "_SetResLoad"
; 0xA99C "_CountResources"
; 0xA99D "_GetIndResource"
; 0xA99E "_CountTypes"
; 0xA99F "_GetIndType"
_CloseResFile	macro
		dc.w 0xA99A
		endm
;A9Ax ==========
_GetResource	macro
		dc.w 0xA9A0
		endm
_GetNamedResource macro
		dc.w 0xA9A1
		endm
_LoadResource	macro
		dc.w 0xA9A2
		endm
; 0xA9A3 "_ReleaseResource"
_HomeResFile	macro
		dc.w 0xA9A4
		endm
; 0xA9A5 "_SizeRsrc"
; 0xA9A6 "_GetResAttrs"
; 0xA9A7 "_SetResAttrs"
; 0xA9A8 "_GetResInfo"
; 0xA9A9 "_SetResInfo"
; 0xA9AA "_ChangedResource"
; 0xA9AB "_AddResource"
; 0xA9AC "_AddReference"
; 0xA9AD "_RmveResource"
; 0xA9AE "_RmveReference"
; 0xA9AF "_ResError"
;A9Bx ==========
; 0xA9B0 "_WriteResource"
; 0xA9B1 "_CreateResFile"
; 0xA9B2 "_SystemEvent"
_SystemClick	macro
		dc.w 0xA9B3
		endm
_SystemTask	macro
		dc.w 0xA9B4
		endm
; 0xA9B5 "_SystemMenu"
_OpenDeskAcc	macro
		dc.w 0xA9B6
		endm
; 0xA9B7 "_CloseDeskAcc"
; 0xA9B8 "_GetPattern"
; 0xA9B9 "_GetCursor"
; 0xA9BA "_GetString"
; 0xA9BB "_GetIcon"
; 0xA9BC "_GetPicture"
_GetNewWindow	macro
		dc.w 0xA9BD
		endm
; 0xA9BE "_GetNewControl"
_GetRMenu	macro
		dc.w 0xA9BF
		endm
;A9Cx ==========
; 0xA9C0 "_GetNewMBar"
; 0xA9C1 "_UniqueID"
_SysEdit	macro
		dc.w 0xA9C2
; A9C3 not used?
; 0xA9C4 "_OpenRFPerm"
; 0xA9C5 "_RsrcMapEntry"
		endm
_Secs2Date	macro
		dc.w 0xA9C6
		endm
_Date2Secs	macro
		dc.w 0xA9C7
		endm
; 0xA9C8 "_SysBeep"
_SysError	macro
		dc.w 0xA9C9
		endm
; 0xA9CA "_PutIcon"
_TEGetText	macro
		dc.w 0xA9CB
		endm
_TEInit		macro
		dc.w 0xA9CC
		endm
_TEDispose	macro
		dc.w 0xA9CD
		endm
; 0xA9CE "_TextBox"
_TESetText	macro
		dc.w 0xA9CF
		endm
;A9Dx ==========
_TECalText	macro
		dc.w 0xA9D0
		endm
_TESetSelect	macro
		dc.w 0xA9D1
		endm
_TENew		macro
		dc.w 0xA9D2
		endm
_TEUpdate	macro
		dc.w 0xA9D3
		endm
_TEClick	macro
		dc.w 0xA9D4
		endm
_TECopy		macro
		dc.w 0xA9D5
		endm
_TECut		macro
		dc.w 0xA9D6
		endm
_TEDelete	macro
		dc.w 0xA9D7
		endm
_TEActivate	macro
		dc.w 0xA9D8
		endm
_TEDeactivate	macro
		dc.w 0xA9D9
		endm
_TEIdle		macro
		dc.w 0xA9DA
		endm
_TEPaste	macro
		dc.w 0xA9DB
		endm
_TEKey		macro
		dc.w 0xA9DC
; 0xA9DD "_TEScroll"
		endm
_TEInsert	macro
		dc.w 0xA9DE
		endm
_TESetJust	macro
		dc.w 0xA9DF
		endm
;A9Ex ==========
; 0xA9E0 "_Munger"
; 0xA9E1 "_HandToHand"
; 0xA9E2 "_PtrToXHand"
; 0xA9E3 "_PtrToHand"
; 0xA9E4 "_HandAndHand"
_InitPack	macro
		dc.w 0xA9E5
		endm
; 0xA9E6 "_InitAllPacks"
; 0xA9E7 "_Pack0"
; 0xA9E8 "_Pack1"
; 0xA9E9 "_Pack2" ; (Disk Initialization)
_Pack3		macro ; // (Standard File Package)
		dc.w 0xA9EA
		endm
; 0xA9EB "_Pack4" // (FP68K)
; 0xA9EC "_Pack5" // (Elems68K)
; 0xA9ED "_Pack6" // (Intl Utilites Package)
; 0xA9EE "_Pack7" // (Bin/Dec Conversion)
; 0xA9EF "_PtrAndHand"
;A9Fx ==========
_LoadSeg	macro
		dc.w 0xA9F0
		endm
_UnloadSeg	macro
		dc.w 0xA9F1
		endm
; 0xA9F2 "_Launch"
; 0xA9F3 "_Chain"
; 0xA9F4 "_ExitToShell"
; 0xA9F5 "_GetAppParms"
; 0xA9F6 "_GetResFileAttrs"
; 0xA9F7 "_SetResFileAttrs"
; 0xA9F9 "_InfoScrap"
_UnlodeScrap	macro
		dc.w 0xA9FA
		endm
_LodeScrap	macro
		dc.w 0xA9FB
		endm
_ZeroScrap	macro
		dc.w 0xA9FC
		endm
_GetScrap	macro
		dc.w 0xA9FD
		endm
_PutScrap	macro
		dc.w 0xA9FE
		endm
_MacsBug	macro
		dc.w 0xA9FF
		endm
_MacsBugStr	macro
		dc.w 0xABFF
		endm
; ==============

