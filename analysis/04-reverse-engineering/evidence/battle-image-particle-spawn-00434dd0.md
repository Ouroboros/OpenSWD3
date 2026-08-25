# 战斗图像非透明像素粒子生成 `0x00434DD0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、caller与外部边界

权威LST函数范围为`0x00434DD0..0x004350D9`，入口`proc`至`endp`连续，共351行，没有外部`FUNCTION CHUNK`。

唯一直接caller为`0x00434790`。caller在初始化源像素总数、非透明数量、批次数和共享增量后，把战斗图像粒子owner与本批尝试次数传入；返回值不用于分支。

本函数调用：

- `0x00487C10`两处：按56字节申请节点；
- `0x00489B20`十处：项目已关闭的CRT线性同余随机数入口；
- `0x00489654`一处：把x87栈顶正平方根按截断模式转换成64位整数并返回低32位；
- `MessageBoxA`两处：初始节点和后继节点申请失败的诊断。

现代实现直接复用`LegacyCrtRng`，用typed 56字节节点池表达旧分配器，用精确整数平方根表达正数`fsqrt`后截断结果，并把平台消息框事件记录为两类诊断计数。没有复制或替换CRT随机序列。

## 2. owner字段

完整LST读取或写入：

- `+0x00`：可修改的u16源图像；
- `+0x04`：unsigned 16位源宽度；
- `+0x08/+0x0C`：源图像目标原点X/Y；
- `+0x10/+0x14`：随机目标X原点与signed范围；
- `+0x18/+0x1C`：随机目标Y原点与signed范围；
- `+0x20`：unsigned 16位距离余数基值；
- `+0x22`：unsigned 16位生命随机除数；
- `+0x24`：unsigned 16位剩余批次数；
- `+0x28`：低字节模式标志；
- `+0x3C`：signed 32位源像素总数；
- `+0x40`：已接受非透明粒子数；
- `+0x4C`：共享随机模数增量；
- `+0x50`：粒子链首节点；
- `+0x54`：已有链继续写入节点。

入口先snapshot `+0x00/+0x08/+0x0C`，后续源访问和普通坐标使用该snapshot。现代接口显式保存这三个入口值。

## 3. 56字节粒子节点

节点布局：

- `+0x00/+0x02/+0x04/+0x06`：摘取的2×2四个u16源像素；
- `+0x08`：`random % owner[+0x22] + 1`；
- `+0x0C`：`random % floor(distance) + owner[+0x20]`；
- `+0x10/+0x14`：普通模式源X/Y；
- `+0x18/+0x1C`：随机目标X/Y；
- `+0x20/+0x24`：普通模式初始当前X/Y；
- `+0x28/+0x2C`：本函数保持申请后的零值；
- `+0x30/+0x34`：前驱与后继节点。

`LegacyBattleImageParticleNode`用静态断言固定总长`0x38`及所有关键位移。token池仅替代裸指针所有权，不改变链写入顺序或零初始化。

## 4. 共享随机模数与初始节点

全局共享随机模数为零时，函数先写入owner `+0x4C`；非零时保持原值。

随后：

- `+0x50 == 0`：申请56字节、失败时先报告初始申请诊断；成功时清零十四个dword并发布为`+0x50`；
- `+0x50 != 0`：直接把`+0x54`作为当前写入节点，不提前验证。

原初始申请失败在消息框后仍对空指针执行`rep stosd`。现代实现在该原空写点返回`initial_allocation_failed`，保留共享模数seed和诊断，不伪造首节点或批次收尾。

已有首节点但`+0x54`无效时也不在入口提前停止。透明像素可完全绕过节点访问；只有第一个实际节点字段写入才返回`current_node_out_of_range`。

## 5. 尝试次数门

初始节点建立发生在尝试次数判断之前。

- `attempt_count <= 0`：不消费随机数、不读取源像素，但仍执行批次收尾；
- 正数：严格执行该次数的尝试；
- 透明尝试和成功尝试都在公共尾部计数；
- 后继申请失败及其他typed-stop发生于公共尾部之前，不把当前尝试计为完成。

## 6. 每次选择的前两个随机数

每次尝试固定先调用两次CRT RNG：

```text
r = first_random signed-rem 28
product = wrapping(second_random * r)
```

两个随机值均为0..32767，`r`为0..27；实现仍按原signed指令表达。

低字节flags按顺序选择源索引模数：

1. bit6置位：`product signed-rem shared_random_modulus`；
2. 否则bit7置位：先计算`source_pixel_count / unsigned_remaining_batches`，再让`product`对该商取signed余数；
3. 否则：`product signed-rem source_pixel_count`。

bit6优先于bit7。两位同时置位时不会读取批次数或每批商。

除数为零分别在原`idiv`位置返回：

- `shared_modulus_zero`；
- `remaining_batch_divisor_zero`；
- `per_batch_modulus_zero`；
- `source_pixel_count_zero`。

所有状态都已消费本次最前两个随机数，并保留入口初始节点与共享seed。

## 7. 首像素、透明门与计数

选择结果按32位地址回绕解释为u16源索引，在原`[source + index*2]`读取点检查span。

- 越界：`source_pixel_out_of_range`；
- 等于共享RGB555特殊色`0x319F`或`0x026B`：直接进入尝试公共尾部，不读取当前节点；
- 其他值：先把owner `+0x40`按32位回绕加一，再计算节点坐标。

因此无效`+0x54`在两种透明色下均可完成；非透明值则保留已递增计数后才在首节点写入点停止。

## 8. 源坐标与flags优先级

非透明像素的坐标分支顺序：

1. bit0置位；
2. 否则bit7置位；
3. 否则普通模式。

bit0置位时使用unsigned `div source_width`，并写：

```text
source_x = source_origin_x + source_width - column
source_y = source_origin_y + row
```

这里没有常见的`-1`；镜像列零得到`origin + width`。bit0同时覆盖bit7坐标分支。

普通模式写：

```text
source_x = source_origin_x + column
source_y = source_origin_y + row
```

两种普通坐标都会同步写入节点`+0x10/+0x14/+0x20/+0x24`。源宽为零只在原unsigned `div`位置返回`source_width_zero`，此前`+0x40`增量保留。

bit0未置且bit7置位时完全跳过这四个节点字段写入。

## 9. bit7的旧局部BUG

入口进入正尝试循环时，EBP保存`attempt_count`。普通坐标分支随后把EBP改为本次源X，并把源Y写入栈局部`var_1C`。

bit7坐标跳过路径既不改EBP，也不初始化`var_1C`，但后续距离仍执行：

```text
dx = target_x - EBP
dy = target_y - var_1C
```

因此首个纯bit7尝试使用：

- X基准=`attempt_count`；
- Y基准=入口栈上的陈旧`var_1C`。

现代`LegacyBattleImageParticleStackSnapshot::stale_source_y`显式携带该旧局部，不擅自改成零、源原点或节点坐标。定向测试令目标恰等于`attempt_count/stale_source_y`，证明平方距离走原零值改一分支。

## 10. 随机目标与距离字段

目标X和Y各再消费一次RNG：

```text
target_x = target_origin_x + random signed-rem target_width
target_y = target_origin_y + random signed-rem target_height
```

signed范围可为负；只对零除数分别在原点返回`target_width_zero`和`target_height_zero`。目标X已写后遇到目标Y零除数时，X前缀保留。

距离平方按32位低位计算：

```text
square = wrapping(dx*dx + dy*dy)
if square == 0: square = 1
```

原代码把该u32放入高dword为零的qword，以x87 `fild/fsqrt`求正平方根，再由`0x00489654`按向零截断。现代整数平方根对0..`UINT32_MAX`给出完全相同的floor。

函数在求根前已消费下一次RNG，随后以unsigned余数加`+0x20`写节点`+0x0C`。再消费一次RNG，以`+0x22`为signed正除数取余并加一写`+0x08`；零除数在该处返回`lifetime_divisor_zero`，保留目标和距离字段。

## 11. 2×2源块读取与清除

完成所有随机字段后，按原顺序读取：

1. `index`；
2. `index + 1`；
3. `index + source_width`；
4. `index + source_width + 1`。

四项逐一写节点`+0..+6`。地址加法按u32回绕。源宽为0或1时索引会重叠，仍先完成四次读取，再开始任何源写入。

任一读取越界只在该次原u16读取点返回`source_pixel_out_of_range`，保留已写节点前缀，且不清除任何源像素。

四次读取都成功后，再按相同顺序把源位置写为第一透明色`0x319F`。重叠索引可被重复写，但最终结果不变。

## 12. 空后继节点与申请失败

每个成功初始化并清除源块的粒子都会再申请一个56字节空后继：

1. 先把申请结果写当前节点`+0x34`；
2. 空结果先报告后继申请诊断，再直接返回，不执行尝试计数或批次收尾；
3. 成功结果清零十四个dword；
4. 写后继`+0x30 = current`、`+0x34 = 0`；
5. 当前局部节点改为后继；
6. owner `+0x54`按LST写为后继节点的`+0x34`，即零。

最后一项看似不符合常规tail设计，但它是`0x00435073..0x00435076`的确定行为。现代实现保留零发布，不替换为新后继token。

后继申请失败状态保留：已递增`+0x40`、完整粒子字段、四个源清除、当前`next=0`和诊断；剩余批次及共享模数不做正常收尾。

## 13. 批次收尾

全部尝试完成或入口尝试次数非正时：

1. unsigned 16位`remaining_batches`减一并回绕；
2. 共享随机模数加owner `+0x4C`，32位回绕；
3. 若减一后批次数等于零，则共享模数清零；
4. 若原批次数为零，减一得到`0xFFFF`，unsigned `ja`成立，共享模数不清零。

任何除法、源、节点或分配typed-stop都不伪造该正常收尾。

## 14. 双向追溯

LST到C++：

- `0x00434DD0..0x00434E3C`映射为共享seed、入口snapshot与初始/已有节点选择；
- `0x00434E3C..0x00434EC7`映射为次数门、两次RNG和三种索引模数；
- `0x00434EC7..0x00434F54`映射为首源读取、双透明门、计数及三种源坐标；
- `0x00434F54..0x00434FCA`映射为目标、旧局部距离、平方根和距离余数；
- `0x00434FCA..0x00435045`映射为生命值、四源读取和四源清除；
- `0x00435045..0x00435079`映射为空后继申请、双向链和零尾发布；
- `0x00435079..0x004350B4`映射为尝试尾部和批次收尾；
- `0x004350BE..0x004350D9`映射为后继申请失败诊断与直接返回。

C++到LST：

- 每个owner和节点字段都有确定偏移来源；
- 十次可能RNG调用严格只出现在对应分支；
- 每个signed/unsigned除法和优先级均对应原指令；
- stale Y由显式snapshot表达，不修复原未初始化读取；
- node pool token仅替代裸指针，链写序和空后继不变；
- 每个typed-stop都落在原除法、源读、节点写或空分配使用点；
- 没有新增尝试上限、随机重试、透明跳过补偿、坐标夹值或源块边缘裁剪。

完整正向与反向追溯未发现未解释基本块、外部chunk、随机消耗或返回出口。

## 15. 测试

定向测试覆盖：

- 正常模式单粒子的固定CRT序列、源/目标/当前坐标、距离和生命字段；
- 源宽1时2×2索引重叠的四像素保存与清除；
- bit0镜像不减一；
- bit7选择、入口attempt X与旧Y snapshot；
- bit6同时覆盖bit7选择；
- 原批次数零的`0xFFFF`回绕；
- 尝试次数零仍建立首节点并执行批次收尾；
- 初始申请失败和后继申请失败的不同前缀与诊断；
- shared、remaining、per-batch、source-count、source-width、target-X、target-Y和lifetime全部零除位置；
- 两个透明色均绕过无效当前节点；
- 无效尾节点只在首节点字段写入停止；
- 2×2短源保留已写节点字段且完全不清源；
- 后继前驱、空后继和owner零尾发布；
- 前两个随机数在选择除零前已经消费。

本函数只消费caller已持有的内存图像和运行时节点，没有独立物理游戏资产文件。定向battle聚合测试通过，目标构建零warning。

## 16. 动态差分

当前没有可用原版战斗粒子owner、CRT状态、旧栈局部与源图像联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。该阻塞不改变完整LST、随机序列、旧局部BUG、typed实现和固定状态已经闭环的结论。
