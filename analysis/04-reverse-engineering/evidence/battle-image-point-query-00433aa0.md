# 战斗TSW命令流像素命中查询 `0x00433AA0`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`。

## 1. 范围与调用

权威LST函数范围为`0x00433AA0..0x00433BEA`，入口`proc`至`endp`连续，没有外部`FUNCTION CHUNK`。

唯一caller是战斗函数`0x0045FC60`，实际调用点共三处：

- `0x00460649`；
- `0x00460868`；
- `0x00460A68`。

三处都压入旧TSW surface记录、屏幕点和原点坐标，调用后立即测试EAX。返回非零表示该点命中literal像素，返回零表示透明或不命中；caller据此继续或拒绝战斗候选。

当前现代工程尚未实现`0x0045FC60`，因此没有既存opaque callback可回收。后续关闭caller时必须直接调用本typed查询，不得重新复制命令流解析。

## 2. ABI与输入

旧入口接收五个栈参数：

1. 旧surface记录指针；
2. 点X；
3. 点Y；
4. 原点X；
5. 原点Y。

surface记录中：

- `+0x00`是命令流数据指针；
- `+0x0C`是零扩展的16位宽度；
- `+0x0E`是零扩展的16位高度。

IDA把EBP显示为额外`__usercall`输入，但完整LST不支持该语义。函数只在行目录循环前`push ebp`，用`mov bp`覆盖低16位作为临时行长，再经右移和`and 0x3FFF`清除所有旧高位，随后`pop ebp`恢复caller寄存器。入口EBP的原值不影响返回结果。

现代接口把surface owner拆成：

- `std::span<const u8>`命令流；
- 独立的`u16 width/height`元数据；
- 四个`i32`坐标。

宽高不从命令流头重新解释，因为旧函数明确读取surface记录的`+0x0C/+0x0E`，允许两者与命令流头不一致。

## 3. 坐标前缀 `0x00433AA0..0x00433AD1`

LST顺序：

1. `EDX = [surface+0]`，只取得裸数据指针，尚未读取数据；
2. `ESI = point_x - origin_x`，32位回绕后按`signed i32`解释；
3. `EDI = point_y - origin_y`，32位回绕后按`signed i32`解释；
4. local X小于零时返回零；
5. local Y小于零时，只有local X大于等于surface宽度才返回零；
6. 其他情况继续。

负local Y不是普通拒绝条件。只要local X小于宽度，它会继续遍历全部行目录，并在目录尾后解释命令。这是原始可观察异常，现代实现保留。

坐标检查发生在首次命令流读取之前。测试用空span证明负X、负Y且X达到宽度、Y达到高度都直接返回透明；合法坐标才在原magic读取点报告短源。

## 4. 高度、magic与行目录 `0x00433AD7..0x00433B1B`

- `ECX`零扩展读取surface高度；
- local Y按signed比较，大于等于高度时返回零；
- 首次读取命令流word，必须等于`0xFFFF`，否则返回零；
- 数据游标固定加8，跳过命令流头；
- 行索引从零开始；高度为零时直接进入当前游标解释；
- 行索引等于local Y时只把游标加2，跳过目标行的行长word；
- 非目标行读取行长word，以`((word >> 1) & 0x3FFF) * 2`推进；
- local Y为负时不会等于任何非负行索引，因此遍历完全部高度后从目录尾继续；
- 目标游标word为零时返回零；
- 随后才读取surface宽度，宽度为零时返回零。

行目录推进使用行长word自身编码的总字节数，不额外为非目标行再加2。

## 5. run扫描 `0x00433B21..0x00433BD6`

扫描位置EAX从零开始。每轮读取当前16位command，按以下原始顺序处理。

### 5.1 组合高标记分支

`test ch, 0xC0`同时检查command的bit14与bit15。任一位存在即：

1. 对完整ECX加`0x4000`；
2. 只保留结果低16位作为run长度；
3. local X若位于`[scan, scan + length]`则返回零；
4. 否则scan增加该长度，游标只加2。

区间上界使用`jle`，因此是包含上界，不是常见的半开区间。

对于正常`0xC000 | count`，加`0x4000`再截低16得到count。对于`0x8000`或`0x4000`标记，该顺序会产生很大的低16位长度。

### 5.2 两个原始不可达分支

组合检查为零后，代码继续测试：

- `test ch, 0x80`，拟对command加`0x8000`；
- 再`test ch, 0x40`，拟对command减`0x4000`。

这两支在同一未修改CH上不可达：若bit15或bit14存在，前面的`test ch,0xC0`已经进入组合分支；若组合检查为零，两个子位也必定为零。

现代C++仍按相同条件顺序保留两个分支，不把它们“修正”为可达，也不把三类tag统一现代化为`command & 0x3FFF`。合成测试固定`0x8002`和`0x4002`都会被组合分支解释成超大透明run。

### 5.3 literal分支

没有bit14/15时：

1. command完整低16位就是literal长度；
2. local X位于包含上界的`[scan, scan + length]`时返回一；
3. 否则scan增加长度；
4. 游标增加`length * 2 + 2`，跳过command和16位literal payload。

函数从不读取literal像素值。若查询点已落入literal区间，即使payload不存在也返回一；若必须读取下一command，才在下一读取点触发短源typed-stop。

### 5.4 循环与返回

每个未命中的run结束后，以signed比较检查`scan < width`：

- 成立则读取下一command；
- 不成立则返回零。

普通零返回统一位于`0x00433BDC`，一返回位于`0x00433BE2`。除安全适配状态外，现代实现的`return_value`保持`0/1`。

## 6. 平台安全适配

原函数对短命令流、越界行目录和损坏run游标执行裸读取。现代接口使用span并返回：

- `transparent`，旧EAX为零；
- `visible`，旧EAX为一；
- `source_exhausted`，只在原代码即将读取magic、行长或command的地点停止，EAX兼容值为零。

游标推进本身不提前报错。literal payload不会被额外验证；只有后续原始command读取需要该地址时才停止。这保留了“缺payload但查询已命中”与“scan已到宽度而不再读取”的原顺序。

裸surface指针的有效性由未来caller建立typed span时隔离；查询内部仍保持坐标前缀先于任何span字节读取。

## 7. 双向追溯

LST到C++：

- `0x00433AA0..0x00433AD1`映射为回绕坐标与三项前置拒绝；
- `0x00433AD7..0x00433AEA`映射为高度、magic和固定头跳过；
- `0x00433AF0..0x00433B1B`映射为目标行/非目标行目录推进及初始零word；
- `0x00433B21..0x00433B60`映射为组合tag分支；
- `0x00433B62..0x00433BB4`保留两个不可达子tag分支；
- `0x00433BB6..0x00433BD6`映射为literal区间、payload跳过和循环；
- `0x00433BDC..0x00433BEA`映射为零/一双出口。

C++到LST：

- 每项坐标判断均有对应`test/cmp/jcc`；
- 每项span失败只对应一个原始word读取；
- 三段tag条件、三个算术常量和低16截断均直接来自LST；
- inclusive区间、signed循环比较、负行异常和零宽检查时点均无现代新增语义；
- 结果状态中的`source_exhausted`是唯一平台隔离，不作为旧成功返回。

最后一轮完整正向与反向追溯未发现未解释基本块或C++行为。

## 8. 测试

合成定向测试覆盖：

- 透明run首点与包含上界；
- literal内部点与包含上界；
- 负X、负Y、Y达到高度的读取顺序；
- 合法坐标的magic短源；
- 负Y遍历目录尾后继续并返回可见；
- 两行目录精确选择；
- `0x8000/0x4000`组合高标记异常；
- 命中literal时不读payload；
- 未命中后只在下一command读取点停止；
- 零宽仍先读取首command。

真实资产测试从`all_item.tsw`、`all_magic.tsw`和`all_map1.tsw`的真实16位首帧命令流中，由独立规范扫描器选择标记run内部点和未经过异常tag的literal内部点，再验证typed查询结果。扫描预期不调用被测函数。

定向命令流测试、TSW合成测试和真实TSW测试全部通过。

## 9. 动态差分

当前没有可用原版战斗像素命中捕获后端，`original_diff_verified`仍为`blocked_runtime_oracle`。该阻塞不改变完整LST、typed实现、合成边界和真实TSW资产已经收敛的结论。
