.assume ADL=1

.public _hnc16		 ;UINT16 hnc16( UINT16 iData );
_hnc16
		ld iy,0
		add iy,sp
		ld h, (iy+3)
		ld l, (iy+4)
		ret

.public _hnc32		 ;UINT32 hnc32( UINT32 iData );
_hnc32
		ld iy,0
		add iy,sp
		ld hl,(iy+2)
		ld e,h
		ld h,(iy+5)
		ld l,(iy+6)
		ret

.public _csum32		 ;UINT16 csum32( UINT32* ipData );
_csum32
		ld iy,0
		add iy,sp
		ld iy,(iy+3)
		ld hl, 0
		ld de, 0
		ld h,(iy+0)
		ld l,(iy+1)
		ld d,(iy+2)
		ld e,(iy+3)
		adc.s hl,de
		ld de,0
		adc hl,de
		ret

.public _csummem	 ;UINT16 csummem( void* ipData, UINT24 iSize );
_csummem
		ld iy,0		 ;IY = SP
		add iy,sp

		ld hl,0		 ;HL = 0
		ld bc,0		 ;BC = 0
		ld de,0		 ;DE = 0
		ld c,(iy+7)  ;C  = MSB(iSize>>1)
		ld b,(iy+6)  ;B  = LSB(iSize>>1)
		srl c
		rr b

		ld a,b		 ;A  = B:LSB(tWords)
		or a		 ;F  = FLAGS(A:LSB(tWords))
		ld a,c		 ;A  = C:MSB(tWords)
		ld c,(iy+6)  ;C  = LSB(iSize)
		
		ld iy,(iy+3) ;IY = ipData
		
		jr nz,rptl	 ;if(A:LSB(tWords) == 0) {		do {
rpth	dec a		 ;	A:MSB(tWords)--
				 	 ;}

rptl			 	 ;do {
		ld d,(iy)	 ;  DE  = HTONL(*IY)
		inc iy		 ;  IY += 2;
		ld e,(iy)
		inc iy
		adc.s hl,de  ;  HL.S += DE.S + FLAGS.C

		djnz rptl	 ;} while(--B:LSB(SIZE));

		ld de,0		 ;HL += FLAGS.C
		adc hl,de
		or a,a		 ;F   = FLAGS(A:MSB(tWords))
		jr nz,rpth	 ;								} while(A:MSB(tWords))

		srl c        ;if(iSize & 1) {
		jr nc, done

		ccf			 ;  HL += HTONL(*IY)
		ld d,(iy)	 
		ld e,0		 
		adc.s hl,de  
		ld d,0		 ;  HL += FLAGS.C
		adc hl,de

done	ret			 ;}

.public _csum16add	;UINT16 csum16add( UINT16* ipCSum, UINT16 iValue );
_csum16add
		ld iy,0		 ;IY = SP
		add iy,sp
		ld e,(iy+6)
		ld d,(iy+7)
		ld iy,(iy+3)
		ld l,(iy)
		ld h,(iy+1)
		adc.s hl,de
		ld de,0
		adc hl,de
		ld (iy),l
		ld (iy+1),h
		ret
