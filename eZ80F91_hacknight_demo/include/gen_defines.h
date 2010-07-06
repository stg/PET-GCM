#include <ez80.h>
#include <stdio.h>
#include <gpio.h>

#ifndef _GEN_DEFINES_H
#define _GEN_DEFINES_H

// type definitions
typedef unsigned short u16;
typedef unsigned char  u8;
typedef unsigned int   u24;
typedef unsigned long  u32;
typedef signed   short s16;
typedef signed   char  s8;
typedef signed   int   s24;
typedef signed   long  s32;

// struct definitions
typedef struct {
	u8           pa_dummy;
  s8          *pa;
	u8           pb_dummy;
	s8          *pb;
	u8  volatile irq;
	u24 volatile addr;
	u16          rr;
	u8  volatile vol;
	u8  volatile mode;
} dma_ch_t;

typedef struct {
	u24 rgb;
	u8  dummy;
} triad_t;

typedef struct {
  u8  pos;
	u8  buttons;
} input_t;

// char access definitions
#define offset8( x, y ) ( *( ( ( u8* )&( x ) ) ) + y )
#define high8( x )      ( *( ( ( u8* )&( x ) ) + sizeof( x ) - 1 ) )
#define low8( x )       ( *( ( ( u8* )&( x ) ) ) )
#define mid8( x )       ( *( ( ( u8* )&( x ) ) + 1 ) )

// short access definitions
#define low16( x )      ( *( ( u16* )( ( ( u8* )&( x ) ) ) ) )
#define high16( x )     ( *( ( u16* )( ( ( u8* )&( x ) ) + sizeof( x ) - 2 ) ) )

// interrupts
#define int_disable asm("DI")
#define int_enable  asm("EI")

// external hardware
extern dma_ch_t dma[ 8 ];
extern s8       audio_ram[ 512 * 1024 ];
extern input_t  input[ 2 ];
extern triad_t  video_ram[ 64 * 48 ];

#endif