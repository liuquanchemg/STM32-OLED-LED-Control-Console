#include "dev_uart.h"

#define UART_BUFFER_SIZE 128

// 准备环形缓冲区
typedef struct {
    uint8_t buffer[UART_BUFFER_SIZE];  // ★ 修正了 bufffer
    volatile uint16_t head;            // ★ 改为 uint16_t 且加 volatile
    volatile uint16_t tail;
} RingBuffer_t;

RingBuffer_t rx_buf = {0};
uint8_t rx_byte;

// 初始化串口接收
void UART_Init(void) {
    rx_buf.head = 0;
    rx_buf.tail = 0;
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

// 检查有没有新数据
uint8_t UART_IsDataAvailable(void) {
    return (rx_buf.head != rx_buf.tail);
}

// 拿走数据
uint8_t UART_ReadByte(void) {
    if (rx_buf.head == rx_buf.tail) {
        return 0; // 没有数据
    }
    uint8_t data = rx_buf.buffer[rx_buf.tail];
    rx_buf.tail = (rx_buf.tail + 1) % UART_BUFFER_SIZE;
    return data;
}

// 串口中断回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) { // ★ 修正了 husrt
        uint16_t next_head = (rx_buf.head + 1) % UART_BUFFER_SIZE;
        if (next_head != rx_buf.tail) {
            rx_buf.buffer[rx_buf.head] = rx_byte;
            rx_buf.head = next_head;
        }
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}

// 串口错误回调
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        __HAL_UART_CLEAR_OREFLAG(huart);
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}
