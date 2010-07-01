// VERY BASIC ARP FUNCTIONALITY

/* == INCLUDES ================================================================================== */

#include "eth_ip.h"
#include <string.h>

/* == LOCAL TYPES =============================================================================== */

/* ARP RECEIVE QUEUE SLOT */
typedef struct {
  u8 opcode;
  u8 ip[ 4 ];
  u8 mac[ 6 ];
} arp_queue_t;

/* ARP ENTRY TABLE SLOT */
typedef struct {
  u8 ip[ 4 ];
  u8 mac[ 6 ];
} arp_entry_t;

/* == LOCAL DATA ================================================================================ */

/* ARP HEADER PROTOTYPE */
static const u8 arp_proto[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,         // Destination: N/A  0x############
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,         // Source:      N/A  0x############
  0x08, 0x06,                                 // Type:        ARP  0x0806
  0x00, 0x01,                                 // Hardware:    Eth  0x0001
  0x08, 0x00,                                 // Protocol:    IP   0x0800
  0x06, 0x04,                                 // Sizes:       6/4  0x0604
  0x00, 0x00,                                 // Opcode:      N/A  0x00##
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,            // Sender MAC:  N/A  0x############
  0x00, 0x00, 0x00, 0x00,                     // Sender IP:   N/A  0x########
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,         // Target MAC:  N/A  0x############
  0x00, 0x00, 0x00, 0x00                      // Target IP:   N/A  0x########
};

/* ARP PACKET BUFFER */
static u8 mpARPA[ sizeof( arp_proto ) ];      // Answer (response)

/* == LOCAL PROCEDURES ========================================================================== */

static void arp_respond( u8 mac[ 6 ], u8 ip[ 4 ] ) {
	ethlist_t bl;
	// Set up destination&target MAC
  memcpy( mpARPA + ETHHDR_DEST, mac, 6 );
  memcpy( mpARPA + ARPHDR_TARGETMAC, mac, 6 );
  // Set up target IP
  memcpy( mpARPA + ARPHDR_TARGETIP, ip, 4 );

	bl.ptr = mpARPA;
	bl.size = sizeof( arp_proto );
	bl.link = 0;
	eth_send( &bl );
}

/* == EXPORTED PROCEDURES =================================================== */

// Initialize ARP subsystem
void arp_init( void ) {

  // Initialize ARP
  memcpy( mpARPA, arp_proto, sizeof( arp_proto ) );
  memcpy( mpARPA + ETHHDR_SOURCE, gpMAC, 6 );
  memcpy( mpARPA + ARPHDR_SENDERMAC, gpMAC, 6 );
  mpARPA[ ARPHDR_OPCODE ] = 0x02;

}

// Notify ARP subsystem that the local IP-address has changed
void arp_notify( void ) {
  memcpy( mpARPA + ARPHDR_SENDERIP, gpIP, 4 );
}

// Decode ARP packet
void arp_decode( void ) {
  u16 tOpcode;
  //if( gEthPktBufsz < ARPHDR_SIZE ) return;                   // INCOMPLETE PACKET
  // Verify hardware type
  if( *( u16* )gpEthPktBuf != HNC16( 0x0001 ) ) return;
  gpEthPktBuf += 2;
  // Verify protocol type
  if( *( u16* )gpEthPktBuf != HNC16( 0x0800 ) ) return;
  gpEthPktBuf += 2;
  // Verify hardware/protocol size
  if( *( u16* )gpEthPktBuf != HNC16( 0x0604 ) ) return;
  gpEthPktBuf += 2;
  // Get opcode
  tOpcode = *( u16* )gpEthPktBuf;
  gpEthPktBuf += 2;
  // Verify IP-address
  if( memcmp( gpIP, gpEthPktBuf + 16, 4 ) != 0 ) {
    return;
  }
	// Discard if not request
  if( tOpcode != HNC16( 0x0001 ) ) {
    return;
  }
	// Send response
  arp_respond( gpEthPktBuf, gpEthPktBuf + 4 );
}