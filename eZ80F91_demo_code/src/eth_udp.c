// VERY BASIC UDP FUNCTIONALITY

/* == INCLUDES ================================================================================== */

#include "eth_ip.h"
#include <string.h>

/* == LOCAL DATA ================================================================================ */

static const u8 udp_proto[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Destination: N/A  0x############
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source:      N/A  0x############
  0x08, 0x00,                         // Type:        IP   0x0800
  0x45,                               // Version:     4    0x45
  0x00,                               // DSF:         None 0x00
  0x00, 0x00,                         // Total size:  N/A  0x####
  0x00, 0x00,                         // ID:          N/A  0x####
  0x00, 0x00,                         // Flags:       None 0x0000
  0xFF,                               // TTL:         Max  0xFF
  0x11,                               // Protocol:    UDP  0x11
  0x00, 0x00,                         // IP CSum:     N/A  0x####
  0x00, 0x00, 0x00, 0x00,             // Dest IP:     N/A  0x########
  0x00, 0x00, 0x00, 0x00,             // Source IP:   N/A  0x########
  0x00, 0x00,                         // Source port: N/A  0x####
  0x00, 0x00,                         // Dest port:   N/A  0x####
  0x00, 0x00,                         // UDP Length:  N/A  0x####
  0x00, 0x00                          // UDP CSum:    N/A  0x####
};

u16 tUnique;

/* == EXPORTED DATA ============================================================================= */

/* == IMPORTED PROCEDURES ======================================================================= */

u8 memcpy_csum( void *dest, const void *src, u24 count );
void petprg_parse( u8 *data, u24 len );

/* == LOCAL PROCEDURES ========================================================================== */

/* == EXPORTED PROCEDURES ======================================================================= */

// Initialize UDP
void udp_open( void ) {
  u8 *tpHdr;

  // Initialize header
	tpHdr = gpSocket.hdr;
	memcpy( tpHdr, udp_proto, sizeof( udp_proto ) );
	memcpy( tpHdr + ETHHDR_SOURCE, gpMAC, 6 );
	memcpy( tpHdr + IPHDR_SOURCE, gpIP, 4 );
	gpSocket.hdr_size = sizeof( udp_proto );

  // Initialize checksum for IP header
  gpSocket.csum_a = 0x4511 + ( 0xFF00 ) + CSUM32( tpHdr + IPHDR_SOURCE );
  
}

// Set parameter
//
// Note: Valid parameters for UDP are:
//
//         P_SRCPORT = Source port number (u16)
//           Must be set before a socket will recieve any data.
//
//         P_DSTPORT = Source port number (u16)
//           Must be set before a socket can be used to send data.
//
//         P_DSTADDRIP = Destination IP-address (u8[ IPV4_ADDR_LEN ])
//           Will not perform an ARP lookup, MAC-address must be set manually.
//
//         P_DSTADDRMAC = Destination MAC-address (u8[ ETH_ADDR_LEN ])
//           Used in conjunction with P_DSTADDRIP to manually set MAC-address.
//
// ipSocket Socket for which to set parameter
// iParam   Parameter to set
// ipData   Pointer to parameter data
//
// return   ERR_OK    OK
//          ERR_PARAM Parameter error

void udp_set( u8 iParam, void* ipData ) {
  u8 *tpHdr = gpSocket.hdr;

  // Set parameter
  if( iParam == P_SRCPORT ) {
    HDR16( UDPHDR_SRCPORT ) = HN16( *( ( u16* )ipData ) );
    gpSocket.srcport = *( ( u16* )ipData );
	} else if( iParam == P_DSTPORT ) {
    HDR16( UDPHDR_DSTPORT ) = HN16( *( ( u16* )ipData ) );
    gpSocket.dstport = *( ( u16* )ipData );
  } else if( iParam == P_DSTADDRIP ) {
    memcpy( tpHdr + IPHDR_DEST, ipData, 4 );
  } else if( iParam == P_DSTADDRMAC ) {
    memcpy( tpHdr + ETHHDR_DEST, ipData, 6 );
  }

  // Initialize checksum for IP header (step 2)
  gpSocket.csum_b = gpSocket.csum_a + CSUM32( tpHdr + IPHDR_DEST );

  // Initialize checksum for UDP header
  gpSocket.csum_c = ( gpSocket.csum_b - 0x4500 - 0xFF00 )
                   + NH16( HDR16( UDPHDR_SRCPORT ) ) + NH16( HDR16( UDPHDR_DSTPORT ) );

}

// Decode UDP packet
void udp_decode( u16 iCSum ) {
  u16   tPktSrcPort, tPktDstPort;                    // Ports
  u16   tPktCSum;
  u24   tPktsz, tWrapsz;

  u8   *tpCSumBuf;                                   // Checksum calculation
  u24   tCSumBufsz;                        
	u8   *tpHdr = gpSocket.hdr;

	u8    tpBroadcast[ 6 ] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

  // Get source & destination ports
  tPktSrcPort = NH16( *( u16* )gpEthPktBuf ); gpEthPktBuf += 2;
  tPktDstPort = NH16( *( u16* )gpEthPktBuf ); gpEthPktBuf += 2;

	if( gpSocket.state != 0 ) {
    if( memcmp( gpIP, gpIPDstIP, 4 ) ) return;

		if( gpSocket.state == 1 ) {
			// Lock communications to session
			udp_set( P_SRCPORT, &tPktDstPort );
			udp_set( P_DSTPORT, &tPktSrcPort );
			udp_set( P_DSTADDRIP, &gpIPSrcIP );
			udp_set( P_DSTADDRMAC, &gpEthSrcMAC );
			gpSocket.state = 2;
		} else if( gpSocket.state == 2 ) {
			// Verify packet source
			if( gpSocket.srcport != tPktDstPort ) return;
	    if( memcmp( tpHdr + IPHDR_DEST, gpIPSrcIP, 4 ) ) return;
	    if( memcmp( tpHdr + ETHHDR_DEST, gpEthSrcMAC, 6 ) ) return;
	    if( memcmp( gpIP, gpIPDstIP, 4 ) ) return;
			// Allow port switch (app restart on pc)
			if( gpSocket.dstport != tPktSrcPort ) {
				udp_set( P_DSTPORT, &tPktSrcPort );
			};
		}
	}

  tPktsz = NH16( *( u16* )gpEthPktBuf );
  gpEthPktBuf += 2;

  // Checksums enabled? Verify it
  tPktCSum = NH16( *( u16* )gpEthPktBuf );
  if( tPktCSum != 0 ) {
    *( u16* )gpEthPktBuf = 0;
    DOCS16( iCSum, csummem( gpEthPktBuf - ( UDPHDR_CSUM - EIPHDR_SIZE ), tPktsz ) );
    if( iCSum != ( u16 )~tPktCSum ) return;           // EXIT: CSUM FAIL
  }
  gpEthPktBuf += 2;

	tPktsz -= UDPHDR_SIZE;

	if( gpSocket.state == 0 ) {
		if( tPktsz == 16 ) {
			if( memcmp( gpEthPktBuf, "PETPRG_INIT", 12 ) == 0 ) {
				assign_address( gpEthPktBuf + 12 );
				gpSocket.state = 1;
				udp_open();
				udp_set( P_SRCPORT, &tPktDstPort );
				udp_set( P_DSTPORT, &tPktSrcPort );
				udp_set( P_DSTADDRIP, &tpBroadcast );
				udp_set( P_DSTADDRMAC, &tpBroadcast );
				udp_send( ( u8* )"PETPRG_READY", 12 );
			}
		}
	} else {
		petprg_parse( gpEthPktBuf, tPktsz );
	}
}

// Send UDP data
void udp_send( u8 *ipData, u24 iData_size ) {
  u8* tpHdr = gpSocket.hdr;
  u32 tCSum;
	u16 tId;
  ethlist_t tHdr, tData;

  // Finalize IP & UDP Length
  HDR16( IPHDR_LENGTH ) = HN16( iData_size + IUDPHDR_SIZE );
  HDR16( UDPHDR_LENGTH ) = HN16( iData_size + UDPHDR_SIZE );

  // Unique ID
  tId = tUnique++;
  HDR16( IPHDR_ID ) = HN16( tId );

  // Finalize IP CSUM
  tCSum = gpSocket.csum_b + tId + iData_size + IUDPHDR_SIZE;
  while( tCSum >> 16 ) tCSum = ( tCSum & 0xFFFF ) + ( tCSum >> 16 );
  HDR16( IPHDR_CSUM ) = HN16( ~tCSum );

  // Set header & data buffer
  tHdr.size = sizeof( udp_proto );
  tHdr.ptr = gpSocket.hdr;
  tHdr.link = &tData;
  tData.ptr = ipData;
  tData.size = iData_size;
  tData.link = 0;
  
  // Finalize UDP CSUM (if enabled)
  tCSum = gpSocket.csum_c + ( ( iData_size + 8 ) << 1 );
  while( iData_size-- ) {
    tCSum += ( *ipData++ ) << 8;
    if( iData_size ) {
      iData_size--;
      tCSum += *ipData++;
    }
  }
  while( tCSum >> 16 ) tCSum = ( tCSum & 0xFFFF ) + ( tCSum >> 16 );
  if( tCSum != 0xFFFF ) tCSum = ~tCSum;
  HDR16( UDPHDR_CSUM ) = HN16( tCSum );

  // Transmit packet
	eth_send( &tHdr );

}
