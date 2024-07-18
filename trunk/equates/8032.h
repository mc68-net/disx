; equates for standard 8032 registers

P0	EQU	80H	; I/O port 0

SP	EQU	81H	; stack pointer
DPL	EQU	82H	; DPTR low
DPH	EQU	83H	; DPTR high
PCON	EQU	87H	; power control

TCON	EQU	88H	; timer 0/1 control
TCON_IT0 EQU	TCON.0	; (bit) 
TCON_IE0 EQU	TCON.1	; (bit) 
TCON_IT1 EQU	TCON.2	; (bit) 
TCON_IE1 EQU	TCON.3	; (bit) 
TCON_TR0 EQU	TCON.4	; (bit) 
TCON_TF0 EQU	TCON.5	; (bit) 
TCON_TR1 EQU	TCON.6	; (bit) 
TCON_TF1 EQU	TCON.7	; (bit) 

TMOD	EQU	89H	; timer mode
TL0	EQU	8AH	; timer low 0
TL1	EQU	8BH	; timer low 1
TH0	EQU	8CH	; timer high 0
TH1	EQU	8DH	; timer high 1

P1	EQU	90H	; I/O port 1
T2	EQU	90H	; (bit) 
T2EX	EQU	91H	; (bit) 

SCON	EQU	98H	; serial controller
SCON_RI	EQU	SCON.0	; (bit) 
SCON_TI	EQU	SCON.1	; (bit) 
SCON_RB8 EQU	SCON.2	; (bit) 
SCON_TB8 EQU	SCON.3	; (bit) 
SCON_REN EQU	SCON.4	; (bit) 
SCON_SM2 EQU	SCON.5	; (bit) 
SCON_SM1 EQU	SCON.6	; (bit) 
SCON_SM0 EQU	SCON.7	; (bit) 

SBUF	EQU	99H	; serial data buffer

P2	EQU	A0H	; I/O port 2

IE	EQU	0A8H	; interrupt enable
EX0	EQU	IE.0	; (bit) 
ET0	EQU	IE.1	; (bit) 
EX1	EQU	IE.2	; (bit) 
ET1	EQU	IE.3	; (bit) 
ES	EQU	IE.4	; (bit) 
ET2	EQU	IE.5	; (bit) 
;	EQU	IE.6	; (bit) undefined
EA	EQU	IE.7	; (bit) 

P3	EQU	B0H	; I/O port 3 (8xx2 only)

IP	EQU	0B8H	; interrupt priority
PX0	EQU	IP.0	; (bit) 
PT0	EQU	IP.1	; (bit) 
PX1	EQU	IP.2	; (bit) 
PT1	EQU	IP.3	; (bit) 
PS	EQU	IP.4	; (bit) 
PT2	EQU	IP.5	; (bit) 
;	EQU	IP.6	; (bit) undefined
;	EQU	IP.7	; (bit) undefined

T2CON	EQU	0C8H	; timer 2 control (8xx2 only)
T2CON_CP    EQU	T2CON.0	; (bit) CP / !RL2
T2CON_C     EQU	T2CON.1	; (bit) C / !T2
T2CON_TR2   EQU	T2CON.2	; (bit) 
T2CON_EXEN2 EQU	T2CON.3	; (bit) 
T2CON_TCLK  EQU	T2CON.4	; (bit) 
T2CON_RCLK  EQU	T2CON.5	; (bit) 
T2CON_EXF2  EQU	T2CON.6	; (bit) 
T2CON_TF2   EQU	T2CON.7	; (bit) 

RCAPL	EQU	0CAH	; capture low (8xx2 only)
RCAP2H	EQU	0CBH	; capture high (8xx2 only)
TL2	EQU	0CCH	; timer low 2 (8xx2 only)
TH2	EQU	0CDH	; timer high 2 (8xx2 only)

PSW	EQU	0D0H	; program status word
P_FLAG	EQU	PSW.0	; (bit) parity flag (?)
;	EQU	PSW.1	; (bit) undefined
OV_FLAG	EQU	PSW.2	; (bit) overflow flag
RS0	EQU	PSW.3	; (bit) register bank select bit 0
RS1	EQU	PSW.4	; (bit) register bank select bit 1
F0_FLAG	EQU	PSW.5	; (bit) ?
AC_FLAG	EQU	PSW.6	; (bit) auxiliary carry (?)
CY_FLAG	EQU	PSW.7	; (bit) carry flag

ACC	EQU	E0H	; Accumulator (bits E0-E7)
B_REG	EQU	F0H	; B register (bits F0-F7)
