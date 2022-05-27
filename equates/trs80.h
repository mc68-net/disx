; constants and other info for TRS-80 Model I/III code
	LIST	OFF

; TRSDOS/LDOS calls
; these names are properly prefixed with "@", but
; many assemblers might not allow that in symbol names
_WHERE	EQU	000BH	; returns address of caller
_GET	EQU	0013H	; read a byte from a device or file
_PUT	EQU	001BH	; output a byte to a device or file
_CTL	EQU	0023H	; output a control byte to a device or file
_KBD	EQU	002BH	; scan keyboard and return key
_DSP	EQU	0033H	; output a byte to the display
_PRT	EQU	003BH	; output a byte to the printer
_KEYIN	EQU	0040H	; input a line of text
_KEY	EQU	0049H	; scan keyboard and return key
_PAUSE	EQU	0060H	; delay loop
_CLS	EQU	01C9H	; clear the screen
_DATE	EQU	3033H	; get date in xx/xx/xx format (M3 only)
_TIME	EQU	3036H	; get time in xx:xx:xx format (M3 only)
;KIDCB$	EQU	4015H	; keyboard DCB
;DODCB$	EQU	401DH	; video DCB
;PRDCB$	EQU	4025H	; printer DCB
_EXIT	EQU	402DH	; normal exit to command line
_ABORT	EQU	4030H	; return to command line and abort any script execution
_DVRHK	EQU	4033H	; device driver hook from ROM for byte I/O
_ADTSK	EQU	403DH	; add an RTC interrupt task
_RMTSK	EQU	4040H	; remove an RTC interrupt task
_RPTSK	EQU	4043H	; replace an RTC interrupt task
_KLTSK	EQU	4046H	; end an RTC interrupt task
;DBGSV$	EQU	405DH	; DEBUG and SYSTEM storage area
;TIME$	EQU	4217H	; contains time of day
;DATE$	EQU	421AH	; contains the current date
_ICNFG	EQU	421DH	; initialize configuration
;JDCB$	EQU	4220H	; storage for DCB address during JCL exec
;JRET$	EQU	4222H	; storage for RET address during JCL exec
;INBUF$	EQU	4225H	; 64 byte buffer for command input
;JFCB$	EQU	4265H	; JCL FCB during DO processing
;_KITSK	EQU	4285H	; task processing during keyboard scan
;TIMER$	EQU	4288H	; 33.333ms counter
;DFLAG$	EQU	4289H	; system device flag
_LOGOT	EQU	428AH	; display and log a message (= _DSPLY + _LOGER)
_LOGER	EQU	428DH	; add a log message to the job log
_CKDRV	EQU	4290H	; check that a drive has a disk in place
_FNAME	EQU	4293H	; get file name and extension from directory
_CMD	EQU	4296H	; return to command line (identical to _EXIT?)
_CMNDI	EQU	4299H	; exit to command line and execute a command
;SFCB$	EQU	42A1H	; FCB for loading system overlays
;KISV$	EQU	42B8H	; save KI DCB vector
;DOSV$	EQU	42BAH	; save DO DCB vector
;PRSV$	EQU	42BCH	; save PR DCB vector
;KIJCL$	EQU	42BEH	; save KIJCL DCB vector
;JLDCB$	EQU	42C2H	; joblog DCB
;SIDCB$	EQU	42C8H	; standard input DCB
;SODCB$	EQU	42CEH	; standard output DCB
;S1DCB$	EQU	42D4H	; spare dcb
;S2DCB$	EQU	42DAH	; spare dcb
;S3DCB$	EQU	42E0H	; spare dcb
;S4DCB$	EQU	42HE6	; spare dcb
;	EQU	42ECH	; storage for 2-char device names
;SBUFF$	EQU	4300H	; 256-byte buffer for disk I/O
;EXTDBG$ EQU	4400H	; vector to extended DEBUG
_MSG	EQU	4402H	; send message to any device
_DBGHK	EQU	4405H	; used with DEBUG
_ERROR	EQU	4409H	; print an error message and optionally abort
_DEBUG	EQU	440DH	; start debugger
;HIGH$	EQU	4411H	; highest unused RAM address
;OVRLY$	EQU	4414H	; 
_DODIR	EQU	4419H	; display directory file list or free space
_FSPEC	EQU	441CH	; parse a file spec into an FCB
;OSVER$	EQU	441FH	; 
_INIT	EQU	4420H	; open a file, create if necessary
;PDRIV$	EQU	4423H	; currently accessed drive, 1/2/4/8
_OPEN	EQU	4424H	; open an existing file
;LDRIV$	EQU	4427H	; currently accessed drive, 0-7
_CLOSE	EQU	4428H	; close a file
;SFLAG$	EQU	442BH	; system bit flag
_KILL	EQU	442CH	; delete a file
_LOAD	EQU	4430H	; load a program file in /CMD format
_RUN	EQU	4433H	; load and execute a program file
_READ	EQU	4436H	; read next record
_WRITE	EQU	4439H	; write next record
_VER	EQU	443CH	; write and verity sector
_REW	EQU	443FH	; rewind a file to start
_POSN	EQU	4442H	; position a file to a logical record
_BKSP	EQU	4445H	; backspace one logical record
_PEOF	EQU	4448H	; position a file to end-of-file position
_FEXT	EQU	444BH	; insert default file extension into FCB
_MULT	EQU	444EH	; multiply HL * A -> HL:A
_DIVIDE	EQU	4451H	; divide HL / A -> HL,A
_PARAM	EQU	4454H	; parse command options
_CKEOF	EQU	4458H	; check for EOF at current logical record number
_WEOF	EQU	445BH	; update directory with current EOF
_RREAD	EQU	445EH	; reread the current sector before next I/O
_RWRIT	EQU	4461H	; rewrite the current sector following a write
_SKIP	EQU	4464H	; skip past next logical record
_DSPLY	EQU	4467H	; display a message line until 0x0D or 0x03
_PRINT	EQU	446AH	; outputs a message line to the printer
_LOC	EQU	446DH	; calculate the curent logical record number
_LOF	EQU	4470H	; calculate the EOF logical record number
;INTIM$	EQU	4473H	; contains an image of the interrupt latch
;INTVCT$ EQU	4475H	; eight vectors, one for each interrupt latch
;CFCB$	EQU	4485H	; file control block for commands
;TCB$	EQU	4500H	; interrupt task table
;DCT$	EQU	4700H	; reserved for drive control table
_SELECT	EQU	4754H	; select a drive and delay if necessary
_TSTBSY	EQU	4759H	; test if selected drive is busy
_SEEK	EQU	475EH	; seek drive to a specified cylinder
_WRSEC	EQU	4763H	; write a sector to disk
_WRSYS	EQU	4768H	; write a sector to disk with system attribute
_WRCYL	EQU	476DH	; write a cylinder/track to disk
_VERSEC	EQU	4772H	; verify a sector on disk
_RDSEC	EQU	4777H	; read a sector from disk
_GETDCT	EQU	478FH	; get drive code table address
_DCTBYT	EQU	479CH	; get a byte from a drive code table entry
_DIRRD	EQU	4B10H	; read directory sector
_DIRWR	EQU	4B1FH	; write directory sector back
_RDSSEC	EQU	4B45H	; read a system (directory) sector
_DIRCYL	EQU	4B64H	; returns directory cylinder for a drive
_MULTEA	EQU	4B6BH	; multiply  A*E -> A
_DIVEA	EQU	4B7AH	; divide E/A -> A,E

; errors
;	EQU	00H	; no error
;	EQU	01H	; parity error during header read
;	EQU	02H	; seek error during read
;	EQU	03H	; lost data during read
;	EQU	04H	; parity error during read
;	EQU	05H	; data record not found during read
;	EQU	06H	; attempted to read system data sector
;	EQU	07H	; attempted to read locked/deleted sector
;	EQU	08H	; device not available
;	EQU	09H	; parity error during header write
;	EQU	0AH	; seek error during write
;	EQU	0BH	; lost data during write
;	EQU	0CH	; parity error during write
;	EQU	0DH	; data record not found during write
;	EQU	0EH	; write fault on disk drive
;	EQU	0FH	; write protected disk
;	EQU	10H	; illegal logical file number
;	EQU	11H	; directory read error
;	EQU	12H	; directory write error
;	EQU	13H	; illegal file name
;	EQU	14H	; GAT read error
;	EQU	15H	; GAT write error
;	EQU	16H	; HIT read error
;	EQU	17H	; HIT write error
;	EQU	18H	; file not in directory
;	EQU	19H	; file access denied
;	EQU	1AH	; full or write-protected disk
;	EQU	1BH	; disk space full
;	EQU	1CH	; end of file encountered
;	EQU	1DH	; record number out of range
;	EQU	1EH	; directory full, can't extend file
;	EQU	1FH	; program not found
;	EQU	20H	; illegal drive number
;	EQU	21H	; no device space available
;	EQU	22H	; load file format error
;	EQU	23H	; memory fault
;	EQU	24H	; attempted to load read-only memory
;	EQU	25H	; illegal access attempted to protected file
;	EQU	26H	; file not open
;	EQU	27H	; device in use
;	EQU	28H	; protected system device

; File control block
;	EQU	00H	; FCB+0 - type of FCB
;	EQU	01H	; FCB+1 - status flags
;	EQU	02H	; FCB+2 - reserved
;	EQU	03H	; FCB+3/4 - buffer address
;	EQU	05H	; FCB+5 - offset within current sector
;	EQU	06H	; FCB+6 - logical driver number
;	EQU	07H	; FCB+7 - directory entry for this file
;	EQU	08H	; FCB+8 - end-of-file byte offset
;	EQU	09H	; FCB+9 - logical record length
;	EQU	0AH	; FCB+10/11 - next record number
;	EQU	0CH	; FCB+12/13 - EOF record number
;	EQU	0EH	; FCB+14/15 - first extent
;	EQU	10H	; FCB+16-19 - granule allocation information
;	EQU	14H	; FCB+20-23 -    "    "
;	EQU	18H	; FCB+24-27 -    "    "
;	EQU	1CH	; FCB+28-31 -    "    "

; Keyboard mapping:
;	0  1  2  3  4  5  6  7
; 3801	@  A  B  C  D  E  F  G
; 3802	H  I  J  K  L  M  N  O
; 3804	P  Q  R  S  T  U  V  W
; 3808	X  Y  Z
; 3810	0  1  2  3  4  5  6  7
; 3820	8  9  :  ;  ,  -  .  /
; 3840	EN CL BR UP DN LF RT SP
; 3880 Model I is SHIFT xx xx xx xx xx xx xx
; 3880 Model III is LSH RSH xx xx xx xx xx xx
; 3880 Model IV row is LSH RSH CTRL CAPS F1 F2 F3 xx

	LIST	ON
