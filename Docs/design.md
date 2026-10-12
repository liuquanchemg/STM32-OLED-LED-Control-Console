# 分层设计说明

## 总体架构

本项目采用六层架构，从底到顶依次为：
Task / App
↓
Module
↓
Component / Device
↓
BSP
↓
HAL

text

**依赖方向**：上层可以调用下层，下层不允许反向调用。

## 各层职责

### BSP 层（板级支持包）

- **文件**：`bsp_led.c/.h`、`bsp_key.c/.h`
- **职责**：封装板级硬件资源，屏蔽 GPIO 有效电平。
- **代表函数**：
  - `BSP_LED_On()` / `BSP_LED_Off()` / `BSP_LED_Toggle()`
  - `BSP_Key_Read(key_id)` / `BSP_Key_Init()`
- **不允许做的事**：不知道菜单、模式、参数等业务概念。

### Device 层（设备驱动）

- **文件**：`dev_uart.c/.h`、`oled.c/.h`、`font.c/.h`
- **职责**：
  - `dev_uart`：环形缓冲区 + 串口中断回调，只负责存字节。
  - `oled`：I2C 通信、显存操作、字符/图形绘制。
- **代表函数**：
  - `UART_Init()` / `UART_IsDataAvailable()` / `UART_ReadByte()`
  - `OLED_Init()` / `OLED_NewFrame()` / `OLED_PrintString()` / `OLED_ShowFrame()`

### Component 层（通用组件）

- **文件**：`comp_key.c/.h`
- **职责**：按键消抖、短按、长按、连续按键的状态机。
- **代表函数**：`Key_Scan_StateMachine(key_id)`
- **返回事件**：`KEY_EVENT_KEY1_SHORT`、`KEY_EVENT_LONG_PRESS`、`KEY_EVENT_REPEAT` 等。

### Module 层（业务逻辑）

- **文件**：`mod_led.c/.h`、`mod_param.c/.h`、`mod_menu.c/.h`
- **职责**：
  - `mod_led`：管理 LED 状态、工作模式、周期、命令计数、最近错误。
  - `mod_param`：Flash 参数保存与加载。
  - `mod_menu`：菜单页面状态机。
- **代表函数**：`Module_LED_SetMode()`、`Module_LED_Process()`、`Param_Save()`、`Menu_Process()`。

### Task 层（任务调度）

- **文件**：`task_uart.c/.h`
- **职责**：串口按行接收、命令解析、错误返回、命令计数。
- **代表函数**：`Task_UART_Process()`

### App 层（应用入口）

- **文件**：`app.c/.h`
- **职责**：
  - `App_Init()`：依次初始化 UART、按键、LED、菜单、Flash 参数。
  - `App_Loop()`：主循环调度，包括命令解析、按键扫描、LED 业务、OLED 刷新。
- **`main.c`** 只保留 `App_Init()` 和 `App_Loop()` 调用。

## 数据流

### 串口命令控制 LED
[电脑] → CH340 → USART2 中断
↓
dev_uart.c: HAL_UART_RxCpltCallback（存字节到环形缓冲区）
↓
task_uart.c: Task_UART_Process（拼字符串、匹配命令）
↓
mod_led.c: Module_LED_SetState（改私有变量）
↓
app.c: App_Loop → Module_LED_Process（按账本驱动硬件）
↓
bsp_led.c: BSP_LED_On / BSP_LED_Off（操作 GPIO）

text

### 按键控制 LED
[按键按下] → GPIO 电平变化
↓
comp_key.c: Key_Scan_StateMachine（消抖 + 事件判定）
↓
mod_menu.c: Menu_Process（根据页面分发事件）
↓
mod_led.c: Module_LED_Toggle（改私有变量）
↓
app.c: App_Loop → Module_LED_Process
↓
bsp_led.c: BSP_LED_Toggle（操作 GPIO）

text

### 参数保存与加载
[串口发 SAVE] → Task_UART_Process → Param_Save
↓
mod_param.c: 解锁 Flash → 擦除页 → 按 32 位字写入 → 锁定 Flash
↓
[断电重启] → App_Init → Param_Init
↓
读 Flash 魔术字：若 0xA5A5A5A5 则加载，否则用默认值
↓
Module_LED_SetMode / SetPeriod / SetState

text

## 关键设计决策

| 决策                       | 原因                                     |
| -------------------------- | ---------------------------------------- |
| 中断只存字节，不解析命令   | 中断要快进快出，避免阻塞其他中断         |
| 环形缓冲区                 | 解耦生产者（中断）和消费者（主循环）     |
| 状态机消抖                 | 避免 `HAL_Delay` 阻塞主循环              |
| `HAL_GetTick()` 非阻塞刷新 | 保证按键和串口响应不被 OLED 拖慢         |
| Flash 参数页独立           | `IROM1` Size = `0xF800`，保护最后一页    |
| 分层架构                   | 修改一层不影响其他层，便于维护和答辩说明 |

## 分层合理性对照

对照考核书第 4 章的禁止项：

- ✅ 业务代码未直接操作寄存器
- ✅ OLED 驱动未直接调用菜单业务
- ✅ BSP 层无菜单、参数逻辑
- ✅ `main.c` 只有 `App_Init` 和 `App_Loop`
- ✅ 无单文件包含全部功能
- ✅ 中断回调未执行复杂协议解析或刷屏
- ✅ 无大量阻塞式 `HAL_Delay` 代替状态机