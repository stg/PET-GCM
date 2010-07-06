// Driver: AM29LV008HH
// senseitg@hotmail.com

/* == INCLUDES ================================================================================== */

#include "gen_defines.h"

/* == DEFINES =================================================================================== */

#define FLASH_STATE_READY 				1
#define FLASH_STATE_ERASING 			2
#define FLASH_STATE_PROGRAMMING 	3
#define FLASH_STATE_ERROR 				4
#define FLASH_OUT_OF_RANGE 			  0xFFFFFF

/* == DECLARATIONS ============================================================================== */

// Reset the device to read-mode. Must be called if an error has occurred, see flash_state.
u8 flash_reset( void );

// This method retrieves the current state of the driver and device. It must be polled continously
// after a erase/program operation to determine if the driver is ready to accept new commands.
//
// Return values:
//   FLASH_STATE_FREE          Driver has not been aquired, commands cannot be issued
//   FLASH_STATE_READY         Driver is ready to recieve commands
//   FLASH_STATE_ERASING       Driver is busy erasing
//   FLASH_STATE_PROGRAMMING   Driver is busy programming
//   FLASH_STATE_ERROR         A timeout error has occured, a reset must be issued
u8 flash_state( void );

// Begin/perform an erase on the entire chip.
// The driver must me in ready state for this operation to succeed.
// 
// Return values:
//    1         Operation completed successfully
//    0         Operation could not be executed
u8 flash_format( void );

// Begin/perform an erase on the specified sector.
// The driver must me in ready state for this operation to succeed.
// 
// iAddress     Any address within the sector that is to be erased
//
// Return values:
//    1         Operation completed successfully
//    0         Operation could not be executed
u8 flash_erase( u24 iAddress );

// Begin/perform byte programming.
// The driver must me in ready state for this operation to succeed.
// 
// iAddress     Any address within erased memory
// iData        Data to be programmed
//
// Return values:
//    1         Operation completed successfully
//    0         Operation could not be executed
u8 flash_program( u24 iAddress, u8 iData );

// Retrieve the size of the sector specified by iSector
//
// iSector      The sector for which to retrieve a size
//
// Return values:
//   FLASH_OUT_OF_RANGE  iSector is out of range
//   Other               Valid sector
u24 flash_sectorSize( u8 iSector );

// Retrieve the sector which iAddress falls within
//
// iAddress     The address for which to retrieve a sector
//
// Return values:
//   FLASH_OUT_OF_RANGE  iAddress is out of range
//   Other                Valid sector
u8 flash_addrSector( u24 iAddress );

// Retrieve the base address of sector specified by iSector
//
// iSector      The sector for which to retrieve an address
//
// Return values:
//   FLASH_OUT_OF_RANGE  iSector is out of range
//   Other                Valid address
u24 flash_sectorAddr( u24 iSector );