#include "mod_param.h"
#include "main.h"
#include "stm32f1xx_hal_flash.h"     // ★ 必须加！为了 FLASH_TYPEPROGRAM_WORD
#include "stm32f1xx_hal_flash_ex.h"  // ★ 必须加！为了 FLASH_EraseInitTypeDef 和 HAL_FLASHEx_Erase
// ★ 选择 Flash 的最后一页作为参数存储区
// STM32F103C8T6 是 64KB Flash，最后一页的起始地址是 0x0800FC00
#define PARAM_FLASH_ADDR  0x0800FC00
#define PARAM_MAGIC       0xA5A5A5A5

static Param_t current_param; // 当前在内存里的参数副本

// ★ 参数有效性检查：防止 Flash 内容损坏（magic 正确但数据被破坏）时加载非法值
static uint8_t Param_IsValid(const Param_t *p) {
    if (p->magic != PARAM_MAGIC) return 0;
    if (p->mode != MODE_MANUAL && p->mode != MODE_FLASH && p->mode != MODE_FAST_FLASH) return 0;
    if (p->period < 100 || p->period > 5000) return 0;
    if (p->default_led > 1) return 0;
    return 1;
}

// 从 Flash 读取参数
void Param_Init(void) {
    // 把 Flash 里的数据强制转换成 Param_t 指针，然后读出来
    Param_t *flash_param = (Param_t *)PARAM_FLASH_ADDR;

    // 判断魔术字 + 参数范围是否都合法
    if (Param_IsValid(flash_param)) {
        // Flash 里有有效数据，加载它
        current_param = *flash_param;
    } else {
        // Flash 里没有有效数据（第一次上电或数据损坏），使用默认值
        current_param.magic = PARAM_MAGIC;
        current_param.mode = MODE_MANUAL;
        current_param.period = 500;
        current_param.default_led = 0; // 默认灯灭
    }

    // 把加载到的参数应用到 Module 层
    // ★ 顺序注意：Module_LED_SetState() 内部会把模式重置为 MANUAL，
    //   所以 SetMode 必须放在最后，确保恢复的是保存的模式
    Module_LED_SetPeriod(current_param.period);
    Module_LED_SetState(current_param.default_led);
    Module_LED_SetMode(current_param.mode);
}

// ★ 统一写入 Flash 的流程：返回 1=成功，0=失败（擦除或写入出错时上报）
static uint8_t Param_FlashWrite(void) {
    uint32_t PageError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;

    // 2. 解锁 Flash
    HAL_FLASH_Unlock();

    // 3. 擦除这一页
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = PARAM_FLASH_ADDR;
    EraseInitStruct.NbPages = 1;
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
        HAL_FLASH_Lock();
        return 0;
    }

    // 4. 按字（32位）写入数据
    uint32_t *pData = (uint32_t *)&current_param;
    uint32_t data_len = sizeof(Param_t) / 4; // 计算有多少个 32 位字

    for (uint32_t i = 0; i < data_len; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, PARAM_FLASH_ADDR + (i * 4), pData[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return 0;
        }
    }

    // 5. 锁定 Flash
    HAL_FLASH_Lock();
    return 1;
}

// 保存参数到 Flash（成功返回 1，失败返回 0）
uint8_t Param_Save(void) {
    // 1. 更新当前参数副本（从 Module 层拿最新状态）
    current_param.mode = Module_LED_GetMode();
    current_param.period = Module_LED_GetPeriod();
    current_param.default_led = Module_LED_GetState(); // 把当前开关状态也存下来

    return Param_FlashWrite();
}

// 恢复出厂设置（成功返回 1，失败返回 0）
uint8_t Param_Reset(void) {
    current_param.magic = PARAM_MAGIC;
    current_param.mode = MODE_MANUAL;
    current_param.period = 500;
    current_param.default_led = 0;

    Module_LED_SetPeriod(current_param.period);
    Module_LED_SetState(current_param.default_led);
    Module_LED_SetMode(current_param.mode);

    // ★ 恢复出厂的同时写入 Flash：断电重启后仍是默认值（任务书 RESET 验收要求）
    return Param_FlashWrite();
}
