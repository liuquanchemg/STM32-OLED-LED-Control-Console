#ifndef __MOD_MENU_H
#define __MOD_MENU_H

#include "main.h"
#include "comp_key.h"  // ★ 包含按键事件枚举定义

// 定义所有菜单页面
typedef enum {
    PAGE_MAIN = 0,        // 主菜单
    PAGE_LED_CONTROL,     // LED 控制子页面
    PAGE_BLINK_PARAM,     // 闪烁参数子页面
    PAGE_SYS_INFO,        // 系统信息
    PAGE_UART_HELP,       // 串口命令说明
    PAGE_MAX
} MenuPage_t;

void Menu_Init(void);
void Menu_Process(uint8_t key_event); // 传入按键事件，更新菜单状态
MenuPage_t Menu_GetCurrentPage(void); // 获取当前页面（给OLED画图用）
uint8_t Menu_GetCursor(void);         // 获取当前光标位置
uint8_t Menu_GetItemCount(void);      // 获取当前页面的选项数

#endif
