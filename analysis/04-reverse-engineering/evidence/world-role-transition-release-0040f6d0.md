# 世界角色过渡清理闭环

状态：`platform_adapted`、`assembly_exact`（有效角色与路径域）、`unit_tested`；原程序动态
差分仍为 `blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst` 的 `0x0040F6D0..0x0040F885`，以及唯一调用者
`sub_40A570:0x0040A682..0x0040A6AF`。该链在主过渡发生时逐个结束角色路径，校正未落在
16 像素网格上的非受控角色，恢复动作覆盖字段，并清除角色的过渡状态。

## 1. ABI、物理布局与调用域

`sub_40F6D0` 是一个 `cdecl` 单参数函数，参数为角色索引。角色表起点为 `0x004BABA8`，
记录步长为 `0xD8`；索引乘法序列 `index * 3 * 9 * 8` 与
`LegacyWorldRoleRecord` 的静态布局完全一致。函数直接访问：

| 角色偏移 | C++ 字段 | 汇编用途 |
|---:|---|---|
| `+0x04/+0x08` | `world_x/world_y` | 低四位对齐判断及坐标校正 |
| `+0x10` | `flags` | bit 31 入口门、bit 11 动作恢复、掩码清理 |
| `+0x24` | `guid` | `sub_40AE20` 表面占用清理所需角色字段 |
| `+0x26` | `interaction_gate` | 唯一调用者在每次调用后清零 |
| `+0x40` | `action` | 内嵌 `0x98` 字节动作记录 |

动作字段 `+0x08/+0x20/+0x34/+0x3C/+0x44/+0x48` 分别对应
`base_variant`、`one_shot_base_variant`、`variant_delta`、
`one_shot_variant_delta`、`wait_remaining`、`wait_override`；这些偏移均由
`static_assert` 固定。

唯一调用者先读取 `dword_49E0C4`，从索引 1 开始以有符号比较处理
`1..count-1`，每次调用后把该角色 `+0x26` 写零。循环内虽会重读 count，但本函数及
`sub_42D920` 都不改变角色表大小；现代 owner 的固定 span 在有效单线程调用域等价。

## 2. 72 槽扫描与坐标校正

入口仅在角色 `flags` bit 31 置位时进入主体；否则立即从物理函数返回，随后仍由调用者
清零 `interaction_gate`。主体从 `0x004AD490` 扫描到偏移 `0x97E0`，步长 `0x21C`，
即严格扫描 72 个活动对象槽。只有槽 `+0x00` 的 16 位角色索引匹配时才执行：

1. 若角色 `world_x` 或 `world_y` 的低四位非零，调用 `sub_40AE20` 清除旧表面占用；
2. 若该角色不是 `dword_4AB378` 指定的受控角色，读取
   `slot[0x1C + (word(slot+2) & 0x7FFF)]` 的方向字节；
3. X 步长表为 `[4, 0, -4, -4, -4, 0, 4, 4]`，Y 步长表为
   `[4, 4, 4, 0, -4, -4, -4, 0]`；各轴反复减去对应步长，直至低四位为零；
4. 无论坐标原本是否对齐，每个匹配槽都执行 `flags &= 0xBBFFFFFF`，并把动作记录
   `wait_override` 写零。

受控角色分支会跳过方向字节读取和两轴校正，因此只要坐标仍未对齐，每个匹配槽都会再次
调用表面清理。定向测试使用越界 cursor 固定了该跳转：两个匹配槽产生两次清理、零次
方向读取效果，坐标保持不变，公共掩码和 wait 写入仍执行。

## 3. 路径与动作收尾

扫描完 72 槽后，无论是否找到匹配槽，均按以下顺序执行：

1. 调用 `sub_42D920(role_index)` 完成或恢复链式剧情路径；调用者不消费其机器返回值；
2. 若角色 `flags` bit 11 置位，则把非 `-1` 的 `one_shot_base_variant` 和
   `one_shot_variant_delta` 分别复制回稳定字段；
3. 把两个 one-shot 字段都写为 `0xFFFFFFFF`；
4. 调用 `sub_4321E0(action)` 更新动作记录；返回零只进入原版空日志函数，不改变后续流程；
5. 再次把两个 one-shot 字段写为 `0xFFFFFFFF`，清除 flags bit 31，把
   `wait_remaining` 写零；
6. 返回唯一调用者后，把 `interaction_gate` 写零。

现代接口保留这套无条件收尾：路径端口或动作更新失败只记录首个诊断状态，不会跳过原汇编
后续写入。

## 4. 平台隔离与双向收敛

原函数信任角色索引、槽 cursor、方向字节、静态表和全局 owner；损坏输入可越界，方向
步长为零而坐标未对齐时也会无限循环。现代实现只在无效角色、cursor、方向或 owner
失败时报告隔离状态；正常域的循环、掩码、写入宽度和调用顺序保持原样，未修复上述原始
逻辑行为。

收敛检查已从两个方向重复执行：

- LST→C++：覆盖 bit 31 早退、72 槽边界、16 位槽索引、cursor 掩码、受控角色跳转、
  两张八项步长表、两个逐轴循环、匹配槽公共写入、全部 helper 调用及最终写入；
- C++→LST：逐项反查每个正常域分支和副作用；越界状态、span/RAII owner 与诊断结果是
  明确的平台隔离，没有伪造游戏逻辑；
- 独立 UT 覆盖早退仍清 gate、两个普通匹配槽、只在首次未对齐槽移动、受控角色跳过方向
  读取、bit 11 动作恢复、无匹配槽、helper 失败仍执行最终写入；
- 最后一轮从入口重新正向与反向追溯后，没有未决控制流、字段宽度、掩码、调用次序或
  正常域状态差异；Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app`
  189/189 CTest 全部通过，两端应用成功链接且未启动任何 EXE。

核心实现为 `legacy_world_role_lifecycle.cpp::release_legacy_world_role_transition`；主过渡循环
位于 `frame_preparation.cpp`，SDL 实际 owner 接线位于
`main.cpp::release_and_clear_world_role_transition`。
