# 战斗角色metric重建 `0x0045B0E0`

既有关闭状态为`platform_adapted`。工作包282正在回收坐标callee并修正栈别名合同；本轮改动尚未完成发布验证。

## 1. 完整LST范围

权威函数为`0x0045B0E0..0x0045B18B`，2个静态call站点、4个`loc_`标签，无外部FUNCTION CHUNK。
两个站点`0x0045B11F/0x0045B164`都调用坐标查询`0x004783B0`。

共有14个静态caller站点。七个已关闭站点位于战斗启动、逐帧协调、角色动作分派、对手动作分派两处和最终角色步进两处；它们使用统一typed实现。
其余七处属于尚未关闭的后续战斗函数，不提前修改。

## 2. 双18 dword清零

入口固定先清零18 dword角色metric表，再清零18 dword角色顺序表。
两次都是`ECX=0x12`、`EAX=0`与正向`rep stosd`；角色数量为零也不跳过。

两张表保存在动作、启动与帧协调端口共同虚继承的单一typed状态中。
角色优先索引与逐帧选择值、启动初始化、调试C/W键共用该owner；攻击顺序出队证明它还是`0x0053AE70`起七dword输出记录的首项，后六dword也由同一state保存。

## 3. 两个WORD覆盖保存的ECX

`0x0045B0E0`的`push ecx`提供4字节栈局部：

- `var_4`对应入口ECX低WORD，是坐标Y输出。
- `var_2`对应入口ECX高WORD，是坐标X输出。

IDA把`var_2`命名为byte不能改变callee的写宽度。
`0x004783C5/0x004783E4`均在第一个输出地址写完整WORD，包含第四个栈byte。
两个局部不预清零；每次成功查询必定先写X再写Y。
查询异常停止时只保留已经执行的输出写入，不伪造callee返回。

typed状态的`local_word`和历史命名`local_byte`现在都为u16，分别承载同一保存dword的低、高WORD。
无角色时它们保持入口ECX；有角色时正常函数尾`pop ecx`得到最后一次完整输出`X << 16 | Y`。
旧文档所说的“恢复入口完整ECX”和“第一输出只覆盖byte”均被机器指令否定。

## 4. group B循环

先读取组B完整u32数量；零才跳过。循环从索引0开始：

1. EAX加载Y输出token，ECX先加载X输出token。
2. 先push Y，再push X，随后ECX加载组B角色token。
3. 调用坐标查询；角色token从`0x00525508`起按`0x2B28`步长计算。
4. Y按i16符号扩展到EDX。
5. 重新读取组B数量到EAX。
6. 把EDX写入metric表当前槽。
7. 索引加1，与刚读取的数量作unsigned比较。

已审计的坐标callee不修改共享数量。
保留原数量重载指令，但不能继续用opaque测试回调虚构callee修改数量的行为。
异常角色token不提前截断；typed视图不可达时在callee的原角色门读取处停止。

## 5. group A循环

组A从表索引8开始。入口计算`group_a_count + 8`的u32结果，以unsigned `<= 8`决定是否跳过。
零和高值回绕域都按原指令保留。

每轮ECX加载Y输出token、EDX加载X输出token；先push Y，再push X，然后ECX加载角色token。
EAX在该调用前没有被LEA覆盖：首次是`group_a_count + 8`，后续为上一轮符号扩展的Y。
角色token从`0x005029D0`起按`0x2F34`步长计算。

查询后把Y按i16符号扩展到EAX，读取组A数量到ECX，再写metric表。
索引加1后计算新的`group_a_count + 8`比较边界。
组A按原布局写表索引8起的区域，不另建隔离副本。

## 6. 返回与typed-stop

正常EAX：

- 无组A迭代时为`group_a_count + 8`的u32结果。
- 有组A迭代时为最后一次Y的i16符号扩展。

`0x0045B18A`的ECX pop消费被两个WORD写入改写的保存dword，不是入口ECX。
EDX在组B尾为最后Y的符号扩展；组A尾保留最后一次坐标查询的分支相关EDX；无角色时保留入口EDX。

坐标callee发生typed-stop时，保留双表清零、已完成的前序metric写入、当前查询的部分WORD输出和寄存器前缀。
不执行当前metric store、后续角色、组A循环或尾部pop。

metric数组仍只有18槽，原store越界检查仍位于实际store点。
对于当前固定组B视图，第九个角色已经超出8元素owner，因此先停在第九次callee角色门读取，而不会到达第十九次metric store。
原先依靠任意成功opaque回调继续到第十九次store的测试不再代表已审计callee的行为。

## 7. caller与本轮测试

七个已关闭caller继续在原调用点组合统一metric实现，子函数typed-stop阻止后续阶段。
本轮删除metric实现内部`0x004783B0` opaque回调，三种端口重载共用直接查询。
坐标查询次数单独计数，不再伪装成外部port调用。

从LST推导的测试覆盖：

- 双表固定清零、零数量与完整入口ECX保留。
- 两组固定token、主/备用坐标和Y的i16符号扩展。
- X的高byte也写入保存dword，正常ECX为`X << 16 | Y`。
- 第二次坐标读取失败，第一WORD已提交但metric表未写、ECX未pop。
- 绑定后访问可达性改变，查询不使用陈旧可达性快照。
- 第九个组B角色门读取停止，保留前八次完整store并阻断组A。
- 组A加8回绕、跨动作/启动端口的单一metric状态。

当前本轮定向门尚未通过。ASan已将首次段错误定位到动作测试的栈溢出；绑定数组改为堆存储后仍需重新验证。
原版两组完整对象、异常内存页、栈地址和寄存器联合捕获缺失，动态差分仍为`blocked_runtime_oracle`，不替代现代侧测试。
