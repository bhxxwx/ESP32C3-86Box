/* board.c - Board-specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 * Additional Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "esp_log.h"
#include "iot_button.h"
#include "board.h"

#define TAG "BOARD"

#define BUTTON_IO_NUM           9
#define BUTTON_ACTIVE_LEVEL     0

//extern void example_ble_mesh_send_vendor_message(bool resend);
//extern void example_ble_mesh_publish_message(_lightModel *lightModel);
static void button_tap_cb(void* arg)
{
//	if(*(char*)arg=='P')
//	{
//		_lightModel light;
//		light.R=100;
//		light.G=120;
//		light.B=140;
//		light.CW=160;
//		light.WW=180;
//		light.CMD=200;
//		example_ble_mesh_publish_message(&light);
//	}
//	if(*(char*)arg=='R')
//	{
//
//	}

}

static void board_button_init(void)
{
    button_handle_t btn_handle = iot_button_create(BUTTON_IO_NUM, BUTTON_ACTIVE_LEVEL);
    if (btn_handle) {
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_PUSH, button_tap_cb, "P");
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_RELEASE, button_tap_cb, "R");
    }
}

void board_init(void)
{
    board_button_init();
}
