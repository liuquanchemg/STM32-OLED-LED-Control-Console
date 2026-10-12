#ifndef __TASK_UART_H
#define __TASK_UART_H

#include "main.h"

// 对外提供的任务处理接口
void Task_UART_Process(void);
uint32_t Task_UART_GetOkCount(void);   // 串口命令成功计数
uint32_t Task_UART_GetErrCount(void);  // 串口命令错误计数

#endif
