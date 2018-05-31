// SPDX-License-Identifier: GPL-2.0
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/list.h>
#include <linux/workqueue.h>
#include "host_interface.h"
#include <linux/spinlock.h>
#include <linux/errno.h>
#include "coreconfigurator.h"
#include "wilc_wlan.h"
#include "linux_wlan.h"
#include "wilc_wlan_if.h"
#include <linux/etherdevice.h>
#include <linux/version.h>
#include "wilc_wfi_netdevice.h"

#define HOST_IF_MSG_SCAN                        0
#define HOST_IF_MSG_CONNECT                     1
#define HOST_IF_MSG_RCVD_GNRL_ASYNC_INFO        2
#define HOST_IF_MSG_KEY                         3
#define HOST_IF_MSG_RCVD_NTWRK_INFO             4
#define HOST_IF_MSG_RCVD_SCAN_COMPLETE          5
#define HOST_IF_MSG_CFG_PARAMS                  6
#define HOST_IF_MSG_SET_CHANNEL                 7
#define HOST_IF_MSG_DISCONNECT                  8
#define HOST_IF_MSG_GET_RSSI                    9
#define HOST_IF_MSG_ADD_BEACON                  11
#define HOST_IF_MSG_DEL_BEACON                  12
#define HOST_IF_MSG_ADD_STATION                 13
#define HOST_IF_MSG_DEL_STATION                 14
#define HOST_IF_MSG_EDIT_STATION                15
#define HOST_IF_MSG_SCAN_TIMER_FIRED            16
#define HOST_IF_MSG_CONNECT_TIMER_FIRED         17
#define HOST_IF_MSG_POWER_MGMT                  18
#define HOST_IF_MSG_GET_INACTIVETIME            19
#define HOST_IF_MSG_REMAIN_ON_CHAN              20
#define HOST_IF_MSG_REGISTER_FRAME              21
#define HOST_IF_MSG_LISTEN_TIMER_FIRED          22
#define HOST_IF_MSG_SET_WFIDRV_HANDLER          24
#define HOST_IF_MSG_SET_MAC_ADDRESS             25
#define HOST_IF_MSG_GET_MAC_ADDRESS             26
#define HOST_IF_MSG_SET_OPERATION_MODE          27
#define HOST_IF_MSG_GET_STATISTICS              31
#define HOST_IF_MSG_SET_MULTICAST_FILTER        32
#define HOST_IF_MSG_DEL_ALL_STA                 36
#define HOST_IF_MSG_SET_TX_POWER		38
#define HOST_IF_MSG_GET_TX_POWER		39
#define HOST_IF_MSG_SET_ANTENNA_MODE		40
#define HOST_IF_MSG_SEND_BUFFERED_EAP		41
#define HOST_IF_MSG_SET_WOWLAN_TRIGGER 		43
#define HOST_IF_MSG_EXIT                        100

#define HOST_IF_SCAN_TIMEOUT                    4000
#define HOST_IF_CONNECT_TIMEOUT                 9500

#define FALSE_FRMWR_CHANNEL			100

#define TCP_ACK_FILTER_LINK_SPEED_THRESH	54
#define DEFAULT_LINK_SPEED			72

/* Generic success will return 0 */
#define WILC_SUCCESS 		0	/* Generic success */

/* Negative numbers to indicate failures */
#define	WILC_FAIL		-100	/* Generic Fail */		
#define	WILC_BUSY		-101	/* Busy with another operation*/
#define	WILC_INVALID_ARGUMENT	-102	/* A given argument is invalid*/
#define	WILC_INVALID_STATE	-103	/* An API request would violate the Driver state machine (i.e. to start PID while not camped)*/
#define	WILC_BUFFER_OVERFLOW	-104	/* In copy operations if the copied data is larger than the allocated buffer*/
#define WILC_NULL_PTR		-105	/* null pointer is passed or used */
#define	WILC_EMPTY		-107
#define WILC_FULL		-108
#define	WILC_TIMEOUT		-109
#define WILC_CANCELED		-110	/* The required operation have been canceled by the user*/
#define WILC_INVALID_FILE	-112	/* The Loaded file is corruped or having an invalid format */
#define WILC_NOT_FOUND		-113	/* Cant find the file to load */
#define WILC_NO_MEM		-114
#define WILC_UNSUPPORTED_VERSION -115
#define WILC_FILE_EOF		-116

struct host_if_wpa_attr {
	u8 *key;
	const u8 *mac_addr;
	u8 *seq;
	u8 seq_len;
	u8 index;
	u8 key_len;
	u8 mode;
};

struct host_if_wep_attr {
	u8 *key;
	u8 key_len;
	u8 index;
	u8 mode;
	enum AUTHTYPE auth_type;
};

union host_if_key_attr {
	struct host_if_wep_attr wep;
	struct host_if_wpa_attr wpa;
	struct host_if_pmkid_attr pmkid;
};

struct key_attr {
	enum KEY_TYPE type;
	u8 action;
	union host_if_key_attr attr;
};

struct send_buffered_eap {
	wilc_frmw_to_linux_t frmw_to_linux;
	free_eap_buf_param eap_buf_param;
	u8 *buff;
	unsigned int size;
	unsigned int pkt_offset;
	void *user_arg;
};

extern void filter_shadow_scan(void* pUserVoid, u8 *ch_freq_list,
							   u8 ch_list_len);

signed int wilc_send_buffered_eap(struct wilc_vif *vif,
				  wilc_frmw_to_linux_t frmw_to_linux,
				  free_eap_buf_param eap_buf_param,
				  u8 *buff, unsigned int size,
				  unsigned int pkt_offset,
				  void *user_arg);

struct scan_attr {
	u8 src;
	u8 type;
	u8 *ch_freq_list;
	u8 ch_list_len;
	u8 *ies;
	size_t ies_len;
	wilc_scan_result result;
	void *arg;
	struct hidden_network hidden_network;
};

struct connect_attr {
	u8 *bssid;
	u8 *ssid;
	size_t ssid_len;
	u8 *ies;
	size_t ies_len;
	u8 security;
	wilc_connect_result result;
	void *arg;
	enum AUTHTYPE auth_type;
	u8 ch;
	void *params;
};

struct rcvd_async_info {
	u8 *buffer;
	u32 len;
};

struct channel_attr {
	u8 set_ch;
};

struct beacon_attr {
	u32 interval;
	u32 dtim_period;
	u32 head_len;
	u8 *head;
	u32 tail_len;
	u8 *tail;
};

struct set_multicast {
	bool enabled;
	u32 cnt;
};

struct del_all_sta {
	u8 del_all_sta[MAX_NUM_STA][ETH_ALEN];
	u8 assoc_sta;
};

struct del_sta {
	u8 mac_addr[ETH_ALEN];
};

struct power_mgmt_param {
	bool enabled;
	u32 timeout;
};

struct sta_inactive_t {
	u8 mac[6];
};

struct host_if_wowlan_trigger {
	u8 wowlan_trigger;
};

struct tx_power {
	u8 tx_pwr;
};

struct bt_coex_mode {
	u8 bt_coex;
};

struct host_if_set_ant {
	u8 mode;
	u8 antenna1;
	u8 antenna2;
	u8 gpio_mode;
};

union message_body {
	struct scan_attr scan_info;
	struct connect_attr con_info;
	struct rcvd_net_info net_info;
	struct rcvd_async_info async_info;
	struct key_attr key_info;
	struct cfg_param_attr cfg_info;
	struct channel_attr channel_info;
	struct beacon_attr beacon_info;
	struct add_sta_param add_sta_info;
	struct del_sta del_sta_info;
	struct add_sta_param edit_sta_info;
	struct power_mgmt_param pwr_mgmt_info;
	struct sta_inactive_t mac_info;
	struct drv_handler drv;
	struct set_multicast multicast_info;
	struct op_mode mode;
	struct dev_mac_addr dev_mac_info;
	struct ba_session_info session_info;
	struct remain_ch remain_on_ch;
	struct reg_frame reg_frame;
	char *data;
	struct del_all_sta del_all_sta_info;
	struct send_buffered_eap send_buff_eap;
	struct tx_power tx_power;
	struct host_if_set_ant set_ant;
	struct host_if_wowlan_trigger wow_trigger;
	struct bt_coex_mode bt_coex_mode;
};

struct host_if_msg {
	u16 id;
	union message_body body;
	struct wilc_vif *vif;
	struct work_struct work;
};

struct join_bss_param {
	enum bss_types bss_type;
	u8 dtim_period;
	u16 beacon_period;
	u16 cap_info;
	u8 bssid[6];
	char ssid[MAX_SSID_LEN];
	u8 ssid_len;
	u8 supp_rates[MAX_RATES_SUPPORTED + 1];
	u8 ht_capable;
	u8 wmm_cap;
	u8 uapsd_cap;
	bool rsn_found;
	u8 rsn_grp_policy;
	u8 mode_802_11i;
	u8 rsn_pcip_policy[3];
	u8 rsn_auth_policy[3];
	u8 rsn_cap[2];
	u32 tsf;
	u8 noa_enabled;
	u8 opp_enabled;
	u8 ct_window;
	u8 cnt;
	u8 idx;
	u8 duration[4];
	u8 interval[4];
	u8 start_time[4];
};

static struct host_if_drv *terminated_handle;
#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
bool wilc_optaining_ip;
#endif
static struct workqueue_struct *hif_workqueue;
static struct completion hif_thread_comp;
static struct completion hif_driver_comp;
static struct completion hif_wait_response;
static struct mutex hif_deinit_lock;
static struct timer_list periodic_rssi;
static struct wilc_vif *periodic_rssi_vif;

u8 wilc_multicast_mac_addr_list[WILC_MULTICAST_TABLE_SIZE][ETH_ALEN];

static u8 rcv_assoc_resp[MAX_ASSOC_RESP_FRAME_SIZE];

static s8 rssi;
static u32 inactive_time;
static u8 del_beacon;
static u32 clients_count;

extern int recovery_on;

#define REAL_JOIN_REQ 0
#define FLUSHED_JOIN_REQ 1
#define FLUSHED_BYTE_POS 79

static void *host_int_parse_join_bss_param(struct network_info *info);
static void host_if_work(struct work_struct *work);

/*!
 *  @author		syounan
 *  @date		1 Sep 2010
 *  @note		copied from FLO glue implementatuion
 *  @version		1.0
 */
static int wilc_enqueue_cmd(struct host_if_msg *msg)
{
	struct host_if_msg *new_msg;

	new_msg = kmemdup(msg, sizeof(*new_msg), GFP_ATOMIC);
	if (!new_msg)
		return -ENOMEM;

	INIT_WORK(&new_msg->work, host_if_work);

	if(!hif_workqueue)
		return -EFAULT;

	queue_work(hif_workqueue, &new_msg->work);
	return 0;
}

/* The u8IfIdx starts from 0 to NUM_CONCURRENT_IFC -1, but 0 index used as
 * special purpose in wilc device, so we add 1 to the index to starts from 1.
 * As a result, the returned index will be 1 to NUM_CONCURRENT_IFC.
 */
int wilc_get_vif_idx(struct wilc_vif *vif)
{
	return vif->idx + 1;
}

/* We need to minus 1 from idx which is from wilc device to get real index
 * of wilc->vif[], because we add 1 when pass to wilc device in the function
 * wilc_get_vif_idx.
 * As a result, the index should be between 0 and NUM_CONCURRENT_IFC -1.
 */
static struct wilc_vif *wilc_get_vif_from_idx(struct wilc *wilc, int idx)
{
	int index = idx - 1;

	if (index < 0 || index >= NUM_CONCURRENT_IFC)
		return NULL;

	return wilc->vif[index];
}

static int handle_send_buffered_eap(struct wilc_vif *vif,
				    struct send_buffered_eap *hif_buff_eap)
{
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Sending bufferd eapol to WPAS\n");
	if (!hif_buff_eap->buff)
		return -EINVAL;

	if (hif_buff_eap->frmw_to_linux)
		hif_buff_eap->frmw_to_linux(vif->wilc, hif_buff_eap->buff,
					    hif_buff_eap->size,
					    hif_buff_eap->pkt_offset,
					    PKT_STATUS_BUFFERED);
	if (hif_buff_eap->eap_buf_param)
		hif_buff_eap->eap_buf_param(hif_buff_eap->user_arg);

	if(hif_buff_eap->buff != NULL){
		kfree(hif_buff_eap->buff);
		hif_buff_eap->buff = NULL;
	}

	return 0;
}

static void handle_set_channel(struct wilc_vif *vif,
			       struct channel_attr *hif_set_ch)
{
	int ret = 0;
	struct wid wid;

	wid.id = (u16)WID_CURRENT_CHANNEL;
	wid.type = WID_CHAR;
	wid.val = (char *)&hif_set_ch->set_ch;
	wid.size = sizeof(char);

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));

	if (ret)
		PRINT_ER(vif->ndev, "Failed to set channel\n");
}

static int handle_set_wfi_drv_handler(struct wilc_vif *vif,
				      struct drv_handler *hif_drv_handler)
{
	int ret = 0;
	struct wid wid;
	u8 *currbyte, *buffer;
	struct host_if_drv *hif_drv = NULL;

	if (!vif->hif_drv)
		return -EINVAL;

	if (!hif_drv_handler)
		return -EINVAL;

	hif_drv	= vif->hif_drv;

	buffer = kzalloc(DRV_HANDLER_SIZE, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	currbyte = buffer;
	*currbyte = hif_drv_handler->handler & DRV_HANDLER_MASK;
	currbyte++;
	*currbyte = (u32)0 & DRV_HANDLER_MASK;
	currbyte++;
	*currbyte = (u32)0 & DRV_HANDLER_MASK;
	currbyte++;
	*currbyte = (u32)0 & DRV_HANDLER_MASK;
	currbyte++;
	*currbyte = (hif_drv_handler->ifc_id | (hif_drv_handler->mode << 1));

	wid.id = (u16)WID_SET_DRV_HANDLER;
	wid.type = WID_STR;
	wid.val = (s8 *)buffer;
	wid.size = DRV_HANDLER_SIZE;

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   hif_drv->driver_handler_id);

	complete(&hif_driver_comp);

	if (ret)
		PRINT_ER(vif->ndev, "Failed to set driver handler\n");
	kfree(buffer);
	return ret;
}

static void handle_set_operation_mode(struct wilc_vif *vif,
				      struct op_mode *hif_op_mode)
{
	int ret = 0;
	struct wid wid;

	wid.id = (u16)WID_SET_OPERATION_MODE;
	wid.type = WID_INT;
	wid.val = (s8 *)&hif_op_mode->mode;
	wid.size = sizeof(u32);

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));

	if (hif_op_mode->mode == IDLE_MODE)
		complete(&hif_driver_comp);

	if (ret)
		PRINT_ER(vif->ndev, "Failed to set driver handler\n");
}

static void handle_get_mac_address(struct wilc_vif *vif,
				   struct dev_mac_addr *get_mac_addr)
{
	int ret = 0;
	struct wid wid;

	wid.id = (u16)WID_MAC_ADDR;
	wid.type = WID_STR;
	wid.val = get_mac_addr->mac_addr;
	wid.size = ETH_ALEN;

	ret = wilc_send_config_pkt(vif, GET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));

	if (ret)
		PRINT_ER(vif->ndev, "Failed to get mac address\n");
	complete(&hif_wait_response);
}

static void handle_set_mac_address(struct wilc_vif *vif,
				   struct dev_mac_addr *set_mac_addr)
{
	int ret;
	struct wid wid;

	wid.id = (u16)WID_MAC_ADDR;
	wid.type = WID_STR;
	wid.val = set_mac_addr->mac_addr;
	wid.size = ETH_ALEN;

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));

	if (ret)
		PRINT_ER(vif->ndev, "Failed to set mac address\n");
	complete(&hif_wait_response);
}

static void handle_cfg_param(struct wilc_vif *vif, struct cfg_param_attr *param)
{
	int ret = 0;
	struct wid wid_list[32];
	struct host_if_drv *hif_drv = vif->hif_drv;
	int i = 0;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setting CFG params\n");
	
	mutex_lock(&hif_drv->cfg_values_lock);

	if (param->flag & BSS_TYPE) {
		u8 bss_type = param->bss_type;

		if (bss_type < 6) {
			wid_list[i].id = WID_BSS_TYPE;
			wid_list[i].val = (s8 *)&param->bss_type;
			wid_list[i].type = WID_CHAR;
			wid_list[i].size = sizeof(char);
			hif_drv->cfg_values.bss_type = bss_type;
		} else {
			PRINT_ER(vif->ndev, "check value 6 over\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & AUTH_TYPE) {
		u8 auth_type = param->auth_type;

		if (auth_type == 1 || auth_type == 2 || auth_type == 5) {
			wid_list[i].id = WID_AUTH_TYPE;
			wid_list[i].val = (s8 *)&param->auth_type;
			wid_list[i].type = WID_CHAR;
			wid_list[i].size = sizeof(char);
			hif_drv->cfg_values.auth_type = auth_type;
		} else {
			PRINT_ER(vif->ndev, "Impossible value\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & AUTHEN_TIMEOUT) {
		if (param->auth_timeout > 0) {
			wid_list[i].id = WID_AUTH_TIMEOUT;
			wid_list[i].val = (s8 *)&param->auth_timeout;
			wid_list[i].type = WID_SHORT;
			wid_list[i].size = sizeof(u16);
			hif_drv->cfg_values.auth_timeout = param->auth_timeout;
		} else {
			PRINT_ER(vif->ndev, "Range(1 ~ 65535) over\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & POWER_MANAGEMENT) {
		u8 pm_mode = param->power_mgmt_mode;

		if (pm_mode < 5) {
			wid_list[i].id = WID_POWER_MANAGEMENT;
			wid_list[i].val = (s8 *)&param->power_mgmt_mode;
			wid_list[i].type = WID_CHAR;
			wid_list[i].size = sizeof(char);
			hif_drv->cfg_values.power_mgmt_mode = pm_mode;
		} else {
			PRINT_ER(vif->ndev, "Invalid power mode\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & RETRY_SHORT) {
		u16 retry_limit = param->short_retry_limit;

		if (retry_limit > 0 && retry_limit < 256) {
			wid_list[i].id = WID_SHORT_RETRY_LIMIT;
			wid_list[i].val = (s8 *)&param->short_retry_limit;
			wid_list[i].type = WID_SHORT;
			wid_list[i].size = sizeof(u16);
			hif_drv->cfg_values.short_retry_limit = retry_limit;
		} else {
			PRINT_ER(vif->ndev, "Range(1~256) over\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & RETRY_LONG) {
		u16 limit = param->long_retry_limit;

		if (limit > 0 && limit < 256) {
			wid_list[i].id = WID_LONG_RETRY_LIMIT;
			wid_list[i].val = (s8 *)&param->long_retry_limit;
			wid_list[i].type = WID_SHORT;
			wid_list[i].size = sizeof(u16);
			hif_drv->cfg_values.long_retry_limit = limit;
		} else {
			PRINT_ER(vif->ndev, "Range(1~256) over\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & FRAG_THRESHOLD) {
		u16 frag_th = param->frag_threshold;

		if (frag_th > 255 && frag_th < 7937) {
			wid_list[i].id = WID_FRAG_THRESHOLD;
			wid_list[i].val = (s8 *)&param->frag_threshold;
			wid_list[i].type = WID_SHORT;
			wid_list[i].size = sizeof(u16);
			hif_drv->cfg_values.frag_threshold = frag_th;
		} else {
			PRINT_ER(vif->ndev, "Threshold Range fail\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & RTS_THRESHOLD) {
		u16 rts_th = param->rts_threshold;

		if (rts_th > 255) {
			wid_list[i].id = WID_RTS_THRESHOLD;
			wid_list[i].val = (s8 *)&param->rts_threshold;
			wid_list[i].type = WID_SHORT;
			wid_list[i].size = sizeof(u16);
			hif_drv->cfg_values.rts_threshold = rts_th;
		} else {
			PRINT_ER(vif->ndev, "Threshold Range fail\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & PREAMBLE) {
		u16 preamble_type = param->preamble_type;

		if (param->preamble_type < 3) {
			wid_list[i].id = WID_PREAMBLE;
			wid_list[i].val = (s8 *)&param->preamble_type;
			wid_list[i].type = WID_CHAR;
			wid_list[i].size = sizeof(char);
			hif_drv->cfg_values.preamble_type = preamble_type;
		} else {
			PRINT_ER(vif->ndev, "Preamle Range(0~2) over\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & SHORT_SLOT_ALLOWED) {
		u8 slot_allowed = param->short_slot_allowed;

		if (slot_allowed < 2) {
			wid_list[i].id = WID_SHORT_SLOT_ALLOWED;
			wid_list[i].val = (s8 *)&param->short_slot_allowed;
			wid_list[i].type = WID_CHAR;
			wid_list[i].size = sizeof(char);
			hif_drv->cfg_values.short_slot_allowed = slot_allowed;
		} else {
			PRINT_ER(vif->ndev, "Short slot(2) over\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & TXOP_PROT_DISABLE) {
		u8 prot_disabled = param->txop_prot_disabled;

		if (param->txop_prot_disabled < 2) {
			wid_list[i].id = WID_11N_TXOP_PROT_DISABLE;
			wid_list[i].val = (s8 *)&param->txop_prot_disabled;
			wid_list[i].type = WID_CHAR;
			wid_list[i].size = sizeof(char);
			hif_drv->cfg_values.txop_prot_disabled = prot_disabled;
		} else {
			PRINT_ER(vif->ndev, "TXOP prot disable\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & BEACON_INTERVAL) {
		u16 beacon_interval = param->beacon_interval;

		if (beacon_interval > 0) {
			wid_list[i].id = WID_BEACON_INTERVAL;
			wid_list[i].val = (s8 *)&param->beacon_interval;
			wid_list[i].type = WID_SHORT;
			wid_list[i].size = sizeof(u16);
			hif_drv->cfg_values.beacon_interval = beacon_interval;
		} else {
			PRINT_ER(vif->ndev, "Beacon interval(1~65535)fail\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & DTIM_PERIOD) {
		if (param->dtim_period > 0 && param->dtim_period < 256) {
			wid_list[i].id = WID_DTIM_PERIOD;
			wid_list[i].val = (s8 *)&param->dtim_period;
			wid_list[i].type = WID_CHAR;
			wid_list[i].size = sizeof(char);
			hif_drv->cfg_values.dtim_period = param->dtim_period;
		} else {
			PRINT_ER(vif->ndev, "DTIM range(1~255) fail\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & SITE_SURVEY) {
		enum SITESURVEY enabled = param->site_survey_enabled;

		if (enabled < 3) {
			wid_list[i].id = WID_SITE_SURVEY;
			wid_list[i].val = (s8 *)&param->site_survey_enabled;
			wid_list[i].type = WID_CHAR;
			wid_list[i].size = sizeof(char);
			hif_drv->cfg_values.site_survey_enabled = enabled;
		} else {
			PRINT_ER(vif->ndev, "Site survey disable\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & SITE_SURVEY_SCAN_TIME) {
		u16 scan_time = param->site_survey_scan_time;

		if (scan_time > 0) {
			wid_list[i].id = WID_SITE_SURVEY_SCAN_TIME;
			wid_list[i].val = (s8 *)&param->site_survey_scan_time;
			wid_list[i].type = WID_SHORT;
			wid_list[i].size = sizeof(u16);
			hif_drv->cfg_values.site_survey_scan_time = scan_time;
		} else {
			PRINT_ER(vif->ndev, "Site scan time(1~65535) over\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & ACTIVE_SCANTIME) {
		u16 active_scan_time = param->active_scan_time;

		if (active_scan_time > 0) {
			wid_list[i].id = WID_ACTIVE_SCAN_TIME;
			wid_list[i].val = (s8 *)&param->active_scan_time;
			wid_list[i].type = WID_SHORT;
			wid_list[i].size = sizeof(u16);
			hif_drv->cfg_values.active_scan_time = active_scan_time;
		} else {
			PRINT_ER(vif->ndev, "Active time(1~65535) over\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & PASSIVE_SCANTIME) {
		u16 time = param->passive_scan_time;

		if (time > 0) {
			wid_list[i].id = WID_PASSIVE_SCAN_TIME;
			wid_list[i].val = (s8 *)&param->passive_scan_time;
			wid_list[i].type = WID_SHORT;
			wid_list[i].size = sizeof(u16);
			hif_drv->cfg_values.passive_scan_time = time;
		} else {
			PRINT_ER(vif->ndev, "Passive time(1~65535) over\n");
			goto unlock;
		}
		i++;
	}
	if (param->flag & CURRENT_TX_RATE) {
		enum CURRENT_TXRATE curr_tx_rate = param->curr_tx_rate;

		if (curr_tx_rate == AUTORATE || curr_tx_rate == MBPS_1 ||
		    curr_tx_rate == MBPS_2 || curr_tx_rate == MBPS_5_5 ||
		    curr_tx_rate == MBPS_11 || curr_tx_rate == MBPS_6 ||
		    curr_tx_rate == MBPS_9 || curr_tx_rate == MBPS_12 ||
		    curr_tx_rate == MBPS_18 || curr_tx_rate == MBPS_24 ||
		    curr_tx_rate == MBPS_36 || curr_tx_rate == MBPS_48 ||
		    curr_tx_rate == MBPS_54) {
			wid_list[i].id = WID_CURRENT_TX_RATE;
			wid_list[i].val = (s8 *)&curr_tx_rate;
			wid_list[i].type = WID_SHORT;
			wid_list[i].size = sizeof(u16);
			hif_drv->cfg_values.curr_tx_rate = (u8)curr_tx_rate;
		} else {
			PRINT_ER(vif->ndev, "out of TX rate\n");
			goto unlock;
		}
		i++;
	}

	ret = wilc_send_config_pkt(vif, SET_CFG, wid_list,
				   i, wilc_get_vif_idx(vif));

	if (ret)
		PRINT_ER(vif->ndev, "Error in setting CFG params\n");

unlock:
	mutex_unlock(&hif_drv->cfg_values_lock);
}

static s32 handle_scan(struct wilc_vif *vif, struct scan_attr *scan_info)
{
	s32 result = 0;
	struct wid wid_list[5];
	u32 index = 0;
	u32 i;
	u8 *buffer;
	u8 valuesize = 0;
	u8 *hdn_ntwk_wid_val = NULL;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct hidden_network *hidden_net = &scan_info->hidden_network;
	struct host_if_drv *hif_drv_p2p  = wilc_get_drv_handler_by_ifc(vif->wilc, P2P_IFC);
	struct host_if_drv *hif_drv_wlan = wilc_get_drv_handler_by_ifc(vif->wilc, WLAN_IFC);

	PRINT_INFO(vif->ndev, HOSTINF_DBG,"Setting SCAN params\n");
	PRINT_INFO(vif->ndev, HOSTINF_DBG,"Scanning: In [%d] state \n", hif_drv->hif_state);

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "Driver is null\n");
		return -EFAULT;
	}
	hif_drv->usr_scan_req.scan_result = scan_info->result;
	hif_drv->usr_scan_req.arg = scan_info->arg;

	if (hif_drv_p2p != NULL) {
		if ((hif_drv_p2p->hif_state != HOST_IF_IDLE) &&
	    (hif_drv_p2p->hif_state != HOST_IF_CONNECTED)) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,"Don't scan. P2P_IFC is in state [%d]\n",
			 hif_drv_p2p->hif_state);
			result = -EBUSY;
			goto error;
		}
	}

	if (hif_drv_wlan != NULL) {
		if (hif_drv_wlan->hif_state != HOST_IF_IDLE &&
	    hif_drv_wlan->hif_state != HOST_IF_CONNECTED) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,"Don't scan. WLAN_IFC is in state [%d]\n",
			 hif_drv_wlan->hif_state);
			result = -EBUSY;
			goto error;
		}
	}
	if(wilc_connecting) {
		PRINT_INFO(vif->ndev, GENERIC_DBG, "[handle_scan]: Don't do scan in (CONNECTING) state\n");
		result = -EBUSY;
		goto error;
	}
#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
	if (wilc_optaining_ip) {
		PRINT_ER(vif->ndev, "Don't do obss scan\n");
		result = -EBUSY;
		goto error;
	}
#endif

	PRINT_INFO(vif->ndev, HOSTINF_DBG,"Setting SCAN params\n");
	hif_drv->usr_scan_req.rcvd_ch_cnt = 0;

	wid_list[index].id = (u16)WID_SSID_PROBE_REQ;
	wid_list[index].type = WID_STR;

	for (i = 0; i < hidden_net->n_ssids; i++)
		valuesize += ((hidden_net->net_info[i].ssid_len) + 1);
	hdn_ntwk_wid_val = kmalloc(valuesize + 1, GFP_KERNEL);
	wid_list[index].val = hdn_ntwk_wid_val;
	if (wid_list[index].val) {
		buffer = wid_list[index].val;

		*buffer++ = hidden_net->n_ssids;

		PRINT_INFO(vif->ndev, HOSTINF_DBG,"In Handle_ProbeRequest number of ssid %d\n",
			 hidden_net->n_ssids);
		for (i = 0; i < hidden_net->n_ssids; i++) {
			*buffer++ = hidden_net->net_info[i].ssid_len;
			memcpy(buffer, hidden_net->net_info[i].ssid,
			       hidden_net->net_info[i].ssid_len);
			buffer += hidden_net->net_info[i].ssid_len;
		}

		wid_list[index].size = (s32)(valuesize + 1);
		index++;
	}

	wid_list[index].id = WID_INFO_ELEMENT_PROBE;
	wid_list[index].type = WID_BIN_DATA;
	wid_list[index].val = scan_info->ies;
	wid_list[index].size = scan_info->ies_len;
	index++;

	wid_list[index].id = WID_SCAN_TYPE;
	wid_list[index].type = WID_CHAR;
	wid_list[index].size = sizeof(char);
	wid_list[index].val = (s8 *)&scan_info->type;
	index++;

	wid_list[index].id = WID_SCAN_CHANNEL_LIST;
	wid_list[index].type = WID_BIN_DATA;

	if (scan_info->ch_freq_list &&
	    scan_info->ch_list_len > 0) {
		int i;

		for (i = 0; i < scan_info->ch_list_len; i++)	{
			if (scan_info->ch_freq_list[i] > 0)
				scan_info->ch_freq_list[i] -= 1;
		}
	}

	wid_list[index].val = scan_info->ch_freq_list;
	wid_list[index].size = scan_info->ch_list_len;
	index++;

	wid_list[index].id = WID_START_SCAN_REQ;
	wid_list[index].type = WID_CHAR;
	wid_list[index].size = sizeof(char);
	wid_list[index].val = (s8 *)&scan_info->src;
	index++;

    /* 
     * Remove APs from shadow scan list which are 
     * not in the requested scan channels list 
     */
	filter_shadow_scan(vif, scan_info->ch_freq_list,
			   scan_info->ch_list_len);
	
	result = wilc_send_config_pkt(vif, SET_CFG, wid_list,
				      index,
				      wilc_get_vif_idx(vif));

	if (result)
		PRINT_ER(vif->ndev, "Failed to send scan parameters\n");

error:
	if (result) {
		del_timer(&hif_drv->scan_timer);
		handle_scan_done(vif, SCAN_EVENT_ABORTED);
	}

	kfree(scan_info->ch_freq_list);
	scan_info->ch_freq_list = NULL;

	kfree(scan_info->ies);
	scan_info->ies = NULL;
	kfree(scan_info->hidden_network.net_info);
	scan_info->hidden_network.net_info = NULL;

	kfree(hdn_ntwk_wid_val);

	return result;
}

s32 handle_scan_done(struct wilc_vif *vif, enum scan_event evt)
{
	s32 result = 0;
	u8 abort_running_scan;
	struct wid wid;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct user_scan_req *scan_req;
	u8 null_bssid[6] = {0};

	PRINT_INFO(vif->ndev, HOSTINF_DBG,"in handle_scan_done()\n");

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "Driver handler is NULL\n");
		return result;
	}

	if(evt == SCAN_EVENT_DONE){
		if (memcmp(hif_drv->assoc_bssid, null_bssid, ETH_ALEN) == 0) {
			hif_drv->hif_state = HOST_IF_IDLE;
		} else {
			hif_drv->hif_state = HOST_IF_CONNECTED;
		}
	} else if (evt == SCAN_EVENT_ABORTED) {
		PRINT_INFO(vif->ndev, GENERIC_DBG,"Abort running scan\n");		
		abort_running_scan = 1;
		wid.id = (u16)WID_ABORT_RUNNING_SCAN;
		wid.type = WID_CHAR;
		wid.val = (s8 *)&abort_running_scan;
		wid.size = sizeof(char);

		result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
					      wilc_get_vif_idx(vif));

		if (result) {
			PRINT_ER(vif->ndev, "Failed to set abort running\n");
			result = -EFAULT;
		}
	}

	scan_req = &hif_drv->usr_scan_req;
	if (scan_req->scan_result) {
		scan_req->scan_result(evt, NULL, scan_req->arg, NULL);
		scan_req->scan_result = NULL;
	}

	return result;
}

u8 wilc_connected_ssid[6] = {0};
static s32 handle_connect(struct wilc_vif *vif,
			  struct connect_attr *conn_attr)
{
	s32 result = 0;
	struct wid wid_list[8];
	u32 wid_cnt = 0, dummyval = 0;
	u8 *cur_byte = NULL;
	struct join_bss_param *bss_param;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct host_if_drv *hif_drv_p2p  = wilc_get_drv_handler_by_ifc(vif->wilc, P2P_IFC);
	struct host_if_drv *hif_drv_wlan = wilc_get_drv_handler_by_ifc(vif->wilc, WLAN_IFC);

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "Driver is null\n");
		return -EFAULT;
	}
	
	if (hif_drv_p2p != NULL) {
		if (hif_drv_p2p->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,"Don't scan. P2P_IFC is in state [%d]\n",
			 hif_drv_p2p->hif_state);
			 result = -EFAULT;
			goto error;
		}
	}
	if (hif_drv_wlan != NULL) {
		if (hif_drv_wlan->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,"Don't scan. WLAN_IFC is in state [%d]\n",
			 hif_drv_wlan->hif_state);
			result = -EFAULT;
			goto error;
		}
	}
	if (memcmp(conn_attr->bssid, wilc_connected_ssid, ETH_ALEN) == 0) {
		result = 0;
		PRINT_ER(vif->ndev, "Discard connect request\n");
		return result;
	}

	PRINT_D(vif->ndev, HOSTINF_DBG, "Saving connection parameters in global structure\n");
	bss_param = conn_attr->params;
	if (!bss_param) {
		PRINT_ER(vif->ndev, "Required BSSID not found\n");
		result = -ENOENT;
		goto error;
	}

	if (conn_attr->bssid) {
		hif_drv->usr_conn_req.bssid = kmemdup(conn_attr->bssid, 6,
						      GFP_KERNEL);
		if (!hif_drv->usr_conn_req.bssid) {
			result = -ENOMEM;
			goto error;
		}
	}

	hif_drv->usr_conn_req.ssid_len = conn_attr->ssid_len;
	if (conn_attr->ssid) {
		hif_drv->usr_conn_req.ssid = kmalloc(conn_attr->ssid_len + 1,
						     GFP_KERNEL);
		if (!hif_drv->usr_conn_req.ssid) {
			result = -ENOMEM;
			goto error;
		}
		memcpy(hif_drv->usr_conn_req.ssid,
		       conn_attr->ssid,
		       conn_attr->ssid_len);
		hif_drv->usr_conn_req.ssid[conn_attr->ssid_len] = '\0';
	}

	hif_drv->usr_conn_req.ies_len = conn_attr->ies_len;
	if (conn_attr->ies) {
		hif_drv->usr_conn_req.ies = kmemdup(conn_attr->ies,
						    conn_attr->ies_len,
						    GFP_KERNEL);
		if (!hif_drv->usr_conn_req.ies) {
			result = -ENOMEM;
			goto error;
		}
	}

	hif_drv->usr_conn_req.security = conn_attr->security;
	hif_drv->usr_conn_req.auth_type = conn_attr->auth_type;
	hif_drv->usr_conn_req.conn_result = conn_attr->result;
	hif_drv->usr_conn_req.arg = conn_attr->arg;

	wid_list[wid_cnt].id = WID_SUCCESS_FRAME_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)(&(dummyval));
	wid_cnt++;

	wid_list[wid_cnt].id = WID_RECEIVED_FRAGMENT_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)(&(dummyval));
	wid_cnt++;

	wid_list[wid_cnt].id = WID_FAILED_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)(&(dummyval));
	wid_cnt++;

	wid_list[wid_cnt].id = WID_INFO_ELEMENT_ASSOCIATE;
	wid_list[wid_cnt].type = WID_BIN_DATA;
	wid_list[wid_cnt].val = hif_drv->usr_conn_req.ies;
	wid_list[wid_cnt].size = hif_drv->usr_conn_req.ies_len;
	wid_cnt++;

	wid_list[wid_cnt].id = (u16)WID_11I_MODE;
	wid_list[wid_cnt].type = WID_CHAR;
	wid_list[wid_cnt].size = sizeof(char);
	wid_list[wid_cnt].val = (s8 *)&hif_drv->usr_conn_req.security;
	wid_cnt++;

	PRINT_D(vif->ndev, HOSTINF_DBG, "Encrypt Mode = %x\n", hif_drv->usr_conn_req.security);
	wid_list[wid_cnt].id = (u16)WID_AUTH_TYPE;
	wid_list[wid_cnt].type = WID_CHAR;
	wid_list[wid_cnt].size = sizeof(char);
	wid_list[wid_cnt].val = (s8 *)&hif_drv->usr_conn_req.auth_type;
	wid_cnt++;

	PRINT_D(vif->ndev, HOSTINF_DBG, "Authentication Type = %x\n", hif_drv->usr_conn_req.auth_type);
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Connecting to network of SSID %s on channel %d\n",
		 hif_drv->usr_conn_req.ssid, conn_attr->ch);

	wid_list[wid_cnt].id = (u16)WID_JOIN_REQ_EXTENDED;
	wid_list[wid_cnt].type = WID_STR;
	wid_list[wid_cnt].size = 112;
	wid_list[wid_cnt].val = kmalloc(wid_list[wid_cnt].size, GFP_KERNEL);

	if (!wid_list[wid_cnt].val) {
		result = -EFAULT;
		goto error;
	}

	cur_byte = wid_list[wid_cnt].val;

	if (conn_attr->ssid) {
		memcpy(cur_byte, conn_attr->ssid, conn_attr->ssid_len);
		cur_byte[conn_attr->ssid_len] = '\0';
	}
	cur_byte += MAX_SSID_LEN;
	*(cur_byte++) = INFRASTRUCTURE;

	if (conn_attr->ch >= 1 && conn_attr->ch <= 14) {
		*(cur_byte++) = conn_attr->ch;
	} else {
		PRINT_ER(vif->ndev, "Channel out of range\n");
		*(cur_byte++) = 0xFF;
	}
	*(cur_byte++)  = (bss_param->cap_info) & 0xFF;
	*(cur_byte++)  = ((bss_param->cap_info) >> 8) & 0xFF;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* Cap Info %0x*\n", (*(cur_byte - 2) | ((*(cur_byte - 1)) << 8)));

	if (conn_attr->bssid)
		memcpy(cur_byte, conn_attr->bssid, 6);
	cur_byte += 6;

	if (conn_attr->bssid)
		memcpy(cur_byte, conn_attr->bssid, 6);
	cur_byte += 6;

	*(cur_byte++)  = (bss_param->beacon_period) & 0xFF;
	*(cur_byte++)  = ((bss_param->beacon_period) >> 8) & 0xFF;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* Beacon Period %d*\n", (*(cur_byte - 2) | ((*(cur_byte - 1)) << 8)));
	*(cur_byte++)  =  bss_param->dtim_period;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* DTIM Period %d*\n", (*(cur_byte - 1)));

	memcpy(cur_byte, bss_param->supp_rates, MAX_RATES_SUPPORTED + 1);
	cur_byte += (MAX_RATES_SUPPORTED + 1);

	*(cur_byte++)  =  bss_param->wmm_cap;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* wmm cap%d*\n", (*(cur_byte - 1)));
	*(cur_byte++)  = bss_param->uapsd_cap;

	*(cur_byte++)  = bss_param->ht_capable;
	hif_drv->usr_conn_req.ht_capable = bss_param->ht_capable;

	*(cur_byte++)  =  bss_param->rsn_found;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* rsn found %d*\n", *(cur_byte - 1));
	*(cur_byte++)  =  bss_param->rsn_grp_policy;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* rsn group policy %0x*\n", (*(cur_byte - 1)));
	*(cur_byte++) =  bss_param->mode_802_11i;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* mode_802_11i %d*\n", (*(cur_byte - 1)));
	memcpy(cur_byte, bss_param->rsn_pcip_policy,
	       sizeof(bss_param->rsn_pcip_policy));
	cur_byte += sizeof(bss_param->rsn_pcip_policy);

	memcpy(cur_byte, bss_param->rsn_auth_policy,
	       sizeof(bss_param->rsn_auth_policy));
	cur_byte += sizeof(bss_param->rsn_auth_policy);

	memcpy(cur_byte, bss_param->rsn_cap, sizeof(bss_param->rsn_cap));
	cur_byte += sizeof(bss_param->rsn_cap);

	*(cur_byte++) = REAL_JOIN_REQ;
	*(cur_byte++) = bss_param->noa_enabled;

	if (bss_param->noa_enabled) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "NOA present\n");
		*(cur_byte++) = (bss_param->tsf) & 0xFF;
		*(cur_byte++) = ((bss_param->tsf) >> 8) & 0xFF;
		*(cur_byte++) = ((bss_param->tsf) >> 16) & 0xFF;
		*(cur_byte++) = ((bss_param->tsf) >> 24) & 0xFF;

		*(cur_byte++) = bss_param->idx;
		*(cur_byte++) = bss_param->opp_enabled;

		if (bss_param->opp_enabled)
			*(cur_byte++) = bss_param->ct_window;

		*(cur_byte++) = bss_param->cnt;

		memcpy(cur_byte, bss_param->duration,
		       sizeof(bss_param->duration));
		cur_byte += sizeof(bss_param->duration);

		memcpy(cur_byte, bss_param->interval,
		       sizeof(bss_param->interval));
		cur_byte += sizeof(bss_param->interval);

		memcpy(cur_byte, bss_param->start_time,
		       sizeof(bss_param->start_time));
		cur_byte += sizeof(bss_param->start_time);
	} else {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "NOA not present\n");
	}

	cur_byte = wid_list[wid_cnt].val;
	wid_cnt++;

	PRINT_INFO(vif->ndev, GENERIC_DBG,"send HOST_IF_WAITING_CONN_RESP\n");
	if (conn_attr->bssid) {
		memcpy(wilc_connected_ssid,
		       conn_attr->bssid, ETH_ALEN);
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "save Bssid = %x:%x:%x:%x:%x:%x\n",
			 (conn_attr->bssid[0]),
			 (conn_attr->bssid[1]),
			 (conn_attr->bssid[2]),
			 (conn_attr->bssid[3]),
			 (conn_attr->bssid[4]),
			 (conn_attr->bssid[5]));
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "save bssid = %x:%x:%x:%x:%x:%x\n",
			 (wilc_connected_ssid[0]), (wilc_connected_ssid[1]),
			 (wilc_connected_ssid[2]), (wilc_connected_ssid[3]),
			 (wilc_connected_ssid[4]), (wilc_connected_ssid[5]));
	}

	result = wilc_send_config_pkt(vif, SET_CFG, wid_list,
				      wid_cnt,
				      wilc_get_vif_idx(vif));
	if (result) {
		PRINT_ER(vif->ndev, "failed to send config packet\n");
		result = -EFAULT;
		goto error;
	} else {
		PRINT_INFO(vif->ndev, GENERIC_DBG,"set HOST_IF_WAITING_CONN_RESP\n");
		hif_drv->hif_state = HOST_IF_WAITING_CONN_RESP;
	}

error:
	if (result) {
		struct connect_info conn_info;

		del_timer(&hif_drv->connect_timer);

		PRINT_INFO(vif->ndev, HOSTINF_DBG, "could not start connecting to the required network\n");
		memset(&conn_info, 0, sizeof(struct connect_info));

		if (conn_attr->result) {
			if (conn_attr->bssid)
				memcpy(conn_info.bssid, conn_attr->bssid, 6);

			if (conn_attr->ies) {
				conn_info.req_ies_len = conn_attr->ies_len;
				conn_info.req_ies = kmalloc(conn_attr->ies_len,
							    GFP_KERNEL);
				memcpy(conn_info.req_ies,
				       conn_attr->ies,
				       conn_attr->ies_len);
			}

			conn_attr->result(CONN_DISCONN_EVENT_CONN_RESP,
							       &conn_info,
							       MAC_STATUS_DISCONNECTED,
							       NULL,
							       conn_attr->arg);
			hif_drv->hif_state = HOST_IF_IDLE;
			kfree(conn_info.req_ies);
			conn_info.req_ies = NULL;

		} else {
			PRINT_ER(vif->ndev, "Connect callback is NULL\n");
		}
	}

	kfree(conn_attr->bssid);
	conn_attr->bssid = NULL;

	kfree(conn_attr->ssid);
	conn_attr->ssid = NULL;

	kfree(conn_attr->ies);
	conn_attr->ies = NULL;

	kfree(cur_byte);
	return result;
}

static s32 handle_connect_timeout(struct wilc_vif *vif)
{
	s32 result = 0;
	struct connect_info info;
	struct wid wid;
	u16 dummy_reason_code = 0;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "Driver handler is NULL\n");
		return result;
	}

	hif_drv->hif_state = HOST_IF_IDLE;

	memset(&info, 0, sizeof(struct connect_info));

	if (hif_drv->usr_conn_req.conn_result) {
		if (hif_drv->usr_conn_req.bssid) {
			memcpy(info.bssid,
			       hif_drv->usr_conn_req.bssid, 6);
		}

		if (hif_drv->usr_conn_req.ies) {
			info.req_ies_len = hif_drv->usr_conn_req.ies_len;
			info.req_ies = kmalloc(hif_drv->usr_conn_req.ies_len, GFP_KERNEL);
			memcpy(info.req_ies,
			       hif_drv->usr_conn_req.ies,
			       hif_drv->usr_conn_req.ies_len);
		}

		hif_drv->usr_conn_req.conn_result(CONN_DISCONN_EVENT_CONN_RESP,
						  &info,
						  MAC_STATUS_DISCONNECTED,
						  NULL,
						  hif_drv->usr_conn_req.arg);

		kfree(info.req_ies);
		info.req_ies = NULL;
	} else {
		PRINT_ER(vif->ndev, "Connect callback is NULL\n");
	}

	wid.id = (u16)WID_DISCONNECT;
	wid.type = WID_CHAR;
	wid.val = (s8 *)&dummy_reason_code;
	wid.size = sizeof(char);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Sending disconnect request\n");
	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send disconect\n");

	hif_drv->usr_conn_req.ssid_len = 0;
	kfree(hif_drv->usr_conn_req.ssid);
	hif_drv->usr_conn_req.ssid = NULL;
	kfree(hif_drv->usr_conn_req.bssid);
	hif_drv->usr_conn_req.bssid = NULL;
	hif_drv->usr_conn_req.ies_len = 0;
	kfree(hif_drv->usr_conn_req.ies);
	hif_drv->usr_conn_req.ies = NULL;

	eth_zero_addr(wilc_connected_ssid);

	return result;
}

static s32 handle_rcvd_ntwrk_info(struct wilc_vif *vif,
				  struct rcvd_net_info *rcvd_info)
{
	u32 i;
	bool found;
	s32 result = 0;
	struct network_info *info = NULL;
	void *params = NULL;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct user_scan_req *scan_req = &hif_drv->usr_scan_req;

	found = true;
	PRINT_D(vif->ndev, HOSTINF_DBG, "Handling received network info\n");

	if (!scan_req->scan_result)
		goto done;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "State: Scanning, parsing network information received\n");
	wilc_parse_network_info(vif, rcvd_info->buffer, &info);
	if (!info || !scan_req->scan_result) {
		PRINT_ER(vif->ndev, "driver is null\n");
		result = -EINVAL;
		goto done;
	}

	for (i = 0; i < scan_req->rcvd_ch_cnt; i++) {
		if (memcmp(scan_req->net_info[i].bssid, info->bssid, 6) == 0) {
			if (info->rssi <= scan_req->net_info[i].rssi) {
				PRINT_INFO(vif->ndev, HOSTINF_DBG, "Network previously discovered\n");
				goto done;
			} else {
				scan_req->net_info[i].rssi = info->rssi;
				found = false;
				break;
			}
		}
	}

	if (found) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "New network found\n");
		if (scan_req->rcvd_ch_cnt < MAX_NUM_SCANNED_NETWORKS) {
			scan_req->net_info[scan_req->rcvd_ch_cnt].rssi = info->rssi;

			memcpy(scan_req->net_info[scan_req->rcvd_ch_cnt].bssid,
			       info->bssid, 6);

			scan_req->rcvd_ch_cnt++;

			info->new_network = true;
			params = host_int_parse_join_bss_param(info);

			scan_req->scan_result(SCAN_EVENT_NETWORK_FOUND, info,
					       scan_req->arg, params);
		} else {
			PRINT_WRN(vif->ndev, HOSTINF_DBG, "Discovered networks exceeded max. limit\n");
		}
	} else {
		info->new_network = false;
		scan_req->scan_result(SCAN_EVENT_NETWORK_FOUND, info,
				      scan_req->arg, NULL);
	}

done:
	kfree(rcvd_info->buffer);
	rcvd_info->buffer = NULL;

	if (info) {
		kfree(info->ies);
		kfree(info);
	}

	return result;
}

static s32 host_int_get_assoc_res_info(struct wilc_vif *vif,
				       u8 *assoc_resp_info,
				       u32 max_assoc_resp_info_len,
				       u32 *rcvd_assoc_resp_info_len);

static inline void host_int_free_user_conn_req(struct host_if_drv *hif_drv)
{
	hif_drv->usr_conn_req.ssid_len = 0;
	kfree(hif_drv->usr_conn_req.ssid);
	hif_drv->usr_conn_req.ssid = NULL;
	kfree(hif_drv->usr_conn_req.bssid);
	hif_drv->usr_conn_req.bssid = NULL;
	hif_drv->usr_conn_req.ies_len = 0;
	kfree(hif_drv->usr_conn_req.ies);
	hif_drv->usr_conn_req.ies = NULL;
}

static inline void host_int_parse_assoc_resp_info(struct wilc_vif *vif,
						  u8 mac_status)
{
	struct connect_resp_info *connect_resp_info = NULL;
	struct connect_info conn_info;
	struct host_if_drv *hif_drv = vif->hif_drv;

	memset(&conn_info, 0, sizeof(struct connect_info));

	if (mac_status == MAC_STATUS_CONNECTED) {
		u32 rcvd_assoc_resp_info_len;

		memset(rcv_assoc_resp, 0, MAX_ASSOC_RESP_FRAME_SIZE);

		host_int_get_assoc_res_info(vif, rcv_assoc_resp,
					    MAX_ASSOC_RESP_FRAME_SIZE,
					    &rcvd_assoc_resp_info_len);

		PRINT_D(vif->ndev, HOSTINF_DBG,
			"Received association response = %d\n",
			rcvd_assoc_resp_info_len);
		if (rcvd_assoc_resp_info_len != 0) {
			s32 err = 0;
			PRINT_INFO(vif->ndev, HOSTINF_DBG, 
				   "Parsing association response\n");
			err = wilc_parse_assoc_resp_info(rcv_assoc_resp, rcvd_assoc_resp_info_len,
							 &connect_resp_info);
			if (err) {
				PRINT_ER(vif->ndev,
					 "wilc_parse_assoc_resp_info() returned error %d\n",
					 err);
			} else {
				conn_info.status = connect_resp_info->status;

				if (conn_info.status == SUCCESSFUL_STATUSCODE &&
				    connect_resp_info->ies) {
					PRINT_D(vif->ndev, HOSTINF_DBG,
						"Association response received : Successful connection status\n");
					conn_info.resp_ies = kmemdup(connect_resp_info->ies,
								     connect_resp_info->ies_len,
								     GFP_KERNEL);
					if (conn_info.resp_ies)
						conn_info.resp_ies_len = connect_resp_info->ies_len;
				}

				kfree(connect_resp_info->ies);
				kfree(connect_resp_info);
			}
		}
	}

	if (mac_status == MAC_STATUS_CONNECTED &&
	    conn_info.status != SUCCESSFUL_STATUSCODE) {
		PRINT_ER(vif->ndev,
			 "Received MAC status is MAC_STATUS_CONNECTED while the received status code in Asoc Resp is not SUCCESSFUL_STATUSCODE\n");
		eth_zero_addr(wilc_connected_ssid);
	} else if (mac_status == MAC_STATUS_DISCONNECTED)    {
		PRINT_ER(vif->ndev, "Received MAC status is MAC_STATUS_DISCONNECTED\n");
		eth_zero_addr(wilc_connected_ssid);
	}

	if (hif_drv->usr_conn_req.bssid) {
		memcpy(conn_info.bssid, hif_drv->usr_conn_req.bssid, 6);

		if (mac_status == MAC_STATUS_CONNECTED &&
		    conn_info.status == SUCCESSFUL_STATUSCODE) {
			memcpy(hif_drv->assoc_bssid,
			       hif_drv->usr_conn_req.bssid, ETH_ALEN);
		}
	}

	if (hif_drv->usr_conn_req.ies) {
		conn_info.req_ies = kmemdup(hif_drv->usr_conn_req.ies,
					    hif_drv->usr_conn_req.ies_len,
					    GFP_KERNEL);
		if (conn_info.req_ies)
			conn_info.req_ies_len = hif_drv->usr_conn_req.ies_len;
	}

	del_timer(&hif_drv->connect_timer);
	hif_drv->usr_conn_req.conn_result(CONN_DISCONN_EVENT_CONN_RESP,
					  &conn_info, mac_status, NULL,
					  hif_drv->usr_conn_req.arg);

	if (mac_status == MAC_STATUS_CONNECTED &&
	    conn_info.status == SUCCESSFUL_STATUSCODE) {

		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "MAC status : CONNECTED and Connect Status : Successful\n");
		hif_drv->hif_state = HOST_IF_CONNECTED;

#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
		handle_pwrsave_during_obtainingIP(vif, IP_STATE_OBTAINING);
#endif
	} else {
		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "MAC status : %d and Connect Status : %d\n",
			   mac_status, conn_info.status);
		hif_drv->hif_state = HOST_IF_IDLE;
	}

	kfree(conn_info.resp_ies);
	conn_info.resp_ies = NULL;

	kfree(conn_info.req_ies);
	conn_info.req_ies = NULL;
	host_int_free_user_conn_req(hif_drv);
}

static inline void host_int_handle_disconnect(struct wilc_vif *vif)
{
	struct disconnect_info disconn_info;
	struct host_if_drv *hif_drv = vif->hif_drv;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Received MAC_STATUS_DISCONNECTED from the FW\n");
	memset(&disconn_info, 0, sizeof(struct disconnect_info));

	if (hif_drv->usr_scan_req.scan_result) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, 
			   "\n\n<< Abort the running OBSS Scan >>\n\n");
		del_timer(&hif_drv->scan_timer);
		handle_scan_done(vif, SCAN_EVENT_ABORTED);
	}

	disconn_info.reason = 0;
	disconn_info.ie = NULL;
	disconn_info.ie_len = 0;

	if (hif_drv->usr_conn_req.conn_result) {
#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
		handle_pwrsave_during_obtainingIP(vif, IP_STATE_DEFAULT);
#endif

		hif_drv->usr_conn_req.conn_result(CONN_DISCONN_EVENT_DISCONN_NOTIF,
						  NULL, 0, &disconn_info,
						  hif_drv->usr_conn_req.arg);
	} else {
		PRINT_ER(vif->ndev, "Connect result NULL\n");
	}

	eth_zero_addr(hif_drv->assoc_bssid);

	host_int_free_user_conn_req(hif_drv);
	hif_drv->hif_state = HOST_IF_IDLE;
}

static s32 handle_rcvd_gnrl_async_info(struct wilc_vif *vif,
				       struct rcvd_async_info *rcvd_info)
{
	s32 result = 0;
	u8 msg_type = 0;
	u8 msg_id = 0;
	u16 msg_len = 0;
	u16 wid_id = (u16)WID_NIL;
	u8 wid_len  = 0;
	u8 mac_status;
	u8 mac_status_reason_code;
	u8 mac_status_additional_info;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!rcvd_info->buffer) {
		netdev_err(vif->ndev, "Received buffer is NULL\n");
		return -EINVAL;
	}

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "Driver handler is NULL\n");
		kfree(rcvd_info->buffer);
		rcvd_info->buffer = NULL;
		return -ENODEV;
	}
	PRINT_INFO(vif->ndev, GENERIC_DBG, "Current State = %d,Received state = %d\n",
		   hif_drv->hif_state,
		   rcvd_info->buffer[7]);

	if (hif_drv->hif_state == HOST_IF_WAITING_CONN_RESP ||
	    hif_drv->hif_state == HOST_IF_CONNECTED ||
	    hif_drv->usr_scan_req.scan_result) {
		if (!hif_drv->usr_conn_req.conn_result) {
			PRINT_ER(vif->ndev, "driver is null\n");
			kfree(rcvd_info->buffer);
			rcvd_info->buffer = NULL;
			return -EINVAL;
		}

		msg_type = rcvd_info->buffer[0];

		if ('I' != msg_type) {
			PRINT_ER(vif->ndev, "Received Message incorrect.\n");
			kfree(rcvd_info->buffer);
			rcvd_info->buffer = NULL;
			return -EFAULT;
		}

		msg_id = rcvd_info->buffer[1];
		msg_len = MAKE_WORD16(rcvd_info->buffer[2], rcvd_info->buffer[3]);
		wid_id = MAKE_WORD16(rcvd_info->buffer[4], rcvd_info->buffer[5]);
		wid_len = rcvd_info->buffer[6];
		mac_status  = rcvd_info->buffer[7];
		mac_status_reason_code = rcvd_info->buffer[8];
		mac_status_additional_info = rcvd_info->buffer[9];
		PRINT_INFO(vif->ndev, HOSTINF_DBG, 
			   "Recieved MAC status= %d Reason= %d Info = %d\n",
			   mac_status, mac_status_reason_code,
			   mac_status_additional_info);
		if (hif_drv->hif_state == HOST_IF_WAITING_CONN_RESP) {
			PRINT_INFO(vif->ndev, HOSTINF_DBG,
				   "Recieved MAC status= %d Reason= %d Code= %d\n",
				   mac_status, mac_status_reason_code,
				   mac_status_additional_info);
			host_int_parse_assoc_resp_info(vif, mac_status);
		} else if ((mac_status == MAC_STATUS_DISCONNECTED) &&
			   (hif_drv->hif_state == HOST_IF_CONNECTED)) {
			host_int_handle_disconnect(vif);
		} else if ((mac_status == MAC_STATUS_DISCONNECTED) &&
			   (hif_drv->usr_scan_req.scan_result)) {
			PRINT_INFO(vif->ndev, HOSTINF_DBG, 
				   "Received MAC_STATUS_DISCONNECTED from the FW while scanning\n");
			PRINT_WRN(vif->ndev, HOSTINF_DBG,
				  "\n\n<< Abort the running Scan >>\n\n");
			del_timer(&hif_drv->scan_timer);
			if (hif_drv->usr_scan_req.scan_result)
				handle_scan_done(vif, SCAN_EVENT_ABORTED);
		}
	}

	kfree(rcvd_info->buffer);
	rcvd_info->buffer = NULL;

	return result;
}

static int handle_key(struct wilc_vif *vif, struct key_attr *hif_key)
{
	s32 result = 0;
	struct wid wid;
	struct wid wid_list[5];
	u8 i;
	u8 *key_buf;
	s8 s8idxarray[1];
	s8 ret = 0;
	struct host_if_drv *hif_drv = vif->hif_drv;

	switch (hif_key->type) {
	case WEP:

		if (hif_key->action & ADDKEY_AP) {
			PRINT_INFO(vif->ndev, HOSTINF_DBG, 
				   "Handling WEP key index: %d\n",
				   hif_key->attr.wep.index);
			wid_list[0].id = (u16)WID_11I_MODE;
			wid_list[0].type = WID_CHAR;
			wid_list[0].size = sizeof(char);
			wid_list[0].val = (s8 *)&hif_key->attr.wep.mode;

			wid_list[1].id = WID_AUTH_TYPE;
			wid_list[1].type = WID_CHAR;
			wid_list[1].size = sizeof(char);
			wid_list[1].val = (s8 *)&hif_key->attr.wep.auth_type;

			key_buf = kmalloc(hif_key->attr.wep.key_len + 2,
					  GFP_KERNEL);
			if (!key_buf) {
				PRINT_ER(vif->ndev, "No buffer to send Key\n");
				return -ENOMEM;
			}
			key_buf[0] = hif_key->attr.wep.index;
			key_buf[1] = hif_key->attr.wep.key_len;

			memcpy(&key_buf[2], hif_key->attr.wep.key,
			       hif_key->attr.wep.key_len);

			kfree(hif_key->attr.wep.key);

			wid_list[2].id = (u16)WID_WEP_KEY_VALUE;
			wid_list[2].type = WID_STR;
			wid_list[2].size = hif_key->attr.wep.key_len + 2;
			wid_list[2].val = (s8 *)key_buf;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      wid_list, 3,
						      wilc_get_vif_idx(vif));
			kfree(key_buf);
		} else if (hif_key->action & ADDKEY) {
			PRINT_INFO(vif->ndev, HOSTINF_DBG, 
				   "Handling WEP key\n");
			key_buf = kmalloc(hif_key->attr.wep.key_len + 2, GFP_KERNEL);
			if (!key_buf) {
				PRINT_ER(vif->ndev, "No buffer to send Key\n");
				return -ENOMEM;
			}
			key_buf[0] = hif_key->attr.wep.index;
			memcpy(key_buf + 1, &hif_key->attr.wep.key_len, 1);
			memcpy(key_buf + 2, hif_key->attr.wep.key,
			       hif_key->attr.wep.key_len);
			kfree(hif_key->attr.wep.key);

			wid.id = (u16)WID_ADD_WEP_KEY;
			wid.type = WID_STR;
			wid.val = (s8 *)key_buf;
			wid.size = hif_key->attr.wep.key_len + 2;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      &wid, 1,
						      wilc_get_vif_idx(vif));
			kfree(key_buf);
		} else if (hif_key->action & REMOVEKEY) {
			PRINT_INFO(vif->ndev, HOSTINF_DBG, "Removing key\n");
			wid.id = (u16)WID_REMOVE_WEP_KEY;
			wid.type = WID_STR;

			s8idxarray[0] = (s8)hif_key->attr.wep.index;
			wid.val = s8idxarray;
			wid.size = 1;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      &wid, 1,
						      wilc_get_vif_idx(vif));
		} else if (hif_key->action & DEFAULTKEY) {
			wid.id = (u16)WID_KEY_ID;
			wid.type = WID_CHAR;
			wid.val = (s8 *)&hif_key->attr.wep.index;
			wid.size = sizeof(char);

			PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setting default key index\n");
			result = wilc_send_config_pkt(vif, SET_CFG,
						      &wid, 1,
						      wilc_get_vif_idx(vif));
		}
		complete(&hif_drv->comp_test_key_block);
		break;

	case WPA_RX_GTK:
		if (hif_key->action & ADDKEY_AP) {
			key_buf = kzalloc(RX_MIC_KEY_MSG_LEN, GFP_KERNEL);
			if (!key_buf) {
				PRINT_ER(vif->ndev, "No buffer to send RxGTK Key\n");
				ret = -ENOMEM;
				goto out_wpa_rx_gtk;
			}

			if (hif_key->attr.wpa.seq)
				memcpy(key_buf + 6, hif_key->attr.wpa.seq, 8);

			memcpy(key_buf + 14, &hif_key->attr.wpa.index, 1);
			memcpy(key_buf + 15, &hif_key->attr.wpa.key_len, 1);
			memcpy(key_buf + 16, hif_key->attr.wpa.key,
			       hif_key->attr.wpa.key_len);

			wid_list[0].id = (u16)WID_11I_MODE;
			wid_list[0].type = WID_CHAR;
			wid_list[0].size = sizeof(char);
			wid_list[0].val = (s8 *)&hif_key->attr.wpa.mode;

			wid_list[1].id = (u16)WID_ADD_RX_GTK;
			wid_list[1].type = WID_STR;
			wid_list[1].val = (s8 *)key_buf;
			wid_list[1].size = RX_MIC_KEY_MSG_LEN;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      wid_list, 2,
						      wilc_get_vif_idx(vif));

			kfree(key_buf);
			complete(&hif_drv->comp_test_key_block);
		} else if (hif_key->action & ADDKEY) {
			PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling group key(Rx) function\n");
			key_buf = kzalloc(RX_MIC_KEY_MSG_LEN, GFP_KERNEL);
			if (!key_buf) {
				PRINT_ER(vif->ndev, "No buffer to send RxGTK Key\n");
				ret = -ENOMEM;
				goto out_wpa_rx_gtk;
			}

			if (hif_drv->hif_state == HOST_IF_CONNECTED)
				memcpy(key_buf, hif_drv->assoc_bssid, ETH_ALEN);
			else
				PRINT_ER(vif->ndev, "Couldn't handle\n");

			memcpy(key_buf + 6, hif_key->attr.wpa.seq, 8);
			memcpy(key_buf + 14, &hif_key->attr.wpa.index, 1);
			memcpy(key_buf + 15, &hif_key->attr.wpa.key_len, 1);
			memcpy(key_buf + 16, hif_key->attr.wpa.key,
			       hif_key->attr.wpa.key_len);

			wid.id = (u16)WID_ADD_RX_GTK;
			wid.type = WID_STR;
			wid.val = (s8 *)key_buf;
			wid.size = RX_MIC_KEY_MSG_LEN;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      &wid, 1,
						      wilc_get_vif_idx(vif));

			kfree(key_buf);
			complete(&hif_drv->comp_test_key_block);
		}
out_wpa_rx_gtk:
		kfree(hif_key->attr.wpa.key);
		kfree(hif_key->attr.wpa.seq);
		if (ret)
			return ret;

		break;

	case WPA_PTK:
		if (hif_key->action & ADDKEY_AP) {
			key_buf = kmalloc(PTK_KEY_MSG_LEN + 1, GFP_KERNEL);
			if (!key_buf) {
				PRINT_ER(vif->ndev, "No buffer to send PTK Key\n");
				ret = -ENOMEM;
				goto out_wpa_ptk;
			}

			memcpy(key_buf, hif_key->attr.wpa.mac_addr, 6);
			memcpy(key_buf + 6, &hif_key->attr.wpa.index, 1);
			memcpy(key_buf + 7, &hif_key->attr.wpa.key_len, 1);
			memcpy(key_buf + 8, hif_key->attr.wpa.key,
			       hif_key->attr.wpa.key_len);

			wid_list[0].id = (u16)WID_11I_MODE;
			wid_list[0].type = WID_CHAR;
			wid_list[0].size = sizeof(char);
			wid_list[0].val = (s8 *)&hif_key->attr.wpa.mode;

			wid_list[1].id = (u16)WID_ADD_PTK;
			wid_list[1].type = WID_STR;
			wid_list[1].val = (s8 *)key_buf;
			wid_list[1].size = PTK_KEY_MSG_LEN + 1;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      wid_list, 2,
						      wilc_get_vif_idx(vif));
			kfree(key_buf);
			complete(&hif_drv->comp_test_key_block);
		} else if (hif_key->action & ADDKEY) {
			key_buf = kmalloc(PTK_KEY_MSG_LEN, GFP_KERNEL);
			if (!key_buf) {
				PRINT_ER(vif->ndev, "No buffer send PTK\n");
				ret = -ENOMEM;
				goto out_wpa_ptk;
			}

			memcpy(key_buf, hif_key->attr.wpa.mac_addr, 6);
			memcpy(key_buf + 6, &hif_key->attr.wpa.key_len, 1);
			memcpy(key_buf + 7, hif_key->attr.wpa.key,
			       hif_key->attr.wpa.key_len);

			wid.id = (u16)WID_ADD_PTK;
			wid.type = WID_STR;
			wid.val = (s8 *)key_buf;
			wid.size = PTK_KEY_MSG_LEN;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      &wid, 1,
						      wilc_get_vif_idx(vif));
			kfree(key_buf);
			complete(&hif_drv->comp_test_key_block);
		}

out_wpa_ptk:
		kfree(hif_key->attr.wpa.key);
		if (ret)
			return ret;

		break;

	case PMKSA:
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling PMKSA key\n");
		key_buf = kmalloc((hif_key->attr.pmkid.numpmkid * PMKSA_KEY_LEN) + 1, GFP_KERNEL);
		if (!key_buf) {
			PRINT_ER(vif->ndev, "No buffer to send PMKSA Key\n");
			return -ENOMEM;
		}

		key_buf[0] = hif_key->attr.pmkid.numpmkid;

		for (i = 0; i < hif_key->attr.pmkid.numpmkid; i++) {
			memcpy(key_buf + ((PMKSA_KEY_LEN * i) + 1), hif_key->attr.pmkid.pmkidlist[i].bssid, ETH_ALEN);
			memcpy(key_buf + ((PMKSA_KEY_LEN * i) + ETH_ALEN + 1), hif_key->attr.pmkid.pmkidlist[i].pmkid, PMKID_LEN);
		}

		wid.id = (u16)WID_PMKID_INFO;
		wid.type = WID_STR;
		wid.val = (s8 *)key_buf;
		wid.size = (hif_key->attr.pmkid.numpmkid * PMKSA_KEY_LEN) + 1;

		result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
					      wilc_get_vif_idx(vif));

		kfree(key_buf);
		break;
	}

	if (result)
		PRINT_ER(vif->ndev, "Failed to send key config packet\n");

	return result;
}

static void handle_disconnect(struct wilc_vif *vif)
{
	struct wid wid;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct disconnect_info disconn_info;
	struct user_scan_req *scan_req;
	struct user_conn_req *conn_req;
	s32 result = 0;
	u16 dummy_reason_code = 0;
	struct host_if_drv *hif_drv_p2p  = wilc_get_drv_handler_by_ifc(vif->wilc, P2P_IFC);
	struct host_if_drv *hif_drv_wlan = wilc_get_drv_handler_by_ifc(vif->wilc, WLAN_IFC);

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "Driver is null\n");
		return;
	}
	
	if (hif_drv_wlan != NULL)	{
		if (hif_drv_wlan->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,"Abort Scan before disconnecting. WLAN_IFC is in state [%d]\n",
				hif_drv_wlan->hif_state);
			del_timer(&(hif_drv_wlan->scan_timer));
			handle_scan_done(vif, SCAN_EVENT_ABORTED);
		}
	}
	if (hif_drv_p2p != NULL) {
		if (hif_drv_p2p->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,"Abort Scan before disconnecting. P2P_IFC is in state [%d]\n",
				 hif_drv_p2p->hif_state);
			del_timer(&(hif_drv_p2p->scan_timer));
			handle_scan_done(vif, SCAN_EVENT_ABORTED);
		}
	}
	wid.id = (u16)WID_DISCONNECT;
	wid.type = WID_CHAR;
	wid.val = (s8 *)&dummy_reason_code;
	wid.size = sizeof(char);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Sending disconnect request\n");

#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
	handle_pwrsave_during_obtainingIP(vif, IP_STATE_DEFAULT);
#endif
	eth_zero_addr(wilc_connected_ssid);

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));

	if (result) {
		PRINT_ER(vif->ndev, "Failed to send dissconect\n");
		goto out;
	}

	memset(&disconn_info, 0, sizeof(struct disconnect_info));

	disconn_info.reason = 0;
	disconn_info.ie = NULL;
	disconn_info.ie_len = 0;
	scan_req = &hif_drv->usr_scan_req;
	conn_req = &hif_drv->usr_conn_req;

	if (scan_req->scan_result) {
		del_timer(&hif_drv->scan_timer);
		scan_req->scan_result(SCAN_EVENT_ABORTED, NULL, scan_req->arg,
				      NULL);
		scan_req->scan_result = NULL;
	}

	if (conn_req->conn_result) {
		if (hif_drv->hif_state == HOST_IF_WAITING_CONN_RESP) {
			struct connect_info strConnectInfo;
			PRINT_INFO(vif->ndev, HOSTINF_DBG,
				   "Upper layer requested termination of connection\n");
			memset(&strConnectInfo, 0, sizeof(struct connect_info));
			del_timer(&hif_drv->connect_timer);
			if (conn_req->bssid != NULL)
				memcpy(strConnectInfo.bssid, conn_req->bssid, 6);
			if (conn_req->ies != NULL) {
				strConnectInfo.req_ies_len = conn_req->ies_len;
				strConnectInfo.req_ies = kmalloc(conn_req->ies_len, GFP_ATOMIC);
				memcpy(strConnectInfo.req_ies,
				       conn_req->ies,
				       conn_req->ies_len);
			}
			conn_req->conn_result(CONN_DISCONN_EVENT_CONN_RESP,
					      &strConnectInfo,
					      MAC_STATUS_DISCONNECTED, NULL,
					      conn_req->arg);

			if (strConnectInfo.req_ies != NULL) {
				kfree(strConnectInfo.req_ies);
				strConnectInfo.req_ies = NULL;
			}

		} else if (hif_drv->hif_state == HOST_IF_CONNECTED) {
			conn_req->conn_result(CONN_DISCONN_EVENT_DISCONN_NOTIF,
					      NULL, 0, &disconn_info,
					      conn_req->arg);
		}
	} else {
		PRINT_ER(vif->ndev, "conn_result = NULL\n");
	}

	hif_drv->hif_state = HOST_IF_IDLE;

	eth_zero_addr(hif_drv->assoc_bssid);

	conn_req->ssid_len = 0;
	kfree(conn_req->ssid);
	conn_req->ssid = NULL;
	kfree(conn_req->bssid);
	conn_req->bssid = NULL;
	conn_req->ies_len = 0;
	kfree(conn_req->ies);
	conn_req->ies = NULL;

out:

	complete(&hif_drv->comp_test_disconn_block);
}

void wilc_resolve_disconnect_aberration(struct wilc_vif *vif)
{
	if (!vif->hif_drv)
		return;
	if (vif->hif_drv->hif_state == HOST_IF_WAITING_CONN_RESP ||
	    vif->hif_drv->hif_state == HOST_IF_CONNECTING) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "\n\n<< correcting Supplicant state machine >>\n\n");
		wilc_disconnect(vif, 1);
	}
}

static void handle_get_rssi(struct wilc_vif *vif)
{
	s32 result = 0;
	struct wid wid;

	wid.id = (u16)WID_RSSI;
	wid.type = WID_CHAR;
	wid.val = &rssi;
	wid.size = sizeof(char);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Getting RSSI value\n");
	result = wilc_send_config_pkt(vif, GET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result) {
		PRINT_ER(vif->ndev, "Failed to get RSSI value\n");
		result = -EFAULT;
	}

	complete(&vif->hif_drv->comp_get_rssi);
}

static s32 handle_get_statistics(struct wilc_vif *vif,
				 struct rf_info *stats)
{
	struct wid wid_list[5];
	u32 wid_cnt = 0, result = 0;

	wid_list[wid_cnt].id = WID_LINKSPEED;
	wid_list[wid_cnt].type = WID_CHAR;
	wid_list[wid_cnt].size = sizeof(char);
	wid_list[wid_cnt].val = (s8 *)&stats->link_speed;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_RSSI;
	wid_list[wid_cnt].type = WID_CHAR;
	wid_list[wid_cnt].size = sizeof(char);
	wid_list[wid_cnt].val = (s8 *)&stats->rssi;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_SUCCESS_FRAME_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)&stats->tx_cnt;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_RECEIVED_FRAGMENT_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)&stats->rx_cnt;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_FAILED_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)&stats->tx_fail_cnt;
	wid_cnt++;

	result = wilc_send_config_pkt(vif, GET_CFG, wid_list,
				      wid_cnt,
				      wilc_get_vif_idx(vif));

	if (result)
		PRINT_ER(vif->ndev, "Failed to send scan parameters\n");

	if (stats->link_speed > TCP_ACK_FILTER_LINK_SPEED_THRESH &&
	    stats->link_speed != DEFAULT_LINK_SPEED) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "Enable TCP filter\n");
		wilc_enable_tcp_ack_filter(true);
	} else if (stats->link_speed != DEFAULT_LINK_SPEED) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "Disable TCP filter %d\n",stats->link_speed);
		wilc_enable_tcp_ack_filter(false);
	}
	if (stats != &vif->wilc->dummy_statistics)
		complete(&hif_wait_response);
	return 0;
}

static s32 handle_get_inactive_time(struct wilc_vif *vif,
				    struct sta_inactive_t *hif_sta_inactive)
{
	s32 result = 0;
	struct wid wid;
	struct host_if_drv *hif_drv = vif->hif_drv;

	wid.id = (u16)WID_SET_STA_MAC_INACTIVE_TIME;
	wid.type = WID_STR;
	wid.size = ETH_ALEN;
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		return -ENOMEM;

	ether_addr_copy(wid.val, hif_sta_inactive->mac);

	PRINT_INFO(vif->ndev, CFG80211_DBG, "SETING STA inactive time\n");
	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	kfree(wid.val);

	if (result) {
		PRINT_ER(vif->ndev, "Failed to SET inactive time\n");
		return -EFAULT;
	}

	wid.id = (u16)WID_GET_INACTIVE_TIME;
	wid.type = WID_INT;
	wid.val = (s8 *)&inactive_time;
	wid.size = sizeof(u32);

	result = wilc_send_config_pkt(vif, GET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));

	if (result) {
		PRINT_ER(vif->ndev, "Failed to get inactive time\n");
		return -EFAULT;
	}

	PRINT_INFO(vif->ndev, CFG80211_DBG, "Getting inactive time : %d\n", inactive_time);
	complete(&hif_drv->comp_inactive_time);

	return result;
}

static void handle_add_beacon(struct wilc_vif *vif, struct beacon_attr *param)
{
	s32 result = 0;
	struct wid wid;
	u8 *cur_byte;

	wid.id = (u16)WID_ADD_BEACON;
	wid.type = WID_BIN;
	wid.size = param->head_len + param->tail_len + 16;
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		goto error;

	cur_byte = wid.val;
	*cur_byte++ = (param->interval & 0xFF);
	*cur_byte++ = ((param->interval >> 8) & 0xFF);
	*cur_byte++ = ((param->interval >> 16) & 0xFF);
	*cur_byte++ = ((param->interval >> 24) & 0xFF);

	*cur_byte++ = (param->dtim_period & 0xFF);
	*cur_byte++ = ((param->dtim_period >> 8) & 0xFF);
	*cur_byte++ = ((param->dtim_period >> 16) & 0xFF);
	*cur_byte++ = ((param->dtim_period >> 24) & 0xFF);

	*cur_byte++ = (param->head_len & 0xFF);
	*cur_byte++ = ((param->head_len >> 8) & 0xFF);
	*cur_byte++ = ((param->head_len >> 16) & 0xFF);
	*cur_byte++ = ((param->head_len >> 24) & 0xFF);

	memcpy(cur_byte, param->head, param->head_len);
	cur_byte += param->head_len;

	*cur_byte++ = (param->tail_len & 0xFF);
	*cur_byte++ = ((param->tail_len >> 8) & 0xFF);
	*cur_byte++ = ((param->tail_len >> 16) & 0xFF);
	*cur_byte++ = ((param->tail_len >> 24) & 0xFF);

	if (param->tail)
		memcpy(cur_byte, param->tail, param->tail_len);
	cur_byte += param->tail_len;

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send add beacon\n");

error:
	kfree(wid.val);
	kfree(param->head);
	kfree(param->tail);
}

static void handle_del_beacon(struct wilc_vif *vif)
{
	s32 result = 0;
	struct wid wid;
	u8 *cur_byte;

	wid.id = (u16)WID_DEL_BEACON;
	wid.type = WID_CHAR;
	wid.size = sizeof(char);
	wid.val = &del_beacon;

	if (!wid.val)
		return;

	cur_byte = wid.val;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Deleting BEACON\n");
	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send delete beacon\n");
}

static u32 wilc_hif_pack_sta_param(struct wilc_vif *vif, u8 *buff,
				    struct add_sta_param *param)
{
	u8 *cur_byte;

	cur_byte = buff;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Packing STA params\n");
	memcpy(cur_byte, param->bssid, ETH_ALEN);
	cur_byte +=  ETH_ALEN;

	*cur_byte++ = param->aid & 0xFF;
	*cur_byte++ = (param->aid >> 8) & 0xFF;

	*cur_byte++ = param->rates_len;
	if (param->rates_len > 0)
		memcpy(cur_byte, param->rates,
		       param->rates_len);
	cur_byte += param->rates_len;

	*cur_byte++ = param->ht_supported;
	memcpy(cur_byte, &param->ht_capa,
	       sizeof(struct ieee80211_ht_cap));
	cur_byte += sizeof(struct ieee80211_ht_cap);

	*cur_byte++ = param->flags_mask & 0xFF;
	*cur_byte++ = (param->flags_mask >> 8) & 0xFF;

	*cur_byte++ = param->flags_set & 0xFF;
	*cur_byte++ = (param->flags_set >> 8) & 0xFF;

	return cur_byte - buff;
}

static void handle_add_station(struct wilc_vif *vif,
			       struct add_sta_param *param)
{
	s32 result = 0;
	struct wid wid;
	u8 *cur_byte;

	wid.id = (u16)WID_ADD_STA;
	wid.type = WID_BIN;
	wid.size = WILC_ADD_STA_LENGTH + param->rates_len;

	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		goto error;

	cur_byte = wid.val;
	cur_byte += wilc_hif_pack_sta_param(vif, cur_byte, param);

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result != 0)
		PRINT_ER(vif->ndev, "Failed to send add station\n");

error:
	kfree(param->rates);
	kfree(wid.val);
}

static void handle_del_all_sta(struct wilc_vif *vif,
			       struct del_all_sta *param)
{
	s32 result = 0;
	struct wid wid;
	u8 *curr_byte;
	u8 i;
	u8 zero_buff[6] = {0};

	wid.id = (u16)WID_DEL_ALL_STA;
	wid.type = WID_STR;
	wid.size = (param->assoc_sta * ETH_ALEN) + 1;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling delete station\n");
	wid.val = kmalloc((param->assoc_sta * ETH_ALEN) + 1, GFP_KERNEL);
	if (!wid.val)
		goto error;

	curr_byte = wid.val;

	*(curr_byte++) = param->assoc_sta;

	for (i = 0; i < MAX_NUM_STA; i++) {
		if (memcmp(param->del_all_sta[i], zero_buff, ETH_ALEN))
			memcpy(curr_byte, param->del_all_sta[i], ETH_ALEN);
		else
			continue;

		curr_byte += ETH_ALEN;
	}

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send add station\n");

error:
	kfree(wid.val);

	complete(&hif_wait_response);
}

static void handle_del_station(struct wilc_vif *vif, struct del_sta *param)
{
	s32 result = 0;
	struct wid wid;
	u8 *cur_byte;

	wid.id = (u16)WID_REMOVE_STA;
	wid.type = WID_BIN;
	wid.size = ETH_ALEN;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling delete station\n");
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		goto error;

	cur_byte = wid.val;

	ether_addr_copy(cur_byte, param->mac_addr);

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send add station\n");

error:
	kfree(wid.val);
}

static void handle_edit_station(struct wilc_vif *vif,
				struct add_sta_param *param)
{
	s32 result = 0;
	struct wid wid;
	u8 *cur_byte;

	wid.id = (u16)WID_EDIT_STA;
	wid.type = WID_BIN;
	wid.size = WILC_ADD_STA_LENGTH + param->rates_len;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling edit station\n");
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		goto error;

	cur_byte = wid.val;
	cur_byte += wilc_hif_pack_sta_param(vif, cur_byte, param);

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send edit station\n");

error:
	kfree(param->rates);
	kfree(wid.val);
}

static int handle_remain_on_chan(struct wilc_vif *vif,
				 struct remain_ch *hif_remain_ch)
{
	s32 result = 0;
	u8 remain_on_chan_flag;
	struct wid wid;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct host_if_drv *hif_drv_p2p  = wilc_get_drv_handler_by_ifc(vif->wilc, P2P_IFC);
	struct host_if_drv *hif_drv_wlan = wilc_get_drv_handler_by_ifc(vif->wilc, WLAN_IFC);

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "Driver is null\n");
		return -EFAULT;
	}
	
	if (!hif_drv->remain_on_ch_pending) {
		hif_drv->remain_on_ch.arg = hif_remain_ch->arg;
		hif_drv->remain_on_ch.expired = hif_remain_ch->expired;
		hif_drv->remain_on_ch.ready = hif_remain_ch->ready;
		hif_drv->remain_on_ch.ch = hif_remain_ch->ch;
		hif_drv->remain_on_ch.id = hif_remain_ch->id;
	} else {
		hif_remain_ch->ch = hif_drv->remain_on_ch.ch;
	}

	if (hif_drv_p2p != NULL) {
		if (hif_drv_p2p->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,"Interface busy scanning. P2P_IFC is in state [%d]\n",
				hif_drv_p2p->hif_state);
			hif_drv->remain_on_ch_pending = 1;
			result = -EBUSY;
			goto error;
		} else if ((hif_drv_p2p->hif_state != HOST_IF_IDLE) &&
		(hif_drv_p2p->hif_state != HOST_IF_CONNECTED)) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,"Interface busy connecting or listening. P2P_IFC is in state [%d]\n",
			 hif_drv_p2p->hif_state);
			result = -EBUSY;
			goto error;
		}
	}
	if (hif_drv_wlan != NULL) {
		if (hif_drv_wlan->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,"Interface busy scanning. WLAN_IFC is in state [%d]\n",
				hif_drv_wlan->hif_state);
			hif_drv->remain_on_ch_pending = 1;
			result = -EBUSY;
			goto error;
		} else if ((hif_drv_wlan->hif_state != HOST_IF_IDLE) &&
		(hif_drv_wlan->hif_state != HOST_IF_CONNECTED)) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,"Interface busy connecting or listening. WLAN_IFC is in state [%d]\n",
			 hif_drv_wlan->hif_state);
			result = -EBUSY;
			goto error;
		}
	}

	if(wilc_connecting) {
		PRINT_INFO(vif->ndev, GENERIC_DBG, "[handle_scan]: Don't do scan in (CONNECTING) state\n");
		result = -EBUSY;
		goto error;
	}
#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
	if (wilc_optaining_ip) {
		PRINT_INFO(vif->ndev, GENERIC_DBG, "[handle_scan]: Don't do obss scan until IP adresss is obtained\n");
		result = -EBUSY;
		goto error;
	}
#endif

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setting channel :%d\n", hif_remain_ch->ch);
	remain_on_chan_flag = true;
	wid.id = (u16)WID_REMAIN_ON_CHAN;
	wid.type = WID_STR;
	wid.size = 2;
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val) {
		result = -ENOMEM;
		goto error;
	}

	wid.val[0] = remain_on_chan_flag;
	wid.val[1] = (s8)hif_remain_ch->ch;

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	kfree(wid.val);
	if (result != 0)
		PRINT_ER(vif->ndev, "Failed to set remain on channel\n");

	hif_drv->hif_state = HOST_IF_P2P_LISTEN;
error:
	
	hif_drv->remain_on_ch_timer_vif = vif;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
	hif_drv->remain_on_ch_timer.data = (unsigned long)hif_drv;
#endif
	mod_timer(&hif_drv->remain_on_ch_timer,
			  jiffies +
			  msecs_to_jiffies(hif_remain_ch->duration));

	if (hif_drv->remain_on_ch.ready)
		hif_drv->remain_on_ch.ready(hif_drv->remain_on_ch.arg);

	if (hif_drv->remain_on_ch_pending)
		hif_drv->remain_on_ch_pending = 0;

	return result;
}

static int handle_register_frame(struct wilc_vif *vif,
				 struct reg_frame *hif_reg_frame)
{
	s32 result = 0;
	struct wid wid;
	u8 *cur_byte;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling frame register Flag : %d FrameType: %d\n",
					hif_reg_frame->reg,
					hif_reg_frame->frame_type);
	wid.id = (u16)WID_REGISTER_FRAME;
	wid.type = WID_STR;
	wid.val = kmalloc(sizeof(u16) + 2, GFP_KERNEL);
	if (!wid.val)
		return -ENOMEM;

	cur_byte = wid.val;

	*cur_byte++ = hif_reg_frame->reg;
	*cur_byte++ = hif_reg_frame->reg_id;
	memcpy(cur_byte, &hif_reg_frame->frame_type, sizeof(u16));

	wid.size = sizeof(u16) + 2;

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	kfree(wid.val);
	if (result) {
		PRINT_ER(vif->ndev, "Failed to frame register\n");
		result = -EINVAL;
	}

	return result;
}

static u32 handle_listen_state_expired(struct wilc_vif *vif,
				       struct remain_ch *hif_remain_ch)
{
	u8 remain_on_chan_flag;
	struct wid wid;
	s32 result = 0;
	struct host_if_drv *hif_drv = vif->hif_drv;
	u8 null_bssid[6] = {0};


	PRINT_INFO(vif->ndev, HOSTINF_DBG, "CANCEL REMAIN ON CHAN\n");

	if (hif_drv->hif_state == HOST_IF_P2P_LISTEN) {
		remain_on_chan_flag = false;
		wid.id = (u16)WID_REMAIN_ON_CHAN;
		wid.type = WID_STR;
		wid.size = 2;
		wid.val = kmalloc(wid.size, GFP_KERNEL);

		if (!wid.val) {
			PRINT_ER(vif->ndev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		wid.val[0] = remain_on_chan_flag;
		wid.val[1] = FALSE_FRMWR_CHANNEL;

		result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
					      wilc_get_vif_idx(vif));
		kfree(wid.val);
		if (result != 0) {
			PRINT_ER(vif->ndev, "Failed to set remain channel\n");
			return result;
		}

		if (hif_drv->remain_on_ch.expired)
			hif_drv->remain_on_ch.expired(hif_drv->remain_on_ch.arg,
						      hif_remain_ch->id);

		if (memcmp(hif_drv->assoc_bssid, null_bssid, ETH_ALEN) == 0)
			hif_drv->hif_state = HOST_IF_IDLE;
		else
			hif_drv->hif_state = HOST_IF_CONNECTED;
	} else {
		PRINT_D(vif->ndev, GENERIC_DBG,  "Not in listen state\n");
		result = -EFAULT;
	}

	return result;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
static void listen_timer_cb(struct timer_list *t)
#else
static void listen_timer_cb(unsigned long arg)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
	struct host_if_drv *hif_drv = from_timer(hif_drv, t,
						      remain_on_ch_timer);
#else
	struct host_if_drv *hif_drv = (struct host_if_drv *)arg;
#endif
	struct wilc_vif *vif = hif_drv->remain_on_ch_timer_vif;
	s32 result = 0;
	struct host_if_msg msg;

	del_timer(&vif->hif_drv->remain_on_ch_timer);

	memset(&msg, 0, sizeof(struct host_if_msg));
	msg.id = HOST_IF_MSG_LISTEN_TIMER_FIRED;
	msg.vif = vif;
	msg.body.remain_on_ch.id = vif->hif_drv->remain_on_ch.id;

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "wilc_mq_send fail\n");
}

static void handle_power_management(struct wilc_vif *vif,
				    struct power_mgmt_param *pm_param)
{
	s32 result = 0;
	struct wid wid;
	s8 power_mode;

	wid.id = (u16)WID_POWER_MANAGEMENT;

	if (pm_param->enabled)
		power_mode = MIN_FAST_PS;
	else
		power_mode = NO_POWERSAVE;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling power mgmt to %d\n", power_mode);
	wid.val = &power_mode;
	wid.size = sizeof(char);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling Power Management\n");
	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result) {
		PRINT_ER(vif->ndev, "Failed to send power management\n");
		return;
	}
	store_power_save_current_state(vif, power_mode);
}

static void handle_set_mcast_filter(struct wilc_vif *vif,
				    struct set_multicast *hif_set_mc)
{
	s32 result = 0;
	struct wid wid;
	u8 *cur_byte;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setup Multicast Filter\n");

	wid.id = (u16)WID_SETUP_MULTICAST_FILTER;
	wid.type = WID_BIN;
	wid.size = sizeof(struct set_multicast) + (hif_set_mc->cnt * ETH_ALEN);
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		goto error;

	cur_byte = wid.val;
	*cur_byte++ = (hif_set_mc->enabled & 0xFF);
	*cur_byte++ = 0;
	*cur_byte++ = 0;
	*cur_byte++ = 0;

	*cur_byte++ = (hif_set_mc->cnt & 0xFF);
	*cur_byte++ = ((hif_set_mc->cnt >> 8) & 0xFF);
	*cur_byte++ = ((hif_set_mc->cnt >> 16) & 0xFF);
	*cur_byte++ = ((hif_set_mc->cnt >> 24) & 0xFF);

	if (hif_set_mc->cnt > 0)
		memcpy(cur_byte, wilc_multicast_mac_addr_list,
		       ((hif_set_mc->cnt) * ETH_ALEN));

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send setup multicast\n");

error:
	kfree(wid.val);
}

static void handle_set_wowlan_trigger(struct wilc_vif *vif, u8 u8WowlanTrigger)
{	
	int ret = 0;
	struct wid wid;

	wid.id = (u16)WID_WOWLAN_TRIGGER;
	wid.type = WID_CHAR;
	wid.val = (s8*)&u8WowlanTrigger;
	wid.size = sizeof(s8);

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));

	if (ret)
		PRINT_ER(vif->ndev, "Failed to send wowlan trigger config packet\n");
}

static void handle_set_tx_pwr(struct wilc_vif *vif, u8 tx_pwr)
{
	int ret;
	struct wid wid;

	wid.id = (u16)WID_TX_POWER;
	wid.type = WID_CHAR;
	wid.val = &tx_pwr;
	wid.size = sizeof(char);

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));
	if (ret)
		PRINT_ER(vif->ndev, "Failed to set TX PWR\n");
}

static void handle_get_tx_pwr(struct wilc_vif *vif, u8 *tx_pwr)
{
	int ret = 0;
	struct wid wid;

	wid.id = (u16)WID_TX_POWER;
	wid.type = WID_CHAR;
	wid.val = (s8 *)tx_pwr;
	wid.size = sizeof(char);

	ret = wilc_send_config_pkt(vif, GET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));
	if (ret)
		PRINT_ER(vif->ndev, "Failed to get TX PWR\n");

	complete(&hif_wait_response);
}

static int handle_set_antenna_mode(struct wilc_vif *vif, struct host_if_set_ant
				   *set_ant)
{
	int ret = 0;
	struct wid wid;
	sysfs_attr_group *attr_syfs_p = &vif->attr_sysfs;

	wid.id = (u16)WID_ANTENNA_SELECTION;
	wid.type = WID_BIN;
	wid.val = (u8 *)set_ant;
	wid.size = sizeof(struct host_if_set_ant);

	if (attr_syfs_p->ant_swtch_mode == ANT_SWTCH_SNGL_GPIO_CTRL)
		PRINT_INFO(vif->ndev, CFG80211_DBG, "set antenna %d on GPIO %d\n",set_ant->mode,set_ant->antenna1);
	else if (attr_syfs_p->ant_swtch_mode == ANT_SWTCH_DUAL_GPIO_CTRL)
		PRINT_INFO(vif->ndev, CFG80211_DBG, "set antenna %d on GPIOs %d and %d\n",set_ant->mode,set_ant->antenna1,set_ant->antenna2);

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));
	if(ret)
		PRINT_ER(vif->ndev, "Failed to set antenna mode\n");

	return ret;
}

static void host_if_work(struct work_struct *work)
{
	struct host_if_msg *msg;
	struct wilc *wilc;
	int ret = 0;

	msg = container_of(work, struct host_if_msg, work);
	wilc = msg->vif->wilc;

	if (msg->id == HOST_IF_MSG_CONNECT &&
	    msg->vif->hif_drv->usr_scan_req.scan_result) {
		wilc_enqueue_cmd(msg);
		usleep_range(2 * 1000, 2 * 1000);
		goto free_msg;
	}
	switch (msg->id) {
	case HOST_IF_MSG_SCAN:
		handle_scan(msg->vif, &msg->body.scan_info);
		break;

	case HOST_IF_MSG_CONNECT:
		handle_connect(msg->vif, &msg->body.con_info);
		break;

	case HOST_IF_MSG_RCVD_NTWRK_INFO:
		handle_rcvd_ntwrk_info(msg->vif, &msg->body.net_info);
		break;

	case HOST_IF_MSG_RCVD_GNRL_ASYNC_INFO:
		handle_rcvd_gnrl_async_info(msg->vif,
					    &msg->body.async_info);
		break;

	case HOST_IF_MSG_KEY:
		handle_key(msg->vif, &msg->body.key_info);
		break;

	case HOST_IF_MSG_CFG_PARAMS:
		handle_cfg_param(msg->vif, &msg->body.cfg_info);
		break;

	case HOST_IF_MSG_SET_CHANNEL:
		handle_set_channel(msg->vif, &msg->body.channel_info);
		break;

	case HOST_IF_MSG_DISCONNECT:
		handle_disconnect(msg->vif);
		break;

	case HOST_IF_MSG_RCVD_SCAN_COMPLETE:
		del_timer(&msg->vif->hif_drv->scan_timer);
		PRINT_INFO(msg->vif->ndev, HOSTINF_DBG, "scan completed successfully\n");

		handle_scan_done(msg->vif, SCAN_EVENT_DONE);

		if (msg->vif->hif_drv->remain_on_ch_pending)
			handle_remain_on_chan(msg->vif,
					      &msg->body.remain_on_ch);

		break;

	case HOST_IF_MSG_GET_RSSI:
		handle_get_rssi(msg->vif);
		break;

	case HOST_IF_MSG_GET_STATISTICS:
		handle_get_statistics(msg->vif,
				      (struct rf_info *)msg->body.data);
		break;

	case HOST_IF_MSG_ADD_BEACON:
		handle_add_beacon(msg->vif, &msg->body.beacon_info);
		break;

	case HOST_IF_MSG_DEL_BEACON:
		handle_del_beacon(msg->vif);
		break;

	case HOST_IF_MSG_ADD_STATION:
		handle_add_station(msg->vif, &msg->body.add_sta_info);
		break;

	case HOST_IF_MSG_DEL_STATION:
		handle_del_station(msg->vif, &msg->body.del_sta_info);
		break;

	case HOST_IF_MSG_EDIT_STATION:
		handle_edit_station(msg->vif, &msg->body.edit_sta_info);
		break;

	case HOST_IF_MSG_GET_INACTIVETIME:
		handle_get_inactive_time(msg->vif, &msg->body.mac_info);
		break;

	case HOST_IF_MSG_SCAN_TIMER_FIRED:
		PRINT_D(msg->vif->ndev, HOSTINF_DBG, "Scan Timeout\n");
		handle_scan_done(msg->vif, SCAN_EVENT_ABORTED);
		break;

	case HOST_IF_MSG_CONNECT_TIMER_FIRED:
		PRINT_D(msg->vif->ndev, HOSTINF_DBG, "Connect Timeout\n");
		handle_connect_timeout(msg->vif);
		break;

	case HOST_IF_MSG_POWER_MGMT:
		handle_power_management(msg->vif,
					&msg->body.pwr_mgmt_info);
		break;

	case HOST_IF_MSG_SET_WFIDRV_HANDLER:
		ret = handle_set_wfi_drv_handler(msg->vif, &msg->body.drv);
		break;

	case HOST_IF_MSG_SET_OPERATION_MODE:
		handle_set_operation_mode(msg->vif, &msg->body.mode);
		break;

	case HOST_IF_MSG_SET_MAC_ADDRESS:
		handle_set_mac_address(msg->vif, &msg->body.dev_mac_info);
		break;

	case HOST_IF_MSG_GET_MAC_ADDRESS:
		handle_get_mac_address(msg->vif, &msg->body.dev_mac_info);
		break;

	case HOST_IF_MSG_REMAIN_ON_CHAN:
		PRINT_INFO(msg->vif->ndev, HOSTINF_DBG, "HOST_IF_MSG_REMAIN_ON_CHAN\n");
		handle_remain_on_chan(msg->vif, &msg->body.remain_on_ch);
		break;

	case HOST_IF_MSG_REGISTER_FRAME:
		PRINT_INFO(msg->vif->ndev, HOSTINF_DBG, "HOST_IF_MSG_REGISTER_FRAME\n");
		handle_register_frame(msg->vif, &msg->body.reg_frame);
		break;

	case HOST_IF_MSG_LISTEN_TIMER_FIRED:
		handle_listen_state_expired(msg->vif, &msg->body.remain_on_ch);
		break;

	case HOST_IF_MSG_SET_MULTICAST_FILTER:
		PRINT_INFO(msg->vif->ndev, HOSTINF_DBG, "HOST_IF_MSG_SET_MULTICAST_FILTER\n");
		handle_set_mcast_filter(msg->vif, &msg->body.multicast_info);
		break;

	case HOST_IF_MSG_DEL_ALL_STA:
		handle_del_all_sta(msg->vif, &msg->body.del_all_sta_info);
		break;

	case HOST_IF_MSG_SET_TX_POWER:
		handle_set_tx_pwr(msg->vif, msg->body.tx_power.tx_pwr);
		break;

	case HOST_IF_MSG_GET_TX_POWER:
		handle_get_tx_pwr(msg->vif, &msg->body.tx_power.tx_pwr);
		break;

	case HOST_IF_MSG_SEND_BUFFERED_EAP:
		handle_send_buffered_eap(msg->vif,
					 &msg->body.send_buff_eap);
		break;

	case HOST_IF_MSG_SET_ANTENNA_MODE:
		handle_set_antenna_mode(msg->vif, &msg->body.set_ant);
		break;

	case HOST_IF_MSG_SET_WOWLAN_TRIGGER:
		handle_set_wowlan_trigger(msg->vif,msg->body.wow_trigger.wowlan_trigger);
		break;

	default:
		PRINT_ER(msg->vif->ndev, "[Host Interface] undefined\n");
		break;
	}
free_msg:
	if (ret)
		netdev_err(msg->vif->ndev, "Host cmd %d failed\n", msg->id);
	kfree(msg);
	PRINT_INFO(msg->vif->ndev, HOSTINF_DBG, "Releasing thread exit completion\n");
	complete(&hif_thread_comp);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
static void timer_scan_cb(struct timer_list *t)
#else
static void timer_scan_cb(unsigned long arg)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
	struct host_if_drv *hif_drv = from_timer(hif_drv, t, scan_timer);
#else
	struct host_if_drv *hif_drv = (struct host_if_drv *)arg;
#endif
	struct wilc_vif *vif = hif_drv->scan_timer_vif;
	struct host_if_msg msg;

	memset(&msg, 0, sizeof(struct host_if_msg));
	msg.vif = vif;
	msg.id = HOST_IF_MSG_SCAN_TIMER_FIRED;

	wilc_enqueue_cmd(&msg);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
static void timer_connect_cb(struct timer_list *t)
#else
static void timer_connect_cb(unsigned long arg)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
	struct host_if_drv *hif_drv = from_timer(hif_drv, t, connect_timer);
#else
	struct host_if_drv *hif_drv = (struct host_if_drv *)arg;
#endif
	struct wilc_vif *vif = hif_drv->connect_timer_vif;
	struct host_if_msg msg;

	memset(&msg, 0, sizeof(struct host_if_msg));
	msg.vif = vif;
	msg.id = HOST_IF_MSG_CONNECT_TIMER_FIRED;

	wilc_enqueue_cmd(&msg);
}

signed int wilc_send_buffered_eap(struct wilc_vif *vif,
				  wilc_frmw_to_linux_t frmw_to_linux,
				  free_eap_buf_param eap_buf_param,
				  u8 *buff, unsigned int size,
				  unsigned int pkt_offset,
				  void *user_arg)
{
	int result = 0;
	struct host_if_msg msg;

	if (!vif || !frmw_to_linux || !eap_buf_param ){
		return -EFAULT;
	}
	memset(&msg, 0, sizeof(struct host_if_msg));
	msg.id = HOST_IF_MSG_SEND_BUFFERED_EAP;
	msg.vif = vif;
	msg.body.send_buff_eap.frmw_to_linux = frmw_to_linux;
	msg.body.send_buff_eap.eap_buf_param = eap_buf_param;
	msg.body.send_buff_eap.size = size;
	msg.body.send_buff_eap.pkt_offset = pkt_offset;
	msg.body.send_buff_eap.buff = kmalloc(size + pkt_offset,
						  GFP_ATOMIC);
	memcpy(msg.body.send_buff_eap.buff, buff, size + pkt_offset);
	msg.body.send_buff_eap.user_arg = user_arg;

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "Coud not send EAP\n");
	return result;
}

s32 wilc_remove_key(struct wilc_vif *vif, const u8 *sta_addr)
{
	struct wid wid;

	wid.id = (u16)WID_REMOVE_KEY;
	wid.type = WID_STR;
	wid.val = (s8 *)sta_addr;
	wid.size = 6;

	return 0;
}

int wilc_remove_wep_key(struct wilc_vif *vif, u8 index)
{
	int result = 0;
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		result = -EFAULT;
		PRINT_ER(vif->ndev, "Driver is null\n");
		return result;
	}

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_KEY;
	msg.body.key_info.type = WEP;
	msg.body.key_info.action = REMOVEKEY;
	msg.vif = vif;
	msg.body.key_info.attr.wep.index = index;

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "Request to remove WEP key\n");
	else
		wait_for_completion(&hif_drv->comp_test_key_block);

	return result;
}

int wilc_set_wep_default_keyid(struct wilc_vif *vif, u8 index)
{
	int result = 0;
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		result = -EFAULT;
		PRINT_ER(vif->ndev, "driver is null\n");
		return result;
	}

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_KEY;
	msg.body.key_info.type = WEP;
	msg.body.key_info.action = DEFAULTKEY;
	msg.vif = vif;
	msg.body.key_info.attr.wep.index = index;

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "Default key index\n");
	else
		wait_for_completion(&hif_drv->comp_test_key_block);

	return result;
}

int wilc_add_wep_key_bss_sta(struct wilc_vif *vif, const u8 *key, u8 len,
			     u8 index)
{
	int result;
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "driver is null\n");
		return -EFAULT;
	}

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_KEY;
	msg.body.key_info.type = WEP;
	msg.body.key_info.action = ADDKEY;
	msg.vif = vif;
	msg.body.key_info.attr.wep.key = kmemdup(key, len, GFP_KERNEL);
	if (!msg.body.key_info.attr.wep.key)
		return -ENOMEM;

	msg.body.key_info.attr.wep.key_len = len;
	msg.body.key_info.attr.wep.index = index;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "STA - WEP Key\n");
		kfree(msg.body.key_info.attr.wep.key);
		return result;
	}

	wait_for_completion(&hif_drv->comp_test_key_block);
	return 0;
}

int wilc_add_wep_key_bss_ap(struct wilc_vif *vif, const u8 *key, u8 len,
			    u8 index, u8 mode, enum AUTHTYPE auth_type)
{
	int result;
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "driver is null\n");
		return -EFAULT;
	}

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_KEY;
	msg.body.key_info.type = WEP;
	msg.body.key_info.action = ADDKEY_AP;
	msg.vif = vif;
	msg.body.key_info.attr.wep.key = kmemdup(key, len, GFP_KERNEL);
	if (!msg.body.key_info.attr.wep.key)
		return -ENOMEM;

	msg.body.key_info.attr.wep.key_len = len;
	msg.body.key_info.attr.wep.index = index;
	msg.body.key_info.attr.wep.mode = mode;
	msg.body.key_info.attr.wep.auth_type = auth_type;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "AP - WEP Key\n");
		kfree(msg.body.key_info.attr.wep.key);
		return result;
	}

	wait_for_completion(&hif_drv->comp_test_key_block);
	return 0;
}

int wilc_add_ptk(struct wilc_vif *vif, const u8 *ptk, u8 ptk_key_len,
		 const u8 *mac_addr, const u8 *rx_mic, const u8 *tx_mic,
		 u8 mode, u8 cipher_mode, u8 index)
{
	int result;
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;
	u8 key_len = ptk_key_len;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "driver is null\n");
		return -EFAULT;
	}

	if (rx_mic)
		key_len += RX_MIC_KEY_LEN;

	if (tx_mic)
		key_len += TX_MIC_KEY_LEN;

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_KEY;
	msg.body.key_info.type = WPA_PTK;
	if (mode == AP_MODE) {
		msg.body.key_info.action = ADDKEY_AP;
		msg.body.key_info.attr.wpa.index = index;
	}
	if (mode == STATION_MODE)
		msg.body.key_info.action = ADDKEY;

	msg.body.key_info.attr.wpa.key = kmemdup(ptk, ptk_key_len, GFP_KERNEL);
	if (!msg.body.key_info.attr.wpa.key)
		return -ENOMEM;

	if (rx_mic)
		memcpy(msg.body.key_info.attr.wpa.key + 16, rx_mic,
		       RX_MIC_KEY_LEN);

	if (tx_mic)
		memcpy(msg.body.key_info.attr.wpa.key + 24, tx_mic,
		       TX_MIC_KEY_LEN);

	msg.body.key_info.attr.wpa.key_len = key_len;
	msg.body.key_info.attr.wpa.mac_addr = mac_addr;
	msg.body.key_info.attr.wpa.mode = cipher_mode;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "PTK Key\n");
		kfree(msg.body.key_info.attr.wpa.key);
		return result;
	}

	wait_for_completion(&hif_drv->comp_test_key_block);
	return 0;
}

int wilc_add_rx_gtk(struct wilc_vif *vif, const u8 *rx_gtk, u8 gtk_key_len,
		    u8 index, u32 key_rsc_len, const u8 *key_rsc,
		    const u8 *rx_mic, const u8 *tx_mic, u8 mode,
		    u8 cipher_mode)
{
	int result;
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;
	u8 key_len = gtk_key_len;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "driver is null\n");
		return -EFAULT;
	}
	memset(&msg, 0, sizeof(struct host_if_msg));

	if (rx_mic)
		key_len += RX_MIC_KEY_LEN;

	if (tx_mic)
		key_len += TX_MIC_KEY_LEN;

	if (key_rsc) {
		msg.body.key_info.attr.wpa.seq = kmemdup(key_rsc,
							 key_rsc_len,
							 GFP_KERNEL);
		if (!msg.body.key_info.attr.wpa.seq)
			return -ENOMEM;
	}

	msg.id = HOST_IF_MSG_KEY;
	msg.body.key_info.type = WPA_RX_GTK;
	msg.vif = vif;

	if (mode == AP_MODE) {
		msg.body.key_info.action = ADDKEY_AP;
		msg.body.key_info.attr.wpa.mode = cipher_mode;
	}
	if (mode == STATION_MODE)
		msg.body.key_info.action = ADDKEY;

	msg.body.key_info.attr.wpa.key = kmemdup(rx_gtk,
						 key_len,
						 GFP_KERNEL);
	if (!msg.body.key_info.attr.wpa.key) {
		kfree(msg.body.key_info.attr.wpa.seq);
		return -ENOMEM;
	}

	if (rx_mic)
		memcpy(msg.body.key_info.attr.wpa.key + 16, rx_mic,
		       RX_MIC_KEY_LEN);

	if (tx_mic)
		memcpy(msg.body.key_info.attr.wpa.key + 24, tx_mic,
		       TX_MIC_KEY_LEN);

	msg.body.key_info.attr.wpa.index = index;
	msg.body.key_info.attr.wpa.key_len = key_len;
	msg.body.key_info.attr.wpa.seq_len = key_rsc_len;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "RX GTK\n");
		kfree(msg.body.key_info.attr.wpa.seq);
		kfree(msg.body.key_info.attr.wpa.key);
		return result;
	}

	wait_for_completion(&hif_drv->comp_test_key_block);
	return 0;
}

int wilc_set_pmkid_info(struct wilc_vif *vif,
			struct host_if_pmkid_attr *pmkid)
{
	int result = 0;
	struct host_if_msg msg;
	int i;

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_KEY;
	msg.body.key_info.type = PMKSA;
	msg.body.key_info.action = ADDKEY;
	msg.vif = vif;

	for (i = 0; i < pmkid->numpmkid; i++) {
		memcpy(msg.body.key_info.attr.pmkid.pmkidlist[i].bssid,
		       &pmkid->pmkidlist[i].bssid, ETH_ALEN);
		memcpy(msg.body.key_info.attr.pmkid.pmkidlist[i].pmkid,
		       &pmkid->pmkidlist[i].pmkid, PMKID_LEN);
	}

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "PMKID Info\n");

	return result;
}

int wilc_get_mac_address(struct wilc_vif *vif, u8 *mac_addr)
{
	int result = 0;
	struct host_if_msg msg;

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_GET_MAC_ADDRESS;
	msg.body.dev_mac_info.mac_addr = mac_addr;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "Failed to send get mac address\n");
		return -EFAULT;
	}

	wait_for_completion(&hif_wait_response);
	return result;
}

int wilc_set_mac_address(struct wilc_vif *vif, u8 *mac_addr)
{
	int result;
	struct host_if_msg msg;

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_SET_MAC_ADDRESS;
	msg.body.dev_mac_info.mac_addr = mac_addr;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "Failed to send get mac address\n");
		return -EFAULT;
	}

	wait_for_completion(&hif_wait_response);
	return result;
}

int wilc_set_join_req(struct wilc_vif *vif, u8 *bssid, const u8 *ssid,
		      size_t ssid_len, const u8 *ies, size_t ies_len,
		      wilc_connect_result connect_result, void *user_arg,
		      u8 security, enum AUTHTYPE auth_type,
		      u8 channel, void *join_params)
{
	int result = 0;
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv || !connect_result) {
		PRINT_ER(vif->ndev, "Driver is null\n");
		return -EFAULT;
	}

	if (!join_params) {
		PRINT_ER(vif->ndev, "Unable to Join - JoinParams is NULL\n");
		return -EFAULT;
	}

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_CONNECT;

	msg.body.con_info.security = security;
	msg.body.con_info.auth_type = auth_type;
	msg.body.con_info.ch = channel;
	msg.body.con_info.result = connect_result;
	msg.body.con_info.arg = user_arg;
	msg.body.con_info.params = join_params;
	msg.vif = vif;

	if (bssid) {
		msg.body.con_info.bssid = kmemdup(bssid, 6, GFP_KERNEL);
		if (!msg.body.con_info.bssid)
			return -ENOMEM;
	}

	if (ssid) {
		msg.body.con_info.ssid_len = ssid_len;
		msg.body.con_info.ssid = kmemdup(ssid, ssid_len, GFP_KERNEL);
		if (!msg.body.con_info.ssid)
			return -ENOMEM;
	}

	if (ies) {
		msg.body.con_info.ies_len = ies_len;
		msg.body.con_info.ies = kmemdup(ies, ies_len, GFP_KERNEL);
		if (!msg.body.con_info.ies)
			return -ENOMEM;
	}
	if (hif_drv->hif_state < HOST_IF_CONNECTING)
		hif_drv->hif_state = HOST_IF_CONNECTING;
	else
		PRINT_INFO(vif->ndev, GENERIC_DBG, "Don't set state to 'connecting' as state is %d\n", hif_drv->hif_state);

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "send message: Set join request\n");
		return -EFAULT;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
	hif_drv->connect_timer.data = (unsigned long)hif_drv;
#endif
	hif_drv->connect_timer_vif = vif;
	mod_timer(&hif_drv->connect_timer,
		  jiffies + msecs_to_jiffies(HOST_IF_CONNECT_TIMEOUT));

	return result;
}

int wilc_disconnect(struct wilc_vif *vif, u16 reason_code)
{
	int result = 0;
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "Driver is null\n");
		return -EFAULT;
	}

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_DISCONNECT;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "Failed to send message: disconnect\n");
	else
		wait_for_completion(&hif_drv->comp_test_disconn_block);

	return result;
}

static s32 host_int_get_assoc_res_info(struct wilc_vif *vif,
				       u8 *assoc_resp_info,
				       u32 max_assoc_resp_info_len,
				       u32 *rcvd_assoc_resp_info_len)
{
	s32 result = 0;
	struct wid wid;

	wid.id = (u16)WID_ASSOC_RES_INFO;
	wid.type = WID_STR;
	wid.val = assoc_resp_info;
	wid.size = max_assoc_resp_info_len;

	result = wilc_send_config_pkt(vif, GET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result) {
		*rcvd_assoc_resp_info_len = 0;
		PRINT_ER(vif->ndev, "Failed to send association response\n");
		return -EINVAL;
	}

	*rcvd_assoc_resp_info_len = wid.size;
	return result;
}

int wilc_set_mac_chnl_num(struct wilc_vif *vif, u8 channel)
{
	int result;
	struct host_if_msg msg;

	memset(&msg, 0, sizeof(struct host_if_msg));
	msg.id = HOST_IF_MSG_SET_CHANNEL;
	msg.body.channel_info.set_ch = channel;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "wilc mq send fail\n");
		return -EINVAL;
	}

	return 0;
}

int wilc_set_wfi_drv_handler(struct wilc_vif *vif, int index, u8 mode,
			     u8 ifc_id)
{
	int result = 0;
	struct host_if_msg msg;

	memset(&msg, 0, sizeof(struct host_if_msg));
	msg.id = HOST_IF_MSG_SET_WFIDRV_HANDLER;
	msg.body.drv.handler = index;
	msg.body.drv.mode = mode;
	msg.body.drv.ifc_id = ifc_id;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "wilc mq send fail\n");
		result = -EINVAL;
	}

	return result;
}

int wilc_set_operation_mode(struct wilc_vif *vif, u32 mode)
{
	int result = 0;
	struct host_if_msg msg;

	memset(&msg, 0, sizeof(struct host_if_msg));
	msg.id = HOST_IF_MSG_SET_OPERATION_MODE;
	msg.body.mode.mode = mode;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "wilc mq send fail\n");
		result = -EINVAL;
	}

	return result;
}

s32 wilc_get_inactive_time(struct wilc_vif *vif, const u8 *mac,
			   u32 *out_val)
{
	s32 result = 0;
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "driver is null\n");
		return -EFAULT;
	}

	memset(&msg, 0, sizeof(struct host_if_msg));
	memcpy(msg.body.mac_info.mac, mac, ETH_ALEN);

	msg.id = HOST_IF_MSG_GET_INACTIVETIME;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "Failed to send get host ch param\n");
	else
		wait_for_completion(&hif_drv->comp_inactive_time);

	*out_val = inactive_time;

	return result;
}

int wilc_get_rssi(struct wilc_vif *vif, s8 *rssi_level)
{
	int result = 0;
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	memset(&msg, 0, sizeof(struct host_if_msg));
	msg.id = HOST_IF_MSG_GET_RSSI;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "Failed to send get host ch param\n");
		return -EFAULT;
	}

	wait_for_completion(&hif_drv->comp_get_rssi);

	if (!rssi_level) {
		PRINT_ER(vif->ndev, "RSS pointer value is null\n");
		return -EFAULT;
	}

	*rssi_level = rssi;

	return result;
}

int wilc_get_statistics(struct wilc_vif *vif, struct rf_info *stats)
{
	int result = 0;
	struct host_if_msg msg;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, " wilc_get_statistics \n");
	memset(&msg, 0, sizeof(struct host_if_msg));
	msg.id = HOST_IF_MSG_GET_STATISTICS;
	msg.body.data = (char *)stats;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "Failed to send get host channel\n");
		return -EFAULT;
	}

	if (stats != &vif->wilc->dummy_statistics)
		wait_for_completion(&hif_wait_response);
	return result;
}

int wilc_scan(struct wilc_vif *vif, u8 scan_source, u8 scan_type,
	      u8 *ch_freq_list, u8 ch_list_len, const u8 *ies,
	      size_t ies_len, wilc_scan_result scan_result, void *user_arg,
	      struct hidden_network *hidden_network)
{
	int result = 0;
	struct host_if_msg msg;
	struct scan_attr *scan_info = &msg.body.scan_info;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv || !scan_result) {
		PRINT_ER(vif->ndev, "hif_drv or scan_result = NULL\n");
		return -EFAULT;
	}

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_SCAN;

	if (hidden_network) {
		scan_info->hidden_network.net_info = hidden_network->net_info;
		scan_info->hidden_network.n_ssids = hidden_network->n_ssids;
	} else {
		PRINT_WRN(vif->ndev, HOSTINF_DBG, "hidden_network IS EQUAL TO NULL\n");
	}

	msg.vif = vif;
	scan_info->src = scan_source;
	scan_info->type = scan_type;
	scan_info->result = scan_result;
	scan_info->arg = user_arg;

	scan_info->ch_list_len = ch_list_len;
	scan_info->ch_freq_list = kmemdup(ch_freq_list,
					  ch_list_len,
					  GFP_KERNEL);
	if (!scan_info->ch_freq_list)
		return -ENOMEM;

	scan_info->ies_len = ies_len;
	scan_info->ies = kmemdup(ies, ies_len, GFP_KERNEL);
	if (!scan_info->ies)
		return -ENOMEM;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "Error in sending message queue\n");
		return -EINVAL;
	}

	PRINT_INFO(vif->ndev, HOSTINF_DBG, ">> Starting the SCAN timer\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
	hif_drv->scan_timer.data = (unsigned long)hif_drv;
#endif
	hif_drv->scan_timer_vif = vif;
	mod_timer(&hif_drv->scan_timer,
		  jiffies + msecs_to_jiffies(HOST_IF_SCAN_TIMEOUT));

	return result;
}

int wilc_hif_set_cfg(struct wilc_vif *vif,
		     struct cfg_param_attr *cfg_param)
{
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif_drv NULL\n");
		return -EFAULT;
	}

	memset(&msg, 0, sizeof(struct host_if_msg));
	msg.id = HOST_IF_MSG_CFG_PARAMS;
	msg.body.cfg_info = *cfg_param;
	msg.vif = vif;

	return wilc_enqueue_cmd(&msg);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
static void get_periodic_rssi(struct timer_list *unused)
#else
static void get_periodic_rssi(unsigned long arg)
#endif
{
	struct wilc_vif *vif = periodic_rssi_vif;

	if (!vif->hif_drv) {
		PRINT_ER(vif->ndev, "Driver handler is NULL\n");
		return;
	}

	if (vif->hif_drv->hif_state == HOST_IF_CONNECTED)
		wilc_get_statistics(vif, &vif->wilc->dummy_statistics);

	mod_timer(&periodic_rssi, jiffies + msecs_to_jiffies(5000));
}

int wilc_init(struct net_device *dev, struct host_if_drv **hif_drv_handler)
{
	struct host_if_drv *hif_drv;
	struct wilc_vif *vif;
	struct wilc *wilc;
	int i;

	vif = netdev_priv(dev);
	wilc = vif->wilc;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Initializing host interface for client %d\n",
		   clients_count + 1);

	hif_drv  = kzalloc(sizeof(*hif_drv), GFP_KERNEL);
	if (!hif_drv) {
		PRINT_ER(dev, "Driver is null\n");
		return -ENOMEM;
	}
	*hif_drv_handler = hif_drv;
	for (i = 0; i <= wilc->vif_num; i++)
		if (dev == wilc->vif[i]->ndev) {
			wilc->vif[i]->hif_drv = hif_drv;
			hif_drv->driver_handler_id = i + 1;
			break;
		}
	
#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP

	wilc_optaining_ip = false;
#endif

	if (clients_count == 0)	{
		init_completion(&hif_thread_comp);
		init_completion(&hif_driver_comp);
		mutex_init(&hif_deinit_lock);
	}

	init_completion(&hif_wait_response);
	init_completion(&hif_drv->comp_test_key_block);
	init_completion(&hif_drv->comp_test_disconn_block);
	init_completion(&hif_drv->comp_get_rssi);
	init_completion(&hif_drv->comp_inactive_time);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "INIT: CLIENT COUNT %d\n", 
				clients_count);
	if (clients_count == 0) {
		hif_workqueue = create_singlethread_workqueue("WILC_wq");
		if (!hif_workqueue) {
			PRINT_ER(vif->ndev, "Failed to create workqueue\n");
			return -ENOMEM;
		}

		periodic_rssi_vif = vif;
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
		timer_setup(&periodic_rssi, get_periodic_rssi, 0);
	#else
		setup_timer(&periodic_rssi, get_periodic_rssi, (unsigned long)vif);
	#endif
		mod_timer(&periodic_rssi, jiffies + msecs_to_jiffies(5000));
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
	timer_setup(&hif_drv->scan_timer, timer_scan_cb, 0);
	timer_setup(&hif_drv->connect_timer, timer_connect_cb, 0);
	timer_setup(&hif_drv->remain_on_ch_timer, listen_timer_cb, 0);
#else
	setup_timer(&hif_drv->scan_timer, timer_scan_cb, 0);
	setup_timer(&hif_drv->connect_timer, timer_connect_cb, 0);
	setup_timer(&hif_drv->remain_on_ch_timer, listen_timer_cb, 0);
#endif

	mutex_init(&hif_drv->cfg_values_lock);
	mutex_lock(&hif_drv->cfg_values_lock);

	hif_drv->hif_state = HOST_IF_IDLE;
	hif_drv->cfg_values.site_survey_enabled = SITE_SURVEY_OFF;
	hif_drv->cfg_values.scan_source = DEFAULT_SCAN;
	hif_drv->cfg_values.active_scan_time = ACTIVE_SCAN_TIME;
	hif_drv->cfg_values.passive_scan_time = PASSIVE_SCAN_TIME;
	hif_drv->cfg_values.curr_tx_rate = AUTORATE;

	hif_drv->p2p_timeout = 0;

	PRINT_D(vif->ndev, HOSTINF_DBG,"Initialization values, Site survey value: %d\nScan source: %d\nActive scan time: %d\nPassive scan time: %d\nCurrent tx Rate = %d\n",
		hif_drv->cfg_values.site_survey_enabled,
		hif_drv->cfg_values.scan_source,
		hif_drv->cfg_values.active_scan_time,
		hif_drv->cfg_values.passive_scan_time,
		hif_drv->cfg_values.curr_tx_rate);
	mutex_unlock(&hif_drv->cfg_values_lock);

	clients_count++;

	return 0;
}

int wilc_deinit(struct wilc_vif *vif)
{
	int result = 0;
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv)	{
		PRINT_ER(vif->ndev, "hif_drv = NULL\n");
		return -EFAULT;
	}

	mutex_lock(&hif_deinit_lock);

	terminated_handle = hif_drv;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, 
		   "De-initializing host interface for client %d\n",
		   clients_count);

	del_timer_sync(&hif_drv->scan_timer);
	del_timer_sync(&hif_drv->connect_timer);
	del_timer_sync(&periodic_rssi);
	del_timer_sync(&hif_drv->remain_on_ch_timer);

	wilc_set_wfi_drv_handler(vif, 0, vif->iftype, vif->ifc_id);
	wait_for_completion(&hif_driver_comp);

	if (hif_drv->usr_scan_req.scan_result) {
		hif_drv->usr_scan_req.scan_result(SCAN_EVENT_ABORTED, NULL,
						  hif_drv->usr_scan_req.arg,
						  NULL);
		hif_drv->usr_scan_req.scan_result = NULL;
	}

	hif_drv->hif_state = HOST_IF_IDLE;

	memset(wilc_connected_ssid, 0, ETH_ALEN);

	memset(&msg, 0, sizeof(struct host_if_msg));

	if (clients_count == 1)	{
		msg.id = HOST_IF_MSG_EXIT;
		msg.vif = vif;

		result = wilc_enqueue_cmd(&msg);
		if (result != 0)
			PRINT_ER(vif->ndev, "deinit : Error(%d)\n", result);
		else
			wait_for_completion(&hif_thread_comp);
		flush_workqueue(hif_workqueue);
		destroy_workqueue(hif_workqueue);
		hif_workqueue = NULL;
	}

	kfree(hif_drv);

	clients_count--;
	terminated_handle = NULL;
	mutex_unlock(&hif_deinit_lock);
	return result;
}

void wilc_network_info_received(struct wilc *wilc, u8 *buffer, u32 length)
{
	s32 result = 0;
	struct host_if_msg msg;
	int id;
	struct host_if_drv *hif_drv = NULL;
	struct wilc_vif *vif;

	id = buffer[length - 4];
	id |= (buffer[length - 3] << 8);
	id |= (buffer[length - 2] << 16);
	id |= (buffer[length - 1] << 24);
	vif = wilc_get_vif_from_idx(wilc, id);
	if (!vif)
		return;
	hif_drv = vif->hif_drv;

	if (!hif_drv || hif_drv == terminated_handle)	{
		PRINT_ER(vif->ndev, "driver not init[%p]\n", hif_drv);
		return;
	}

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_RCVD_NTWRK_INFO;
	msg.vif = vif;

	msg.body.net_info.len = length;
	msg.body.net_info.buffer = kmemdup(buffer, length, GFP_KERNEL);
	if (!msg.body.net_info.buffer)
		return;

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "message parameters (%d)\n", result);
		kfree(msg.body.net_info.buffer);
	}
}

void wilc_gnrl_async_info_received(struct wilc *wilc, u8 *buffer, u32 length)
{
	s32 result = 0;
	struct host_if_msg msg;
	int id;
	struct host_if_drv *hif_drv = NULL;
	struct wilc_vif *vif;

	mutex_lock(&hif_deinit_lock);

	id = buffer[length - 4];
	id |= (buffer[length - 3] << 8);
	id |= (buffer[length - 2] << 16);
	id |= (buffer[length - 1] << 24);
	vif = wilc_get_vif_from_idx(wilc, id);
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "General asynchronous info packet received\n");
	if (!vif) {
		mutex_unlock(&hif_deinit_lock);
		return;
	}

	hif_drv = vif->hif_drv;

	if (!hif_drv || hif_drv == terminated_handle) {
		PRINT_ER(vif->ndev, "hif_drv = NULL\n");
		mutex_unlock(&hif_deinit_lock);
		return;
	}

	if (!hif_drv->usr_conn_req.conn_result) {
		PRINT_ER(vif->ndev, "there is no current Connect Request\n");
		mutex_unlock(&hif_deinit_lock);
		return;
	}

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_RCVD_GNRL_ASYNC_INFO;
	msg.vif = vif;

	msg.body.async_info.len = length;
	msg.body.async_info.buffer = kmemdup(buffer, length, GFP_KERNEL);
	if (!msg.body.async_info.buffer) {
		mutex_unlock(&hif_deinit_lock);
		return;
	}

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "synchronous info (%d)\n", result);
		kfree(msg.body.async_info.buffer);
	}

	mutex_unlock(&hif_deinit_lock);
}

void wilc_scan_complete_received(struct wilc *wilc, u8 *buffer, u32 length)
{
	s32 result = 0;
	struct host_if_msg msg;
	int id;
	struct host_if_drv *hif_drv = NULL;
	struct wilc_vif *vif;

	id = buffer[length - 4];
	id |= buffer[length - 3] << 8;
	id |= buffer[length - 2] << 16;
	id |= buffer[length - 1] << 24;
	vif = wilc_get_vif_from_idx(wilc, id);
	if (!vif)
		return;
	hif_drv = vif->hif_drv;
	PRINT_INFO(vif->ndev, GENERIC_DBG, "Scan notification received\n");

	if (!hif_drv || hif_drv == terminated_handle) {
		PRINT_ER(vif->ndev, "hif_drv = NULL\n");
		return;
	}

	if (hif_drv->usr_scan_req.scan_result) {
		memset(&msg, 0, sizeof(struct host_if_msg));

		msg.id = HOST_IF_MSG_RCVD_SCAN_COMPLETE;
		msg.vif = vif;

		result = wilc_enqueue_cmd(&msg);
		if (result)
			PRINT_ER(vif->ndev, "complete param (%d)\n", result);
	}
}

int wilc_remain_on_channel(struct wilc_vif *vif, u32 session_id,
			   u32 duration, u16 chan,
			   wilc_remain_on_chan_expired expired,
			   wilc_remain_on_chan_ready ready,
			   void *user_arg)
{
	int result = 0;
	struct host_if_msg msg;

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_REMAIN_ON_CHAN;
	msg.body.remain_on_ch.ch = chan;
	msg.body.remain_on_ch.expired = expired;
	msg.body.remain_on_ch.ready = ready;
	msg.body.remain_on_ch.arg = user_arg;
	msg.body.remain_on_ch.duration = duration;
	msg.body.remain_on_ch.id = session_id;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "wilc mq send fail\n");

	return result;
}

int wilc_listen_state_expired(struct wilc_vif *vif, u32 session_id)
{
	int result = 0;
	struct host_if_msg msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "Driver is null\n");
		return -EFAULT;
	}

	del_timer(&hif_drv->remain_on_ch_timer);

	memset(&msg, 0, sizeof(struct host_if_msg));
	msg.id = HOST_IF_MSG_LISTEN_TIMER_FIRED;
	msg.vif = vif;
	msg.body.remain_on_ch.id = session_id;

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "wilc mq send fail\n");

	return result;
}

int wilc_frame_register(struct wilc_vif *vif, u16 frame_type, bool reg)
{
	int result = 0;
	struct host_if_msg msg;

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_REGISTER_FRAME;
	switch (frame_type) {
	case ACTION:
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "ACTION\n");
		msg.body.reg_frame.reg_id = ACTION_FRM_IDX;
		break;

	case PROBE_REQ:
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "PROBE REQ\n");
		msg.body.reg_frame.reg_id = PROBE_REQ_IDX;
		break;

	default:
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "Not valid frame type\n");
		break;
	}
	msg.body.reg_frame.frame_type = frame_type;
	msg.body.reg_frame.reg = reg;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "wilc mq send fail\n");

	return result;
}

int wilc_add_beacon(struct wilc_vif *vif, u32 interval, u32 dtim_period,
		    u32 head_len, u8 *head, u32 tail_len, u8 *tail)
{
	int result = 0;
	struct host_if_msg msg;
	struct beacon_attr *beacon_info = &msg.body.beacon_info;

	memset(&msg, 0, sizeof(struct host_if_msg));

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setting adding beacon message queue params\n");
	msg.id = HOST_IF_MSG_ADD_BEACON;
	msg.vif = vif;
	beacon_info->interval = interval;
	beacon_info->dtim_period = dtim_period;
	beacon_info->head_len = head_len;
	beacon_info->head = kmemdup(head, head_len, GFP_KERNEL);
	if (!beacon_info->head) {
		result = -ENOMEM;
		goto error;
	}
	beacon_info->tail_len = tail_len;

	if (tail_len > 0) {
		beacon_info->tail = kmemdup(tail, tail_len, GFP_KERNEL);
		if (!beacon_info->tail) {
			result = -ENOMEM;
			goto error;
		}
	} else {
		beacon_info->tail = NULL;
	}

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "wilc mq send fail\n");

error:
	if (result) {
		kfree(beacon_info->head);

		kfree(beacon_info->tail);
	}

	return result;
}

int wilc_del_beacon(struct wilc_vif *vif)
{
	int result = 0;
	struct host_if_msg msg;

	msg.id = HOST_IF_MSG_DEL_BEACON;
	msg.vif = vif;
	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting deleting beacon message queue params\n");

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "wilc_mq_send fail\n");

	return result;
}

int wilc_add_station(struct wilc_vif *vif, struct add_sta_param *sta_param)
{
	int result = 0;
	struct host_if_msg msg;
	struct add_sta_param *add_sta_info = &msg.body.add_sta_info;

	memset(&msg, 0, sizeof(struct host_if_msg));

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setting adding station message queue params\n");
	msg.id = HOST_IF_MSG_ADD_STATION;
	msg.vif = vif;

	memcpy(add_sta_info, sta_param, sizeof(struct add_sta_param));
	if (add_sta_info->rates_len > 0) {
		add_sta_info->rates = kmemdup(sta_param->rates,
					      add_sta_info->rates_len,
					      GFP_KERNEL);
		if (!add_sta_info->rates)
			return -ENOMEM;
	}

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "wilc_mq_send fail\n");
		kfree(add_sta_info->rates);
	}
	return result;
}

int wilc_del_station(struct wilc_vif *vif, const u8 *mac_addr)
{
	int result = 0;
	struct host_if_msg msg;
	struct del_sta *del_sta_info = &msg.body.del_sta_info;

	memset(&msg, 0, sizeof(struct host_if_msg));

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting deleting station message queue params\n");
	msg.id = HOST_IF_MSG_DEL_STATION;
	msg.vif = vif;

	if (!mac_addr)
		eth_broadcast_addr(del_sta_info->mac_addr);
	else
		memcpy(del_sta_info->mac_addr, mac_addr, ETH_ALEN);

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "wilc_mq_send fail\n");
	return result;
}

int wilc_del_allstation(struct wilc_vif *vif, u8 mac_addr[][ETH_ALEN])
{
	int result = 0;
	struct host_if_msg msg;
	struct del_all_sta *del_all_sta_info = &msg.body.del_all_sta_info;
	u8 zero_addr[ETH_ALEN] = {0};
	int i;
	u8 assoc_sta = 0;

	memset(&msg, 0, sizeof(struct host_if_msg));

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting deauthenticating station message queue params\n");
	msg.id = HOST_IF_MSG_DEL_ALL_STA;
	msg.vif = vif;

	for (i = 0; i < MAX_NUM_STA; i++) {
		if (memcmp(mac_addr[i], zero_addr, ETH_ALEN)) {
			memcpy(del_all_sta_info->del_all_sta[i], mac_addr[i],
			       ETH_ALEN);
			PRINT_INFO(vif->ndev,
				   CFG80211_DBG, "BSSID = %x%x%x%x%x%x\n",
				   del_all_sta_info->del_all_sta[i][0],
				   del_all_sta_info->del_all_sta[i][1],
				   del_all_sta_info->del_all_sta[i][2],
				   del_all_sta_info->del_all_sta[i][3],
				   del_all_sta_info->del_all_sta[i][4],
				   del_all_sta_info->del_all_sta[i][5]);
			assoc_sta++;
		}
	}
	if (!assoc_sta) {
		PRINT_INFO(vif->ndev, CFG80211_DBG, "NO ASSOCIATED STAS\n");
		return result;
	}

	del_all_sta_info->assoc_sta = assoc_sta;
	result = wilc_enqueue_cmd(&msg);

	if (result)
		PRINT_ER(vif->ndev, "wilc_mq_send fail\n");
	else
		wait_for_completion(&hif_wait_response);

	return result;
}

int wilc_edit_station(struct wilc_vif *vif,
		      struct add_sta_param *sta_param)
{
	int result = 0;
	struct host_if_msg msg;
	struct add_sta_param *add_sta_info = &msg.body.add_sta_info;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting editing station message queue params\n");
	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_EDIT_STATION;
	msg.vif = vif;

	memcpy(add_sta_info, sta_param, sizeof(struct add_sta_param));
	if (add_sta_info->rates_len > 0) {
		add_sta_info->rates = kmemdup(sta_param->rates,
					      add_sta_info->rates_len,
					      GFP_KERNEL);
		if (!add_sta_info->rates)
			return -ENOMEM;
	}

	result = wilc_enqueue_cmd(&msg);
	if (result) {
		PRINT_ER(vif->ndev, "wilc_mq_send fail\n");
		kfree(add_sta_info->rates);
	}

	return result;
}

int wilc_set_power_mgmt(struct wilc_vif *vif, bool enabled, u32 timeout)
{
	int result = 0;
	struct host_if_msg msg;
	struct power_mgmt_param *pwr_mgmt_info = &msg.body.pwr_mgmt_info;

	if (wilc_wlan_get_num_conn_ifcs(vif->wilc) == 2 && enabled)
		return 0;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "\n\n>> Setting PS to %d <<\n\n",
		   enabled);
	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_POWER_MGMT;
	msg.vif = vif;

	pwr_mgmt_info->enabled = enabled;
	pwr_mgmt_info->timeout = timeout;

	if(!hif_workqueue)
		return 0;

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "wilc_mq_send fail\n");
	return result;
}

int wilc_setup_multicast_filter(struct wilc_vif *vif, bool enabled,
				u32 count)
{
	int result = 0;
	struct host_if_msg msg;
	struct set_multicast *multicast_filter_param = &msg.body.multicast_info;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting Multicast Filter params\n");
	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_SET_MULTICAST_FILTER;
	msg.vif = vif;

	multicast_filter_param->enabled = enabled;
	multicast_filter_param->cnt = count;

	result = wilc_enqueue_cmd(&msg);
	if (result)
		PRINT_ER(vif->ndev, "wilc_mq_send fail\n");
	return result;
}

static void *host_int_parse_join_bss_param(struct network_info *info)
{
	struct join_bss_param *param = NULL;
	u8 *ies;
	u16 ies_len;
	u16 index = 0;
	u8 rates_no = 0;
	u8 ext_rates_no;
	u16 offset;
	u8 pcipher_cnt;
	u8 auth_cnt;
	u8 pcipher_total_cnt = 0;
	u8 auth_total_cnt = 0;
	u8 i, j;

	ies = info->ies;
	ies_len = info->ies_len;

	param = kzalloc(sizeof(*param), GFP_KERNEL);
	if (!param)
		return NULL;

	param->dtim_period = info->dtim_period;
	param->beacon_period = info->beacon_period;
	param->cap_info = info->cap_info;
	memcpy(param->bssid, info->bssid, 6);
	memcpy((u8 *)param->ssid, info->ssid, info->ssid_len + 1);
	param->ssid_len = info->ssid_len;
	memset(param->rsn_pcip_policy, 0xFF, 3);
	memset(param->rsn_auth_policy, 0xFF, 3);

	while (index < ies_len) {
		if (ies[index] == SUPP_RATES_IE) {
			rates_no = ies[index + 1];
			param->supp_rates[0] = rates_no;
			index += 2;

			for (i = 0; i < rates_no; i++)
				param->supp_rates[i + 1] = ies[index + i];

			index += rates_no;
		} else if (ies[index] == EXT_SUPP_RATES_IE) {
			ext_rates_no = ies[index + 1];
			if (ext_rates_no > (MAX_RATES_SUPPORTED - rates_no))
				param->supp_rates[0] = MAX_RATES_SUPPORTED;
			else
				param->supp_rates[0] += ext_rates_no;
			index += 2;
			for (i = 0; i < (param->supp_rates[0] - rates_no); i++)
				param->supp_rates[rates_no + i + 1] = ies[index + i];

			index += ext_rates_no;
		} else if (ies[index] == HT_CAPABILITY_IE) {
			param->ht_capable = true;
			index += ies[index + 1] + 2;
		} else if ((ies[index] == WMM_IE) &&
			   (ies[index + 2] == 0x00) && (ies[index + 3] == 0x50) &&
			   (ies[index + 4] == 0xF2) &&
			   (ies[index + 5] == 0x02) &&
			   ((ies[index + 6] == 0x00) || (ies[index + 6] == 0x01)) &&
			   (ies[index + 7] == 0x01)) {
			param->wmm_cap = true;

			if (ies[index + 8] & BIT(7))
				param->uapsd_cap = true;
			index += ies[index + 1] + 2;
		} else if ((ies[index] == P2P_IE) &&
			 (ies[index + 2] == 0x50) && (ies[index + 3] == 0x6f) &&
			 (ies[index + 4] == 0x9a) &&
			 (ies[index + 5] == 0x09) && (ies[index + 6] == 0x0c)) {
			u16 p2p_cnt;

			param->tsf = info->tsf_lo;
			param->noa_enabled = 1;
			param->idx = ies[index + 9];

			if (ies[index + 10] & BIT(7)) {
				param->opp_enabled = 1;
				param->ct_window = ies[index + 10];
			} else {
				param->opp_enabled = 0;
			}

			param->cnt = ies[index + 11];
			p2p_cnt = index + 12;

			memcpy(param->duration, ies + p2p_cnt, 4);
			p2p_cnt += 4;

			memcpy(param->interval, ies + p2p_cnt, 4);
			p2p_cnt += 4;

			memcpy(param->start_time, ies + p2p_cnt, 4);

			index += ies[index + 1] + 2;
		} else if ((ies[index] == RSN_IE) ||
			 ((ies[index] == WPA_IE) && (ies[index + 2] == 0x00) &&
			  (ies[index + 3] == 0x50) && (ies[index + 4] == 0xF2) &&
			  (ies[index + 5] == 0x01)))	{
			u16 rsn_idx = index;

			if (ies[rsn_idx] == RSN_IE)	{
				param->mode_802_11i = 2;
			} else {
				if (param->mode_802_11i == 0)
					param->mode_802_11i = 1;
				rsn_idx += 4;
			}

			rsn_idx += 7;
			param->rsn_grp_policy = ies[rsn_idx];
			rsn_idx++;
			offset = ies[rsn_idx] * 4;
			pcipher_cnt = (ies[rsn_idx] > 3) ? 3 : ies[rsn_idx];
			rsn_idx += 2;

			for (i = pcipher_total_cnt, j = 0; i < pcipher_cnt + pcipher_total_cnt && i < 3; i++, j++)
				param->rsn_pcip_policy[i] = ies[rsn_idx + ((j + 1) * 4) - 1];

			pcipher_total_cnt += pcipher_cnt;
			rsn_idx += offset;

			offset = ies[rsn_idx] * 4;

			auth_cnt = (ies[rsn_idx] > 3) ? 3 : ies[rsn_idx];
			rsn_idx += 2;

			for (i = auth_total_cnt, j = 0; i < auth_total_cnt + auth_cnt; i++, j++)
				param->rsn_auth_policy[i] = ies[rsn_idx + ((j + 1) * 4) - 1];

			auth_total_cnt += auth_cnt;
			rsn_idx += offset;

			if (ies[index] == RSN_IE) {
				param->rsn_cap[0] = ies[rsn_idx];
				param->rsn_cap[1] = ies[rsn_idx + 1];
				rsn_idx += 2;
			}
			param->rsn_found = true;
			index += ies[index + 1] + 2;
		} else {
			index += ies[index + 1] + 2;
		}
	}

	return (void *)param;
}

int wilc_set_tx_power(struct wilc_vif *vif, u8 tx_power)
{
	int ret = 0;
	struct host_if_msg msg;

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_SET_TX_POWER;
	msg.body.tx_power.tx_pwr = tx_power;
	msg.vif = vif;

	ret = wilc_enqueue_cmd(&msg);
	if (ret)
		PRINT_ER(vif->ndev, "wilc_mq_send fail\n");

	return ret;
}

int wilc_get_tx_power(struct wilc_vif *vif, u8 *tx_power)
{
	int ret = 0;
	struct host_if_msg msg;

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_GET_TX_POWER;
	msg.vif = vif;

	ret = wilc_enqueue_cmd(&msg);
	if (ret)
		PRINT_ER(vif->ndev, "Failed to get TX PWR\n");

	wait_for_completion(&hif_wait_response);
	*tx_power = msg.body.tx_power.tx_pwr;

	return ret;
}

bool is_valid_gpio(struct wilc_vif *vif, u8 gpio)
{
	switch(vif->wilc->chip) {
	case WILC_1000:
		if(gpio == 0 || gpio == 1 || gpio == 4 || gpio == 6)
			return true;
		else
			return false;
	case WILC_3000:
		if(gpio == 0 || gpio == 3 || gpio == 4 ||
			(gpio >= 17 && gpio <= 20))
			return true;
		else
			return false;
	default:
		return false;
	}
}

int wilc_set_antenna(struct wilc_vif *vif, u8 mode)
{
	int ret = 0;
	struct host_if_msg msg;
	sysfs_attr_group *attr_syfs_p;

	memset(&msg, 0, sizeof(struct host_if_msg));

	msg.id = HOST_IF_MSG_SET_ANTENNA_MODE;

	msg.vif = vif;
	msg.body.set_ant.mode = mode;
	attr_syfs_p = &vif->attr_sysfs;

	if(attr_syfs_p->ant_swtch_mode == ANT_SWTCH_INVALID_GPIO_CTRL) {
		PRINT_ER(vif->ndev, "Ant switch GPIO mode is invalid.\n");
		PRINT_ER(vif->ndev, "Set it using /sys/wilc/ant_swtch_mode\n");
		return WILC_FAIL;
	}

	if (is_valid_gpio(vif, attr_syfs_p->antenna1)) { 
		msg.body.set_ant.antenna1 = attr_syfs_p->antenna1;
	} else {
		PRINT_ER(vif->ndev, "Invalid GPIO%d\n", attr_syfs_p->antenna1);
		return WILC_FAIL;
	}

	if (attr_syfs_p->ant_swtch_mode == ANT_SWTCH_DUAL_GPIO_CTRL) {
		if ((attr_syfs_p->antenna2 != attr_syfs_p->antenna1) &&
			is_valid_gpio(vif, attr_syfs_p->antenna2)) {
			msg.body.set_ant.antenna2 = attr_syfs_p->antenna2;
		} else {
			PRINT_ER(vif->ndev, "Invalid GPIO %d\n", 
				 attr_syfs_p->antenna2);
			return WILC_FAIL;
		}
	}

	msg.body.set_ant.gpio_mode = attr_syfs_p->ant_swtch_mode;
	ret = wilc_enqueue_cmd(&msg);
	if(ret) {
		PRINT_ER(vif->ndev,
			 "Failed to send get host channel param's message queue ");
		return -EINVAL;
	}	
	return ret;
}

int host_int_set_wowlan_trigger(struct wilc_vif *vif, u8 wowlan_trigger)
{
	int result = 0;
	struct host_if_msg msg;

	memset(&msg, 0, sizeof(struct host_if_msg));
	msg.id = HOST_IF_MSG_SET_WOWLAN_TRIGGER;
	msg.body.wow_trigger.wowlan_trigger = wowlan_trigger;
	msg.vif = vif;

	result = wilc_enqueue_cmd(&msg);

	if(result)
		PRINT_ER(vif->ndev,
			 "Failed to enq wowlan trigger req\n");
	return result;
}	