#include "mod_menu.h"
#include "mod_led.h"   
// 菜单状态变量
static MenuPage_t current_page = PAGE_MAIN; // 当前所在页面
static uint8_t cursor = 0;                  // 光标位置（0开始）

// 各页面的选项数量（用于光标循环）
static const uint8_t page_item_counts[] = {
    4, // PAGE_MAIN: 4个选项 (LED控制/闪烁参数/系统信息/串口说明)
    0, // PAGE_LED_CONTROL: 暂时不在这里选，后面再说
    0, // PAGE_BLINK_PARAM
    0, // PAGE_SYS_INFO
    0, // PAGE_UART_HELP
};

void Menu_Init(void) {
    current_page = PAGE_MAIN;
    cursor = 0;
}

void Menu_Process(uint8_t key_event) {
    // 如果是短按：光标往下走
    uint8_t max_items = page_item_counts[current_page];

    // ★ KEY1 短按：光标下移
    if (key_event == KEY_EVENT_KEY1_SHORT) {
        if (max_items > 0) {
            cursor = (cursor + 1) % max_items;
        }
    }

    // ★ KEY1 长按：切换 LED 模式
    else if (key_event == KEY_EVENT_LONG_PRESS) {
        LedMode_t cur = Module_LED_GetMode();
        if (cur == MODE_MANUAL)           Module_LED_SetMode(MODE_FLASH);
        else if (cur == MODE_FLASH)       Module_LED_SetMode(MODE_FAST_FLASH);
        else                              Module_LED_SetMode(MODE_MANUAL);
        Module_LED_IncreaseCount();
    }

    // ★ KEY2 短按：进入子页面 或 执行动作
    else if (key_event == KEY_EVENT_KEY2_SHORT) {
        if (current_page == PAGE_MAIN) {
            if (cursor == 0) current_page = PAGE_LED_CONTROL;
            else if (cursor == 1) current_page = PAGE_BLINK_PARAM;
            else if (cursor == 2) current_page = PAGE_SYS_INFO;
            else if (cursor == 3) current_page = PAGE_UART_HELP;
            cursor = 0;
        }
        else if (current_page == PAGE_LED_CONTROL) {
            Module_LED_Toggle();
            Module_LED_IncreaseCount();
        }
        else if (current_page == PAGE_BLINK_PARAM) {
            uint32_t p = Module_LED_GetPeriod();
            if (p == 500) Module_LED_SetPeriod(800);
            else if (p == 800) Module_LED_SetPeriod(1000);
            else Module_LED_SetPeriod(500);
        }
    }

    // ★ KEY3 短按：返回主菜单
    else if (key_event == KEY_EVENT_KEY3_SHORT) {
        if (current_page != PAGE_MAIN) {
            current_page = PAGE_MAIN;
            cursor = 0;
        }
    }
    
    // ★ 连续按键：光标持续移动，参数持续递增
    else if (key_event == KEY_EVENT_REPEAT) {
        if (current_page == PAGE_MAIN) {
            // 主菜单里，重复 KEY1 相当于光标持续下移
            if (max_items > 0) {
                cursor = (cursor + 1) % max_items;
            }
        }
        else if (current_page == PAGE_BLINK_PARAM) {
            // Blink Param 页面里，重复 KEY1 相当于周期持续递增
            uint32_t p = Module_LED_GetPeriod();
            if (p < 5000) Module_LED_SetPeriod(p + 100);   // 每次 +100ms，上限 5000ms
        }
    }
}   // ★★★ 这个是 Menu_Process 函数的闭合大括号，绝对不能丢！★★★
MenuPage_t Menu_GetCurrentPage(void) { return current_page; }
uint8_t Menu_GetCursor(void) { return cursor; }
uint8_t Menu_GetItemCount(void) { return page_item_counts[current_page]; }
