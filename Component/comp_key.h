#ifndef __COMP_KEY_H
#define __COMP_KEY_H

#include "main.h"
#include <stdint.h>

// 定义按键事件枚举
typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_KEY1_SHORT,  // KEY1 短按
    KEY_EVENT_KEY2_SHORT,  // KEY2 短按
    KEY_EVENT_KEY3_SHORT,  // KEY3 短按
    KEY_EVENT_LONG_PRESS,    //长按
    KEY_EVENT_REPEAT
} KeyEvent_t;

void Key_Component_Init(void);
KeyEvent_t Key_Scan_StateMachine(uint8_t key_id);
uint32_t Key_GetPressCount(void);

#endif
