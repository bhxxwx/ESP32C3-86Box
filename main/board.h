/*
 * @Author: Wangxiang
 * @Date: 2022-02-23 10:36:05
 * @LastEditTime: 2022-03-25 18:24:35
 * @LastEditors: Wangxiang
 * @Description: 
 * @FilePath: /ESPC3_Client_86Prov/main/board.h
 * 江苏大学-王翔
 */
/* board.h - Board-specific hooks */

#ifndef _BOARD_H_
#define _BOARD_H_
#include "Servers.h"

typedef struct
{
	uint8_t R;
	uint8_t G;
	uint8_t B;
	uint8_t CW;
	uint8_t WW;
	uint8_t CMD;
} _lightModel;
static void save_light_task(void *arg);
void board_init(void);
void save_light(_lightModel s, nvs_handle_t nvs_handle);

#endif /* _BOARD_H_ */
