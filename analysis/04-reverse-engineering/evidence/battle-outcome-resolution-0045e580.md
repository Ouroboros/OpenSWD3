# 战斗结果判定前置流程 `0x0045E580`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围与调用图

权威函数为`0x0045E580..0x0045E651`，从proc到endp完整93行、51条实际指令、5个call站点、9个分支和4个局部标签，无外部FUNCTION CHUNK。

唯一caller是逐帧画面协调器`0x00453200`。五个call站点为：

- 两次已关闭全帧暗化`0x0045BD10`，现直接组合typed实现；
- 一次音频stream暂停，保留窄端口且忽略返回；
- 两次尚未审计的结果整理`0x0045E9C0`，保留同一窄端口；其工作包为`audit_order=96`。

## 2. 组A剩余量与无符号门

入口严格按以下读取顺序取得snapshot：

1. 组A动态数量完整dword；
2. 已结算值低word；
3. 打包进度高word；
4. 已完成数量低byte。

剩余量按低32位执行：

```text
remaining = group_a_count - packed_high_word - resolved_low_word
```

随后将零扩展完成byte与remaining作unsigned比较。只有`completed >= remaining`才进入组A完成路径。减法下溢不夹零；例如`0 - 0 - 1`得到`0xFFFFFFFF`，完成byte 255仍不足。

门成立后先读取完整暗化门，再把结果latch写1，最后比较先前门snapshot是否精确等于1。门不等于1时不暗化，但继续执行组B判定。

## 3. 组A暗化、音频与模式

暗化门等于1时直接组合已关闭全帧暗化：

- channel delta、三个颜色偏移和唯一owned framebuffer沿用原typed owner；
- 返回不精确等于1时不暂停音频、不调用结果整理，并继续组B判定；
- framebuffer typed-stop保留结果latch和三个颜色偏移发布，阻断音频、结果整理与组B判定。

暗化精确返回1时依次：

1. 暂停音频stream；
2. 调用结果整理，忽略返回；
3. callee后读取唯一共享message state；
4. 逐帧active值写2；
5. message state等于104时active清0；
6. 再读取战斗mode flags，低byte bit3置位时active清0。

message读取发生在模式写2之前。结果整理可改写后续组B的packed进度、数量、暗化门和channel delta；组B阶段必须重新读取，不缓存入口值。

## 4. 组B signed差值门

组A路径完成或跳过后，严格按以下顺序取得组Bsnapshot：

1. 同一packed dword一次性读取；
2. 组B动态数量完整dword。

差值按低32位执行：

```text
difference = zero_extend(low_byte) - zero_extend(byte_2)
```

比较把difference和group B数量都解释为signed i32。满足`signed(difference) >= signed(group_b_count)`即进入组B路径；否则只有强制值精确等于1才进入。强制值2等异常值不生效。低byte小于byte2时的回绕负值按signed比较，不按unsigned巨大值处理。

未进入组B路径时返回组B数量完整dword。

## 5. 组B暗化与返回

组B路径同样先读取暗化门，再写结果latch 1，最后比较先前门snapshot：

- 门不等于1时返回该门完整dword；
- 暗化typed-stop保留latch和暗化前缀并停止；
- 暗化正常但返回不等于1时，直接返回暗化EAX；
- 暗化精确返回1时，只调用结果整理，不暂停音频；随后逐帧active清0，并返回结果整理完整EAX。

组A和组B可在同一函数调用中分别暗化。若组A暗化只返回0，组B门仍成立，则channel delta继续由第一次减后值推进第二次。若组A结果整理改写组B输入，第二阶段使用callee后的新值。

## 6. 单一typed状态与全局重置

函数直接复用：

- actor metric的两组动态数量；
- 最终角色状态的组A排除u16与完成u8；
- 动作状态的phase counter高word与packed actor counter低byte/byte2；
- 调试快捷键的battle mode flags；
- 预帧、调试叠加与结果判定共享的message state；
- 逐帧协调器active返回值；
- 组A/组B帧、调试快捷键、全局重置与本函数共用的唯一结果状态端口；
- 全帧暗化channel delta、唯一framebuffer和共享颜色偏移。

只新增结果latch、暗化门与组B强制值；channel delta复用已关闭暗化状态。全局重置按原write program：

- 清最终角色的排除u16与完成u8；
- 只清动作phase counter低word，保留本函数读取的高word；
- 只清动作packed actor counter低byte，保留byte2及其他高byte；
- 清结果latch、暗化门与强制值；
- 逐帧active由每帧入口重写1，channel delta不在reset写集合中并保持入口值。

## 7. caller回收

逐帧协调器在内部bit17与可选调试叠加之后无条件调用本函数，再直接组合已关闭的上下文提示绘制。旧第一个post-input opaque stage已删除并直接组合typed结果判定。

子暗化typed-stop保留此前整帧全部角色、UI、对话、倒计时与调试叠加副作用，以及本函数latch/颜色偏移前缀；随后阻断上下文提示、颜色初始化/累加、临时surface和截图。正常返回EAX按原caller不消费，继续后一阶段。

## 8. 测试与动态差分

定向测试覆盖：

- 两侧均不成立和组A减法unsigned下溢；
- 组B差值signed负数、强制值精确1/异常2；
- 组A暗化完成后的音频与结果整理顺序、模式2及两类模式清零；
- 组B独立完成不暂停音频并保留结果整理完整EAX；
- 两侧同帧暗化、第一次非终态后第二次delta继续推进；
- 第一次结果整理改写组B进度与delta后的动态重读和第二次结果整理；
- framebuffer typed-stop的latch、三颜色偏移和caller后续阻断；
- 全局重置低word/byte宽度、phase高word/packed byte2/channel delta保留；
- 唯一逐帧caller直连且旧opaque slot不调用。

当前缺少原版两组计数、七项结果状态、完整framebuffer、暗化callee寄存器、音频stream和`0x0045E9C0`结果整理联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
