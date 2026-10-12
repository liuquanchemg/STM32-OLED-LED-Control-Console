#ifndef __MOD_PARAM_H
#define __MOD_PARAM_H

#include "main.h"
#include "mod_led.h" // 为了使用 LedMode_t

// 参数结构体（保存到 Flash 的数据格式）
typedef struct {
    uint32_t magic;           // 有效标志（比如 0xA5A5A5A5，用于判断 Flash 里有没有数据）
    LedMode_t mode;           // 保存的模式
    uint32_t period;          // 保存的闪烁周期
    uint8_t  default_led;     // 默认 LED 状态（上电后灯是亮还是灭）
} Param_t;

void Param_Init(void);            // 初始化：从 Flash 加载参数（若无则用默认）
uint8_t Param_Save(void);         // 保存参数到 Flash（1=成功，0=失败）
uint8_t Param_Reset(void);        // 恢复出厂设置并写入 Flash（1=成功，0=失败）

#endif
