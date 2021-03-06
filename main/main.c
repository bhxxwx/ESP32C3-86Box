
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"

#include "ble_mesh_example_init.h"
#include "ble_mesh_example_nvs.h"
#include "board.h"
#include "Servers.h"
#include "NodeManage.h"
#include "CommandSystem.h"

#define TAG "EXAMPLE"

#define CID_ESP             0x02E5

#define PROV_OWN_ADDR       0x0001

#define MSG_SEND_TTL        3
#define MSG_SEND_REL        false
#define MSG_TIMEOUT         0
#define MSG_ROLE            ROLE_PROVISIONER

#define COMP_DATA_PAGE_0    0x00

#define APP_KEY_IDX         0x0000
#define APP_KEY_OCTET       0x12

#define COMP_DATA_1_OCTET(msg, offset)      (msg[offset])
#define COMP_DATA_2_OCTET(msg, offset)      (msg[offset + 1] << 8 | msg[offset])

#define ESP_BLE_MESH_VND_MODEL_ID_CLIENT    0x0000
#define ESP_BLE_MESH_VND_MODEL_ID_SERVER    0x0001
#define ESP_BLE_MESH_VND_MODEL2_ID_CLIENT   0x0002
#define ESP_BLE_MESH_VND_MODEL2_ID_SERVER   0x0003

#define ESP_BLE_MESH_VND_MODEL_OP_SEND      ESP_BLE_MESH_MODEL_OP_3(0x00, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_STATUS    ESP_BLE_MESH_MODEL_OP_3(0x01, CID_ESP)

#define ESP_BLE_MESH_VND_MODEL2_OP_SEND      ESP_BLE_MESH_MODEL_OP_3(0x02, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL2_OP_STATUS    ESP_BLE_MESH_MODEL_OP_3(0x03, CID_ESP)
void OP_PROV_Func(void *arg);
void CK_HBTK_Func(void *arg);
static void echo_task(void *arg);
void SE_MSGE_Func(void *arg);
void TouchData_Func(void *arg);
// void echo_print()
static uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN];

static struct example_info_store
{
	uint16_t server_addr; /* Vendor server unicast address */
	uint16_t vnd_tid; /* TID contained in the vendor message */
} store = { .server_addr = ESP_BLE_MESH_ADDR_UNASSIGNED };

static NodeList_t NodeList;

static nvs_handle_t NVS_HANDLE, NVS_USER_HANDLE;
static const char *NVS_KEY = "vendor_client";

static struct esp_ble_mesh_key
{
	uint16_t net_idx;
	uint16_t app_idx;
	uint8_t app_key[ESP_BLE_MESH_OCTET16_LEN];
} prov_key;

static esp_ble_mesh_cfg_srv_t config_server = { .beacon = ESP_BLE_MESH_BEACON_DISABLED,
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
	.friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
	.default_ttl = 7,
	/* 3 transmissions with 20ms interval */
	.net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20), .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20), };

static esp_ble_mesh_client_t config_client;

static const esp_ble_mesh_client_op_pair_t vnd_op_pair[] = { { ESP_BLE_MESH_VND_MODEL_OP_SEND, ESP_BLE_MESH_VND_MODEL_OP_STATUS, }, };
static const esp_ble_mesh_client_op_pair_t vnd_op_pair2[] = { { ESP_BLE_MESH_VND_MODEL2_OP_SEND, ESP_BLE_MESH_VND_MODEL2_OP_STATUS, }, };

static esp_ble_mesh_client_t vendor_client = { .op_pair_size = ARRAY_SIZE(vnd_op_pair), .op_pair = vnd_op_pair, };
static esp_ble_mesh_client_t vendor_client2 = { .op_pair_size = ARRAY_SIZE(vnd_op_pair2), .op_pair = vnd_op_pair2, };

static esp_ble_mesh_model_op_t vnd_op[] = {
ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_STATUS, sizeof(_lightModel)),
ESP_BLE_MESH_MODEL_OP_END, };
static esp_ble_mesh_model_op_t vnd_op2[] = {
ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL2_OP_STATUS, 2),
ESP_BLE_MESH_MODEL_OP_END, };

static esp_ble_mesh_model_t root_models[] = {
ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
ESP_BLE_MESH_MODEL_CFG_CLI(&config_client), };

ESP_BLE_MESH_MODEL_PUB_DEFINE(pub, sizeof(_lightModel) + 3, ROLE_PROVISIONER);

static esp_ble_mesh_model_t vnd_models[] = {
ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_CLIENT,
		vnd_op, &pub, &vendor_client),
ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL2_ID_CLIENT,
		vnd_op2, NULL, &vendor_client2), };

static esp_ble_mesh_elem_t elements[] = {
ESP_BLE_MESH_ELEMENT(0, root_models, vnd_models), };

static esp_ble_mesh_comp_t composition = { .cid = CID_ESP, .elements = elements, .element_count = ARRAY_SIZE(elements), };

static esp_ble_mesh_prov_t provision = { .prov_uuid = dev_uuid, .prov_unicast_addr = PROV_OWN_ADDR, .prov_start_address = 0x0005, };

static void mesh_example_info_store(void)
{
//	ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY, &store, sizeof(store));
}

static void mesh_example_info_restore(void)
{
//	esp_err_t err = ESP_OK;
//	bool exist = false;
//
//	err = ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY, &store, sizeof(store), &exist);
//	if (err != ESP_OK)
//	{
//		return;
//	}
//
//	if (exist)
//	{
//		ESP_LOGI(TAG, "Restore, server_addr 0x%04x", store.server_addr);
//	}
}

void Del_all_node()
{
	for (int i = 0; i < NodeList.node_num; i++)
	{
		ESP_LOGI("Delete_all_node", "Delete Node_idx:%d addr:0x%04x", i, NodeList.server_addr[i]);
		esp_ble_mesh_provisioner_delete_node_with_addr(NodeList.server_addr[i]);
	}
	Clean_node_list(&NodeList, NVS_USER_HANDLE);
}

static void example_ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common, esp_ble_mesh_node_t *node,
		esp_ble_mesh_model_t *model, uint32_t opcode)
{
	common->opcode = opcode;
	common->model = model;
	common->ctx.net_idx = prov_key.net_idx;
	common->ctx.app_idx = prov_key.app_idx;
	common->ctx.addr = node->unicast_addr;
	common->ctx.send_ttl = MSG_SEND_TTL;
	common->ctx.send_rel = MSG_SEND_REL;
	common->msg_timeout = MSG_TIMEOUT;
	common->msg_role = MSG_ROLE;
}

static esp_err_t prov_complete(uint16_t node_index, const esp_ble_mesh_octet16_t uuid, uint16_t primary_addr, uint8_t element_num,
		uint16_t net_idx)
{
	esp_ble_mesh_client_common_param_t common = { 0 };
	esp_ble_mesh_cfg_client_get_state_t get = { 0 };
	esp_ble_mesh_node_t *node = NULL;
	char name[10] = { '\0' };
	esp_err_t err;

	ESP_LOGI(TAG, "node_index %u, primary_addr 0x%04x, element_num %u, net_idx 0x%03x", node_index, primary_addr, element_num, net_idx);
	ESP_LOG_BUFFER_HEX("uuid", uuid, ESP_BLE_MESH_OCTET16_LEN);

	store.server_addr = primary_addr;
	mesh_example_info_store(); /* Store proper mesh example info */
	store_node_in_list(&NodeList, primary_addr, NVS_USER_HANDLE);
	sprintf(name, "%s%02x", "NODE-", node_index);
	err = esp_ble_mesh_provisioner_set_node_name(node_index, name);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to set node name");
		return ESP_FAIL;
	}

	node = esp_ble_mesh_provisioner_get_node_with_addr(primary_addr);
	if (node == NULL)
	{
		ESP_LOGE(TAG, "Failed to get node 0x%04x info", primary_addr);
		return ESP_FAIL;
	}

	example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
	get.comp_data_get.page = COMP_DATA_PAGE_0;
	err = esp_ble_mesh_config_client_get_state(&common, &get);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to send Config Composition Data Get");
		return ESP_FAIL;
	}

	return ESP_OK;
}

static void recv_unprov_adv_pkt(uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN], uint8_t addr[BD_ADDR_LEN], esp_ble_mesh_addr_type_t addr_type,
		uint16_t oob_info, uint8_t adv_type, esp_ble_mesh_prov_bearer_t bearer)
{
	esp_ble_mesh_unprov_dev_add_t add_dev = { 0 };
	esp_err_t err;

	/* Due to the API esp_ble_mesh_provisioner_set_dev_uuid_match, Provisioner will only
	 * use this callback to report the devices, whose device UUID starts with 0xdd & 0xdd,
	 * to the application layer.
	 */

	ESP_LOG_BUFFER_HEX("Device address", addr, BD_ADDR_LEN);
	ESP_LOGI(TAG, "Address type 0x%02x, adv type 0x%02x", addr_type, adv_type);
	ESP_LOG_BUFFER_HEX("Device UUID", dev_uuid, ESP_BLE_MESH_OCTET16_LEN);
	ESP_LOGI(TAG, "oob info 0x%04x, bearer %s", oob_info, (bearer & ESP_BLE_MESH_PROV_ADV) ? "PB-ADV" : "PB-GATT");

	memcpy(add_dev.addr, addr, BD_ADDR_LEN);
	add_dev.addr_type = (uint8_t) addr_type;
	memcpy(add_dev.uuid, dev_uuid, ESP_BLE_MESH_OCTET16_LEN);
	add_dev.oob_info = oob_info;
	add_dev.bearer = (uint8_t) bearer;
	/* Note: If unprovisioned device adv packets have not been received, we should not add
	 device with ADD_DEV_START_PROV_NOW_FLAG set. */
	err = esp_ble_mesh_provisioner_add_unprov_dev(&add_dev,
	ADD_DEV_RM_AFTER_PROV_FLAG | ADD_DEV_START_PROV_NOW_FLAG | ADD_DEV_FLUSHABLE_DEV_FLAG);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to start provisioning device");
	}
}

static void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t *param)
{
	switch (event)
	{
		case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
			ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
			mesh_example_info_restore(); /* Restore proper mesh example info */

			break;
		case ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT:
			ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT, err_code %d", param->provisioner_prov_enable_comp.err_code);
			break;
		case ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT:
			ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT, err_code %d", param->provisioner_prov_disable_comp.err_code);
			break;
		case ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT:
			ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT");
			recv_unprov_adv_pkt(param->provisioner_recv_unprov_adv_pkt.dev_uuid, param->provisioner_recv_unprov_adv_pkt.addr, param->provisioner_recv_unprov_adv_pkt.addr_type, param->provisioner_recv_unprov_adv_pkt.oob_info, param->provisioner_recv_unprov_adv_pkt.adv_type, param->provisioner_recv_unprov_adv_pkt.bearer);
			break;
		case ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT:
			ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT, bearer %s",
					param->provisioner_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
			break;
		case ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT:
			ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT, bearer %s, reason 0x%02x",
					param->provisioner_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT", param->provisioner_prov_link_close.reason);
			show_node(NodeList);
			break;
		case ESP_BLE_MESH_PROVISIONER_PROV_COMPLETE_EVT:
			prov_complete(param->provisioner_prov_complete.node_idx, param->provisioner_prov_complete.device_uuid, param->provisioner_prov_complete.unicast_addr, param->provisioner_prov_complete.element_num, param->provisioner_prov_complete.netkey_idx);
			break;
		case ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT:
			ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT, err_code %d", param->provisioner_add_unprov_dev_comp.err_code);
			break;
		case ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT:
			ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT, err_code %d", param->provisioner_set_dev_uuid_match_comp.err_code);
			break;
		case ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT:
			ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT, err_code %d", param->provisioner_set_node_name_comp.err_code);
			if (param->provisioner_set_node_name_comp.err_code == 0)
			{
				const char *name = esp_ble_mesh_provisioner_get_node_name(param->provisioner_set_node_name_comp.node_index);
				if (name)
				{
					ESP_LOGI(TAG, "Node %d name %s", param->provisioner_set_node_name_comp.node_index, name);
				}
			}
			break;
		case ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT:
			ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT, err_code %d", param->provisioner_add_app_key_comp.err_code);
			if (param->provisioner_add_app_key_comp.err_code == 0)
			{
				prov_key.app_idx = param->provisioner_add_app_key_comp.app_idx;
				esp_err_t err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
				ESP_BLE_MESH_VND_MODEL_ID_CLIENT, CID_ESP);
				if (err != ESP_OK)
				{
					ESP_LOGE(TAG, "Failed to bind AppKey to vendor 0 client");
				}
				err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
				ESP_BLE_MESH_VND_MODEL2_ID_CLIENT, CID_ESP);
				if (err != ESP_OK)
				{
					ESP_LOGE(TAG, "Failed to bind AppKey to vendor 1 client");
				}
			}
			break;
		case ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT:
			ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT, err_code %d", param->provisioner_bind_app_key_to_model_comp.err_code);
			break;
		case ESP_BLE_MESH_PROVISIONER_STORE_NODE_COMP_DATA_COMP_EVT:
			ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_STORE_NODE_COMP_DATA_COMP_EVT, err_code %d", param->provisioner_store_node_comp_data_comp.err_code);
			break;
		default:
			break;
	}
}

static void example_ble_mesh_parse_node_comp_data(const uint8_t *data, uint16_t length)
{
	uint16_t cid, pid, vid, crpl, feat;
	uint16_t loc, model_id, company_id;
	uint8_t nums, numv;
	uint16_t offset;
	int i;

	cid = COMP_DATA_2_OCTET(data, 0);
	pid = COMP_DATA_2_OCTET(data, 2);
	vid = COMP_DATA_2_OCTET(data, 4);
	crpl = COMP_DATA_2_OCTET(data, 6);
	feat = COMP_DATA_2_OCTET(data, 8);
	offset = 10;

	ESP_LOGI(TAG, "********************** Composition Data Start **********************");
	ESP_LOGI(TAG, "* CID 0x%04x, PID 0x%04x, VID 0x%04x, CRPL 0x%04x, Features 0x%04x *", cid, pid, vid, crpl, feat);
	for (; offset < length;)
	{
		loc = COMP_DATA_2_OCTET(data, offset);
		nums = COMP_DATA_1_OCTET(data, offset + 2);
		numv = COMP_DATA_1_OCTET(data, offset + 3);
		offset += 4;
		ESP_LOGI(TAG, "* Loc 0x%04x, NumS 0x%02x, NumV 0x%02x *", loc, nums, numv);
		for (i = 0; i < nums; i++)
		{
			model_id = COMP_DATA_2_OCTET(data, offset);
			ESP_LOGI(TAG, "* SIG Model ID 0x%04x *", model_id);
			offset += 2;
		}
		for (i = 0; i < numv; i++)
		{
			company_id = COMP_DATA_2_OCTET(data, offset);
			model_id = COMP_DATA_2_OCTET(data, offset + 2);
			ESP_LOGI(TAG, "* Vendor Model ID 0x%04x, Company ID 0x%04x *", model_id, company_id);
			offset += 4;
		}
	}
	ESP_LOGI(TAG, "*********************** Composition Data End ***********************");
}

static void example_ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event, esp_ble_mesh_cfg_client_cb_param_t *param)
{
	esp_ble_mesh_client_common_param_t common = { 0 };
	esp_ble_mesh_cfg_client_set_state_t set = { 0 };
	esp_ble_mesh_node_t *node = NULL;
	esp_err_t err;

	ESP_LOGI(TAG, "Config client, err_code %d, event %u, addr 0x%04x, opcode 0x%04x", param->error_code, event, param->params->ctx.addr, param->params->opcode);

	if (param->error_code)
	{
		ESP_LOGE(TAG, "Send config client message failed, opcode 0x%04x", param->params->opcode);
		return;
	}

	node = esp_ble_mesh_provisioner_get_node_with_addr(param->params->ctx.addr);
	if (!node)
	{
		ESP_LOGE(TAG, "Failed to get node 0x%04x info", param->params->ctx.addr);
		return;
	}

	switch (event)
	{
		case ESP_BLE_MESH_CFG_CLIENT_GET_STATE_EVT:
			if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET)
			{
				ESP_LOG_BUFFER_HEX("Composition data[]", param->status_cb.comp_data_status.composition_data->data, param->status_cb.comp_data_status.composition_data->len);
				ESP_LOGI(TAG, "tt");
				example_ble_mesh_parse_node_comp_data(param->status_cb.comp_data_status.composition_data->data, param->status_cb.comp_data_status.composition_data->len);
				err = esp_ble_mesh_provisioner_store_node_comp_data(param->params->ctx.addr, param->status_cb.comp_data_status.composition_data->data, param->status_cb.comp_data_status.composition_data->len);
				if (err != ESP_OK)
				{
					ESP_LOGE(TAG, "Failed to store node composition data");
					break;
				}

				example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
				set.app_key_add.net_idx = prov_key.net_idx;
				set.app_key_add.app_idx = prov_key.app_idx;
				memcpy(set.app_key_add.app_key, prov_key.app_key, ESP_BLE_MESH_OCTET16_LEN);
				err = esp_ble_mesh_config_client_set_state(&common, &set);
				if (err != ESP_OK)
				{
					ESP_LOGE(TAG, "Failed to send Config AppKey Add");
				}
			}
			break;
		case ESP_BLE_MESH_CFG_CLIENT_SET_STATE_EVT:
			if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD)
			{

				example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
				set.model_app_bind.element_addr = node->unicast_addr;
				ESP_LOGI("ex", "set.model_app_bind.element_addr: %02x", set.model_app_bind.element_addr);
				set.model_app_bind.model_app_idx = prov_key.app_idx;
				set.model_app_bind.model_id = ESP_BLE_MESH_VND_MODEL_ID_SERVER;
				set.model_app_bind.company_id = CID_ESP;
				err = esp_ble_mesh_config_client_set_state(&common, &set);
				if (err != ESP_OK)
				{
					ESP_LOGE(TAG, "Failed to send Config Model App Bind");
				}

			}
			else if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND)
			{
				ESP_LOGW(TAG, "%s, Provision and config successfully", __func__);
				if (node->dev_uuid[2] == 0xAA)
				{
					example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD);
					set.model_sub_add.company_id = CID_ESP;
					set.model_sub_add.model_id = ESP_BLE_MESH_VND_MODEL_ID_SERVER;
					set.model_sub_add.element_addr = param->params->ctx.addr;
					set.model_sub_add.sub_addr = 0xc000;
					esp_ble_mesh_config_client_set_state(&common, &set);
					ESP_LOGI(TAG, "Add publish group");
				}
			}
			break;
		case ESP_BLE_MESH_CFG_CLIENT_PUBLISH_EVT:
			if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_STATUS)
			{
				ESP_LOG_BUFFER_HEX("Composition data", param->status_cb.comp_data_status.composition_data->data, param->status_cb.comp_data_status.composition_data->len);
			}
			break;
		case ESP_BLE_MESH_CFG_CLIENT_TIMEOUT_EVT:
			switch (param->params->opcode)
			{
				case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET:
				{
					esp_ble_mesh_cfg_client_get_state_t get = { 0 };
					example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
					get.comp_data_get.page = COMP_DATA_PAGE_0;
					err = esp_ble_mesh_config_client_get_state(&common, &get);
					if (err != ESP_OK)
					{
						ESP_LOGE(TAG, "Failed to send Config Composition Data Get");
					}
					break;
				}
				case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
					example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
					set.app_key_add.net_idx = prov_key.net_idx;
					set.app_key_add.app_idx = prov_key.app_idx;
					memcpy(set.app_key_add.app_key, prov_key.app_key, ESP_BLE_MESH_OCTET16_LEN);
					err = esp_ble_mesh_config_client_set_state(&common, &set);
					if (err != ESP_OK)
					{
						ESP_LOGE(TAG, "Failed to send Config AppKey Add");
					}
					break;
				case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
					example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
					set.model_app_bind.element_addr = node->unicast_addr;
					set.model_app_bind.model_app_idx = prov_key.app_idx;
					set.model_app_bind.model_id = ESP_BLE_MESH_VND_MODEL_ID_SERVER;
					set.model_app_bind.company_id = CID_ESP;
					err = esp_ble_mesh_config_client_set_state(&common, &set);
					if (err != ESP_OK)
					{
						ESP_LOGE(TAG, "Failed to send Config Model App Bind");
					}
					break;
				default:
					break;
			}
			break;
		default:
			ESP_LOGE(TAG, "Invalid config client event %u", event);
			break;
	}
}

void example_ble_mesh_send_vendor_message(bool resend)
{
	esp_ble_mesh_msg_ctx_t ctx = { 0 };
	uint32_t opcode;
	esp_err_t err;
	uint8_t data[9];
	_lightModel light;
	ctx.net_idx = prov_key.net_idx;
	ctx.app_idx = prov_key.app_idx;
	ctx.addr = store.server_addr;
	ESP_LOGI(TAG, "ctx.addr 0x%02x", ctx.addr);
	ctx.send_ttl = MSG_SEND_TTL;
	ctx.send_rel = MSG_SEND_REL;

	light.R = 10;
	light.G = 20;
	light.B = 30;
	light.CW = 40;
	light.WW = 60;
	light.CMD = 80;

	opcode = ESP_BLE_MESH_VND_MODEL_OP_SEND;

	err = esp_ble_mesh_client_model_send_msg(vendor_client.model, &ctx, opcode, sizeof(_lightModel), (uint8_t*) &light, MSG_TIMEOUT, true, MSG_ROLE);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to send vendor message 0x%06x", opcode);
		return;
	}
	mesh_example_info_store(); /* Store proper mesh example info */
}

void example_ble_mesh_publish_message(_lightModel *lightModel)
{
	vendor_client.model->pub->app_idx = prov_key.app_idx;
	vendor_client.model->pub->publish_addr = 0xC000;
	vendor_client.model->pub->send_rel = MSG_SEND_REL;
	vendor_client.model->pub->ttl = 7;
	esp_ble_mesh_model_publish(vendor_client.model, ESP_BLE_MESH_VND_MODEL_OP_SEND, sizeof(_lightModel), (uint8_t*) lightModel, ROLE_PROVISIONER);

	ESP_LOGI(TAG, "PUBLISH MSGE");
}

static void example_ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event, esp_ble_mesh_model_cb_param_t *param)
{
	ESP_LOGI(TAG, "example_ble_mesh_custom_model_cb EVENT:0x%04x", event);
	switch (event)
	{
		case ESP_BLE_MESH_MODEL_OPERATION_EVT:
			ESP_LOGI(TAG, "example_ble_mesh_custom_model_cb opc:0x%04x", param->model_operation.opcode);
			if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_STATUS)
			{
				ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OPERATION_EVT opc:0x%04x", param->model_operation.opcode);
			}
			if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL2_OP_STATUS)
			{
				ESP_LOGI(TAG, "ESP_BLE_MESH_VND_MODEL2_OP_STATUS Recv 0x%06x,\n  ", param->model_operation.opcode, param->client_recv_publish_msg.msg);
				for (int i = 0; i < param->client_recv_publish_msg.length; i++)
				{
					printf("       D%d: %d", i, *(param->client_recv_publish_msg.msg + i));
				}
			}
			break;
		case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
			if (param->model_send_comp.err_code)
			{
				ESP_LOGE(TAG, "Failed to send message 0x%06x", param->model_send_comp.opcode);
				break;
			}
			ESP_LOGI(TAG, "Send 0x%06x", param->model_send_comp.opcode);
			break;
		case ESP_BLE_MESH_CLIENT_MODEL_RECV_PUBLISH_MSG_EVT:
		{
			ESP_LOGI(TAG, "Receive publish message 0x%06x", param->client_recv_publish_msg.opcode);
			if (param->client_recv_publish_msg.opcode == 0xc302e5)
			{
				// char data[22] = { 0 };
				// char buf[22] = { 0 };
				// uint8_t len = 0;
				// data[0] = '$';
				// for (int i = 0; i < param->client_recv_publish_msg.length; i++)
				// {
				// 	uint8_t value = *(param->client_recv_publish_msg.msg + i);
				// 	data[i * 2 + 1] = value;
				// 	data[i * 2 + 2] = i == param->client_recv_publish_msg.length - 1 ? ';' : ',';
				// }
				// data[param->client_recv_publish_msg.length*2+1]='\r';
				// data[param->client_recv_publish_msg.length*2+2]='\n';
				// len = sprintf(buf, "%s\r\n", data);
				
				// uart_write_bytes(ECHO_UART_PORT_NUM, data, param->client_recv_publish_msg.length*2+3);
				IRC_t rc_data;
				memset(&rc_data,0,sizeof(IRC_t));
				rc_data.op_code = *(param->client_recv_publish_msg.msg + 0);
				rc_data.func_code = *(param->client_recv_publish_msg.msg + 1);
				rc_data.value_code = *(param->client_recv_publish_msg.msg + 2);
				ESP_LOGI("RECV RC DATA", ">>%x %x %x", rc_data.op_code, rc_data.func_code, rc_data.value_code);
				remote_controler_opc(&rc_data);
				//test func
				// if (*(param->client_recv_publish_msg.msg) == 5)
				// {
				// 	_lightModel l;
				// 	l.CMD = *(param->client_recv_publish_msg.msg+1);
				// 	example_ble_mesh_publish_message(&l);
				// }
				//end text func
			}
			break;
		}

		case ESP_BLE_MESH_CLIENT_MODEL_SEND_TIMEOUT_EVT:
			ESP_LOGW(TAG, "Client message 0x%06x timeout", param->client_send_timeout.opcode);
//			example_ble_mesh_send_vendor_message(true);
			break;
		default:
			break;
	}
}

static esp_err_t ble_mesh_init(void)
{
	uint8_t match[2] = { 0xFA, 0x00 }; //???????????????
	esp_err_t err;

	prov_key.net_idx = ESP_BLE_MESH_KEY_PRIMARY;
	prov_key.app_idx = APP_KEY_IDX;
	memset(prov_key.app_key, APP_KEY_OCTET, sizeof(prov_key.app_key));

	esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
	esp_ble_mesh_register_config_client_callback(example_ble_mesh_config_client_cb);
	esp_ble_mesh_register_custom_model_callback(example_ble_mesh_custom_model_cb);

	err = esp_ble_mesh_init(&provision, &composition);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to initialize mesh stack");
		return err;
	}

	err = esp_ble_mesh_client_model_init(&vnd_models[0]);
	if (err)
	{
		ESP_LOGE(TAG, "Failed to initialize vendor client-0");
		return err;
	}

	err = esp_ble_mesh_client_model_init(&vnd_models[1]);
	if (err)
	{
		ESP_LOGE(TAG, "Failed to initialize vendor client-1");
		return err;
	}

	err = esp_ble_mesh_provisioner_set_dev_uuid_match(match, sizeof(match), 0x0, false);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to set matching device uuid");
		return err;
	}

	err = esp_ble_mesh_provisioner_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to enable mesh provisioner");
		return err;
	}

	err = esp_ble_mesh_provisioner_add_local_app_key(prov_key.app_key, prov_key.net_idx, prov_key.app_idx);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to add local AppKey");
		return err;
	}

	ESP_LOGI(TAG, "ESP BLE Mesh Provisioner initialized");

	return ESP_OK;
}

void app_main(void)
{
	esp_err_t err;
	uint8_t FactoryMode = 0xFF;
	ESP_LOGI(TAG, "Initializing...");
	err = nvs_flash_init_partition("nvs_user");
	if (err == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		ESP_ERROR_CHECK(nvs_flash_erase_partition("nvs_user"));
		err = nvs_flash_init_partition("nvs_user");
	}
	ESP_ERROR_CHECK(err);
	nvs_open_from_partition("nvs_user", "user_infos", NVS_READWRITE, &NVS_USER_HANDLE);
	init_node_list(&NodeList, NVS_USER_HANDLE);
	show_node(NodeList);

	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	board_init(NVS_USER_HANDLE);

	err = bluetooth_init();
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
		return;
	}

	/* Open nvs namespace for storing/restoring mesh example info */
	err = ble_mesh_nvs_open(&NVS_HANDLE);
	if (err)
	{
		ESP_LOGE(TAG, "flash store node init failue");
		return;
	}

	ble_mesh_get_dev_uuid(dev_uuid);

	/* Initialize the Bluetooth Mesh Subsystem */
	err = ble_mesh_init();
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
	}

	if (ReadFromNVS("FactoryMode", &FactoryMode, NVS_USER_HANDLE))
	{
		if (FactoryMode != 0)
		{
			//???????????????
			Del_all_node();
			ProvSet(true, true);
			ESP_LOGI(TAG, "in Factory mode");
		}
		else
		{
			ProvSet(false, false);
			ESP_LOGI(TAG, "in Normal mode");
		}
	}
	else
	{
		//???????????????,???????????????,?????????????????????
		ProvSet(true, true);
		ESP_LOGI(TAG, "in Factory mode");
	}

	CommandSystemInit();
	CommandReg("OP_PROV", OP_PROV_Func);
	CommandReg("CL_PROV", CL_PROV_Func);
	CommandReg("RE_FACT", RE_FACT_Func);
	CommandReg("SE_MSGE", SE_MSGE_Func);
	CommandReg("CK_HBTK", CK_HBTK_Func);
	CommandReg("ST_TCHD", TouchData_Func);
	//	CommandReg("DL_ALND", CK_HBTK_Func);
	xTaskCreatePinnedToCore(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL, 0);

}

void TouchData_Func(void *arg)
{
	uint8_t data[15] = {0};
	uint8_t len = 0;
	len=DecodeCommandValue(arg, data);
	ITouchPad_t TouchPadData;
	TouchPadData.wheel = (data[0] << 8 )+ data[1];
	TouchPadData.left_slider = data[2];
	TouchPadData.right_slider = data[3];
	TouchPadData.btns = data[4];
	light_opc(&TouchPadData);
}

void OP_PROV_Func(void *arg)
{
	ProvSet(true, false);
	WriteToNVS("FactoryMode", 0, NVS_USER_HANDLE);
}
void CK_HBTK_Func(void *arg)
{
	uart_write_bytes(ECHO_UART_PORT_NUM, ">>CMD_OK\r\n", 10);
}
static void echo_task(void *arg)
{
	uint8_t *data = (uint8_t*) malloc(BUF_SIZE);
	while (1)
	{
		// Read data from the UART
		int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE, 20 / portTICK_RATE_MS);
		if (len > 7)
		{
			data[len] = 0;
			ESP_LOGI("echo_task", "<<%s", data);
			if (DecodeCommandHead(len, (char*) data))
				uart_write_bytes(ECHO_UART_PORT_NUM, ">>CMD_OK\r\n", 10);
			
			bzero(data, BUF_SIZE);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

void SE_MSGE_Func(void *arg)
{
	uint8_t data[15] = {0};
	uint8_t count = DecodeCommandValue(arg, data);
	_lightModel light;
	memcpy(&light, data, 6);
	example_ble_mesh_publish_message(&light);
}
