/*
 * CommandSystem.c
 *
 *  Created on: 2022年3月4日
 *      Author: wxujs
 */
#include "CommandSystem.h"


static CMD_t CMDs[10];
static CMD_Packet_t CMD_Packet;


void NULL_Func(void *arg)
{
	ESP_LOGI("CommandSystem",">>CMD_ERR:illegal command");
	uart_write_bytes(ECHO_UART_PORT_NUM, ">>CMD_ERR:illegal command\r\n", 27);
}
void CommandSystemInit()
{
	CMD_Packet.len=0;
	memcpy(CMDs[CMD_Packet.len].CMD_Head,"NULL_",5);
	CMDs[CMD_Packet.len].Func=NULL_Func;
	CMD_Packet.CMDs=CMDs;
	CMD_Packet.len=1;
	/* Configure parameters of an UART driver,
	 * communication pins and install the driver */
	uart_config_t uart_config = {
		.baud_rate = ECHO_UART_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 122,
		 };
	int intr_alloc_flags = 0;

	ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
	ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

	// Configure a temporary buffer for the incoming data

	uart_write_bytes(ECHO_UART_PORT_NUM, "\r\nBLE Mesh [Provisioner]", 24);
	uart_write_bytes(ECHO_UART_PORT_NUM, "\r\nYuFeng Tec.\r\nBuild time:2022/3/4 Version:V3.0.0 Written by:Wangxiang\r\n", 78);
	uart_flush_input(ECHO_UART_PORT_NUM);
	uart_write_bytes(ECHO_UART_PORT_NUM, "\r\n>>CMD_RDY\r\n", 15);
	ESP_LOGI("CommandSystem",">>CMD_RDY");
}

void CommandReg(char cmd[8],void(*Func)(void *arg))
{
	memcpy(CMDs[CMD_Packet.len].CMD_Head,cmd,8);
	CMDs[CMD_Packet.len].Func=Func;
	CMD_Packet.len++;
}

/**
 * @fn CMD DecodeCommandHead(int, char*)
 * @brief 判断输入的字符串内是否有指令，并解析指令的类型并运行对应的服务函数
 *
 * @param len 传入的字符串长度
 * @param str 传入的字符串
 * @return 指令的类型
 */
uint8_t DecodeCommandHead(int len, char *str)
{
	int i = 0;
	for (i = 0; i < len; i++)
	{
		if (str[i] == '$')
		{
			for (int j = i; j < len; j++)
			{
				if (str[j] == ';')
					goto CheckIsaCMD;
			}
		}
	}
	return 0;
CheckIsaCMD:
	for (int x = 1; x < CMD_Packet.len; x++)
	{
		if (memcmp(&str[i + 1], CMD_Packet.CMDs[x].CMD_Head, 7) == 0)
		{
			strtok(str, ";");
			str = &str[i];
			CMD_Packet.CMDs[x].Func(str);
			return x;
		}
	}
	CMD_Packet.CMDs[0].Func(str);
	return 0;
}

/**
 * @fn uint8_t DecodeCommandValue(char*, uint8_t[])
 * @brief 解析命令的数据
 *
 * @param str 命令字符串
 * @param data 解析出来的数据数组
 * @return 解析出来的数据个数
 */
uint8_t DecodeCommandValue(char *str, uint8_t data[])
{
	uint8_t count = 0, j = 0;
	uint8_t _data[8] = { '\0' };
	char *_str = Slpit(str, ":", 1);
	uint8_t _strlen = getStrLen(_str);
	for (int i = 0; i < _strlen + 1; i++)
	{
		if (_str[i] == ',' || _str[i] == '\0')
		{
			data[count] = Touint((char*) _data);
//			CleanArray(_data);
			bzero(_data, 8);
			j = 0;
			count++;
		}
		else
		{
			_data[j] = _str[i];
			j++;
		}
	}
	return count;
}


/**
 * @fn char Slpit*(char*, const char*, uint8_t)
 * @brief 分割字符串
 *
 * @param s 需要被分割的字符串地址
 * @param delim 分割的字符,可不止一个例如:"-,"
 * @param ArrayNo 返回分割后第几段的首地址,从0开始
 * @return 返回分割后第几段的首地址
 */
char* Slpit(char *s, const char *delim, uint8_t ArrayNo)
{
	char *address = strtok(s, delim);
	for (int i = 0; i < ArrayNo; i++)
		address = strtok(NULL, delim);
	return address;
}

/**
 * @fn uint8_t getStrLen(char*)
 * @brief 获取字符串长度(\0的位置)
 *
 * @param str 字符串地址
 * @return 字符串长度
 */
uint8_t getStrLen(char *str)
{
	int i = 0;
	for (i = 0; str[i] != '\0'; i++)
		;
	return i;
}

/**
 * @fn unsigned int Touint(char*)
 * @brief 将char字符串转化为uint
 *
 * @param value 字符串
 * @return 数字
 */
unsigned int Touint(char *value)
{
	uint8_t len = getStrLen(value);
	int res = 0;
	for (int i = 0; i < len; i++)
	{
		if (value[i] < 48 || value[i] > 57)
		{
			return res;
		}
		else
			res = res + (value[i] - 48) * (int) pow(10, len - 1 - i);
	}
	return res;
}
