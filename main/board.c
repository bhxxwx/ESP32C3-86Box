/*
 * @Author: Wangxiang
 * @Date: 2022-02-23 10:36:05
 * @LastEditTime: 2022-03-25 18:37:34
 * @LastEditors: Wangxiang
 * @Description: 
 * @FilePath: /ESPC3_Client_86Prov/main/board.c
 * 江苏大学-王翔
 */
/* board.c - Board-specific hooks */

#include <stdio.h>
#include "esp_log.h"
#include "iot_button.h"
#include "board.h"

#define TAG "BOARD"

#define BUTTON_IO_NUM           9
#define BUTTON_ACTIVE_LEVEL     0

static void save_light_task(void *arg);
TaskHandle_t xTaskSaveLight;
static uint16_t saveLightTick = 0;
static _lightModel SaveLight;
static uint8_t taskSuspen = false;
static nvs_handle_t user_nvs_handle;
static void button_tap_cb(void *arg)
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
    // board_button_init();
    // xTaskCreatePinnedToCore(save_light_task, "save_light_task", ECHO_TASK_STACK_SIZE, NULL, 9, &xTaskSaveLight, 0);
    // vTaskSuspend(xTaskSaveLight);
}

void save_light(_lightModel s, nvs_handle_t nvs_handle)
{
    user_nvs_handle = nvs_handle;
    SaveLight = s;
    saveLightTick = 0;
    vTaskResume(xTaskSaveLight);
    taskSuspen = true;
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (taskSuspen)
    {
        vTaskResume(xTaskSaveLight);
    }
}

static void save_light_task(void *arg)
{
    while (1)
    {

        for (; saveLightTick < 50; saveLightTick++)
        {
            taskSuspen = false;
            vTaskDelay(100 / portTICK_PERIOD_MS);
            taskSuspen = false;
        }
        WriteToNVS_blob("Slight", &SaveLight, sizeof(_lightModel), user_nvs_handle);
        vTaskSuspend(xTaskSaveLight);
    }
}
