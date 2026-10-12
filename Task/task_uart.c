#include "task_uart.h"    // 包含自己的头文件
#include "dev_uart.h"     // ★ 引用 Device 层：为了使用 UART_IsDataAvailable 和 UART_ReadByte
#include <string.h>       // 为了使用 strcmp 和 strlen
#include <stdio.h>        // 为了使用 sprintf
#include <stdlib.h>       // 为了使用 strtoul
#include "mod_led.h"      // ★ 引用 Module 层：为了使用 led_state 和 run_count
#include "mod_param.h"    // ★ 引用 Module 层：为了使用 Param_Save / Param_Reset
#include "comp_key.h"     // ★ 为了使用 Key_GetPressCount（按键事件计数）

// 串口命令接收缓冲区（任务书建议最大 64 字符）
char cmd_buffer[64];
uint8_t cmd_index = 0;
uint8_t cmd_too_long = 0;   // 命令太长标志

// ★ 串口命令统计（成功/失败计数放在 mod_led 里统一管理）
static uint32_t cmd_ok_count = 0;
static uint32_t cmd_err_count = 0;

// ★ 统一的发送接口：长度用 strlen 自动计算，避免硬编码长度越界读取（对照总结必修3）
static void Cmd_Reply(const char *s, uint8_t is_ok) {
    HAL_UART_Transmit(&huart2, (uint8_t*)s, strlen(s), 10);
    if (is_ok) { cmd_ok_count++;  Module_LED_IncreaseCmdOk(); }
    else       { cmd_err_count++; Module_LED_IncreaseCmdErr(); }
}

uint32_t Task_UART_GetOkCount(void)  { return cmd_ok_count; }
uint32_t Task_UART_GetErrCount(void) { return cmd_err_count; }

// ★ 去除命令首尾空格（任务书 6.4.4：解析前应去除首尾空格）
static void Cmd_Trim(void) {
    uint16_t s = 0, e = cmd_index;
    while (s < e && cmd_buffer[s] == ' ') s++;
    while (e > s && cmd_buffer[e - 1] == ' ') e--;
    cmd_buffer[e] = '\0';
    if (s > 0) memmove(cmd_buffer, cmd_buffer + s, e - s + 1);
}

// 获取当前模式字符串（复用，避免多处重复判断）
static const char* Mode_ToStr(void) {
    if (Module_LED_GetMode() == MODE_FLASH)           return "BLINK";
    else if (Module_LED_GetMode() == MODE_FAST_FLASH) return "FAST_BLINK";
    else                                              return "MANUAL";
}

/**
  * @brief 串口命令解析函数（非阻塞）
  * @note  在主循环 while(1) 中不断调用
  */
void Task_UART_Process(void)
{
    // 1. 只要 Device 层的环形缓冲区里有未处理的数据
    while (UART_IsDataAvailable()) 
    {
        // 2. 从环形缓冲区取出一个字节
        uint8_t received_byte = UART_ReadByte();

        // 3. 判断是否是一行命令的结束符（\r 或 \n）
        if (received_byte == '\n' || received_byte == '\r') 
        {
            // ★ 超长标志已经处理过了，这里只清空，准备接下一句
            if (cmd_too_long) 
            {
                cmd_index = 0;
                cmd_too_long = 0;
            }
            // 如果确实收到了内容
            else if (cmd_index > 0) 
            {
                cmd_buffer[cmd_index] = '\0'; // 字符串结尾加结束符

                // ★ 去除首尾空格后再匹配
                Cmd_Trim();

                // 开始匹配命令
                if (strcmp(cmd_buffer, "HELP") == 0) 
                {
                    Cmd_Reply("HELP: LED ON/OFF/TOGGLE\r\n"
                              "      SET MODE 0/1/2\r\n"
                              "      SET PERIOD <ms>\r\n"
                              "      GET STATUS/PARAM\r\n"
                              "      SAVE / RESET\r\n", 1);
                }
                else if (strcmp(cmd_buffer, "LED ON") == 0) 
                {
                    Module_LED_SetState(1);
                    Module_LED_IncreaseCount();
                    Cmd_Reply("OK LED=ON\r\n", 1);
                }
                else if (strcmp(cmd_buffer, "LED OFF") == 0) 
                {
                    Module_LED_SetState(0);
                    Module_LED_IncreaseCount();
                    Cmd_Reply("OK LED=OFF\r\n", 1);
                }
                else if (strcmp(cmd_buffer, "LED TOGGLE") == 0) 
                {
                    Module_LED_Toggle();
                    Module_LED_IncreaseCount();
                    Cmd_Reply(Module_LED_GetState() ? "OK LED=ON\r\n" : "OK LED=OFF\r\n", 1);
                }
                else if (strcmp(cmd_buffer, "STATUS") == 0) 
                {
                    char status_msg[80];
                    sprintf(status_msg, "MODE=%s LED=%s COUNT=%04d RUN=%lus\r\n",
                            Mode_ToStr(),
                            Module_LED_GetState() ? "ON" : "OFF",
                            (int)Module_LED_GetCount(),
                            (unsigned long)(HAL_GetTick() / 1000));
                    Cmd_Reply(status_msg, 1);
                }
                // 保存参数到 Flash
                else if (strcmp(cmd_buffer, "SAVE") == 0) 
                {
                    if (Param_Save()) {
                        Cmd_Reply("OK SAVE\r\n", 1);
                    } else {
                        Module_LED_SetLastError("SAVE FAIL");
                        Cmd_Reply("ERROR: SAVE FAIL\r\n", 0);
                    }
                }
                // 恢复出厂设置（★ 现在会同时写入 Flash，断电重启后也是默认值）
                else if (strcmp(cmd_buffer, "RESET") == 0) 
                {
                    if (Param_Reset()) {
                        Cmd_Reply("OK RESET\r\n", 1);
                    } else {
                        Module_LED_SetLastError("RESET FAIL");
                        Cmd_Reply("ERROR: RESET FAIL\r\n", 0);
                    }
                }
                // 设置模式（★ 严格校验：数字后不能有多余字符，拒绝 "SET MODE 10"）
                else if (strncmp(cmd_buffer, "SET MODE ", 9) == 0) 
                {
                    char mode_num = cmd_buffer[9];
                    if (cmd_buffer[10] != '\0') {
                        Module_LED_SetLastError("MODE FORMAT");
                        Cmd_Reply("ERROR: MODE FORMAT\r\n", 0);
                    }
                    else if (mode_num == '0') {
                        Module_LED_SetMode(MODE_MANUAL);
                        Cmd_Reply("OK MODE=MANUAL\r\n", 1);
                    } else if (mode_num == '1') {
                        Module_LED_SetMode(MODE_FLASH);
                        Module_LED_SetPeriod(500); // 模式默认周期，之后可用 SET PERIOD 覆盖
                        Cmd_Reply("OK MODE=BLINK\r\n", 1);
                    } else if (mode_num == '2') {
                        Module_LED_SetMode(MODE_FAST_FLASH);
                        Module_LED_SetPeriod(200);
                        Cmd_Reply("OK MODE=FAST_BLINK\r\n", 1);
                    } else {
                        Module_LED_SetLastError("MODE RANGE");
                        Cmd_Reply("ERROR: MODE RANGE\r\n", 0);
                    }
                }
                // ★ 提高题命令：SET LED ON
                else if (strcmp(cmd_buffer, "SET LED ON") == 0) 
                {
                    Module_LED_SetState(1);
                    Module_LED_IncreaseCount();
                    Cmd_Reply("OK LED=ON\r\n", 1);
                }
                // ★ 提高题命令：SET LED OFF
                else if (strcmp(cmd_buffer, "SET LED OFF") == 0) 
                {
                    Module_LED_SetState(0);
                    Module_LED_IncreaseCount();
                    Cmd_Reply("OK LED=OFF\r\n", 1);
                }
                // 设置周期（★ 用 strtoul + 结束指针严格校验，拒绝 "SET PERIOD 500abc"）
                else if (strncmp(cmd_buffer, "SET PERIOD ", 11) == 0)
                {
                    char *end_ptr = NULL;
                    unsigned long period = strtoul(&cmd_buffer[11], &end_ptr, 10);

                    if (*end_ptr != '\0') {
                        Module_LED_SetLastError("PERIOD FORMAT");
                        Cmd_Reply("ERROR: PERIOD FORMAT\r\n", 0);
                    }
                    else if (period >= 100 && period <= 5000) {
                        Module_LED_SetPeriod((uint32_t)period);
                        char msg[40];
                        sprintf(msg, "OK PERIOD=%lums\r\n", period);
                        Cmd_Reply(msg, 1);
                    } else {
                        Module_LED_SetLastError("PERIOD RANGE");
                        Cmd_Reply("ERROR: PERIOD RANGE\r\n", 0);
                    }
                }
                // 获取参数
                else if (strcmp(cmd_buffer, "GET PARAM") == 0)
                {
                    char msg[80];
                    sprintf(msg, "MODE=%s PERIOD=%dms\r\n",
                            Mode_ToStr(),
                            (int)Module_LED_GetPeriod());
                    Cmd_Reply(msg, 1);
                }
                // 获取状态（★ 补全：运行时间 + 按键计数 + 命令成功/错误计数）
                else if (strcmp(cmd_buffer, "GET STATUS") == 0)
                {
                    char msg[120];
                    sprintf(msg, "MODE=%s LED=%s COUNT=%04d RUN=%lus KEY=%lu OK=%lu ERR=%lu\r\n",
                            Mode_ToStr(),
                            Module_LED_GetState() ? "ON" : "OFF",
                            (int)Module_LED_GetCount(),
                            (unsigned long)(HAL_GetTick() / 1000),
                            (unsigned long)Key_GetPressCount(),
                            (unsigned long)cmd_ok_count,
                            (unsigned long)cmd_err_count);
                    Cmd_Reply(msg, 1);
                }
                else 
                {
                    Module_LED_SetLastError("UNKNOWN");
                    Cmd_Reply("ERROR: Unknown Command\r\n", 0);
                }

                // 处理完一条命令后，清空当前命令缓冲区，准备接收下一条
                cmd_index = 0; 
            }
        }
        else 
        {
            // 4. 如果不是结束符，就把字符追加到命令缓冲区
            if (cmd_index >= sizeof(cmd_buffer) - 1) 
            {
                // ★ 只在第一次超过缓冲区时，立刻返回错误，并标记
                if (!cmd_too_long) 
                {
                    cmd_too_long = 1;
                    Module_LED_SetLastError("TOO LONG");
                    Cmd_Reply("ERROR: COMMAND TOO LONG\r\n", 0);
                }
            } 
            else 
            {
                cmd_buffer[cmd_index++] = received_byte;
            }
        }
    }
}
