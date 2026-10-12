#ifndef __MOD_LED_H
#define __MOD_LED_H

#include "main.h"

// 定义LED工作模式
typedef enum {
    MODE_MANUAL = 0, // 手动模式
    MODE_FLASH,       // 闪烁模式
    MODE_FAST_FLASH  // 快速闪烁模式
} LedMode_t;

void Module_LED_Init(void);
void Module_LED_Process(void);
void Module_LED_SetState(uint8_t state);
void Module_LED_Toggle(void);
uint8_t Module_LED_GetState(void);
void Module_LED_SetMode(LedMode_t mode);
LedMode_t Module_LED_GetMode(void);
uint32_t Module_LED_GetCount(void);
void Module_LED_IncreaseCount(void);
void Module_LED_SetPeriod(uint32_t period);
uint32_t Module_LED_GetPeriod(void);
// 串口命令统计
void Module_LED_IncreaseCmdOk(void);        // 成功命令 +1
void Module_LED_IncreaseCmdErr(void);       // 错误命令 +1
uint32_t Module_LED_GetCmdOkCount(void);    // 读成功计数
uint32_t Module_LED_GetCmdErrCount(void);   // 读错误计数

// 最近错误信息
void Module_LED_SetLastError(const char *err);
const char* Module_LED_GetLastError(void);
#endif

