#include "mod_led.h"
#include "bsp_led.h"
#include <stdint.h>
#include <string.h>

static uint8_t led_state = 0;       // 0: OFF, 1: ON
static LedMode_t led_mode = MODE_MANUAL;
static uint32_t run_count = 0;
static char last_error[16] = "NONE";

// 闪烁模式的时间控制
static uint32_t last_flash_time = 0;
static uint32_t flash_period = 500; // 私有变量，默认500ms

// 串口命令统计
static uint32_t cmd_ok_count = 0;     // 成功命令计数
static uint32_t cmd_err_count = 0;    // 错误命令计数

// 最近错误信息
void Module_LED_SetLastError(const char *err) {
    strncpy(last_error, err, sizeof(last_error) - 1);
    last_error[sizeof(last_error) - 1] = '\0';
}

const char* Module_LED_GetLastError(void) {
    return last_error;
}

void Module_LED_Init(void) {
    led_state = 0;
    led_mode = MODE_MANUAL;
    run_count = 0;
    BSP_LED_Off();
}

// 这个函数需要在 App_Loop 里不断调用
void Module_LED_Process(void) {

    // 闪烁模式：统一使用 flash_period（可被 SET PERIOD 修改，验收现象才成立）
    if (led_mode == MODE_FLASH || led_mode == MODE_FAST_FLASH) {
        if (HAL_GetTick() - last_flash_time >= flash_period) {
            last_flash_time = HAL_GetTick();
            led_state = !led_state;
        }
    }

    // 统一根据当前状态控制硬件（低电平点亮）
    if (led_state) {
        BSP_LED_On();
    } else {
        BSP_LED_Off();
    }
}

void Module_LED_SetState(uint8_t state) {
    led_mode = MODE_MANUAL; // 串口强制控制时，自动切回手动模式
    led_state = state;
}

void Module_LED_Toggle(void) {
    led_mode = MODE_MANUAL;
    led_state = !led_state;
}

uint8_t Module_LED_GetState(void) { return led_state; }
void Module_LED_SetMode(LedMode_t mode) { led_mode = mode; }
LedMode_t Module_LED_GetMode(void) { return led_mode; }
uint32_t Module_LED_GetCount(void) { return run_count; }
void Module_LED_IncreaseCount(void) { run_count++; }
void Module_LED_SetPeriod(uint32_t period) { flash_period = period;}//设置周期
uint32_t Module_LED_GetPeriod(void) {return flash_period;}  //读取周期
void Module_LED_IncreaseCmdOk(void)  { cmd_ok_count++; }
void Module_LED_IncreaseCmdErr(void) { cmd_err_count++; }
uint32_t Module_LED_GetCmdOkCount(void)  { return cmd_ok_count; }
uint32_t Module_LED_GetCmdErrCount(void) { return cmd_err_count; }

