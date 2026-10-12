#include "comp_key.h"
#include "bsp_key.h"

//定义枚举四种状态
typedef enum{
    KEY_STATE_IDLE = 0,
    KEY_STATE_DEBOUNCE,
    KEY_STATE_PRESSED,
    KEY_STATE_RELEASE
}KeyState_t;

#define REPEAT_TIME 200 //长按200ms触发
#define DEBOUNCE_TIME 20    //消抖时间
#define LONG_PRESS_TIME 1000 //长按时间

// ★ 每个按键独立的状态和时间
static KeyState_t keyState[3] = {KEY_STATE_IDLE, KEY_STATE_IDLE, KEY_STATE_IDLE};
static uint32_t keyTimer[3] = {0, 0, 0};
static uint32_t pressStartTime[3] = {0,0,0};
static uint8_t LongPressHandled[3] = {0,0,0}; // 标记每个按键的长按是否已处理
static uint32_t key_press_count = 0;    //按键事件计数

void Key_Component_Init(void) {
    BSP_Key_Init(); // 调用 BSP 层的硬件初始化
}

KeyEvent_t Key_Scan_StateMachine(uint8_t key_id) {
    uint8_t pinState = BSP_Key_Read(key_id); // 调用BSP层的按键读取函数
    
    switch (keyState[key_id]){

        //初始状态检测进入按下
        case  KEY_STATE_IDLE:
        if(pinState == 0){
            keyState[key_id] = KEY_STATE_DEBOUNCE;
            keyTimer[key_id] = HAL_GetTick();
        }
        break;

        //按下延时
        case  KEY_STATE_DEBOUNCE:
        if(HAL_GetTick() -keyTimer[key_id] >=DEBOUNCE_TIME){
            if(pinState == 0){
                keyState[key_id] = KEY_STATE_PRESSED;
                pressStartTime[key_id] = HAL_GetTick(); // 记录按下的时间
                LongPressHandled[key_id] = 0; // 重置长按处理标记
                
                key_press_count++;
                if(key_id == KEY1) return KEY_EVENT_KEY1_SHORT; //返回一个按键事件
                if(key_id == KEY2) return KEY_EVENT_KEY2_SHORT; //返回一个按键事件
                if(key_id == KEY3) return KEY_EVENT_KEY3_SHORT; //返回一个按键事件
            } else {
                keyState[key_id] = KEY_STATE_IDLE;
            }
        }
        break;

        //检测按键是否释放检测长按，进入延时
        case KEY_STATE_PRESSED:
            // ★ 先判断松开（无论有没有长按，松开都要检测）
            if (pinState == 1) {
                keyState[key_id] = KEY_STATE_RELEASE;
                keyTimer[key_id] = HAL_GetTick();
            }
            // ★ 再判断长按（只有一直按住才检测）
            else if (!LongPressHandled[key_id]) {
                if (HAL_GetTick() - pressStartTime[key_id] >= LONG_PRESS_TIME) {
                    LongPressHandled[key_id] = 1;
                    pressStartTime[key_id] = HAL_GetTick();
                    key_press_count++;
                    return KEY_EVENT_LONG_PRESS;
                }
            }
            // ★ 长按之后，每 200ms 触发一次重复
            else {
                if (HAL_GetTick() - pressStartTime[key_id] >= REPEAT_TIME) {
                    pressStartTime[key_id] = HAL_GetTick();
                    key_press_count++;
                    return KEY_EVENT_REPEAT;
                }
            }
            break;


        //按键释放延时
        case KEY_STATE_RELEASE:
        if(HAL_GetTick() - keyTimer[key_id] >= DEBOUNCE_TIME){
                if(pinState == 1){
                    keyState[key_id] = KEY_STATE_IDLE;
                } else {
                    keyState[key_id] = KEY_STATE_PRESSED;
                }
            }
            break;  
    }
    return KEY_EVENT_NONE; //没有按键事件
}
uint32_t Key_GetPressCount(void) {
    return key_press_count;
}
