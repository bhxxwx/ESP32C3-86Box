/*
 * @Author: Wangxiang
 * @Date: 2022-02-23 10:36:05
 * @LastEditTime: 2022-05-17 17:46:23
 * @LastEditors: Wangxiang
 * @Description: 
 * @FilePath: /ESPC3_Client_86Prov/main/board.h
 * 江苏大学-王翔
 */
/* board.h - Board-specific hooks */

#ifndef _BOARD_H_
#define _BOARD_H_
#include "Servers.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

#define _MinFunc(a, b) (a > b ? b : a)
#define MinFunc(a, b, c) _MinFunc(_MinFunc(a, b), c)

#define _MaxFunc(a, b) (a > b ? a : b)
#define MaxFunc(a, b, c) _MaxFunc(_MaxFunc(a, b), c)

/**
 * @def MAX_COLOR_TEMP
 * @brief
 *
 */
#define MAX_COLOR_TEMP 100
/**
 * @def MIN_COLOR_TEMP
 * @brief
 *
 */
#define MIN_COLOR_TEMP 0
/**
 * @def PWM_DUTY
 * @brief
 *
 */
#define PWM_DUTY 255

#define LED_ONOFF_PIN GPIO_NUM_18
#define LED_CCT_PIN GPIO_NUM_10
#define LED_RGB_PIN GPIO_NUM_19
#define LED_M1_PIN GPIO_NUM_3
#define LED_M2_PIN GPIO_NUM_7
#define LED_AUTO_PIN GPIO_NUM_6

#define LED_ONOFF 0x01
#define LED_CCT 0x02
#define LED_RGB 0x04
#define LED_M1 0x08
#define LED_M2 0x10
#define LED_AUTO 0x20
#define LED_NUMBER 6
#define LED_ALL 0x3F

// enum
// {
// 	enLED_ONOFF = 0x01,
// 	enLED_CCT=0x02,
// 	enLED_RGB=0X04,
// 	enLED_M1=0X08,
// 	enLED_M2=0X10,
// 	enLED_AUTO=0X20,
// 	enLED_ALL
// }

typedef struct
{
	bool level_flg;
	bool normal_level_flg;
	uint8_t blink_pin;
	uint8_t start_flg;
	
	uint16_t target_count;
	uint16_t count;
	uint16_t target_cycle;
	uint16_t cycle_count;
}blink_t;

typedef struct
{
	uint8_t R;
	uint8_t G;
	uint8_t B;
	uint8_t CW;
	uint8_t WW;
	uint8_t CMD;
} _lightModel;

typedef struct 
{
	uint16_t halo_angle;
	uint8_t halo_brightness;
	uint8_t main_colortemp;
	uint8_t main_brightness;

}light_hal_t;

typedef struct 
{
	uint16_t wheel;
	uint8_t left_slider;
	uint8_t right_slider;
	uint8_t btns;
}ITouchPad_t;

enum
{
	enum_no_btn,
	enum_onoff,
	enum_rest,
	enum_hall,
	enum_CCT,
	enum_RGB,
	enum_60s,
	enum_m1,
	enum_m2,
	enum_m1_long,
	enum_m2_long,
	enum_calibrate,
} enum_btns;

typedef struct
{
	uint8_t op_code;//first byte
	uint8_t func_code;//senond byte
	uint8_t value_code;//third byte
}IRC_t;

enum
{
	enum_rc_op_light_level=1,
	enum_rc_op_sense_change,
	enum_rc_op_sense_set,
	enum_rc_op_toggle_light,
	enum_rc_op_dance,
} enum_remote_controller_op;

enum
{
	enum_rc_toggle_all=1,
	enum_rc_toggle_halo,
	enum_rc_toggle_main,
} enum_remote_controller_toggle_light_func;

enum
{
	enum_rc_level_cw = 1,
	enum_rc_level_light_level,
	enum_rc_level_color,
	enum_rc_level_color_level,
} enum_remote_controller_light_level_func;

typedef struct
{
	float COLOR_H;
	float COLOR_S;
	float COLOR_V;
} COLOR_HSV;

typedef struct
{
	uint8_t R;
	uint8_t G;
	uint8_t B;
	uint8_t l;
} COLOR_RGB;

enum
{
	main_selected,
	halo_selected,
} light_selected;


void board_init(nvs_handle_t nvs_handle);
void remote_controler_opc(IRC_t *RemoteControllerData);
void light_opc(ITouchPad_t *TouchPadData);
void change_main_light(_lightModel *light, uint8_t color_temp, uint8_t level);
void change_halo_light(_lightModel *light, uint16_t angle, uint8_t level);

void set_led(uint16_t led_pin, uint8_t level, bool normal_flg);
void open_led(uint16_t led_pin);
void close_led(uint16_t led_pin);
void stop_blink_led(uint16_t led_pin);

void blink_led(uint16_t led_pin, uint16_t blink_ms, uint16_t cycle);
void blink_tick();
void relay_reset_tick();

void open_relay();
void close_relay();
#endif /* _BOARD_H_ */
