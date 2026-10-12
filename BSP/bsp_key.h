#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#include "stm32f1xx_hal.h"
#include "main.h"

#define KEY1 0
#define KEY2 1
#define KEY3 2

void BSP_Key_Init(void);
uint8_t BSP_Key_Read(uint8_t key_id);  // 返回1=松开，0=按下

#endif
