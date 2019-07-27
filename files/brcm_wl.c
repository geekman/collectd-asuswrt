/*
 * brcm_wl.c
 * Broadcom wireless clients stats plugin for collectd
 *
 * Copyright (C) 2019 Darell Tan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "collectd.h"
#include "plugin.h"
#include "utils/common/common.h"

#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include <net/if.h>

#include "wl_ioctl.h"


#define PLUGIN_NAME "wireless_stations"

#define IOCTL_BUF_SIZE 8192

// 64 interfaces ought to be enough for everyone
static char *interfaces[64] = {NULL};

static bool secondsResolution   = true;
static bool normalizeTimestamps = true;

static bool warnedStructSize = false;

static const char *config_keys[] = {
	"Interface",
	"SecondsResolution",
	"NormalizeTimestamps",
};

int sockfd = -1;
void *ioctl_buf = NULL;

static int init_sock() {
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		ERROR("cant open sock: %s", STRERRNO);
	} else {
		ioctl_buf = malloc(IOCTL_BUF_SIZE);
		if (ioctl_buf == NULL) {
			close(sockfd);
			sockfd = -1;
		}
	}

	return sockfd;
}

static void close_sock() {
	if (sockfd >= 0) {
		close(sockfd);
		sockfd = -1;
	}

	if (ioctl_buf != NULL) {
		free(ioctl_buf);
		ioctl_buf = NULL;
	}
}

static int wl_ioctl(const char *ifname, uint32_t cmd, uint32_t len) {
	struct ifreq ifr;
	wl_ioctl_t ioc;
	int r;

	if (sockfd < 0) 
		return -1;

	ioc.cmd = cmd;
	ioc.buf = ioctl_buf;
	ioc.len = len;
	ioc.set = 0;	// false

	strcpy(ifr.ifr_name, ifname);
	ifr.ifr_data = (char *) &ioc;

	r = ioctl(sockfd, SIOCDEVPRIVATE, &ifr);
	return r;
}

static void *_memdup(void *src, size_t len) {
	void *dst = malloc(len);
	if (dst == NULL)
		return NULL;

	return memcpy(dst, src, len);
}

static int brcm_wl_config(const char *key, const char *value) {
	if (strcasecmp(key, "Interface") == 0) {
		int i;
		for (i = 0; i < STATIC_ARRAY_SIZE(interfaces); i++) {
			if (interfaces[i] == NULL) {
				interfaces[i] = sstrdup(value);
				return 0;
			}
		}
		ERROR("cant add more interfaces");
	} else if (strcasecmp(key, "SecondsResolution") == 0) {
		secondsResolution = IS_TRUE(value) ? true : false;
		return 0;
	} else if (strcasecmp(key, "NormalizeTimestamps") == 0) {
		normalizeTimestamps = IS_TRUE(value) ? true : false;
		return 0;
	}

	return -1;
}

static inline void handleTimestamp(cdtime_t time, value_list_t *vl) {
	vl->time = normalizeTimestamps ? time : cdtime();
	if (secondsResolution) {
		vl->time >>= 30;
		vl->time <<= 30;
	}
}

static void brcm_wl_submit_stats(cdtime_t time, const char *plugin_instance, 
		const char *type_instance, sta_info_t *info) {
	value_list_t vl = VALUE_LIST_INIT;

	value_t values[] = {
		{.gauge = info->in},
		{.gauge = info->idle},

		{.gauge = info->rssi[0]},
		{.gauge = info->rssi[1]},
		{.gauge = info->rssi[2]},
		{.gauge = info->rssi[3]},
	};

	handleTimestamp(time, &vl);

	vl.values = values;
	vl.values_len = STATIC_ARRAY_SIZE(values);

	sstrncpy(vl.plugin, PLUGIN_NAME, sizeof(vl.plugin));
	sstrncpy(vl.plugin_instance, plugin_instance, sizeof(vl.plugin_instance));
	sstrncpy(vl.type, "wl_stats", sizeof(vl.type));
	sstrncpy(vl.type_instance, type_instance, sizeof(vl.type_instance));

	plugin_dispatch_values(&vl);
}

static void brcm_wl_submit_traffic(cdtime_t time, const char *plugin_instance, 
		const char *type_instance, sta_info_t *info) {
	value_list_t vl = VALUE_LIST_INIT;
	value_t values[] = {
		{.gauge = info->tx_pkts},
		{.gauge = info->tx_ucast_bytes},
		{.gauge = info->tx_mcast_pkts},
		{.gauge = info->tx_mcast_bytes},

		{.gauge = info->rx_ucast_pkts},
		{.gauge = info->rx_ucast_bytes},
		{.gauge = info->rx_mcast_pkts},
		{.gauge = info->rx_mcast_bytes},

		{.gauge = info->rx_decrypt_failures},
	};

	handleTimestamp(time, &vl);

	vl.values = values;
	vl.values_len = STATIC_ARRAY_SIZE(values);

	sstrncpy(vl.plugin, PLUGIN_NAME, sizeof(vl.plugin));
	sstrncpy(vl.plugin_instance, plugin_instance, sizeof(vl.plugin_instance));
	sstrncpy(vl.type, "wl_traffic", sizeof(vl.type));
	sstrncpy(vl.type_instance, type_instance, sizeof(vl.type_instance));

	plugin_dispatch_values(&vl);
}

static sta_info_t *get_sta_info(const char *ifname, struct ether_addr *sta_addr) {
	int r;

	// format request
	strcpy(ioctl_buf, "sta_info");
	memcpy(ioctl_buf + strlen("sta_info") + 1, sta_addr->ether_addr_octet, 
			sizeof(sta_addr->ether_addr_octet));

	r = wl_ioctl(ifname, WLC_GET_VAR, IOCTL_BUF_SIZE);
	if (r < 0) {
		ERROR("cant get sta_info: %s", STRERRNO);
		return NULL;
	}

	sta_info_t *info = (sta_info_t *) ioctl_buf;

	// warn user about differing struct sizes
	if (info->len != sizeof(sta_info_t) && !warnedStructSize) {
		WARNING("unexpected sta_info_t size. wanted %d got %d",
				sizeof(sta_info_t), info->len);
		warnedStructSize = true;
	}

	if (info->len >= sizeof(sta_info_t)) {
		sta_info_t *new_info = _memdup(info, sizeof(sta_info_t));
		if (new_info == NULL)
			ERROR("cant dup sta_info_t");
		return new_info;
	}

	return NULL;
}

static maclist_t *get_assoclist(const char *ifname) {
	int max_count = (IOCTL_BUF_SIZE - sizeof(int)) / sizeof(struct ether_addr);

	((maclist_t *) ioctl_buf)->count = max_count;
	int r = wl_ioctl(ifname, WLC_GET_ASSOCLIST, IOCTL_BUF_SIZE);
	if (r < 0) {
		ERROR("cant get associated clients: %s", STRERRNO);
	} else {
		maclist_t *list = (maclist_t *) ioctl_buf;
		list = _memdup(ioctl_buf, list->count * sizeof(struct ether_addr) + sizeof(uint32_t));
		if (list == NULL) {
			ERROR("cant dup assoc list");
			return NULL;
		}

		return list;
	}

	return NULL;
}

static int process_assoclist(const char *iface) {
	int processed = 0;
	char mac_str[12+1];

	maclist_t *assoclist = get_assoclist(iface);
	if (assoclist == NULL) {
		return -1;
	}

	cdtime_t now = cdtime();

	if (assoclist->count > 0)
		INFO("processing %d clients on %s...", assoclist->count, iface);

	for (int i = 0; i < assoclist->count; i++) {
		struct ether_addr *addr = &assoclist->addr[i];
		// convert MAC address to string
		snprintf(mac_str, 12+1, "%02x%02x%02x%02x%02x%02x", 
				addr->ether_addr_octet[0],
				addr->ether_addr_octet[1],
				addr->ether_addr_octet[2],
				addr->ether_addr_octet[3],
				addr->ether_addr_octet[4],
				addr->ether_addr_octet[5]);

		sta_info_t *info = get_sta_info(iface, addr);
		if (info == NULL)
			continue;

		INFO("submitting stats for %s", mac_str);
		brcm_wl_submit_stats(now, iface, mac_str, info);
		brcm_wl_submit_traffic(now, iface, mac_str, info);

		free(info);
		processed++;
	}

	free(assoclist);

	return processed;
}

static int brcm_wl_read(void) {
	if (init_sock() < 0) {
		return -1;
	}

	int i, processed = 0;
	for (i = 0; i < STATIC_ARRAY_SIZE(interfaces); i++) {
		char *iface = interfaces[i];
		if (iface == NULL)
			break;

		int r = wl_ioctl(iface, WLC_GET_MAGIC, sizeof(uint32_t));
		if (r < 0 || * (uint32_t *) ioctl_buf != WLC_IOCTL_MAGIC) {
			ERROR("interface %s is not a brcm_wl dev", iface);
			continue;
		}

		if (process_assoclist(iface) < 0) {
			ERROR("errror processing clients for %s", iface);
			continue;
		}

		processed++;
	}

	close_sock();

	// could not query any interface, this will throttle us
	if (processed == 0)
		return -1;

	return 0;
}

void module_register(void) {
	plugin_register_config("brcm_wl", brcm_wl_config, 
			config_keys, STATIC_ARRAY_SIZE(config_keys));
	plugin_register_read("brcm_wl", brcm_wl_read);
}

