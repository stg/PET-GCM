#include "gen_defines.h"
#include "mod_player.h"
#include "fire.h"
#include "scroller.h"

// GENERAL MEMORY LAYOUT
//
// Range               Size Device         CS  Speed Use
// 0x000000 - 0x03FFFF 256K Internal Flash --  4 ws  Bootloader
// 0x100000 - 0x1FFFFF   1M External Flash CS0 5 ws  Savegames
// 0x800000 - 0x8FFFFF   1M FPGA           CS2 5 ws  Hardware
// 0x900000 - 0x97FFFF 512K External SRAM  CS1 1 ws  User
// 0x980000 - 0x98BFFF  48K None           --  ----- ---
// 0x98C000 - 0x98DFFF   8K EMAC RAM       --  1 ws  EMAC
// 0x98E000 - 0x98FFFF   8K Internal RAM   --  0 ws  Stack

// FPGA MEMORY LAYOUT
//
// Range               Size Function
// 0xX00000 - 0xX3FFFF 256K Video memory
// 0xX40000 - 0xX5FFFF 128K Input devices
// 0xX60000 - 0xX7FFFF 128K Audio registers
// 0xX80000 . 0xXFFFFF 512K Audio memory

// VIDEO MEMORY LAYOUT
//
//         Col  0 ... Col 63
// Row  0 0xXX0000   0xXX00FC
//  ...
// Row 47 0xXX2F00   0xXX2FFC
//
// 0xXXXXX0 Red channel   6bits (2msb = Don't care, read as 0)
// 0xXXXXX1 Green channel 6bits (2msb = Don't care, read as 0)
// 0xXXXXX2 Blue channel  6bits (2msb = Don't care, read as 0)

// INPUT DEVICES LAYOUT
//
// 0xXXXXX0 1byte Port A rotary counter
// 0xXXXXX1 1byte Port A buttons (4msb read as 0)
// 0xXXXXX2 1byte Port B rotary counter
// 0xXXXXX3 1byte Port B buttons (4msb read as 0)

// AUDIO REGISTERS LAYOUT
//
// 0xXXXX0X 16byte DMA channel 0
//   ....
// 0xXXXX7X 16byte DMA channel 7
//
// 0xXXXXX1 3byte  Address point A
// 0xXXXXX5 3byte  Address point B
// 0xXXXXX8 1byte  IRQ register
// 0xXXXXX9 3byte  Address counter
// 0xXXXXXC 2byte  Rate register
// 0xXXXXXE 1byte  Volume
// 0xXXXXXF 1byte  Volume

u8 page = 0;
u8 need_render = 1;

u24 logo[] = {
	0x000, 0x001, 0x002, 0x004, 0x005, 0x006, 0x008, 0x009, 0x00A, 0x040,
	0x045, 0x048, 0x080, 0x081, 0x082, 0x085, 0x088, 0x08A, 0x0C2, 0x0C5,
	0x0C8, 0x0CA, 0x100, 0x101, 0x102, 0x105, 0x108, 0x109, 0x10A
};

#pragma interrupt
void video_isr( void ) {
	u8* gp = ( u8* )video_ram;
	u24 n;

	if( !need_render ) {

		page ^= 1;
  	PB_DR = page;

		need_render = 1;

	}

}

#pragma interrupt
void audio_isr( void ) {
  u8 volatile irq;
  // Read interrupt state (clear timer interrupt)
  irq = dma[ 0 ].irq;
  if( irq == 0x00 ) {
    // Groove...
    play_module();
    // Timer interrupt
    irq = dma[ 0 ].irq;
  }
  if( irq ) {
    // One or more DMA interrupts, irq contains bitmask
  }
}

void main( void ) {
  u8  t;
	u16 p;
	u24 n;

  int_disable;

	// Stop audio
	for( n = 0; n < 8; n++ ) dma[ n ].mode = 0x00;

	// Configure ports
  PA_DR   = 0x03;
  PA_ALT2 = 0x03;
  PA_ALT1 = 0x03;
  PA_DDR  = 0x00;
  PA_ALT0 = 0x03;

	PB_DR   = 0x00;
	PB_ALT2 = 0x00;
	PB_ALT1 = 0x00;
	PB_DDR  = 0x00;
	PB_ALT0 = 0x01;

	// Register interrupts
  _set_vector( PA0_IVECT, video_isr );
  _set_vector( PA1_IVECT, audio_isr );

	// Initialize fire effect
	fire_init();

	// Load ProTracker MOD
  //mod_data_reset();
  //load_module( mod_data_read );

  int_enable;

	while( 1 ) {
		if( need_render ) {
			// Render fire
			fire_render();
			// Render scroller
			scroller_render();
			// Render logo
			for( n = 0; n < 29; n++ ) {
				video_ram[ 0xAB3 	+ logo[ n ] ].rgb = 0;
			}
			// Trigger page-flip
			need_render = 0;
		}
	}

}

