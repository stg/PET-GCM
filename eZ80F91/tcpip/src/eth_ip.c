// VERY BASIC ETHERNET AND IP FUNCTIONALITY

/* == INCLUDES ================================================================================== */

#include "eth_ip.h"
#include <string.h>

/* == EXPORTED DATA ============================================================================= */

/* ADDRESSES */
u8 gpMAC[ 6 ] = "PETPRG"; 			// Ethernet address
u8 gpIP [ 4 ];            			// IP address

/* ETHERNET RECEIVE BUFFER */
u8  ethbuf[ ETH_RXBUFFER ];     // Buffer

/* CURRENTLY PROCESSING PACKET */
u8  *gpEthPktBuf;               // Data pointer
u8   gpEthSrcMAC[ 6 ];					// MAC address from Ethernet II layer
u8   gpIPSrcIP  [ 4 ];   				// IP address from IPv4 layer
u8   gpIPDstIP  [ 4 ];   				// IP address from IPv4 layer

/* SOCKETS */
ip_socket_t gpSocket;   				// Socket

/* == IMPORTED PROCEDURES ======================================================================= */

/* ARP IMPORTS */
void arp_init( void );
void arp_notify( void );
void arp_decode( void );

/* UDP IMPORTS */
void udp_decode( u16 iCSum );

/* == LOCAL PROCEDURES ========================================================================== */

// Decodes IP header
static void ip_decode( void ) {
  u8   tPktProtocol;
  u16  tPktCSum;
  u16  tPktSz;
  u24  tHdrSz;
  u16  tCSum, tPseudoCSum;
  u8  *tpCSumBuf;                                
  u24  tCSumBufsz;                        

  //if( gEthPktBufsz < IPHDR_SIZE ) return;                   // EXIT:INCOMPLETE PACKET

  // Verify protocol version (IPv4 = 0x4n) and header size
  if( ( *gpEthPktBuf & 0xF0 ) != 0x40 ) return;                       // Verify version
  tHdrSz = ( *gpEthPktBuf & 0x0F ) << 2;                              // Get header size
  if( tHdrSz < IPHDR_SIZE ) return;                                   // Verify header size
  
  // Verify checksum
  tPktCSum = NH16( *( u16* )( gpEthPktBuf + IPHDR_CSUM - ETHHDR_SIZE ) );  // Get csum
  *( u16* )( gpEthPktBuf + IPHDR_CSUM - ETHHDR_SIZE ) = 0;         // Clear csum
  tCSum = csummem( gpEthPktBuf, tHdrSz );                             // Calc csum
  if( tCSum != ( u16 )~tPktCSum ) return;                          // Verify
  
  gpEthPktBuf += 2;                                                   // Skip version, ignore TOS

  // Verify length, skip ID
  tPktSz = NH16( *( u16* )gpEthPktBuf );
  //if( tPktSz > gEthPktBufsz ) return;
  gpEthPktBuf += 4;

  // Fragmentation is not supported, ignore TTL
  if( ( *( u16* )gpEthPktBuf & HNC16( 0xBFFF ) ) != HNC16( 0x0000 ) ) return;
  gpEthPktBuf += 3;

  // Get protocol, skip checksum
  tPktProtocol = *gpEthPktBuf;
  tPseudoCSum  = ( u16 )tPktProtocol;                               // Pseudoheader: Protocol
  gpEthPktBuf += 3;

  // Get source address
  *( u32* )gpIPSrcIP = *( u32* )gpEthPktBuf;
  DOCS16( tPseudoCSum, CSUM32( gpEthPktBuf ) );                        // Pseudoheader: Source IP
  gpEthPktBuf += 4;

  // Verify address against local and broadcast
	memcpy( gpIPDstIP, gpEthPktBuf, 4 );
  if (
    *( u32* )gpIP != *( u32* )gpEthPktBuf &&
    0xFFFFFFFF    != *( u32* )gpEthPktBuf
  ) return;

  // Destination address
  DOCS16( tPseudoCSum, CSUM32( gpEthPktBuf ) );                        // Pseudoheader: Dest IP
  gpEthPktBuf += 4;

  // Skip header and options
  gpEthPktBuf += tHdrSz - IPHDR_SIZE;

  // Length
  tPktSz -= tHdrSz;
  DOCS16( tPseudoCSum, tPktSz );

  // Supported IP protocols
	if( tPktProtocol == 0x11 ) {  // UDP
    udp_decode( tPseudoCSum );
  }

}

// Decodes an Ethernet II header
void eth_decode( void ) {
  u16 tEthType;
  //if( gEthPktBufsz < ETHHDR_SIZE ) return;            // EXIT:INCOMPLETE PACKET
  
  // Skip destination (MII does filtering for us)
  gpEthPktBuf += 6;
  
  // Store source
  memcpy( gpEthSrcMAC, gpEthPktBuf, 6 );
  gpEthPktBuf += 6;
  
  // Get type
  tEthType = *( u16* )gpEthPktBuf; gpEthPktBuf += 2;
  
  // Call appropriate decoder
  if( tEthType == HNC16( 0x0800 ) ) {                  // IP
    ip_decode();
  } else if( tEthType == HNC16( 0x0806 ) ) {           // ARP
    arp_decode();
  }
}

// Assigns IP address
void assign_address( u8 ipIP[ 4 ] ) {
		memcpy( gpIP,  ipIP, 4 );
    arp_notify();
}

/* == EXPORTED PROCEDURES ======================================================================= */

// Initialize IP and lower layers
void ip_init( void ) {
  u24 n;
  u8 *tpHdr;

  // Initialize ARP
  arp_init();

  // Initialize MII and PHY
  eth_init();

}