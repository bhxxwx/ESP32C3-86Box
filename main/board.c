/*
 * @Author: Wangxiang
 * @Date: 2022-02-23 10:36:05
 * @LastEditTime: 2022-05-18 16:48:27
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
#define LED_ELEMET(pin, hz)   \
    {                         \
        .blink_pin = pin,     \
        .start_flg = false,   \
        .level_flg = 1,       \
        .normal_level_flg = 0 \
    }

static TimerHandle_t save_timer_handle;
static TimerHandle_t auto_close_timer_handle;
static TimerHandle_t prov_timer_handle;
static TimerHandle_t blink_timer_handle;
static nvs_handle_t user_nvs_handle;

static _lightModel light_status;
static light_hal_t interface_touch_status;
static uint8_t selected_light = main_selected;
static bool all_light_onoff_state = true;
static bool halo_light_onoff_state = true;
static bool main_light_onoff_state = true;
static bool start_rest_light_node = false;
static light_hal_t sense[2] = {0};
static bool toogle_timer_start_flg = false;

static bool need_send2_light = false;
static bool relay_tick_flg = false;
static bool is_first_time_open_relay = true;

blink_t led_blink_state[LED_NUMBER] =
    {
        LED_ELEMET(LED_ONOFF_PIN, 100),
        LED_ELEMET(LED_CCT_PIN, 100),
        LED_ELEMET(LED_RGB_PIN, 100),
        LED_ELEMET(LED_M1_PIN, 100),
        LED_ELEMET(LED_M2_PIN, 100),
        LED_ELEMET(LED_AUTO_PIN, 100),
};

extern void
Del_all_node();
extern void example_ble_mesh_publish_message(_lightModel *lightModel);
void RGB_TO_HSV(const COLOR_RGB *input, COLOR_HSV *output);
static void HSV_TO_RGB(COLOR_HSV *input, COLOR_RGB *output);
void save_light();
void auto_close_light();
void prov_timer_func();
void blink_timer_func();

void board_init(nvs_handle_t nvs_handle)
{
    uint8_t data = 0;
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
    if (ReadFromNVS_blob("sense2", &sense[1], &len, user_nvs_handle) != ESP_OK)
    {
        sense[1].halo_angle = 180;
        sense[1].halo_brightness = 100;
        sense[1].main_colortemp = 0;
        sense[1].main_brightness = 100;
        WriteToNVS_blob("sense2", &sense[1], sizeof(light_hal_t), user_nvs_handle);
    }
    if (ReadFromNVS_blob("sense3", &sense[2], &len, user_nvs_handle) != ESP_OK)
    {
        sense[1].halo_angle = 90;
        sense[1].halo_brightness = 100;
        sense[1].main_colortemp = 0;
        sense[1].main_brightness = 100;
        WriteToNVS_blob("sense3", &sense[2], sizeof(light_hal_t), user_nvs_handle);
    }
    if (ReadFromNVS_blob("Slight_hal", &interface_touch_status, &len, user_nvs_handle) != ESP_OK)
    {
        interface_touch_status.halo_angle = 60;
        interface_touch_status.halo_brightness = 100;
        interface_touch_status.main_colortemp = 0;
        interface_touch_status.main_brightness = 50;

        WriteToNVS_blob("Slight_hal", &interface_touch_status, sizeof(light_hal_t), user_nvs_handle);
    }
    gpio_config_t gpio_conf={};
    gpio_conf.pin_bit_mask = (1ULL << LED_ONOFF_PIN) | (1UL << LED_CCT_PIN) | (1UL << LED_RGB_PIN) | (1UL << LED_M1_PIN) | (1UL << LED_M2_PIN) | (1UL << LED_AUTO_PIN);
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_conf.pull_down_en = 0;
    gpio_conf.pull_up_en = 0;
    esp_err_t err= gpio_config(&gpio_conf);
    ESP_LOGI("haha", "%d", err);

    // led_blink_state[0];

    change_halo_light(&light_status, interface_touch_status.halo_angle, interface_touch_status.halo_brightness);
    change_main_light(&light_status, interface_touch_status.main_colortemp, interface_touch_status.main_brightness);
    save_timer_handle = xTimerCreate("TimeToSave", pdMS_TO_TICKS(15000), false, NULL, save_light);
    auto_close_timer_handle = xTimerCreate("CloseLight", pdMS_TO_TICKS(60000), false, NULL, auto_close_light);
    prov_timer_handle = xTimerCreate("provtimer", pdMS_TO_TICKS(50000), false, NULL, prov_timer_func);
    blink_timer_handle = xTimerCreate("blink", pdMS_TO_TICKS(100), true, NULL, blink_timer_func);
    xTimerStart(blink_timer_handle, 100);
    uart_write_bytes(ECHO_UART_PORT_NUM, &data, 1);
    close_led(LED_ALL);
    open_led(LED_CCT);
}

/**
 * @brief timer to save light status
 *
 */
void save_light()
{
    // WriteToNVS_blob("Slight", &light_status, sizeof(_lightModel), user_nvs_handle);
    WriteToNVS_blob("Slight_hal", &interface_touch_status, sizeof(light_hal_t), user_nvs_handle);
    // xTimerStop(save_timer_handle, 100);
}

/**
 * @brief timer to close light
 *
 */
void auto_close_light()
{
    // if (all_light_onoff_state)
    // {
        memset(&light_status, 0, sizeof(_lightModel));
        all_light_onoff_state = false;
        halo_light_onoff_state = false;
        main_light_onoff_state = false;
        close_relay();
    // }
    // else
    // {
    //     open_relay();
    //     all_light_onoff_state = true;
    //     change_halo_light(&light_status, interface_touch_status.halo_angle, interface_touch_status.halo_brightness);
    //     change_main_light(&light_status, interface_touch_status.main_colortemp, interface_touch_status.main_brightness);
    //     halo_light_onoff_state = true;
    //     main_light_onoff_state = true;
    // }
    // xTimerStop(auto_close_timer_handle, 100);
    example_ble_mesh_publish_message(&light_status);
    toogle_timer_start_flg = false;
    close_led(LED_AUTO_PIN);
}

/**
 * @brief timer to close prov
 *
 */
void prov_timer_func()
{
    ProvSet(false, false);
    stop_blink_led(LED_ALL);
}

/**
 * @brief 100ms tick
 *
 */
void blink_timer_func()
{
    if (relay_tick_flg == false)
        blink_tick();
    relay_reset_tick();
}

/**
 * @brief 100ms tick
 *
 */
void relay_reset_tick()
{
    static uint8_t time_count = 0;
    static bool relay_flg = 0;
    static uint8_t count;
    if (relay_tick_flg)
    {
        uint8_t data;
        time_count++;
        if (time_count >= 15) // 1.5s
        {
            data = relay_flg;
            relay_flg = !relay_flg;
            uart_write_bytes(ECHO_UART_PORT_NUM, &data, 1);
            set_led(LED_ALL, data, false);
            time_count = 0;
            count++;
            if (count >= 10 && data == 1)
            {
                relay_tick_flg = false;
                count = 0;
                relay_flg = 0;
                blink_led(LED_ALL, 5, 0);
            }
        }
    }
}

void open_relay()
{
    uint8_t data;
    data = 1;
    uart_write_bytes(ECHO_UART_PORT_NUM, &data, 1);
    if (is_first_time_open_relay)
    {
        esp_rom_delay_us(80000);
    }
    is_first_time_open_relay = false;
}
void close_relay()
{
    uint8_t data;
    data = 0;
    uart_write_bytes(ECHO_UART_PORT_NUM, &data, 1);
    is_first_time_open_relay = true;
}
void blink_led(uint16_t led_pin, uint16_t blink_ms, uint16_t cycle)
{
    for (int i = 0; i < LED_NUMBER; i++)
    {
        if (((led_pin >> i) & 0x0001) == 0x0001)
        {
            led_blink_state[i].count = 0;
            led_blink_state[i].start_flg = true;
            led_blink_state[i].target_count = blink_ms;
            led_blink_state[i].level_flg = 1;
            led_blink_state[i].target_cycle = cycle * 2;
            led_blink_state[i].cycle_count = 0;
        }
    }
}

void blink_tick()
{
    for (int i = 0; i < LED_NUMBER; i++)
    {
        if (led_blink_state[i].start_flg == true)
        {
            led_blink_state[i].count++;
            if (led_blink_state[i].count >= led_blink_state[i].target_count)
            {
                led_blink_state[i].count = 0;
                gpio_set_level(led_blink_state[i].blink_pin, led_blink_state[i].level_flg);
                led_blink_state[i].level_flg = !led_blink_state[i].level_flg;
                if (led_blink_state[i].target_cycle == 0)
                {
                    continue;
                }
                led_blink_state[i].cycle_count++;
            }

            if (led_blink_state[i].cycle_count >= led_blink_state[i].target_cycle)
            {
                led_blink_state[i].start_flg = false;
                led_blink_state[i].count = 0;
                led_blink_state[i].cycle_count = 0;
                gpio_set_level(led_blink_state[i].blink_pin, led_blink_state[i].normal_level_flg);
            }
        }
    }
}
void stop_blink_led(uint16_t led_pin)
{
    for (int i = 0; i < LED_NUMBER; i++)
    {
        if (((led_pin >> i) & 0x0001) == 0x0001)
        {
            led_blink_state[i].start_flg = false;
            led_blink_state[i].count = 0;
            gpio_set_level(led_blink_state[i].blink_pin, led_blink_state[i].normal_level_flg);
        }
    }
}

void close_led(uint16_t led_pin)
{
    set_led(led_pin, 0, true);
}

void open_led(uint16_t led_pin)
{
    set_led(led_pin, 1, true);
}

void set_led(uint16_t led_pin, uint8_t level, bool normal_flg)
{
    for (int i = 0; i < LED_NUMBER; i++)
    {
        if (((led_pin >> i) & 0x0001) == 0x0001)
        {
            if (normal_flg)
                led_blink_state[i].normal_level_flg = level;
            gpio_set_level(led_blink_state[i].blink_pin, level);
        }
    }
}

/**
 * @brief decode remote controller data, and change light ststus
 *
 * @param RemoteControllerData remote controller data
 */
void remote_controler_opc(IRC_t *RemoteControllerData)
{
    switch (RemoteControllerData->op_code)
    {
    case enum_rc_op_light_level: // first byte is change light level
    {

        if (RemoteControllerData->func_code == enum_rc_level_light_level) // change main light brightness
        {
            if (RemoteControllerData->value_code == 1) // add
            {

                if (interface_touch_status.main_brightness > 100)
                    interface_touch_status.main_brightness = 100;
                else
                    interface_touch_status.main_brightness += 5;
            }
            else // sub
            {

                if (interface_touch_status.main_brightness <= 5)
                    interface_touch_status.main_brightness = 5;
                else
                    interface_touch_status.main_brightness -= 5;
            }
            change_main_light(&light_status, interface_touch_status.main_colortemp, interface_touch_status.main_brightness);
        }
        if (RemoteControllerData->func_code == enum_rc_level_cw) // change main light colortemp
        {
            if (RemoteControllerData->value_code == 1)
            {

                if (interface_touch_status.main_colortemp >= MAX_COLOR_TEMP - 5)
                    interface_touch_status.main_colortemp = MAX_COLOR_TEMP;
                else
                    interface_touch_status.main_colortemp += 5;
            }
            else
            {

                if (interface_touch_status.main_colortemp <= MIN_COLOR_TEMP)
                    interface_touch_status.main_colortemp = MIN_COLOR_TEMP;
                else
                    interface_touch_status.main_colortemp -= 5;
            }
            change_main_light(&light_status, interface_touch_status.main_colortemp, interface_touch_status.main_brightness);
        }

        if (RemoteControllerData->func_code == enum_rc_level_color_level) // change halo light brightness
        {
            if (RemoteControllerData->value_code == 1)
            {

                if (interface_touch_status.halo_brightness >= 250)
                    interface_touch_status.halo_brightness = 255;
                else
                    interface_touch_status.halo_brightness += 5;
            }
            else
            {

                if (interface_touch_status.halo_brightness <= 5)
                {
                    interface_touch_status.halo_brightness = 0;
                }
                else
                {
                    interface_touch_status.halo_brightness -= 5;
                }
            }
            change_halo_light(&light_status, interface_touch_status.halo_angle, interface_touch_status.halo_brightness);
        }

        if (RemoteControllerData->func_code == enum_rc_level_color) // change halo color
        {
            if (RemoteControllerData->value_code == 1)
            {

                if (interface_touch_status.halo_angle >= 360)
                    interface_touch_status.halo_angle = 360;
                else
                    interface_touch_status.halo_angle += 5;
            }
            else
            {

                if (interface_touch_status.halo_angle <= 5)
                {
                    interface_touch_status.halo_angle = 0;
                }
                else
                {
                    interface_touch_status.halo_angle -= 5;
                }
            }
            change_halo_light(&light_status, interface_touch_status.halo_angle, interface_touch_status.halo_brightness);
        }
        example_ble_mesh_publish_message(&light_status);
        break;
    }
    case enum_rc_op_toggle_light: // first byte is toggle the light
    {
        if (RemoteControllerData->func_code == enum_rc_toggle_all) // toggle all light
        {
            if (all_light_onoff_state) // if any light is open, close all
            {
                memset(&light_status, 0, sizeof(_lightModel));
                all_light_onoff_state = false;
                halo_light_onoff_state = false;
                main_light_onoff_state = false;
            }
            else // else open all by last status
            {
                all_light_onoff_state = true;
                change_halo_light(&light_status, interface_touch_status.halo_angle, interface_touch_status.halo_brightness);
                change_main_light(&light_status, interface_touch_status.main_colortemp, interface_touch_status.main_brightness);
                halo_light_onoff_state = true;
                main_light_onoff_state = true;
            }
        }

        if (RemoteControllerData->func_code == enum_rc_toggle_halo) // toggle halo alone
        {
            if (halo_light_onoff_state) // if halo is already open, close it
            {
                light_status.R = 0;
                light_status.G = 0;
                light_status.B = 0;
                halo_light_onoff_state = false;
            }
            else // else open it by last status
            {
                change_halo_light(&light_status, interface_touch_status.halo_angle, interface_touch_status.halo_brightness);
                halo_light_onoff_state = true;
                all_light_onoff_state = true;
            }
        }
        if (RemoteControllerData->func_code == enum_rc_toggle_main) // toggle main alone
        {
            if (main_light_onoff_state) // if main light is open already, close it
            {
                light_status.CW = 0;
                light_status.WW = 0;
                main_light_onoff_state = false;
            }
            else // else open it by last status
            {
                change_main_light(&light_status, interface_touch_status.main_colortemp, interface_touch_status.main_brightness);
                main_light_onoff_state = true;
                all_light_onoff_state = true;
            }
        }
        example_ble_mesh_publish_message(&light_status);
        break;
    }
    case enum_rc_op_sense_change: // first byte is change the light to sense
    {
        change_halo_light(&light_status, sense[RemoteControllerData->func_code - 1].halo_angle, sense[RemoteControllerData->func_code - 1].halo_brightness);
        change_main_light(&light_status, sense[RemoteControllerData->func_code - 1].main_colortemp, sense[RemoteControllerData->func_code - 1].main_brightness);
        example_ble_mesh_publish_message(&light_status);
        all_light_onoff_state = true;
        break;
    }
    case enum_rc_op_sense_set: // first byte is save sense
    {
        char str[6] = {0};
        uint8_t len = 0;
        len = sprintf(str, "sense%d", RemoteControllerData->func_code);
        if (len == 6)
        {
            memcpy(&sense[RemoteControllerData->func_code - 1], &interface_touch_status, sizeof(light_hal_t));
            WriteToNVS_blob(str, &sense[RemoteControllerData->func_code - 1], sizeof(light_hal_t), user_nvs_handle);
            ESP_LOGI("RCDATA", "saved %s", str);
        }
    }
    case enum_rc_op_dance:
    {
        _lightModel dance_light;
        dance_light.CMD = RemoteControllerData->func_code;
        example_ble_mesh_publish_message(&dance_light);
        halo_light_onoff_state = true;
        all_light_onoff_state = true;
        break;
    }
    default:
        break;
    }
}

/**
 * @brief decode the touchpad data and operation the light
 *
 * @param TouchPadData touch pad data
 */
void light_opc(ITouchPad_t *TouchPadData)
{
    switch (TouchPadData->btns)
    {
    case enum_no_btn: // no buttons pressed, change the light by slider and wheel
    {
        set_led(LED_M1, 0, true);
        set_led(LED_M2, 0, true);
        open_relay();
        if (TouchPadData->wheel != 0xFFFF)
        {
            ESP_LOGI("BOARD", "WHEEL:%d", TouchPadData->wheel);
            change_halo_light(&light_status, TouchPadData->wheel, interface_touch_status.halo_brightness);
            interface_touch_status.halo_angle = TouchPadData->wheel;
            halo_light_onoff_state = true;
        }

        if (TouchPadData->right_slider != 0xFF)
        {
            if (selected_light == halo_selected)
            {
                change_halo_light(&light_status, interface_touch_status.halo_angle, TouchPadData->right_slider);
                interface_touch_status.halo_brightness = TouchPadData->right_slider;
                ESP_LOGI("BORD", "HALO LIGHT wheel:%d", TouchPadData->right_slider);
                halo_light_onoff_state = true;
            }
            if (selected_light == main_selected)
            {
                change_main_light(&light_status, interface_touch_status.main_colortemp, TouchPadData->right_slider);
                interface_touch_status.main_brightness = TouchPadData->right_slider;
                main_light_onoff_state = true;
            }
        }
        if (TouchPadData->left_slider != 0xFF)
        {
            change_main_light(&light_status, TouchPadData->left_slider, interface_touch_status.main_brightness);
            interface_touch_status.main_colortemp = TouchPadData->left_slider;
            main_light_onoff_state = true;
        }
        xTimerReset(save_timer_handle, 100);
        need_send2_light = true;
        all_light_onoff_state = true;
        break;
    }
    case enum_CCT: // CCT button pressed, remap right slider funcion
    {
        selected_light = main_selected;
        set_led(LED_CCT_PIN, 1, true);
        set_led(LED_RGB_PIN, 0, true);
        break;
    }
    case enum_RGB: // RGB button pressed, remap right slider function
    {
        selected_light = halo_selected;
        set_led(LED_CCT, 0, true);
        set_led(LED_RGB, 1, true);
        break;
    }

    case enum_onoff: // onoff button pressed, toggle the light
    {
        set_led(LED_M1, 0, true);
        set_led(LED_M2, 0, true);
        if (all_light_onoff_state) // any light(main/halo) is open
        {
            memset(&light_status, 0, sizeof(_lightModel)); // close all
            all_light_onoff_state = false;
            close_relay();
        }
        else // open light by last status
        {
            open_relay();
            all_light_onoff_state = true;
            change_halo_light(&light_status, interface_touch_status.halo_angle, interface_touch_status.halo_brightness);
            change_main_light(&light_status, interface_touch_status.main_colortemp, interface_touch_status.main_brightness);
            
        }

        need_send2_light = true;
        break;
    }
    case enum_m1: // sense1 button pressed, change light to this
    {
        set_led(LED_M1, 1, true);
        set_led(LED_M2, 0, true);
        open_relay();
        change_halo_light(&light_status, sense[0].halo_angle, sense[0].halo_brightness);
        change_main_light(&light_status, sense[0].main_colortemp, sense[0].main_brightness);
        need_send2_light = true;
        all_light_onoff_state = true;
        break;
    }
    case enum_m2: // sense2 button pressed, change light to this
    {
        set_led(LED_M1, 0, true);
        set_led(LED_M2, 1, true);
        open_relay();
        change_halo_light(&light_status, sense[1].halo_angle, sense[1].halo_brightness);
        change_main_light(&light_status, sense[1].main_colortemp, sense[1].main_brightness);
        need_send2_light = true;
        all_light_onoff_state = true;
        break;
    }
    case enum_m1_long: // long press m1 button, save current status to sense1
    {
        set_led(LED_M1, 1, true);
        blink_led(LED_M1, 10, 10);
        // sense[0].halo_angle = interface_touch_status.halo_angle;
        memcpy(&sense[0], &interface_touch_status, sizeof(light_hal_t));
        WriteToNVS_blob("sense1", &sense[0], sizeof(light_hal_t), user_nvs_handle);
        break;
    }
    case enum_m2_long: // long press m2 button, save current status to sense2
    {
        memcpy(&sense[1], &interface_touch_status, sizeof(light_hal_t));
        WriteToNVS_blob("sense1", &sense[1], sizeof(light_hal_t), user_nvs_handle);
        break;
    }
    case enum_60s: // auto-close button pressed
    {
        if (toogle_timer_start_flg == false) // if the timer isnot open, start the timer
        {
            xTimerReset(auto_close_timer_handle, 100);
            toogle_timer_start_flg = true;
            //open the light
            open_relay();
            all_light_onoff_state = true;
            change_halo_light(&light_status, interface_touch_status.halo_angle, interface_touch_status.halo_brightness);
            change_main_light(&light_status, interface_touch_status.main_colortemp, interface_touch_status.main_brightness);
            halo_light_onoff_state = true;
            main_light_onoff_state = true;
            need_send2_light = true;
        }
        else // if the timer is open, close the timer
        {
            xTimerStop(auto_close_timer_handle, 100);
            toogle_timer_start_flg = false;
        }
        break;
    }
    case enum_rest: // long press on-off button, rest the light node and start prov
    {
        set_led(LED_ALL, 1, false);
        relay_tick_flg = true;
        xTimerStart(prov_timer_handle, 100);
        Del_all_node();
        ProvSet(true, false);
        WriteToNVS("FactoryMode", 0, user_nvs_handle);
        // todo reset the light node
        break;
    }
    case enum_hall: // hall sensor, means need to prov remote controller
    {
        xTimerStart(prov_timer_handle, 100);
        blink_led(LED_ALL, 5,5);
        ProvSet(true, false);
    }
    case enum_calibrate:
    {
        blink_led(LED_ALL, 1, 5);
        break;
    } default:
        break;
    }

    if (need_send2_light) // if need to change light status
    {
        example_ble_mesh_publish_message(&light_status);
        need_send2_light = false;
    }
}

/**
 * @brief change the halo light color and brightness
 *
 * @param light the light struct
 * @param angle the collor angle, value from 0 to 360
 * @param level the light brightness, value from 0 to 100
 */
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

    ESP_LOGI("ORG RGB", "R:%d ,G:%d ,B:%d", R, G, B);
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
    ESP_LOGI("FIN RGB", "R:%d ,G:%d ,B:%d", ORG_COLOR.R, ORG_COLOR.G, ORG_COLOR.B);
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
    r = input->R / 255.0f;
    g = input->G / 255.0f;
    b = input->B / 255.0f;
    minRGB = MinFunc(r, g, b);
    maxRGB = MaxFunc(r, g, b);

    deltaRGB = maxRGB - minRGB;
    output->COLOR_V = maxRGB;
    if (maxRGB != 0.0f)
        output->COLOR_S = deltaRGB / maxRGB;
    else
        output->COLOR_S = 0.0f;
    if (output->COLOR_S <= 0.0f)
    {
        output->COLOR_H = 0.0f;
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
                output->COLOR_H = 2.0f + (b - r) / deltaRGB;
            }
            else
            {
                if (b == maxRGB)
                {
                    output->COLOR_H = 4.0f + (r - g) / deltaRGB;
                }
            }
        }
        output->COLOR_H = output->COLOR_H * 60.0f;
        if (output->COLOR_H < 0.0f)
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
    if (input->COLOR_S <= 0.0f)
        R = G = B = input->COLOR_V;
    else
    {
        if (input->COLOR_H == 1.0f)
            input->COLOR_H = 0.0f;
        input->COLOR_H *= 6.0f;
        k = (int)floor(input->COLOR_H);
        f = input->COLOR_H - k;
        aa = input->COLOR_V * (1.0f - input->COLOR_S);
        bb = input->COLOR_V * (1.0f - input->COLOR_S * f);
        cc = input->COLOR_V * (1.0f - (input->COLOR_S * (1.0f - f)));
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
