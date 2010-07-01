#include "gen_defines.h"

static u8 *pData;

void mod_data_reset( void ) {
  pData = ( u8* )0x100000;
}

void mod_data_read( u8* pDest, u24 nof_bytes ) {
  if( pDest ) {
    while( nof_bytes-- ) {
      *pDest++ = *pData++;
    }
  } else {
    pData += nof_bytes;
  }
}
