#include "gen_defines.h"

// Driver error codes
#define SD_ERR_OK       	0x00	// Operation completed successfully
#define SD_ERR_NOCARD   	0x01	// No card is inserted
#define SD_ERR_TIMEOUT  	0x02	// An operation has timed out
#define SD_ERR_GENERIC  	0x03	// Generic error
#define SD_ERR_OCR		  	0x04	// Operating conditions (card does not support 3.3V)
#define SD_ERR_READ      	0x05	// Error while reading block
#define SD_ERR_WRITE      0x06	// Error while writing block
#define SD_ERR_R1_MASK		0x80  // Bitmask: bits 6-0 contains R1 error field

// R1 response masks
#define SD_ERR_R1_IDLE 										  0x01
#define SD_ERR_R1_ERASE_RST 								  0x02
#define SD_ERR_R1_ILL_CMD 									  0x04
#define SD_ERR_R1_CRC_ERR 									  0x08
#define SD_ERR_R1_ERASE_SEQ_ERR 						  0x10
#define SD_ERR_R1_ADDR_ERR 								  0x20
#define SD_ERR_R1_PARAM_ERR 								  0x40



void sd_init( void );
u8 sd_open( void );


// media access functions for the fat library from robs-projects.com
s24 sd_fatlib_read( u32 sector, u8 *buffer);
s24 sd_fatlib_write( u32 sector, u8 *buffer);