# 战斗绘制对象surface行偏移表重建 `0x00433E90`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围与调用

权威LST函数范围为`0x00433E90..0x00433EFC`，入口`proc`至`endp`连续，没有外部`FUNCTION CHUNK`。

三个直接caller为：

- `0x00433C40`：固定以`row_stride = 640`、`row_count = 480`调用；
- `0x00433DC0`：以外部宽度的一半和完整高度调用；
- `0x00433F30`：以其两个宿主surface尺寸参数调用。

三个caller当前都未进入现代实现，因此没有既存opaque callback可回收。后续关闭caller时必须直接调用本typed入口。

## 2. thiscall ABI与独立对象切片

ECX指向旧战斗绘制对象，两个栈参数依次为signed 32位行步长与signed 32位行数。

本函数写入的对象切片与主行表不同：

- `+0x0B44`：surface行偏移表指针；
- `+0x0B50`：surface行步长，同时被后续绘制矩形当作surface宽度；
- `+0x0B54`：surface行数，同时被后续绘制矩形当作surface高度。

现代`LegacyBattleRenderGeometry`以`surface_row_offsets`、`surface_width`和`surface_height`映射该切片。主行表的独占数组、步长和行数是另一个owner，调用本函数时不得改变。

返回寄存器合同与完整LST一致：分配失败或成功的非正行数返回零；正行数完整填充返回行数；typed写入停止返回已递增到原越界写入点的行索引。

## 3. 与主行表函数的同构关系

`0x00433E90`不是对`0x00433E20`的跳转或调用，而是独立函数体。两者控制流和指令顺序同构，仅以下对象位移不同：

- 旧指针`+0x0B40`改为`+0x0B44`；
- 步长`+0x0B48`改为`+0x0B50`；
- 行数`+0x0B4C`改为`+0x0B54`。

两者各自调用CRT free和malloc，拥有独立返回与`retn 8`。因此工作包仍需独立LST审计、证据、测试和关闭记录。

现代实现只复用已经验证的内部行表核心。两个公开typed入口分别绑定主表与surface表的独立数组和元数据引用，不通过地址、selector或运行时分支混用owner。

## 4. 释放与申请 `0x00433E90..0x00433ECF`

LST顺序：

1. 读取surface表指针；
2. 非空时free旧表；
3. free返回后清surface表指针；
4. 行数乘4并只保留32位低位；
5. 调用malloc；
6. 先发布返回指针，再测试是否为空；
7. 为空立即退出。

分配失败只影响surface表指针，不清`surface_width/surface_height`，也不触碰主行表。

行数的符号不参与申请大小计算。零行仍申请零字节；负行和大正行均保留乘4回绕结果。

## 5. 成功后的尺寸发布 `0x00433ECF..0x00433EE5`

指针非空后，函数：

1. 读取行步长；
2. 清EAX为零；
3. 测试signed行数；
4. 发布surface宽度/行步长；
5. 发布surface高度/行数；
6. 行数小于等于零时退出。

因此零行或负行的成功分配仍会替换后续矩形消费者看到的surface尺寸，只是不填充任何行偏移。

已关闭的`0x004342E0`直接读取这两个元数据。定向测试先用本函数发布640×480，再让矩形放置处理右下越界，结果反向移动到`left=620/top=460/right=640/bottom=480`，证明typed owner连接正确。

## 6. 逐行填充 `0x00433EE5..0x00433EFA`

正行数路径与LST逐项对应：

- 当前字节偏移从零开始；
- 每轮重新读取surface表指针；
- EAX先递增；
- 把当前偏移写入`table[eax - 1]`；
- 以32位回绕增加行步长；
- signed比较EAX与行数决定继续。

固定640×480结果：

- 第0行为`0`；
- 第1行为`640`；
- 第479行为`0x4AD80`；
- EAX返回`480`。

主行表的指针、步长和行数在整个调用中保持原值。

## 7. 乘法回绕与typed-stop

与独立LST相同，大正行数可让`row_count * 4`回绕后申请过小内存。现代surface入口不在申请前拒绝：

- 先释放旧surface表；
- 按回绕字节数申请；
- 发布surface表指针；
- 发布新surface宽高；
- 写入所有仍在实际容量内的前缀；
- 只在原首次越界表项写入点返回`write_out_of_range`。

`row_count = 0x40000001`时申请4字节，第一项写零成功，第二轮EAX递增为2后在第二项写入点停止。surface宽度、巨大高度和第一项前缀均保留，主行表不受影响。

## 8. 双向追溯

LST到C++：

- `0x00433E90..0x00433EB1`映射为surface旧表释放；
- `0x00433EB1..0x00433ECF`映射为行数乘4回绕、申请及指针先发布；
- `0x00433ECF..0x00433EE5`映射为成功后的surface宽高发布与非正行门；
- `0x00433EE5..0x00433EFA`映射为指针重读、索引递增、写表和步长回绕；
- `0x00433EFA..0x00433EFC`映射为EAX残值返回和callee清理参数。

C++到LST：

- surface独占数组只对应`+0x0B44`；
- surface宽高只对应`+0x0B50/+0x0B54`；
- 共用内部核心的每个操作都在本函数LST中有同序指令；
- `allocation_failed`只对应malloc空返回；
- `write_out_of_range`只隔离原surface表写入的内存破坏域；
- 没有新增主表写入、尺寸夹值、清零或预先拒绝。

完整正向与反向追溯未发现未解释基本块、错误owner绑定或遗漏出口。

## 9. 测试

独立surface入口测试覆盖：

- 固定caller的640×480行表与EAX返回；
- surface尺寸发布后被已关闭矩形放置直接消费；
- 主行表指针、步长和行数保持不变；
- 旧surface表先释放；
- 分配失败清surface表但保留旧surface尺寸；
- 乘4回绕后的第一项前缀、第二项typed-stop与新surface尺寸。

主行表既有成功、零、负、失败和步长回绕测试继续通过，证明内部核心复用没有交叉污染。

本函数只消费整数参数与堆分配，没有适用的物理游戏资产读取。固定状态直接来自三个权威LST caller。

定向battle聚合测试通过。

## 10. 动态差分

当前没有可用原版战斗surface行表捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。该阻塞不改变独立完整LST、owner差异、回绕域、typed实现和固定caller状态已经闭环的结论。
