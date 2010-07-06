#include <string.h>
#include "gen_defines.h"
#include "eth_ip.h"
#include "driver_flash.h"

#define PETPRG_CMD_STATUS  0x00
#define PETPRG_CMD_RESET   0x10
#define PETPRG_CMD_RUN     0x20
#define PETPRG_CMD_FORMAT  0xF0
#define PETPRG_CMD_ERASE   0xE0
#define PETPRG_CMD_DATA    0xD0

u8   tLastIndex = 0;
u8   tState = FLASH_STATE_READY;
u24  tAddr;
u8   tData[ 1024 ];
u8  *tpData;
u24  tSize;
u8   tRun;

// respond with last received index and status
void respond( void ) {
	u8 tpResponse[ 2 ];
	tpResponse[ 0 ] = tLastIndex;
	tpResponse[ 1 ] = tState;
	udp_send( tpResponse, 2 );
}

// parse petprg packet
void petprg_parse( u8 *data, u24 len ) {
	u8  tIndex;
	u8  tpResponse[ 2 ];
	u24 tTemp;
	if( len < 2 ) return;
	tIndex = ( *data++ ) - 1;
	if( tIndex == tLastIndex ) {
		switch( *data++ ) {
			case PETPRG_CMD_RUN:
				tRun = 1;
				break;
			case PETPRG_CMD_RESET:
				flash_reset(); // reset
				tState = flash_state();
				break;
			case PETPRG_CMD_FORMAT:
				flash_format(); // format
				tState = flash_state();
				break;
			case PETPRG_CMD_ERASE:
				if( len < 3 || tSize != 0 ) return;
				tAddr = flash_sectorAddr( *data++ ); // address
				flash_erase( tAddr );	// erase
				tState = flash_state();
				break;
			case PETPRG_CMD_DATA:
				if( len < 6 || len > 1029 || tSize != 0 ) return;
				high8( tAddr ) = *data++; // address
				mid8 ( tAddr ) = *data++;
				low8 ( tAddr ) = *data++;
				tSize = len - 5;	// bytes left
				memcpy( tData, data, tSize ); // copy data
				tpData = tData; // data pointer
				flash_program( tAddr, *tpData ); // begin programming
				tState = FLASH_STATE_PROGRAMMING;
				break;
			case PETPRG_CMD_STATUS:
				break;
			default:
				return;	// no response
		}
		tLastIndex++;
	}
	respond();
}

void petprg( void ) {
	u8 tTemp;
	tRun = 0;
	while( 1 ) {
		if( tSize ) {
			di;
			tTemp = flash_state();
			if( tTemp != FLASH_STATE_PROGRAMMING ) {
				if( tTemp == FLASH_STATE_READY ) {
					tSize--;	// bytes left
					if( tSize ) {
						tAddr++;	// next address
						tpData++;
						flash_program( tAddr, *tpData ); // keep programming
					} else {
						tState = tTemp;
						respond(); // notify done programming
					}
				} else {
					tState = tTemp;
					tSize = 0; // stop programming
					respond(); // notify error programming
				}
			}
			ei;
		} else if( tState == FLASH_STATE_ERASING ) {
			di;
			tState = flash_state();
			if( tState != FLASH_STATE_ERASING ) {
				respond(); // notify done erasing
			}
			ei;
		} else if( tRun ) {
			flash_reset();
			return;
		}
	}
}