# 战斗宿主surface行表与全画面矩形设置 `0x00433F30`

状态：`assembly_exact`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、callee与caller

权威LST函数范围为`0x00433F30..0x00433F63`，入口`proc`至`endp`连续，没有外部`FUNCTION CHUNK`。

函数的两个callee已经独立关闭：

- `0x00433E90`：surface行偏移表释放与重建；
- `0x004342E0`：按surface边界发布绘制矩形。

本函数不再通过opaque callback表示这两个边界，typed实现直接组合两个已关闭入口，完成callee回收。

工作包记录三个直接caller、五个调用点：

- `0x004710D0`一次；
- `0x00479850`三次；
- `0x00484020`一次。

五处都先以Windows系统尺寸查询取得宽度和高度，再调用本函数。宽高属于动态宿主状态，不是游戏资产。

前三类调用点随后覆盖EAX；`0x00484020`短暂把EAX带到自身返回，但唯一上层紧接着调用其他函数覆盖，不作行为判断。typed入口仍显式保留原EAX合同。

## 2. thiscall ABI

ECX为战斗绘制对象，两个栈参数依次为：

1. signed 32位宿主surface宽度；
2. signed 32位宿主surface高度。

函数把高度保存到EBX、宽度保存到EDI、对象保存到ESI。两个参数寄存器snapshot跨越第一个callee，不在调用后从共享对象重读。

现代入口以不可变局部参数表达EDI/EBX snapshot，避免将可变对象字段误当作调用后寄存器。

## 3. callee前尺寸预发布 `0x00433F3D..0x00433F4B`

LST顺序为：

1. 压入保存的高度；
2. 压入保存的宽度；
3. 写`[this+0x0B50] = width`；
4. 写`[this+0x0B54] = height`；
5. 调用surface行表重建。

尺寸写入发生在行表旧存储释放、申请和失败判断之前。这与直接调用`0x00433E90`不同：即使后续分配失败，本caller看到的surface宽高也已经是新参数，不会保留更早尺寸。

现代组合入口先预发布`surface_width/surface_height`，再调用已关闭surface行表typed入口，保持该前缀。

## 4. 调用后寄存器与矩形参数 `0x00433F50..0x00433F58`

行表callee正常返回后，本函数没有重读对象宽高，也不消费其EAX。它直接使用跨调用保存的EBX/EDI：

1. 压入保存的高度；
2. 压入保存的宽度；
3. 压入top零；
4. 压入left零；
5. 以原对象为ECX调用矩形发布。

因此矩形参数始终是入口snapshot，不受对象字段在callee内被重写的方式影响。正常路径发布：

- left为零；
- top为零；
- right为入口宽度；
- bottom为入口高度。

即使宽度或高度为零、负值或发生32位异常域，矩形callee中的相等比较仍走原边界分支并得到同一组结果，不新增夹值。

## 5. 分配失败前缀

surface行表申请失败时，已关闭callee：

- 已释放并清空旧surface表；
- 不在自身成功块重新发布元数据；
- EAX返回零。

但本caller在callee前已经发布新宽高，并忽略该EAX，所以仍继续调用矩形发布。最终状态为：

- surface表为空；
- surface宽高为新入口参数；
- 矩形为`0,0,width,height`；
- 本函数EAX为height。

typed测试用可注入失败分配器证明旧表在申请前清空，同时锁定新尺寸和矩形后缀。

## 6. 原越界写域的typed-stop传播

若`row_count * 4`回绕导致surface表容量过小，原函数会在`0x00433E90`的表项写指令破坏内存，不能证明会返回到`0x00433F30`执行矩形。

现代组合入口保留：

- caller的尺寸预发布；
- surface旧表释放；
- 回绕申请；
- surface表指针及新尺寸发布；
- 已写表前缀；
- 到达原越界写入点时的EAX行索引。

随后返回`rectangle_published = false`，不伪造原来无法到达的矩形副作用。`legacy_return_value`在该typed-stop结果中保留callee到达写点时的EAX；它不是原函数的正常返回。

## 7. 正常返回 `0x00433F58..0x00433F63`

矩形callee返回EAX等于bottom。本函数只恢复EDI、ESI、EBX并`retn 8`，不再修改EAX。

所以所有能从两个callee正常返回的路径最终EAX都等于入口height，包括：

- 行表完整填充；
- 行表申请失败；
- 成功分配但行数为零；
- 成功分配但行数为负。

现代`LegacyBattleHostSurfaceResult`分别记录行表结果、矩形是否发布及最终EAX，避免把callee内部状态与caller正常返回混为一类。

## 8. 双向追溯

LST到C++：

- `0x00433F30..0x00433F3D`映射为this、宽度、高度snapshot；
- `0x00433F3D..0x00433F4B`映射为参数压入、尺寸预发布和typed surface行表调用；
- `0x00433F50..0x00433F58`映射为保存参数组成`0,0,width,height`并直接调用typed矩形；
- `0x00433F58..0x00433F63`映射为矩形bottom透传到EAX及callee清理参数。

C++到LST：

- 尺寸预发布对应两条caller对象写指令；
- surface行表调用对应已关闭的唯一callee；
- `rectangle_published = false`只隔离callee原越界写破坏域；
- 分配失败继续矩形对应callee正常零返回后的顺序执行；
- 全surface矩形参数对应四条push；
- 最终EAX对应矩形callee残值；
- 没有额外重读、失败提前返回、尺寸夹值或opaque回调。

完整正向与反向追溯未发现未解释指令、未回收callee或遗漏出口。

## 9. 测试

定向测试覆盖：

- 1920×1080固定宿主状态的1080项行表、最后一行偏移、全surface矩形与EAX；
- 分配失败后的新尺寸预发布、空表、全surface矩形和height返回；
- 乘法回绕只写第一项后typed-stop，且旧矩形字段保持不变；
- 负宽高成功分配时两个callee仍发布原负尺寸和height返回；
- 既有主表、surface表和矩形独立入口测试继续通过。

本函数没有适用的物理游戏资产读取。固定状态测试模拟五个caller提供的宿主尺寸；系统尺寸本身由后续平台caller适配负责。

定向battle聚合测试通过。

## 10. 动态差分

当前没有可用原版战斗绘制对象与系统尺寸联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。该阻塞不改变完整LST、两个已关闭callee的直接回收、失败前缀、typed-stop传播和固定状态已经闭环的结论。
