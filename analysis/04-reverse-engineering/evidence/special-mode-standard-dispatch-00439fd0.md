# 标准特殊模式总入口 `0x00439FD0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x00439FD0..0x0043A293`，唯一直接调用点为主帧`0x0040AB02`。主帧仅对低28位mode1、3、4、5、6调用该入口；mode2由`0x0044EA60`处理。

入口读取带标签mode值`[0x004B8740]`。bit31为一次性初始化门，bit30只选择低模式初始化变体，bit29不是初始化位。

## 2. 初始化路径

bit31为一时首先按顺序：

1. 清`[0x004FBAC4]`和`[0x004FBED0]`。
2. 置`[0x004FC848]=1`。
3. 取低28位mode。

低28位小于等于2时，固定写入`0x232A`动作组：主variant`0x34`、两个次记录variant`0x1A/0x1B`、四个选择记录variant`8..11`。bit30为零时选择`(resource=0x1E, selector=1, word=0)`；bit30为一时选择`(resource=0x24, selector=2, word=1)`并安装`0x004407F0`回调。之后播放sound `0x00BB`。

mode3和6先调用`0x00431960`，清四个`0x98`字节动作记录，再将主variant写为`0x4E`，四个选择动作写为`0x232B/0x2C..0x2F`，最后以resource`0xEA60`和selector`0/3`调用`0x0043A2A0`。mode4和5同样先调用`0x00431960`，再以resource`0xEA60`和selector`1/2`调用`0x0043A2A0`。

初始化公共尾将帧字段写`0x40`，清bit31和bit30但保留bit29及低30位，并清`[0x004FBED8]`。现代核心把进程全局动作记录写入整理为显式初始化request；SDL mode3将共享记录落到`LegacyInitialMenuState`，其余mode继续由各自未关闭子工作包消费。

## 3. 每帧公共尾

无论是否进入初始化：

1. 帧字段按`u32`加一。
2. 调用`0x0043BA40`。
3. 以参数0调用`0x0043A470`。
4. 重新读取完整mode值；非零时以参数0调用`0x0043A610`。
5. input或draw将mode清零时，清`[0x004FBED8]`的bit1。

现代端口保持`update -> input -> conditional draw -> exit cleanup`顺序。SDL主帧对mode1、3、4、5、6统一调用该总入口；mode3的draw端继续消费既有`LegacyInitialMenuState`完整帧路径并上交新游戏或关闭事件，低模式初始化音效接到真实sample owner。未关闭的mode1、4、5、6子图仍只执行既有呈现占位，没有在总入口中伪造输入或业务成功。

## 4. 验证

`special_modes.legacy_initial_menu`同一测试二进制覆盖：

- bit30正常和alternate两侧的完整低模式请求、sound和高位消费。
- mode3/4/5/6到selector`0/1/2/3`，以及仅mode3/6执行的共享动作记录写入。
- bit29保留，bit31/30清除，初始化帧值最终为`0x41`。
- 无初始化路径的`u32`帧回绕。
- input清mode时跳过draw；draw清mode时保留本次draw；两者均在尾部清transient bit1。
- 表外mode仍消费bit31并执行公共尾，但不伪造mode初始化。

Linux core定向与完整门、Linux app完整门和Windows LLVM app完整门通过后，工作包只关闭`0x00439FD0`。workpack连续两轮生成均为`1/227`，SHA256均为`2f654cc75235148ab08330efc25ff9a072b75f4233adaa45bb5ce45aa800c167`。`0x0043A2A0`、`0x0043A470`、`0x0043A610`、`0x0043BA40`及各mode子图保持`pending_audit`。
