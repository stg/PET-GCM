	XREF _fire
	XDEF _fire_render

_fire_render

	PUSH IX
	LD DE, 0
	LD B, 12
	LD C, 1
	LD   IX, _fire

FIRE
	INC IX

	LD  HL, 0
	
	LD  E, (IX+62)
	ADD HL, DE 
	LD  E, (IX+64)
	ADD HL, DE 
	LD  E, (IX+63)
	ADD HL, DE 
	
	LD  A,B			; Don't copy from out of bounds
	XOR A,47
	JR  Z, SKIP
	LD  E, (IX+127)
SKIP
	ADD HL, DE

	SRL H			; Divide by 4
	RR  L
	SRL H
	RR  L

	LD  (IX-1), L

	INC C			; Loop X up to 63
	LD A, C
	XOR A, 63
	JR NZ, FIRE

	LD C, 1			; Reset X
	
	INC IX
	INC IX

	INC B			; Loop Y up to 47
	LD A, B
	XOR A, 48
	JR NZ, FIRE
	



	POP IX
	RET