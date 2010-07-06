#include <ez80f91.h>
#include "gen_defines.h"
#include "driver_sd.h"

#define SD_CMD_TIMEOUT  								   100
#define SD_IDLE_TIMEOUT 				  			   100
#define SD_READ_TIMEOUT									   100

// Application specific command mask
#define MSK_APP_CMD

// Commands
#define CMD0_GO_IDLE_STATE 							0x0000
#define CMD1_SEND_OP_COND 							0x0001
#define CMD8_SEND_IF_COND 							0x0008
#define CMD9_SEND_CSD     							0x0009
#define CMD10_SEND_CID		   						0x000A
#define CMD12_STOP_TRANSMISSION 				0x000C
#define CMD16_SET_BLOCKLEN 							0x0010
#define CMD17_READ_SINGLE_BLOCK 				0x0011
#define CMD18_READ_MULTIPLE_BLOCK				0x0012
#define CMD23_SET_BLOCK_COUNT						0x0017
#define CMD24_WRITE_BLOCK  							0x0018
#define CMD25_WRITE_MULTIPLE_BLOCK  		0x0019
#define CMD55_APP_CMD        						0x0037
#define CMD58_READ_OCR    							0x003A

// Idle command
#define CMD_IDLE         								0x00FF

// Application specific commands
#define ACMD23_SET_WR_BLOCK_ERASE_COUNT	0x0117
#define ACMD41_APP_SEND_OP_COND					0x0129

// Command response lengths
#define R1															     1
#define R1b															     0
#define R2															     2
#define R3															     5
#define R7															     5

// OCR response masks
#define MSK_OCR_33											  0xC0

// Token error mask
#define MSK_TOK_DATAERROR 							  0xE0

// Token error bits
#define MSK_TOK_ERROR										  0x01
#define MSK_TOK_CC_ERROR								  0x02
#define MSK_TOK_ECC_FAILED							  0x04
#define MSK_TOK_CC_OUTOFRANGE						  0x08
#define MSK_TOK_CC_LOCKED								  0x10

#define ACMD_INIT       								0x0141

u32 blocklen;

u8 block[ 512 ];

u8 sd_card_present( void ) {
	return( ( PB_DR & 0x02 ) == 0 );
}

static void sd_assert( void ) {
	PB_DR &= 0xEF;
}

static void sd_deassert( void ) {
	PB_DR |= 0x10;
}

void sd_init( void ) {
	// SPI to 400kHz
	SPI_BRG_H = 0x00;
	SPI_BRG_L = 0x7D;
	// Configure SPI
	SPI_CTL = 0x30;
}

static u8 spi_txrx( u8 byte ) {
	SPI_TSR = byte;
	while( !( SPI_SR & 0x80 ) );
	return( SPI_RBR );
}

static void sd_delay( u8 count ) {
	u8 i;
	for( i = 0; i != count; i++ ) {
		spi_txrx( CMD_IDLE );
	}
}

static u8 sd_send_command( u16 cmd, void *arg, void* resp ) {
	u8 i, tmp;
	// Prefix with an ACMD?
	if( cmd & 0x0100 ) {
		tmp = sd_send_command( CMD55_APP_CMD, NULL, NULL );
		if( tmp != SD_ERR_OK ) return( tmp );
	}
	// Select SD card
	sd_assert();
	// Send command
	spi_txrx( ( ( ( u8 )cmd ) & 0x3F ) | 0x40 );
	// Send arguments
	if( arg ) {
		spi_txrx( *( ( ( u8* )arg ) + 3 ) );
		spi_txrx( *( ( ( u8* )arg ) + 2 ) );
		spi_txrx( *( ( ( u8* )arg ) + 1 ) );
		spi_txrx( *( ( ( u8* )arg ) + 0 ) );
	} else {
		spi_txrx( 0 );
		spi_txrx( 0 );
		spi_txrx( 0 );
		spi_txrx( 0 );
	}
	// CRC, ignored for all commands except reset - where it is always 0x95
	spi_txrx( 0x95 );
	// Read first response byte with timeout
	i = 0;
	do {
		tmp = spi_txrx( CMD_IDLE );
		i++;
	} while( ( tmp & 0x80 ) != 0 && i < SD_CMD_TIMEOUT );
	// Bail if timed out
	if( ( tmp & 0x80 ) != 0 ) {
		sd_deassert();
		return( SD_ERR_TIMEOUT );
	}
	switch( cmd ) {
		case CMD12_STOP_TRANSMISSION:
			i = R1b;
			break;
		case CMD8_SEND_IF_COND:
			i = R7;
			break;
		case CMD58_READ_OCR:
			i = R3;
			break;
		case CMD9_SEND_CSD:
		case CMD10_SEND_CID:
			i = R1 + 16 + 2;
			break;
		default:
			i = R1;
	}
	// Read remaining response data
	while( i-- ) {
		if( resp ) ( ( u8* )resp )[ i ] = tmp;
		tmp = spi_txrx( CMD_IDLE );
	}
	// Deselect card
	sd_deassert();
	// All seems to be in order
	return( SD_ERR_OK );
}

static u8 sd_set_blocklen( u32 len ) {
	u8 resp;
	if( sd_send_command( CMD16_SET_BLOCKLEN, &len, &resp ) != SD_ERR_OK ) return( SD_ERR_GENERIC );
	if( resp ) return( SD_ERR_R1_MASK | resp );
	blocklen = len;
	return( SD_ERR_OK );
}

static u8 sd_readblock( u32 addr, u8 *data ) {
	u8 tmp, i, resp[ R2 ];
	u24 count;
	u16 crc;

	addr <<= 9;

	//sd_waitidle();

	if( sd_send_command( CMD17_READ_SINGLE_BLOCK, &addr, resp ) != SD_ERR_OK ) return( SD_ERR_GENERIC );

	if( resp[ 0 ] ) return( SD_ERR_READ );

	sd_assert();

	// Wait (with timeout) for data token
	i = 0;
	do {
		tmp = spi_txrx( CMD_IDLE );
	} while( ++i != SD_READ_TIMEOUT && tmp == 0xFF );

	// Bail on error
	if( !( tmp & MSK_TOK_DATAERROR ) ) {
		spi_txrx( CMD_IDLE );
		sd_deassert();
		return( SD_ERR_READ );
	}

	// Read data
	count = 512;
	while( count-- ) {
		*data++ = spi_txrx( CMD_IDLE );
	}

	// Read CRC
	high8( crc ) = spi_txrx( CMD_IDLE );
	low8( crc ) = spi_txrx( CMD_IDLE );

	tmp = spi_txrx( CMD_IDLE );

	sd_deassert();

	return( SD_ERR_OK );
}

static u8 sd_writeblock( u32 sector, u8 *data ) {
	return( SD_ERR_OK );
}

u8 sd_open( void ) {
	u8 i;
	u8 resp[ R1 + 16 + 2 ];
	u24 deleteme;
	// Check for card
	if( !sd_card_present() ) return( SD_ERR_NOCARD );
	// SPI to 400kHz
	SPI_BRG_H = 0x00;
	SPI_BRG_L = 0x7D;

	// Required delays before initialization
	sd_assert();
	sd_delay( 100 );
	sd_deassert();
	sd_delay( 2 );

	// Enter idle state
	if( sd_send_command( CMD0_GO_IDLE_STATE, NULL, NULL ) != SD_ERR_OK ) return( SD_ERR_GENERIC );

	// Read OCR (for operating conditions) and verify 3.3V capability
	if( sd_send_command( CMD58_READ_OCR, NULL, resp ) != SD_ERR_OK ) return( SD_ERR_GENERIC );
	if( !( resp[ 2 ] & MSK_OCR_33 ) ) return( SD_ERR_OCR );

	// Initialize card
	i = 0;
	do {
		if( sd_send_command( ACMD41_APP_SEND_OP_COND, NULL, resp ) != SD_ERR_OK ) return( SD_ERR_GENERIC );
	} while( ( resp[ 0 ] & SD_ERR_R1_IDLE ) && ++i != SD_IDLE_TIMEOUT );

	// Set block length to 512 (compatible with SDHC)
	if( sd_set_blocklen( 512 ) != SD_ERR_OK ) return( SD_ERR_GENERIC );

	// Read CSD
	//if( sd_send_command( CMD9_SEND_CSD, NULL, resp ) != SD_ERR_OK ) return( SD_ERR_GENERIC );
	/*
	for( i = 0; i < 255; i++ ) {
		sd_readblock( i, block );
		for( deleteme = 0; deleteme < 512; deleteme++ ) {
			if( block[ deleteme ] != 0 ) {
				asm( "NOP" );
				asm( "NOP" );
				asm( "NOP" );
				asm( "NOP" );
				asm( "NOP" );
				asm( "NOP" );
				asm( "NOP" );
				asm( "NOP" );
				break;
			}
		}
	}
	*/

	// All hail the initialized SD card!
	return( SD_ERR_OK );
}	

s24 sd_fatlib_read( u32 sector, u8 *buffer) {
	return( sd_readblock( sector, buffer ) == SD_ERR_OK );
}
s24 sd_fatlib_write( u32 sector, u8 *buffer) {
	return( 1 );
}