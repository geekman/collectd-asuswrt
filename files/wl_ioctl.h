//
// Broadcom ioctl defines
// https://github.com/RMerl/asuswrt-merlin.ng/blob/master/release/src-rt-6.x.4708/include/wlioctl.h
//

#ifndef __WLIOCTL_H__
#define __WLIOCTL_H__

#include <stdint.h>

#define WLC_IOCTL_MAGIC 0x14e46c77

// commands
#define WLC_GET_MAGIC		0
#define WLC_GET_VERSION		1

#define WLC_GET_ASSOCLIST	159
#define WLC_GET_VAR		262

// ioctl packet for driver communication
typedef struct wl_ioctl {
	uint32_t cmd;
	void     *buf;
	uint32_t len;
	uint8_t  set;
	uint32_t used;
	uint32_t needed;
} wl_ioctl_t;


// a list of MAC addresses
typedef struct maclist {
	uint32_t count;
	struct ether_addr addr[];
} maclist_t;


#define WL_MAXRATES_IN_SET		16	/* max # of rates in a rateset */
typedef struct wl_rateset {
	uint32_t	count;			/* # rates in this set */
	uint8_t		rates[WL_MAXRATES_IN_SET];	/* rates in 500kbps units w/hi bit set if basic */
} wl_rateset_t;


#define WL_STA_ANT_MAX		4		/* max possible rx antennas */

typedef struct {
	uint16_t	ver;		/* version of this struct */
	uint16_t	len;		/* length in bytes of this structure */
	uint16_t	cap;		/* sta's advertised capabilities */
	uint32_t	flags;		/* flags defined below */
	uint32_t	idle;		/* time since data pkt rx'd from sta */
	struct ether_addr ea;		/* Station address */
	wl_rateset_t	rateset;	/* rateset in use */
	uint32_t	in;		/* seconds elapsed since associated */
	uint32_t	listen_interval_inms; /* Min Listen interval in ms for this STA */
	uint32_t	tx_pkts;	/* # of packets transmitted */
	uint32_t	tx_failures;	/* # of packets failed */
	uint32_t	rx_ucast_pkts;	/* # of unicast packets received */
	uint32_t	rx_mcast_pkts;	/* # of multicast packets received */
	uint32_t	tx_rate;	/* Rate of last successful tx frame */
	uint32_t	rx_rate;	/* Rate of last successful rx frame */
	uint32_t	rx_decrypt_succeeds;	/* # of packet decrypted successfully */
	uint32_t	rx_decrypt_failures;	/* # of packet decrypted unsuccessfully */
	uint32_t	tx_tot_pkts;	/* # of tx pkts (ucast + mcast) */
	uint32_t	rx_tot_pkts;	/* # of data packets recvd (uni + mcast) */
	uint32_t	tx_mcast_pkts;	/* # of mcast pkts txed */
	uint64_t	tx_tot_bytes;	/* data bytes txed (ucast + mcast) */
	uint64_t	rx_tot_bytes;	/* data bytes recvd (ucast + mcast) */
	uint64_t	tx_ucast_bytes;	/* data bytes txed (ucast) */
	uint64_t	tx_mcast_bytes;	/* # data bytes txed (mcast) */
	uint64_t	rx_ucast_bytes;	/* data bytes recvd (ucast) */
	uint64_t	rx_mcast_bytes;	/* data bytes recvd (mcast) */
	int8_t  	rssi[WL_STA_ANT_MAX];	/* average rssi per antenna of data
						 * frames
						 */
	int8_t		nf[WL_STA_ANT_MAX];	/* per antenna noise floor */
	uint16_t	aid;			/* association ID */
	uint16_t	ht_capabilities;	/* advertised ht caps */
	uint16_t	vht_flags;		/* converted vht flags */
	uint32_t	tx_pkts_retried;		/* # of frames where a retry was
				                 *   necessary
				                 */
	uint32_t	tx_pkts_retry_exhausted; /* # of frames where a retry was
						  * exhausted
						  */
	int8_t		rx_lastpkt_rssi[WL_STA_ANT_MAX]; /* Per antenna RSSI of last
					                  * received data frame
					                  */
} sta_info_t;


#endif/*!__WLIOCTL_H__*/

