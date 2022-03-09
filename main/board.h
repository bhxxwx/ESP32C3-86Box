/* board.h - Board-specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 * Additional Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _BOARD_H_
#define _BOARD_H_


typedef struct
{
	uint8_t R;
	uint8_t G;
	uint8_t B;
	uint8_t CW;
	uint8_t WW;
	uint8_t CMD;
} _lightModel;

void board_init(void);

#endif /* _BOARD_H_ */
