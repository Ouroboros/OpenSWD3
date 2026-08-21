# 剧情 VM 地图角色写入 `0x004296DE`

## 结论

`sub_427920` 的 opcode62 是一条固定 18 字节的 MAPS 角色源记录写入指令。它先按角色选择器清理现有运行角色，再调用 `sub_40D460` 修改 22 字节 MAPS 角色源；仅当修改后的目标地图等于当前地图时，才即时物化、替换或追加一个 `0xD8` 字节运行角色。完成路径推进 18 字节、令 `ESI=1`，经共同出口发布归一化 previous 后在同一次解释器调用继续取指。

现代实现分类为 `platform_adapted`：MAPS payload、MAPS 数据库、角色 vector、空间索引、地表网格与四槽角色粒子 effect 都由 typed owner 代替原版进程全局和裸指针；有效域的字段宽度、回绕、顺序、同调用继续与原版四槽全填 bug 保持不变。

唯一行为依据是 `swd3.exe.lst` 的机器码和指令。现有 C++、旧语义表与相邻 handler 只用于 REVIEW，不作为本 handler 的完成依据。

## 指令布局

| 偏移 | 原版读取 | 含义 |
| --- | --- | --- |
| `+0` | `u16`，dispatch 取低 14 位 | opcode62 |
| `+2` | `u16` | 角色选择器；`0xFFF0` 改用 Talk context `+0x24` 的 source GUID |
| `+4` | `u16` | 目标 logical map；`0xFFFF` 改用当前 map |
| `+6` | `u16` | path data id；交给 `sub_40D460`，`0xFFFF` 保留源字段 |
| `+8` | `u16` | tile X；`0xFFFF` 改用受控角色 `world_x >> 4` 的低 16 位 |
| `+10` | `u16` | tile Y；`0xFFFF` 改用受控角色 `world_y >> 4` 的低 16 位 |
| `+12` | `u16` | action id；`0xFFFF` 保留源字段 |
| `+14` | `u16` | base variant；`0xFFFF` 保留源字段 |
| `+16` | `u16` | variant delta；`0xFFFF` 保留源字段 |

X/Y 的继承源是受控角色，不是选择器命中的旧目标角色。path/action/base/variant 的 `0xFFFF` 由 `sub_40D460` 逐字段解释；不能把八个参数归并为同一种 sentinel 规则。

## 旧运行角色清理顺序

入口先读取 `+2`，完成 `0xFFF0` 替换并调用 `sub_40C0D0`。命中运行角色时，后续顺序固定为：

1. 遍历 72 个 `0x21C` 活动对象槽；对象首 word 等于命中角色索引时调用 `sub_40DD40`，全部匹配项都重置；
2. 保存角色 flags 的低 16 位；
3. 只清低 16 位的 bits14/15，即完整 flags 执行 `&= 0xFFFF3FFF`；
4. 调用 `sub_40AE20` 清角色地表占用；
5. OR flags bit28 `0x10000000`；
6. 保存角色 `talk_script_id`。

未命中角色时，继承的 flags OR mask 为 0，Talk id 为 `0xFFFF`。旧角色清理发生在其余 14 字节参数读取之前；因此 `+2` 可读而后续截断时，对象重置、flags 修改和地表清理已经发生，IP 与 previous 尚未发布。

## `sub_40D460` 十一参数

调用按以下字段写 MAPS 源记录：

```text
guid             = resolved selector
action_id        = +12
base_variant     = +14
variant_delta    = +16
tile_x           = +8 或受控角色 X>>4
tile_y           = +10 或受控角色 Y>>4
talk_script_id   = 旧运行角色 Talk id，未命中则 FFFF
path_data_id     = +6
flags_or_mask    = 旧运行角色 flags 低16，未命中则 0
flags_and_mask   = FFFF（helper sentinel，保持不改）
logical_map_id   = +4 或当前 map
```

`sub_40D460` 先按 GUID 扫描 22 字节记录，逐字段跳过 `0xFFFF`；path id 被实际写入时同时把源 `path_word_index` 清零。flags 先 AND、后 OR，logical map 最后写。GUID 缺失只产生 diagnostic 并返回 0；opcode62仍推进、发布 previous 并同调用继续。

## 当前地图的临时角色物化

MAPS patch 成功后，handler 比较目标 map 与当前 map。不同地图只保留 MAPS 修改；相同地图按下列顺序继续：

1. `_malloc(0xD8)`，随后 `rep stosd` 清零 54 个 dword；
2. `sub_40D560` 从刚修改的 MAPS 源复制 GUID、动作三字段、整格坐标、Talk、Path 和 flags；运行角色 path cursor 固定清零；
3. `sub_40F280` 先清动作记录 `+0x18`，调用 `sub_4321E0` 更新动作；返回 0 只诊断，后续继续；
4. flags 执行 `&= 0xDF0FFFFF`；按 `(world_y >> 4) * map_width + (world_x >> 4)` 建立地表单元引用；
5. flags bit8 置位时，从地表 dword 映射 bit11 到角色 bit29，并把地表 bits12..15 映射到角色 bits20..23；
6. 更新后的 action id 非零且 `(flags & 0x8400) == 0x8000` 时，立即调用 `sub_40AEC0` 标记地表占用。

现代运行角色把原版地表裸指针适配为单元索引；32 位乘加仍按 `u32` 回绕。动作更新失败只计数，不阻断物化。

## 替换、追加与空间链

临时角色准备完成后，原版从运行角色索引 1 开始按 GUID 直接搜索，不使用 lookup skip bit：

- 命中时，以**临时角色新 flags 的低两位**作为 group，调用 `sub_411530(guid, group, 0, 1)` 摘链；使用 helper 返回的实际角色位置覆盖完整 `0xD8` 记录，再调用 `sub_411490` 按同一 group 插回；
- 未命中时，把完整临时记录复制到当前 role count 位置，调用 `sub_411490`，成功后才递增 role count。

现代 vector 追加后立即刷新本次 VM 和 story-path runtime 的 span，避免重分配留下悬空视图。容量、分配、owner、损坏空间链和插入失败属于原版未防护域，现代在对应阶段 typed-stop；已经完成的 MAPS、旧角色、动作或地表副作用不回滚。

## flags bit9 与四槽角色粒子记录

最终运行角色 flags 含 `0x0200` 时，handler 遍历 `0x004CACE0` 起四个 16 字节 emitter。比较字段是每槽 `+0x0C` 的 role selector；值为 0 时写：

- `+0x0C`：角色 GUID 低 word；
- `+0x04/+0x06`：角色 world X/Y 低 word；
- `+0x08/+0x0A/+0x0E`：0。

槽首 `+0x00` 的粒子链 head 不清。循环没有 `break`，所以会把**所有空槽**都写成同一角色，而不是只占一个槽；非空槽保持原值。现代 `LegacyAniRoleParticleEffect::emitters()` 保留这一行为和 head token。

## IP、previous 与边界

正常、目标地图不同和 GUID 缺失三路都汇合到：

```text
IP += 18
ESI = 1
common join publishes normalized previous opcode 62
same-call fetch
```

因此 opcode62本身不跨帧让出。完整记录恰好结束于 `0x8000` 时，MAPS/角色/粒子副作用、IP 和 previous 均先完成，下一次取指才返回窗口越界。仅 `+2` 可读的截断则先完成旧角色清理，再在其余参数边界 typed-stop，IP/previous 不变。

## 真实资产锁

对 `story-vm-talk-linear-records.tsv` 的全部 opcode62 entry 逐条回读原始 TALK 文件：

- 共 443 条，TALK1/2/3/4 分布 `77/59/128/179`；
- 443/443 均为 raw `0x003E`、长度 18、单一 entry probe；偏移和原始 word 校验零错误；
- selector 114 种，真实记录中没有 `0xFFF0/0xFFFF` selector；
- map `0xFFFF` 124 条；tile X/Y `0xFFFF` 各 16 条；
- path id 没有 `0xFFFF`，418 条为 0；
- action/base/variant 的 `0xFFFF` 分别为 `59/8/11` 条。

真实回放使用 `TALK1.DAT@0x00005B5D`：GUID 248、map 81、path 0、tile `(26,26)`、action/base/variant `(623,0,4)`；MAPS 源写入后同调用进入下一条等待指令。

## 测试覆盖

- 四种 raw alias 全部归一化为 opcode62；
- 非当前地图只修改 MAPS，不物化运行角色；
- GUID 缺失 diagnostic-only 路径仍推进并同调用取下一条；
- `0xFFF0` selector、map/坐标/path 的多重继承和 action/base/variant 字段分别覆盖；
- 旧角色 72 槽清理、flags bits14/15 清除、bit28 置位、Talk/低 flags 继承及先于动作更新的顺序；
- 当前地图同 GUID 替换与未命中追加、空间摘链/重插、vector span 刷新；
- 动作更新失败只记录不阻断；地表 bit/nibble 映射与占用写入；
- 粒子非空槽跳过、所有空槽全填、坐标低 word、零字段和 head token 保留；
- `+2` 后截断保留旧角色部分副作用，MAPS owner 缺失在清理后 typed-stop；
- `0x7FEE` 精确尾先完成 publication，再由下一取指返回 `instruction_out_of_range`；
- TALK1真实记录回放；剧情 VM 定向三项测试全部通过。

## 双向收敛与分类

LST→实现 REVIEW 修正了一处初版差异：opcode62不得直接 yield，必须设置同调用继续。修正后再次逐段对照 `0x004296DE..0x00429A16`、`sub_40D460`、`sub_40D560`、`sub_40F280`、空间 helper 与四槽 emitter 布局，未发现剩余有效域差异。

分类：`platform_adapted`。原版可运行动态差分仍为 `blocked_runtime_oracle`；静态 LST、typed owner、真实资产和现代运行时测试已闭环。
