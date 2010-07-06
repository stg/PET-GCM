// Not implemented:
//   8xx Panning extension (non-amiga)
//   E3x Glissando
//   E8x Panning extension (non-amiga)
//   EFx Funk-repeat

#include <string.h>
#include <stdlib.h>
#include "gen_defines.h"
#include "mod_player.h"

#define ntohs(w)  ( ( ( w & 0x00FF )<<8 ) | ( ( w>>8 ) & 0x00FF ) )
#define htons(w)  ( ( ( w & 0x00FF )<<8 ) | ( ( w>>8 ) & 0x00FF ) )

// Architecture dependant
#define hi4(b)     ( b >> 4 )
#define lo4(b)     ( b & 15 )

// Sample definition
typedef struct {
  s8* dma_pa;
  s8* dma_pb;
  s8* loop_p;
  s8  dma_mode;
  u8  dma_vol;
  u8  tuning;
  u24 len;
} sample_t;

// ProTracker cell
typedef struct {
  u8  ix_period;
  u8  sample;
  u8  effect;
  u8  param;
} cell_t;

typedef struct {
  u8  fxp;
  u8  offset;
  u8  mode;
} osc_t;

// Effect memory
typedef struct {
  u8    sample;
  u8    vol;
  u8    port_speed;
  u16   port_target;
  osc_t vibr;
  osc_t trem;
  u16   period;
  u8    loop_order;
  u8    loop_row;
  u8    loop_count;
  //u8    dummy_for_alignment;
} fxmem_t;

// Sample descriptors
static sample_t  sample[ 31 ];

// Pattern order
static u8        order_count;
static u8        order[ 128 ];

// Patterns
static cell_t    pattern[ MOD_MAX_PATTERNS ][ 64 ][ 4 ];

// Playback
static u8        loaded = 0;                    // Module is loaded
static u8        speed;                         // Current song speed
static u8        ix_order, ix_row;              // Position indexes
static u8        ix_nextorder, ix_nextrow;      // Force next order/row
static u8        tick;                          // Current tick
static u8        delay;                         // Row delay
fxmem_t          fxm[ 4 ];                      // Effect memory
static u8        mute[ 4 ];											// Channel mute

// ProTracker note periods for finetune values -8...7 (signed nibble)
static const u16 period_tbl[ 16 ][ 36 ] = {
  { 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453, 428, 404, 381, 360, 339, 320,
    302, 285, 269, 254, 240, 226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113 },
  { 850, 802, 757, 715, 674, 637, 601, 567, 535, 505, 477, 450, 425, 401, 379, 357, 337, 318, 
    300, 284, 268, 253, 239, 225, 213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113 },
  { 844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447, 422, 398, 376, 355, 335, 316,
    298, 282, 266, 251, 237, 224, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112 },
  { 838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444, 419, 395, 373, 352, 332, 314,
    296, 280, 264, 249, 235, 222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111 },
  { 832, 785, 741, 699, 660, 623, 588, 555, 524, 495, 467, 441, 416, 392, 370, 350, 330, 312,
    294, 278, 262, 247, 233, 220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110 },
  { 826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437, 413, 390, 368, 347, 328, 309,
    292, 276, 260, 245, 232, 219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109 },
  { 820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434, 410, 387, 365, 345, 325, 307,
    290, 274, 258, 244, 230, 217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109 },
  { 814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431, 407, 384, 363, 342, 323, 305,
    288, 272, 256, 242, 228, 216, 204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108 },
  { 907, 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453, 428, 404, 381, 360, 339,
    320, 302, 285, 269, 254, 240, 226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120 },
  { 900, 850, 802, 757, 715, 675, 636, 601, 567, 535, 505, 477, 450, 425, 401, 379, 357, 337,
    318, 300, 284, 268, 253, 238, 225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119 },
  { 894, 844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447, 422, 398, 376, 355, 335,
    316, 298, 282, 266, 251, 237, 223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118 },
  { 887, 838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444, 419, 395, 373, 352, 332,
    314, 296, 280, 264, 249, 235, 222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118 },
  { 881, 832, 785, 741, 699, 660, 623, 588, 555, 524, 494, 467, 441, 416, 392, 370, 350, 330,
    312, 294, 278, 262, 247, 233, 220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117 },
  { 875, 826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437, 413, 390, 368, 347, 328,
    309, 292, 276, 260, 245, 232, 219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116 },
  { 868, 820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434, 410, 387, 365, 345, 325,
    307, 290, 274, 258, 244, 230, 217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115 },
  { 862, 814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431, 407, 384, 363, 342, 323,
    305, 288, 272, 256, 242, 228, 216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114 }
};

// Min/max
static u16 min( s16 a, s16 b ) {
  return( a < b ? a : b );
}
static u16 max( s16 a, s16 b ) {
  return( a > b ? a : b );
}

// Sine-table for vibrato and tremolo
static s8  sine[ 64 ] = {
  0x00, 0x0C, 0x18, 0x25, 0x30, 0x3C, 0x47, 0x51, 0x5A, 0x62, 0x6A, 0x70, 0x76, 0x7A, 0x7D, 0x7F,
  0x7F, 0x7F, 0x7D, 0x7A, 0x76, 0x70, 0x6A, 0x62, 0x5A, 0x51, 0x47, 0x3C, 0x30, 0x25, 0x18, 0x0C,
  0x00, 0xF3, 0xE7, 0xDA, 0xCF, 0xC3, 0xB8, 0xAE, 0xA5, 0x9D, 0x95, 0x8F, 0x89, 0x85, 0x82, 0x80,
  0x80, 0x80, 0x82, 0x85, 0x89, 0x8F, 0x95, 0x9D, 0xA5, 0xAE, 0xB8, 0xC3, 0xCF, 0xDA, 0xE7, 0xF3
};

// Look up or generate waveform for ProTracker vibrato/tremolo
static s8 do_osc( osc_t *p_osc ) { //u8 waveform, u8 index, u8 amplitude ) {
  s8  sample = 0;
  s16 mul;

  switch( p_osc->mode & 0x03 ) {
    case 0: sample = sine[ p_osc->offset & 0x3F ];          // Sine
    case 1: sample = ( p_osc->offset & 0x20 ) ? 127 : -127; // Square
    case 2: sample = ( p_osc->offset << 2 ) ^ 0xFF;         // Saw
    case 3: sample = rand();                                // Noise
  }
  mul = sample * lo4( p_osc->fxp );
  // TODO: does this preserve sign?
  return( mul / 128 );
  // Next offset
  p_osc->offset += hi4( p_osc->fxp );
}

// Calculate arpeggio period
static u16 arpeggio( u8 ch, u8 halftones ) {
  u8 n, tuning = sample[ fxm[ ch ].sample ].tuning;
  // Find base note
  for( n = 0; n != 35; n++ ) {
    if( fxm[ ch ].period >= period_tbl[ tuning ][ n ] ) break;
  }
  // Clamp and return arpeggio period
  return( period_tbl[ tuning ][ n + halftones > 35 ? 35 : n + halftones ] );
}

// Start note
void note_start( dma_ch_t *p_dma, u8 ix_sample, u16 period, u8 offset ) {
  // Set address points
  p_dma->pa        = sample[ ix_sample ].dma_pa + ( offset << 8 );
  p_dma->pb        = sample[ ix_sample ].dma_pb;
  // Set reload register (defines rate)
  p_dma->rr        = period_tbl[ sample[ ix_sample ].tuning ][ period ];
  // Set mode (begin playback)
  p_dma->mode      = sample[ ix_sample ].dma_mode;
  // Set loop-point
  p_dma->pa        = sample[ ix_sample ].loop_p;
}

// Play module
void play_module( void ) {
  u8           ch, fx, fxp;
  u8 register  temp;
  cell_t      *p_ptn;
  fxmem_t     *p_fxm;
  dma_ch_t    *p_dma;

  // Abort if no module is loaded
  if( !loaded ) return;

  // Advance tick
  if( ++tick == speed ) tick = 0;

  // Handle row delay
  if( delay ) {
    if( tick == 0 ) delay--;
    return;
  }

 // Advance playback
  if( tick == 0 ) {
    if( ++ix_row == 64 ) {
      ix_row = 0;
      if( ++ix_order == order_count ) ix_order = 0;
    }

	  // Forced order/row
	  if( ix_nextorder != 0xFF ) {
	    ix_order = ix_nextorder;
	    ix_nextorder = 0xFF;
	  }
	  if( ix_nextrow != 0xFF ) {
	    ix_row = ix_nextrow;
	    ix_nextrow = 0xFF;
	  }

  }

  // Pointer setup
  p_ptn = &pattern[ order[ ix_order ] ][ ix_row ][ 0 ];
  p_fxm = &fxm[ 0 ];
  p_dma = &dma[ 0 ];

  for( ch = 0; ch != 4; ch++ ) {

    // Quick access to effect and parameters speeds up code
    fx  = p_ptn->effect;
    fxp = p_ptn->param;

    if( tick == 0 ) {
  
      // Set tuning needs processing before anything else
      if( fx == 0xE5 ) {
        sample[ p_fxm->sample ].tuning = fxp;
      }
      
      // If sample is available
      if( p_ptn->sample ) {
        // Sample "latch"
        temp = p_ptn->sample - 1;
        p_fxm->sample = temp;
        p_fxm->vol    = sample[ temp ].dma_vol;
        // Reset volume (unless delayed note)
        if( fx != 0xED || fxp== 0x00 ) {
          if( !mute[ ch ] ) p_dma->vol  = sample[ temp ].dma_vol << 1;
        }
        // Re-trigger oscillator offsets
        if( ( p_fxm->vibr.mode & 0x4 ) == 0x0 ) p_fxm->vibr.offset = 0;
        if( ( p_fxm->trem.mode & 0x4 ) == 0x0 ) p_fxm->trem.offset = 0;
      }
  
      // If note is available
      if( p_ptn->ix_period != 0xFF ) {
        if( fx == 0x30 || fx == 0x50 ) {
          // Tone-portamento effect setup
          p_fxm->port_target = period_tbl[ sample[ p_ptn->sample ].tuning ][ p_ptn->ix_period ];
        } else {
          // Start note (unless delayed note)
          temp = p_fxm->sample;
          if( fx != 0xED || fxp== 0x00 ) {
            note_start( p_dma, temp, p_ptn->ix_period, ( fx == 0x90 ? fxp : 0 ) );
          }
          // Set required effect memory parameters
          p_fxm->period = period_tbl[ sample[ temp ].tuning ][ p_ptn->ix_period ];
        }
      }

	    // Effects processed when tick = 0
      switch( fx ) {
        // Portamento
        case 0x30:
          if( fxp ) p_fxm->port_speed = fxp;
          break;
        // Jump to pattern
        case 0xB0:
          ix_nextorder = ( fxp >= order_count ? 0x00 : fxp );
          ix_nextrow = 0;
          break;
        // Set volume
        case 0xC0:
          p_fxm->vol = ( fxp & 0x40 ? 0x40 : fxp );
          if( !mute[ ch ] ) p_dma->vol = p_fxm->vol << 1;
          break;
        // Jump to row
        case 0xD0:
          fxp = hi4( fxp ) * 10 + lo4( fxp );
          ix_nextorder = ( ix_order + 1 >= order_count ? 0x00 : ix_order + 1 );
          ix_nextrow = ( fxp > 63 ? 0 : fxp );
          break;
        // Set speed
        case 0xF0:
          speed = fxp;
          break;
        // Vibrato
        case 0x40:
          if( fxp ) p_fxm->vibr.fxp = fxp;
          break;
        // Fine slide up
        case 0xE1:
          p_fxm->period -= fxp;
          p_dma->rr = p_fxm->period;
          break;
        // Fine slide down
        case 0xE2:
          p_fxm->period += fxp;
          p_dma->rr = p_fxm->period;
          break;
        // Set vibrato waveform
        case 0xE4:
          p_fxm->vibr.mode = fxp;
          break;
        // Loop-back
        case 0xE6:
          if( fxp == 0x0 ) {
            p_fxm->loop_order = ix_order;
            p_fxm->loop_row   = ix_row;
          } else {
            p_fxm->loop_count = ( p_fxm->loop_count ? p_fxm->loop_count - 1 : fxp );
            if( p_fxm->loop_count ) {
              ix_nextorder = p_fxm->loop_order;
              ix_nextrow   = p_fxm->loop_row;
            }
          }
          break;
        // Set tremolo waveform
        case 0xE7:
          p_fxm->trem.mode = fxp;
          break;
        // Delay
        case 0xEE:
          delay = fxp;
          break;
      }

    // Effects processed when tick > 0
    } else {
      switch( fx ) {
        // Slide up
        case 0x10:
          p_fxm->period -= fxp;
          p_dma->rr = p_fxm->period;
          break;
        // Slide down
        case 0x20:
          p_fxm->period += fxp;
          p_dma->rr = p_fxm->period;
          break;
        // Retrigger note
        case 0xE9:
          // Modulo fxp
          temp = tick; while( temp >= fxp ) temp -= fxp;
          if( temp == 0 ) {
            // Restart note
            note_start( p_dma, p_fxm->sample, p_ptn->ix_period, ( fx == 0x90 ? fxp : 0 ) );
          }
          break;
        // Note cut
        case 0xEC:
          if( fxp == tick ) {
            if( !mute[ ch ] ) p_dma->vol = 0x00;
          }
          break;
        // Delayed note
        case 0xED:
          if( fxp == tick ) {
            // Set volume
            if( p_ptn->sample ) {
              if( !mute[ ch ] ) p_dma->vol  = sample[ p_fxm->sample ].dma_vol << 1;
            }
            // Start note
            if( p_ptn->ix_period != 0xFF ) {
              note_start( p_dma, p_fxm->sample, p_ptn->ix_period, ( fx == 0x90 ? fxp : 0 ) );
            }
          }
          break;
        // Processing that has multiple effects
        default:
          // Portamento
          if( fx == 0x30 || fx == 0x50 ) {
              // Slide down/up
              if( p_fxm->period < p_fxm->port_target ) {
                p_fxm->period = min( p_fxm->period + p_fxm->port_speed,  p_fxm->port_target );
              } else {
                p_fxm->period = max( p_fxm->period - p_fxm->port_speed,  p_fxm->port_target );
              }
              // Set new period
              p_dma->rr = p_fxm->period;
          }
          // Volume slide
          if( fx == 0x50 || fx == 0x60 || fx == 0xA0 ) {
              // Slide down/up
              if( ( fxp & 0xF0 ) == 0 ) p_fxm->vol -= ( lo4( fxp ) );
              if( ( fxp & 0x0F ) == 0 ) p_fxm->vol += ( hi4( fxp ) );
              // Clamp
              if( p_fxm->vol > 0x40 ) p_fxm->vol = ( p_fxm->vol > 0x4F ? 0x00 : 0x40 );
              // Set volume
              if( !mute[ ch ] ) p_dma->vol = p_fxm->vol << 1;
          }
      }
    }
    
    // Normal play/arpeggio processing
    if( fx == 0x00 ) {
      // Modulo 3
      temp = tick; while( temp > 2 ) temp -= 2;
      if( temp == 0 ) {
        // Reset
        p_dma->rr = p_fxm->period;
      } else if( fxp ) {
        // Arpeggio
        p_dma->rr = arpeggio( ch, ( temp == 1 ? hi4( fxp ) : lo4( fxp ) ) );
      }
    // Vibrato processing
    } else if( fx == 0x40 || fx == 0x60 ) {
      p_dma->rr = p_fxm->period + do_osc( &p_fxm->vibr );
    // Tremolo processing
    } else if( fx == 0x70 ) {
      temp = p_fxm->vol + do_osc( &p_fxm->trem );
      // Clamp
      if( !mute[ ch ] ) p_dma->vol = ( temp > 0x40 ? ( temp > 0x4F ? 0x00 : 0x40 ) : temp ) << 1;
    }

		if( mute[ ch ] ) p_dma->vol = 0x00;

    // Pointer advance
    p_ptn++;
    p_fxm++;
    p_dma++;
  }

}

// Load a module into memory
void load_module( fp_read read_data ) {
  u8        n, z, row, ch;
  s8       *p_audio_ram;
  sample_t *p_sample;
  u24       loop_offset, loop_len;
  u16       temp_w;
  u8        temp_b;
  cell_t   *p_ptn;
  u8        patterns;

  // Reset
  loaded       = 0;
  patterns     = 0;
  speed        = 6;
  tick         = 5;
  ix_nextrow   = 0;
  ix_nextorder = 0;
  delay        = 0;
  for( n = 0; n != 4; n++ ) {
    fxm[ n ].vibr.mode = 0x00;
    fxm[ n ].trem.mode = 0x00;
  }

  // Skip song length
  read_data( NULL, 20 );
  
  // Read 31 samples
  p_sample = sample;
  p_audio_ram = audio_ram;
  for( n = 0; n != 31; n++ ) {
    
    // Skip sample name
    read_data( NULL, 22 );
    
    // Set sample starting address
    p_sample->dma_pa = p_audio_ram;
    
    // Get length
    read_data( &temp_w, 2 );
    p_sample->len = ntohs( temp_w ) << 1;
    
    // Get finetune value
    read_data( &p_sample->tuning, 1 );
    
    // Get volume
    read_data( &p_sample->dma_vol, 1 );
    
    // Get loop start
    read_data( &temp_w, 2 );
    loop_offset = ntohs( temp_w ) << 1;
    
    // Get loop length
    read_data( &temp_w, 2 );
    loop_len = ntohs( temp_w ) << 1;
    
    // Calculate DMA settings
    if( loop_len < 3 ) {

      // Sample is not looped
      p_sample->dma_pb    = p_audio_ram + p_sample->len - 1;
      p_sample->loop_p    = p_sample->dma_pa;
      p_sample->dma_mode  = 0x02;
      p_audio_ram       += p_sample->len;

    } else {

      // Sample is looped
      p_sample->dma_pb    = p_audio_ram + loop_offset + loop_len - 1;
      p_sample->loop_p    = p_sample->dma_pa + loop_offset;
      p_sample->dma_mode  = 0x0A;
      p_audio_ram       += p_sample->len;
      p_audio_ram       += p_sample->dma_pb - p_sample->loop_p + 1;

    }

    // Next sample and address
    p_sample++;
  }

  // Read order order count
  read_data( &order_count, 1 );
  // Skip repeat/tracker id
  read_data( NULL, 1 );

  // Read order order, find pattern count
  for( n = 0; n < 128; n++ ) {
    read_data( &order[ n ], 1 );
    if( order[ n ] > patterns ) patterns = order[ n ];
  }
  patterns++;

  // Check constraints
  if( patterns > MOD_MAX_PATTERNS ) return;

  // Skip tracker ID
  read_data( NULL, 4 );

  // Deconstruct pattern data, cells are SPPPSEXX where:
  //   SS = sample, PPP = period, E = effect, XX = parameters
  for( n = 0; n != patterns; n++ ) {
    for( row = 0; row != 64; row++ ) {
      for( ch = 0; ch != 4; ch++ ) {
        p_ptn            = &pattern[ n ][ row ][ ch ];
        
        // Deconstruct sample.msb and period.msb
        read_data( &temp_b, 1 );
        
        temp_w           = ( temp_b & 0x0F ) << 8;
        p_ptn->sample    = temp_b & 0xF0;

        // Deconstruct period.lsb
        read_data( &temp_b, 1 );

        temp_w          |= temp_b;

        // Deconstruct sample.lsb and effect
        read_data( &temp_b, 1 );

        p_ptn->sample   |= hi4( temp_b );
        p_ptn->effect    = lo4( temp_b ) << 4;

        // Deconstruct parameters
        read_data( &p_ptn->param, 1 );

        // Deconstruct extended parameters
        if( p_ptn->effect == 0xE0 ) {
          p_ptn->effect |= hi4( p_ptn->param ) >> 4;
          p_ptn->param  &= 0x0F;
        }

        // Find note index from period
        p_ptn->ix_period = 0xFF;
        if( temp_w ) {
          for( z = 0; z != 36; z++ ) {
            if( period_tbl[ 0 ][ z ] == temp_w ) {
              
              // Note index found
              p_ptn->ix_period = z;
              break;

            }
          }
          if( z == 36 ) {
            
            // This is not a ProTracker module
            return;

          }
        }
      }
    }
  }

  // Copy sample data to sample memory
  p_sample = sample;
  p_audio_ram = audio_ram;
  for( n = 0; n != 31; n++ ) {
    
    // Copy sample data
    read_data( p_audio_ram, p_sample->len );
    p_audio_ram += p_sample->len;
    
    // Unroll sample loop (allows DMA to handle looping samples without CPU intervention)
    if( p_sample->dma_mode == 0x0A ) {
      
      // Copy looped range
      memcpy( p_audio_ram, p_sample->loop_p, p_sample->dma_pb - p_sample->loop_p + 1 );
			if( memcmp( p_audio_ram, p_sample->loop_p, p_sample->dma_pb - p_sample->loop_p + 1 ) != 0 )  {
				asm("NOP");
				asm("NOP");
				asm("NOP");
				asm("NOP");
				asm("NOP");
				asm("NOP");
				asm("NOP");
				asm("NOP");

			}
      
      // Modify pointers
      p_audio_ram += p_sample->dma_pb - p_sample->loop_p;
      p_sample->loop_p = p_sample->dma_pa + p_sample->len;
			p_sample->dma_pa--;
      p_sample->dma_pb = p_audio_ram;
      p_audio_ram++;

    }
    
    p_sample++;
  }

  // Initialize
  loaded = 1;
}