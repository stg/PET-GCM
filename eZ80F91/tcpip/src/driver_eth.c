// VERY BASIC ETHERNET DRIVER

#include "driver_eth.h"
#include "eth_ip.h"
#include <string.h>

extern void * _set_vector( u24 vector, void ( *handler )( void ) );

extern void   emac_sys_isr();
extern void   emac_rx_isr( void );
extern void   emac_tx_isr( void );

/* Configuration */
static const F91_EMAC_CONF_T F91_emac_config = {
   2048,    // MAC transmit buffer
   10,      // Speed - 0: negotiate, 10: 10Mbps, 100: 100Mbps
   0,       // Duplex - 1: full duplex, 0: half duplex
   BUF32    // EMAC_RAM packet buffer size
};

static u8    write_phy         ( u16 phy_reg, u16 data );
static u8    read_phy          ( u16 phy_reg, u16 *data );
static u24   init_txhdr        ( u16 pkt_size);
static void  check_overrun     ( void );
static u8    init_phy          ( void );

/* EMAC shared memory pointers */
static volatile DescTbl *twp;
static volatile DescTbl *trp;
static volatile DescTbl *rrp;
static          DescTbl *tlbp;
static          DescTbl *bp;
static          DescTbl *rhbp;
static          u16      buf_mask;
static          u16      buf_size;
static          u16      buf_size_shift;
static volatile u16      txBufsLeft;           // Tx packet buffers available
static          u8      *p_internal_sram_base; // MAC SRAM base

/* write phy register */
static u8 write_phy( u16 reg, u16 data ) {
  // begin write
  EMAC_CTLD_L = low8( data );
  EMAC_CTLD_H = high8( data );
  EMAC_RGAD = reg;
  EMAC_MIIMGT |= LCTLD;

	// wait for completion
  while( !( EMAC_ISTAT & MIIMGTDONE ) );

	// ok
  EMAC_ISTAT = MIIMGTDONE;

  return( 1 );
}

/* read phy register */
static u8 read_phy( u16 reg, u16* data ) {
  // begin read
  EMAC_RGAD = reg;
  EMAC_MIIMGT |= RSTAT;

	// wait for completion
  while( !( EMAC_ISTAT & MIIMGTDONE ) );

	// get value
	high8( *data ) = EMAC_PRSD_H;
	low8( *data ) = EMAC_PRSD_L;

	// ok
  EMAC_ISTAT = MIIMGTDONE;

	return( 1 );
}

/* initialize tx buffer header */
static u24 init_txhdr( u16 pkt_size ) {
  u16 temp;

	// wait for ownership
	while( twp->stat & EMAC_OWNS );

  // check if there is room in the buffer
	if( twp >= trp )  {
    if( pkt_size > ( ( u8* )trp - ( u8* )tlbp + ( u8* )bp - ( u8* )twp - DATA_OS ) ) {
      return( 0 );
    }
  } else if( pkt_size > ( ( u8 * )trp - ( u8 * )twp - DATA_OS ) ) {
    return( 0 );
  }

  // init header
  temp = pkt_size + buf_size - 1 + DATA_OS;
  twp->np = ( struct desctbl* )( ( u8 * )twp + ( temp & buf_mask ) );

	// check for wrap
  if( twp->np >= bp ) {
    twp->np = ( struct desctbl* )( ( u8* )tlbp + ( ( u8* )twp->np - ( u8* )bp ) );
  }

  twp->pktsz = pkt_size;

  return( 1 );
}

/* emac transmit isr */
void interrupt emac_tx_isr(void) {
  u16 temp;

  // clear interrupt
  EMAC_ISTAT = TXDONE;
   
  // update buffer count
  if( trp->np < trp ) {
    temp = ( ( ( u8* )bp - ( u8* )trp ) + ( ( u8* )trp->np - ( u8* )tlbp ) );
  } else {
    temp = ( ( u8* )trp->np - ( u8* )trp );
  }   
  txBufsLeft += ( temp >> buf_size_shift );

  // update trp
  trp = trp->np;
}

/* emac receive isr */
void interrupt emac_rx_isr(void) {
  u16 length;
  u8  *data;
	u24 wlen;

  // clear interrupt
  if( rrp->np == ( ( struct desctbl* )( p_internal_sram_base + EMAC_RWP_L + ( ( u16 )EMAC_RWP_H << 8 ) ) ) ) {
    EMAC_ISTAT = RXDONE;
  }

  // check for receive errors
  if( !( rrp->stat & ( RxOVR | RxAlignError | RxCrcError ) ) ) {
    
		// get packet
		length = ( rrp->pktsz - CRCSIZE );
    data = (u8 *)rrp + DATA_OS;

		if( data + length > ( u8* )rhbp ) {
			wlen = ( u8* )rhbp - data;
			memcpy( ethbuf, data, wlen );
			memcpy( ethbuf + wlen, bp, length - wlen );
		} else {
			memcpy( ethbuf, data, length );
		}
		gpEthPktBuf = ethbuf;
		eth_decode();

		// parse packet
		while( length-- ) {
			if( ++data == ( u8* )rhbp ) data = ( u8* )bp;
		}

  } else if( rrp->stat & RxOVR ) {
    check_overrun();
  }

  // update rrp
  rrp = rrp->np;
  EMAC_RRP_L = low8( rrp );
  EMAC_RRP_H = mid8( rrp );
}

/* emac events isr */
void interrupt emac_sys_isr(void) {
  u8 status;
  u16 temp;

  status = EMAC_ISTAT;

  if (status & RXOVRRUN) {
  	check_overrun();
	}

  if (status & MIIMGTDONE) {
		EMAC_ISTAT = MIIMGTDONE;
  }

	if (status & TXFSMERR) {
		EMAC_ISTAT = TXFSMERR;
		EMAC_RST = HRTFN | HRTMC;
		twp = trp = tlbp;
		temp = (char*)bp - (char*)tlbp;
		txBufsLeft = temp >> buf_size_shift;
		twp->stat = 0x0000;
		EMAC_RST = 0;  
	}
}

/* check and reset rx overrun */
static void check_overrun(void) {
  u16 count;
  count = ( ( ( u16 )EMAC_BLKSLFT_H ) << 8 ) | EMAC_BLKSLFT_L;
  if( count > MIN_NUM_RX_BUFS ) {
    EMAC_ISTAT = RXOVRRUN;
  }
}

/* initialize mii and phy */
u8 eth_init(void) {
  u16 i, temp;
  u16 phy_data, phy_creg_data;

	// enable and clear emac ram
  p_internal_sram_base = ( u8* )( ( ( ( unsigned )RAM_ADDR_U ) << 16 ) + EMAC_RAM_OFFSET );
  RAM_CTL |= ERAM_EN;
  memset( (char*)p_internal_sram_base, 0, EMAC_RAM_SIZE );

  // disable emac int
  EMAC_IEN = 0;

  // reset emac
  EMAC_RST = SRST | HRTFN | HRRFN | HRTMC | HRRMC | HRMGT;

  // set up mac
  EMAC_STAD_0 = gpMAC[ 0 ];
  EMAC_STAD_1 = gpMAC[ 1 ];
  EMAC_STAD_2 = gpMAC[ 2 ];
  EMAC_STAD_3 = gpMAC[ 3 ];
  EMAC_STAD_4 = gpMAC[ 4 ];
  EMAC_STAD_5 = gpMAC[ 5 ];

  // clear interrupt status
  EMAC_ISTAT = 0xff;

  // setup buffer size
  if( F91_emac_config.buf_size > BUF32 ) {
    buf_size = BUF32;
  } else {
    buf_size = F91_emac_config.buf_size;
  }
  buf_size_shift = 8 - buf_size;
  EMAC_BUFSZ = ( buf_size << 6 );
  buf_mask = 0xFFFF << buf_size_shift;
  buf_size = 1 << buf_size_shift;

  // set pause frame control timeout value
  EMAC_TPTV_L = 0x14;
  EMAC_TPTV_H = 0x00;

  // init descriptor tables boundry pointers
  tlbp = ( struct desctbl* )p_internal_sram_base;
  EMAC_TLBP_L = ( ( u8 )tlbp );
  EMAC_TLBP_H = ( ( u16 )tlbp ) >> 8;

  if( F91_emac_config.tx_buf_size > EMAC_RAM_SIZE ) {
    bp = ( struct desctbl* )( p_internal_sram_base + EMAC_RAM_SIZE );
  } else {
    bp = ( struct desctbl* )( p_internal_sram_base + F91_emac_config.tx_buf_size );
  }
  EMAC_BP_L   = low8( bp );
  EMAC_BP_H   = mid8( bp );
  EMAC_BP_U   = high8( bp );

  rhbp = ( struct desctbl* )( p_internal_sram_base + EMAC_RAM_SIZE );
  EMAC_RHBP_L = low8( rhbp );
  EMAC_RHBP_H = mid8( rhbp );

  // setup ring buffer pointers
  twp = trp = tlbp;
  rrp = bp;

  // write rrp into i/o reg rrp
  EMAC_RRP_L = low8( rrp );
  EMAC_RRP_H = mid8( rrp );

  // init # of tx buffers left in tx ring buffer
  temp = ( char* )bp - ( char* )tlbp;
  txBufsLeft = temp >> buf_size_shift;

  // host owns first buffer
  twp->stat = 0x0000;

  // set polling timer
  EMAC_PTMR = 1;

  // disable EMAC test modes
  EMAC_TEST = 0;

  // set the late collision byte at byte 55 */
  EMAC_CFG2 = 55;

  // set max # collisions = 15
  EMAC_CFG3 = 15;

  // enable the receiver
  EMAC_CFG4 = RXEN;

  // init the address filter
  EMAC_AFR = BROADCAST;

  // set the max # bytes per packet to 1536
  EMAC_MAXF_L = 0x00;
  EMAC_MAXF_H = 0x06;

  // take emac out of reset
  EMAC_RST = 0;  

  // address phy
  EMAC_FIAD = 0x1F;

  // set mmiimgt mdc clock rate
  EMAC_MIIMGT = CLKDIV20;

  if( !init_phy() ) {
    return( 0 );
  }

  // clear interrupt status register
  EMAC_ISTAT = 0xff;   

  // setup EMAC interrupts
  _set_vector( EMACRX_IVECT, emac_rx_isr );
  _set_vector( EMACTX_IVECT, emac_tx_isr );
  _set_vector( EMACSYS_IVECT, emac_sys_isr );

  // set emac int to prio 1
	INT_P0 |= 0x07;

  // enable ethernet interrupts
  EMAC_IEN = RXDONEIEN | TXDONEIEN | RXOVRRUNIEN | TXFSMERRIEN;

  return( 1 );
}

/* initialize phy */
static u8 init_phy( void ) {
  u16 phy_data;
  u8 cfg_data, auto_neg;

  // power down the phy
  write_phy( PHY_CREG, ( PHY_POWER_DOWN ) );
	
	// delay some
	phy_data = 0xFFFF; while( --phy_data );

  // enable padding and crc calculation
  cfg_data = PADEN | CRCEN;

  // set capabilities
  read_phy( PHY_SREG, &phy_data );
  if( ( phy_data & PHY_CAN_AUTO_NEG ) && ( F91_emac_config.link_speed == 0 ) ) {
    auto_neg = 1;
    phy_data = PHY_RESTART_AUTO_NEG | PHY_AUTO_NEG_ENABLE;
    write_phy(PHY_ANEG_ADV_REG, PHY_ANEG_100_FD | PHY_ANEG_100_HD |
                                    PHY_ANEG_10_FD  | PHY_ANEG_10_HD  | PHY_ANEG_802_3 );
  } else {
    auto_neg = 0;

    if( F91_emac_config.link_speed == 100 ) {
      phy_data = PHY_100BT;
    } else {
      phy_data = PHY_10BT;
    }

    if ( F91_emac_config.full_duplex ) {
      phy_data |= PHY_FULLD;
      cfg_data |= FULLD;
    }

    EMAC_CFG1 = cfg_data;
  }

  write_phy( PHY_CREG, phy_data );

  return( 1 );
  
}

/* check for link established */
u8 eth_established( void ) {
  u16 status;
  read_phy( PHY_SREG, &status );
  return( ( status & PHY_LINK_ESTABLISHED ) != 0 );
}

/* wrapping progressive copy */
static u8* memcpy_tx( u8 *ipDst, u8 *ipSrc, u24 iCount ) {
  u24 tPktsz;

  while( iCount ) {    
    // Clamp size
    if( ( ipDst + iCount ) > ( void* )bp ) tPktsz = ( u8* )bp - ipDst;
    else                                   tPktsz = iCount;
    // Copy
    memcpy( ipDst, ipSrc, tPktsz );
    // Update pointers and count
    ipSrc  += tPktsz;
    ipDst  += tPktsz;
    iCount -= tPktsz;
    // Wrap
    if ( ipDst >= ( void* )bp ) ipDst = ( void* )tlbp;
  }
  return( ipDst );
}

/* send data */
u8 eth_send( ethlist_t *bl ) {
  u16 bufsReq, temp = 0;
  u8 * pdst;
  u16 phy_data;
  u16 * pstat;
  ethlist_t *nl;

  // Clear errors, if any
  read_phy( PHY_SREG, &phy_data );

  // Calculate total size
  nl = bl;
  while( nl ) {
    temp += nl->size;
    nl = nl->link;
  }

  // Make sure we have enough buffers
  bufsReq = ( temp + DATA_OS + buf_size - 1 ) >> buf_size_shift;
  /*
	if( bufsReq > txBufsLeft ) {
    return( 0 );
  }
	*/

  // Initialize descriptor
  if( !init_txhdr( temp ) ) {
    return( 0 );
  }

  // Track buffer count
  txBufsLeft -= bufsReq;

  // Send data list
  pdst = (u8*)twp + DATA_OS;
  while( bl ) {
    if( bl->size ) pdst = memcpy_tx( pdst, bl->ptr, bl->size );
    bl = bl->link;
  }

  // Take ownership
  if( txBufsLeft > 0 ) twp->np->stat = HOST_OWNS;

  // Next buffer...
  pstat = &twp->stat;
  twp = twp->np;

  // Give ownership - initiates transmission
  *pstat = EMAC_OWNS;

	return( 1 );
}

/* kill phy */
void eth_disable( void ) {
  // power down the phy
  write_phy( PHY_CREG, ( PHY_POWER_DOWN ) );
  // disable emac int
  EMAC_IEN = 0;
  // reset emac
  EMAC_RST = SRST | HRTFN | HRRFN | HRTMC | HRRMC | HRMGT;
}