# 剧情 VM 角色头像动作创建 `0x0042A200`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为`blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A200..0x0042A2C1`

opcode：`81`

## 1. 节点与operand合同

handler先`malloc(0xB4)`、清45个dword并调用`sub_40DC00`，再依次读取：

```text
+2 u16 action id      -> node+0x00 dword zero-extend
+4 u16 base variant   -> node+0x08 dword zero-extend
+6 u16 target X       -> node+0x9C word，后续按i16比较/消费
+8 u16 encoded Y      -> low15写node+0x9E，bit15控制特殊路径
```

默认写`current_x=-120`、`horizontal_motion=0`；若signed target X严格大于320，则`current_x=760`。target X等于320或负数都从-120开始。

若encoded Y bit15置位，覆盖默认路径：`current_x=target_x`，`horizontal_motion=0x8000`；Y仍只保留low15。最后节点前插到`dword_4BA6E0`，推进10、common join发布normalized previous81并same-call继续。

## 2. 消费者与位语义

既有`LegacyRoleHeadActionNode`精确大小0xB4，字段offset为`+98/+9A/+9C/+9E/+B0`。`sub_414CE0`每帧更新/绘制action：

- `(horizontal_motion & 0x7FFF)==0`时，以signed X距离的`2/3`向目标收敛；步长在[-1,1]时吸附目标；
- 否则先以word wrapping加motion，再把motion以word wrapping乘3；X到`<=-120`或`>=760`时释放。

因此bit15特殊值`0x8000`不是Y的一部分，也不是普通零motion；它进入第二条wrapping离场路径。现代创建case保留raw i16位型。

## 3. 失败顺序与平台适配

原版分配不检查null。现代用临时`LegacyRoleHeadActionList::emplace_front`，聚合零初始化后调用action initializer；`bad_alloc/length_error`映射`role_head_action_allocation_failed`。四个operand逐word staged读取，任一截断都会销毁未链接节点并保持IP/previous。

固定全局链表映射为nullable`runtime.role_head_actions`；owner在全部operand和字段初始化后的原始链接点首次检查，缺失返回`runtime_unavailable`。成功以`splice(begin)`前插；host list链接替代裸指针，legacy `next_pointer_32`保持零。SDL早已拥有`world_role_head_actions_`，逐帧更新、场景重置和退出释放均复用既有生命周期。

## 4. alias、窗口与资产锁

四raw alias `0051/4051/8051/C051`都归一为81。`0x7FF6`精确尾先完成节点前插、IP=`0x8000`和previous81，下一fetch才失败。ordinary路径同调用继续到opcode14。

线性TALK目录含1888条物理记录/1888 probes：

```text
TALK1.DAT 488
TALK2.DAT 340
TALK3.DAT 398
TALK4.DAT 662
```

全部raw `0x0051`、长度10；984条target X按i16严格大于320。仅3条bit15特殊记录，全部位于TALK2：`0x0000EBAC`、`0x0000F181`、`0x0000F1F5`。

real CTest回放`TALK1.DAT@0x000060D2`（target560/Y460，默认current760）与`TALK2.DAT@0x0000EBAC`（target50/encodedY8078，current50/motion8000）。

## 5. 测试与分类

覆盖四alias精确尾、signed X=320/321/-1、bit15特殊路径、前插/same-call、四截断点、owner缺失及两条真实记录。剧情VM synthetic/real/initial-session-real为3/3。

分类：`platform_adapted`。合法域字段、signed/low15/bit15语义、链接时点、+10、previous和same-call保持；仅将unchecked裸分配、固定全局与裸链指针收敛为typed list owner。
