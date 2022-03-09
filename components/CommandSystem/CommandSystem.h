/*
 * CommandSystem.h
 *
 *  Created on: 2022年3月4日
 *      Author: wxujs
 */

#ifndef COMPONENTS_COMMANDSYSTEM_COMMANDSYSTEM_H_
#define COMPONENTS_COMMANDSYSTEM_COMMANDSYSTEM_H_
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "driver/uart.h"
#include "esp_log.h"
#define USEUART 1
#if(USEUART==0)
#define ECHO_UART_PORT_NUM      (0)
#define ECHO_TEST_TXD (21)//TXD引脚号
#define ECHO_TEST_RXD (20)//RXD引脚号
#elif(USEUART==1)
#define ECHO_UART_PORT_NUM      (1)
#define ECHO_TEST_TXD (5)//TXD引脚号
#define ECHO_TEST_RXD (4)//RXD引脚号
#endif
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)
#define ECHO_UART_BAUD_RATE     (115200)
#define ECHO_TASK_STACK_SIZE    (4096)
#define BUF_SIZE (1024)


typedef struct
{
		char CMD_Head[8];
		void (*Func)(void *arg);
} CMD_t;

typedef struct
{
		CMD_t *CMDs;
		uint8_t len;
}CMD_Packet_t;


char* Slpit(char *s, const char *delim, uint8_t ArrayNo);
unsigned int Touint(char *value);
uint8_t getStrLen(char *str);
void CleanArray(uint8_t *array);


void CommandSystemInit();
void CommandReg(char cmd[8],void(*Func)(void *arg));
uint8_t DecodeCommandHead(int len, char *str);
uint8_t DecodeCommandValue(char *str, uint8_t data[]);

#endif /* COMPONENTS_COMMANDSYSTEM_COMMANDSYSTEM_H_ */
