/*
 * @Author: Wangxiang
 * @Date: 2022-03-03 13:11:46
 * @LastEditTime: 2022-03-25 18:17:22
 * @LastEditors: Wangxiang
 * @Description: 
 * @FilePath: /ESPC3_Client_86Prov/main/Servers.h
 * 江苏大学-王翔
 */
/*
 * Servers.h
 *
 *  Created on: 2021年4月13日
 *      Author: wxujs
 */

#ifndef MAIN_SERVERS_H_
#define MAIN_SERVERS_H_

/********************************************************************************************include区域开始********/
/** include 区域开始**/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "nvs_flash.h"
#include "esp_log.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"

#include "ble_mesh_example_init.h"
#include "driver/uart.h"
#include "driver/periph_ctrl.h"
#include "board.h"
#include "CommandSystem.h"
/** include 区域结束**/
/*******************************************************************************************include区域结束********/

/********************************************************************************************数据结构体定义区域开始********/
/** 数据结构体定义 区域开始**/
//typedef enum
//{
//	NULL_CMD,
//	OP_PROV,
//	CL_PROV,
//	RE_FACT,
//	SE_MSGE,
//	OP_CDRT,
//	ST_NSUB,
//	ENUM_LEN,
//} CMD;



/** 数据结构体定义 区域结束**/
/*******************************************************************************************数据结构体定义区域结束********/

/********************************************************************************************系统API区域开始********/
/** 系统API 区域开始**/
void ProvSet(bool provState, bool factoryMode);
void FactoryReset();

void WriteToNVS(const char *key, uint8_t data, nvs_handle_t nvs_handle);
uint8_t ReadFromNVS(const char *key, uint8_t *data, nvs_handle_t nvs_handle);

void ReadFromNVS_blob(const char *key, void *data, uint16_t *len, nvs_handle_t nvs_handle);
void WriteToNVS_blob(const char *key, void *data, uint16_t len, nvs_handle_t nvs_handle);

/** 系统API 区域结束**/
/*******************************************************************************************系统API区域结束********/

/********************************************************************************************中间服务API区域开始********/
/** 中间服务API 区域开始**/
//CMD DecodeCommandHead(int len, char *str);
//uint8_t DecodeCommandValue(char *str, uint8_t data[]);
//char* Slpit(char *s, const char *delim, uint8_t ArrayNo);
//unsigned int Touint(char *value);
//uint8_t getStrLen(char *str);
//void CleanArray(uint8_t *array);

void CL_PROV_Func(void *arg);
void RE_FACT_Func(void *arg);
// void SE_MSGE_Func(void *arg);
/** 中间服务API 区域结束**/
/*******************************************************************************************中间服务API区域结束********/

#endif /* MAIN_SERVERS_H_ */
