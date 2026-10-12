#include "bsp_led.h"

void BSP_LED_Init(void) {
    // CubeMX �Ѿ��� MX_GPIO_Init ���ʼ���ˣ�������ż���
}

void BSP_LED_On(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); // ����͵�ƽ����
}

void BSP_LED_Off(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
}

void BSP_LED_Toggle(void) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
}
