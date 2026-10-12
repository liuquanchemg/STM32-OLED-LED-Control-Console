# STM32 OLED LED Control Console

基于 STM32F103C8T6 的 OLED 嵌入式控制终端，通过按键和串口命令控制 LED 工作模式，支持多级菜单、参数保存和系统状态显示。

## 1. 项目概述

本项目使用 `STM32CubeMX + HAL + Keil MDK + VSCode + Git` 开发，实现以下闭环：
按键 / CH340 串口输入
↓
BSP 接收与硬件封装
↓
Device / Component 解析与通用处理
↓
Module 修改 LED、菜单和参数状态
↓
Task 周期调度
↓
OLED / 板载 LED 输出

**核心功能：**

- OLED 多级菜单（首页 / LED 控制 / 闪烁参数 / 系统信息 / 串口命令说明）
- 按键消抖 + 短按 + 长按 + 连续按键
- LED 三种工作模式（手动 / 固定闪烁 / 快速闪烁）
- 串口按行接收命令，支持 `SET` / `GET` 系列命令，命令格式严格校验
- 参数保存到 Flash，断电重启后自动恢复；`SAVE` 与 `RESET` 均持久化
- 系统状态信息显示（版本、运行时间、模式、LED 状态、按键计数、命令计数、最近错误、OLED 通信状态）

## 2. 硬件接线

### 2.1 DAPLink 下载和调试

| DAPLink | STM32F103C8T6 |
| ------- | ------------- |
| SWDIO   | PA13          |
| SWCLK   | PA14          |
| GND     | GND           |
| 3.3V    | 3.3V          |

- DAPLink 用于下载、调试和供电。
- 下载前确认 SWD 引脚连接正确，PA13/PA14 不得改作普通 GPIO。

### 2.2 CH340 串口

| CH340 | STM32F103C8T6   |
| ----- | --------------- |
| TXD   | PA3 (USART2_RX) |
| RXD   | PA2 (USART2_TX) |
| GND   | GND             |

- 串口参数：**115200-8-N-1**（波特率 115200，8 数据位，1 停止位，无校验）
- CH340 的 **VCC 不接**（由 DAPLink 给板子供电），避免电源倒灌。

### 2.3 OLED I2C

| OLED I2C | STM32F103C8T6 |
| -------- | ------------- |
| VCC      | 3.3V          |
| GND      | GND           |
| SCL      | PB6           |
| SDA      | PB7           |

- OLED 设备地址：`0x78`（部分 1.3 寸模块为 `0x7A`，需修改 `oled.c` 中的 `OLED_ADDRESS`）

### 2.4 按键

| 按键 | STM32F103C8T6 | 功能                             |
| ---- | ------------- | -------------------------------- |
| KEY1 | PB13          | 光标移动 / 切换 LED 模式（长按） |
| KEY2 | PB14          | 确认 / 翻转 LED / 调整参数       |
| KEY3 | PB15          | 返回 / 退出                      |

- 按键另一端接 GND，GPIO 配置为**上拉输入**，按下为低电平。

### 2.5 板载 LED

| 外接 LED | STM32F103C8T6        |
| -------- | -------------------- |
| 负极     | PA6                  |
| 正极     | 3.3V（通过限流电阻） |

- **低电平点亮**：`GPIO_PIN_RESET` 点亮，`GPIO_PIN_SET` 熄灭。
- 有效电平统一封装在 `BSP/bsp_led.c`。

## 3. CubeMX 主要配置

| 外设   | 配置                                              |
| ------ | ------------------------------------------------- |
| 时钟   | HSE 外部晶振，PLL ×9，主频 72MHz                  |
| SWD    | PA13 / PA14 保持为 Serial Wire Debug              |
| USART2 | Asynchronous，115200-8-N-1，开启 global interrupt |
| I2C1   | I2C 模式，400kHz，PB6 / PB7                       |
| GPIO   | PA6 输出（LED），PB13/PB14/PB15 上拉输入（按键）  |

## 4. 工程结构
Project/
├── Core/ # CubeMX 生成的 HAL 底层文件
├── BSP/ # 板级资源封装
│ ├── bsp_led.c/.h # LED 硬件封装
│ └── bsp_key.c/.h # 按键硬件封装
├── Device/ # 设备驱动层
│ ├── oled.c/.h # OLED 驱动（含通信超时降级）
│ ├── font.c/.h # 字库
│ └── dev_uart.c/.h # 串口环形缓冲区
├── Component/ # 通用组件层
│ └── comp_key.c/.h # 按键状态机
├── Module/ # 业务逻辑层
│ ├── mod_led.c/.h # LED 状态管理
│ ├── mod_param.c/.h # Flash 参数保存
│ └── mod_menu.c/.h # 菜单状态机
├── Task/ # 任务调度层
│ └── task_uart.c/.h # 串口命令解析
├── App/ # 应用层
│ └── app.c/.h # 初始化与主循环调度
├── Docs/ # 文档
└── README.md

## **依赖方向**：
Task / App → Module → Component / Device → BSP → HAL

## 5. 串口命令说明

串口参数：`115200`、8 数据位、1 停止位、无校验。**每条命令必须以 `\r\n`（回车换行）结束**，串口助手请勾选"发送新行"；命令前/后的多余空格会被自动裁剪，但命令中间的空格不能省。

### 5.1 基础命令

| 命令         | 作用             | 返回                                        |
| ------------ | ---------------- | ------------------------------------------- |
| `HELP`       | 列出所有支持命令 | 命令列表                                    |
| `LED ON`     | 点亮 LED         | `OK LED=ON`                                 |
| `LED OFF`    | 熄灭 LED         | `OK LED=OFF`                                |
| `LED TOGGLE` | 翻转 LED         | `OK LED=ON/OFF`                             |
| `STATUS`     | 查询当前状态     | `MODE=... LED=... COUNT=... RUN=...`        |

### 5.2 提高题命令

模式编号：`0=手动`、`1=固定闪烁`、`2=快速闪烁`

| 命令              | 作用                       | 返回                                                  |
| ----------------- | -------------------------- | ----------------------------------------------------- |
| `GET STATUS`      | 查询完整状态               | `MODE=... LED=... COUNT=... RUN=... KEY=... OK=... ERR=...` |
| `GET PARAM`       | 查询可配置参数             | `MODE=... PERIOD=...ms`                               |
| `SET MODE 0/1/2`  | 切换工作模式               | `OK MODE=...`（模式默认周期：1→500ms，2→200ms）       |
| `SET PERIOD <ms>` | 修改闪烁周期（100~5000ms） | `OK PERIOD=...ms`（对当前模式立即生效）               |
| `SET LED ON/OFF`  | 设置 LED 状态              | `OK LED=ON/OFF`                                       |
| `SAVE`            | 保存参数到 Flash           | `OK SAVE` / `ERROR: SAVE FAIL`                        |
| `RESET`           | 恢复出厂设置（并写入 Flash）| `OK RESET` / `ERROR: RESET FAIL`                      |

> 说明：`SET MODE 1/2` 会先把周期设为该模式的默认值（500ms / 200ms），之后可用 `SET PERIOD` 覆盖，实现"模式档位 + 用户细调"。

### 5.3 错误命令

| 输入                     | 返回                      |
| ------------------------ | ------------------------- |
| `ABC`                    | `ERROR: Unknown Command`  |
| `SET MODE 9`             | `ERROR: MODE RANGE`       |
| `SET MODE 10`            | `ERROR: MODE FORMAT`（拒绝多余字符） |
| `SET PERIOD 20`          | `ERROR: PERIOD RANGE`     |
| `SET PERIOD 500abc`      | `ERROR: PERIOD FORMAT`（拒绝尾部字母） |
| 超过 63 字符的命令       | `ERROR: COMMAND TOO LONG` |

## 6. Keil 编译与下载

1. 打开 `MDK-ARM/oled UART led.uvprojx`。
2. 点击 **Rebuild**（双箭头）。
3. 确认 **0 Error 0 Warning**。
4. 连接 DAPLink，点击 **Download**（F8）。
5. 复位板子，观察 OLED 上电自检。

**⚠️ 关键配置**：

- **Options for Target → Target → IROM1 Size** 必须为 `0xF800`（不能是 `0x10000`，否则会覆盖 Flash 参数区）。
- 编译器字符集需设置为 **UTF-8**（OLED 中文/ASCII 显示依赖字库匹配）。

## 7. Git 提交说明

主要提交记录（按时间顺序）：

- `init: 建立工程, 板载LED+按键测试通过` — 初始工程搭建
- `feat: oled显示某点` / `feat: oled显示增加API实现功能实现` — OLED 最小显示
- `feat: 串口发送接收信号控制LED` / `feat: 串口环形接收` — 串口收发与环形缓冲区
- `feat: 按键消抖状态机` — 按键状态机
- `refactor: move LED control to BSP layer, UART command parser working` — BSP 层重构
- `refactor: add BSP key layer, complete Device UART migration and verify` — Device 层重构
- `refactor: move key scanner to Component layer and UART parser to Task layer` — Component/Task 层重构
- `refactor: complete Module and App layers, eliminate extern, separate concerns` — Module/App 层重构
- `feat: add flash param save/load, support SAVE/RESET, fix IROM1 size to 0xF800` — Flash 参数保存
- `feat: add menu framework with main menu cursor navigation` — OLED 多级菜单
- `fix: resolve OLED single-line display issue` — 修复 OLED 显示问题
- `fix: complete all basic and advanced functions, fix key state machine nested bug` — 功能联调
- `feat: add LED fast flash mode` — LED 三种模式
- `feat: add key press counter` — 按键计数
- `feat: add key repeat event` — 连续按键
- `feat: add SET LED ON/OFF commands` — 提高题命令
- `fix: command too long protection` — 超长命令保护
- `feat: display LED state in sys info page` — 系统信息页补全
- `fix: LED blink period now controlled by flash_period` — 周期参数生效（SET PERIOD 真正控制闪烁）
- `fix: RESET persists to Flash` — RESET 持久化
- `fix: strict command parsing (MODE FORMAT / PERIOD FORMAT)` — 命令严格校验
- `fix: OLED I2C timeout degradation` — OLED 通信异常降级
- `docs: complete demo-checklist and test-report` — 文档补全

## 8. 已知问题和改进方向

- **OLED 通信异常降级**：已实现 I2C 发送 50ms 超时 + 错误标志，OLED 断开时按键、LED、串口不受影响，系统信息页显示 `OLED:BAD`，重新接上自动恢复 `OLED:OK`。
- **两模式共用周期**：快速闪烁与固定闪烁共用同一周期变量，用户 `SET PERIOD` 后两模式节拍一致（模式作为预置档位，周期可细调）。
- **流水灯**：多个外接 LED 的流水灯效果可作为后续加分项扩展。

## 9. 作者

- 姓名：刘泉成
- 学号：2520011209
- 日期：2026-10-10
