# 战斗图像粒子整帧协调 `0x00434790`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST函数范围为`0x00434790..0x00434DC3`，入口`proc`至`endp`连续，共719行，没有外部`FUNCTION CHUNK`。

ABI为thiscall：

- ECX：战斗绘制owner，提供surface行表、宽和高；
- 第一个栈参数：u16输出surface；
- 第二个栈参数：图像粒子owner。

三个直接caller为`0x00471270`、`0x00479850`和`0x004841B0`。三处都传共享战斗surface与各自对象内粒子owner，并显式比较EAX是否为1；返回1会推进或结束对应战斗动作，返回0继续当前阶段。

callee：

- `0x00434DD0`：已关闭的图像粒子生成，一次；
- `0x00434350`：已关闭的线段光栅单步推进，一次循环调用点；
- `0x004207E0`：已关闭的单像素通道溢出清零合成，六个调用点；
- `0x004885A0`：旧分配器释放，两个调用点；
- `0x00489B20`：固定CRT随机数，一处；
- `0x00489B10`与`0x00489B70`：仅首次初始化时以当前时间seed CRT。

现代实现直接组合两个battle callee、`LegacyCrtRng`、公共颜色合成与token节点池，不复制线段或颜色算法。

## 2. 首次初始化 `0x0043479A..0x0043484E`

owner `+0x38 == 0`时：

1. 取得当前time值并seed CRT；
2. 按顺序把owner `+0x2C/+0x30/+0x34`发布到三个共享渲染值；
3. `source_pixel_count = wrapping(unsigned_width * unsigned_height)`；
4. `spawned_count = 0`；
5. `initialized = 1`；
6. `nontransparent_pixel_count = 0`；
7. 若回绕总数为正，按源指针顺序读取该数量的u16，排除两种共享透明色后递增非透明数；
8. `target_particle_count = signed(nontransparent_pixel_count / 20)`；
9. `shared_modulus_increment = signed(source_pixel_count / unsigned_remaining_batches)`。

源像素总数使用16位宽高零扩展后的32位低位乘积。若乘积成为负数，原`signed jle`跳过源扫描，但仍计算目标数和每批增量。

现代实现由调用方提供time seed，避免在核心库读取平台时钟；seed时机与后续随机状态不变。

源span不足只在顺序u16读取点返回`initialization_source_out_of_range`，保留seed、三项共享发布、初始化标志和此前非透明计数。剩余批次为零只在最后signed `idiv`位置返回`initialization_batch_divisor_zero`。

已初始化owner完全跳过time、共享发布、总数和扫描。

## 3. 生成门与callee回收 `0x0043484E..0x0043487D`

剩余批次数大于零时：

```text
spawn_threshold = nontransparent_pixel_count / unsigned_spawn_divisor
if spawned_count < spawn_threshold:
    attempt_count = source_pixel_count / unsigned_remaining_batches
    call closed particle spawn
```

两个除法都是signed `idiv`，除数来自u16零扩展。spawn divisor为零在第一除法点返回`spawn_divisor_zero`。

已关闭`0x00434DD0`的所有状态直接映射为`spawn_failed`并保留具体`spawn_status`。生成成功产生的源清除、空后继、批次减一和共享随机模数立即供本函数后续使用，不保留opaque callback。

## 4. 返回1完成门

生成门之后先检查：

```text
spawned_count <= 0 && remaining_batches == 0
```

成立就直接返回EAX=1，不回放源图、不读取粒子链。即使`head`仍指向生成函数留下的空节点，也保持不触碰。

其他路径最终返回EAX=0。

## 5. 剩余批次期间的源图回放

只有`remaining_batches >= 1`才回放源图；行数固定为`unsigned_height - 1`。最后一行永远不处理。

每个候选像素先做surface边界检查，越界时不读取源图、行表或目标。

### 5.1 普通模式

每行源索引按`0..width-1`递增：

```text
checked_x = written_x = source_origin_x + column
source_index = row_base + column
```

### 5.2 bit0镜像模式的错位

原LST同时使用两个不同X：

```text
mirrored = width - column
checked_x = source_origin_x + mirrored
source_index = row_base + mirrored
written_x = source_origin_x + column
```

没有`-1`。第一列读取下一行首像素，却在普通第一列位置写入；边界检查也是镜像X，不是实际写入X。现代实现分别保存`checked_x`和`written_x`，不把它们合并修正。

## 6. 回放像素写入

目标边界通过后才读取源u16。两种共享透明色均跳过。

flags低字节与`0x16`：

- 为零：直接写目标；
- 非零：调用已关闭单像素通道溢出清零合成。

目标地址按surface行表值加实际写入X，32位回绕。行表span和目标span分别只在原u32读取或u16读写点返回`row_table_out_of_range`与`destination_out_of_range`，保留此前回放前缀。

## 7. 粒子链入口与生命刷新

源图回放后从owner `+0x50`遍历双向链。空head直接返回0。

每个节点先比较：

```text
spawned_count <= target_particle_count
```

成立时先消费一次CRT随机数，再按：

```text
node.random_lifetime = random % unsigned_lifetime_divisor
                             + unsigned_lifetime_divisor
```

写入范围为`divisor..2*divisor-1`，不是生成阶段的`remainder+1`。除数为零只在random已消费后的原`idiv`点返回`lifetime_divisor_zero`。

published head或后继token无效时不在遍历入口统一预检；若生命刷新分支成立，先消费random，再在节点`+0x08`写入点停止。其他情况在节点`+0x08`读取点停止。

## 8. 线段推进与剩余距离

节点`+0x10..+0x2C`与已关闭32字节线段记录完全同构：起点、终点、当前坐标和X/Y误差。

`random_lifetime > 0`时严格调用该次数的线段单步typed入口；返回值被忽略。全部步进完成后发布回同一节点。

随后：

```text
distance_offset = wrapping(distance_offset - random_lifetime)
```

结果小于等于零就摘除节点。

否则仅在当前X/Y都不小于随机目标矩形左上时检查右下：

- 当前X达到`wrapping(target_origin_x + target_width)`：摘除；
- 当前Y达到`wrapping(target_origin_y + target_height)`：摘除；
- 当前X或Y还在左/上方：不摘除，继续尝试画到surface。

## 9. 2×2粒子的非对称surface边界

上、下两行都只用`current_x + 1`检查水平边界：

```text
0 <= current_x + 1 < surface_width
```

左像素的`current_x`不单独检查。因此`current_x == -1`时边界门通过，随后左像素目标地址在原写点越界。现代实现不提前检查左X，只在实际目标span访问返回`destination_out_of_range`。

上行检查当前Y；下行检查`current_y + 1`。每行整体越界时跳过该行两个像素。

保存的四像素渲染只比较第一透明色`0x319F`。第二透明色`0x026B`不会跳过，会作为普通像素写入。这与源图回放的双透明门不同。

## 10. 直接模式

flags与`0x16`为零时：

- 左上、右上分别在非第一透明色时直接写；
- 左下、右下分别在非第一透明色时直接写；
- 每次写入独立读取行表并计算目标。

## 11. 合成模式与右上覆盖BUG

flags与`0x16`非零时：

- 左上非透明：单像素合成；
- 右上非透明：先单像素合成；
- 随后无论右上是否为第一透明色，都把保存的右上源u16直接写到同一目标；
- 左下和右下非透明：只做单像素合成。

因此右上合成结果立即被原值覆盖；若右上是`0x319F`，仍直接把透明色写入目标。现代实现保留合成调用的颜色状态副作用和随后的直写，不把它简化成一次直写。

## 12. 不摘除节点的推进

两行绘制完成、部分跳过或完全越界后，都读取当前节点`next`继续。下行右下合成成功路径在LST中提前装载next再跳公共循环；其他路径在公共块装载，结果相同。

## 13. 四类摘链

摘除节点时snapshot `previous/current/next`，分四类：

1. 唯一节点：`previous == 0 && next == 0`；
2. 首节点：`previous == 0 && next != 0`；
3. 尾节点：`previous != 0 && next == 0`；
4. 中间节点：前后均非零。

### 13.1 唯一节点

按顺序清owner head、tail、spawned_count，释放节点并立即返回0。**不递减**`nontransparent_pixel_count`。

### 13.2 首节点

先写`next.previous = 0`，再发布owner head=next。

### 13.3 尾节点

先写`previous.next = 0`，再发布owner tail=previous。

### 13.4 中间节点

先写`previous.next = next`，再写`next.previous = previous`。若第二个邻接token损坏，现代typed-stop保留已经完成的第一项链接写。

后三类随后：

1. 保存next；
2. 释放current；
3. `spawned_count`按32位回绕减一；
4. `nontransparent_pixel_count`按32位回绕减一；
5. 从next继续遍历。

现代token池只替代裸指针释放；四类链接、发布和计数顺序不变。

## 14. 故障与typed-stop位置

现代状态严格落在原操作点：

- 初始化源短读；
- 初始化剩余批次除零；
- 生成阈值除零；
- 已关闭生成callee失败；
- 回放源短读；
- surface行表短读；
- 目标像素越界；
- 节点或邻接token无效；
- 生命刷新除零；
- 单像素颜色合成失败。

停止前的seed、共享发布、计数、random消费、源清除、surface写入、节点字段、单侧链接和已释放前缀全部保留；不回滚整帧，也不伪造返回1。

## 15. 双向追溯

LST到C++：

- `0x0043479A..0x0043484E`映射为首次seed、共享发布、源扫描和三项派生计数；
- `0x0043484E..0x00434898`映射为生成门与返回1；
- `0x0043489B..0x00434A7F`映射为两种源图回放；
- `0x00434A7F..0x00434AE8`映射为链入口、生命刷新、线段推进和距离减法；
- `0x00434AEE..0x00434B28`映射为目标矩形摘除条件；
- `0x00434B28..0x00434D20`映射为2×2直接/合成绘制及非对称边界；
- `0x00434D25..0x00434D90`映射为首/尾/中间摘链、释放和双计数递减；
- `0x00434DA4..0x00434DBA`映射为唯一节点清理；
- 三个出口分别对应完成1、普通0和唯一节点0。

C++到LST：

- 所有owner字段、surface值和节点位移均有明确LST来源；
- time只在首次初始化转为显式seed；
- 粒子生成、线段推进和颜色合成直接调用已关闭typed入口；
- 回放镜像检查X、读取X和写入X分别对应原寄存器；
- 粒子第二透明色、右边界、右上覆盖和唯一节点计数不对称全部保留；
- 每个token/span检查仅替代原实际裸访问；
- 没有新增插值、随机重试、边缘裁剪、颜色修正、链排序或粒子上限。

完整正向与反向追溯未发现未解释基本块、外部chunk、callee、随机消费或出口。

## 16. 测试

定向测试覆盖：

- 首次time seed、三项共享发布、总像素、非透明数、目标数和每批增量；
- 初始化短源与批次数零除前缀；
- 生成阈值零除及生成callee状态传播；
- 普通回放只处理`height-1`行；
- bit0镜像读取与普通列写入错位；
- 回放直接写和通道溢出清零合成；
- 回放短源、短行表和损坏行偏移；
- 生成成功后直接处理空后继；
- 生命刷新random消费与线段一步推进；
- 2×2直接模式只跳第一透明色；
- 合成模式右上先合成后直写；
- `current_x=-1`通过右像素边界后在左目标写点停止；
- 唯一、首、尾和中间四类摘链；
- 唯一节点保留非透明计数，其他摘链递减双计数；
- 无效head和生命除零的原位停止；
- 返回1早于陈旧head访问。

本函数消费运行时源图、节点链和surface，没有独立物理游戏资产文件。定向battle聚合测试通过，目标构建零warning。

## 17. 动态差分

当前没有可用原版战斗粒子owner、time/CRT状态、节点链、源图与surface联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。该阻塞不改变完整719行LST、两个battle callee直接回收、全部像素/链非对称、typed实现和固定状态已经闭环的结论。
