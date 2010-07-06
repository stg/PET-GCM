/* **** INCLUDES **** */

#include "gen_defines.h"
#include "driver_flash.h"

/* **** LOCAL DEFINES **** */

#define FLASH_ADDR       ( 0x100000 )
#define FLASH_SIZE       ( 0x100000 )

#define OFFSET_BASE      ( 0x000 )
#define OFFSET_UNLOCK1   ( 0x555 )
#define OFFSET_UNLOCK2   ( 0x2AA )
#define OFFSET_COMMAND   ( 0x555 )

#define CMD_UNLOCK1      ( 0xAA )
#define CMD_UNLOCK2      ( 0x55 )
#define CMD_RESET        ( 0xF0 )
#define CMD_ERASE_SETUP  ( 0x80 )
#define CMD_CHIP_ERASE   ( 0x10 )
#define CMD_SECTOR_ERASE ( 0x30 )
#define CMD_PROGRAM      ( 0xA0 )

#define DQ5              ( 0x20 )
#define DQ7              ( 0x80 )

#define FLASHREGISTER( FLASHREG ) *( volatile u8* )( FLASH_ADDR + FLASHREG )
#define FLASHBUSY FLASHREGISTER( OFFSET_BASE ) != FLASHREGISTER( OFFSET_BASE )


/* **** LOCAL VARIABLES **** */

static u8  mDriverState = FLASH_STATE_READY;
static u8 *mpCurrentAddress;
static u8  mCurrentData;

/* **** LOCAL METHODS **** */

/* **** PUBLIC METHODS  **** */

// Reset the selected device
u8 flash_reset( void ) {
  // Issue reset command
  FLASHREGISTER( OFFSET_BASE ) = CMD_RESET;

  // Determine if reset was successful (resets are ignored when erasing)
  if( FLASHBUSY ) {
    return( 0 );
  } else {
    mDriverState = FLASH_STATE_READY;
    return( 1 );
  }
}

// Retrieve the current driver state
u8 flash_state( void ) {
	u8 tStatus;
  // When device is in write mode (erase/program)...
  if( mDriverState == FLASH_STATE_ERASING || mDriverState == FLASH_STATE_PROGRAMMING ) {
		tStatus = *mpCurrentAddress;
		if( *( volatile u8 * )mpCurrentAddress == tStatus ) {
			if( *mpCurrentAddress == mCurrentData ) {
				mDriverState = FLASH_STATE_READY;
			} else {
				mDriverState = FLASH_STATE_ERROR;
			}
		} else {
			if( tStatus & DQ5 ) {
				mDriverState = FLASH_STATE_ERROR;
			}
		}
	}
/*
    // ... determine operation status and set appropriate state (ready/error) by:
    // 1) Checking DQ7
    // 2) Checking DQ5 for operation time-out
    // 3) Re-checking DQ7 to ensure valid DQ5 data
		tStatus = *mpCurrentAddress;
    if ( ( *mpCurrentAddress & DQ7 ) == ( mCurrentData & DQ7 ) ) {
      mDriverState = FLASH_STATE_READY;
    } else if ( *mpCurrentAddress & DQ5 ) {
      if ( ( *mpCurrentAddress & DQ7 ) == ( mCurrentData & DQ7 ) ) {
        mDriverState = FLASH_STATE_READY;
      } else {
        mDriverState = FLASH_STATE_ERROR;
      }
    }
  }
*/
  // Return state
  return( mDriverState );
}

// Begin/perform a chip erase
u8 flash_format( void ) {
  
  // When device is not in ready state or address out of range
  if( mDriverState != FLASH_STATE_READY ) return( 0 );

  // Issue sector erase command sequence
  FLASHREGISTER( OFFSET_UNLOCK1 ) = CMD_UNLOCK1;
  FLASHREGISTER( OFFSET_UNLOCK2 ) = CMD_UNLOCK2;
  FLASHREGISTER( OFFSET_COMMAND ) = CMD_ERASE_SETUP;
  FLASHREGISTER( OFFSET_UNLOCK1 ) = CMD_UNLOCK1;
  FLASHREGISTER( OFFSET_UNLOCK2 ) = CMD_UNLOCK2;
  FLASHREGISTER( OFFSET_COMMAND ) = CMD_CHIP_ERASE;

  // Store current address/data
  mpCurrentAddress = ( u8* )( FLASH_ADDR + FLASH_SIZE - 1 );
  mCurrentData = 0xFF;

  // Set erasing state and return
  mDriverState = FLASH_STATE_ERASING;

  // Success
  return( 1 );

}

// Begin/perform a sector erase
u8 flash_erase( u24 iAddress ) {
  
  // When device is not in ready state or address out of range
  if( mDriverState != FLASH_STATE_READY ) return( 0 );
  if( iAddress >= FLASH_SIZE ) return( 0 );

  // Issue sector erase command sequence
  FLASHREGISTER( OFFSET_UNLOCK1 ) = CMD_UNLOCK1;
  FLASHREGISTER( OFFSET_UNLOCK2 ) = CMD_UNLOCK2;
  FLASHREGISTER( OFFSET_COMMAND ) = CMD_ERASE_SETUP;
  FLASHREGISTER( OFFSET_UNLOCK1 ) = CMD_UNLOCK1;
  FLASHREGISTER( OFFSET_UNLOCK2 ) = CMD_UNLOCK2;
  FLASHREGISTER( iAddress       ) = CMD_SECTOR_ERASE;

  // Store current address/data
  mpCurrentAddress = ( u8* )( FLASH_ADDR + iAddress );
  mCurrentData = 0xFF;

  // Set erasing state and return
  mDriverState = FLASH_STATE_ERASING;

  // Success
  return( 1 );
}

// Program a single byte to the selected device
u8 flash_program( u24 iAddress, u8 iData ) {
  
  // When device is not in ready state or address out of range
  if( mDriverState != FLASH_STATE_READY ) return( 0 );
  if( iAddress >= FLASH_SIZE ) return( 0 );
  
  // Write program command sequence.
  FLASHREGISTER( OFFSET_UNLOCK1 ) = CMD_UNLOCK1;
  FLASHREGISTER( OFFSET_UNLOCK2 ) = CMD_UNLOCK2;
  FLASHREGISTER( OFFSET_COMMAND ) = CMD_PROGRAM;
  FLASHREGISTER( iAddress       ) = iData;

  // Store current address/data
  mpCurrentAddress = ( u8* )( FLASH_ADDR + iAddress );
  mCurrentData = iData;

  // Set programming state
  mDriverState = FLASH_STATE_PROGRAMMING;

  // Success
  return( 1 );
}

// Retrieve real address
u8 *flash_addrPtr( u24 iAddress ) {
  return( ( u8* )( FLASH_ADDR + iAddress ) );
}

// Retrieve the number of sectors available
u8 flash_sectorCount( void ) {
  return( 19 );
}

// Retrieve the size of a sector
u24 flash_sectorSize( u8 iSector ) {
  if( iSector > 18 ) return( FLASH_OUT_OF_RANGE );
  if( iSector == 0 ) return( 0x4000 );
  if( iSector == 1 ) return( 0x2000 );
  if( iSector == 2 ) return( 0x2000 );
  if( iSector == 3 ) return( 0x8000 );
  return( 0x10000 );
}

// Retrieve the sector for an address
u8 flash_addrSector( u24 iAddress ) {
  if( iAddress > 0x0FFFFF ) return( ( u8 )FLASH_OUT_OF_RANGE );
  if( iAddress > 0x7FFF )   return( ( iAddress >> 16 ) + 3 );
  if( iAddress > 0x3FFF )   return( 3 );
  if( iAddress > 0x2FFF )   return( 2 );
  if( iAddress > 0x1FFF )   return( 1 );
  return( 0 );
}

// Retrieve base address for a sector
u24 flash_sectorAddr( u24 iSector ) {
  if( iSector > 18 ) return( FLASH_OUT_OF_RANGE );
  if( iSector >= 4 ) return( ( iSector - 3 ) << 16 );
  if( iSector == 3 ) return( 0x008000 );
  if( iSector == 2 ) return( 0x006000 );
  if( iSector == 1 ) return( 0x004000 );
  return( 0x000000 );
}