#include "app.h"
#include "main.h"
#include "oled.h"
#include "font.h"
#include <stdio.h>
#include "dev_uart.h"
#include "comp_key.h"
#include "bsp_key.h"
#include "mod_led.h"
#include "mod_param.h"
#include "mod_menu.h"    // ★ 加回菜单模块
#include "bsp_led.h"
#include "task_uart.h"

void App_Init(void) {
    OLED_Init();
    UART_Init();
    Key_Component_Init();
    Module_LED_Init();
    Menu_Init();         // ★ 初始化菜单
    Param_Init();

    // ★ 上电自检：显示项目信息 + LED 闪烁一次
    OLED_NewFrame();
    OLED_PrintString(0, 0,  "STM32 CONTROL", &font16x16, OLED_COLOR_NORMAL);
    OLED_PrintString(0, 16, "OLED CONSOLE", &font16x16, OLED_COLOR_NORMAL);
    OLED_PrintString(0, 32, "VER: V1.0.0", &font16x16, OLED_COLOR_NORMAL);
    OLED_PrintString(0, 48, "SELF TEST...", &font16x16, OLED_COLOR_NORMAL);
    OLED_ShowFrame();

    // LED 自检：亮 200ms，灭 200ms，亮 200ms，灭
    BSP_LED_On();  HAL_Delay(200);
    BSP_LED_Off(); HAL_Delay(200);
    BSP_LED_On();  HAL_Delay(200);
    BSP_LED_Off(); HAL_Delay(500);

    // 打印启动信息（串口）
    HAL_UART_Transmit(&huart2, (uint8_t*)"STM32 Console Boot OK\r\n", 24, 100);
}

void App_Loop(void) {
    // 1. 串口命令解析
    Task_UART_Process();

    // 2. 按键扫描（分别扫描三个按键）
    KeyEvent_t e1 = Key_Scan_StateMachine(KEY1);
    KeyEvent_t e2 = Key_Scan_StateMachine(KEY2);
    KeyEvent_t e3 = Key_Scan_StateMachine(KEY3);
    if (e1 != KEY_EVENT_NONE) Menu_Process(e1);
    if (e2 != KEY_EVENT_NONE) Menu_Process(e2);
    if (e3 != KEY_EVENT_NONE) Menu_Process(e3);

    // 3. LED 业务逻辑处理
    Module_LED_Process();

    // 4. OLED 菜单刷新（每200ms刷一次，非阻塞）
    static uint32_t last_oled_time = 0;
    if (HAL_GetTick() - last_oled_time >= 200) {  // ★ 关键：时间限流
        last_oled_time = HAL_GetTick();
        OLED_NewFrame();

        // ★ 根据当前页面绘制不同内容
        switch (Menu_GetCurrentPage()) {
            case PAGE_MAIN:
        // ★ 顶部状态栏（8x6 小字体）：模式 + LED 状态 + 开关计数（任务书 6.2 要求主界面显示）
        {
            char status[24];
            const char *mode_str = "MANUAL";
            if (Module_LED_GetMode() == MODE_FLASH)           mode_str = "BLINK";
            else if (Module_LED_GetMode() == MODE_FAST_FLASH) mode_str = "FAST";
            sprintf(status, "M:%s L:%s C:%04d", mode_str,
                    Module_LED_GetState() ? "ON" : "OFF",
                    (int)Module_LED_GetCount());
            OLED_PrintASCIIString(0, 0, status, &afont8x6, OLED_COLOR_NORMAL);
        }
        // ★ 4 个菜单项改用 12x6 字体：原"16x16 标题+4项"共 5 行会超出 64px 高度，第 4 项被裁掉
        {
            const char *items[] = {"LED Control", "Blink Param", "System Info", "UART Help"};
            for (int i = 0; i < 4; i++) {
                char line[24];
                sprintf(line, "%s %s", (i == Menu_GetCursor()) ? ">" : " ", items[i]);
                OLED_PrintASCIIString(0, 12 + i * 12, line, &afont12x6, OLED_COLOR_NORMAL);
            }
        }
        break;

            case PAGE_LED_CONTROL:
                OLED_PrintString(0, 0,  "== LED CTRL ==", &font16x16, OLED_COLOR_NORMAL);
                OLED_PrintString(0, 16, "KEY2: TOGGLE LED", &font16x16, OLED_COLOR_NORMAL);
                OLED_PrintString(0, 32, Module_LED_GetState() ? "STATE: ON" : "STATE: OFF", &font16x16, OLED_COLOR_NORMAL);
                OLED_PrintString(0, 48, "KEY3: BACK",   &font16x16, OLED_COLOR_NORMAL);
                break;

            case PAGE_BLINK_PARAM:
                OLED_PrintString(0, 0,  "== BLINK PARAM ==", &font16x16, OLED_COLOR_NORMAL);
                OLED_PrintString(0, 16, "KEY2: CHANGE PERIOD", &font16x16, OLED_COLOR_NORMAL);
                {
                    char msg[20];
                    sprintf(msg, "PERIOD: %dms", (int)Module_LED_GetPeriod());
                    OLED_PrintString(0, 32, msg, &font16x16, OLED_COLOR_NORMAL);
                }
                OLED_PrintString(0, 48, "KEY3: BACK",   &font16x16, OLED_COLOR_NORMAL);
                break;

            case PAGE_SYS_INFO:
            // ★ 系统信息全部用 8x6 小字体，显示完整状态（任务书 7.6：版本/运行时间/模式/LED/按键计数/命令计数/错误/最近错误/OLED 状态）
            {
                char msg[32];
                const char *mode_str = "MANUAL";
                if (Module_LED_GetMode() == MODE_FLASH)           mode_str = "BLINK";
                else if (Module_LED_GetMode() == MODE_FAST_FLASH) mode_str = "FAST_BLINK";

                OLED_PrintASCIIString(0, 0,  "== SYS INFO ==", &afont8x6, OLED_COLOR_NORMAL);
                sprintf(msg, "VER:V1.0.0 RUN:%lus", (unsigned long)(HAL_GetTick() / 1000));
                OLED_PrintASCIIString(0, 8,  msg, &afont8x6, OLED_COLOR_NORMAL);
                sprintf(msg, "MODE:%s LED:%s", mode_str, Module_LED_GetState() ? "ON" : "OFF");
                OLED_PrintASCIIString(0, 16, msg, &afont8x6, OLED_COLOR_NORMAL);
                sprintf(msg, "KEY:%lu CMD:%lu", (unsigned long)Key_GetPressCount(),
                        (unsigned long)Module_LED_GetCmdOkCount());
                OLED_PrintASCIIString(0, 24, msg, &afont8x6, OLED_COLOR_NORMAL);
                sprintf(msg, "ERR:%lu OLED:%s", (unsigned long)Module_LED_GetCmdErrCount(),
                        OLED_IsError() ? "BAD" : "OK");
                OLED_PrintASCIIString(0, 32, msg, &afont8x6, OLED_COLOR_NORMAL);
                sprintf(msg, "LAST:%s", Module_LED_GetLastError());
                OLED_PrintASCIIString(0, 40, msg, &afont8x6, OLED_COLOR_NORMAL);
            }
            break;
            case PAGE_UART_HELP:
                OLED_PrintString(0, 0,  "== UART HELP ==", &font16x16, OLED_COLOR_NORMAL);
                OLED_PrintString(0, 16, "LED ON/OFF", &font16x16, OLED_COLOR_NORMAL);
                OLED_PrintString(0, 32, "SET MODE 0/1/2", &font16x16, OLED_COLOR_NORMAL);
                OLED_PrintString(0, 48, "SET PERIOD <ms>", &font16x16, OLED_COLOR_NORMAL);
                break;
            default:
                break;
        }
        OLED_ShowFrame();
    }
}
