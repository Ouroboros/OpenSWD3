# 剧情 VM 角色路径碰撞绕过 `0x0042CF7C`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`external_dependency_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042CF7C..0x0042CFBC`，共享推进尾`0x0042D182..0x0042D193`

opcode：178 / `OP_178_SET_ROLE_COLLISION_BYPASS`

## 1. operand与lookup

物理记录固定4字节：

```text
+0 u16 opcode
+2 u16 role selector
```

handler先读取selector低16位。只有字面`0xFFF0`在调用helper前替换为受控角色index的低word；替换值不直接当作role index，而是继续作为ordinary GUID selector查找。因此controlled index为2时，`FFF0`命中的是首个合法GUID 2角色，不一定是受控角色本身。

共享`sub_40C0D0`保持自身规则：

- `0xFFFE`直接返回受控角色index；
- ordinary selector从角色表查首个相同u16 GUID；
- flags bit28置位的记录跳过；
- miss返回false，handler静默继续。

机器caller以`mov bx`留下的高16位不参与helper语义；helper比较、ordinary查找和GUID字段均使用低16位。现代使用u16 selector精确承担该合同。

## 2. 状态写入与控制流

lookup命中时，机器对实际角色记录`+0x10`执行：

```text
role.flags |= 0x00040000
```

该bit18由剧情路径owner作为collision bypass使用：路径finding与方向占用probe在位已置时跳过碰撞掩码，普通世界角色路径到达链随后按自身合同清除该位。`LegacyWorldRoleRecord::flags`是实际完整u32 owner，现代直接OR同一mask；其余31位保持，重复执行幂等。

lookup miss不写角色、不发MAPS patch、不诊断。成功与miss都进入共享尾：物理指针和u16 IP固定+4，`ESI=1`，common join发布normalized previous178并在同一VM调用读取后继；不service audio、不yield。

selector截断时modern在原`[script+2]`读取点返回`operand_out_of_range`，无lookup、flags、IP或previous副作用。完整记录从`IP=0x7FFC`开始时，flags、IP=`0x8000`和previous178先提交，再由same-call下一fetch返回`instruction_out_of_range`。

所有合法owner已由现有typed角色span承接，无nullable、I/O、分配或平台失败边界，故分类为`assembly_exact`。

## 3. 资产锁与验证

线性TALK目录锁定6条物理记录/6 probes，全部raw `0x00B2`、固定长度4：

```text
TALK1/2/3/4 = 1/0/3/2
selectors = 2x2, 104x2, 323x1, 919x1
```

逐条回放：

```text
TALK1.DAT@0x0003FB3D  selector 323
TALK3.DAT@0x0001B471  selector 2
TALK3.DAT@0x0001B485  selector 2
TALK3.DAT@0x00033812  selector 919
TALK4.DAT@0x00018AF1  selector 104
TALK4.DAT@0x00018B80  selector 104
```

六条均置于`IP=0x7FFC`，命中对应GUID角色、设置bit18、推进至`0x8000`并发布previous178，再由下一fetch越界；无audio。全文件双字节候选计数为00B2 80处、40B2 5处、80B2 0处、C0B2 4处；除上述6条00B2外均非线性入口。

synthetic覆盖四raw alias、完整u32其他位保持、已置幂等、ordinary miss、FFF0替换后非shortcut查找、helper-native FFFE、bit28跳过与首个合法重复GUID、selector截断和精确窗口尾。role lookup依赖与Story VM synthetic/real/initial-session共4/4通过，SDL app编译通过。Linux core完整门186/186、app完整门192/192通过。未启动原版或OpenSWD3游戏EXE。
