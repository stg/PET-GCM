/*********************************************************
Copyright (c) CMX Systems, Inc. 2007. All rights reserved
*********************************************************/

#include "gen_defines.h"
#include <ez80f91.h>
#ifndef F91ETHER_H_INC
#define F91ETHER_H_INC  1

#define EMAC_RAM_SIZE                  0x2000
#define EMAC_RAM_OFFSET                0xC000

/* descriptor buffer sizes */
#define BUF256                         0x00
#define BUF128                         0x01
#define BUF64                          0x02
#define BUF32                          0x03

/* buffer mask */
#define BUFMSK256                      0xFF00
#define BUFMSK128                      0xFF80
#define BUFMSK64                       0xFFC0
#define BUFMSK32                       0xFFE0

#define ABITS                          13

#define TBL_INIT                       0x010b00

#define DATA_OS                        (sizeof(struct desctbl))

#define MAXDATASIZE                    1500
#define HEADERSIZE                     14
#define CRCSIZE                        4
#define MAXFRAMESIZE                   (MAXDATASIZE + HEADERSIZE + CRCSIZE)
#define MINFRAMESIZE                   64

#define MIN_NUM_RX_BUFS                2

/* tx statusH bit definition */
#define EMAC_OWNS                      0x8000   /* 1=mac owns */
#define HOST_OWNS                      0x0000   /* 0=host owns */
#define TxOK                           0x0000   /* pkt transmitted ok */
#define TxAbort                        0x4000   /* pkt aborted */
#define TxBPA                          0x2000   /* back pressure applied */
#define TxHuge                         0x1000   /* pktsize > maxf */
#define TxLOOR                         0x0800   /* length out of range */
#define TxLCError                      0x0400   /* length check error */
#define TxCrcError                     0x0200   /* crc error */
#define TxPktDeferred                  0x0100   /* pkt was deferred */

/* tx statusL bit definition */
#define TxXsdfr                        0x0080   /* excessive defer */
#define TxFifoUnderRun                 0x0040   /* fifo under run error */
#define TxLateCol                      0x0020   /* late collision */
#define TxMaxCol                       0x0010   /* max # collisions */
#define TxNumColMask                   0x000f   /* # collisions */

/* rx statusH bit definition */
#define RxOK                           0x8000   /* pkt received ok */
#define RxAlignError                   0x4000   /* checks for an even # of nibbles */
#define RxCrcError                     0x2000   /* checks for CRC# == FCS# */
#define RxLongEvent                    0x1000   /* long event or dropped event */
#define RxPCF                          0x0800   /* pause control frame */
#define RxCF                           0x0400   /* control frame */
#define RxMCPkt                        0x0200   /* multicast pkt */
#define RxBCPkt                        0x0100   /* broadcast pkt */

/* rx statusL bit definition */
#define RxVLAN                         0x0080   /* vlan frame type */
#define RxUOpCode                      0x0040   /* unsupported opcode */
#define RxLOOR                         0x0020   /* length out of range */
#define RxLCError                      0x0010   /* length check error */
#define RxCodeV                        0x0008   /* receive code violation */
#define RxCEvent                       0x0004   /* carrier event previously seen */
#define RxDvEvent                      0x0002   /* RXDV event previously seen */
#define RxOVR                          0x0001   /* rx fifo overrun */


/* emac interrupt status */
#define TXDONE                         0x0001
#define TXCFDONE                       0x0002
#define RXOVRRUN                       0x0004
#define RXDONE                         0x0008
#define RXPCF                          0x0010
#define RXCFDONE                       0x0020
#define MIIMGTDONE                     0x0040
#define TXFSMERR                       0x0080

/* emac interrupt enable */
#define TXDONEIEN                      0x0001
#define RXOVRRUNIEN                    0x0004
#define RXDONEIEN                      0x0008
#define TXFSMERRIEN                    0x0080

/* MII Management */
#define LCTLD                          0x0080
#define RSTAT                          0x0040
#define CLKDIV20                       0x0006

/* MII status */
#define MGTBUSY                        0x0080

/* emac reset */
#define SRST                           0x0020
#define HRTFN                          0x0010
#define HRRFN                          0x0008
#define HRTMC                          0x0004
#define HRRMC                          0x0002
#define HRMGT                          0x0001

/* RAM control register */
#define GPRAM_EN                       0x0080
#define ERAM_EN                        0x0040

/* emac cfg 1 */
#define PADEN                          0x0080
#define VLPAD                          0x0020
#define CRCEN                          0x0010
#define FULLD                          0x0008

/* emac cfg 4 */
#define RXEN                           0x0001
#define PARF                           0x0010

/* emac afr */
#define BROADCAST                      0x0001
#define QMC                            0x0002
#define MC                             0x0004
#define PROM                           0x0008

#define PEMAC_NOT_FOUND                -1
#define PEMAC_PHY_NOT_FOUND            -2
#define PEMAC_PHYREAD_ERROR            -3
#define PEMAC_PHYINIT_FAILED           -4
#define PEMAC_SMEM_FAIL                -5

#define OUT_OF_BUFS                    -6
#define TX_FULLBUF                     3
#define TX_WAITING                     2
#define TX_DONE                        1
#define PKTTOOBIG                      -7


typedef struct desctbl {
   struct desctbl*            np;
   u16                        pktsz;
   u16                        stat;
} DescTbl;

typedef struct                f91_emac_conf_s {
   u16                        tx_buf_size;   /* total tx buffer size in EMAC RAM */
   u16                        link_speed;    /* set Phy operation to either 10 or 100 MBPS */
   u16                        full_duplex;   /* set to 1 for full duplex */
   u16                        buf_size;      /* EMAC_Ram packet buffer size */
} F91_EMAC_CONF_T;

/*
 * Register set - there are 32 registers on the 79C874, but
 * the SW driver only uses the following subset.
 */
#define PHY_CREG                       0
#define PHY_SREG                       1
#define PHY_ID1_REG                    2
#define PHY_ID2_REG                    3
#define PHY_ANEG_ADV_REG               4
#define PHY_ANEG_PARTNER_REG           5
#define PHY_ANEG_EXPANSION_REG         6

/* PHY addresses */
#define AMD_PHY_ADDRESS                0x001F

/*
 * ID register values to identify the Phy.
 * PHY_ID1 is the expected value from PHY_ID1_REG.
 * PHY_ID2 is the expected value from PHY_ID2_REG.
 */
#define AMD_PHY_ID1                    0x0022
#define AMD_PHY_ID2                    0x561B

/*
 * MII Management Control Register (Register 0) bit definitions
 */
#define PHY_RST                        0x8000
#define PHY_LOOPBACK                   0x4000
#define PHY_100BT                      0x2000
#define PHY_AUTO_NEG_ENABLE            0x1000
#define PHY_POWER_DOWN                 0x0800
#define PHY_ISOLATE                    0x0400
#define PHY_RESTART_AUTO_NEG           0x0200
#define PHY_FULLD                      0x0100
#define PHY_COLLISION_TEST             0x0080
#define PHY_10BT                       0x0000
#define PHY_HALFD                      0x0000

/*
 * MII Management Status Register (Register 1) bit definitions
 */
#define PHY_AUTO_NEG_COMPLETE          0x0020
#define PHY_REMOTE_FAULT               0x0010
#define PHY_CAN_AUTO_NEG               0x0008
#define PHY_LINK_ESTABLISHED           0x0004

/*
 * Auto-Negotiation Advertisement Register (Register 4) bit definitions
 */
#define PHY_ANEG_100_FD                0x0100
#define PHY_ANEG_100_HD                0x0080
#define PHY_ANEG_10_FD                 0x0040
#define PHY_ANEG_10_HD                 0x0020
#define PHY_ANEG_802_3                 0x0001

/*
 * Auto-Negotiation Expansion Register (Register 6) bit definitions
 */
#define PHY_PARALLEL_FAULT             0x0010
#define PHY_PARTNER_ANEG_ABLE          0x0001

/* Phy registers unique to the AMD79C874. */
#define PHY_DIAG_REG                   18
#define PHY_MODE_CTRL_REG              21

/*
 * Diagnostic Register (Register 18) bit definitions
 */
#define PHY_FULL_DUPLEX                0x0800
#define PHY_100_MBPS                   0x0400
#define PHY_RX_PASS                    0x0200
#define PHY_RX_LOCK                    0x0100

/*
 * Mode Control Register (Register 21) bit definitions
 */
#define PHY_GPSI_EN                    0x0800

/*
 * 100BaseTx PHY Control Register (Register 31) bit definitions
 */
#define PHY_ANEG_COMPLETE              0x0080
#define PHY_MII_ISOLATE                0x001C
#define PHY_MODE_MASK                  0x001C
#define PHY_MODE_100_FD                0x0018
#define PHY_MODE_10_FD                 0x0014
#define PHY_MODE_DEFAULT               0x000C
#define PHY_MODE_100_HD                0x0008
#define PHY_MODE_10_HD                 0x0004

#endif      /* F91ETHER_H_INC */
