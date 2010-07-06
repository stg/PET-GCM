#include "gen_defines.h"

#define ETH_RXBUFFER   1152																	// Max packet size

#define P_SRCPORT         0  		        										// u16   Source port
#define P_DSTPORT         1     		     										// u16   Destination port
#define P_DSTADDRIP       3         		 										// u8[4] Destination IP
#define P_DSTADDRMAC      4          												// u8[6] Destination MAC

/* HEADER SIZES (TOP LEVEL) */

#define ETHHDR_SIZE      14                                 // Ethernet II (Data-link)
#define ARPHDR_SIZE      28                                 // ARP (Packet)
#define IPHDR_SIZE       20                                 // IP (Packet)
#define UDPHDR_SIZE       8                                 // UDP (Transport)

/* HEADER SIZES (ETHERNET LEVEL) */

#define EIPHDR_SIZE        ( ETHHDR_SIZE + IPHDR_SIZE )
#define EARPHDR_SIZE       ( ETHHDR_SIZE + ARPHDR_SIZE )
#define EUDPHDR_SIZE       ( ETHHDR_SIZE + IPHDR_SIZE + UDPHDR_SIZE )

/* HEADER SIZES (IP LEVEL) */

#define IUDPHDR_SIZE       ( IPHDR_SIZE + UDPHDR_SIZE )

/* HEADER OFFSETS */

#define ETHHDR_DEST       0                                 // Ethernet II: Destination MAC
#define ETHHDR_SOURCE     6                                 // Ethernet II: Source MAC
#define ETHHDR_TYPE      12                                 // Ethernet II: Type

#define ARPHDR_OPCODE    21                                 // ARP: Opcode
#define ARPHDR_SENDERMAC 22                                 // ARP: Sender MAC
#define ARPHDR_SENDERIP  28                                 // ARP: Sender IP
#define ARPHDR_TARGETMAC 32                                 // ARP: Target MAC
#define ARPHDR_TARGETIP  38                                 // ARP: Target IP

#define IPHDR_LENGTH     16                                 // IP: Length
#define IPHDR_ID         18                                 // IP: Identification
#define IPHDR_CSUM       24                                 // IP: Checksum
#define IPHDR_SOURCE     26                                 // IP: Source IP
#define IPHDR_DEST       30                                 // IP: Destination IP

#define UDPHDR_SRCPORT   34                                 // UDP: Source port
#define UDPHDR_DSTPORT   36                                 // UDP: Destination port
#define UDPHDR_LENGTH    38                                 // UDP: Length
#define UDPHDR_CSUM      40                                 // UDP: Checksum

/* HEADER ACCESS */

#define HDR8(o)     ( *( u8 * )( tpHdr + o ) )
#define HDR16(o)    ( *( u16* )( tpHdr + o ) )
#define HDR32(o)    ( *( u32* )( tpHdr + o ) )
#define CSUM32(o)   ( csum32( ( ( u32* )( o ) ) ) )
#define DOCS16(o,i) csum16add( &o, i )

/* NETWORK/HOST BYTE ORDER CONVERSION */

#define NH16(w)   ( hnc16(w) )
#define HN16(w)   ( hnc16(w) )
#define NH32(dw)  ( hnc32(dw) )
#define HN32(dw)  ( hnc32(dw) )
#define NHC16(w)   ( ( ( w & 0x00FF )<<8 ) | ( ( w>>8 ) & 0x00FF ) )
#define HNC16(w)   ( ( ( w & 0x00FF )<<8 ) | ( ( w>>8 ) & 0x00FF ) )
#define NHC32(dw)  ( ( dw>>24 ) | ( dw<<24 ) | ( ( dw>>8 ) & 0xFF00 ) | ( ( dw<<8 ) & 0xFF0000 ) )
#define HNC32(dw)  ( ( dw>>24 ) | ( dw<<24 ) | ( ( dw>>8 ) & 0xFF00 ) | ( ( dw<<8 ) & 0xFF0000 ) )

/* LINKED LIST OF BUFFERS */

typedef struct {
  u8* ptr;
  u24 size;
  void* link;
} ethlist_t;

/* SOCKET DESCRIPTOR */

typedef volatile struct {
  u8        state;                                          // Sockets: State
  u16       srcport;                                        // Sockets: Source port/service
  u16       dstport;                                        // Sockets: Destination port/service
  u8        hdr[ EUDPHDR_SIZE ];                            // Ethernet II: Header data
  u24       hdr_size;                                       // Ethernet II: Header data size
  u32       csum_a;                                         // Various: Checksum calculations
  u32       csum_b;
  u32       csum_c;
} ip_socket_t;

/* ASM HAL */

u16  hnc16( u16 );
u32  hnc32( u32 );
u16  csum32( u32* );
u16  csummem( void* ipData, u16 iBytes );
void csum16add( u16* ipCSum, u16 iValue );

/* GLOBALS */

extern u8           gpMAC[ 6 ];							// MAC address
extern u8           gpIP [ 4 ];							// IPv4 address
extern u8           ethbuf[ ETH_RXBUFFER ];	// Packet buffer
extern u8          *gpEthPktBuf;        		// Packet parse pointer
extern u24          gEthPktBufsz;       		// Packet size
extern ip_socket_t  gpSocket;   						// Socket
extern u8           gpEthSrcMAC[ 6 ];       // MAC address from Ethernet II layer
extern u8           gpIPSrcIP[ 4 ];         // IP address from IPv4 layer
extern u8           gpIPDstIP[ 4 ];         // IP address from IPv4 layer

/* DECLARATIONS */

u8   eth_send       ( ethlist_t *bl );
u8   eth_established( void );
u8   eth_init       ( void );
void eth_decode     ( void );
void ip_init        ( void );
void udp_send       ( u8 *ipData, u24 iData_size );
void assign_address ( u8 ipIP[ 4 ] );
void eth_disable    ( void );