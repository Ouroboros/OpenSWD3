# 标准特殊模式全局初始化 `0x00439DE0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x00439DE0..0x00439FC8`。直接调用点只有两个：

- 世界/MAPS重载路径`0x0040F1BF`，紧跟`0x0040E0B0`全局初始化。
- 进程启动路径`0x00424ECA`，同样紧跟`0x0040E0B0`，随后把特殊模式写为`0x80000003`。

两个调用者均不读取EAX。函数机器尾保留`EAX=0x232B`。

## 2. 固定调用与18个Act记录

函数先清`[0x004FBED8]`，再直接调用已关闭的`0x00444FC0`安装三张7槽模式回调表并完成第一次flag49查询，之后按地址顺序对18个`0x98`字节Act记录调用`0x0040DC00`：

```text
0x004FB970  0x004FC790  0x004FC840  0x004FB810
0x004FC450  0x004FC518  0x004FC328  0x004FC6E8
0x004FBA18  0x004FB8D8  0x004FC650  0x004FBFA0
0x004FC038  0x004FC0D0  0x004FC168  0x004FBEE0
0x004FC208  0x004FC5B0
```

`0x0040DC00`只重置Act记录内部的一次性variant、等待、命令游标和external mode字段，不清action id或base variant。现代实现复用`LegacyActionRecord`和`initialize_legacy_action_record()`，因此两个后续未写动作键的记录仍保留先前键值，不能扩大为整块清零。

## 3. 动作键、剧情标志和顺序

18次初始化后，机器先提交三个`0x232A`记录：

- 记录0：base variant `0`。
- 记录2：base variant `1`。
- 记录1：base variant `2`。

随后以索引`0x49`调用`0x0040DC50`。只有返回值字面等于`1`时，记录1的base variant才改为`3`；其他非零值仍保留`2`。查询后继续写：

- 记录3..9：action `0x232A`，base variant依次为`4, 5, 6, 0x18, 0x19, 0x1A, 0x1B`。
- 记录11..14：action `0x232B`，base variant依次为`0x2C..0x2F`。
- 记录16：action `0x232A`，base variant `3`。
- 记录17：action `0x233B`，base variant `0`。
- 记录10与15只有`0x0040DC00`的部分重置，不覆盖原动作键。

现代端口只隔离剧情标志查询；FC0三表写入、Act字段写入、两次查询位置和字面比较均由兼容核心直接保持。SDL在进程启动和新游戏MAPS重载两处、且都在剧情VM初始化之后调用该typed owner。

## 4. 验证

`special_modes.legacy_initial_menu`覆盖：

- FC0 callback表安装和第一次flag49查询先于18个Act初始化；第二次查询位于前三个动作键提交之后。
- 18个记录的部分重置字段、16个固定动作键和两个保留动作键。
- 剧情标志结果`1`选择variant `3`，结果`2`保持variant `2`。
- 共享退出位清零、调用计数和机器尾返回值`0x232B`。

Linux core定向与完整门、Linux app完整门和Windows LLVM app完整门通过后，workpack只新增关闭`0x00439DE0`。workpack连续两轮生成均为`2/227`，SHA256均为`895ddf3dc0d11da21a3fe4e216719878e8da3901923999183aa88fa9a172c34e`。后续FC0工作包已直接回收该callee；其独立证据记录三表21项写入与flag1六项交换。
