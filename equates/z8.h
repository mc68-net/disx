; Z8 standard built-in register names

SIO	EQU	0F0H	; Serial I/O Register
TMR	EQU	0F1H	; Timer Mode Register
T1	EQU	0F2H	; Counter/Timer 1 Register
PRE1	EQU	0F3H	; Prescaler 1 Register (write only)
T0	EQU	0F4H	; Counter/Timer 0 Register
PRE0	EQU	0F5H	; Prescaler 0 Register (write only)
P2M	EQU	0F6H	; Port 2 Mode Register (write only)
P3M	EQU	0F7H	; Port 3 Mode Register (write only)
P01M	EQU	0F8H	; Ports 0 and 1 Mode Registers (write only)
IPR	EQU	0F9H	; Interrupt Priority Register (write only)
IRQ	EQU	0FAH	; Interrupt Request Register
IMR	EQU	0FBH	; Interrupt Mask Register
FLAGS	EQU	0FCH	; Flag Register
RP	EQU	0FDH	; Register Pointer
SPH	EQU	0FEH	; Stack Pointer High
SPL	EQU	0FFH	; Stack Pointer Low
