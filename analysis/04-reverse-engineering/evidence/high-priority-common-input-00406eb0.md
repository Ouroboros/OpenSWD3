# 高优先级菜单公共原始输入 `0x00406EB0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x00406EB0..0x00406F62`，共92行，无外部`FUNCTION CHUNK`。唯一caller为逐帧协调`0x00406E30`；该caller已回收原opaque边界并直接调用本typed入口。

原始键读取`0x004372D0`和合成置位`0x00437300`已关闭，现代直接复用256字节DIK快照helper。尾部状态处理`0x00409540`跨后续特殊模式和B11预览owner，继续由窄端口表达。

## 2. 右键首样本合成Escape

入口先检查归一化右键记录：rapid multiplicity非零且held sample count字面等于1时，对当前DIK快照索引1执行`OR 0x80`。该写入发生在本帧设备采样之后，供后续同函数Escape查询消费；不创建SDL事件，也不清右键记录。

## 3. F1/F2独立切换

按固定顺序查询DIK `0x3B`和`0x3C`。每个键仅在raw高位置位且input mode等于0时：

1. 阻塞等待500毫秒。
2. F1写submode 0，F2写submode 1。

两个判断不是`else-if`。两键同按会等待两次，先写0再写1，最终为1。input mode非零只禁止等待和submode写入，不禁止后续Escape/Ctrl查询。

## 4. Escape与Ctrl优先级

随后依次查询：

1. DIK 1，Escape。
2. DIK `0x1D`，左Ctrl。
3. DIK `0x9D`，右Ctrl。

首个按下键立即尾调状态处理，后续键不再查询。无键时返回最后一次右Ctrl raw结果0。端口不可用时保留右键合成、已发生的F键等待、submode写入及当前raw返回`0x80`。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- 右键首样本合成Escape并在同帧触发尾调。
- F1/F2同按的两次500毫秒等待和最终submode 1。
- input mode非零时禁止F键副作用，但仍完整查询到右Ctrl并尾调。
- 空输入固定五次raw查询并返回0。
- 左Ctrl触发时只查询到第四项，尾调不可用保留raw `0x80`。
- `0x00406E30`通过本typed入口同步submode/activity，不再保留公共输入opaque回调。
