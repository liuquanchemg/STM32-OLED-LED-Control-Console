# 现场演示检查表（demo-checklist）

> 使用说明：答辩/验收前按顺序逐项实测，`实际结果` 一栏填写 **PASS / FAIL**，并在演示前把所有 FAIL 项清零。
> 硬件环境：STM32F103C8T6 + DAPLink + CH340(USART2) + I2C OLED(0x78) + 按键(PB13/14/15) + LED(PA6 低电平点亮)

## 一、上电与自检（基础题 6.1）

| # | 操作 | 预期现象 | 实际结果 |
| - | ---- | -------- | -------- |
| 1 | DAPLink 下载程序后复位 | OLED 显示启动页（项目名+版本+SELF TEST） |  |
| 2 | 观察 LED | 亮-灭-亮-灭 两轮自检后熄灭 |  |
| 3 | 打开串口助手 115200-8-N-1 | 收到 `STM32 Console Boot OK` |  |
| 4 | 等待约 1 秒 | OLED 进入主菜单（顶部状态栏 + 4 个选项） |  |

## 二、OLED 主界面（基础题 6.2）

| # | 操作 | 预期现象 | 实际结果 |
| - | ---- | -------- | -------- |
| 5 | 观察主菜单 | 顶部显示 `M:MANUAL L:OFF C:0000`，下方 4 个菜单项全部可见（LED Control / Blink Param / System Info / UART Help） |  |
| 6 | 连续观察 30 秒 | 画面无明显闪烁 |  |

## 三、按键功能（基础题 6.3 + 提高题 7.2）

| # | 操作 | 预期现象 | 实际结果 |
| - | ---- | -------- | -------- |
| 7 | 短按 KEY1 两次 | 光标在主菜单下移两格（4 项循环） |  |
| 8 | 进入 LED Control 页后短按 KEY2 | 板载 LED 翻转，页面 `STATE: ON/OFF` 同步变化 |  |
| 9 | 短按 KEY3 | 返回主菜单 |  |
| 10 | 进入 Blink Param 页，短按 KEY2 | 周期在 500→800→1000ms 循环，页面周期数值同步 |  |
| 11 | 主菜单长按 KEY1（约 1 秒） | LED 模式切换 MANUAL→BLINK→FAST_BLINK，顶部状态栏 `M:` 变化；快速闪烁模式 LED 明显快闪 |  |
| 12 | 主菜单按住 KEY1 不放（长按后） | 光标每 200ms 持续下移（连续按键） |  |
| 13 | Blink Param 页按住 KEY1 不放 | 周期每 200ms +100ms，上限 5000ms |  |

## 四、基础串口命令（基础题 6.4）

| # | 发送命令 | 预期现象 | 实际结果 |
| - | -------- | -------- | -------- |
| 14 | `LED ON` | LED 亮，OLED 状态变 `L:ON`，串口回 `OK LED=ON` |  |
| 15 | `LED TOGGLE` | LED 灭，OLED 状态变 `L:OFF`，串口回 `OK LED=OFF` |  |
| 16 | `LED TOGGLE`（再次） | LED 亮，串口回 `OK LED=ON` |  |
| 17 | `STATUS` | 串口返回 `MODE=... LED=... COUNT=... RUN=...`，与 OLED/灯一致 |  |
| 18 | `HELP` | 串口返回完整命令列表 |  |

## 五、提高题串口命令（提高题 7.4）

| # | 发送命令 | 预期现象 | 实际结果 |
| - | -------- | -------- | -------- |
| 19 | `SET MODE 1` | 串口回 `OK MODE=BLINK`，LED 按周期闪烁，OLED 状态栏 `M:BLINK` |  |
| 20 | `SET PERIOD 1000` | 串口回 `OK PERIOD=1000ms`，**LED 闪烁明显变慢**（周期参数真实生效） |  |
| 21 | `SET PERIOD 200` | 串口回 `OK PERIOD=200ms`，LED 闪烁明显变快 |  |
| 22 | `SET MODE 0` | 串口回 `OK MODE=MANUAL`，LED 停止自动闪烁 |  |
| 23 | `SET LED ON` / `SET LED OFF` | 串口回 `OK LED=ON/OFF`，LED 与 OLED 状态同步 |  |
| 24 | `GET PARAM` | 串口返回 `MODE=... PERIOD=...ms`，与 OLED 参数页一致 |  |
| 25 | `GET STATUS` | 串口返回 `MODE/LED/COUNT/RUN/KEY/OK/ERR` 完整状态 |  |

## 六、参数保存与恢复（提高题 7.5）

| # | 操作 | 预期现象 | 实际结果 |
| - | ---- | -------- | -------- |
| 26 | `SET MODE 1` + `SET PERIOD 800` + `SAVE` | 串口回 `OK SAVE`，LED 保持闪烁 |  |
| 27 | 断电后重新上电 | OLED 启动后 LED 直接以闪烁模式运行，周期 800ms，状态栏 `M:BLINK` |  |
| 28 | `RESET` | 串口回 `OK RESET`，LED 立即手动熄灭 |  |
| 29 | 断电后重新上电 | 恢复默认（手动模式/500ms/灯灭）——RESET 已持久化 |  |

## 七、错误命令与容错（6.4.3 + 提高题可靠性）

| # | 发送命令 | 预期现象 | 实际结果 |
| - | -------- | -------- | -------- |
| 30 | `ABC` | 串口回 `ERROR: Unknown Command`，LED/OLED 状态不变 |  |
| 31 | `SET MODE 9` | 串口回 `ERROR: MODE RANGE`，当前闪烁继续 |  |
| 32 | `SET MODE 10` | 串口回 `ERROR: MODE FORMAT`（拒绝误判为 MODE 1） |  |
| 33 | `SET PERIOD 20` | 串口回 `ERROR: PERIOD RANGE`，周期不变 |  |
| 34 | `SET PERIOD 99999` | 串口回 `ERROR: PERIOD RANGE`，系统继续工作 |  |
| 35 | `SET PERIOD 500abc` | 串口回 `ERROR: PERIOD FORMAT`（拒绝 atoi 误收） |  |
| 36 | 发送超过 63 字符的命令 | 串口回 `ERROR: COMMAND TOO LONG`，丢弃本行，下一条正常 |  |
| 37 | ` LED ON`（前面带空格） | 正常执行 `OK LED=ON`（已做首尾空格处理） |  |
| 38 | 空行（直接回车） | 无返回，系统不受影响 |  |
| 39 | 快速连续发送 `LED ON/LED OFF/LED TOGGLE` | 每一条都有正确返回，无粘连、无死机 |  |

## 八、OLED 异常降级（提高题验收最后一项）

| # | 操作 | 预期现象 | 实际结果 |
| - | ---- | -------- | -------- |
| 40 | 运行时拔掉 OLED 的 I2C 线 | **程序不死机**：按键仍能翻转 LED，串口命令仍能正常执行 |  |
| 41 | 重新插上 OLED 线 | OLED 恢复刷新显示 |  |

## 九、Keil / Git / 文档

| # | 检查项 | 预期 | 实际结果 |
| - | ------ | ---- | -------- |
| 42 | Keil Rebuild | 0 Error 0 Warning |  |
| 43 | Options for Target → IROM1 Size | `0xF800`（不覆盖 Flash 参数区 0x0800FC00） |  |
| 44 | Git 仓库 | 有 `.git`，提交记录按功能分次提交 |  |
| 45 | README / Docs | README、wiring、design、test-report、本检查表齐全且与代码一致 |  |

## 十、答辩话术要点（对照验收清单）

1. **分层关系**：App/Task → Module → Component/Device → BSP → HAL，依赖只向下，业务不碰寄存器。
2. **中断职责**：USART2 中断只收字节进环形缓冲，命令解析在主循环（Task 层）。
3. **按键状态机**：IDLE→DEBOUNCE→PRESSED→RELEASE，20ms 消抖，短按/长按(1s)/连续(200ms) 三态事件。
4. **非阻塞刷新**：OLED 200ms 限流整帧刷新；LED 闪烁用 HAL_GetTick 时间片，无 HAL_Delay。
5. **参数持久化**：Flash 最后一页 + magic 校验 + 范围校验，只在 SAVE/RESET 时写入（防擦写寿命损耗）。
6. **命令可靠性**：按行接收、64 字节上限、超长丢弃、首尾空格、范围/格式双重校验、错误计数。

> 全部 PASS 后，在 test-report.md 中按"命令/预期/实际/结论"补录实测记录。
