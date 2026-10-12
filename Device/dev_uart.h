#ifndef __DEV_UART_H
#define __DEV_UART_H

#include <stdint.h>
#include "main.h"
#include "usart.h" // ★ 必须包含，否则找不到 huart2 和 UART_HandleTypeDef

void UART_Init(void);
uint8_t UART_IsDataAvailable(void); // ★ 修正了拼写
uint8_t UART_ReadByte(void);

#endif
