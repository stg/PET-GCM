.ASSUME ADL=1

	XREF _fire
	XREF _firepal
	XREF _video_ram
	XDEF _fire_render_asm

_fire_render_asm

	PUSH IX
	LD   DE,     0
	LD   B,      12
	LD   C,      1
	LD   IX,     _fire + 64*12+1

FIRE
	INC  IX								; Inc IX first, so that displacement can reach +2 rows

	LD   HL,     0						; HL <= 0 (clear)
	
	LD   E,      (IX+62)				; HL <= HL + _fire(X-1, Y+1)
	ADD  HL,     DE 
	LD   E,      (IX+64)				; HL <= HL + _fire(X+1, Y+1)
	ADD  HL,     DE 
	LD   E,      (IX+63)				; HL <= HL + _fire(X  , Y+1)
	ADD  HL,     DE 
	
	LD   A,      B						; Check Y(B) against last row (47)
	XOR  A,      47
	JR   Z,      SKIP					; Skip readout of _fire(X, Y+2) if not available
	LD   E,      (IX+127)				; DE <= _fire(X  , Y+2)
SKIP
	ADD  HL,     DE						; HL <= HL + _fire(X  , Y+1 or 2)

	SRL  H								; HL <= HL/4 (shift right by 2)
	RR   L
	SRL  H
	RR   L								; L now contains Average

	LD   A,      L						; A <= Average-20 with clamp to 0
	SUB  A,      5
	JR   NC,     FLOOR
	XOR  A,      A

FLOOR
	LD   (IX-1), A						; Copy clamped value to _fire(X,Y)

	INC  C								; Check X(C) againt X'END - 1 (63)
	LD   A,      C
	XOR  A,      63
	JR   NZ,     FIRE					; Loop if not reached

	LD   C,      1						; IX <= 1 (reset loop)
	
	INC  IX								; IX <= IX + 2 (skip last and first column)
	INC  IX

	INC  B								; Check Y(B) against Y'END (48)
	LD   A,      B
	XOR  A,      48
	JR   NZ,     FIRE					; Loop if not reached

	LD   IX,     _fire					; IX <= _fire'START
	LD   IY,     _video_ram				; IY <= _video_ram'START
	LD   DE,     _firepal				; DE <= _firepal'START
	LD   BC,     _fire + ( 64 * 48 )	; BC <= _fire'END
RENDER

	LD   HL,     0						; HL <= Color index @ IX
	LD   L,      (IX)
	ADD  HL,     HL						; HL <= HL * 4
	ADD  HL,     HL
	ADD  HL,     DE						; HL <= HL + _firepal'START (DE)
	
	LD   HL,     (HL)
	LD   (IY),   HL						; Copy _firepal entry to _video_ram

	INC  IX								; Next index in _fire
	LEA  IY,     IY+4					; Next pixel in _video_ram

	LD   HL,     IX						; Check IX against _fire'END (BC)
	SBC  HL,     BC
	JR   NZ,     RENDER					; Loop if not reached

	POP  IX
	RET