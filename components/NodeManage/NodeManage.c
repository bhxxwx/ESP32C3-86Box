/*
 * NodeManage.c
 *
 *  Created on: 2022年3月3日
 *      Author: wxujs
 */

#include "NodeManage.h"

void store_node_in_list(NodeList_t *NodeList, uint16_t addr, nvs_handle_t nvs_handle)
{
	esp_err_t err;

	for (int i = 0; i < NodeList->node_num; i++)
	{
		if (NodeList->server_addr[i] == addr)
		{
			ESP_LOGW("store_node_in_list", "node 0x%04x already exits", addr);
			return;
		}
	}
	NodeList->node_num++;
	NodeList->node_index = NodeList->node_num - 1;
	NodeList->server_addr[NodeList->node_index] = addr;
	err = nvs_set_blob(nvs_handle, "NodeList", NodeList, sizeof(NodeList_t));
	nvs_commit(nvs_handle);
	ESP_LOGI("store_node_in_list", "store_node_in_list");
}

void init_node_list(NodeList_t *nodelist, nvs_handle_t nvs_handle)
{
	esp_err_t err;
	size_t length;
	err = nvs_get_blob(nvs_handle, "NodeList", NULL, &length);
	err = nvs_get_blob(nvs_handle, "NodeList", nodelist, &length);
	if (err == ESP_ERR_NVS_NOT_FOUND)
	{
		nodelist->node_index = 0;
		nodelist->node_num = 0;
		ESP_LOGW("init_node_list", "init the node list");
	}
	ESP_LOGI("init_node_list", "init the node list ok");
}

void show_node(NodeList_t nodelist)
{
	int i = 0;
	for (i = 0; i < nodelist.node_num; i++)
	{
		ESP_LOGI("show_node", "Node_idx:%d addr:0x%04x", i, nodelist.server_addr[i]);
	}
	ESP_LOGI("show_node", "Total node:%d  Now idx:%d", nodelist.node_num, nodelist.node_index);
}

void Clean_node_list(NodeList_t *nodelist, nvs_handle_t nvs_handle)
{
	nodelist->node_num = 0;
	nodelist->node_index = 0;
	nvs_set_blob(nvs_handle, "NodeList", nodelist, sizeof(NodeList_t));
	nvs_commit(nvs_handle);
}

