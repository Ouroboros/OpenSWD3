# 剧情 VM 角色 Path id 修改边界（0x00428713）

## 结论

`sub_427920` 的一级分派入口 `0x00428713` 是 effective opcode 28 的独立 handler。记录固定为 6 字节：

```text
+0  u16 raw opcode
+2  u16 role selector
+4  u16 path data id
```

该 handler 不把 `0xFFF0` 替换为当前剧情来源角色。角色存在时，它先释放旧 Path 动态载荷并协调至多一个匹配的 active object，再写入新 Path id；角色不存在时，它只向 MAPS source 提交 Path id 与 `0x1000` flag patch。两支都经 `0x0042BEE9` 推进 6 字节，以 `ESI=0` 发布 previous opcode，执行 `_AIL_serve`，并跨帧让出。

现代实现以 typed payload owner、surface owner、spatial index 和 MAPS patch port 复现有效域行为；原程序在损坏方向、不可达对齐或断裂空间链上的越界/死循环风险改为显式 checked failure，因此闭环分类为 `platform_adapted`。

## Handler 边界与 staged operand 读取

入口 `0x00428713` 先读取 `+2` selector，再调用 `sub_40C0D0`：

```asm
00428713  mov  dx, [ebx+2]
00428717  lea  ecx, [esp+var_4C]
0042871B  push ecx
0042871C  push edx
0042871D  call sub_40C0D0
00428725  test eax, eax
00428727  jz   004288C7
```

`+4` Path id 并未在入口统一读取：

- live-role 分支先完成 payload 释放、object 协调和可能的坐标/空间副作用，到 `0x004288AA` 才读取 `[instruction+4]`；
- missing-role 分支到 `0x004288DA` 才读取 `[instruction+4]`，随后才调用 `sub_40D460`。

因此现代 VM 不能预先把 6 字节整体校验为无副作用事务。尾部截断时，live-role 分支已经发生的 payload/object 副作用必须保留；missing-role 分支则不能提前发布 MAPS patch。

最后一条 missing-role 近跳从 `0x0042890A` 开始并结束于 `0x0042890E`。下一个独立 handler 是 `0x0042890F`，所以本边界严格为半开区间 `0x00428713..0x0042890F`。

## Live-role 分支

### 旧 Path 动态载荷

角色索引按 `index * 0xD8` 定位。`role+0x38` 非零时：

1. 将旧指针传给 `sub_4885A0`；
2. 清 `role+0x34`；
3. 清 `role+0x38`。

`role+0x38` 为零时不调用释放器，也不改这两个字段。SDL owner 通过 `LegacyWorldStoryVmPorts::release_role_path_payload(role_index)` 对 `world_path_script_state_.role_label_payloads[index]` 执行空 vector swap；VM 随后按机器顺序清两个 32 位镜像字段。

### 固定 72 槽扫描

`0x0042875E..0x00428785` 从 `word_4AD490` 开始，以 `0x21C` 步长扫描到 `0x004B6C70`，总计 72 个 active object slots。比较键是槽 `+0x00` 的 `u16 role_index`；只处理第一个匹配槽。

匹配槽 type 为 `slot[0x1B] & 0x0F`：

- type 2：只把 `+0x08/+0x0A/+0x0C/+0x0E` 四个 `u16` 写成 `0xFFFF`，不 reset 整槽；
- type 1：进入对齐/空间分支，最终调用 `sub_40DD40(slot)` reset 整个 `0x21C` 槽；
- 其他 type：不改槽。

### Type 1 反向对齐与重插

只有 `role.world_x` 或 `role.world_y` 的低 4 位非零时才执行对齐。机器先保存旧的 `(world_y >> 4) - 1`，再在角色 flags bit14 未置位时调用 `sub_40AE20(role)` 清旧 surface occupancy。

方向字节来自：

```text
slot[0x1C + (u16(slot+0x02) & 0x7FFF)]
```

八方向步长表由 `0x0049963C/0x0049965C` 给出：

```text
dx = [ 4,  0, -4, -4, -4,  0,  4,  4]
dy = [ 4,  4,  4,  0, -4, -4, -4,  0]
```

对每个未对齐轴，原程序反复执行 `coordinate -= step[direction]`，直到低 4 位为零。因此负步长会增加坐标；它不是既有 role-map 顺向 snap helper 的语义。

随后 `0x00428870..0x00428886` 调用：

```text
sub_411530(role.guid, role.flags & 3, saved_first_row, 0)
```

`sub_411530` 内部只在第四参数等于 1 时跳过重插；现场 literal 0 会移除旧链并按新坐标重新插入。现代调用对应 `relocate_legacy_role_spatially_by_guid(..., reinsert=true)`，并与原程序一样忽略返回值。

### 角色字段提交

object 协调结束后，`0x004288A6..0x004288BF`：

- `role.path_data_id`（`+0x1C`）=`u16(+4)`；
- `role.path_word_index`（`+0x18`）=`0`；
- `role.flags`（`+0x10`）OR `0x1000`。

## Missing-role fallback

`0x004288C7..0x00428907` 调用 11 参数 `sub_40D460`。唯一业务修改为：

- selector/GUID=`u16(+2)`；
- Path id=`u16(+4)`；
- flags OR mask=`0x1000`；
- 其余 action、variant、位置、talk、AND mask 与 logical map 参数均为 `0xFFFF` preserve sentinel。

现代实现复用 `LegacyMapsRolePatchRequest` 与 SDL active/pending world 的 typed MAPS database owner。若 opcode 27 在同一次 VM 调用中刚完成同步 world reload，patch 会选择 pending session，使本 handler 仍观察并修改新 world。

## IP、previous opcode 与让出

两支都跳到 `0x0042BEE9`：

```asm
0042BEE9  mov ebx, [esp+var_50]
0042BEED  add ebx, 6
0042BEF0  add word ptr [ebp], 6
0042BEF5  mov [esp+var_50], ebx
0042BEF9  jmp 0042B0AE
```

该 handler 没有把 `ESI` 置 1。`0x0042B0AE` 发布 effective opcode 到 `dword_4CF6D8` 后，以 `ESI=0` 跳到 `0x0042D4D7`，调用 `_AIL_serve` 并返回 1。现代结果因此必须是：

- IP 增加 6；
- `previous_opcode=28`；
- 直接 audio service 一次；
- `LegacyWorldStoryVmStatus::yielded`；
- 同次 VM 调用不读取下一指令。

## 真实资产

锁定线性记录 inventory 中共有 45 条 effective opcode 28：

| 文件 | 记录数 |
| --- | ---: |
| `TALK1.DAT` | 7 |
| `TALK2.DAT` | 27 |
| `TALK3.DAT` | 2 |
| `TALK4.DAT` | 9 |

全部记录：

- raw word 都是 `0x001C`；
- decoded length 都是 6；
- entry probe 都命中，合计 45；
- selector 共 36 个不同值，范围 `2..7021`；
- Path id 共 30 个不同值，范围 `0..652`。

独立 live-role 回放使用 `TALK2.DAT@0x0001938D`：`(28, 2, 30)`。另一个真实链 `TALK2.DAT@0x00010C93` 从 opcode 19 同调用继续到 `(28, 102, 102)`，验证 missing-role MAPS patch 与 yield。

## 现代验证

Synthetic 覆盖包括：

- 四个 raw alias 归一化；
- live-role 与 missing-role 两支；
- payload 非零才释放以及 `+0x34/+0x38` 清零；
- type 2 只清四个 link words；
- type 1 已对齐直接 reset；
- 正步长反向减法、负步长反向减法、bit14 跳过 surface clear；
- spatial relocation 的重插路径；
- 非法方向 checked failure；
- live-role 与 missing-role `+4` 延迟读取的不同副作用顺序；
- +6、previous opcode、audio service 和跨帧 yield。

Real replay 覆盖独立 live-role 记录及 opcode19→opcode28 missing-role 链。Linux 定向 CTest 三项均通过：

- `world_map.legacy_world_story_vm`
- `world_map.legacy_world_story_vm_real`
- `world_map.legacy_world_story_vm_initial_session_real`

原程序动态 differential 仍依赖不可用的原始 runtime oracle；闭环依据为完整 LST→typed C++→LST 静态收敛、synthetic 边界测试和真实资产离线回放。
