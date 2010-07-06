#include "gen_defines.h"
// Classic text scroller

typedef struct {
  u24* ptr;
  u8   width;
} fchar_t;

static s24 text_pos = 64;
static u8  sine_count = 0;

extern u24 zuul[];
extern fchar_t zuul_desc[];

static const u8 text[] = {
	0x06, 0x0F, 0x12, 0x13, 0x0B, 0x0E, 0x09, 0x0E, 0x07, 0x13, 0x01, 0x16,
	0x04, 0x05, 0x0C, 0x0E, 0x09, 0x0E, 0x07, 0x05, 0x0E, 0x00, 0x10, 0x12,
	0x0F, 0x15, 0x04, 0x0C, 0x19, 0x00, 0x10, 0x12, 0x05, 0x13, 0x05, 0x0E,
	0x14, 0x13, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1F, 0x00, 0x08, 0x01, 0x03,
	0x0B, 0x0E, 0x09, 0x07, 0x08, 0x14, 0x00, 0x09, 0x09, 0x00, 0x20, 0x00,
	0x1E, 0x1E, 0x1E, 0x00, 0x24, 0x00, 0x04, 0x01, 0x19, 0x13, 0x00, 0x0F,
	0x06, 0x00, 0x0C, 0x0F, 0x16, 0x05, 0x00, 0x21, 0x00, 0x0E, 0x09, 0x07,
	0x08, 0x14, 0x13, 0x00, 0x0F, 0x06, 0x00, 0x08, 0x01, 0x03, 0x0B, 0x00,
	0x24, 
};

static const u8 sine[] = {
	0x08, 0x09, 0x09, 0x0A, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F,
	0x0F, 0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F, 0x0F, 0x0F, 0x0E, 0x0E,
	0x0D, 0x0D, 0x0C, 0x0C, 0x0B, 0x0A, 0x09, 0x09, 0x08, 0x07, 0x06, 0x05,
	0x05, 0x04, 0x03, 0x03, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04,
	0x05, 0x05, 0x06, 0x07
};

static void draw_char( s8 px, s8 py, u8 c ) {
	u8       w   = zuul_desc[ c ].width;
	u24     *rgb = zuul_desc[ c ].ptr;
	triad_t *out;
	u8	     x, y;
	s8       xp;

	for( y = 0; y != 16; y++ ) {
		out = &video_ram[ (py+y)*64+px ];
		xp = px;
		for( x = 0; x != w; x++ ) {
			if( xp >= 0 && xp < 64 ) {
				if( *( u8* )rgb != 0x01 ) {
					out->rgb = *rgb;
				}
			}
			xp++;
			rgb++;
			out++;
		}
	}
}

void scroller_render( void ) {
	s24 pos;
	u8  n, c;

	// Local start pos copy
	pos = text_pos--;
	// Wave effect counter
	sine_count++;
	for( n = 0; n < sizeof( text ); n++ ) {
		// Fetch character index
		c = text[ n ];
		// Handle space
		if( c-- == 0 ) {
			pos += 8;
		// Draw character
		} else {
			if( pos > -16 && pos < 64 ) {
				draw_char( pos, sine[ ( sine_count + ( n << 3 ) ) & 63 ], c );
			}
			pos += zuul_desc[ c ].width + 1;
		}
	}
	// Loop when end is reached
	if( pos <= 0 ) text_pos = 64;
}