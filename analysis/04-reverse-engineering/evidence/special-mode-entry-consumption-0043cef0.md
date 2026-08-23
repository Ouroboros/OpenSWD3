# 标准模式entry消费 `0x0043CEF0`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与边界

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043CEF0..0x0043D04E`，154行。七个caller为`0x0043C0D0`、`0x0043C3C0`、`0x0043C520`、`0x0043C590`、`0x0043C670`、`0x0043C760`及尚未关闭的`0x00446420`。本工作包回接前六个已关闭caller；`0x00446420`仍按其独立workpack审计，不提前计数。

四个直接callee中，record loader、record release和失败后的空通知属于已有资源owner；尾部`0x0043D050`现已作为下一独立模块9单元关闭。新typed helper保留完整CEF0控制流，port只留下精确的完整u32 selected-record loader；旧`consume_entry`和synthetic D050 dispatch高层占位均已移除。

## 2. 无条件release与零entry

入口读取scratch `+0xAC` u32 token并无条件调用release，包括token为0。随后以44个dword精确清零整个`0xB0` scratch，并把两个derived offset owner清零。此顺序发生在entry判零之前。

entry为0时直接沿`xor eax,eax`返回0：不写header、不调用loader、不调用D050。C0D0因其首entry为0，真实结果从旧占位伪造的port EAX改为0，并多记录一次真实`release(0)`。C3C0的零entry路径也不再伪造entry-consume事件，但caller在CEF0返回后仍按原顺序播放sample。

## 3. 非零entry加载

非零entry按LST写scratch header：

- `+0x04 = low_u16(entry)`。
- `+0x08 = 1`。
- `+0x0A = 0`。
- `+0x06 = 0`。

loader实参仍是调用者传入的完整u32 entry，而不是header里的low16；这修正了旧port把契约窄化为u16的风险。destination为scratch `+0x0C`。loader失败后的原空通知无可观察副作用，控制流照常继续，不typed-stop、不跳过D050。

loader返回后两个offset owner再次清零，然后读取scratch `+0x60`的u16 base及七个u16开关。

## 4. derived offsets

第二offset对应原`dword_4FC92C`，累加寄存器初值0：

- `+0x72 != 0`：设为`2*base`并发布。
- `+0x76 != 0`：累加`3*base`并发布。
- `+0x7A != 0`：累加`5*base`并发布。
- `+0x86 != 0`：累加`2*base`并发布。
- `+0x8A != 0`：发布当前累计值再加`4*base`。

第一offset对应原`dword_4FC924`，独立base寄存器初值0：

- `+0x7E != 0`：设为`3*base`并发布。
- `+0x82 != 0`：发布该寄存器再加`5*base`；因此单独启用时为`5*base`，与`+0x7E`同时启用时为`8*base`。

全部开关、base=7时，第一offset=56、第二offset=112。

## 5. D050实参与EAX

非零entry最后按u32回绕计算`window_offset + local_cursor`，以i32直接调用已关闭D050，并传播其status、文本owner指针/release EAX联合。`INT_MAX + 1`覆盖为`INT_MIN`，没有宿主有符号溢出。

结果同时记录release次数、loader是否尝试/成功以及D050是否调用，便于测试控制流；这些诊断字段不引入额外行为。

## 6. caller回接与验证

C0D0、C520、C590、C670、C760和C3C0不再调用synthetic `consume_entry`。它们直接调用本helper，并保留CEF0后的mode flag和sample顺序。C0D0将本helper的release计入其typed诊断计数。

定向UT覆盖：

- 非零旧token与零entry的无条件release、全scratch清零、owner清零、EAX0及无loader/D050。
- entry `0xA1B2C3D4`完整传给loader，而header写low16 `0xC3D4`。
- 七个开关、base7、第一/第二offset 56/112。
- `INT_MAX+1`的D050索引回绕及D050 EAX `-777`。
- loader失败仍以零offset调用D050。
- C0D0额外`release(0)`、C3C0零entry不产生loader事件但仍播放sample。

`special_modes.legacy_initial_menu`定向测试通过。workpack连续两轮稳定为`40/227`，SHA256均为`5d0095ca98c73d02f6cf8296e51d854713f0511e34801b9832cf5911a8fb46fd`；下一独立单元为`0x0043D050`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
