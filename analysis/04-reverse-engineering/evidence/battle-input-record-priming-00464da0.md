# 战斗输入记录预置 `0x00464DA0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00464DA0..0x00464DC0`，从proc到endp共16行、7条实际指令、0个call、0个跳转、0个局部标签、1个`retn`，没有外部`FUNCTION CHUNK`。

唯一caller是已关闭目标选择刷新；权威LST中共有14个静态调用点：`0x00462B6B`、`0x0046307B`、`0x00463374`、`0x00463546`、`0x004635C7`、`0x00463638`、`0x0046365C`、`0x004636F3`、`0x00463859`、`0x0046387D`、`0x004639C2`、`0x00463A8F`、`0x00463CCB`、`0x00463DF1`。

## 2. 四项物理写

函数先无条件置ECX=1、EAX=2，再严格按下列顺序写四个dword：

1. `0x004B7CC0 = 1`；
2. `0x004B7CCC = 2`；
3. `0x004B7DAC = 2`；
4. `0x004B7D7C = 1`。

输入记录物理基址是`0x004B7CB0`，每项16字节。因此四个地址分别映射到唯一输入归一化owner：

- 记录1的rapid-press multiplicity；
- 记录1的held sample count；
- 记录15的held sample count；
- 记录12的held sample count。

实现不建立第二份battle输入状态，只从主帧已有input normalization依次经输入分派、目标选择入口传入同一records span。

## 3. typed-stop与寄存器

EAX和ECX在首次内存访问前已经成为2和1，EDX从caller原样保留。records不含记录1时，在首项真实写停止，零写入；只含记录0..1时，先完成记录1的两项写，再在记录15首次访问停止。连续span若含记录15必然也含记录12，因此成功路径固定完成四写。

不预先要求20项完整数组，也不回滚先前写入。普通返回固定EAX=2、ECX=1、EDX不变。

## 4. caller回收

目标选择刷新原先把该callee误保留为`refresh_target_display`窄端口。现将同一枚举数值改为`reserved_input_record_priming_slot`，生产代码零调用；所有原路径统一直连typed预置。

现有C++把权威LST中多个等价返回块收敛为共享分支，因此7处直接组合覆盖14个原静态调用点：message 1公共尾、message 2/4/8/27/30共同尾、message 3清理尾、动作15直接提交、message 5、message 7动作99、message 200全清尾。每个位置都保留原callee相对前后写与后续对象调用的顺序；子typed-stop保留此前动作、缓存、角色或目标发布并阻断后续路径。

定向测试覆盖首项停止、前两写后停止、四项精确写集合、EAX/ECX常量、EDX保留、message 1普通caller、message 7前两写后传播以及reserved槽零调用。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。

函数无输入分支、无callee和无不确定外部状态；完整LST与逐字段固定状态测试已覆盖全部可观察行为，不依赖动态oracle。
