#include <stdlib.h>
#include "gen_defines.h"

extern void fire_render_asm( void );

u8 fire[ 64 * 49 ];
u8 firepal[ 256*4 ];

static void palgen( u24 i0, u8 i1, u8 r0, u8 g0, u8 b0, u8 r1, u8 g1, u8 b1 ) {
	u8  idif = i1 - i0;
	s24 rdif = r1 - r0;
	s24 gdif = g1 - g0;
	s24 bdif = b1 - b0;
	u8  n;
	u8 *fp = ( u8* )firepal;

	fp += i0 << 2;

	for( n = 0; n <= idif; n++ ) {
		*fp++ = r0 + ( rdif * n ) / idif;
		*fp++ = g0 + ( gdif * n ) / idif;
		*fp++ = b0 + ( bdif * n ) / idif;
		fp++;
	}
	
	
}

void fire_render( void ) {
	/*u24 sum;
	triad_t* rgb;
	u8* pal;
	u8 x,y;
	*/
	u8 n;

	for( n = 0; n < 64; n++ ) {
		fire[ 3072 + n ] = rand();
	}

	fire_render_asm();



	/*
	// Equivalent C code
	n= 0;
	for( y = 12; y < 48; y++ ) {
		for( x = 1; x < 63; x++ ) {
			if( y == 47 ) {
				sum = fire[ y * 64 + x + 64 ];
			} else {
				sum = fire[ y * 64 + x + 128 ];
			}
			sum += fire[ y * 64 + x + 64 ];
			sum += fire[ y * 64 + x + 63 ];
			sum += fire[ y * 64 + x + 65 ];
			if( sum <= 20 ) {
				sum = 0;
			} else {
				sum -= 20;
			}
			fire[ y *64 + x ] = sum >> 2;
			if( ++n == 3 ) n = 0;
		}
	}
	rgb = video_ram;
	pal = fire;
	while( pal < &fire[ 64 * 48 ] ) {
		//rgb->rgb = firepal[ *pal ];
		sum = ( *pal ) << 2;
		//sum <<= 2;
		sum += ( u24 )firepal;
		rgb->rgb = *( u24* )sum;
		rgb++;
		pal++;
	}
	*/
}

void fire_init( void ) {
	palgen( 0, 15, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00 );
	palgen( 16, 127, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00 );
	palgen( 128, 255, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF );
}
