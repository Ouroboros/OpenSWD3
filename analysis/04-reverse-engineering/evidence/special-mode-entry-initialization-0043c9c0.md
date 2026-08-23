# 标准模式entry初始化 `0x0043C9C0`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围、参数与caller

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043C9C0..0x0043CBC6`，无自身外置chunk。三个直接callsite为：

- `0x0043C0D0`在`0x0043C2A4`调用。
- `0x0043C760`在`0x0043C781`调用。
- `0x00446420`的外置chunk `0x0043C6E0`在`0x0043C704`调用。

IDA导航decompile显示两个参数，但函数体只读取第二个stack参数作为mode index；第一个entry base参数完全未读。typed API不伪造未读输入，直接使用runtime state中的64项entry owner。

`0x0043C0D0`和`0x0043C760`已回接本helper；未关闭的`0x00446420`及其`0x0043C6E0`chunk留待该owner独立LST审计，不提前计数。

## 2. 固定mode映射与第一轮1..500扫描

函数先清64项entry表，并只清64个short text槽首字节。随后按mode index裸读15项stack映射：

```text
index:  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14
class:  0 1 2 3 4 5 6 7 8 9 13 14  0  0  0
```

原程序对负值或大于14的index越界读stack；modern在该原读取点typed-stop。因此entry和short text首字节已清，但后续64-byte status表、total/window/cursor/alias尚未改写。

合法mode继续清64-byte entry status表、把total置0，并严格查询record ID 1..500。classification byte按`movsx`解释为signed i8，再与映射出的signed dword比较。每个命中依次写entry ID、重清对应short首字节与status byte，再递增total。

64项表还要求在`entries[total]`写终止0：

- 第65个命中在`entries[64]`首次写入点typed-stop，保留前64项和total64。
- 恰好64个命中完成500次查询后，在终止项`entries[64]`写入点typed-stop。

不把这两种原始越界折叠成安全截断或自动丢弃。

## 3. 第二轮status/load/text/token

写入终止0后再次严格扫描ID 1..500。每个ID先读取status byte；零值跳过，非零值固定遍历全部64项entry并寻找相同ID。

每个匹配项：

1. 以一次性清零的本地`0xB0` scratch之`+0x0C`和ID调用record loader；scratch不会在不同匹配项之间重新清零。
2. load成功时，以Windows `lstrcpyA`语义把`scratch+0x0C` NUL字符串复制到16-byte short槽。
3. 成功复制后再次读取该ID的status byte并写对应entry status；所以成功项会发生两次status读取，第二次值可以不同。
4. load成功或失败均读取`scratch+0xAC` token、调用release并把四个token字节清0。

原`lstrcpyA`在source无NUL或文本长度大于15时会越界读/写。modern分别在原copy点报告`loaded_text_not_terminated`和`loaded_text_out_of_range`；此前status读取和load副作用保留，copy后的status二次读取、token release及尾部owner重置不伪造。

## 4. 尾部owner与返回EAX

完成第二轮后释放本地scratch，并只清：

- window offset `FC90C`。
- local cursor `FC928`。
- entry alias设为entry base，即typed index0。

mode index和visible count不由本函数改写。最后tail调用`0x0043CBD0`，返回其EAX。

record loader、classification/status数据库、token release与尚未关闭的page refresh继续由共享typed port隔离；表控制流、scratch、entry/text/status owner和边界均已在本helper内关闭。

## 5. caller纠正

`0x0043C0D0`不再调用“填零并伪造entry0”的窄port，而是执行完整C9C0两轮扫描、page refresh后再写action record0并消费真实entry0。

`0x0043C760`推进/clamp mode后执行本helper。C9C0会把既有window/cursor/alias清0，因此后续selected读取通常为entry0；此前测试中保留window64并在selected点停止的synthetic行为已删除。`INT_MAX→INT_MIN`仍由C760回绕产生，但随后在C9C0 mode映射读取点typed-stop，不再伪造任意mode entry表。

## 6. 验证

`special_modes.legacy_initial_menu`覆盖：

- mode1映射class1，signed classification查询精确1..500。
- IDs 2/4/500形成`[2,4,500,0]`、total3。
- status扫描500次加两个成功load的二次读取，共502次。
- ID2/500成功文本与第二次status发布；ID4 load失败保持空文本/status0。
- scratch不逐次清零，三个load前token字段均因前次release后清0。
- 成功及失败load的三个token按序释放，tail refresh EAX返回。
- mode越界的精确副作用截面。
- 第65项写入与64项terminator两种独立typed-stop。
- source无NUL与16字符目的越界均在copy点停止且不伪造release。
- C0D0的500 load、500 query、C9C0双扫描/refresh、entry0消费顺序。
- C760 mode10/11映射14、window/cursor清0、entry0消费；`INT_MAX→INT_MIN`传播C9C0停止。
- C3C0 mode caller保留内部/外部两次sample，但现在包含C9C0 refresh和真实entry1消费。

定向测试通过。workpack连续生成两轮均为`36/227`，SHA256均为`6d00895fa07920a2aa3d71411fe434ce98aeacf38e907603a8e9b8b51b0719f7`；只新增关闭`0x0043C9C0`，`0x0043CBD0`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
