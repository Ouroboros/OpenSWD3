# 战斗方向图块双层扫描与surface像素写入 `0x004344E0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、callee与caller

权威LST函数范围为`0x004344E0..0x00434788`，入口`proc`至`endp`连续，没有外部`FUNCTION CHUNK`。

两个callee已经关闭：

- `0x00434420`：按360项方向向量表推进局部线段记录，本函数调用两处；
- `0x004207E0`：对一个源像素与目标像素执行通道和达到32时清零的合成，本函数调用一处。

现代实现直接调用两个typed入口，不复制方向表推进或颜色合成，完成callee回收。

唯一直接caller为`0x00479850`。它把共享16位输出surface指针与战斗对象内的图块扫描记录传入，调用后立即清EAX为零，不消费本函数返回值。

## 2. 输入记录字段

完整LST使用记录字段：

- `+0x00`：源图字节指针；
- `+0x04`：unsigned 16位源宽度；
- `+0x06`：unsigned 16位源高度；
- `+0x10`：signed 32位目标起始X；
- `+0x14`：signed 32位目标起始Y；
- `+0x20`：signed 32位水平固定点除数；
- `+0x24`：signed 32位垂直固定点除数；
- `+0x28`：低字节扫描标志；
- `+0x2C/+0x30/+0x34`：调用前发布到三个共享渲染标量的值；
- `+0x38`：第一条线段的方向索引。

其余字段不在本函数中读取。现代`LegacyBattleDirectionalScanSource`只暴露该函数实际消费的值，并用字节span表达可能非结构化的源图存储。

## 3. 共享值发布与源基址 `0x004344E9..0x0043452D`

函数先按顺序：

1. 发布`+0x2C`；
2. 计算源字节基址相对量；
3. 发布`+0x30`；
4. 发布`+0x34`。

源相对基址按32位回绕计算：

```text
(2 * unsigned_height - 2) * unsigned_width
```

该结果直接按字节加到`+0x00`源指针，不乘像素字节数。后续源索引才以`index * 2`定位16位像素。现代实现保留这两个不同单位，不把源基址误写成u16数组下标。

三个共享发布发生在任何除法、循环、源读取或目标写入之前。除零typed-stop仍保留全部三项。

## 4. 两次signed固定点除法 `0x00434523..0x00434568`

水平商：

```text
horizontal_fixed_step = (unsigned_width << 10) / horizontal_divisor
```

垂直商：

```text
vertical_fixed_step = (unsigned_height << 10) / vertical_divisor
```

两次均为x86 signed `idiv`；分子先在32位EAX中形成并符号扩展到EDX:EAX。unsigned 16位尺寸左移10不会超出signed 32位，因此唯一故障域是除数为零。

现代实现：

- 水平除数为零时，在第一条`idiv`位置返回`horizontal_divisor_zero`；
- 垂直除数为零时，保留已完成的水平商局部前缀，在第二条`idiv`位置返回`vertical_divisor_zero`；
- 两种状态都保留此前三个共享发布，不执行局部记录推进或像素访问；
- 不把除零改成零商或成功返回。

## 5. 两份局部方向记录 `0x00434547..0x004345A6`

两个28字节局部记录先分别清零七个dword。

第一份记录：

- 方向=`source.direction_index`；
- 当前X=`start_x`；
- 当前Y=`start_y`；
- 两个误差为零。

第二份记录：

- 方向=`wrapping(source.direction_index + 90) signed-rem 360`；
- 当前X=`start_x`；
- 当前Y=`start_y`；
- 两个误差为零。

`direction + 90`先32位回绕，再以signed余数写第二方向；负余数不会修正到正区间。

随后测试`vertical_fixed_step`。商小于等于零时立即退出，EAX仍为刚读入的`start_y`。现代结果显式保留该正常返回残值。

## 6. 外层循环初始化与推进

垂直商为正时：

- `surface_y = wrapping(start_y + vertical_fixed_step)`；
- 外层次数=`vertical_fixed_step`；
- 行表字节偏移=`surface_y << 2`，32位回绕；
- 垂直固定点累加器和源行偏移均为零。

每轮首先直接调用第一份方向记录的typed推进。合法方向下，把推进后的当前X/Y复制到第二记录，并把第二记录两个误差重新清零。

若第一方向索引不在0..359，typed-stop发生在已关闭callee的原首次水平表读取点。三个共享值保留，但尚无源读取或目标写入。

外层尾部按原顺序：

1. 固定点累加器加`vertical_divisor`，32位回绕；
2. 算术右移10；
3. 与源宽度相乘，低32位成为下一轮源行偏移；
4. `surface_y`减一；
5. 行表字节偏移减4；
6. 外层剩余次数减一。

## 7. 内层循环与固定点采样

`horizontal_fixed_step <= 0`时，本轮完全跳过内层，但仍执行外层尾部和后续外层方向推进。

正商时：

- 目标X从`start_x`开始；
- 内层次数=`horizontal_fixed_step`；
- 水平固定点累加器与源X均从零开始。

每轮先直接调用第二份方向记录的typed推进；该局部记录的坐标不作为目标坐标，原函数只保留其方向误差状态。

无论本轮是否越界、透明或写入，尾部都：

1. 水平固定点累加器加`horizontal_divisor`，32位回绕；
2. 算术右移10得到下一源X；
3. 目标X加一，32位回绕；
4. 内层剩余次数减一。

## 8. 目标边界门

方向推进后先检查：

- `0 <= destination_x < surface_width`；
- `0 <= surface_y < surface_height`。

任一失败就跳到内层尾部，不读取源图、透明色、surface行表或目标像素。

现代测试用空源和空行表证明越界目标仍能完成该轮，不产生伪造typed-stop。

## 9. 源索引、镜像与原读取点

低字节flags的bit0选择源索引：

- bit0为零：`source_x - source_row_offset`；
- bit0为一：`unsigned_width - source_x - source_row_offset`。

所有减法均32位回绕。

最终源字节偏移：

```text
source_base_byte_offset + wrapping(source_index * 2)
```

原代码从该地址读取一个可能仅按字节计算得到的little-endian u16。现代实现只在该两字节读取实际越出源span时返回`source_out_of_range`，保留此前方向推进及已完成像素前缀。

## 10. 透明色与写入模式

源像素低16位依次与两个共享RGB555特殊色比较：

- `0x319F`；
- `0x026B`。

命中任一颜色立即跳到内层尾部，且发生在surface行表读取之前。

非透明像素再测试flags低字节与`0x16`：

- 结果为零：直接把源u16写到目标；
- 结果非零：以`source,destination,count=1`直接调用已关闭的通道溢出清零合成。

合成入口继续更新其原有效颜色mask状态；本函数不复制算法，也不把它替换为饱和加色。

## 11. surface行表与目标写入typed-stop

需要写像素时才：

1. 按回绕行表字节偏移读取u32行偏移；
2. 计算`wrapping(row_offset + destination_x)`；
3. 在输出u16 surface读取/写入一个目标像素。

现代边界：

- 行表span不足时在原u32行偏移读取点返回`row_table_out_of_range`；
- 行偏移导致目标span不足时，在原目标像素读取/写入点返回`destination_out_of_range`；
- 已完成的透明跳过、边界跳过和先前像素写入均保留；
- 不把损坏行表夹值到最后一行或丢弃整个扫描。

## 12. 返回EAX

正常路径有两个EAX合同：

- `vertical_fixed_step <= 0`：EAX为入口`start_y`；
- 正商完成全部外层循环：最终`dec outer_remaining`得到零，EAX为零。

唯一caller不消费该值，但typed结果仍记录`legacy_return_value`。

除零、非法方向、源/行表/目标越界是原故障或内存破坏域的typed-stop，不伪装为正常EAX返回。

## 13. 双向追溯

LST到C++：

- `0x004344E9..0x0043452D`映射为三项共享发布和字节源基址；
- `0x00434531..0x00434568`映射为两次signed固定点除法；
- `0x00434547..0x004345A2`映射为两份局部记录清零、方向余数和起始坐标；
- `0x004345A6..0x004345FE`映射为垂直商门、外层初始化、第一方向推进和内层初始化；
- `0x004345FE..0x0043471B`映射为第二方向推进、目标边界、源读取、透明门和两种写入；
- `0x0043471B..0x00434740`映射为内层固定点尾部；
- `0x00434740..0x00434781`映射为外层固定点、行表和次数尾部；
- `0x00434781..0x00434788`映射为EAX残值出口。

C++到LST：

- 每个输入字段对应明确记录位移；
- 两个方向调用和颜色合成直接对应已关闭callee；
- 所有wrap、signed除法和算术右移对应原x86指令；
- source/row/destination状态只隔离各自原实际访问点；
- 结果计数只记录已发生的可观察前缀，不驱动行为；
- 没有新增插值、颜色转换、边界夹值、循环上限或资源访问。

完整正向与反向追溯未发现未解释基本块、未回收callee、错误单位或遗漏出口。

## 14. 测试

定向测试覆盖：

- 单像素直接写与三项共享发布；
- bit0镜像源索引；
- 两种透明色均在行表读取前跳过；
- `0x16`标志门直接复用单像素溢出清零合成；
- 两外层、两内层固定点采样，首行越界跳过、次行反向源行读取；
- 目标越界时空源与空行表仍不读取；
- 水平除零与垂直除零前缀；
- 垂直商非正立即返回start_y；
- 水平商非正只跳过内层；
- 非法方向在首次方向表读取点停止；
- 源短读、行表短读与损坏行偏移目标越界的逐点typed-stop；
- 既有方向推进和frame-color测试继续通过。

本函数消费运行时图块记录与surface，没有独立物理游戏资产文件。固定状态覆盖全部LST基本块、两个callee路径和内存边界顺序。

定向battle聚合测试通过，目标构建零warning。

## 15. 动态差分

当前没有可用原版战斗图块记录、方向表和surface像素联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。该阻塞不改变完整LST、两个callee直接回收、全部循环与故障前缀、typed实现和固定状态已经闭环的结论。
