#include "bsp_key.h"

void BSP_Key_Init(void)
{
    //CubeMx配置
}
uint8_t BSP_Key_Read(uint8_t key_id)
{
    if(key_id == KEY1) return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13);
    if(key_id == KEY2) return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
    if(key_id == KEY3) return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15);
    return 1;
}
