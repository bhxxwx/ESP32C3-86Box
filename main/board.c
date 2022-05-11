/*
 * @Author: Wangxiang
 * @Date: 2022-02-23 10:36:05
 * @LastEditTime: 2022-05-11 22:48:38
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

#define BUTTON_IO_NUM 9
#define BUTTON_ACTIVE_LEVEL 0

static TimerHandle_t save_timer_handle;
static TimerHandle_t toggle_timer_handle;
static TimerHandle_t prov_timer_handle;
static TimerHandle_t blink_timer_handle;
static nvs_handle_t user_nvs_handle;

static _lightModel light_status;
static light_hal_t interface_touch_status;
static uint8_t selected_light = main_selected;
static bool light_onoff_state = true;
static bool start_rest_light_node = false;
static light_hal_t sense[2] = {0};
static bool toogle_timer_start_flg = false;

static bool need_send2_light = false;

extern void Del_all_node();
extern void example_ble_mesh_publish_message(_lightModel *lightModel);
void RGB_TO_HSV(const COLOR_RGB *input, COLOR_HSV *output);
static void HSV_TO_RGB(COLOR_HSV *input, COLOR_RGB *output);
void save_light();
void toggle_light();
void prov_timer_func();
void blink_timer_func();

void board_init(nvs_handle_t nvs_handle)
{
    user_nvs_handle = nvs_handle;
    interface_touch_status.halo_angle = 360;
    interface_touch_status.halo_brightness = 100;
    interface_touch_status.main_colortemp = 50;
    interface_touch_status.main_brightness = 100;

    int len = 0;
    if (ReadFromNVS_blob("sense1", &sense[0], &len, user_nvs_handle) != ESP_OK)
    {
        sense[0].halo_angle = 360;
        sense[0].halo_brightness = 100;
        sense[0].main_colortemp = 100;
        sense[0].main_brightness = 100;
        WriteToNVS_blob("sense1", &sense[0], sizeof(light_hal_t), user_nvs_handle);
    }
    if (ReadFromNVS_blob("sense2", &sense[0], &len, user_nvs_handle) != ESP_OK)
    {
        sense[1].halo_angle = 180;
        sense[1].halo_brightness = 100;
        sense[1].main_colortemp = 0;
        sense[1].main_brightness = 100;
        WriteToNVS_blob("sense2", &sense[1], sizeof(light_hal_t), user_nvs_handle);
    }

    save_timer_handle = xTimerCreate("TimeToSave", pdMS_TO_TICKS(15000), false, NULL, save_light);
    toggle_timer_handle = xTimerCreate("ToggleLight", pdMS_TO_TICKS(60000), false, NULL, toggle_light);
    prov_timer_handle = xTimerCreate("provtimer", pdMS_TO_TICKS(30000), false, NULL, prov_timer_func);
    blink_timer_handle = xTimerCreate("rest", pdMS_TO_TICKS(1000), true, NULL, blink_timer_func);
    // board_button_init();
    // xTaskCreatePinnedToCore(save_light_task, "save_light_task", ECHO_TASK_STACK_SIZE, NULL, 9, &xTaskSaveLight, 0);
    // vTaskSuspend(xTaskSaveLight);
}

void save_light()
{
    WriteToNVS_blob("Slight", &light_status, sizeof(_lightModel), user_nvs_handle);
    // xTimerStop(save_timer_handle, 100);
}

void toggle_light()
{

    if (light_onoff_state)
    {
        memset(&light_status, 0, sizeof(_lightModel));
        light_onoff_state = false;
    }
    else
    {
        light_onoff_state = true;
        change_halo_light(&light_status, interface_touch_status.halo_angle, interface_touch_status.halo_brightness);
        change_main_light(&light_status, interface_touch_status.main_colortemp, interface_touch_status.main_brightness);
    }
    // xTimerStop(toggle_timer_handle, 100);
    toogle_timer_start_flg = false;
}

void prov_timer_func()
{
    ProvSet(false, false);
}

void blink_timer_func()
{
}

void light_opc(ITouchPad_t *TouchPadData)
{
    switch (TouchPadData->btns)
    {
    case enum_no_btn:
    {
        if (TouchPadData->wheel != 0xFFFF)
        {
            change_halo_light(&light_status, TouchPadData->wheel, interface_touch_status.halo_brightness);
            interface_touch_status.halo_angle = TouchPadData->wheel;
        }

        if (TouchPadData->right_slider != 0xFF)
        {
            if (light_selected == halo_selected)
            {
                change_halo_light(&light_status, interface_touch_status.halo_angle, TouchPadData->right_slider);
                interface_touch_status.halo_brightness = TouchPadData->right_slider;
            }
            if (light_selected == main_selected)
            {
                change_main_light(&light_status, interface_touch_status.main_colortemp, TouchPadData->right_slider);
                interface_touch_status.main_brightness = TouchPadData->right_slider;
            }
        }
        if (TouchPadData->left_slider != 0xFF)
        {
            change_main_light(&light_status, TouchPadData->left_slider, interface_touch_status.main_brightness);
            interface_touch_status.main_colortemp = TouchPadData->left_slider;
        }
        xTimerReset(save_timer_handle, 100);
        need_send2_light = true;
        break;
    }
    case enum_CCT:
    {
        selected_light = main_selected;
        break;
    }
    case enum_RGB:
    {
        selected_light = halo_selected;
        break;
    }

    case enum_onoff:
    {
        if (light_onoff_state)
        {
            memset(&light_status, 0, sizeof(_lightModel));
            light_onoff_state = false;
        }
        else
        {
            light_onoff_state = true;
            change_halo_light(&light_status, interface_touch_status.halo_angle, interface_touch_status.halo_brightness);
            change_main_light(&light_status, interface_touch_status.main_colortemp, interface_touch_status.main_brightness);
        }
        need_send2_light = true;
        break;
    }
    case enum_m1:
    {
        change_halo_light(&light_status, sense[0].halo_angle, sense[0].halo_brightness);
        change_main_light(&light_status, sense[0].main_colortemp, sense[0].main_brightness);
        need_send2_light = true;
        break;
    }
    case enum_m2:
    {
        change_halo_light(&light_status, sense[1].halo_angle, sense[0].halo_brightness);
        change_main_light(&light_status, sense[1].main_colortemp, sense[0].main_brightness);
        need_send2_light = true;
        break;
    }
    case enum_m1_long:
    {
        WriteToNVS_blob("sense1", &sense[0], sizeof(light_hal_t), user_nvs_handle);
        break;
    }
    case enum_m2_long:
    {
        WriteToNVS_blob("sense1", &sense[1], sizeof(light_hal_t), user_nvs_handle);
        break;
    }
    case enum_60s:
    {
        if (toogle_timer_start_flg == false)
        {
            xTimerReset(toggle_timer_handle, 100);
            toogle_timer_start_flg = true;
        }
        else
        {
            xTimerStop(toggle_timer_handle, 100);
            toogle_timer_start_flg = false;
        }
        break;
    }
    case enum_rest:
    {
        xTimerStart(prov_timer_handle, 100);
        Del_all_node();
        ProvSet(true, false);
        WriteToNVS("FactoryMode", 0, user_nvs_handle);
        break;
    }
    case enum_hall:
    {
        xTimerStart(prov_timer_handle, 100);
        ProvSet(true, false);
    }
    default:
        break;
    }

    if(need_send2_light)
    {
        example_ble_mesh_publish_message(&light_status);
        need_send2_light = false;
    }
}

void change_halo_light(_lightModel *light, uint16_t angle, uint8_t level)
{
    uint8_t j = angle / 60;
    uint8_t R, G, B;
    COLOR_RGB ORG_COLOR;
    COLOR_HSV HSV_OUT;
    switch (j)
    {
    case 0:
    {
        R = 255;
        G = (uint8_t)(((float)angle / 60.0) * 255.0);
        B = 0;
        break;
    }
    case 1:
    {
        R = (uint8_t)(255 - (((float)(angle - j * 60) / 60.0) * 255.0));
        G = 255;
        B = 0;
        break;
    }
    case 2:
    {
        R = 0;
        G = 255;
        B = (uint8_t)((((float)(angle - j * 60) / 60.0) * 255.0));
        break;
    }
    case 3:
    {
        R = 0;
        G = (uint8_t)(255 - (((float)(angle - j * 60) / 60.0) * 255.0));
        B = 255;
        break;
    }
    case 4:
    {
        R = (uint8_t)((((float)(angle - j * 60) / 60.0) * 255.0));
        G = 0;
        B = 255;
        break;
    }
    case 5:
    {
        R = 255;
        G = 0;
        B = (uint8_t)(255 - (((float)(angle - j * 60) / 60.0) * 255.0));
        break;
    }
    default:
    {
        R = 255;
        G = 0;
        B = 0;
        break;
    }
    }
    ORG_COLOR.R = R;
    ORG_COLOR.G = G;
    ORG_COLOR.B = B;
    ORG_COLOR.l = 100;
    RGB_TO_HSV(&ORG_COLOR, &HSV_OUT);
    if (level >= 100)
        HSV_OUT.COLOR_V = 1.0;
    else
        HSV_OUT.COLOR_V = level / 100.0;
    HSV_TO_RGB(&HSV_OUT, &ORG_COLOR);
    light->R = ORG_COLOR.R;
    light->G = ORG_COLOR.G;
    light->B = ORG_COLOR.B;
}

/**
 * @brief change main light ststus
 *
 * @param light      the light struct to change light CE or WW
 * @param color_temp range from @def MAX_COLOR_TEMP to @def MIN_COLOR_TEMP map from coldest to warmest
 * @param level      the light brightness, range 0~100
 */
void change_main_light(_lightModel *light, uint8_t color_temp, uint8_t level)
{
    int part = (MAX_COLOR_TEMP - MIN_COLOR_TEMP) / 100;
    int temp = MAX_COLOR_TEMP - color_temp;
    int tempww = (temp / part);
    uint8_t out_cw = (level - (level * tempww / 100));
    uint8_t out_ww = (level * tempww / 100);
    light->CW = PWM_DUTY * out_cw / 100;
    light->WW = PWM_DUTY * out_ww / 100;
}

/**
 * @brief convert RGB to HSV
 *
 * @param input  RGB struct point
 * @param output HSV struct point
 */
void RGB_TO_HSV(const COLOR_RGB *input, COLOR_HSV *output)
{
    float r, g, b, minRGB, maxRGB, deltaRGB;
    r = input->R / 255;
    g = input->G / 255;
    b = input->B / 255;
    minRGB = MinFunc(r, g, b);
    maxRGB = MaxFunc(r, g, b);

    deltaRGB = maxRGB - minRGB;
    output->COLOR_V = maxRGB;
    if (maxRGB != 0)
        output->COLOR_S = deltaRGB / maxRGB;
    else
        output->COLOR_S = 0;
    if (output->COLOR_S <= 0)
    {
        output->COLOR_H = 0;
    }
    else
    {
        if (r == maxRGB)
        {
            output->COLOR_H = (g - b) / deltaRGB;
        }
        else
        {
            if (g == maxRGB)
            {
                output->COLOR_H = 2 + (b - r) / deltaRGB;
            }
            else
            {
                if (b == maxRGB)
                {
                    output->COLOR_H = 4 + (r - g) / deltaRGB;
                }
            }
        }
        output->COLOR_H = output->COLOR_H * 60;
        if (output->COLOR_H < 0)
        {
            output->COLOR_H += 360;
        }
        output->COLOR_H /= 360;
    }
}

/**
 * @brief convert HSV to RGB
 *
 * @param input  HSV struct point
 * @param output RGB struct point
 */
static void HSV_TO_RGB(COLOR_HSV *input, COLOR_RGB *output)
{
    float R, G, B;
    int k;
    float aa, bb, cc, f;
    if (input->COLOR_S <= 0)
        R = G = B = input->COLOR_V;
    else
    {
        if (input->COLOR_H == 1)
            input->COLOR_H = 0;
        input->COLOR_H *= 6;
        k = (int)floor(input->COLOR_H);
        f = input->COLOR_H - k;
        aa = input->COLOR_V * (1 - input->COLOR_S);
        bb = input->COLOR_V * (1 - input->COLOR_S * f);
        cc = input->COLOR_V * (1 - (input->COLOR_S * (1 - f)));
        switch (k)
        {
        case 0:
            R = input->COLOR_V;
            G = cc;
            B = aa;
            break;
        case 1:
            R = bb;
            G = input->COLOR_V;
            B = aa;
            break;
        case 2:
            R = aa;
            G = input->COLOR_V;
            B = cc;
            break;
        case 3:
            R = aa;
            G = bb;
            B = input->COLOR_V;
            break;
        case 4:
            R = cc;
            G = aa;
            B = input->COLOR_V;
            break;
        case 5:
            R = input->COLOR_V;
            G = aa;
            B = bb;
            break;
        default:
        {
            R = input->COLOR_V;
            G = aa;
            B = bb;
            break;
        }
        }
    }
    output->R = (unsigned char)(R * 255);
    output->G = (unsigned char)(G * 255);
    output->B = (unsigned char)(B * 255);
}
