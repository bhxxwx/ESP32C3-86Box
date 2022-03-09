/*
 * NodeManage.h
 *
 *  Created on: 2022年3月3日
 *      Author: wxujs
 */

#ifndef COMPONENTS_NODEMANAGE_NODEMANAGE_H_
#define COMPONENTS_NODEMANAGE_NODEMANAGE_H_

#include "nvs_flash.h"
#include "esp_log.h"

typedef struct
{
		uint16_t server_addr[20];
		uint16_t node_index;
		uint16_t node_num;
}NodeList_t;

void store_node_in_list(NodeList_t *NodeList,uint16_t addr,nvs_handle_t nvs_handle);
void init_node_list(NodeList_t *nodelist,nvs_handle_t nvs_handle);
void show_node(NodeList_t nodelist);
void Clean_node_list(NodeList_t *nodelist, nvs_handle_t nvs_handle);
#endif /* COMPONENTS_NODEMANAGE_NODEMANAGE_H_ */
